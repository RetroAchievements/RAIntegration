#pragma once

//	NULL, etc
#include <stddef.h>
#include <vector>

#ifndef RA_EXPORTS

//NB. These must ONLY accessible from the emulator!
#define RAGENS_VERSION			"0.050"
#define RASNES9X_VERSION		"0.014"
#define RAVBA_VERSION			"0.017"
#define RANES_VERSION			"0.008"
#define RAPCE_VERSION			"0.002"

#else

//NB. These must NOT be accessible from the emulator!
#define RA_INTEGRATION_VERSION	"0.051"

//	RA-Only
#include "rapidjson/include/rapidjson/document.h"
using namespace rapidjson;

#endif	//RA_EXPORTS


#define PASTE(x, y) x##y
#define MAKEWIDE(x) PASTE(L,x)

#define RA_DIR_BASE		".\\RACache\\"
#define RA_DIR_BADGE	".\\RACache\\Badge\\"
#define RA_DIR_DATA		".\\RACache\\Data\\"
#define RA_DIR_OVERLAY	".\\Overlay\\"

//	Seconds
#define RA_SERVER_POLL_DURATION 1*60

#if defined _DEBUG
#define RA_HOST "localhost"
#else
#define RA_HOST "retroachievements.org"
#endif

#define RA_HOST_W MAKEWIDE( RA_HOST )

#define RA_IMG_HOST "retroachievements.org"
#define RA_IMG_HOST_W MAKEWIDE( RA_IMG_HOST )

#define SIZEOF_ARRAY(ar) (sizeof(ar)/sizeof(ar[0]))

typedef unsigned char       BYTE;
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef DWORD				ARGB;

typedef std::vector<BYTE> DataStream;

char* DataStreamAsString( DataStream& stream );

extern void DebugLog( const char* sFormat, ... );
extern BOOL DirectoryExists( const char* sPath );

#ifdef _DEBUG
#define RA_LOG DebugLog
#else
#define RA_LOG DebugLog
#endif