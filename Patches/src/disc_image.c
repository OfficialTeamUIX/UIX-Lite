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

static int detach_cerbios(void)
{
	IO_STATUS_BLOCK io;
	HANDLE h;
	if (!open_virtual_drive(&h))
		return 0;
	int ok = NtDeviceIoControlFile(h, 0, 0, 0, &io,
		IOCTL_VIRTUAL_DETACH, 0, 0, 0, 0) >= 0;
	NtClose(h);
	return ok;
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
		DWORD freeSize = 0;
		NtFreeVirtualMemory(&mem, &freeSize, 0x8000 /* MEM_RELEASE */);
		return 0;
	}
	NtDeviceIoControlFile(h, 0, 0, 0, &io, IOCTL_VIRTUAL_DETACH, 0, 0, 0, 0);
	int ok = NtDeviceIoControlFile(h, 0, 0, 0, &io, IOCTL_VIRTUAL_ATTACH, asd, sizeof(*asd), 0, 0) >= 0;
	NtClose(h);
	if (!ok)
	{
		OutputDebugStringA("[iso] attach failed\n");
		DWORD freeSize = 0;
		NtFreeVirtualMemory(&mem, &freeSize, 0x8000 /* MEM_RELEASE */);
		return 0;
	}
	// Cerbios consumes the attach descriptor asynchronously after the IOCTL.
	// Keep its strings alive while the virtual disc settles before rebooting.
	SleepEx(1500, 0);
	OutputDebugStringA("[iso] attached, rebooting into the disc\n");
	HalReturnToFirmware(HalQuickRebootRoutine);
	// HalReturnToFirmware normally never returns.
	DWORD freeSize = 0;
	NtFreeVirtualMemory(&mem, &freeSize, 0x8000 /* MEM_RELEASE */);
	return 0;
}

static int get_image_path(const WCHAR* szPath, char* path, int cap, BOOL* cci)
{
	int n = 0;
	for (; szPath && szPath[n] && n < cap - 1; n++)
		path[n] = (char)szPath[n];
	path[n] = 0;
	*cci = ends_with(path, n, ".cci");
	return *cci || ends_with(path, n, ".iso");
}

int THISCALL Config_LaunchDiscImage(void* pThis, const WCHAR* szPath)
{
	(void)pThis;
	char path[260];
	BOOL cci;
	if (!get_image_path(szPath, path, sizeof(path), &cci))
		return 0;
	if (XboxKrnlVersion[2] < CERBIOS_MIN_BUILD)
	{
		OutputDebugStringA("[iso] needs Cerbios\n");
		return 0;
	}
	return attach_cerbios(path, cci);
}

// Called when the dashboard starts after a mounted title exits. The initial
// attach reboot boots the disc instead of reaching this dashboard code.
void disc_image_detach_on_dashboard_start(void)
{
	if (XboxKrnlVersion[2] >= CERBIOS_MIN_BUILD)
		detach_cerbios();
}

typedef struct IMAGE_READER
{
	HANDLE File;
	BOOL Cci;
	unsigned long long UncompressedSize;
	unsigned long long IndexOffset;
} IMAGE_READER;

static int read_file_at(HANDLE h, unsigned long long off, void* data, DWORD size)
{
	int high = (int)(off >> 32);
	DWORD low = SetFilePointer(h, (int)(DWORD)off, &high, FILE_BEGIN);
	if (low == INVALID_SET_FILE_POINTER && GetLastError() != 0)
		return 0;
	DWORD got = 0;
	return ReadFile(h, data, size, &got, 0) && got == size;
}

static unsigned long long read_u64(const BYTE* p)
{
	return (unsigned long long)*(const DWORD*)p
		| ((unsigned long long)*(const DWORD*)(p + 4) << 32);
}

