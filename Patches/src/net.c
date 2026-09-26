// net.c: listen / accept for the dashboard's network stack.
//
// The retail XBE links socket, bind, select and WSARecv/WSASend, but not
// listen or accept, since the dashboard never serves anything. The TCP code
// underneath is all there: a socket in state 1 answers SYNs by cloning a
// child socket onto its queue, and select already reports a listener with a
// connected child as readable. These two just drive that state.
//
// Sockets from here are the stack's own, so every other call works on them.

#include "dash5960.h"
#include "net.h"

#define net_object        (*(BYTE**)0x00179df0)
#define Socket_Lock       ((BYTE* (STDCALL *)(SOCKET))0x0013dabf)
#define Tcp_ConnectedChild ((BYTE* (FASTCALL *)(BYTE*))0x0014089c)
#define Tcp_DequeueChild  ((void (THISCALL *)(BYTE*, BYTE*))0x00140255)
#define KeRaiseIrqlToDpcLevel KIMPORT(BYTE (STDCALL *)(void), 0x00012100)
#define KfLowerIrql       KIMPORT(void (FASTCALL *)(BYTE), 0x000120fc)
#define SetLastError      ((void (STDCALL *)(DWORD))0x0006cd3e)

#define SOCK_MAGIC        0x2b434f53 // "SOC+", unlocked
#define SOCK_FLAGS(s)     (*(DWORD*)((s) + 0x0c))
#define SOCK_TCP          0x02
#define SOCK_BOUND        0x20
#define TCP_STATE(s)      (*(BYTE*)((s) + 0x7c)) // 0 closed, 1 listening
#define TCP_BACKLOG(s)    (*(BYTE*)((s) + 0x7e))

#define WSAEINVAL         10022
#define WSAEWOULDBLOCK    10035
#define WSAEOPNOTSUPP     10045

static void unlock(BYTE* s)
{
	*(DWORD*)(s + 8) = SOCK_MAGIC;
}

int net_listen(SOCKET h, int backlog)
{
	BYTE* s = Socket_Lock(h);
	if (!s)
		return -1;
	int err = 0;
	if (!(SOCK_FLAGS(s) & SOCK_TCP))
		err = WSAEOPNOTSUPP;
	else if (!(SOCK_FLAGS(s) & SOCK_BOUND) || TCP_STATE(s) > 1)
		err = WSAEINVAL;
	else
	{
		if (backlog < 1)
			backlog = 1;
		if (backlog > 8)
			backlog = 8;
		BYTE irql = KeRaiseIrqlToDpcLevel();
		TCP_BACKLOG(s) = (BYTE)backlog;
		TCP_STATE(s) = 1;
		KfLowerIrql(irql);
	}
	unlock(s);
	if (err)
	{
		SetLastError(err);
		return -1;
	}
	return 0;
}

// Non-blocking: select the listener for read first
SOCKET net_accept(SOCKET h)
{
	BYTE* s = Socket_Lock(h);
	if (!s)
		return INVALID_SOCKET;
	BYTE* child = 0;
	int err = 0;
	if (TCP_STATE(s) != 1)
		err = WSAEINVAL;
	else
	{
		BYTE irql = KeRaiseIrqlToDpcLevel();
		child = Tcp_ConnectedChild(s);
		if (child)
			Tcp_DequeueChild(s, child);
		KfLowerIrql(irql);
		if (!child)
			err = WSAEWOULDBLOCK;
	}
	unlock(s);
	if (err)
	{
		SetLastError(err);
		return INVALID_SOCKET;
	}
	return (SOCKET)child;
}

// Let a socket take plain LAN traffic. The stack only allows it for callers
// holding its online object's pointer; accepted children inherit the flag.
int net_allow_insecure(SOCKET h)
{
	BYTE* net = net_object;
	if (!net || !*(DWORD*)(net + 0xaec))
		return -1;
	return setsockopt(h, 0xffff, 0x4001, (const char*)*(DWORD*)(net + 0xaec), 4);
}

int net_ready(void)
{
	BYTE* net = net_object;
	return net && *(DWORD*)(net + 0xaec);
}

// Our LAN address (network order), or 0 until DHCP or a static IP is up
DWORD net_address(void)
{
	BYTE xnaddr[36];
	for (int i = 0; i < 36; i++)
		xnaddr[i] = 0;
	if (XNetGetTitleXnAddr(xnaddr) == 0)
		return 0;
	return *(DWORD*)xnaddr;
}
