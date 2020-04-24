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

#include "WriteBuffer.h"

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

	SOCKET _sockfd;
	sockaddr_in _clientSockAddr;
	AVFormatContext* _formatContext;
};