// Decode one independent LZ4 block. CCI stores each 2048-byte sector as a
// separate block, so no frame header or cross-sector dictionary is involved.
static int lz4_decode_sector(const BYTE* src, DWORD srcSize, BYTE* dst)
{
	const BYTE* ip = src;
	const BYTE* end = src + srcSize;
	BYTE* op = dst;
	BYTE* outEnd = dst + 2048;
	while (ip < end)
	{
		BYTE token = *ip++;
		DWORD literals = token >> 4;
		if (literals == 15)
		{
			BYTE more;
			do {
				if (ip >= end) return 0;
				more = *ip++;
				if (literals > 0xffffffff - more) return 0;
				literals += more;
			} while (more == 255);
		}
		if (literals > (DWORD)(end - ip) || literals > (DWORD)(outEnd - op))
			return 0;
		for (DWORD i = 0; i < literals; i++)
			*op++ = *ip++;
		if (ip == end)
			return op == outEnd;
		if (end - ip < 2)
			return 0;
		DWORD offset = ip[0] | ((DWORD)ip[1] << 8);
		ip += 2;
		if (!offset || offset > (DWORD)(op - dst))
			return 0;
		DWORD match = token & 15;
		if (match == 15)
		{
			BYTE more;
			do {
				if (ip >= end) return 0;
				more = *ip++;
				if (match > 0xffffffff - more) return 0;
				match += more;
			} while (more == 255);
		}
		if (match > 0xffffffff - 4)
			return 0;
		match += 4;
		if (match > (DWORD)(outEnd - op))
			return 0;
		BYTE* copy = op - offset;
		for (DWORD i = 0; i < match; i++)
			*op++ = *copy++;
	}
	return op == outEnd;
}

static int open_image(IMAGE_READER* image, const char* path, BOOL cci)
{
	image->File = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	image->Cci = cci;
	image->UncompressedSize = 0;
	image->IndexOffset = 0;
	if (image->File == INVALID_HANDLE_VALUE)
		return 0;
	if (!cci)
		return 1;
	BYTE header[32];
	if (!read_file_at(image->File, 0, header, sizeof(header))
		|| *(DWORD*)(header + 0) != 0x4d494343
		|| *(DWORD*)(header + 4) != 32
		|| *(DWORD*)(header + 24) != 2048
		|| header[28] != 1 || header[29] != 2)
		return 0;
	image->UncompressedSize = read_u64(header + 8);
	image->IndexOffset = read_u64(header + 16);
	return image->UncompressedSize >= 2048 && !(image->UncompressedSize & 2047);
}

static int read_cci_sector(IMAGE_READER* image, DWORD sector, BYTE* out)
{
	if ((unsigned long long)sector >= image->UncompressedSize / 2048)
		return 0;
	DWORD index[2];
	if (!read_file_at(image->File, image->IndexOffset + (unsigned long long)sector * 4,
		index, sizeof(index)))
		return 0;
	unsigned long long pos = (unsigned long long)(index[0] & 0x7fffffff) << 2;
	unsigned long long next = (unsigned long long)(index[1] & 0x7fffffff) << 2;
	if (next < pos || next - pos > 0x10000)
		return 0;
	DWORD stored = (DWORD)(next - pos);
	if (stored == 2048 && !(index[0] & 0x80000000))
		return read_file_at(image->File, pos, out, 2048);
	if (stored < 2)
		return 0;
	BYTE padding;
	if (!read_file_at(image->File, pos, &padding, 1)
		|| (DWORD)padding + 1 >= stored)
		return 0;
	DWORD packed = stored - (DWORD)padding - 1;
	BYTE* compressed = (BYTE*)dash_malloc(packed);
	int ok = compressed
		&& read_file_at(image->File, pos + 1, compressed, packed)
		&& lz4_decode_sector(compressed, packed, out);
	if (compressed)
		dash_free(compressed);
	return ok;
}

