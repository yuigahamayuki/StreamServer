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
	int len = sizeof(_clientSockAddr);	// *** 必须初始化长度，不能初始化为0
	int receive_len = recvfrom(_sockfd, file_name, sizeof(file_name), 0, (sockaddr*)&_clientSockAddr, &len);

	// 赋值format context
	int ret = avformat_open_input(&_formatContext, file_name, nullptr, nullptr);
	if (ret < 0)
	{
		char error_str[64];
		av_strerror(ret, error_str, 64);
		av_log(NULL, AV_LOG_ERROR, "error: %s\n", error_str);
	}


	ret = avformat_find_stream_info(_formatContext, NULL);	// 改变了format context字段stream里codec的time_base, ticks_per_frame, coded_width, coded_height(1088), pix_fmt
	int video_idx = -1;		// 找到第一个视频流在整个流的index
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
	int size_expected = writeBuffer.sizeWritten();	// 期望发送的大小
	int write_size = sendto(_sockfd, writeBuffer.buf(), size_expected, 0, (const sockaddr*)&_clientSockAddr, sizeof(_clientSockAddr));
}


// 往Buffer写入视频解码器的参数
void StreamServer::writeVideoDecoderParameters(WriteBuffer& writeBuffer, const AVCodecParameters * decoderPara)
{
	writeBuffer.writeInt8(decoderPara->codec_type);		// media type  --> audio or video or others
	writeBuffer.writeUInt32(decoderPara->codec_id);		// codec_id
	writeBuffer.writeUInt32(decoderPara->codec_tag);		
	writeBuffer.writeUInt32(decoderPara->extradata_size);
	writeBuffer.writeChunck(decoderPara->extradata, decoderPara->extradata_size);
	writeBuffer.writeInt16(decoderPara->format);			// 对video, 像素格式； 对audio，采样格式
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
