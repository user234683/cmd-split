//#define UNICODE
#ifdef UNICODE
#define STRLEN wcslen
#else
#define STRLEN strlen
#endif

#include <windows.h>
#include "buffer.h"
#include "cmd.h"

#include "cmd_win.c"
#include "buffer.c"
