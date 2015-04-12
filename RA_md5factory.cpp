#include "RA_md5factory.h"

#include <string.h>
#include <stdio.h>

#include "md5.h"
#include "RA_Defs.h"

namespace
{	
	const static unsigned int MD5_STRING_LEN = 32;
}

std::string RAGenerateMD5( const std::string& sStringToMD5 )
{
	static char buffer[33];
	
	md5_state_t pms;
	md5_byte_t digest[16];

	md5_byte_t* pDataBuffer = new md5_byte_t[ sStringToMD5.length() ];
	ASSERT( pDataBuffer != NULL );
	if( pDataBuffer == NULL )
		return "";

	memcpy( pDataBuffer, sStringToMD5.c_str(), sStringToMD5.length() );

	md5_init( &pms );
	md5_append( &pms, pDataBuffer, sStringToMD5.length() );
	md5_finish( &pms, digest );

	memset( buffer, 0, MD5_STRING_LEN+1 );
	sprintf_s( buffer, MD5_STRING_LEN+1, 
			  "%02x%02x%02x%02x%02x%02x%02x%02x"
			  "%02x%02x%02x%02x%02x%02x%02x%02x",
			  digest[0],digest[1],digest[2],digest[3],digest[4],digest[5],digest[6],digest[7],
			  digest[8],digest[9],digest[10],digest[11],digest[12],digest[13],digest[14],digest[15] );

	delete[] pDataBuffer;
	pDataBuffer = NULL;

	return buffer;
}

std::string RAGenerateMD5( const BYTE* pRawData, size_t nDataLen )
{
	static char buffer[33];
	
	md5_state_t pms;
	md5_byte_t digest[16];

	static_assert( sizeof( md5_byte_t ) == sizeof( BYTE ), "Must be equivalent for the MD5 to work!" );

	md5_init( &pms );
	md5_append( &pms, static_cast<const md5_byte_t*>( pRawData ), nDataLen );
	md5_finish( &pms, digest );

	memset( buffer, 0, MD5_STRING_LEN+1 );
	sprintf_s( buffer, MD5_STRING_LEN + 1,
			   "%02x%02x%02x%02x%02x%02x%02x%02x"
			   "%02x%02x%02x%02x%02x%02x%02x%02x",
			   digest[ 0 ], digest[ 1 ], digest[ 2 ], digest[ 3 ], digest[ 4 ], digest[ 5 ], digest[ 6 ], digest[ 7 ],
			   digest[ 8 ], digest[ 9 ], digest[ 10 ], digest[ 11 ], digest[ 12 ], digest[ 13 ], digest[ 14 ], digest[ 15 ] );

	return buffer;	//	Implicit promotion to std::string
}

std::string RAGenerateMD5( const std::vector<BYTE> DataIn )
{
	return RAGenerateMD5( DataIn.data(), DataIn.size() );
}