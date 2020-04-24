/*
	Buffer
	不同操作系统可能存在字节序问题

*/

#pragma once

#include <stdint.h>

class WriteBuffer
{
public:
	WriteBuffer();

	size_t sizeWritten() { return insert_pos - start_pos; }
	size_t sizeWritable() { return buffer_length - sizeWritten(); }

	const char* buf() { return start_pos; }

	void writeUInt8(uint8_t val) 
	{
		if (sizeWritable() >= sizeof(uint8_t))
		{
			*insert_pos = static_cast<char>(val);
			insert_pos++;
		}
	}

	void writeInt8(char val)
	{
		if (sizeWritable() >= sizeof(char))
		{
			*insert_pos = val;
			insert_pos++;
		}
	}

	void writeUInt16(uint16_t val)
	{
		if (sizeWritable() >= sizeof(uint16_t))
		{
			uint16_t *addr = reinterpret_cast<uint16_t*>(insert_pos);
			*addr = val;
			insert_pos += sizeof(uint16_t);
		}
	}

	void writeInt16(int16_t val)
	{
		if (sizeWritable() >= sizeof(int16_t))
		{
			int16_t *addr = reinterpret_cast<int16_t*>(insert_pos);
			*addr = val;
			insert_pos += sizeof(int16_t);
		}
	}

	void writeUInt32(uint32_t val)
	{
		if (sizeWritable() >= sizeof(uint32_t))
		{
			uint32_t *addr = reinterpret_cast<uint32_t*>(insert_pos);
			*addr = val;
			insert_pos += sizeof(uint32_t);
		}
	}

	void writeInt32(int32_t val)
	{
		if (sizeWritable() >= sizeof(int32_t))
		{
			int32_t *addr = reinterpret_cast<int32_t*>(insert_pos);
			*addr = val;
			insert_pos += sizeof(int32_t);
		}
	}

	void writeUInt64(uint64_t val)
	{
		if (sizeWritable() >= sizeof(uint64_t))
		{
			uint64_t *addr = reinterpret_cast<uint64_t*>(insert_pos);
			*addr = val;
			insert_pos += sizeof(uint64_t);
		}
	}

	void writeInt64(int64_t val)
	{
		if (sizeWritable() >= sizeof(int64_t))
		{
			int64_t *addr = reinterpret_cast<int64_t*>(insert_pos);
			*addr = val;
			insert_pos += sizeof(int64_t);
		}
	}

	void writeChunck(const void* src, size_t size);		// 写连续内存

private:
	static const size_t buffer_length = 1024;	// FIXME: 可能更改
	char _buffer[buffer_length];
	const char* start_pos;
	char* insert_pos;
};