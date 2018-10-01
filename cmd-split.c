//#define UNICODE
#ifdef UNICODE
#define STRLEN wcslen
#else
#define STRLEN strlen
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "buffer.h"
#include "dialogs.c"

#include "cmd_win.c"
#include "buffer.c"
