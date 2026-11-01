// skin_meshes.c: skin-folder overrides for Mesh { url ... } nodes.
//
// Mesh::Load resolves a relative URL into an archive-qualified virtual path,
// then calls the cache lookup at 0x3dba0. The hook at that boundary saves the
// exact retail path and substitutes a skin file when one exists. ApplySkin
// marks tracked Mesh nodes dirty; a missing skin mesh then reuses the saved
// retail XIP path through the normal cache/loader code.

#include "dash5960.h"

#define SKIN_DIR "T:\\UIXLite\\Skins\\"
#define MAX_TRACKED_MESHES 256
#define MAX_MESH_PATH 260

extern char g_szSkin[64];
extern int skin_is_skinnable(const char* base);
extern int skin_candidates(const char* base, const char** out, int max);

typedef struct TrackedMesh
{
	void* object;
	char base[64];
	WCHAR retailPath[MAX_MESH_PATH];
	DWORD loadedUrl;
	BYTE usingSkin;
} TrackedMesh;

static TrackedMesh s_meshes[MAX_TRACKED_MESHES];
static TrackedMesh* s_loadingMesh;
// Keep the observed reload result so the before/after resource reads used by
// the proven hardware path remain ordered and are not optimized away.
static volatile int s_meshReloadState;

static int lower(int c)
{
	return (c >= 'A' && c <= 'Z') ? c + 32 : c;
}

static DWORD url_signature(const WCHAR* url)
{
	DWORD hash = 2166136261u;
	if (!url || !url[0]) return 0;
	for (; *url; url++)
	{
		DWORD c = *url;
		if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
		hash = (hash ^ c) * 16777619u;
	}
	return hash ? hash : 1;
}

static int ends_xm(const char* name)
{
	int n = 0;
	while (name[n]) n++;
	return n >= 3 && name[n - 3] == '.' && lower(name[n - 2]) == 'x' && lower(name[n - 1]) == 'm';
}

static int base_name(const WCHAR* url, char* out, int cap)
{
	const WCHAR* base = url;
	for (const WCHAR* p = url; p && *p; p++)
		if (*p == '\\' || *p == '/') base = p + 1;
	int n = 0;
	while (base && base[n] && n < cap - 1)
	{
		if (base[n] > 0x7f) return 0;
		out[n] = (char)base[n];
		n++;
	}
	out[n] = 0;
	return ends_xm(out);
}

static void copy_wide(WCHAR* dst, const WCHAR* src, int cap)
{
	int n = 0;
	while (src[n] && n < cap - 1)
	{
		dst[n] = src[n];
		n++;
	}
	dst[n] = 0;
}

static void replace_wide_base(WCHAR* path, const char* base, int cap)
{
	int end = 0;
	int start = 0;
	while (path[end] && end < cap - 1)
	{
		if (path[end] == '\\' || path[end] == '/') start = end + 1;
		end++;
	}
	int n = 0;
	while (base[n] && start + n < cap - 1)
	{
		path[start + n] = (WCHAR)(BYTE)base[n];
		n++;
	}
	path[start + n] = 0;
}

static TrackedMesh* track(void* object, const char* base)
{
	for (int i = 0; i < MAX_TRACKED_MESHES; i++)
		if (s_meshes[i].object == object)
		{
			TrackedMesh* mesh = &s_meshes[i];
			int n = 0;
			while (mesh->base[n] && base[n]
				&& lower(mesh->base[n]) == lower(base[n])) n++;
			if (!mesh->base[n] && !base[n])
				return mesh;

			// The XAP changed this Mesh object's URL. Preserve the original
			// archive-qualified directory: ApplySkin runs outside the Inline's
			// archive context, so resolving the new relative name from scratch
			// would search the wrong XIP. Replace only the saved basename.
			for (n = 0; base[n] && n < (int)sizeof(mesh->base) - 1; n++)
				mesh->base[n] = base[n];
			mesh->base[n] = 0;
			if (mesh->retailPath[0])
				replace_wide_base(mesh->retailPath, base, MAX_MESH_PATH);
			return mesh;
		}
	for (int i = 0; i < MAX_TRACKED_MESHES; i++)
		if (!s_meshes[i].object)
		{
			TrackedMesh* mesh = &s_meshes[i];
			mesh->object = object;
			mesh->retailPath[0] = 0;
			mesh->loadedUrl = 0;
			mesh->usingSkin = 0;
			int n = 0;
			for (; base[n] && n < (int)sizeof(mesh->base) - 1; n++)
				mesh->base[n] = base[n];
			mesh->base[n] = 0;
			return mesh;
		}
	return 0;
}

static void untrack(void* object)
{
	for (int i = 0; i < MAX_TRACKED_MESHES; i++)
		if (s_meshes[i].object == object)
		{
			if (s_loadingMesh == &s_meshes[i]) s_loadingMesh = 0;
			s_meshes[i].object = 0;
			s_meshes[i].retailPath[0] = 0;
			s_meshes[i].loadedUrl = 0;
			s_meshes[i].usingSkin = 0;
		}
}

