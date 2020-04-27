#include "StreamServer.h"


int main(int argc, char** argv)
{
	av_log_set_level(AV_LOG_ERROR);
	StreamServer server;
	server.start();
	return 0;
}