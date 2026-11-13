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

static DWORD STDCALL warm_extended_partitions(void* unused);
static volatile DWORD s_extended_partitions_initialized;

int THISCALL Config_ExtendedPartitionsInitialized(void* pThis)
{
	(void)pThis;
	return s_extended_partitions_initialized != 0;
}

// Replaces the call at 0002d830.  Retail's initializer establishes C:, Y:,
// and T:; only after it returns is the title-data path safe to use.
void patch_drive_init(void)
{
	((void (*)(void))0x0002dd20)();
	CreateDirectoryA("T:\\UIXLite", 0);
	CreateDirectoryA("T:\\UIXLite\\Skins", 0);
	if (patch_features & FEATURE_FTP)
		ftp_start();
	if (patch_features & FEATURE_DRIVES)
	{
		HANDLE thread = CreateThread(0, 0, (void*)warm_extended_partitions,
		                             0, 0, 0);
		if (thread)
			CloseHandle(thread);
	}
}

// The first XAPI access to an extended partition makes the storage stack do
// its deferred initialization.  FTP root listings already prove that these
// exact probes can absorb that delay on a worker without stalling the UI.
static DWORD STDCALL warm_extended_partitions(void* unused)
{
	(void)unused;
	GetFileAttributesA("N:\\"); // HDD0 partition 6 (scene F:)
	GetFileAttributesA("O:\\"); // HDD0 partition 7 (scene G:)
	GetFileAttributesA("P:\\"); // HDD1 partition 6 (scene F2:)
	GetFileAttributesA("Q:\\"); // HDD1 partition 7 (scene G2:)
	s_extended_partitions_initialized = 1;
	return 0;
}

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

	dash_run();
}