static int skin_path(const char* file, WCHAR* wide, int cap)
{
	char path[192];
	int n = 0;
	for (const char* p = SKIN_DIR; *p && n < (int)sizeof(path) - 1; p++) path[n++] = *p;
	for (const char* p = g_szSkin; *p && n < (int)sizeof(path) - 1; p++) path[n++] = *p;
	if (n < (int)sizeof(path) - 1) path[n++] = '\\';
	for (const char* p = file; *p && n < (int)sizeof(path) - 1; p++) path[n++] = *p;
	path[n] = 0;
	if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES || n >= cap) return 0;
	for (int i = 0; i <= n; i++) wide[i] = (WCHAR)(BYTE)path[i];
	return 1;
}

void THISCALL patch_mesh_load(void* object, const WCHAR* url)
{
	char base[64];
	if (!url || !base_name(url, base, sizeof(base)))
	{
		dash_mesh_load(object, url);
		return;
	}
	// Track every XM node. Layers can select models that are not part of the
	// small retail alias list, and those nodes must also be reloaded when a
	// skin is reapplied or their URL changes.
	TrackedMesh* previous = s_loadingMesh;
	TrackedMesh* mesh = track(object, base);
	s_loadingMesh = mesh;
	dash_mesh_load(object, url);
	if (mesh)
		mesh->loadedUrl = url_signature(url);
	s_loadingMesh = previous;
}

void* STDCALL patch_mesh_cache_lookup(WCHAR* path, int which)
{
	TrackedMesh* mesh = s_loadingMesh;
	if (mesh)
	{
		if (!mesh->retailPath[0])
			copy_wide(mesh->retailPath, path, MAX_MESH_PATH);

		WCHAR selected[MAX_MESH_PATH];
		int haveSkin = 0;
		if (g_szSkin[0])
		{
			const char* candidates[4];
			int count = skin_candidates(mesh->base, candidates, 4);
			for (int i = 0; i < count && !haveSkin; i++)
				haveSkin = skin_path(candidates[i], selected, MAX_MESH_PATH);
		}
		mesh->usingSkin = (BYTE)haveSkin;
		copy_wide(path, haveSkin ? selected : mesh->retailPath, MAX_MESH_PATH);
	}
	return dash_mesh_cache_lookup(path, which);
}

void STDCALL patch_mesh_destroy(void* object)
{
	untrack(object);
	dash_mesh_destroy(object);
}

void skin_meshes_apply(void)
{
	int dbgChanged = 0;
	int dbgReloaded = 0;
	int dbgResource = 0;
	int dbgResourceChanged = 0;
	// Reload meshes that previously passed through Mesh::Load. Their detailed
	// records preserve the archive-qualified retail path. Do not reload every
	// retail mesh: nodes unrelated to either skin may depend on their original
	// Inline context and can disappear if resolved again from ApplySkin.
	for (int i = 0; i < MAX_TRACKED_MESHES; i++)
		if (s_meshes[i].object)
		{
			TrackedMesh* mesh = &s_meshes[i];
			BYTE* object = (BYTE*)mesh->object;
			WCHAR* url = *(WCHAR**)(object + 0x1c);
			DWORD currentUrl = url_signature(url);
			int urlChanged = currentUrl != mesh->loadedUrl;
			if (urlChanged) dbgChanged++;
			int haveNewSkin = 0;
			if (g_szSkin[0])
			{
				WCHAR selected[MAX_MESH_PATH];
				const char* candidates[4];
				int count = skin_candidates(mesh->base, candidates, 4);
				for (int j = 0; j < count && !haveNewSkin; j++)
					haveNewSkin = skin_path(candidates[j], selected, MAX_MESH_PATH);
			}
			if (!mesh->usingSkin && !haveNewSkin && !urlChanged)
				continue;

			if (url && url[0])
			{
				void* oldResource = *(void**)(object + 0x30);
				object[0x20] = 1;
				dash_mesh_update(object);
				dbgReloaded++;
				if (*(void**)(object + 0x30)) dbgResource++;
				if (*(void**)(object + 0x30) != oldResource) dbgResourceChanged++;
			}
		}
	// Decimal fields stay below 2^24 for the expected small counts, so the XAP
	// numeric representation preserves every digit exactly:
	// C + R*100 + N*10000 + P*1000000.
	if (dbgChanged > 99) dbgChanged = 99;
	if (dbgReloaded > 99) dbgReloaded = 99;
	if (dbgResource > 99) dbgResource = 99;
	if (dbgResourceChanged > 15) dbgResourceChanged = 15;
	s_meshReloadState = dbgChanged + dbgReloaded * 100
		+ dbgResource * 10000 + dbgResourceChanged * 1000000;
}
