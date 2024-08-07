#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN      // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <io.h>
#pragma comment(lib, "User32.lib")
#else
#define BOOL bool
#include <unistd.h>
#endif


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <assert.h>
#include <math.h>
#include <errno.h> 
#include <time.h>
#include <ctype.h>
