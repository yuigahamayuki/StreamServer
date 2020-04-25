#include "StreamServer.h"

extern "C"
{
#include <libavutil/log.h>
};


#pragma comment(lib, "ws2_32.lib")

StreamServer::StreamServer()
	: _sockfd(0), _formatContext(nullptr)
{
	memset(&_clientSockAddr, 0, sizeof(_clientSockAddr));

	av_log_set_level(AV_LOG_VERBOSE);
}

StreamServer::~StreamServer()
{
	closesocket(_sockfd);
	WSACleanup();


	avformat_close_input(&_formatContext);

}

void StreamServer::start()
{
	initSock();

	receiveAndSendDecoderParameters();

	loop();
}

void StreamServer::initSock()
{
	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
		//std::cout << "Failed to link winsock.dll!" << std::endl;
		exit(1);
	}


	_sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	sockaddr_in serverSockAddr;
	memset(&serverSockAddr, 0, sizeof(serverSockAddr));
	serverSockAddr.sin_family = AF_INET;
	serverSockAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serverSockAddr.sin_port = htons(6666);

	int ret = bind(_sockfd, (const sockaddr*)&serverSockAddr, sizeof(serverSockAddr));
	if (ret < 0)
	{
		//av_log(nullptr, AV_LOG_ERROR, "bind failed\n");
		exit(1);
	}
}

void StreamServer::receiveAndSendDecoderParameters()
{
	//const char* file_name = "D:\\Movie\\[Mabors-Sub][Kono Subarashii Sekai ni Shukufuku o! Kurenai Densetsu][Movie][1080P][CHS&JPN][BDrip][AVC AAC YUV420P8].mp4";
	char file_name[256];
	memset(file_name, 0, sizeof(file_name));
	int len = sizeof(_clientSockAddr);	// *** �����ʼ�����ȣ����ܳ�ʼ��Ϊ0
	int receive_len = recvfrom(_sockfd, file_name, sizeof(file_name), 0, (sockaddr*)&_clientSockAddr, &len);

	// ��ֵformat context
	int ret = avformat_open_input(&_formatContext, file_name, nullptr, nullptr);
	if (ret < 0)
	{
		char error_str[64];
		av_strerror(ret, error_str, 64);
		av_log(NULL, AV_LOG_ERROR, "error: %s\n", error_str);
	}


	ret = avformat_find_stream_info(_formatContext, NULL);	// �ı���format context�ֶ�stream��codec��time_base, ticks_per_frame, coded_width, coded_height(1088), pix_fmt
	int video_idx = -1;		// �ҵ���һ����Ƶ������������index
	for (int i = 0; i < _formatContext->nb_streams; ++i)
	{
		if (_formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_idx = i;
			break;
		}
	}

	WriteBuffer writeBuffer;
	writeVideoDecoderParameters(writeBuffer, _formatContext->streams[video_idx]->codecpar);
	int size_expected = writeBuffer.sizeWritten();	// �������͵Ĵ�С
	int write_size = sendto(_sockfd, writeBuffer.buf(), size_expected, 0, (const sockaddr*)&_clientSockAddr, sizeof(_clientSockAddr));

	_videoStream = video_idx;
}


// ��Bufferд����Ƶ�������Ĳ���
void StreamServer::writeVideoDecoderParameters(WriteBuffer& writeBuffer, const AVCodecParameters * decoderPara)
{
	writeBuffer.writeInt8(decoderPara->codec_type);		// media type  --> audio or video or others
	writeBuffer.writeUInt32(decoderPara->codec_id);		// codec_id
	writeBuffer.writeUInt32(decoderPara->codec_tag);		
	writeBuffer.writeUInt32(decoderPara->extradata_size);
	writeBuffer.writeChunck(decoderPara->extradata, decoderPara->extradata_size);
	writeBuffer.writeInt16(decoderPara->format);			// ��video, ���ظ�ʽ�� ��audio��������ʽ
	writeBuffer.writeUInt64(decoderPara->bit_rate);
	writeBuffer.writeUInt8(decoderPara->bits_per_coded_sample);		// Warn: overflow
	writeBuffer.writeUInt8(decoderPara->bits_per_raw_sample);			// Warn: overflow;
	writeBuffer.writeUInt16(decoderPara->profile);						// Warn: overflow
	writeBuffer.writeUInt16(decoderPara->level);						// Warn: overflow
	writeBuffer.writeUInt16(decoderPara->width);
	writeBuffer.writeUInt16(decoderPara->height);
	writeBuffer.writeUInt8(decoderPara->chroma_location);
	writeBuffer.writeUInt8(decoderPara->video_delay);
}

