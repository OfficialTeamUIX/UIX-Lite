// ntp.c: asynchronous SNTP clock synchronization for theConfig.
//
// The complete transaction runs on a worker: wait for networking, resolve the
// server, send through the completion-confirmed UDP path, receive the reply,
// and set the UTC system clock. The XAP caller returns immediately.

// XAP entry points:
//   SyncNetworkTimeAsync()  starts a query, returning 1 if accepted
//   GetNetworkTimeStatus()  0 idle, 2 querying, 3 synchronized, negative error

// [NTP] config.ini keys (all optional): Enabled, Server, Port, Timeout.

// Server accepts a numeric IPv4 address or hostname and defaults to
// time.nist.gov. Hostnames use the retail dashboard's XNet DNS resolver.

#include "dash5960.h"
#include "net.h"
#include "uixlite.h"

#define AF_INET       2
#define SOCK_DGRAM    2
#define NTP_PACKET    48
#define NTP_EPOCH     2208988800ULL
#define FILETIME_EPOCH 11644473600ULL

typedef struct WSAOVERLAPPED
{
	DWORD Internal;
	DWORD InternalHigh;
	DWORD Offset;
	DWORD OffsetHigh;
	HANDLE hEvent;
} WSAOVERLAPPED;

static volatile int s_ntpStatus;
static SOCKET s_ntpSocket = INVALID_SOCKET;
static HANDLE s_ntpEvent;
static HANDLE s_dnsEvent;
static WSAOVERLAPPED s_ntpOverlapped;
static sockaddr_in s_ntpAddress;
static WSABUF s_ntpBuffer;
static BYTE s_ntpRequest[NTP_PACKET];
static DWORD s_ntpCookie;
static int s_ntpTimeoutMs = 5000;

#define WSAEINPROGRESS 10036

static WORD htons16(WORD v) { return (WORD)((v << 8) | (v >> 8)); }

static int wait_network(int seconds)
{
	for (int i = 0; i < seconds * 10; i++)
	{
		if (net_ready() && net_address()) return 1;
		SleepEx(100, 0);
	}
	return net_ready() && net_address();
}

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

static int parse_uint(const char* s, int fallback, int low, int high)
{
	int value = 0, digits = 0;
	while (*s >= '0' && *s <= '9')
	{
		if (value > high / 10) return fallback;
		value = value * 10 + *s++ - '0';
		digits++;
	}
	return digits && !*s && value >= low && value <= high ? value : fallback;
}

static int lower(int c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; }

static int equals_ci(const char* a, const char* b)
{
	while (*a && *b && lower(*a) == lower(*b)) { a++; b++; }
	return !*a && !*b;
}

static int enabled_value(const char* s)
{
	if (!*s) return 1;
	if (equals_ci(s, "0") || equals_ci(s, "false") ||
	    equals_ci(s, "no") || equals_ci(s, "off")) return 0;
	return 1;
}

static DWORD resolve_server(const char* server)
{
	DWORD address = parse_ip(server);
	if (address) return address;
	if (!*server) server = "time.nist.gov";
	if (!s_dnsEvent && (NtCreateEvent(&s_dnsEvent, 0, 1, 0) < 0 || !s_dnsEvent))
		return 0;
	XNDNS* dns = 0;
	if (XNetDnsLookup(server, s_dnsEvent, &dns) != 0 || !dns)
		return 0;
	if (dns->status == WSAEINPROGRESS)
	{
		long long wait = -(long long)s_ntpTimeoutMs * 10000LL;
		if (NtWaitForSingleObject(s_dnsEvent, 0, &wait) != 0)
		{
			XNetDnsRelease(dns);
			return 0;
		}
	}
	address = dns->status == 0 && dns->count && dns->count <= 8
	        ? dns->addresses[0] : 0;
	XNetDnsRelease(dns);
	return address;
}

static DWORD read_be32(const BYTE* p)
{
	return ((DWORD)p[0] << 24) | ((DWORD)p[1] << 16) |
	       ((DWORD)p[2] << 8) | p[3];
}

static void write_be32(BYTE* p, DWORD v)
{
	p[0] = (BYTE)(v >> 24); p[1] = (BYTE)(v >> 16);
	p[2] = (BYTE)(v >> 8);  p[3] = (BYTE)v;
}

static int set_clock_from_reply(const BYTE* reply, DWORD count)
{
	if (count < NTP_PACKET) return -21009;
	int leap = reply[0] >> 6;
	int version = (reply[0] >> 3) & 7;
	int mode = reply[0] & 7;
	if (leap == 3 || (version != 3 && version != 4) || mode != 4 ||
	    !reply[1] || reply[1] > 15) return -21010;
	for (int i = 0; i < 8; i++)
		if (reply[24 + i] != s_ntpRequest[40 + i]) return -21011;

	unsigned long long seconds = read_be32(reply + 40);
	DWORD fraction = read_be32(reply + 44);
	if (!seconds) return -21012;
	if (seconds < NTP_EPOCH) seconds += 0x100000000ULL; // NTP era 1
	if (seconds < NTP_EPOCH) return -21012;
	seconds -= NTP_EPOCH;
	unsigned long long ticks = (seconds + FILETIME_EPOCH) * 10000000ULL;
	ticks += ((unsigned long long)fraction * 10000000ULL) >> 32;
	DWORD systemTime[2];
	systemTime[0] = (DWORD)ticks;
	systemTime[1] = (DWORD)(ticks >> 32);
	TIME_FIELDS fields;
	RtlTimeToTimeFields(systemTime, &fields);
	if (fields.Year < 2020 || fields.Year > 2099) return -21013;
	return NtSetSystemTime(systemTime, 0) >= 0 ? 3 : -21014;
}