static int read_image_at(IMAGE_READER* image, DWORD sector, DWORD within,
	                     void* data, DWORD size)
{
	unsigned long long absolute = (unsigned long long)sector * 2048 + within;
	if (!image->Cci)
		return read_file_at(image->File, absolute, data, size);
	if (absolute > image->UncompressedSize
		|| size > image->UncompressedSize - absolute)
		return 0;
	BYTE block[2048];
	BYTE* dst = (BYTE*)data;
	while (size)
	{
		DWORD blockNumber = (DWORD)(absolute / 2048);
		DWORD blockOffset = (DWORD)(absolute & 2047);
		DWORD take = 2048 - blockOffset;
		if (take > size) take = size;
		if (!read_cci_sector(image, blockNumber, block))
			return 0;
		for (DWORD i = 0; i < take; i++)
			dst[i] = block[blockOffset + i];
		dst += take;
		absolute += take;
		size -= take;
	}
	return 1;
}

static int ascii_ieq_n(const BYTE* a, int n, const char* b)
{
	int i = 0;
	for (; i < n && b[i]; i++)
		if (lower(a[i]) != lower(b[i]))
			return 0;
	return i == n && b[i] == 0;
}

// Locate a file in the XDVDFS root-directory binary tree. Child offsets are
// stored in four-byte units relative to the start of the directory table.
static int find_root_file(const BYTE* dir, DWORD size, const char* name,
	                      DWORD* sector, DWORD* fileSize)
{
	DWORD pending[64];
	int count = 1;
	pending[0] = 0;
	for (int visited = 0; count && visited < 128; visited++)
	{
		DWORD off = pending[--count];
		if (off > size || size - off < 14)
			continue;
		const BYTE* e = dir + off;
		WORD left = *(const WORD*)(e + 0);
		WORD right = *(const WORD*)(e + 2);
		BYTE nameLen = e[13];
		if (nameLen && nameLen <= size - off - 14
			&& ascii_ieq_n(e + 14, nameLen, name))
		{
			*sector = *(const DWORD*)(e + 4);
			*fileSize = *(const DWORD*)(e + 8);
			return 1;
		}
		if (left && count < 64)
			pending[count++] = (DWORD)left * 4;
		if (right && count < 64)
			pending[count++] = (DWORD)right * 4;
	}
	return 0;
}

static int find_image_xbe(IMAGE_READER* image, DWORD* xbeSector, DWORD* xbeSize)
{
	BYTE volume[2048];
	if (!read_image_at(image, 32, 0, volume, sizeof(volume)))
		return 0;
	static const char magic[] = "MICROSOFT*XBOX*MEDIA";
	for (int i = 0; i < 20; i++)
		if (volume[i] != (BYTE)magic[i])
			return 0;
	DWORD rootSector = *(DWORD*)(volume + 20);
	DWORD rootSize = *(DWORD*)(volume + 24);
	if (rootSize < 14 || rootSize > 0x100000)
		return 0;
	BYTE* dir = (BYTE*)dash_malloc(rootSize);
	int ok = dir && read_image_at(image, rootSector, 0, dir, rootSize)
		&& find_root_file(dir, rootSize, "default.xbe", xbeSector, xbeSize);
	if (dir)
		dash_free(dir);
	return ok;
}

static int open_disc_xbe(IMAGE_READER* image, const WCHAR* szPath,
	                     DWORD* xbeSector, DWORD* xbeSize)
{
	char path[260];
	BOOL cci;
	image->File = INVALID_HANDLE_VALUE;
	if (!get_image_path(szPath, path, sizeof(path), &cci)
		|| !open_image(image, path, cci))
		return 0;
	return find_image_xbe(image, xbeSector, xbeSize);
}

