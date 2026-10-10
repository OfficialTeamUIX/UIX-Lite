// fileops.c: file helpers for XAP -- copy/move/delete a file, and read a
// title's XBE for its TitleID (handy for matching icons without a hand-kept
// Icons.ini). Registered on theConfig alongside the other script functions.

#include "dash5960.h"

// Minimal VM object layouts used by the generic script-function adapter.
// CStrObject is 0x20 bytes in retail 5960; its owned WCHAR buffer is at +0x1c.
// XAP booleans are numeric objects, represented as float 0 or 1.
typedef struct ScriptObject
{
	void* vtable;
	int refCount;
	int type;
	void* parent;
	void* members;
} ScriptObject;

typedef struct ScriptStringObject
{
	ScriptObject base;
	int length;
	int capacity;
	WCHAR* text;
} ScriptStringObject;

typedef struct ScriptNumberObject
{
	ScriptObject base;
	float value;
} ScriptNumberObject;

#define SCRIPT_TYPE_NUMBER 2
#define SCRIPT_TYPE_STRING 3
#define SCRIPT_NUMBER_VTABLE ((void*)0x00022c50)

static void* make_bool_object(int value)
{
	ScriptNumberObject* o = (ScriptNumberObject*)op_new(sizeof(ScriptNumberObject));
	if (!o)
		return 0;
	o->base.vtable = SCRIPT_NUMBER_VTABLE;
	o->base.refCount = 1;
	o->base.type = SCRIPT_TYPE_NUMBER;
	o->base.parent = 0;
	o->base.members = 0;
	o->value = value ? 1.0f : 0.0f;
	return o;
}

static int get_two_string_args(void** args, int count, const WCHAR** first, const WCHAR** second)
{
	if (count != 2 || !args || !args[0] || !args[1])
		return 0;
	ScriptStringObject* a = (ScriptStringObject*)args[0];
	ScriptStringObject* b = (ScriptStringObject*)args[1];
	if (a->base.type != SCRIPT_TYPE_STRING || b->base.type != SCRIPT_TYPE_STRING
		|| !a->text || !b->text)
		return 0;
	*first = a->text;
	*second = b->text;
	return 1;
}

// Wide script string -> ASCII path for the file calls.
static void w2a(const WCHAR* w, char* out, int cap)
{
	int i = 0;
	for (; w && w[i] && i < cap - 1; i++)
		out[i] = (char)w[i];
	out[i] = 0;
}

// theConfig.GetXbeTitleID(path): the 8-hex TitleID from a title's default.xbe,
// e.g. "4D530064" (the UDATA folder name), or "" if it can't be read.
void* THISCALL Config_GetXbeTitleID(void* pThis, const WCHAR* wpath)
{
	(void)pThis;
	char path[260];
	w2a(wpath, path, sizeof(path));

	WCHAR out[12];
	out[0] = 0;
	HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (h != INVALID_HANDLE_VALUE)
	{
		BYTE* hdr = (BYTE*)dash_malloc(0x1000);
		DWORD got = 0;
		// XBE header holds the base and the certificate's virtual address; the
		// cert (in the header) has the TitleID at +0x08.
		if (hdr && ReadFile(h, hdr, 0x1000, &got, 0) && got > 0x11c && hdr[0] == 'X' && hdr[1] == 'B' && hdr[2] == 'E' && hdr[3] == 'H')
		{
			DWORD base = *(DWORD*)(hdr + 0x104);
			DWORD certVA = *(DWORD*)(hdr + 0x118);
			if (certVA >= base)
			{
				DWORD certOff = certVA - base;
				if (certOff <= got && got - certOff >= 0x0c)
				{
					DWORD tid = *(DWORD*)(hdr + certOff + 8);
					const WCHAR* d = u"0123456789ABCDEF";
					for (int i = 0; i < 8; i++)
						out[i] = d[(tid >> ((7 - i) * 4)) & 0xf];
					out[8] = 0;
				}
			}
		}
		if (hdr) dash_free(hdr);
		CloseHandle(h);
	}
	return make_strobj(out);
}

