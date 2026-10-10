// discord.c: send a title ID to a Discord relay over UDP, TCP, or WebSocket.
//
// XAP entry points:
//   SendDiscordRelay(titleID)       blocking, 1 on delivery / 0 on failure
//   SendDiscordRelayAsync(titleID)  async for TCP/WebSocket; blocking for UDP
//   GetDiscordRelayStatus()         0 idle, 1 sending, 2 sent, negative error

#include "dash5960.h"
#include "net.h"
#include "uixlite.h"

#define AF_INET       2
#define SOCK_STREAM   1
#define SOCK_DGRAM    2
#define IPPROTO_TCP   6
#define SD_SEND       1
#define RELAY_TIMEOUT 10

typedef struct RelayJob
{
	char title[33];
	char ip[16];
	WORD port;
} RelayJob;

typedef struct WSAOVERLAPPED
{
	DWORD Internal;
	DWORD InternalHigh;
	DWORD Offset;
	DWORD OffsetHigh;
	HANDLE hEvent;
} WSAOVERLAPPED;

static volatile int s_status;
static volatile DWORD s_relayError;
static DWORD s_maskCounter = 0x12345678;
static SOCKET s_udpSocket = INVALID_SOCKET;
static HANDLE s_udpEvent;
static WSAOVERLAPPED s_udpOverlapped;
static sockaddr_in s_udpAddress;
static WSABUF s_udpBuffer;
static char s_udpMessage[48];

static int wait_network(int seconds)
{
	for (int i = 0; i < seconds * 10; i++)
	{
		if (net_ready() && net_address()) return 1;
		SleepEx(100, 0);
	}
	return net_ready() && net_address();
}

static int contains(const char* text, const char* needle)
{
	for (int i = 0; text[i]; i++)
	{
		int j = 0;
		while (needle[j] && text[i + j] == needle[j]) j++;
		if (!needle[j]) return 1;
	}
	return 0;
}

static WORD htons16(WORD v) { return (WORD)((v << 8) | (v >> 8)); }

static DWORD parse_ip(const char* s)
{
	DWORD out = 0;
	for (int part = 0; part < 4; part++)
	{
		unsigned v = 0;
		int digits = 0;
		while (*s >= '0' && *s <= '9')
		{
			v = v * 10 + (unsigned)(*s++ - '0');
			digits++;
		}
		if (!digits || v > 255 || (part < 3 && *s++ != '.') || (part == 3 && *s))
			return 0;
		out |= v << (part * 8);
	}
	return out;
}

static int wait_socket(SOCKET s, int write)
{
	fd_set ready, error;
	timeval tv;
	ready.fd_count = 1; ready.fd_array[0] = s;
	error.fd_count = 1; error.fd_array[0] = s;
	tv.tv_sec = RELAY_TIMEOUT; tv.tv_usec = 0;
	int n = write ? select(0, 0, &ready, &error, &tv)
	              : select(0, &ready, 0, &error, &tv);
	return n > 0 && ready.fd_count && !error.fd_count;
}

static int send_all(SOCKET s, const char* data, int len)
{
	while (len > 0)
	{
		WSABUF wb = { (DWORD)len, (char*)data };
		DWORD sent = 0;
		if (!wait_socket(s, 1) || WSASend(s, &wb, 1, &sent, 0, 0, 0) != 0 || !sent)
			return 0;
		data += sent;
		len -= (int)sent;
	}
	return 1;
}

static int recv_some(SOCKET s, char* data, int cap)
{
	WSABUF wb = { (DWORD)cap, data };
	DWORD got = 0, flags = 0;
	if (!wait_socket(s, 0) || WSARecv(s, &wb, 1, &got, &flags, 0, 0) != 0)
		return -1;
	return (int)got;
}

static SOCKET connect_relay(const RelayJob* job, int type, int protocol)
{
	SOCKET s = socket(AF_INET, type, protocol);
	if (s == INVALID_SOCKET) return INVALID_SOCKET;
	net_allow_insecure(s);
	sockaddr_in a;
	for (int i = 0; i < (int)sizeof(a); i++) ((char*)&a)[i] = 0;
	a.sin_family = AF_INET;
	a.sin_port = htons16(job->port);
	a.sin_addr = parse_ip(job->ip);
	if (!a.sin_addr || (connect(s, &a, sizeof(a)) != 0 && !wait_socket(s, 1)))
	{
		closesocket(s);
		return INVALID_SOCKET;
	}
	return s;
}

