// dash5960.h: addresses in the retail 5960 xboxdash.xbe that the patches use.
// Found by analyzing the retail binary; the patcher refuses any other build.

#pragma once

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned int   DWORD;
typedef int            BOOL;
typedef void*          HANDLE;
typedef unsigned short WCHAR;

#define STDCALL  __attribute__((stdcall))
#define FASTCALL __attribute__((fastcall))
#define THISCALL __attribute__((thiscall))

#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#define GENERIC_READ          0x80000000
#define FILE_SHARE_READ       0x00000001
#define OPEN_EXISTING         3
#define FILE_ATTRIBUTE_NORMAL 0x80

// XAPI and C runtime, linked into the retail XBE
#define CreateFileA        ((HANDLE (STDCALL *)(const char*, DWORD, DWORD, void*, DWORD, DWORD, HANDLE))0x0006ac68)
#define ReadFile           ((BOOL (STDCALL *)(HANDLE, void*, DWORD, DWORD*, void*))0x0006ce10)
#define CloseHandle        ((BOOL (STDCALL *)(HANDLE))0x0006c8c9)
#define GetFileSize        ((DWORD (STDCALL *)(HANDLE, DWORD*))0x0006d454)
#define SleepEx            ((DWORD (STDCALL *)(DWORD, BOOL))0x0006d732)
#define XGetAVPack         ((DWORD (STDCALL *)(void))0x0006c563)
#define XGetVideoFlags     ((DWORD (STDCALL *)(void))0x0006c51a)
#define XGetVideoStandard  ((DWORD (STDCALL *)(void))0x0006c56b)
#define OutputDebugStringA ((void (STDCALL *)(const char*))0x000c6463)
#define dash_malloc        ((void* (*)(DWORD))0x000741c0)
#define dash_free          ((void (*)(void*))0x000741d2)

// File and directory calls (xapilib), for the FTP file server.
// NOTE: this XBE has no plain WriteFile; the only writer is CRT stdio (below),
// which is what the dashboard itself uses. Reads still go through ReadFile.
#define SetFilePointer     ((DWORD (STDCALL *)(HANDLE, int, int*, DWORD))0x0006d057)
#define GetFileAttributesA ((DWORD (STDCALL *)(const char*))0x0006c63a)
#define DeleteFileA        ((BOOL (STDCALL *)(const char*))0x0006c707)
#define CreateDirectoryA   ((BOOL (STDCALL *)(const char*, void*))0x0006c8e7)
#define RemoveDirectoryA   ((BOOL (STDCALL *)(const char*))0x0006c953)
#define FindFirstFileA     ((HANDLE (STDCALL *)(const char*, void*))0x0006d82a)
#define FindNextFileA      ((BOOL (STDCALL *)(HANDLE, void*))0x0006d937) // FindClose = NtClose(handle)
#define MoveFileExA        ((BOOL (STDCALL *)(const char*, const char*, DWORD))0x0006c8b1)
#define MOVEFILE_REPLACE_EXISTING 0x1
#define XSetFileCacheSize  ((BOOL (STDCALL *)(DWORD))0x0006c795)

// CRT stdio (__cdecl, NOT stdcall) is the only file-write path in this XBE.
// fopen is inlined to _fsopen(name, mode, shflag); _SH_DENYNO = 0x40.
#define crt_fopen   ((void* (*)(const char*, const char*, int))0x00074681)
#define crt_fwrite  ((DWORD  (*)(const void*, DWORD, DWORD, void*))0x000747b9)
#define crt_fseek   ((int    (*)(void*, long, int))0x000742b9)
#define crt_fclose  ((int    (*)(void*))0x00073df8)
#define SH_DENYNO   0x40
#define STDIO_SEEK_SET 0
#define STDIO_SEEK_END 2

// Returning a string to the XAP VM (sig 8): build a CStrObject on the heap the
// same way GetXdashVersion does and return it; the VM takes ownership.
// operator new (cdecl); the ctor is ecx = source wide string, object pointer on
// the stack, ret 4 -> a THISCALL(sz, obj). Same heap as the VM's delete.
#define op_new          ((void* (*)(DWORD))0x00072b2f)
#define CStrObject_ctor ((void* (THISCALL *)(const WCHAR*, void*))0x00032090)
#define snwprintf       ((int (*)(WCHAR*, DWORD, const WCHAR*, ...))0x00073c6f)
#define CSTROBJECT_SIZE 0x20
void* make_strobj(const WCHAR* sz); // heap CStrObject the VM owns (hostinfo.c)

// Threads (xapilib)
#define CreateThread       ((HANDLE (STDCALL *)(void*, DWORD, void*, void*, DWORD, DWORD*))0x0006ba6f)
#define SetThreadPriority  ((BOOL (STDCALL *)(HANDLE, int))0x0006b7f9)

