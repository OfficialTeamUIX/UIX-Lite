// net.h: the stack's socket calls (public thunks in the XBE) plus ours.

#pragma once
#include "dash5960.h"

typedef DWORD SOCKET;
#define INVALID_SOCKET ((SOCKET)-1)

typedef struct sockaddr_in
{
	short sin_family;
	WORD  sin_port;
	DWORD sin_addr;
	char  sin_zero[8];
} sockaddr_in;

typedef struct fd_set
{
	DWORD  fd_count;
	SOCKET fd_array[64];
} fd_set;

typedef struct timeval
{
	int tv_sec;
	int tv_usec;
} timeval;

typedef struct WSABUF
{
	DWORD len;
	char* buf;
} WSABUF;

#define socket      ((SOCKET (STDCALL *)(int, int, int))0x00130789)
#define closesocket ((int (STDCALL *)(SOCKET))0x00130794)
#define shutdown    ((int (STDCALL *)(SOCKET, int))0x0013079f)
#define ioctlsocket ((int (STDCALL *)(SOCKET, int, DWORD*))0x001307aa)
#define setsockopt  ((int (STDCALL *)(SOCKET, int, int, const char*, int))0x001307b5)
#define bind        ((int (STDCALL *)(SOCKET, const sockaddr_in*, int))0x001307c4)
#define connect     ((int (STDCALL *)(SOCKET, const sockaddr_in*, int))0x001307d5)
#define select      ((int (STDCALL *)(int, fd_set*, fd_set*, fd_set*, const timeval*))0x001307da)
#define WSARecv     ((int (STDCALL *)(SOCKET, WSABUF*, DWORD, DWORD*, DWORD*, void*, void*))0x00130803)
#define WSASend     ((int (STDCALL *)(SOCKET, WSABUF*, DWORD, DWORD*, DWORD, void*, void*))0x00130812)
#define XNetGetTitleXnAddr ((DWORD (STDCALL *)(BYTE*))0x0013077e) // first DWORD of the XNADDR is our IP

int    net_listen(SOCKET s, int backlog);
SOCKET net_accept(SOCKET s);
int    net_allow_insecure(SOCKET s);
int    net_ready(void);
DWORD  net_address(void);
