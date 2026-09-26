// ftp.c: a small FTP server for the dashboard, so you can push files to the
// console over the LAN like every other softmod dash does.
//
// It runs on the retail network stack: net.c gives us listen/accept, the
// socket thunks give the rest, and the transfers go straight through the
// XAPI file calls. Auth is xbox/xbox (or the [FTP] keys in config.ini).
// The path space is one folder per drive, using UIX Lite's scene letters
// (the browser's letters), each mapped to its real partition for file I/O.

#include "dash5960.h"
#include "net.h"
#include "ftp.h"
#include "uixlite.h"

#define AF_INET        2
#define SOCK_STREAM    1
#define IPPROTO_TCP    6
#define INADDR_ANY     0

#define CMD_PORT       21
#define PASV_LOW       6000
#define PASV_HIGH      6999
#define CMD_TIMEOUT    300 // seconds idle on the command channel
#define DATA_TIMEOUT   15  // seconds to make a data connection
#define XFER_BUF       (32 * 1024)
#define LINE_MAX       512
#define PATH_MAX       300
#define RESP_MAX       640

// Drives shown as UIX Lite's scene letters, not the raw system letters, so the
// FTP tree matches what the dashboard's file browser shows. Same mapping as
// harddrive.xap's GetDrive(): scene -> real drive the file calls actually use.
// A scene drive whose real letter isn't mounted is skipped (no disc, no 2nd HDD).
static const struct { const char* scene; char real; } kDrives[] = {
	{ "C",  'Y' }, // system
	{ "D",  'D' }, // DVD
	{ "E",  'C' }, // main data
	{ "F",  'N' }, // HDD0 partition 6
	{ "G",  'O' }, // HDD0 partition 7
	{ "E2", 'R' }, // HDD1 partition 1
	{ "F2", 'P' }, // HDD1 partition 6
	{ "G2", 'Q' }, // HDD1 partition 7
};
#define NDRIVES ((int)(sizeof(kDrives) / sizeof(kDrives[0])))

// Real drive letter for a scene name ("C","E2",...), 0 if unknown. Case-insensitive.
static char scene_to_real(const char* scene, int len)
{
	for (int i = 0; i < NDRIVES; i++)
	{
		const char* s = kDrives[i].scene;
		int j = 0;
		for (; j < len && s[j]; j++)
		{
			int a = scene[j], b = s[j];
			if (a >= 'a' && a <= 'z') a -= 32;
			if (a != b) break;
		}
		if (j == len && s[j] == 0)
			return kDrives[i].real;
	}
	return 0;
}

typedef struct Conn
{
	SOCKET cmd;
	SOCKET pasv;        // passive data listener, or INVALID_SOCKET
	sockaddr_in port;   // active-mode target
	int    hasPort;
	int    loggedIn;
	DWORD  rest;        // REST offset for the next transfer
	char   user[32];    // name from the USER command, checked at PASS
	char   cwd[PATH_MAX];
} Conn;

// Login and port, from [FTP] in config.ini (defaults below). Read once at start.
static char g_user[32] = "xbox";
static char g_pass[32] = "xbox";
static WORD g_port = CMD_PORT;

static void copy_str(char* dst, const char* src, int cap)
{
	int i = 0;
	for (; src[i] && i < cap - 1; i++) dst[i] = src[i];
	dst[i] = 0;
}

static int str_eq(const char* a, const char* b) // case-sensitive
{
	while (*a && *a == *b) { a++; b++; }
	return *a == *b;
}

// --- tiny freestanding string / format helpers ------------------------------

static int s_len(const char* s) { int n = 0; while (s[n]) n++; return n; }

static int ieq(const char* a, const char* b)
{
	for (;; a++, b++)
	{
		int ca = *a, cb = *b;
		if (ca >= 'a' && ca <= 'z') ca -= 32;
		if (cb >= 'a' && cb <= 'z') cb -= 32;
		if (ca != cb) return 0;
		if (!ca) return 1;
	}
}

static char* put_uint(char* p, DWORD v)
{
	char tmp[12];
	int n = 0;
	do { tmp[n++] = (char)('0' + v % 10); v /= 10; } while (v);
	while (n--) *p++ = tmp[n];
	return p;
}

