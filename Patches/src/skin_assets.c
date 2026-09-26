// skin_assets.c: which files a skin folder may override, and the names to try.
// Port of Theseus's render/skin_assets.h, same list and groups.

#include "dash5960.h"

static int lower(int c)
{
	return (c >= 'A' && c <= 'Z') ? c + 32 : c;
}

static int ieq(const char* a, const char* b)
{
	while (*a && *b && lower(*a) == lower(*b))
		a++, b++;
	return *a == 0 && *b == 0;
}

static const char* const kGroups[][4] = {
	{ "cellwall.xm",       "Inner_cell-FACES.xm", 0,                  0 },
	{ "cellwall.xbx",      "shell.xbx",           0,                  0 },
	{ "GameHilite_01.xbx", "menu_hilite.xbx",     "menu_hilight.xbx", 0 },
};

static const char* const kSkinnable[] = {
	"cellwall.xbx", "dvd_button.xbx", "DVD_paneltex.xbx", "dvdaudio.xbx", "dvdempty.xbx",
	"dvdstop.xbx", "dvdstopw.xbx", "dvdtitle.xbx", "dvdunknown.xbx", "dvdvideo.xbx",
	"GameHilite_01.xbx", "menu_hilite.xbx", "menu_hilight.xbx", "outline.xbx", "screenshot.xbx",
	"shell.xbx", "status_gauge.xbx", "xbox4.xbx", "xboxlogo.xbx", "xboxlogo64.xbx",
	"xboxlogo128.xbx", "xboxlogow.xbx", "cellwall.xm", "Inner_cell-FACES.xm", 0
};

int skin_is_skinnable(const char* base)
{
	for (int i = 0; kSkinnable[i]; i++)
		if (ieq(base, kSkinnable[i]))
			return 1;
	return 0;
}

// Requested name first, then the rest of its group
int skin_candidates(const char* base, const char** out, int max)
{
	int n = 0;
	out[n++] = base;
	for (unsigned i = 0; i < sizeof(kGroups) / sizeof(kGroups[0]); i++)
	{
		int in = 0;
		for (int j = 0; j < 4 && kGroups[i][j]; j++)
			in |= ieq(base, kGroups[i][j]);
		if (!in)
			continue;
		for (int j = 0; j < 4 && kGroups[i][j] && n < max; j++)
			if (!ieq(base, kGroups[i][j]))
				out[n++] = kGroups[i][j];
		break;
	}
	return n;
}
