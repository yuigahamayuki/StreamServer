#include "WriteBuffer.h"
#include <string.h>

WriteBuffer::WriteBuffer()
	: start_pos(_buffer), insert_pos(_buffer)
{
	memset(_buffer, 0, sizeof(_buffer));
}

void WriteBuffer::writeChunck(const void * src, size_t size)
{
	if (sizeWritable() >= size)
	{
		memcpy(insert_pos, src, size);
		insert_pos += size;
	}
}


