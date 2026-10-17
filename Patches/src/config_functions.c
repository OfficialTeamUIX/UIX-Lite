// config_functions.c: extra theConfig functions for XAP.
//
// theConfig's function table lookup (0003a490) is patched to jump here. The first call builds a copy
// of retail's table with our entries on the end; every call returns that copy.

#include "dash5960.h"
#include "uixlite.h"

extern void skin_apply_current(void);
extern int THISCALL Config_LaunchDiscImage(void* pThis, const WCHAR* szPath);
extern void* THISCALL Config_GetDiscImageTitleID(void* pThis, const WCHAR* szPath);
extern void* THISCALL Config_GetDiscImageTitleName(void* pThis, const WCHAR* szPath);
extern void* THISCALL Config_ExportDiscImageTitleImage(void* pThis, void** args, int count);
extern int THISCALL Config_DiscImagesSupported(void* pThis);
extern void* THISCALL Config_GetIPAddress(void* pThis);
extern void* THISCALL Config_GetXbeTitleID(void* pThis, const WCHAR* szPath);
extern void* THISCALL Config_GetXbeTitleName(void* pThis, const WCHAR* szPath);
extern int THISCALL Config_DeleteFile(void* pThis, const WCHAR* szPath);
extern int THISCALL Config_CreateDir(void* pThis, const WCHAR* szPath);
extern int THISCALL Config_RemoveDir(void* pThis, const WCHAR* szPath);
extern void* THISCALL Config_ExportXbeTitleImage(void* pThis, void** args, int count);
extern void* THISCALL Config_MoveFile(void* pThis, void** args, int count);
extern void* THISCALL Config_CopyFile(void* pThis, void** args, int count);
extern void* THISCALL Config_CreateTitleMeta(void* pThis, void** args, int count);
extern int THISCALL Config_ClearCache(void* pThis);
extern int THISCALL Config_SendDiscordRelay(void* pThis, const WCHAR* title);
extern int THISCALL Config_SendDiscordRelayAsync(void* pThis, const WCHAR* title);
extern int THISCALL Config_GetDiscordRelayStatus(void* pThis);

#define MAX_FNS 192

static ScriptFn s_fns[MAX_FNS];
static int s_bBuilt;

// theConfig has no retail string f(string) member to clone.  Signature 5 is
// nevertheless supported by the XAP dispatcher and is used by Translate on
// another dashboard script object.  Its member-pointer adjustors are all zero.
static const ScriptFn s_stringStringEntry =
{
	0, { 0, 0, 0 }, SIG_STRING_STRING, 0, 0
};

// Signature 0 is CObject* f(CObject** args, int count). It is the retail VM's
// generic adapter and lets this two-string function return a numeric boolean.
static const ScriptFn s_genericEntry =
{
	0, { 0, 0, 0 }, SIG_DEFAULT, 0, 0
};

