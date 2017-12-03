#include "RA_Defs.h"

#include <stdio.h>
#include <Windows.h>
#include <locale>
#include <codecvt>

GetParseErrorFunc GetJSONParseErrorStr = GetParseError_En;

void RADebugLogNoFormat( const char* data )
{
	OutputDebugString( Widen( data ).c_str() );

	//SetCurrentDirectory( g_sHomeDir.c_str() );//?
	FILE* pf = NULL;
	if( fopen_s( &pf, RA_LOG_FILENAME, "a" ) == 0 )
	{
		fwrite( data, sizeof( char ), strlen( data ), pf );
		fclose( pf );
	}
}

void RADebugLog( const char* format, ... )
{
	char buf[ 4096 ];
	char* p = buf;

	va_list args;
	va_start( args, format );
	int n = _vsnprintf_s( p, 4096, sizeof buf - 3, format, args ); // buf-3 is room for CR/LF/NUL
	va_end( args );

	p += ( n < 0 ) ? sizeof buf - 3 : n;

	while( ( p > buf ) && ( isspace( p[ -1 ] ) ) )
		*--p = '\0';

	*p++ = '\r';
	*p++ = '\n';
	*p   = '\0';

	OutputDebugString( Widen( buf ).c_str() );
	
	//SetCurrentDirectory( g_sHomeDir.c_str() );//?
	FILE* pf = NULL;
	if( fopen_s( &pf, RA_LOG_FILENAME, "a" ) == 0 )
	{
		fwrite( buf, sizeof(char), strlen( buf ), pf );
		fclose( pf );
	}
}

BOOL DirectoryExists( const char* sPath )
{
	DWORD dwAttrib = GetFileAttributes( Widen( sPath ).c_str() );
	return( dwAttrib != INVALID_FILE_ATTRIBUTES && ( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) );
}

static_assert( sizeof( BYTE* ) == sizeof( char* ), "dangerous cast ahead" );
char* DataStreamAsString( DataStream& stream )
{
	return reinterpret_cast<char*>( stream.data() );
}

std::string Narrow( const wchar_t* wstr )
{
	static std::wstring_convert< std::codecvt_utf8_utf16< wchar_t >, wchar_t > converter;
	return converter.to_bytes( wstr );
}

std::string Narrow( const std::wstring& wstr )
{
	static std::wstring_convert< std::codecvt_utf8_utf16< wchar_t >, wchar_t > converter;
	return converter.to_bytes( wstr );
}

std::wstring Widen( const char* str )
{
	static std::wstring_convert< std::codecvt_utf8_utf16< wchar_t >, wchar_t > converter;
	return converter.from_bytes( str );
}

std::wstring Widen( const std::string& str )
{
	static std::wstring_convert< std::codecvt_utf8_utf16< wchar_t >, wchar_t > converter;
	return converter.from_bytes( str );
}