static int send_websocket(const RelayJob* job, const char* message, int messageLen)
{
	SOCKET s = connect_relay(job, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) return 0;
	char request[320];
	int n = 0;
	const char* a = "GET / HTTP/1.1\r\nHost: ";
	for (int i = 0; a[i]; i++) request[n++] = a[i];
	for (int i = 0; job->ip[i]; i++) request[n++] = job->ip[i];
	request[n++] = ':';
	char port[6]; int pn = 0; unsigned p = job->port;
	do { port[pn++] = (char)('0' + p % 10); p /= 10; } while (p);
	while (pn) request[n++] = port[--pn];
	a = "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";
	for (int i = 0; a[i]; i++) request[n++] = a[i];
	if (!send_all(s, request, n)) { closesocket(s); return 0; }

	char response[768]; int used = 0; response[0] = 0;
	while (used < (int)sizeof(response) - 1 && !contains(response, "\r\n\r\n"))
	{
		int got = recv_some(s, response + used, (int)sizeof(response) - 1 - used);
		if (got <= 0) { closesocket(s); return 0; }
		used += got; response[used] = 0;
	}
	if (!contains(response, "101 Switching Protocols") ||
	    !contains(response, "Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo="))
	{
		closesocket(s); return 0;
	}

	BYTE frame[192]; int f = 0;
	frame[f++] = 0x81;
	frame[f++] = (BYTE)(0x80 | messageLen); // title message is validated < 126 bytes
	DWORD mask = ++s_maskCounter * 1664525u + 1013904223u;
	BYTE* key = (BYTE*)&mask;
	for (int i = 0; i < 4; i++) frame[f++] = key[i];
	for (int i = 0; i < messageLen; i++) frame[f++] = (BYTE)message[i] ^ key[i & 3];
	int ok = send_all(s, (const char*)frame, f);
	if (ok)
	{
		// Finish the WebSocket session with a masked Close frame carrying status
		// 1000 (normal closure), rather than making the peer report EOF/1006.
		BYTE closeFrame[8];
		closeFrame[0] = 0x88;
		closeFrame[1] = 0x82;
		mask = ++s_maskCounter * 1664525u + 1013904223u;
		key = (BYTE*)&mask;
		for (int i = 0; i < 4; i++) closeFrame[2 + i] = key[i];
		closeFrame[6] = (BYTE)(0x03 ^ key[0]);
		closeFrame[7] = (BYTE)(0xe8 ^ key[1]);
		ok = send_all(s, (const char*)closeFrame, sizeof(closeFrame));
	}
	shutdown(s, SD_SEND);
	closesocket(s);
	return ok;
}

static int send_udp(const RelayJob* job, const char* message, int messageLen)
{
	// Proven completion-confirmed path used by the blocking API.
	if (s_udpSocket == INVALID_SOCKET)
	{
		s_udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
		if (s_udpSocket == INVALID_SOCKET)
		{
			s_relayError = GetLastError();
			if (!s_relayError) s_relayError = 20010;
			OutputDebugStringA("[discord] UDP socket failed\n");
			return 0;
		}
		if (net_allow_insecure(s_udpSocket) != 0)
		{
			s_relayError = GetLastError();
			if (!s_relayError) s_relayError = 20011;
			OutputDebugStringA("[discord] UDP insecure option failed\n");
			closesocket(s_udpSocket);
			s_udpSocket = INVALID_SOCKET;
			return 0;
		}
	}
	for (int i = 0; i < (int)sizeof(s_udpAddress); i++) ((char*)&s_udpAddress)[i] = 0;
	s_udpAddress.sin_family = AF_INET;
	s_udpAddress.sin_port = htons16(job->port);
	s_udpAddress.sin_addr = parse_ip(job->ip);
	for (int i = 0; i < messageLen; i++) s_udpMessage[i] = message[i];
	s_udpBuffer.len = (DWORD)messageLen;
	s_udpBuffer.buf = s_udpMessage;
	if (!s_udpEvent)
	{
		if (NtCreateEvent(&s_udpEvent, 0, 1, 0) < 0 || !s_udpEvent)
		{
			s_relayError = 10107;
			return 0;
		}
	}
	for (int i = 0; i < (int)sizeof(s_udpOverlapped); i++)
		((char*)&s_udpOverlapped)[i] = 0;
	s_udpOverlapped.hEvent = s_udpEvent;
	DWORD sent = 0;
	int result = WSASendTo(s_udpSocket, &s_udpBuffer, 1, &sent, 0,
	                       &s_udpAddress, sizeof(s_udpAddress),
	                       &s_udpOverlapped, 0);
	if (result == -1)
	{
		s_relayError = GetLastError();
		if (!s_relayError) s_relayError = 20012;
	}
	else
	{
		long long timeout = -100000000LL;
		int waitStatus = NtWaitForSingleObject(s_udpEvent, 0, &timeout);
		if (waitStatus != 0)
			s_relayError = 20013;
		else if (s_udpOverlapped.Internal != 0)
		{
			DWORD code = s_udpOverlapped.Internal & 0xffff;
			s_relayError = code ? code : 20014;
		}
		else if (sent != (DWORD)messageLen)
			s_relayError = 20015;
		else
			s_relayError = 0;
	}
	if (s_relayError)
	{
		OutputDebugStringA("[discord] UDP sendto failed\n");
		closesocket(s_udpSocket);
		s_udpSocket = INVALID_SOCKET;
		return 0;
	}
	return 1;
}

