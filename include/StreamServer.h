/*
	�������Ҫ�����Ǹ��ͻ��˷���AVPacket��
	�ڿ�ʼʱ�ᷢ�ͽ�������һЩ����

*/

#pragma once

#include <WinSock2.h>

extern "C"
{
#include <libavformat/avformat.h>
};

#include <vector>
#include "WriteBuffer.h"
#include "Status.h"

struct AVPacket;

class StreamServer
{
public:
	StreamServer();
	~StreamServer();
	void start();
private:
	void initSock();
	void receiveAndSendDecoderParameters();
	void writeVideoDecoderParameters(WriteBuffer& writeBuffer, const AVCodecParameters* decoderPara);
	void loop();
	// ����AVPacket�Ĳ��������ݣ����һ��packet���󣬻���зְ�
	void setPacketDataBufferAndSend(AVPacket* packet, std::vector<WriteBuffer>& bufferVec);
	// ʵ�ʷ���AVPacket
	void sendPacketDataCore(const std::vector<WriteBuffer>& bufferVec);
	// �ж��Ƿ��ͳɹ�
	bool isSendSucceed();

	Status _serverStatus;
	SOCKET _sockfd;
	sockaddr_in _clientSockAddr;
	AVFormatContext* _formatContext;
	// ���͵�packet�����
	unsigned int _packetCnt = 0;
	// ��Ƶ����idx
	int _videoStream = -1;
};