// theConfig.GetXbeTitleName(path): the title name stored in the XBE certificate,
// or "" if the file or certificate cannot be read. The field is 40 WCHARs.
void* THISCALL Config_GetXbeTitleName(void* pThis, const WCHAR* wpath)
{
	(void)pThis;
	char path[260];
	w2a(wpath, path, sizeof(path));

	WCHAR out[41];
	out[0] = 0;
	HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (h != INVALID_HANDLE_VALUE)
	{
		BYTE* hdr = (BYTE*)dash_malloc(0x1000);
		DWORD got = 0;
		if (hdr && ReadFile(h, hdr, 0x1000, &got, 0) && got > 0x11c && hdr[0] == 'X' && hdr[1] == 'B' && hdr[2] == 'E' && hdr[3] == 'H')
		{
			DWORD base = *(DWORD*)(hdr + 0x104);
			DWORD certVA = *(DWORD*)(hdr + 0x118);
			if (certVA >= base)
			{
				DWORD certOff = certVA - base;
				if (certOff <= got && got - certOff >= 0x0c + 40 * sizeof(WCHAR))
				{
					const WCHAR* title = (const WCHAR*)(hdr + certOff + 0x0c);
					int i = 0;
					for (; i < 40 && title[i]; i++)
						out[i] = title[i];
					out[i] = 0;
				}
			}
		}
		if (hdr) dash_free(hdr);
		CloseHandle(h);
	}
	return make_strobj(out);
}

// theConfig.DeleteFile(path): 1 on success, 0 on failure.
int THISCALL Config_DeleteFile(void* pThis, const WCHAR* wpath)
{
	(void)pThis;
	char path[260];
	w2a(wpath, path, sizeof(path));
	return DeleteFileA(path) ? 1 : 0;
}

// Directory helpers use the dashboard's Win32-compatible file API. CreateDir
// creates one directory (not missing parents); RemoveDir only removes an empty
// directory. Both are exposed to XAP as boolean/int results.
int THISCALL Config_CreateDir(void* pThis, const WCHAR* wpath)
{
	(void)pThis;
	char path[260];
	w2a(wpath, path, sizeof(path));
	return CreateDirectoryA(path, 0) ? 1 : 0;
}

int THISCALL Config_RemoveDir(void* pThis, const WCHAR* wpath)
{
	(void)pThis;
	char path[260];
	w2a(wpath, path, sizeof(path));
	return RemoveDirectoryA(path) ? 1 : 0;
}

static int section_name_is(const BYTE* header, DWORD headerSize, DWORD base,
	                       DWORD nameVA, const char* wanted)
{
	if (nameVA < base)
		return 0;
	DWORD off = nameVA - base;
	if (off >= headerSize || headerSize - off <= 9)
		return 0;
	for (DWORD i = 0; wanted[i]; i++)
		if (header[off + i] != (BYTE)wanted[i])
			return 0;
	return header[off + 9] == 0;
}