// theConfig.GetDiscImageTitleID(imagePath): read default.xbe directly from
// an XISO or a sector-compressed CCI image.
void* THISCALL Config_GetDiscImageTitleID(void* pThis, const WCHAR* szPath)
{
	(void)pThis;
	WCHAR out[9];
	out[0] = 0;
	IMAGE_READER image;
	image.File = INVALID_HANDLE_VALUE;
	DWORD xbeSector, xbeSize;
	BYTE hdr[0x1000];
	if (open_disc_xbe(&image, szPath, &xbeSector, &xbeSize)
		&& xbeSize >= sizeof(hdr)
		&& read_image_at(&image, xbeSector, 0, hdr, sizeof(hdr))
		&& hdr[0] == 'X' && hdr[1] == 'B' && hdr[2] == 'E' && hdr[3] == 'H')
	{
		DWORD base = *(DWORD*)(hdr + 0x104);
		DWORD certVA = *(DWORD*)(hdr + 0x118);
		DWORD certOff = certVA >= base ? certVA - base : 0xffffffff;
		if (certOff <= sizeof(hdr) - 12)
		{
			DWORD tid = *(DWORD*)(hdr + certOff + 8);
			const WCHAR* digits = u"0123456789ABCDEF";
			for (int i = 0; i < 8; i++)
				out[i] = digits[(tid >> ((7 - i) * 4)) & 15];
			out[8] = 0;
		}
	}
	if (image.File != INVALID_HANDLE_VALUE)
		CloseHandle(image.File);
	return make_strobj(out);
}

// theConfig.GetDiscImageTitleName(imagePath): certificate title from the
// default.xbe stored in an ISO or CCI image.
void* THISCALL Config_GetDiscImageTitleName(void* pThis, const WCHAR* szPath)
{
	(void)pThis;
	WCHAR out[41];
	out[0] = 0;
	IMAGE_READER image;
	image.File = INVALID_HANDLE_VALUE;
	DWORD xbeSector, xbeSize;
	BYTE hdr[0x1000];
	if (open_disc_xbe(&image, szPath, &xbeSector, &xbeSize)
		&& xbeSize >= sizeof(hdr)
		&& read_image_at(&image, xbeSector, 0, hdr, sizeof(hdr))
		&& hdr[0] == 'X' && hdr[1] == 'B' && hdr[2] == 'E' && hdr[3] == 'H')
	{
		DWORD base = *(DWORD*)(hdr + 0x104);
		DWORD certVA = *(DWORD*)(hdr + 0x118);
		DWORD certOff = certVA >= base ? certVA - base : 0xffffffff;
		if (certOff <= sizeof(hdr) - 0x0c - 40 * sizeof(WCHAR))
		{
			const WCHAR* title = (const WCHAR*)(hdr + certOff + 0x0c);
			int i = 0;
			for (; i < 40 && title[i]; i++)
				out[i] = title[i];
			out[i] = 0;
		}
	}
	if (image.File != INVALID_HANDLE_VALUE)
		CloseHandle(image.File);
	return make_strobj(out);
}

typedef struct DiscScriptObject
{
	void* vtable;
	int refCount;
	int type;
	void* parent;
	void* members;
} DiscScriptObject;

typedef struct DiscStringObject
{
	DiscScriptObject base;
	int length;
	int capacity;
	WCHAR* text;
} DiscStringObject;

typedef struct DiscNumberObject
{
	DiscScriptObject base;
	float value;
} DiscNumberObject;

static void* disc_bool_object(int value)
{
	DiscNumberObject* o = (DiscNumberObject*)op_new(sizeof(DiscNumberObject));
	if (!o) return 0;
	o->base.vtable = (void*)0x00022c50;
	o->base.refCount = 1;
	o->base.type = 2;
	o->base.parent = 0;
	o->base.members = 0;
	o->value = value ? 1.0f : 0.0f;
	return o;
}

static int disc_two_strings(void** args, int count, const WCHAR** a, const WCHAR** b)
{
	if (count != 2 || !args || !args[0] || !args[1]) return 0;
	DiscStringObject* first = (DiscStringObject*)args[0];
	DiscStringObject* second = (DiscStringObject*)args[1];
	if (first->base.type != 3 || second->base.type != 3 || !first->text || !second->text)
		return 0;
	*a = first->text;
	*b = second->text;
	return 1;
}

