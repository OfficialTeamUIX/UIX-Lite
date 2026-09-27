// config_functions.c: extra theConfig functions for XAP.
//
// theConfig's function table lookup (0003a490) is patched to jump here. The first call builds a copy
// of retail's table with our entries on the end; every call returns that copy.

#include "dash5960.h"

extern void skin_apply_current(void);
extern int THISCALL Config_LaunchDiscImage(void* pThis, const WCHAR* szPath);
extern int THISCALL Config_DiscImagesSupported(void* pThis);
extern void* THISCALL Config_GetIPAddress(void* pThis);
extern void* THISCALL Config_GetXbeTitleID(void* pThis, const WCHAR* szPath);
extern int THISCALL Config_DeleteFile(void* pThis, const WCHAR* szPath);
extern void THISCALL Config_MoveFile(void* pThis, const WCHAR* szSrc, const WCHAR* szDst);
extern void THISCALL Config_CopyFile(void* pThis, const WCHAR* szSrc, const WCHAR* szDst);

#define MAX_FNS 192

static ScriptFn s_fns[MAX_FNS];
static int s_bBuilt;

static void THISCALL Config_ApplySkin(void* pThis)
{
	(void)pThis;
	skin_apply_current();
}

static const unsigned short s_szApplySkin[] = u"ApplySkin";
static const unsigned short s_szLaunchDiscImage[] = u"LaunchDiscImage";
static const unsigned short s_szDiscImagesSupported[] = u"DiscImagesSupported";
static const unsigned short s_szGetIPAddress[] = u"GetIPAddress";
static const unsigned short s_szGetXbeTitleID[] = u"getXbeTitleID";
static const unsigned short s_szDeleteFile[] = u"deleteFile";
static const unsigned short s_szMoveFile[] = u"moveFile";
static const unsigned short s_szCopyFile[] = u"copyFile";

static void add(int* n, const ScriptFn* like, void* fn, const unsigned short* name)
{
	if (*n >= MAX_FNS - 1)
		return;
	ScriptFn* f = &s_fns[(*n)++];
	*f = *like;
	f->code = (DWORD)fn;
	f->name = (const WCHAR*)name;
}

ScriptFn* patch_config_getfunctionmap(void)
{
	if (!s_bBuilt)
	{
		int n = 0;
		const ScriptFn* voidEntry = 0;
		const ScriptFn* intStringEntry = 0; // sig 25, int f(string), like NtFileExists
		const ScriptFn* intEntry = 0;       // sig 2, int f(void)
		const ScriptFn* stringEntry = 0;    // sig 8, string f(void), like GetXdashVersion
		const ScriptFn* strStrEntry = 0;    // sig 5, string f(string), like Translate
		const ScriptFn* voidSSEntry = 0;    // sig 13, void f(string, string), like SetValue
		for (const ScriptFn* f = dash_config_functions; f->code && n < MAX_FNS - 8; f++)
		{
			s_fns[n++] = *f;
			if (!voidEntry && f->sig == SIG_VOID)
				voidEntry = f;
			if (!intStringEntry && f->sig == SIG_INT_STRING)
				intStringEntry = f;
			if (!intEntry && f->sig == SIG_INT)
				intEntry = f;
			if (!stringEntry && f->sig == SIG_STRING)
				stringEntry = f;
			if (!strStrEntry && f->sig == SIG_STRING_STRING)
				strStrEntry = f;
			if (!voidSSEntry && f->sig == SIG_VOID_2STRING)
				voidSSEntry = f;
		}
		if (voidEntry)
			add(&n, voidEntry, (void*)Config_ApplySkin, s_szApplySkin);
		if (intStringEntry)
			add(&n, intStringEntry, (void*)Config_LaunchDiscImage, s_szLaunchDiscImage);
		if (intEntry)
			add(&n, intEntry, (void*)Config_DiscImagesSupported, s_szDiscImagesSupported);
		if (stringEntry)
			add(&n, stringEntry, (void*)Config_GetIPAddress, s_szGetIPAddress);
		if (intStringEntry)
			add(&n, intStringEntry, (void*)Config_DeleteFile, s_szDeleteFile);
		if (strStrEntry)
			add(&n, strStrEntry, (void*)Config_GetXbeTitleID, s_szGetXbeTitleID);
		if (voidSSEntry)
		{
			add(&n, voidSSEntry, (void*)Config_MoveFile, s_szMoveFile);
			add(&n, voidSSEntry, (void*)Config_CopyFile, s_szCopyFile);
		}
		s_fns[n].code = 0;
		s_bBuilt = 1;
	}
	return s_fns;
}
