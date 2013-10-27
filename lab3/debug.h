#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdarg.h>
#include <stdio.h>

#ifndef DEBUG
#define DEBUG 0
#endif

#define DEBUG_TEST(x, y)\
	if(x != y) { fprintf(stderr, "TEST FAILURE: %s != %s. %s %d %s\n",  __FILE__, __LINE__, __func__); } else

//To use this macro, you use two layers of parentheses, like this:
// DEBUG_PRINT(("Hello %s!\n", "world"));
#define DEBUG_PRINT(x) if(DEBUG) do { debug_printf x ; } while(0)

static void debug_printf(const char * fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}

#endif
