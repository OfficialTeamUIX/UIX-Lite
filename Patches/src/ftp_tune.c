#include "net.h"

#define SOL_SOCKET 0xffff
#define SO_RCVBUF  0x1002

int net_tune_ftp_data(SOCKET socket_handle)
{
	int size = 32 * 1024;
	int result = net_allow_insecure(socket_handle);
	// xemu testing found 32 KB faster and reliable. Larger receive windows
	// caused long delivery stalls or failure when the transfer reached 100%.
	setsockopt(socket_handle, SOL_SOCKET, SO_RCVBUF,
	           (const char*)&size, sizeof(size));
	return result;
}

SOCKET net_accept_ftp(SOCKET listener)
{
	SOCKET data = net_accept(listener);
	if (data != INVALID_SOCKET)
		net_tune_ftp_data(data);
	return data;
}

const char* ftp_stor_reply(int result)
{
	if (result > 0)
		return "226 Transfer complete.\r\n";
	if (result < 0)
		return "452 Insufficient storage space.\r\n";
	return "426 Transfer failed.\r\n";
}