// Generic XAP adapter for bool ExportXbeTitleImage(string source, string destination).
// It exports the raw bytes belonging to the optional $$XTIMAGE XBE section.
void* THISCALL Config_ExportXbeTitleImage(void* pThis, void** args, int count)
{
	(void)pThis;
	int ok = 0, outputStarted = 0;
	const WCHAR *wsource, *wdestination;
	if (!get_two_string_args(args, count, &wsource, &wdestination))
		return make_bool_object(0);

	char source[260], destination[260];
	w2a(wsource, source, sizeof(source));
	w2a(wdestination, destination, sizeof(destination));
	HANDLE h = CreateFileA(source, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
	                     FILE_ATTRIBUTE_NORMAL, 0);
	if (h == INVALID_HANDLE_VALUE)
		return make_bool_object(0);

	DWORD fileSize = GetFileSize(h, 0);
	DWORD headerCap = fileSize < 0x10000 ? fileSize : 0x10000;
	BYTE* header = headerCap >= 0x124 ? (BYTE*)dash_malloc(headerCap) : 0;
	DWORD got = 0, rawOffset = 0, rawSize = 0;
	if (header && fileSize != 0xffffffff && ReadFile(h, header, headerCap, &got, 0)
		&& got >= 0x124 && header[0] == 'X' && header[1] == 'B'
		&& header[2] == 'E' && header[3] == 'H')
	{
		DWORD base = *(DWORD*)(header + 0x104);
		DWORD sectionCount = *(DWORD*)(header + 0x11c);
		DWORD sectionVA = *(DWORD*)(header + 0x120);
		if (sectionCount <= 64 && sectionVA >= base)
		{
			DWORD sectionOff = sectionVA - base;
			for (DWORD i = 0; i < sectionCount; i++)
			{
				DWORD off = sectionOff + i * 0x38;
				if (off > got || got - off < 0x18)
					break;
				DWORD nameVA = *(DWORD*)(header + off + 0x14);
				if (!section_name_is(header, got, base, nameVA, "$$XTIMAGE"))
					continue;
				rawOffset = *(DWORD*)(header + off + 0x0c);
				rawSize = *(DWORD*)(header + off + 0x10);
				if (rawOffset > fileSize || rawSize > fileSize - rawOffset)
					rawSize = 0;
				break;
			}
		}
	}
	if (header)
		dash_free(header);

	if (rawSize && SetFilePointer(h, (int)rawOffset, 0, 0) != 0xffffffff)
	{
		void* out = crt_fopen(destination, "wb", SH_DENYNO);
		if (out)
		{
			outputStarted = 1;
			BYTE* buf = (BYTE*)dash_malloc(32 * 1024);
			DWORD remaining = rawSize;
			ok = buf != 0;
			while (ok && remaining)
			{
				DWORD want = remaining < 32 * 1024 ? remaining : 32 * 1024;
				DWORD read = 0;
				if (!ReadFile(h, buf, want, &read, 0) || read != want
					|| crt_fwrite(buf, 1, read, out) != read)
					ok = 0;
				else
					remaining -= read;
			}
			if (buf)
				dash_free(buf);
			crt_fclose(out);
		}
	}
	CloseHandle(h);
	if (!ok && outputStarted)
		DeleteFileA(destination);
	return make_bool_object(ok);
}

// Generic two-string adapters return proper XAP numeric booleans.
void* THISCALL Config_MoveFile(void* pThis, void** args, int count)
{
	(void)pThis;
	const WCHAR *wsrc, *wdst;
	if (!get_two_string_args(args, count, &wsrc, &wdst))
		return make_bool_object(0);
	char src[260], dst[260];
	w2a(wsrc, src, sizeof(src));
	w2a(wdst, dst, sizeof(dst));
	return make_bool_object(MoveFileExA(src, dst, MOVEFILE_REPLACE_EXISTING) ? 1 : 0);
}

// theConfig.CopyFile(src, dst): copy the bytes across. There's no CopyFile in
// this XBE, so we read (ReadFile) and write (CRT stdio, the only writer here).
void* THISCALL Config_CopyFile(void* pThis, void** args, int count)
{
	(void)pThis;
	int succeeded = 0;
	const WCHAR *wsrc, *wdst;
	if (!get_two_string_args(args, count, &wsrc, &wdst))
		return make_bool_object(0);
	static int cacheSet;
	if (!cacheSet) // large writes need a bigger file cache than the tiny default
	{
		XSetFileCacheSize(4 * 1024 * 1024);
		cacheSet = 1;
	}

	char src[260], dst[260];
	w2a(wsrc, src, sizeof(src));
	w2a(wdst, dst, sizeof(dst));

	HANDLE h = CreateFileA(src, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (h == INVALID_HANDLE_VALUE)
		return make_bool_object(0);
	void* f = crt_fopen(dst, "wb", SH_DENYNO);
	if (f)
	{
		char* buf = (char*)dash_malloc(32 * 1024);
		if (buf)
		{
			int ok = 1;
			for (;;)
			{
				DWORD got = 0;
				if (!ReadFile(h, buf, 32 * 1024, &got, 0)) { ok = 0; break; }
				if (!got) break;
				if (crt_fwrite(buf, 1, got, f) != got) { ok = 0; break; }
			}
			succeeded = ok;
			dash_free(buf);
		}
		crt_fclose(f);
	}
	CloseHandle(h);
	if (!succeeded)
		DeleteFileA(dst); // never leave a partial copy looking successful
	return make_bool_object(succeeded);
}
