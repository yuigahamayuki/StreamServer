# StreamServer

## 概述
此服务器程序StreamServer与项目中另一客户端程序StreamPlayer搭配，构成了Windows/Linux上的视频点播软件。此视频点播软件的原理并不是基于HTTP的文件下载，而采用了流媒体传输的方式。   
网络传输使用的是UDP。    
视频文件的读取和demux使用的是FFmpeg。
## 服务器主要原理
服务器打开需播放的视频文件，demux视频流，将视频解码器的解码参数发送给客户端，以让客户端构造解码器上下文。接着服务器不断将编码的AVPacket包（对于H.264来说就是NALU）发给客户端，让客户端解码并渲染。  
## 服务器大致流程
1.  阻塞等待客户端发来需要播放的视频的文件名。
2.  打开视频文件，读取视频流，将解码器的解码参数发给客户端，然后进入发送AVPacket的循环。
3.  循环发送每一个AVPacket，只有当成功发送完1个AVPacket，才发送下一个。
## 服务器技术细节
一个AVPacket的size可能非常大，超过MTU，所以进行了分片，一个UDP的数据部分大小设成了1400字节。一个UDP包除了AVPacket数据部分，每个包带有固定的头，包括序列号(Sequnce Number)，分片总数(Fragment Count），分片号（Fragment Number）。属于同一个AVPacket的UDP的序列号相同，但分片号不同。  
服务器发送完一个AVPacket的所有分片后，会阻塞等待客户端的回应。如果有分片丢失，客户端会请求该分片号的再次发送，服务器会将该分片再次发送，直到全部分片发送成功收到客户端的ACK，才进行下一个AVPacket的传输。
