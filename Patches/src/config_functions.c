// config_functions.c: extra theConfig functions for XAP.
//
// theConfig's function table lookup (0003a490) is patched to jump here. The first call builds a copy
// of retail's table with our entries on the end; every call returns that copy.

#include "dash5960.h"

extern void skin_apply_current(void);
extern int THISCALL Config_LaunchDiscImage(void* pThis, const WCHAR* szPath);
extern int THISCALL Config_DiscImagesSupported(void* pThis);
extern void* THISCALL Config_GetIPAddress(void* pThis);

#define MAX_FNS 160

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
		}
		if (voidEntry)
			add(&n, voidEntry, (void*)Config_ApplySkin, s_szApplySkin);
		if (intStringEntry)
			add(&n, intStringEntry, (void*)Config_LaunchDiscImage, s_szLaunchDiscImage);
		if (intEntry)
			add(&n, intEntry, (void*)Config_DiscImagesSupported, s_szDiscImagesSupported);
		if (stringEntry)
			add(&n, stringEntry, (void*)Config_GetIPAddress, s_szGetIPAddress);
		s_fns[n].code = 0;
		s_bBuilt = 1;
	}
	return s_fns;
}
