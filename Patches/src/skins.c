// skins.c: Theseus-style color skins on the retail material table.
//
// Retail colors stay as the dashboard built them; a skin only overrides the
// materials it lists. Same .xbx format as Theseus: [Material] then
// Color=r,g,b,a for solid/backing/modulate, ColorA/ColorB for the falloff
// family. Keys, EggGlow and EggGlowPulse pick their colors in Setup every
// frame, so they're left alone for now.

#include "dash5960.h"

#define CONFIG_PATH "Y:\\UIX Configs\\config.ini"
#define MAX_MATERIALS 256

// Retail colors as the dashboard built them, so each skin starts from retail
static DWORD s_retail[MAX_MATERIALS][2];
static int s_nRetail;
#define SKIN_DIR    "Y:\\UIX Configs\\Skins\\"

char g_szSkin[64];

char* skin_read_file(const char* path, DWORD* pSize)
{
	HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (h == INVALID_HANDLE_VALUE)
		return 0;

	DWORD size = GetFileSize(h, 0);
	char* buf = 0;
	if (size != 0xFFFFFFFF && size < 0x400000)
	{
		buf = (char*)dash_malloc(size + 1);
		DWORD got = 0;
		if (buf && ReadFile(h, buf, size, &got, 0))
		{
			buf[got] = 0;
			if (pSize)
				*pSize = got;
		}
		else if (buf)
		{
			dash_free(buf);
			buf = 0;
		}
	}
	CloseHandle(h);
	return buf;
}

static int lower(int c)
{
	return (c >= 'A' && c <= 'Z') ? c + 32 : c;
}

// ASCII section name vs the material's wide name. Retail has names that differ
// only in case (XBOXgreen / XBoxGreen), so exact case is tried first.
static BOOL name_matches(const char* a, int n, const WCHAR* w, BOOL anyCase)
{
	for (int i = 0; i < n; i++, w++)
	{
		if (!*w)
			return 0;
		if (anyCase ? lower(a[i]) != lower(*w) : a[i] != *w)
			return 0;
	}
	return *w == 0;
}

static BOOL key_is(const char* line, int n, const char* key)
{
	int i = 0;
	for (; key[i]; i++)
	{
		if (i >= n || lower(line[i]) != lower(key[i]))
			return 0;
	}
	while (i < n && (line[i] == ' ' || line[i] == '\t'))
		i++;
	return i < n && line[i] == '=';
}

// "r, g, b, a" after the '='; needs at least three numbers, alpha defaults to 0 (as Theseus)
static BOOL parse_rgba(const char* p, const char* end, DWORD* out)
{
	while (p < end && *p != '=')
		p++;
	int v[4] = { 0, 0, 0, 0 };
	int count = 0;
	while (p < end && count < 4)
	{
		if (*p >= '0' && *p <= '9')
		{
			int n = 0;
			while (p < end && *p >= '0' && *p <= '9')
				n = n * 10 + (*p++ - '0');
			v[count++] = n & 255;
		}
		else
			p++;
	}
	if (count < 3)
		return 0;
	*out = ((DWORD)v[3] << 24) | ((DWORD)v[0] << 16) | ((DWORD)v[1] << 8) | (DWORD)v[2];
	return 1;
}

static Material* find_material(const char* name, int n)
{
	for (int pass = 0; pass < 2; pass++)
	{
		for (int i = 0; i < dash_material_count; i++)
		{
			if (name_matches(name, n, dash_materials[i]->name, pass))
				return dash_materials[i];
		}
	}
	return 0;
}

static void set_color(Material* m, int which, DWORD argb)
{
	DWORD vt = (DWORD)m->vtable;
	if (vt == MAT_SOLID || vt == MAT_BACKING || vt == MAT_MODULATE)
	{
		if (which != 0)
			return;
		// stored as r, g, b, a bytes
		BYTE* b = (BYTE*)&m->colorA;
		b[0] = (BYTE)(argb >> 16);
		b[1] = (BYTE)(argb >> 8);
		b[2] = (BYTE)argb;
		b[3] = (BYTE)(argb >> 24);
	}
	else if (vt == MAT_FALLOFF || vt == MAT_FALLOFFTEX || vt == MAT_INNERWALL || vt == MAT_ANISO)
	{
		if (which == 1)
			m->colorA = argb;
		else if (which == 2)
			m->colorB = argb;
	}
}

