/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 *
 * Error handling/printing/exiting code
 *
 */

#include <cstdlib>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "pstypes.h"
#include "console.h"
#include "dxxerror.h"

#define MAX_MSG_LEN 256

static void (*ErrorPrintFunc)(const char *);

//takes string in register, calls printf with string on stack
static void warn_printf(const char *s)
{
	con_puts(CON_URGENT,s);
}

static void (*warn_func)(const char *s)=warn_printf;

//provides a function to call with warning messages
void set_warn_func(void (*f)(const char *s))
{
	warn_func = f;
}

//uninstall warning function - install default printf
void clear_warn_func()
{
	warn_func = warn_printf;
}

static void __noreturn print_exit_message(const char *exit_message, size_t len)
{
		if (ErrorPrintFunc)
		{
			(*ErrorPrintFunc)(exit_message);
		}
		con_puts(CON_CRITICAL, exit_message, len);
	d_debugbreak();
	std::abort();
}

void (Error_puts)(const char *func, const unsigned line, const char *str)
{
	char exit_message[MAX_MSG_LEN]; // don't put the new line in for dialog output
	int len = snprintf(exit_message, sizeof(exit_message), "%s:%u: error: %s", func, line, str);
	print_exit_message(exit_message, len);
}

//terminates with error code 1, printing message
void (Error)(const char *func, const unsigned line, const char *fmt,...)
{
	char exit_message[MAX_MSG_LEN]; // don't put the new line in for dialog output
	va_list arglist;

	int leader = snprintf(exit_message, sizeof(exit_message), "%s:%u: error: ", func, line);
	va_start(arglist,fmt);
	int len = vsnprintf(exit_message+leader,sizeof(exit_message)-leader,fmt,arglist);
	va_end(arglist);
	print_exit_message(exit_message, len);
}

void Warning_puts(const char *str)
{
	if (warn_func == NULL)
		return;
	char warn_message[MAX_MSG_LEN];
	snprintf(warn_message, sizeof(warn_message), "Warning: %s", str);
	(*warn_func)(warn_message);
}

//print out warning message to user
void (Warning)(const char *fmt,...)
{
	va_list arglist;

	if (warn_func == NULL)
		return;

	char warn_message[MAX_MSG_LEN];
	strcpy(warn_message,"Warning: ");

	va_start(arglist,fmt);
	vsnprintf(warn_message+9,sizeof(warn_message)-9,fmt,arglist);
	va_end(arglist);

	(*warn_func)(warn_message);

}

//initialize error handling system, and set default message. returns 0=ok
int error_init(void (*func)(const char *))
{
	ErrorPrintFunc = func;          // Set Error Print Functions
	return 0;
}
