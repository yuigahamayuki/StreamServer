/*
	����ָʾ������
*/

#pragma once

class Status
{
public:
	Status() : _status(STATUS_SEND_PACKET)
	{

	}

	Status(const Status&) = delete;
	Status& operator=(const Status&) = delete;

	typedef enum
	{
		STATUS_SEND_PACKET = 0,		// ���Է���AVPacket
		STATUS_SEND_PAUSE,			// ��ͣ����AVPacket
	} eStatus;

	void setStatus(eStatus status)
	{
		_status = status;
	}

	eStatus getStatus()
	{
		return _status;
	}
private:
	eStatus _status;
};
