// cache.c: destructive dashboard-cache cleanup exposed as
// theConfig.ClearCache().

#include "dash5960.h"

static int is_dot_name(const char* name)
{
	return name[0] == '.' && (!name[1] || (name[1] == '.' && !name[2]));
}

static int name_is(const char* name, const char* wanted)
{
	for (;; name++, wanted++)
	{
		int a = *name, b = *wanted;
		if (a >= 'a' && a <= 'z') a -= 'a' - 'A';
		if (b >= 'a' && b <= 'z') b -= 'a' - 'A';
		if (a != b)
			return 0;
		if (!a)
			return 1;
	}
}

static int append_child(char* path, int capacity, const char* name)
{
	int n = 0;
	while (path[n])
		n++;
	if (n && path[n - 1] != '\\')
	{
		if (n >= capacity - 1)
			return 0;
		path[n++] = '\\';
	}
	for (int i = 0; name[i]; i++)
	{
		if (n >= capacity - 1)
			return 0;
		path[n++] = name[i];
	}
	path[n] = 0;
	return 1;
}

static int remove_tree(const char* root)
{
	DWORD attributes = GetFileAttributesA(root);
	if (attributes == INVALID_FILE_ATTRIBUTES)
		return 1; // already absent
	if (!(attributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		SetFileAttributesA(root, FILE_ATTRIBUTE_NORMAL);
		return DeleteFileA(root) ? 1 : 0;
	}

	char pattern[260];
	int i = 0;
	for (; root[i] && i < (int)sizeof(pattern) - 3; i++)
		pattern[i] = root[i];
	if (root[i])
		return 0;
	if (i && pattern[i - 1] != '\\')
		pattern[i++] = '\\';
	pattern[i++] = '*';
	pattern[i] = 0;

	WIN32_FIND_DATAA data;
	HANDLE find = FindFirstFileA(pattern, &data);
	if (find != INVALID_HANDLE_VALUE)
	{
		int ok = 1;
		do
		{
			if (is_dot_name(data.cFileName))
				continue;
			char child[260];
			int j = 0;
			for (; root[j] && j < (int)sizeof(child) - 1; j++)
				child[j] = root[j];
			child[j] = 0;
			if (root[j] || !append_child(child, sizeof(child), data.cFileName)
				|| !remove_tree(child))
				ok = 0;
		} while (FindNextFileA(find, &data));
		NtClose(find);
		if (!ok)
			return 0;
	}
	SetFileAttributesA(root, FILE_ATTRIBUTE_NORMAL);
	return RemoveDirectoryA(root) ? 1 : 0;
}

static int cache_contains_only_live_file(void)
{
	WIN32_FIND_DATAA data;
	HANDLE find = FindFirstFileA("C:\\cache\\*", &data);
	if (find == INVALID_HANDLE_VALUE)
		return GetFileAttributesA("C:\\cache") == INVALID_FILE_ATTRIBUTES;
	int found = 0, ok = 1;
	do
	{
		if (is_dot_name(data.cFileName))
			continue;
		if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			|| !name_is(data.cFileName, "LocalCache00.bin") || found)
			ok = 0;
		found = 1;
	} while (FindNextFileA(find, &data));
	NtClose(find);
	return ok && found;
}

// Formats X:, Y:, and Z: (physical partitions 3, 4, and 5), then removes
// the dashboard's persistent cache folder on the data partition. This call
// intentionally blocks so its Boolean result describes completed cleanup.
int THISCALL Config_ClearCache(void* pThis)
{
	(void)pThis;
	static const char* const partitions[] =
	{
		"\\Device\\Harddisk0\\Partition3",
		"\\Device\\Harddisk0\\Partition4",
		"\\Device\\Harddisk0\\Partition5"
	};

	for (int i = 0; i < 3; i++)
	{
		ANSI_STRING path;
		RtlInitAnsiString(&path, partitions[i]);
		if (!XapiFormatFATVolume(&path))
			return 0;
		SleepEx(100, 0);
	}

	if (remove_tree("C:\\cache"))
		return 1;

	// LocalCache00.bin is the running dashboard's active cache database. It is
	// intentionally ignored if every other cache entry was removed.
	return cache_contains_only_live_file();
}
