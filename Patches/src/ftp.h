// ftp.h: in-dash FTP file server for pushing files to the console over the LAN.

#pragma once

// Spawn the listener thread. It waits for the network stack to come up, then
// serves port 21. Safe to call once from startup; later calls do nothing.
void ftp_start(void);
