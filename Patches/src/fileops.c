// fileops.c: file helpers for XAP -- copy/move/delete a file, and read a
// title's XBE for its TitleID (handy for matching icons without a hand-kept
// Icons.ini). Registered on theConfig alongside the other script functions.

#include "dash5960.h"

// Wide script string -> ASCII path for the file calls.
static void w2a(const WCHAR* w, char* out, int cap)
{
	int i = 0;
	for (; w && w[i] && i < cap - 1; i++)
		out[i] = (char)w[i];
	out[i] = 0;
}

// theConfig.getXbeTitleID(path): the 8-hex TitleID from a title's default.xbe,
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
		BYTE hdr[0x1000];
		DWORD got = 0;
		// XBE header holds the base and the certificate's virtual address; the
		// cert (in the header) has the TitleID at +0x08.
		if (ReadFile(h, hdr, sizeof(hdr), &got, 0) && got > 0x11c && hdr[0] == 'X' && hdr[1] == 'B' && hdr[2] == 'E' && hdr[3] == 'H')
		{
			DWORD base = *(DWORD*)(hdr + 0x104);
			DWORD certVA = *(DWORD*)(hdr + 0x118);
			DWORD certOff = certVA - base;
			if (certOff + 0x0c <= got)
			{
				DWORD tid = *(DWORD*)(hdr + certOff + 8);
				const WCHAR* d = u"0123456789ABCDEF";
				for (int i = 0; i < 8; i++)
					out[i] = d[(tid >> ((7 - i) * 4)) & 0xf];
				out[8] = 0;
			}
		}
		CloseHandle(h);
	}
	return make_strobj(out);
}

// theConfig.deleteFile(path): 1 on success, 0 on failure.
int THISCALL Config_DeleteFile(void* pThis, const WCHAR* wpath)
{
	(void)pThis;
	char path[260];
	w2a(wpath, path, sizeof(path));
	return DeleteFileA(path) ? 1 : 0;
}

// theConfig.moveFile(src, dst): rename/move, replacing an existing dst. Void
// (the VM discards a two-string function's return); check with a follow-up if
// you need to be sure it landed.
void THISCALL Config_MoveFile(void* pThis, const WCHAR* wsrc, const WCHAR* wdst)
{
	(void)pThis;
	char src[260], dst[260];
	w2a(wsrc, src, sizeof(src));
	w2a(wdst, dst, sizeof(dst));
	MoveFileExA(src, dst, MOVEFILE_REPLACE_EXISTING);
}

// theConfig.copyFile(src, dst): copy the bytes across. There's no CopyFile in
// this XBE, so we read (ReadFile) and write (CRT stdio, the only writer here).
void THISCALL Config_CopyFile(void* pThis, const WCHAR* wsrc, const WCHAR* wdst)
{
	(void)pThis;
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
		return;
	void* f = crt_fopen(dst, "wb", SH_DENYNO);
	if (f)
	{
		char* buf = (char*)dash_malloc(32 * 1024);
		if (buf)
		{
			DWORD got = 0;
			while (ReadFile(h, buf, 32 * 1024, &got, 0) && got)
				crt_fwrite(buf, 1, got, f);
			dash_free(buf);
		}
		crt_fclose(f);
	}
	CloseHandle(h);
}