// Minimal formatter: %s %u %d %c %%. Enough for FTP replies.
static void fmt(char* out, const char* f, ...)
{
	__builtin_va_list ap;
	__builtin_va_start(ap, f);
	char* p = out;
	for (; *f; f++)
	{
		if (*f != '%') { *p++ = *f; continue; }
		f++;
		if (*f == 's')
		{
			const char* s = __builtin_va_arg(ap, const char*);
			while (s && *s) *p++ = *s++;
		}
		else if (*f == 'u') p = put_uint(p, __builtin_va_arg(ap, DWORD));
		else if (*f == 'd')
		{
			int v = __builtin_va_arg(ap, int);
			if (v < 0) { *p++ = '-'; v = -v; }
			p = put_uint(p, (DWORD)v);
		}
		else if (*f == 'c') *p++ = (char)__builtin_va_arg(ap, int);
		else *p++ = *f; // %% and anything else
	}
	*p = 0;
	__builtin_va_end(ap);
}

// --- socket wrappers --------------------------------------------------------

static WORD hton16(WORD v) { return (WORD)((v << 8) | (v >> 8)); }

static int sel_read(SOCKET s, int sec)
{
	fd_set r;
	timeval tv;
	r.fd_count = 1;
	r.fd_array[0] = s;
	tv.tv_sec = sec;
	tv.tv_usec = 0;
	return select(0, &r, 0, 0, &tv);
}

static int recv_some(SOCKET s, char* buf, int len)
{
	WSABUF wb;
	DWORD got = 0, flags = 0;
	wb.len = (DWORD)len;
	wb.buf = buf;
	if (WSARecv(s, &wb, 1, &got, &flags, 0, 0) != 0)
		return -1;
	return (int)got; // 0 = peer closed
}

static int send_all(SOCKET s, const char* buf, int len)
{
	while (len > 0)
	{
		WSABUF wb;
		DWORD sent = 0;
		wb.len = (DWORD)len;
		wb.buf = (char*)buf;
		if (WSASend(s, &wb, 1, &sent, 0, 0, 0) != 0 || sent == 0)
			return -1;
		buf += sent;
		len -= (int)sent;
	}
	return 0;
}

static void reply(SOCKET s, const char* line) { send_all(s, line, s_len(line)); }

// One CRLF-terminated command line. -1 on close/timeout.
static int recv_line(SOCKET s, char* buf, int cap)
{
	int n = 0;
	for (;;)
	{
		char c;
		if (sel_read(s, CMD_TIMEOUT) <= 0)
			return -1;
		if (recv_some(s, &c, 1) <= 0)
			return -1;
		if (c == '\r')
			continue;
		if (c == '\n') { buf[n] = 0; return n; }
		if (n < cap - 1)
			buf[n++] = c;
	}
}

static SOCKET make_listener(WORD port, int backlog)
{
	sockaddr_in sa;
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == INVALID_SOCKET)
		return INVALID_SOCKET;
	for (int i = 0; i < (int)sizeof(sa); i++)
		((char*)&sa)[i] = 0;
	sa.sin_family = AF_INET;
	sa.sin_port = hton16(port);
	sa.sin_addr = INADDR_ANY;
	if (bind(s, &sa, sizeof(sa)) != 0)
	{
		closesocket(s);
		return INVALID_SOCKET;
	}
	net_allow_insecure(s);
	if (net_listen(s, backlog) != 0)
	{
		closesocket(s);
		return INVALID_SOCKET;
	}
	return s;
}

// --- path handling ----------------------------------------------------------

