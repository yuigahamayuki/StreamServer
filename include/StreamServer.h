/*
	这个类主要功能是给客户端发送AVPacket。
	在开始时会发送解码器的一些数据

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
	// 发送AVPacket的参数与数据，如果一个packet过大，会进行分包
	void setPacketDataBufferAndSend(AVPacket* packet, std::vector<WriteBuffer>& bufferVec);
	// 实际发送AVPacket
	void sendPacketDataCore(const std::vector<WriteBuffer>& bufferVec);
	// 判断是否发送成功
	bool isSendSucceed();

	Status _serverStatus;
	SOCKET _sockfd;
	sockaddr_in _clientSockAddr;
	AVFormatContext* _formatContext;
	// 发送的packet的序号
	unsigned int _packetCnt = 0;
	// 视频流的idx
	int _videoStream = -1;
};