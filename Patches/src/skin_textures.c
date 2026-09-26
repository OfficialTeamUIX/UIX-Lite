// skin_textures.c: skin folder textures, live.
//
// On a texture cache miss the dashboard looks the texture up in the loaded
// XIPs (dash_texture_from_xip, url in ecx). That call (0006172f) comes here. Skinnable textures still come
// from the XIP, so every node shares the XIP's object; we remember it and its
// stock image, then point it at the skin's image (parsed with the dashboard's own
// texture loader). Switching skins just repoints the same objects.
//
// Ownership never moves: the XIP object keeps its stock data, the skin
// texture keeps the skin data, and we only copy the Data/Format/Size fields.

#include "dash5960.h"

extern char g_szSkin[64];
extern int skin_is_skinnable(const char* base);
extern int skin_candidates(const char* base, const char** out, int max);
extern char* skin_read_file(const char* path, DWORD* size);


// D3DResource / D3DPixelContainer
typedef struct Tex
{
	DWORD common; // refcount in the low word, type in 0x70000
	DWORD data;
	DWORD lock;
	DWORD format;
	DWORD size;
} Tex;

#define TYPE_TEXTURE 0x40000

typedef struct Tracked
{
	Tex*  live;      // the XIP's object every node holds
	DWORD home[3];   // its stock data, format, size
	Tex*  skin;      // skin texture whose image live shows, or 0
	char  base[40];
} Tracked;

#define MAX_TRACKED 48
static Tracked s_tex[MAX_TRACKED];

static void* parse_texture(const WCHAR* url, void* buf, DWORD size)
{
	void* tex;
	DWORD fn = DASH_PARSE_TEXTURE, zero = 0;
	__asm__ volatile(
		"pushl %[h]\n\t"
		"pushl %[w]\n\t"
		"pushl %[sz]\n\t"
		"call *%[fn]\n\t"
		: "=a"(tex), "+b"(buf), [fn] "+d"(fn)
		: "a"(url), [sz] "m"(size), [w] "m"(zero), [h] "m"(zero)
		: "ecx", "memory");
	return tex;
}

// wide url -> ascii basename, with the extension forced to .xbx like Theseus
static int base_name(const WCHAR* url, char* out, int cap)
{
	const WCHAR* b = url;
	for (const WCHAR* p = url; *p; p++)
		if (*p == '\\' || *p == '/')
			b = p + 1;
	int n = 0;
	int dot = -1;
	for (; b[n] && n < cap - 5; n++)
	{
		out[n] = (char)b[n];
		if (b[n] == '.')
			dot = n;
	}
	if (dot >= 0)
		n = dot;
	out[n++] = '.';
	out[n++] = 'x';
	out[n++] = 'b';
	out[n++] = 'x';
	out[n] = 0;
	return n;
}

void* skin_load_texture(const char* file)
{
	char path[200];
	int i = 0;
	for (const char* s = "Y:\\UIX Configs\\Skins\\"; *s; s++)
		path[i++] = *s;
	for (const char* s = g_szSkin; *s && i < 120; s++)
		path[i++] = *s;
	path[i++] = '\\';
	for (const char* s = file; *s && i < 199; s++)
		path[i++] = *s;
	path[i] = 0;

	DWORD size;
	char* buf = skin_read_file(path, &size);
	if (!buf)
		return 0;
	WCHAR wide[64];
	int k = 0;
	for (; file[k] && k < 63; k++)
		wide[k] = (WCHAR)file[k];
	wide[k] = 0;
	void* tex = parse_texture(wide, buf, size);
	dash_free(buf);
	return tex;
}

static Tex* load_for_skin(const char* base)
{
	if (!g_szSkin[0])
		return 0;
	const char* cand[4];
	int n = skin_candidates(base, cand, 4);
	for (int i = 0; i < n; i++)
	{
		Tex* t = (Tex*)skin_load_texture(cand[i]);
		if (t && (t->common & 0x70000) == TYPE_TEXTURE)
			return t;
		if (t)
			D3DResource_Release(t);
	}
	return 0;
}

// Back to the stock image and drop the skin texture; the GPU has to be done with live first
static void unskin(Tracked* e)
{
	if (!e->skin)
		return;
	D3D_BlockOnResource(e->live);
	e->live->data = e->home[0];
	e->live->format = e->home[1];
	e->live->size = e->home[2];
	D3DResource_Release(e->skin);
	e->skin = 0;
}

static void skin_entry(Tracked* e)
{
	Tex* t = load_for_skin(e->base);
	if (!t)
		return;
	D3D_BlockOnResource(e->live);
	e->live->data = t->data;
	e->live->format = t->format;
	e->live->size = t->size;
	e->skin = t;
}

// Entries only we still hold: the XIP went away and its stock data may be gone
// too. Drop the skin image and our ref without letting D3D free anything.
static void prune(void)
{
	for (int i = 0; i < MAX_TRACKED; i++)
	{
		Tracked* e = &s_tex[i];
		if (e->live && (e->live->common & 0xffff) == 1)
		{
			if (e->skin)
				D3DResource_Release(e->skin);
			e->live->common--;
			e->live = 0;
			e->skin = 0;
		}
	}
}

static Tracked* track(Tex* live, const char* base)
{
	for (int i = 0; i < MAX_TRACKED; i++)
		if (s_tex[i].live == live)
			return 0;
	prune();
	for (int i = 0; i < MAX_TRACKED; i++)
	{
		Tracked* e = &s_tex[i];
		if (e->live)
			continue;
		D3DResource_AddRef(live);
		e->live = live;
		e->home[0] = live->data;
		e->home[1] = live->format;
		e->home[2] = live->size;
		e->skin = 0;
		int k = 0;
		for (; base[k] && k < (int)sizeof(e->base) - 1; k++)
			e->base[k] = base[k];
		e->base[k] = 0;
		return e;
	}
	return 0;
}

// Called by skin_apply after g_szSkin changes
void skin_textures_apply(void)
{
	prune();
	for (int i = 0; i < MAX_TRACKED; i++)
	{
		if (!s_tex[i].live)
			continue;
		unskin(&s_tex[i]);
		skin_entry(&s_tex[i]);
	}
}

void* __attribute__((fastcall)) patch_texture_from_xip(const WCHAR* url)
{
	Tex* tex = (Tex*)dash_texture_from_xip(url);
	if (!url)
		return tex;

	char base[64];
	base_name(url, base, sizeof(base));
	if (!skin_is_skinnable(base))
		return tex;

	// no stock copy to hang it on: hand the skin's texture over as is
	if (!tex || (tex->common & 0x70000) != TYPE_TEXTURE)
	{
		Tex* t = load_for_skin(base);
		if (t)
		{
			if (tex)
				D3DResource_Release(tex);
			return t;
		}
		return tex;
	}

	Tracked* e = track(tex, base);
	if (e)
		skin_entry(e);
	return tex;
}