// Normalize a virtual path: start from base unless arg is absolute, fold in
// "." and "..", write the result (always leading '/', no trailing '/') to out.
static void resolve(const char* base, const char* arg, char* out, int cap)
{
	char tmp[PATH_MAX * 2];
	int n = 0;
	if (arg[0] != '/')
	{
		for (const char* b = base; *b && n < (int)sizeof(tmp) - 1; b++) tmp[n++] = *b;
		if (n == 0 || tmp[n - 1] != '/') if (n < (int)sizeof(tmp) - 1) tmp[n++] = '/';
	}
	for (const char* a = arg; *a && n < (int)sizeof(tmp) - 1; a++)
		tmp[n++] = (*a == '\\') ? '/' : *a;
	tmp[n] = 0;

	// walk segments
	int o = 0;
	out[o++] = '/';
	const char* p = tmp;
	while (*p)
	{
		while (*p == '/') p++;
		const char* seg = p;
		while (*p && *p != '/') p++;
		int len = (int)(p - seg);
		if (len == 0) continue;
		if (len == 1 && seg[0] == '.') continue;
		if (len == 2 && seg[0] == '.' && seg[1] == '.')
		{
			if (o > 1) { o--; while (o > 1 && out[o - 1] != '/') o--; if (o > 1) o--; }
			continue;
		}
		if (o > 1 && o < cap - 1) out[o++] = '/';
		for (int i = 0; i < len && o < cap - 1; i++) out[o++] = seg[i];
	}
	if (o == 0) out[o++] = '/';
	out[o] = 0;
}

// Virtual "/E/dir/file" (scene letters) -> real "C:\dir\file". Root, and an
// unknown scene drive, return 0; a mapped drive returns 1.
static int map_real(const char* v, char* out, int cap)
{
	const char* p = v;
	while (*p == '/') p++;
	if (!*p)
		return 0;
	const char* seg = p;
	while (*p && *p != '/') p++;
	char real = scene_to_real(seg, (int)(p - seg));
	if (!real)
		return 0;
	int o = 0;
	out[o++] = real;
	if (o < cap - 1) out[o++] = ':';
	if (o < cap - 1) out[o++] = '\\';
	if (*p == '/') p++;
	while (*p && o < cap - 1)
	{
		out[o++] = (*p == '/') ? '\\' : *p;
		p++;
	}
	out[o] = 0;
	return 1;
}

static int is_drive_root(const char* real)
{
	return real[0] && real[1] == ':' && real[2] == '\\' && real[3] == 0;
}

static int path_is_dir(const char* real)
{
	if (is_drive_root(real))
		return 1;
	DWORD a = GetFileAttributesA(real);
	return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY);
}

// --- directory listing ------------------------------------------------------

static void send_root_listing(SOCKET data, int namesOnly)
{
	for (int i = 0; i < NDRIVES; i++)
	{
		char root[8];
		root[0] = kDrives[i].real; root[1] = ':'; root[2] = '\\'; root[3] = 0;
		if (GetFileAttributesA(root) == INVALID_FILE_ATTRIBUTES)
			continue;
		char line[80];
		if (namesOnly)
			fmt(line, "%s\r\n", kDrives[i].scene);
		else
			fmt(line, "drwxrwxrwx 1 xbox xbox 0 Jan 01 00:00 %s\r\n", kDrives[i].scene);
		reply(data, line);
	}
}

static void send_dir_listing(SOCKET data, const char* real, int namesOnly)
{
	char search[PATH_MAX];
	int n = 0;
	for (const char* s = real; *s && n < PATH_MAX - 4; s++) search[n++] = *s;
	if (n && search[n - 1] != '\\') search[n++] = '\\';
	search[n++] = '*';
	search[n] = 0;

	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA(search, &fd);
	if (h == INVALID_HANDLE_VALUE)
		return;
	do
	{
		const char* name = fd.cFileName;
		if (name[0] == '.' && (name[1] == 0 || (name[1] == '.' && name[2] == 0)))
			continue;
		int dir = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		char line[400];
		if (namesOnly)
			fmt(line, "%s\r\n", name);
		else
			fmt(line, "%crwxrwxrwx 1 xbox xbox %u Jan 01 00:00 %s\r\n",
			    dir ? 'd' : '-', fd.nFileSizeLow, name);
		reply(data, line);
	} while (FindNextFileA(h, &fd));
	NtClose(h); // FindClose
}

// --- data channel -----------------------------------------------------------

