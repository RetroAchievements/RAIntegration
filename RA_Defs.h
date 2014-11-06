#pragma once

//	NULL, etc
#include <stddef.h>
#include <vector>

#ifndef RA_EXPORTS

//NB. These must ONLY accessible from the emulator!
#define RAGENS_VERSION			"0.050"
#define RASNES9X_VERSION		"0.015"
#define RAVBA_VERSION			"0.018"
#define RANES_VERSION			"0.008"
#define RAPCE_VERSION			"0.002"

#else

//NB. These must NOT be accessible from the emulator!
#define RA_INTEGRATION_VERSION	"0.051"

//	RA-Only
#include "rapidjson/include/rapidjson/document.h"
#include "rapidjson/include/rapidjson/reader.h"
#include "rapidjson/include/rapidjson/writer.h"
#include "rapidjson/include/rapidjson/filestream.h"
using namespace rapidjson;

#endif	//RA_EXPORTS


#define PASTE(x, y) x##y
#define MAKEWIDE(x) PASTE(L,x)

#define RA_PREFERENCES_FILENAME_PREFIX	"RAPrefs_"
#define RA_LOCKED_BADGE_IMAGE_URI		"00000.png"

#define RA_DIR_OVERLAY					".\\Overlay\\"
#define RA_DIR_BASE						".\\RACache\\"
#define RA_DIR_BADGE					RA_DIR_BASE##"Badge\\"
#define RA_DIR_DATA						RA_DIR_BASE##"Data\\"
#define RA_DIR_USERPIC					RA_DIR_BASE##"UserPic\\"

#define RA_GAME_HASH_FILENAME			RA_DIR_DATA##"gamehashlibrary.txt"
#define RA_GAME_LIST_FILENAME			RA_DIR_DATA##"gametitles.txt"
#define RA_MY_PROGRESS_FILENAME			RA_DIR_DATA##"myprogress.txt"
#define RA_MY_GAME_LIBRARY_FILENAME		RA_DIR_DATA##"mygamelibrary.txt"

#define RA_NEWS_FILENAME				RA_DIR_DATA##"ra_news.txt"
#define RA_OVERLAY_BG_FILENAME			RA_DIR_OVERLAY##"overlayBG.png"
#define RA_TITLES_FILENAME				RA_DIR_DATA##"gametitles.txt"
#define RA_LOCKED_BADGE_IMAGE_FILENAME	RA_DIR_BADGE##RA_LOCKED_BADGE_IMAGE_URI
#define RA_LOG_FILENAME					RA_DIR_DATA##"RALog.txt"


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

#define RA_HOST_URL "http://" RA_HOST

#define SIZEOF_ARRAY( ar ) ( sizeof( ar )/sizeof( ar[0] ) )

#define SAFE_DELETE( x ) { if( x != NULL ) { delete x; x = NULL; } }

typedef unsigned char       BYTE;
typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef DWORD				ARGB;

typedef std::vector<BYTE> DataStream;

typedef unsigned long ByteAddress;

typedef unsigned int AchievementID;
typedef unsigned int LeaderboardID;
typedef unsigned int GameID;

class RASize
{
public:
	RASize() : m_nWidth( 0 ), m_nHeight( 0 ) {}
	RASize( const RASize& rhs ) : m_nWidth( rhs.m_nWidth ), m_nHeight( rhs.m_nHeight ) {}
	RASize( int nW, int nH ) : m_nWidth( nW ), m_nHeight( nH ) {}

public:
	inline int Width() const		{ return m_nWidth; }
	inline int Height() const		{ return m_nHeight; }
	inline void SetWidth( int nW )	{ m_nWidth = nW; }
	inline void SetHeight( int nH )	{ m_nHeight = nH; }

private:
	int m_nWidth;
	int m_nHeight;
};

const RASize RA_BADGE_PX( 64, 64 );
const RASize RA_USERPIC_PX( 64, 64 );

enum AchievementSetType
{
	AT_CORE,
	AT_UNOFFICIAL,
	AT_USER
};

char* DataStreamAsString( DataStream& stream );

extern void DebugLog( const char* sFormat, ... );
extern BOOL DirectoryExists( const char* sPath );

namespace RA
{
	template<typename T>
	static inline const T& Clamp( const T& val, const T& lower, const T& upper )
	{
		return( val < lower ) ? lower : ( ( val > upper ) ? upper : val );
	}
};

#ifdef _DEBUG
#define RA_LOG DebugLog
#define ASSERT(x) assert(x)
#else
#define RA_LOG DebugLog
#define ASSERT(x) {}
#endif