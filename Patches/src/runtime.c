// runtime.c: the two helpers the compiler may emit calls to.

void* memset(void* dst, int c, unsigned int n)
{
	unsigned char* d = (unsigned char*)dst;
	while (n--)
		*d++ = (unsigned char)c;
	return dst;
}

void* memcpy(void* dst, const void* src, unsigned int n)
{
	unsigned char* d = (unsigned char*)dst;
	const unsigned char* s = (const unsigned char*)src;
	while (n--)
		*d++ = *s++;
	return dst;
}
