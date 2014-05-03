#include "RA_md5factory.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <malloc.h>

#include "md5.h"


void md5_GenerateMD5( const char* strIn, const unsigned int nLen, char* strOut )
{
	const static unsigned int StrOutLen = 33;

	md5_state_t pms;
	md5_byte_t digest[16];
	md5_byte_t* passBuffer = NULL;

	assert( nLen < 20000000 );
	if( nLen > 20000000 )
		return;

	passBuffer = (md5_byte_t*)malloc(nLen);
	assert( passBuffer != NULL );
	if( passBuffer == NULL )
		return;

	memset( passBuffer, 0, nLen );
	memcpy( passBuffer, strIn, nLen );

	md5_init( &pms );
	md5_append( &pms, passBuffer, nLen );
	md5_finish( &pms, digest );

	memset( strOut, 0, StrOutLen );
	sprintf_s( strOut, StrOutLen, 
			  "%02x%02x%02x%02x%02x%02x%02x%02x"
			  "%02x%02x%02x%02x%02x%02x%02x%02x",
			  digest[0],digest[1],digest[2],digest[3],digest[4],digest[5],digest[6],digest[7],
			  digest[8],digest[9],digest[10],digest[11],digest[12],digest[13],digest[14],digest[15] );

	free( passBuffer );
	passBuffer = NULL;
}

void md5_GenerateMD5Raw( const unsigned char* rawIn, const unsigned int nLen, char* strOut )
{
	const static unsigned int StrOutLen = 33;

	md5_state_t pms;
	md5_byte_t digest[16];

	assert( sizeof( md5_byte_t ) == sizeof( unsigned char ) );

	md5_init( &pms );
	md5_append( &pms, (md5_byte_t*)rawIn, nLen );
	md5_finish( &pms, digest );

	memset( strOut, 0, StrOutLen );
	sprintf_s( strOut, StrOutLen, 
			  "%02x%02x%02x%02x%02x%02x%02x%02x"
			  "%02x%02x%02x%02x%02x%02x%02x%02x",
			  digest[0],digest[1],digest[2],digest[3],digest[4],digest[5],digest[6],digest[7],
			  digest[8],digest[9],digest[10],digest[11],digest[12],digest[13],digest[14],digest[15] );
}