static SOCKET open_data(Conn* c)
{
	if (c->pasv != INVALID_SOCKET)
	{
		SOCKET d = INVALID_SOCKET;
		if (sel_read(c->pasv, DATA_TIMEOUT) > 0)
			d = net_accept(c->pasv);
		closesocket(c->pasv);
		c->pasv = INVALID_SOCKET;
		return d;
	}
	if (c->hasPort)
	{
		c->hasPort = 0;
		SOCKET d = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (d == INVALID_SOCKET)
			return INVALID_SOCKET;
		net_allow_insecure(d);
		if (connect(d, &c->port, sizeof(c->port)) != 0)
		{
			closesocket(d);
			return INVALID_SOCKET;
		}
		return d;
	}
	return INVALID_SOCKET;
}

// --- command handlers -------------------------------------------------------

static void cmd_list(Conn* c, const char* arg, int namesOnly)
{
	char virt[PATH_MAX], real[PATH_MAX];
	// LIST may carry flags like "-la"; skip a leading option word
	if (arg[0] == '-')
	{
		while (*arg && *arg != ' ') arg++;
		while (*arg == ' ') arg++;
	}
	if (*arg)
		resolve(c->cwd, arg, virt, sizeof(virt));
	else
	{
		int i = 0;
		for (; c->cwd[i] && i < PATH_MAX - 1; i++) virt[i] = c->cwd[i];
		virt[i] = 0;
	}

	int atRoot = !map_real(virt, real, sizeof(real));
	if (!atRoot && !path_is_dir(real))
	{
		reply(c->cmd, "550 Not a directory.\r\n");
		return;
	}
	SOCKET data = open_data(c);
	if (data == INVALID_SOCKET)
	{
		reply(c->cmd, "425 Can't open data connection.\r\n");
		return;
	}
	reply(c->cmd, "150 Here comes the directory listing.\r\n");
	if (atRoot)
		send_root_listing(data, namesOnly);
	else
		send_dir_listing(data, real, namesOnly);
	closesocket(data);
	reply(c->cmd, "226 Directory send OK.\r\n");
}