static void THISCALL Config_ApplySkin(void* pThis)
{
	(void)pThis;
	skin_apply_current();
}
static const unsigned short s_szApplySkin[] = u"ApplySkin";
static const unsigned short s_szLaunchDiscImage[] = u"LaunchDiscImage";
static const unsigned short s_szGetDiscImageTitleID[] = u"GetDiscImageTitleID";
static const unsigned short s_szGetDiscImageTitleName[] = u"GetDiscImageTitleName";
static const unsigned short s_szExportDiscImageTitleImage[] = u"ExportDiscImageTitleImage";
static const unsigned short s_szDiscImagesSupported[] = u"DiscImagesSupported";
static const unsigned short s_szGetIPAddress[] = u"GetIPAddress";
static const unsigned short s_szGetXbeTitleID[] = u"GetXbeTitleID";
static const unsigned short s_szGetXbeTitleName[] = u"GetXbeTitleName";
static const unsigned short s_szDeleteFile[] = u"DeleteFile";
static const unsigned short s_szCreateDir[] = u"CreateDir";
static const unsigned short s_szRemoveDir[] = u"RemoveDir";
static const unsigned short s_szExportXbeTitleImage[] = u"ExportXbeTitleImage";
static const unsigned short s_szMoveFile[] = u"MoveFile";
static const unsigned short s_szCopyFile[] = u"CopyFile";
static const unsigned short s_szCreateTitleMeta[] = u"CreateTitleMeta";
static const unsigned short s_szClearCache[] = u"ClearCache";
static const unsigned short s_szSendDiscordRelay[] = u"SendDiscordRelay";
static const unsigned short s_szSendDiscordRelayAsync[] = u"SendDiscordRelayAsync";
static const unsigned short s_szGetDiscordRelayStatus[] = u"GetDiscordRelayStatus";

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
		// Reserve twenty possible extension entries plus the terminating zero entry.
		for (const ScriptFn* f = dash_config_functions; f->code && n < MAX_FNS - 21; f++)
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
		}
		if ((patch_features & FEATURE_SKINS) && voidEntry)
			add(&n, voidEntry, (void*)Config_ApplySkin, s_szApplySkin);
		if ((patch_features & FEATURE_DISCIMAGES) && intStringEntry)
			add(&n, intStringEntry, (void*)Config_LaunchDiscImage, s_szLaunchDiscImage);
		if ((patch_features & FEATURE_DISCIMAGES) && intEntry)
			add(&n, intEntry, (void*)Config_DiscImagesSupported, s_szDiscImagesSupported);
		if (patch_features & FEATURE_DISCIMAGES)
		{
			const ScriptFn* stringString = strStrEntry ? strStrEntry : &s_stringStringEntry;
			add(&n, stringString, (void*)Config_GetDiscImageTitleID, s_szGetDiscImageTitleID);
			add(&n, stringString, (void*)Config_GetDiscImageTitleName, s_szGetDiscImageTitleName);
			add(&n, &s_genericEntry, (void*)Config_ExportDiscImageTitleImage, s_szExportDiscImageTitleImage);
		}
		if ((patch_features & FEATURE_CONFIG) && stringEntry)
			add(&n, stringEntry, (void*)Config_GetIPAddress, s_szGetIPAddress);
		if ((patch_features & FEATURE_CONFIG) && intStringEntry)
		{
			add(&n, intStringEntry, (void*)Config_DeleteFile, s_szDeleteFile);
			add(&n, intStringEntry, (void*)Config_CreateDir, s_szCreateDir);
			add(&n, intStringEntry, (void*)Config_RemoveDir, s_szRemoveDir);
		}
		if (patch_features & FEATURE_CONFIG)
		{
			const ScriptFn* stringString = strStrEntry ? strStrEntry : &s_stringStringEntry;
			add(&n, stringString, (void*)Config_GetXbeTitleID, s_szGetXbeTitleID);
			add(&n, stringString, (void*)Config_GetXbeTitleName, s_szGetXbeTitleName);
			add(&n, &s_genericEntry, (void*)Config_ExportXbeTitleImage, s_szExportXbeTitleImage);
		}
		if (patch_features & FEATURE_CONFIG)
		{
			add(&n, &s_genericEntry, (void*)Config_MoveFile, s_szMoveFile);
			add(&n, &s_genericEntry, (void*)Config_CopyFile, s_szCopyFile);
			add(&n, &s_genericEntry, (void*)Config_CreateTitleMeta, s_szCreateTitleMeta);
		}
		if ((patch_features & FEATURE_CONFIG) && intStringEntry)
		{
			add(&n, intStringEntry, (void*)Config_SendDiscordRelay, s_szSendDiscordRelay);
			add(&n, intStringEntry, (void*)Config_SendDiscordRelayAsync, s_szSendDiscordRelayAsync);
		}
		if ((patch_features & FEATURE_CONFIG) && intEntry)
		{
			add(&n, intEntry, (void*)Config_ClearCache, s_szClearCache);
			add(&n, intEntry, (void*)Config_GetDiscordRelayStatus, s_szGetDiscordRelayStatus);
		}
		s_fns[n].code = 0;
		s_bBuilt = 1;
	}
	return s_fns;
}