static int make_message(const RelayJob* job, char* message)
{
	int n = 0;
	const char* pre = "{\"id\":\"";
	for (int i = 0; pre[i]; i++) message[n++] = pre[i];
	for (int i = 0; job->title[i]; i++) message[n++] = job->title[i];
	message[n++] = '"'; message[n++] = '}'; message[n] = 0;
	return n;
}

static int send_relay(const RelayJob* job)
{
	char message[48];
	int n = make_message(job, message);

	if (job->port == 1101)
		return send_websocket(job, message, n);
	if (job->port == 1102)
		return send_udp(job, message, n);
	if (job->port != 1103) { s_relayError = 20004; return 0; }
	SOCKET s = connect_relay(job, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET) return 0;
	int ok = send_all(s, message, n);
	shutdown(s, SD_SEND);
	closesocket(s);
	return ok;
}

static int make_job(const WCHAR* title, RelayJob* job)
{
	int n = 0;
	for (; title && title[n] && n < 32; n++)
	{
		int c = title[n];
		if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')))
			return 0;
		job->title[n] = (char)c;
	}
	if (!n || (title && title[n])) return 0;
	job->title[n] = 0;
	char* cfg = skin_read_file(CONFIG_INI, 0);
	if (!cfg) return 0;
	char port[8];
	ini_value(cfg, "DiscordRelay", "IP", job->ip, sizeof(job->ip));
	ini_value(cfg, "DiscordRelay", "Port", port, sizeof(port));
	dash_free(cfg);
	if (!parse_ip(job->ip)) return 0;
	int p = 0, i = 0;
	for (; port[i] >= '0' && port[i] <= '9'; i++) p = p * 10 + port[i] - '0';
	if (!i || port[i]) return 0;
	if (p != 1101 && p != 1102 && p != 1103) return 0;
	job->port = (WORD)p;
	return 1;
}

static DWORD STDCALL relay_thread(void* parameter)
{
	RelayJob* job = (RelayJob*)parameter;
	s_relayError = 0;
	int ready = wait_network(60);
	if (!ready) s_relayError = 20002;
	int ok = ready && send_relay(job);
	if (!ok) OutputDebugStringA("[discord] asynchronous relay failed\n");
	s_status = ok ? 2 : (s_relayError ? -(int)s_relayError : -1);
	dash_free(job);
	return 0;
}

int THISCALL Config_SendDiscordRelay(void* pThis, const WCHAR* title)
{
	(void)pThis;
	if (s_status == 1) return 0;
	RelayJob job;
	if (!make_job(title, &job))
	{
		OutputDebugStringA("[discord] invalid title or relay config\n");
		s_relayError = 20001;
		s_status = -(int)s_relayError; return 0;
	}
	s_status = 1;
	s_relayError = 0;
	int ready = wait_network(RELAY_TIMEOUT);
	if (!ready) s_relayError = 20002;
	int ok = ready && send_relay(&job);
	if (!ok) OutputDebugStringA("[discord] blocking relay failed\n");
	s_status = ok ? 2 : (s_relayError ? -(int)s_relayError : -1);
	return ok;
}

int THISCALL Config_SendDiscordRelayAsync(void* pThis, const WCHAR* title)
{
	(void)pThis;
	if (s_status == 1) return 0;
	RelayJob parsed;
	if (!make_job(title, &parsed))
	{
		OutputDebugStringA("[discord] invalid title or relay config\n");
		s_relayError = 20001;
		s_status = -(int)s_relayError;
		return 0;
	}

	// Xbox UDP transmission is only reliable through the completion-confirmed
	// caller-thread path. TCP and WebSocket safely run on a worker thread.
	if (parsed.port == 1102)
		return Config_SendDiscordRelay(pThis, title);

	RelayJob* job = (RelayJob*)dash_malloc(sizeof(RelayJob));
	if (!job)
	{
		s_relayError = 20003;
		s_status = -(int)s_relayError;
		return 0;
	}
	*job = parsed;
	s_relayError = 0;
	s_status = 1;
	HANDLE thread = CreateThread(0, 0, (void*)relay_thread, job, 0, 0);
	if (!thread)
	{
		dash_free(job);
		s_relayError = GetLastError();
		if (!s_relayError) s_relayError = 20005;
		s_status = -(int)s_relayError;
		return 0;
	}
	CloseHandle(thread);
	return 1;
}

int THISCALL Config_GetDiscordRelayStatus(void* pThis)
{
	(void)pThis;
	return s_status;
}
