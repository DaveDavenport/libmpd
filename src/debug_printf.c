#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "config.h"
#include "debug_printf.h"

#define RED "\x1b[31;01m"
#define DARKRED "\x1b[31;06m"
#define RESET "\x1b[0m"
#define GREEN "\x1b[32;06m"
#define YELLOW "\x1b[33;06m"

int debug_level = 0;
/* Compiler does not like it when I initialize this to stdout, complaints about
 * not being constant. stoud is a macro..
 * So use this "hack"
 */
FILE *rout = NULL;

void debug_set_output(FILE *fp)
{
    rout = fp;
}

void debug_set_level(DebugLevel dl)
{
	debug_level = (dl<0)?DEBUG_NO_OUTPUT:((dl > DEBUG_INFO)?DEBUG_INFO:dl);
}


void debug_printf_real(DebugLevel dp, const char *file,const int line,const char *function, const char *format,...)
{
	if(debug_level >= dp)
	{
		va_list arglist;
        time_t ts = time(NULL);
        struct tm tm;
        char buffer[32];
        FILE *out = stdout;
        if(rout) out = rout;
        va_start(arglist,format);
  
  /* Windows has no thread-safe localtime_r function, so ignore it for now */
#ifndef WIN32
        localtime_r(&ts, &tm);
        strftime(buffer, 32, "%d/%m/%y %T",&tm); 
#else
        buffer[0] = '\0';
#endif

		if(dp == DEBUG_INFO)
		{
			fprintf(out,"%s: "GREEN"INFO:"RESET"    %s %s():#%d:\t",buffer,file,function,line);
		}
		else if(dp == DEBUG_WARNING)
		{
			fprintf(out,"%s: "YELLOW"WARNING:"RESET" %s %s():#%i:\t",buffer,file,function,line);
		}
		else
		{
			fprintf(out,"%s: "DARKRED"ERROR:"RESET"   %s %s():#%i:\t",buffer,file,function,line);
		}
		vfprintf(out,format, arglist);
		if(format[strlen(format)-1] != '\n')
		{
			fprintf(out,"\n");
		}
		fflush(out);
		va_end(arglist);
	}
}
