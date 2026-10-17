// drives.c: drive letters for the partitions retail never maps.
//
// The optical drive uses B: internally because the retail dashboard has D:
// mapped to HDD0 partition 2. Cache partitions 3/4/5 use S:/V:/W: internally so UIX can present them as
// X:/Y:/Z: without replacing the dashboard's own X: and Y: links. F: and G:
// (HDD0 partitions 6 and 7) show up as N: and O:, and a second hard drive's
// E:/F:/G: as R:/P:/Q:. Runs before the dashboard starts.

#include "dash5960.h"
#include "uixlite.h"
#include "ftp.h"

extern void disc_image_detach_on_dashboard_start(void);

static const struct
{
	char        letter;
	const char* device;
} kDrives[] = {
	{ 'B', "\\Device\\CdRom0" },
	{ 'S', "\\Device\\Harddisk0\\Partition3" },
	{ 'V', "\\Device\\Harddisk0\\Partition4" },
	{ 'W', "\\Device\\Harddisk0\\Partition5" },
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
	if (patch_features & FEATURE_DISCIMAGES)
	{
		disc_image_detach_on_dashboard_start();
		if (!(patch_features & FEATURE_DRIVES))
			mount('B', "\\Device\\CdRom0");
	}

	if (patch_features & FEATURE_DRIVES)
		for (unsigned i = 0; i < sizeof(kDrives) / sizeof(kDrives[0]); i++)
			mount(kDrives[i].letter, kDrives[i].device);

	// Start the bootstrap thread here. It waits for readiness, then reads all
	// FTP settings (including Enabled) before it creates the listener.
	if (patch_features & FEATURE_FTP)
		ftp_start();

	dash_run();
}