static DWORD STDCALL ntp_receive_thread(void* parameter)
{
	(void)parameter;
	fd_set ready, error;
	timeval tv;
	ready.fd_count = 1; ready.fd_array[0] = s_ntpSocket;
	error.fd_count = 1; error.fd_array[0] = s_ntpSocket;
	tv.tv_sec = s_ntpTimeoutMs / 1000;
	tv.tv_usec = (s_ntpTimeoutMs % 1000) * 1000;
	int selected = select(0, &ready, 0, &error, &tv);
	if (selected <= 0 || !ready.fd_count || error.fd_count)
		s_ntpStatus = selected == 0 ? -21008 : -21007;
	else
	{
		BYTE reply[NTP_PACKET];
		WSABUF buffer = { sizeof(reply), (char*)reply };
		DWORD received = 0, flags = 0;
		if (WSARecv(s_ntpSocket, &buffer, 1, &received, &flags, 0, 0) != 0)
			s_ntpStatus = -21007;
		else
			s_ntpStatus = set_clock_from_reply(reply, received);
	}
	closesocket(s_ntpSocket);
	s_ntpSocket = INVALID_SOCKET;
	return 0;
}

static DWORD STDCALL ntp_sync_thread(void* parameter)
{
	(void)parameter;

	char enabled[8] = "";
	char server[256] = "";
	char portText[8] = "";
	char timeoutText[12] = "";
	char* cfg = skin_read_file(CONFIG_INI, 0);
	if (cfg)
	{
		ini_value(cfg, "NTP", "Enabled", enabled, sizeof(enabled));
		ini_value(cfg, "NTP", "Server", server, sizeof(server));
		ini_value(cfg, "NTP", "Port", portText, sizeof(portText));
		ini_value(cfg, "NTP", "Timeout", timeoutText, sizeof(timeoutText));
		dash_free(cfg);
	}
	if (!enabled_value(enabled)) { s_ntpStatus = -21001; return 0; }
	if (!wait_network(60)) { s_ntpStatus = -21002; return 0; }

	int port = parse_uint(portText, 123, 1, 65535);
	s_ntpTimeoutMs = parse_uint(timeoutText, 5000, 1000, 30000);
	DWORD address = resolve_server(server);
	if (!address) { s_ntpStatus = -21016; return 0; }

	s_ntpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_ntpSocket == INVALID_SOCKET) { s_ntpStatus = -21003; return 0; }
	if (net_allow_insecure(s_ntpSocket) != 0)
	{
		closesocket(s_ntpSocket); s_ntpSocket = INVALID_SOCKET;
		s_ntpStatus = -21004; return 0;
	}
	for (int i = 0; i < NTP_PACKET; i++) s_ntpRequest[i] = 0;
	s_ntpRequest[0] = 0x23; // LI=0, version=4, mode=client
	s_ntpCookie += 0x9e3779b9u;
	write_be32(s_ntpRequest + 40, s_ntpCookie ^ net_address());
	write_be32(s_ntpRequest + 44, s_ntpCookie + 0x6d2b79f5u);

	for (int i = 0; i < (int)sizeof(s_ntpAddress); i++) ((char*)&s_ntpAddress)[i] = 0;
	s_ntpAddress.sin_family = AF_INET;
	s_ntpAddress.sin_port = htons16((WORD)port);
	s_ntpAddress.sin_addr = address;
	s_ntpBuffer.len = NTP_PACKET;
	s_ntpBuffer.buf = (char*)s_ntpRequest;
	if (!s_ntpEvent && (NtCreateEvent(&s_ntpEvent, 0, 1, 0) < 0 || !s_ntpEvent))
	{
		closesocket(s_ntpSocket); s_ntpSocket = INVALID_SOCKET;
		s_ntpStatus = -21005; return 0;
	}
	for (int i = 0; i < (int)sizeof(s_ntpOverlapped); i++) ((char*)&s_ntpOverlapped)[i] = 0;
	s_ntpOverlapped.hEvent = s_ntpEvent;
	DWORD sent = 0;
	int result = WSASendTo(s_ntpSocket, &s_ntpBuffer, 1, &sent, 0,
	                       &s_ntpAddress, sizeof(s_ntpAddress),
	                       &s_ntpOverlapped, 0);
	if (result == -1)
		s_ntpStatus = -21006;
	else
	{
		long long wait = -100000000LL;
		if (NtWaitForSingleObject(s_ntpEvent, 0, &wait) != 0 ||
		    s_ntpOverlapped.Internal != 0 || sent != NTP_PACKET)
			s_ntpStatus = -21006;
		else
			s_ntpStatus = 2;
	}
	if (s_ntpStatus != 2)
	{
		closesocket(s_ntpSocket); s_ntpSocket = INVALID_SOCKET;
		return 0;
	}
	return ntp_receive_thread(0);
}

int THISCALL Config_SyncNetworkTimeAsync(void* pThis)
{
	(void)pThis;
	if (s_ntpStatus == 2) return 0;
	s_ntpStatus = 2;
	HANDLE thread = CreateThread(0, 0, (void*)ntp_sync_thread, 0, 0, 0);
	if (!thread)
	{
		s_ntpStatus = -21015;
		return 0;
	}
	CloseHandle(thread);
	return 1;
}

int THISCALL Config_GetNetworkTimeStatus(void* pThis)
{
	(void)pThis;
	return s_ntpStatus;
}