#define GENERIC_WRITE            0x40000000
#define CREATE_ALWAYS            2
#define OPEN_ALWAYS              4
#define FILE_BEGIN               0
#define FILE_END                 2
#define INVALID_SET_FILE_POINTER 0xFFFFFFFF
#define INVALID_FILE_ATTRIBUTES  0xFFFFFFFF
#define FILE_ATTRIBUTE_DIRECTORY 0x10

// WIN32_FIND_DATAA as xapilib fills it (name at +44, size low at +32)
typedef struct WIN32_FIND_DATAA
{
	DWORD dwFileAttributes;
	DWORD ftCreationTime[2];
	DWORD ftLastAccessTime[2];
	DWORD ftLastWriteTime[2];
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	DWORD dwReserved0;
	DWORD dwReserved1;
	char  cFileName[260];
	char  cAlternateFileName[14];
} WIN32_FIND_DATAA;

// D3D
#define D3DResource_AddRef  ((DWORD (STDCALL *)(void*))0x00148620)
#define D3DResource_Release ((DWORD (STDCALL *)(void*))0x00148660)
#define D3D_BlockOnResource ((void (FASTCALL *)(void*))0x001496e0)
#define D3D_CREATE_DEVICE   0x001482d0 // eax = behavior flags, ecx = device out, stack = present params

// Kernel imports, read from the XBE's thunk table
typedef struct ANSI_STRING
{
	WORD  Length;
	WORD  MaximumLength;
	char* Buffer;
} ANSI_STRING;

typedef struct OBJECT_ATTRIBUTES
{
	HANDLE       RootDirectory;
	ANSI_STRING* ObjectName;
	DWORD        Attributes;
} OBJECT_ATTRIBUTES;

typedef struct IO_STATUS_BLOCK
{
	int   Status;
	DWORD Information;
} IO_STATUS_BLOCK;

#define KIMPORT(type, slot) ((type)(*(void**)(slot)))
#define RtlInitAnsiString       KIMPORT(void (STDCALL *)(ANSI_STRING*, const char*), 0x0001200c)
#define NtOpenFile              KIMPORT(int (STDCALL *)(HANDLE*, DWORD, OBJECT_ATTRIBUTES*, IO_STATUS_BLOCK*, DWORD, DWORD), 0x00012010)
#define NtClose                 KIMPORT(int (STDCALL *)(HANDLE), 0x00012014)
#define HalReturnToFirmware     KIMPORT(void (STDCALL *)(int), 0x00012018)
#define IoCreateSymbolicLink    KIMPORT(int (STDCALL *)(ANSI_STRING*, ANSI_STRING*), 0x00012024)
#define NtDeviceIoControlFile   KIMPORT(int (STDCALL *)(HANDLE, HANDLE, void*, void*, IO_STATUS_BLOCK*, DWORD, void*, DWORD, void*, DWORD), 0x00012114)
#define NtAllocateVirtualMemory KIMPORT(int (STDCALL *)(void**, DWORD, DWORD*, DWORD, DWORD), 0x0001215c)
#define XboxKrnlVersion         (*(WORD**)0x00012050) // Major, Minor, Build, Qfe

// The dashboard
#define dash_run            ((void (*)(void))0x0002da00)       // what main calls to run the dashboard
#define dash_boot_reason    (*(DWORD*)0x0017aee4)
#define dash_init_materials ((void (*)(void))0x00057cb0)       // fills the material table
#define dash_materials      ((Material**)0x0017c6b8)
#define dash_material_count (*(int*)0x0017e754)
#define dash_texture_from_xip ((void* (FASTCALL *)(const WCHAR*))0x00060c90)
#define DASH_PARSE_TEXTURE  0x00060990 // eax = name, ebx = file data, stack = size, width, height; callee pops

typedef struct Material
{
	void*   vtable;
	WCHAR*  name;
	DWORD   flags;
	DWORD   colorA; // solid kinds: r,g,b,a bytes; falloff kinds: side color (ARGB)
	DWORD   colorB; // falloff kinds: front color (ARGB)
} Material;

// Material kinds, by vtable
#define MAT_SOLID      0x00028bf4
#define MAT_BACKING    0x00028bf8
#define MAT_MODULATE   0x00028bec
#define MAT_FALLOFF    0x00028bf0
#define MAT_FALLOFFTEX 0x00028be8
#define MAT_INNERWALL  0x00028b6c
#define MAT_ANISO      0x00028be0

// Script-callable functions: {member pointer (code + 3 adjustors), signature, name}
typedef struct ScriptFn
{
	DWORD        code;
	DWORD        adj[3];
	WORD         sig;
	WORD         pad;
	const WCHAR* name;
} ScriptFn;

#define SIG_VOID          1  // void f()
#define SIG_INT           2  // int f()
#define SIG_STRING_STRING 5  // string f(string), like Translate
#define SIG_STRING        8  // string f(), like GetXdashVersion
#define SIG_VOID_2STRING  13 // void f(string, string), like CSettings::SetValue
#define SIG_INT_STRING    25 // int f(string)

#define dash_config_functions ((ScriptFn*)0x0017fdf0) // theConfig's table, built before main
