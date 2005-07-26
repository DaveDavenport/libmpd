#ifndef __DEBUG_PRINTF_H__
#define __DEBUG_PRINTF_H__

#define NO_DEBUG_OUTPUT -1
enum {
	DEBUG_ERROR,
	DEBUG_WARNING,
	DEBUG_INFO
};

void debug_printf(int dp, char *format, ...);



#endif
