// video.c: 720p on HDTV.
//
// Retail always creates a 480-line device. On an HDTV pack with 720p and
// widescreen enabled (NTSC only), switch the present parameters to
// 1280x720 progressive before the device is created. A title-launch boot
// (boot reason 4) is left alone.

#include "dash5960.h"

// D3DPRESENT_PARAMETERS, the fields we touch
#define PP_WIDTH    0x00
#define PP_HEIGHT   0x04
#define PP_MULTISAMPLE 0x10
#define PP_FLAGS    0x28
#define PP_REFRESH  0x2c
#define PP_INTERVAL 0x30

#define AV_PACK_HDTV          1
#define VIDEO_WIDESCREEN      0x1
#define VIDEO_720P            0x2
#define PRESENT_FLAG_INTERLACED  0x20
#define PRESENT_FLAG_PROGRESSIVE 0x40
#define PRESENT_INTERVAL_IMMEDIATE 0x80000000

#define PP(pp, off) (*(DWORD*)((BYTE*)(pp) + (off)))

void video_setup(void* pp)
{
	if (dash_boot_reason == 4)
		return;
	if (XGetAVPack() != AV_PACK_HDTV)
		return;
	if ((XGetVideoFlags() & (VIDEO_WIDESCREEN | VIDEO_720P)) != (VIDEO_WIDESCREEN | VIDEO_720P))
		return;
	DWORD standard = XGetVideoStandard();
	if (standard != 1 && standard != 2) // NTSC-M, NTSC-J
		return;

	PP(pp, PP_FLAGS) = (PP(pp, PP_FLAGS) & ~PRESENT_FLAG_INTERLACED) | PRESENT_FLAG_PROGRESSIVE;
	PP(pp, PP_WIDTH) = 1280;
	PP(pp, PP_HEIGHT) = 720;
	PP(pp, PP_MULTISAMPLE) = 0;
	PP(pp, PP_REFRESH) = 0;
	PP(pp, PP_INTERVAL) = PRESENT_INTERVAL_IMMEDIATE;
}

// Replaces the device creation call in the D3D setup (0002ce6f). Device
// creation takes eax and ecx as inputs, so keep every register and jump on.
__asm__(
	".globl patch_create_device\n"
	"patch_create_device:\n"
	"	pushl %eax\n"
	"	pushl %ecx\n"
	"	pushl %edx\n"
	"	pushl 16(%esp)\n"          // present params, the caller's one stack arg
	"	call video_setup\n"
	"	addl $4, %esp\n"
	"	popl %edx\n"
	"	popl %ecx\n"
	"	popl %eax\n"
	"	jmp 0x001482d0\n");
