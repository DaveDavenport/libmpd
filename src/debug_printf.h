#ifndef __DEBUG_PRINTF_H__
#define __DEBUG_PRINTF_H__

/**
 * DEBUG_NO_OUTPUT
 *
 * Don't display debug output
 */
#define DEBUG_NO_OUTPUT 0

/**
 * DebugLevel
 * @DEBUG_ERROR: Print only error messages.
 * @DEBUG_ERROR: Print error's and warning messages.
 * @DEBUG_INFO: Print all messages.
 */
typedef enum _DebugLevel {
	DEBUG_ERROR = 1,
	DEBUG_WARNING = 2,
	DEBUG_INFO = 3
} DebugLevel;

/**
 * debug_set_level
 * @dl: The #DebugLevel
 * 
 * Set the debug level
 */
void debug_set_level(DebugLevel dl);
void debug_printf_real(DebugLevel dp, const char *file,const int line,const char *function, const char *format,...);
#define debug_printf(dp, format, ARGS...) debug_printf_real(dp,__FILE__,__LINE__,__FUNCTION__,format,##ARGS)

#endif