static void disc_w2a(const WCHAR* w, char* out, int cap)
{
	int i = 0;
	for (; w && w[i] && i < cap - 1; i++) out[i] = (char)w[i];
	out[i] = 0;
}

static int image_section_name(const BYTE* hdr, DWORD hdrSize, DWORD base,
	                          DWORD nameVA, const char* wanted)
{
	if (nameVA < base) return 0;
	DWORD off = nameVA - base;
	if (off >= hdrSize || hdrSize - off <= 9) return 0;
	for (DWORD i = 0; wanted[i]; i++)
		if (hdr[off + i] != (BYTE)wanted[i]) return 0;
	return hdr[off + 9] == 0;
}

// Generic XAP adapter for bool ExportDiscImageTitleImage(imagePath, destination).
void* THISCALL Config_ExportDiscImageTitleImage(void* pThis, void** args, int count)
{
	(void)pThis;
	const WCHAR *wimage, *wdestination;
	if (!disc_two_strings(args, count, &wimage, &wdestination))
		return disc_bool_object(0);
	IMAGE_READER image;
	image.File = INVALID_HANDLE_VALUE;
	DWORD xbeSector, xbeSize;
	DWORD rawOffset = 0, rawSize = 0;
	DWORD headerSize = 0;
	BYTE* header = 0;
	if (open_disc_xbe(&image, wimage, &xbeSector, &xbeSize))
	{
		headerSize = xbeSize < 0x10000 ? xbeSize : 0x10000;
		header = headerSize >= 0x124 ? (BYTE*)dash_malloc(headerSize) : 0;
	}
	if (header && read_image_at(&image, xbeSector, 0, header, headerSize)
		&& header[0] == 'X' && header[1] == 'B' && header[2] == 'E' && header[3] == 'H')
	{
		DWORD base = *(DWORD*)(header + 0x104);
		DWORD countSections = *(DWORD*)(header + 0x11c);
		DWORD tableVA = *(DWORD*)(header + 0x120);
		if (countSections <= 64 && tableVA >= base)
		{
			DWORD table = tableVA - base;
			for (DWORD i = 0; i < countSections; i++)
			{
				DWORD off = table + i * 0x38;
				if (off > headerSize || headerSize - off < 0x18) break;
				if (!image_section_name(header, headerSize, base,
					*(DWORD*)(header + off + 0x14), "$$XTIMAGE")) continue;
				rawOffset = *(DWORD*)(header + off + 0x0c);
				rawSize = *(DWORD*)(header + off + 0x10);
				if (rawOffset > xbeSize || rawSize > xbeSize - rawOffset) rawSize = 0;
				break;
			}
		}
	}
	if (header) dash_free(header);

	int ok = 0, outputStarted = 0;
	char destination[260];
	disc_w2a(wdestination, destination, sizeof(destination));
	if (rawSize)
	{
		void* out = crt_fopen(destination, "wb", 0x40);
		if (out)
		{
			outputStarted = 1;
			BYTE* buf = (BYTE*)dash_malloc(32 * 1024);
			DWORD done = 0;
			ok = buf != 0;
			while (ok && done < rawSize)
			{
				DWORD take = rawSize - done;
				if (take > 32 * 1024) take = 32 * 1024;
				if (!read_image_at(&image, xbeSector, rawOffset + done, buf, take)
					|| crt_fwrite(buf, 1, take, out) != take) ok = 0;
				else done += take;
			}
			if (buf) dash_free(buf);
			crt_fclose(out);
		}
	}
	if (image.File != INVALID_HANDLE_VALUE) CloseHandle(image.File);
	if (!ok && outputStarted) DeleteFileA(destination);
	return disc_bool_object(ok);
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