void StreamServer::loop()
{
	AVPacket* packet = av_packet_alloc();
	int ret = 0;

	while (1)
	{
		ret = av_read_frame(_formatContext, packet);
		if (ret < 0)
			break;

		if (packet->stream_index != _videoStream)		// FIXME: �Ժ������Ƶ��Ҫ��
			continue;

		std::vector<WriteBuffer> bufferVec;
		setPacketDataBufferAndSend(packet, bufferVec);
		
		while (!isSendSucceed())
		{
			sendPacketDataCore(bufferVec);
		}


		av_log(nullptr, AV_LOG_INFO, "packet cnt: %d\n", _packetCnt);
		av_log(nullptr, AV_LOG_INFO, "packet size: %d\n", packet->size);


		av_packet_unref(packet);
		
		_packetCnt++;
	}
}

/*
	һ��udp���ݰ������ݲ��ְ���ͷ��ʵ������
	udp�����ݳ������Ϊ1400�ֽڣ������޸ģ�
	ͷ(64 bits = 8�ֽ�)��
	sequence number(32 bits = uint32):	��ʾpacket����ţ���0��ʼ
	�ܹ��ķ�Ƭ��(16 bit = uint16)��		packet���󣬻���з�Ƭ�����ֵΪ1����û�з�Ƭ��ֻ����1����
	Ƭ���(16 bit = uint16)��				һ��packet��Ƭ����ţ�ȡֵ[0, ��Ƭ��-1]��0������Ƭ������data����Ĳ���

	Packet��������data�����ֳ����ڵ�һ����Ƭ
	һ�� 37 bytes
	stream_index(1 byte = uint8)
	pos(8 byte = int64)
	pts(8 byte = int64)
	dts(8 byte = int64)
	duration(8 byte = int64)
	size(4 byte = int32)

*/

