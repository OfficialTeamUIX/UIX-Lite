// disc_image.c: theConfig.LaunchDiscImage(path), boot a .iso / .cci.
//
// Same Cerbios flow as Theseus's launcher: open the virtual drive, detach
// whatever is mounted, attach the image as \Device\CdRom0, quick reboot.
// Cerbios keeps the mount across the reboot and boots the disc. The path is
// the launcher's NT path (\Device\Harddisk0\Partition1\Games\Halo\default.iso).
// Other BIOSes return false and the script carries on.

#include "dash5960.h"

#define MAX_SLICES 8

typedef struct ATTACH_CERBIOS
{
	BYTE        SliceCount;
	BYTE        DeviceType; // 'D' iso, 'd' cci
	BYTE        Reserved1;
	BYTE        Reserved2;
	ANSI_STRING SliceFile[MAX_SLICES];
	ANSI_STRING MountPoint;
} ATTACH_CERBIOS;

#define IOCTL_VIRTUAL_ATTACH 0x0CE52B01
#define IOCTL_VIRTUAL_DETACH 0x0CE52B02
#define CERBIOS_MIN_BUILD    8008

#define HalQuickRebootRoutine 2

static int lower(int c)
{
	return (c >= 'A' && c <= 'Z') ? c + 32 : c;
}

static BOOL ends_with(const char* s, int n, const char* ext)
{
	int e = 0;
	while (ext[e])
		e++;
	if (n < e)
		return 0;
	for (int i = 0; i < e; i++)
		if (lower(s[n - e + i]) != ext[i])
			return 0;
	return 1;
}

static int open_virtual_drive(HANDLE* h)
{
	ANSI_STRING dev;
	RtlInitAnsiString(&dev, "\\Device\\Virtual0\\Image0");
	OBJECT_ATTRIBUTES oa = { 0, &dev, 0x40 /* OBJ_CASE_INSENSITIVE */ };
	IO_STATUS_BLOCK io;
	// GENERIC_READ | SYNCHRONIZE, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE
	return NtOpenFile(h, 0x80100000, &oa, &io, 1, 0x60) >= 0;
}

static int attach_cerbios(const char* path, BOOL cci)
{
	void* mem = 0;
	DWORD size = 0x10000;
	if (NtAllocateVirtualMemory(&mem, 0, &size, 0x1000 /* MEM_COMMIT */, 4 /* PAGE_READWRITE */) < 0)
		return 0;

	ATTACH_CERBIOS* asd = (ATTACH_CERBIOS*)mem;
	char* str = (char*)(asd + 1);
	asd->SliceCount = 1;
	asd->DeviceType = cci ? 'd' : 'D';

	const char* mount = "\\Device\\CdRom0";
	int n = 0;
	for (; mount[n]; n++)
		str[n] = mount[n];
	str[n] = 0;
	RtlInitAnsiString(&asd->MountPoint, str);
	asd->MountPoint.MaximumLength = asd->MountPoint.Length + 1;
	str += n + 1;

	for (n = 0; path[n]; n++)
		str[n] = path[n];
	str[n] = 0;
	RtlInitAnsiString(&asd->SliceFile[0], str);
	asd->SliceFile[0].MaximumLength = asd->SliceFile[0].Length + 1;

	IO_STATUS_BLOCK io;
	HANDLE h;
	if (!open_virtual_drive(&h))
	{
		OutputDebugStringA("[iso] no virtual drive; not Cerbios?\n");
		return 0;
	}
	NtDeviceIoControlFile(h, 0, 0, 0, &io, IOCTL_VIRTUAL_DETACH, 0, 0, 0, 0);
	int ok = NtDeviceIoControlFile(h, 0, 0, 0, &io, IOCTL_VIRTUAL_ATTACH, asd, sizeof(*asd), 0, 0) >= 0;
	NtClose(h);
	if (!ok)
	{
		OutputDebugStringA("[iso] attach failed\n");
		return 0;
	}
	// the Theseus overlay gives Cerbios a moment before rebooting; ISOs need it
	OutputDebugStringA("[iso] attached, rebooting into the disc\n");
	SleepEx(1500, 0);
	HalReturnToFirmware(HalQuickRebootRoutine);
	return 1;
}

int THISCALL Config_LaunchDiscImage(void* pThis, const WCHAR* szPath)
{
	(void)pThis;
	char path[260];
	int n = 0;
	for (; szPath && szPath[n] && n < (int)sizeof(path) - 1; n++)
		path[n] = (char)szPath[n];
	path[n] = 0;

	BOOL cci = ends_with(path, n, ".cci");
	if (!cci && !ends_with(path, n, ".iso"))
		return 0;
	if (XboxKrnlVersion[2] < CERBIOS_MIN_BUILD)
	{
		OutputDebugStringA("[iso] needs Cerbios\n");
		return 0;
	}
	return attach_cerbios(path, cci);
}

// theConfig.DiscImagesSupported(): Cerbios new enough to have the virtual drive
int THISCALL Config_DiscImagesSupported(void* pThis)
{
	(void)pThis;
	if (XboxKrnlVersion[2] < CERBIOS_MIN_BUILD)
		return 0;
	HANDLE h;
	if (!open_virtual_drive(&h))
		return 0;
	NtClose(h);
	return 1;
}
