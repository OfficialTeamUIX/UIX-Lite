// uixlite.h: shared helpers between the patch files.

#pragma once
#include "dash5960.h"

#define CONFIG_INI "Y:\\UIX Configs\\config.ini"

// Whole file, NUL-terminated, from the dashboard heap (dash_free it); 0 if missing
char* skin_read_file(const char* path, DWORD* size);
// Value of key in [section] of an ini text, "" if missing
void ini_value(const char* text, const char* section, const char* key, char* out, int cap);

// Features patch.py switched on, written into patch_features
#define FEATURE_DRIVES 0x1
#define FEATURE_FTP    0x2
extern DWORD patch_features;
