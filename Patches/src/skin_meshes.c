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
} TrackedMesh;

static TrackedMesh s_meshes[MAX_TRACKED_MESHES];
static TrackedMesh* s_loadingMesh;

static int lower(int c)
{
	return (c >= 'A' && c <= 'Z') ? c + 32 : c;
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
	s_loadingMesh = track(object, base);
	dash_mesh_load(object, url);
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
	for (int i = 0; i < MAX_TRACKED_MESHES; i++)
		if (s_meshes[i].object)
		{
			BYTE* object = (BYTE*)s_meshes[i].object;
			WCHAR* url = *(WCHAR**)(object + 0x1c);
			if (url && url[0])
			{
				// Hidden or fully transparent Mesh nodes are skipped by normal scene
				// traversal. Force the retail update routine so a changed URL is
				// resolved even when the node is not currently being rendered. This
				// also runs the Mesh post-load callback and clears the dirty flag.
				object[0x20] = 1;
				dash_mesh_update(object);
			}
		}
}
