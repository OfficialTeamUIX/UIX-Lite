// hostinfo.c: theConfig.GetIPAddress() for XAP, so a skin can show the box's
// LAN IP the same way it shows GetXdashVersion. Handy for finding where to
// point an FTP client.

#include "dash5960.h"
#include "net.h"

// Build a CStrObject the VM owns (see dash5960.h).
static void* make_strobj(const WCHAR* sz)
{
	void* o = op_new(CSTROBJECT_SIZE);
	if (!o)
		return 0;
	return CStrObject_ctor(sz, o);
}

// sig 8 (string f(void)): this in ecx, no args, returns the string object.
void* THISCALL Config_GetIPAddress(void* pThis)
{
	(void)pThis;
	DWORD ip = net_address(); // network order, 0 until the LAN is up
	WCHAR buf[24];
	snwprintf(buf, 24, u"%d.%d.%d.%d", ip & 0xff, (ip >> 8) & 0xff, (ip >> 16) & 0xff, (ip >> 24) & 0xff);
	buf[23] = 0;
	return make_strobj(buf);
}
