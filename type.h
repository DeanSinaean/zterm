#include <stdint.h>
#include <errno.h>
#include <stdio.h>

#define DWORD uint32_t
#define LONG uint32_t
#define HWND void *
#define CString char *
#define HANDLE void *
#define LPDWORD uint64_t *
#define LPOVERLAPPED uint64_t *
#define NO_ERROR 0
#define GetLastError() errno
#define SetLastError(x) errno = x
#define TRACE printf
#define CString char *
