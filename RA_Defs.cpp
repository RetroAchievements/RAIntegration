#include "RA_Defs.h"

#include <stdio.h>
#include <Windows.h>

#define DEBUG_LOGFILE "RALog.txt"

void DebugLog( const char* format, ... )
{
	char    buf[4096], *p = buf;
	va_list args;
	int     n;

	va_start(args, format);
	n = _vsnprintf_s(p, 4096, sizeof buf - 3, format, args); // buf-3 is room for CR/LF/NUL
	va_end(args);

	p += (n < 0) ? sizeof buf - 3 : n;

	while ( p > buf  &&  isspace(p[-1]) )
		*--p = '\0';

	*p++ = '\r';
	*p++ = '\n';
	*p   = '\0';

	OutputDebugString( buf );

	FILE* pFile = NULL;
	if( fopen_s( &pFile, RA_DIR_DATA DEBUG_LOGFILE, "a" ) == 0 )
	{
		fwrite( buf, sizeof(char), strlen( buf ), pFile );
		fclose( pFile );
	}
}
