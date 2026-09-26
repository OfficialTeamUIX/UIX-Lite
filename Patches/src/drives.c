// drives.c: drive letters for the partitions retail never maps.
//
// F: and G: (HDD0 partitions 6 and 7) show up as N: and O:, and a second
// hard drive's E:/F:/G: as R:/P:/Q:, since the dashboard keeps F: and G: for
// itself. Runs before the dashboard starts.

#include "dash5960.h"
#include "uixlite.h"
#include "ftp.h"

static const struct
{
	char        letter;
	const char* device;
} kDrives[] = {
	{ 'N', "\\Device\\Harddisk0\\Partition6" },
	{ 'O', "\\Device\\Harddisk0\\Partition7" },
	{ 'P', "\\Device\\Harddisk1\\Partition6" },
	{ 'Q', "\\Device\\Harddisk1\\Partition7" },
	{ 'R', "\\Device\\Harddisk1\\Partition1" },
};

static void mount(char letter, const char* device)
{
	char link[] = "\\??\\X:";
	link[4] = letter;
	ANSI_STRING l, d;
	RtlInitAnsiString(&l, link);
	RtlInitAnsiString(&d, device);
	IoCreateSymbolicLink(&l, &d);
}

// patch.py writes the selected features here
DWORD patch_features = 0xFFFFFFFF;

// Replaces main's call to dash_run (0002dca1)
void patch_start(void)
{
	if (patch_features & FEATURE_DRIVES)
		for (unsigned i = 0; i < sizeof(kDrives) / sizeof(kDrives[0]); i++)
			mount(kDrives[i].letter, kDrives[i].device);

	// FTP: on unless config.ini turns it off. The listener waits for the
	// network stack itself, so starting here (before the dashboard runs) is fine.
	if (patch_features & FEATURE_FTP)
	{
		char* cfg = skin_read_file(CONFIG_INI, 0);
		char enabled[8] = "1";
		if (cfg)
		{
			ini_value(cfg, "FTP", "Enabled", enabled, sizeof(enabled));
			dash_free(cfg);
		}
		if (enabled[0] != '0')
			ftp_start();
	}

	dash_run();
}