static void cmd_retr(Conn* c, const char* arg)
{
	char virt[PATH_MAX], real[PATH_MAX];
	resolve(c->cwd, arg, virt, sizeof(virt));
	if (!map_real(virt, real, sizeof(real)))
	{
		reply(c->cmd, "550 No such file.\r\n");
		return;
	}
	HANDLE h = CreateFileA(real, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (h == INVALID_HANDLE_VALUE)
	{
		reply(c->cmd, "550 Failed to open file.\r\n");
		return;
	}
	if (c->rest)
	{
		SetFilePointer(h, (int)c->rest, 0, FILE_BEGIN);
		c->rest = 0;
	}
	SOCKET data = open_data(c);
	if (data == INVALID_SOCKET)
	{
		CloseHandle(h);
		reply(c->cmd, "425 Can't open data connection.\r\n");
		return;
	}
	reply(c->cmd, "150 Opening data connection.\r\n");
	char* buf = (char*)dash_malloc(XFER_BUF);
	int ok = buf != 0;
	while (ok)
	{
		DWORD got = 0;
		if (!ReadFile(h, buf, XFER_BUF, &got, 0)) { ok = 0; break; }
		if (got == 0)
			break; // eof
		if (send_all(data, buf, (int)got) != 0) { ok = 0; break; }
	}
	if (buf) dash_free(buf);
	closesocket(data);
	CloseHandle(h);
	reply(c->cmd, ok ? "226 Transfer complete.\r\n" : "426 Transfer failed.\r\n");
}

static void cmd_stor(Conn* c, const char* arg, int append)
{
	char virt[PATH_MAX], real[PATH_MAX];
	resolve(c->cwd, arg, virt, sizeof(virt));
	if (!map_real(virt, real, sizeof(real)))
	{
		reply(c->cmd, "550 No such path.\r\n");
		return;
	}
	// This XBE has no WriteFile; the dashboard writes through CRT stdio, so we
	// do too (fopen/fwrite/fclose). APPE appends, a REST resume opens the file
	// for update and seeks, a plain STOR truncates and starts fresh.
	const char* mode = append ? "ab" : (c->rest ? "r+b" : "wb");
	void* f = crt_fopen(real, mode, SH_DENYNO);
	if (!f && c->rest)          // resuming a file that isn't there yet
		f = crt_fopen(real, "wb", SH_DENYNO);
	if (!f)
	{
		reply(c->cmd, "550 Failed to create file.\r\n");
		return;
	}
	if (append)
		crt_fseek(f, 0, STDIO_SEEK_END);
	else if (c->rest)
		crt_fseek(f, (long)c->rest, STDIO_SEEK_SET);
	c->rest = 0;
	SOCKET data = open_data(c);
	if (data == INVALID_SOCKET)
	{
		crt_fclose(f);
		reply(c->cmd, "425 Can't open data connection.\r\n");
		return;
	}
	reply(c->cmd, "150 Ok to send data.\r\n");
	char* buf = (char*)dash_malloc(XFER_BUF);
	int ok = buf != 0;
	while (ok)
	{
		// Wait for the socket to be readable first; recv straight away can come
		// back would-block on the stack's sockets and look like a failure.
		int sel = sel_read(data, DATA_TIMEOUT);
		if (sel <= 0) { ok = 0; break; } // timeout or error before the peer closed
		int got = recv_some(data, buf, XFER_BUF);
		if (got < 0) { ok = 0; break; }
		if (got == 0)
			break; // peer closed cleanly = done
		if (crt_fwrite(buf, 1, (DWORD)got, f) != (DWORD)got) { ok = 0; break; }
	}
	if (buf) dash_free(buf);
	closesocket(data);
	crt_fclose(f);
	reply(c->cmd, ok ? "226 Transfer complete.\r\n" : "426 Transfer failed.\r\n");
}

static void cmd_pasv(Conn* c)
{
	static WORD next = PASV_LOW;
	if (c->pasv != INVALID_SOCKET)
		closesocket(c->pasv);
	c->pasv = INVALID_SOCKET;

	WORD port = 0;
	for (int tries = 0; tries < (PASV_HIGH - PASV_LOW + 1); tries++)
	{
		port = next++;
		if (next > PASV_HIGH) next = PASV_LOW;
		c->pasv = make_listener(port, 1);
		if (c->pasv != INVALID_SOCKET)
			break;
	}
	if (c->pasv == INVALID_SOCKET)
	{
		reply(c->cmd, "425 Can't open passive connection.\r\n");
		return;
	}
	DWORD ip = net_address(); // network order
	char line[80];
	fmt(line, "227 Entering Passive Mode (%u,%u,%u,%u,%u,%u)\r\n",
	    ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff,
	    (port >> 8) & 0xff, port & 0xff);
	reply(c->cmd, line);
}

static void cmd_port(Conn* c, const char* arg)
{
	unsigned v[6];
	int idx = 0;
	for (int i = 0; i < 6; i++) v[i] = 0;
	for (const char* p = arg; *p && idx < 6; p++)
	{
		if (*p >= '0' && *p <= '9')
			v[idx] = v[idx] * 10 + (unsigned)(*p - '0');
		else if (*p == ',')
			idx++;
	}
	if (idx != 5)
	{
		reply(c->cmd, "501 Bad PORT.\r\n");
		return;
	}
	for (int i = 0; i < (int)sizeof(c->port); i++) ((char*)&c->port)[i] = 0;
	c->port.sin_family = AF_INET;
	c->port.sin_addr = (DWORD)(v[0] | (v[1] << 8) | (v[2] << 16) | (v[3] << 24));
	c->port.sin_port = (WORD)((v[5] << 8) | v[4]); // already network order
	c->hasPort = 1;
	if (c->pasv != INVALID_SOCKET) { closesocket(c->pasv); c->pasv = INVALID_SOCKET; }
	reply(c->cmd, "200 PORT command successful.\r\n");
}

static void cmd_size(Conn* c, const char* arg)
{
	char virt[PATH_MAX], real[PATH_MAX];
	resolve(c->cwd, arg, virt, sizeof(virt));
	if (!map_real(virt, real, sizeof(real)))
	{
		reply(c->cmd, "550 No such file.\r\n");
		return;
	}
	HANDLE h = CreateFileA(real, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (h == INVALID_HANDLE_VALUE)
	{
		reply(c->cmd, "550 File not found.\r\n");
		return;
	}
	DWORD sz = GetFileSize(h, 0);
	CloseHandle(h);
	char line[48];
	fmt(line, "213 %u\r\n", sz);
	reply(c->cmd, line);
}

// --- connection thread ------------------------------------------------------

static DWORD STDCALL conn_thread(void* param)
{
	Conn c;
	for (int i = 0; i < (int)sizeof(c); i++) ((char*)&c)[i] = 0;
	c.cmd = (SOCKET)(DWORD)param;
	c.pasv = INVALID_SOCKET;
	c.cwd[0] = '/'; c.cwd[1] = 0;

	net_allow_insecure(c.cmd); // accepted children inherit it, but be sure
	reply(c.cmd,
	      "220-UIX Lite FTP  //  Xbox Dashboard 5960\r\n"
	      "220 Serving live from the dashboard. Send USER to log in.\r\n");

	char line[LINE_MAX];
	while (recv_line(c.cmd, line, sizeof(line)) >= 0)
	{
		char* arg = line;
		while (*arg && *arg != ' ') arg++;
		if (*arg == ' ') *arg++ = 0;
		else arg = line + s_len(line);

		if (ieq(line, "USER"))
		{
			copy_str(c.user, arg, sizeof(c.user));
			reply(c.cmd, "331 Password required.\r\n");
		}
		else if (ieq(line, "PASS"))
		{
			if (str_eq(c.user, g_user) && str_eq(arg, g_pass))
			{
				c.loggedIn = 1;
				reply(c.cmd, "230 Logged in.\r\n");
			}
			else
				reply(c.cmd, "530 Login incorrect.\r\n");
		}
		else if (ieq(line, "QUIT"))
		{
			reply(c.cmd, "221 Goodbye.\r\n");
			break;
		}
		else if (!c.loggedIn)
			reply(c.cmd, "530 Not logged in.\r\n");
		else if (ieq(line, "SYST"))
			reply(c.cmd, "215 UNIX Type: L8\r\n");
		else if (ieq(line, "FEAT"))
			reply(c.cmd, "211-Features:\r\n SIZE\r\n REST STREAM\r\n211 End\r\n");
		else if (ieq(line, "NOOP"))
			reply(c.cmd, "200 NOOP ok.\r\n");
		else if (ieq(line, "TYPE"))
			reply(c.cmd, "200 Type set.\r\n");
		else if (ieq(line, "PWD") || ieq(line, "XPWD"))
		{
			char out[PATH_MAX + 32];
			fmt(out, "257 \"%s\" is current directory.\r\n", c.cwd);
			reply(c.cmd, out);
		}
		else if (ieq(line, "CWD") || ieq(line, "XCWD"))
		{
			char virt[PATH_MAX], real[PATH_MAX];
			resolve(c.cwd, arg, virt, sizeof(virt));
			int atRoot = !map_real(virt, real, sizeof(real));
			if (atRoot || path_is_dir(real))
			{
				int i = 0;
				for (; virt[i] && i < PATH_MAX - 1; i++) c.cwd[i] = virt[i];
				c.cwd[i] = 0;
				reply(c.cmd, "250 Directory changed.\r\n");
			}
			else
				reply(c.cmd, "550 No such directory.\r\n");
		}
		else if (ieq(line, "CDUP") || ieq(line, "XCUP"))
		{
			char virt[PATH_MAX];
			resolve(c.cwd, "..", virt, sizeof(virt));
			int i = 0;
			for (; virt[i] && i < PATH_MAX - 1; i++) c.cwd[i] = virt[i];
			c.cwd[i] = 0;
			reply(c.cmd, "250 Directory changed.\r\n");
		}
		else if (ieq(line, "PASV"))
			cmd_pasv(&c);
		else if (ieq(line, "PORT"))
			cmd_port(&c, arg);
		else if (ieq(line, "LIST"))
			cmd_list(&c, arg, 0);
		else if (ieq(line, "NLST"))
			cmd_list(&c, arg, 1);
		else if (ieq(line, "RETR"))
			cmd_retr(&c, arg);
		else if (ieq(line, "STOR"))
			cmd_stor(&c, arg, 0);
		else if (ieq(line, "APPE"))
			cmd_stor(&c, arg, 1);
		else if (ieq(line, "REST"))
		{
			DWORD v = 0;
			for (const char* p = arg; *p >= '0' && *p <= '9'; p++) v = v * 10 + (DWORD)(*p - '0');
			c.rest = v;
			reply(c.cmd, "350 Restart position accepted.\r\n");
		}
		else if (ieq(line, "SIZE"))
			cmd_size(&c, arg);
		else if (ieq(line, "DELE"))
		{
			char virt[PATH_MAX], real[PATH_MAX];
			resolve(c.cwd, arg, virt, sizeof(virt));
			int ok = map_real(virt, real, sizeof(real)) && DeleteFileA(real);
			reply(c.cmd, ok ? "250 File deleted.\r\n" : "550 Delete failed.\r\n");
		}
		else if (ieq(line, "MKD") || ieq(line, "XMKD"))
		{
			char virt[PATH_MAX], real[PATH_MAX];
			resolve(c.cwd, arg, virt, sizeof(virt));
			int ok = map_real(virt, real, sizeof(real)) && CreateDirectoryA(real, 0);
			if (ok)
			{
				char out[PATH_MAX + 32];
				fmt(out, "257 \"%s\" created.\r\n", virt);
				reply(c.cmd, out);
			}
			else
				reply(c.cmd, "550 Create failed.\r\n");
		}
		else if (ieq(line, "RMD") || ieq(line, "XRMD"))
		{
			char virt[PATH_MAX], real[PATH_MAX];
			resolve(c.cwd, arg, virt, sizeof(virt));
			int ok = map_real(virt, real, sizeof(real)) && RemoveDirectoryA(real);
			reply(c.cmd, ok ? "250 Directory removed.\r\n" : "550 Remove failed.\r\n");
		}
		else if (ieq(line, "OPTS"))
			reply(c.cmd, "200 Ok.\r\n");
		else
			reply(c.cmd, "500 Command not understood.\r\n");
	}

	if (c.pasv != INVALID_SOCKET)
		closesocket(c.pasv);
	closesocket(c.cmd);
	return 0;
}

// --- listener thread --------------------------------------------------------

static DWORD STDCALL listen_thread(void* param)
{
	(void)param;
	while (!net_ready())
		SleepEx(500, 0);

	SOCKET ls = INVALID_SOCKET;
	while (ls == INVALID_SOCKET)
	{
		ls = make_listener(g_port, 8);
		if (ls == INVALID_SOCKET)
			SleepEx(1000, 0);
	}
	OutputDebugStringA("[ftp] listening\n");

	for (;;)
	{
		if (sel_read(ls, 1) > 0)
		{
			SOCKET c = net_accept(ls);
			if (c != INVALID_SOCKET)
			{
				HANDLE t = CreateThread(0, 0, (void*)conn_thread, (void*)(DWORD)c, 0, 0);
				if (t) { SetThreadPriority(t, 2); CloseHandle(t); }
				else closesocket(c);
			}
		}
		else
			SleepEx(20, 0);
	}
}

// [FTP] User / Pass / Port from config.ini; unset keys keep the defaults.
static void load_config(void)
{
	char* cfg = skin_read_file(CONFIG_INI, 0);
	if (!cfg)
		return;
	char v[32];
	ini_value(cfg, "FTP", "User", v, sizeof(v));
	if (v[0]) copy_str(g_user, v, sizeof(g_user));
	ini_value(cfg, "FTP", "Pass", v, sizeof(v));
	if (v[0]) copy_str(g_pass, v, sizeof(g_pass));
	ini_value(cfg, "FTP", "Port", v, sizeof(v));
	if (v[0])
	{
		int p = 0;
		for (const char* q = v; *q >= '0' && *q <= '9'; q++) p = p * 10 + (*q - '0');
		if (p > 0 && p < 65536) g_port = (WORD)p;
	}
	dash_free(cfg);
}

void ftp_start(void)
{
	static int started;
	if (started)
		return;
	started = 1;
	load_config();
	// Buffered writes go through the file cache; the default is tiny (~128 KB)
	// and a large upload fills it and starts failing. Give it room.
	XSetFileCacheSize(4 * 1024 * 1024);
	HANDLE t = CreateThread(0, 0, (void*)listen_thread, 0, 0, 0);
	if (t)
	{
		SetThreadPriority(t, 1);
		CloseHandle(t);
	}
}