static void apply_skin_text(const char* text)
{
	Material* cur = 0;
	const char* p = text;
	while (*p)
	{
		const char* line = p;
		while (*p && *p != '\n')
			p++;
		const char* end = p;
		if (*p)
			p++;
		while (end > line && (end[-1] == '\r' || end[-1] == ' ' || end[-1] == '\t'))
			end--;
		while (line < end && (*line == ' ' || *line == '\t'))
			line++;
		int n = (int)(end - line);
		if (n <= 0 || *line == '#' || *line == ';')
			continue;

		if (*line == '[')
		{
			const char* close = line + 1;
			while (close < end && *close != ']')
				close++;
			cur = find_material(line + 1, (int)(close - line - 1));
			continue;
		}
		if (!cur)
			continue;

		DWORD argb;
		if (key_is(line, n, "Color") && parse_rgba(line, end, &argb))
			set_color(cur, 0, argb);
		else if (key_is(line, n, "ColorA") && parse_rgba(line, end, &argb))
			set_color(cur, 1, argb);
		else if (key_is(line, n, "ColorB") && parse_rgba(line, end, &argb))
			set_color(cur, 2, argb);
	}
}

// Value of key in [section] of an ini, copied into out (empty if missing)
void ini_value(const char* text, const char* section, const char* key, char* out, int cap)
{
	BOOL in = 0;
	int sl = 0;
	while (section[sl])
		sl++;
	out[0] = 0;
	const char* p = text;
	while (*p)
	{
		const char* line = p;
		while (*p && *p != '\n')
			p++;
		const char* end = p;
		if (*p)
			p++;
		while (end > line && (end[-1] == '\r' || end[-1] == ' '))
			end--;
		while (line < end && (*line == ' ' || *line == '\t'))
			line++;
		int n = (int)(end - line);
		if (n <= 0)
			continue;
		if (*line == '[')
		{
			in = n >= sl + 2 && line[sl + 1] == ']';
			for (int i = 0; in && i < sl; i++)
				in = lower(line[i + 1]) == lower(section[i]);
			continue;
		}
		if (in && key_is(line, n, key))
		{
			const char* v = line;
			while (*v != '=')
				v++;
			v++;
			while (v < end && *v == ' ')
				v++;
			int i = 0;
			while (v < end && i < cap - 1)
				out[i++] = *v++;
			out[i] = 0;
			return;
		}
	}
}

static void snapshot_retail(void)
{
	s_nRetail = dash_material_count < MAX_MATERIALS ? dash_material_count : MAX_MATERIALS;
	for (int i = 0; i < s_nRetail; i++)
	{
		s_retail[i][0] = dash_materials[i]->colorA;
		s_retail[i][1] = dash_materials[i]->colorB;
	}
}

static void restore_retail(void)
{
	for (int i = 0; i < s_nRetail; i++)
	{
		DWORD vt = (DWORD)dash_materials[i]->vtable;
		if (vt == MAT_SOLID || vt == MAT_BACKING || vt == MAT_MODULATE || vt == MAT_FALLOFF
			|| vt == MAT_FALLOFFTEX || vt == MAT_INNERWALL || vt == MAT_ANISO)
		{
			dash_materials[i]->colorA = s_retail[i][0];
			dash_materials[i]->colorB = s_retail[i][1];
		}
	}
}

void skin_textures_apply(void);

// Retail colors, then Skins\<name>.xbx on top; an empty name leaves retail
int skin_apply(const char* name)
{
	restore_retail();
	int k = 0;
	for (; name[k] && k < 63; k++)
		g_szSkin[k] = name[k];
	g_szSkin[k] = 0;
	skin_textures_apply();
	if (!name[0])
		return 1;

	char path[160];
	int i = 0;
	for (const char* s = SKIN_DIR; *s && i < 150; s++)
		path[i++] = *s;
	// Theseus layout: Skins\<Name>\<Name>.xbx, with the skin's textures beside it
	for (const char* s = name; *s && i < 100; s++)
		path[i++] = *s;
	path[i++] = '\\';
	for (const char* s = name; *s && i < 150; s++)
		path[i++] = *s;
	for (const char* s = ".xbx"; *s && i < 159; s++)
		path[i++] = *s;
	path[i] = 0;

	char* text = skin_read_file(path, 0);
	if (!text)
	{
		OutputDebugStringA("[skins] skin file not found\n");
		return 0;
	}
	apply_skin_text(text);
	dash_free(text);
	OutputDebugStringA("[skins] skin applied\n");
	return 1;
}

void skin_apply_current(void);

// Replaces the startup call to dash_init_materials (0002d89d)
void patch_material_init(void)
{
	dash_init_materials();
	snapshot_retail();

	skin_apply_current();
}

// Theseus's key: [Dashboard Settings] Current Skin
void skin_apply_current(void)
{
	char name[64];
	name[0] = 0;
	char* cfg = skin_read_file(CONFIG_PATH, 0);
	if (cfg)
	{
		ini_value(cfg, "Dashboard Settings", "Current Skin", name, sizeof(name));
		dash_free(cfg);
	}
	skin_apply(name);
}
