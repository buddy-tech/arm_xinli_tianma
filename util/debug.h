#ifndef _DEBUG_H
#define _DEBUG_H

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "common.h"
#define MSG	printf
void print_error(char* file, char* function, int line, const char *fmt, ...);
void hex_dump( unsigned char * buf, int len );
char* hex_str(unsigned char *buf, int len, char* outstr );
void debug_term_on();
void debug_term_off();
void debug_file_on();
void debug_file_off();
void debug_set_dir(char* str);

void  traceMsg(const char *fmt,...);
#endif //_DEBUG_H