void StreamServer::setPacketDataBufferAndSend(AVPacket* packet, std::vector<WriteBuffer>& bufferVec)
{
	// FIXME: Ŀǰֻ������Ƶ������������Ƶ������Ҫȥ����������ж�
	if (packet->stream_index != _videoStream)
		return;

	const size_t maxSize = WriteBuffer::sizeLength();	// ���UDP����
	const size_t headerSize = 8;						// �̶�ͷ�����ȣ�8�ֽ�
	const size_t packetParaSize = 37;					// packet����data���ֵĳ���
	const size_t remainingSizeFor1stBuf = maxSize - headerSize - packetParaSize;	// ��1����Ƭʣ��������data�ĳ���
	const size_t remainingSizeForOtherBuf = maxSize - headerSize;		// ���˵�1����Ƭ��������data�ĳ���

	size_t dataSize = packet->size;
	bool needFrag = false;		// �Ƿ���Ҫ��Ƭ��true��ʾҪ��Ƭ
	if (dataSize > remainingSizeFor1stBuf)
		needFrag = true;

	int frag_cnt = 1;		// ��Ƭ����
	if (needFrag)
	{
		size_t remainingDataSize = dataSize - remainingSizeFor1stBuf;
		auto x = remainingDataSize / remainingSizeForOtherBuf;

		if (remainingDataSize % remainingSizeForOtherBuf == 0)
		{
			frag_cnt += x;
		}
		else
		{
			frag_cnt += x + 1;
		}
	}

	//std::vector<WriteBuffer> bufferVec(frag_cnt);
	bufferVec.resize(frag_cnt);
	// ȫ����Ƭ��������send������������1������1������

	// ����ͷ
	for (uint16_t i = 0; i < bufferVec.size(); ++i)
	{
		bufferVec[i].writeUInt32(_packetCnt);
		bufferVec[i].writeUInt16(static_cast<uint16_t>(frag_cnt));
		bufferVec[i].writeUInt16(i);
	}

	// ���õ�1����Ƭ��packet����
	bufferVec[0].writeUInt8(packet->stream_index);
	bufferVec[0].writeInt64(packet->pos);
	bufferVec[0].writeInt64(packet->pts);
	bufferVec[0].writeInt64(packet->dts);
	bufferVec[0].writeInt64(packet->duration);
	bufferVec[0].writeInt32(packet->size);
	
	// дpacket������
	const uint8_t* ptr = packet->data;
	size_t bytesNeedToWrite = packet->size;

	// ��1����Ƭ
	if (!needFrag)
	{
		bufferVec[0].writeChunck(ptr, packet->size);
		bytesNeedToWrite -= packet->size;
	}		
	else
	{
		bufferVec[0].writeChunck(ptr, remainingSizeFor1stBuf);
		ptr += remainingSizeFor1stBuf;
		bytesNeedToWrite -= remainingSizeFor1stBuf;
	}
		
	int i = 1;		// д�ڼ���buffer����Ƭ�� 
	while (bytesNeedToWrite > 0)
	{
		size_t bytesWrittenToThisFrag = bytesNeedToWrite > remainingSizeForOtherBuf ? remainingSizeForOtherBuf : bytesNeedToWrite;	// �÷�ƬҪд����ֽ���

		bufferVec[i].writeChunck(ptr, bytesWrittenToThisFrag);

		ptr += bytesWrittenToThisFrag;

		i++;
		bytesNeedToWrite -= bytesWrittenToThisFrag;
	}
	

	sendPacketDataCore(bufferVec);
	
}

void StreamServer::sendPacketDataCore(const std::vector<WriteBuffer>& bufferVec)
{
	for (auto writeBuffer : bufferVec)
	{
		int expected_send_size = writeBuffer.sizeWritten();
		int actual_send_size = sendto(_sockfd, writeBuffer.buf(), expected_send_size, 0, (const sockaddr*)&_clientSockAddr, sizeof(_clientSockAddr));

		if (actual_send_size != expected_send_size)
		{
			auto pp = 1;
		}

		if (actual_send_size < 0)
		{
			av_log(nullptr, AV_LOG_ERROR, "send fail\n");
		}
	}
}

bool StreamServer::isSendSucceed()
{
	timeval tv;
	tv.tv_sec = 0, tv.tv_usec = 1000;		// FIXME: ��ʱֵ����Ҫ��
	fd_set readFDSet;
	FD_ZERO(&readFDSet);
	FD_SET(_sockfd, &readFDSet);
	int ret = select(_sockfd + 1, &readFDSet, nullptr, nullptr, &tv);

	if (ret < 0)
	{
		av_log(nullptr, AV_LOG_ERROR, "select error.\n");
		return false;
	}

	if (FD_ISSET(_sockfd, &readFDSet))
	{
		char buf = '\0';
		int read_size = recvfrom(_sockfd, &buf, sizeof(buf), 0, NULL, NULL);
		
		if (buf == 'a')
		{
			av_log(nullptr, AV_LOG_INFO, "send packet succeed.\n");
			return true;
		}

		if (buf == 'r')
		{
			av_log(nullptr, AV_LOG_INFO, "send packet failure.\n");
			return false;
		}
	}

	av_log(nullptr, AV_LOG_INFO, "send packet failure, time out.\n");
	return false;
}
