#pragma once

#include <Windows.h>
#include <WindowsX.h>
#include <ShlObj.h>
#include <tchar.h>
#include <assert.h>
#include <string>
#include <sstream>
#include <vector>
#include <queue>
#include <deque>
#include <map>

#ifndef RA_EXPORTS

//	Version Information is integrated into tags

#else

//NB. These must NOT be accessible from the emulator!
//#define RA_INTEGRATION_VERSION	"0.053"

//	RA-Only
#include "rapidjson/include/rapidjson/document.h"
#include "rapidjson/include/rapidjson/reader.h"
#include "rapidjson/include/rapidjson/writer.h"
#include "rapidjson/include/rapidjson/filestream.h"
#include "rapidjson/include/rapidjson/stringbuffer.h"
#include "rapidjson/include/rapidjson/error/en.h"
using namespace rapidjson;
extern GetParseErrorFunc GetJSONParseErrorStr;

#endif	//RA_EXPORTS


#define RA_KEYS_DLL						"RA_Keys.dll"
#define RA_PREFERENCES_FILENAME_PREFIX	"RAPrefs_"
#define RA_UNKNOWN_BADGE_IMAGE_URI		"00000"

#define RA_DIR_OVERLAY					".\\Overlay\\"
#define RA_DIR_BASE						".\\RACache\\"
#define RA_DIR_DATA						RA_DIR_BASE##"Data\\"
#define RA_DIR_BADGE					RA_DIR_BASE##"Badge\\"
#define RA_DIR_USERPIC					RA_DIR_BASE##"UserPic\\"

#define RA_GAME_HASH_FILENAME			RA_DIR_DATA##"gamehashlibrary.txt"
#define RA_GAME_LIST_FILENAME			RA_DIR_DATA##"gametitles.txt"
#define RA_MY_PROGRESS_FILENAME			RA_DIR_DATA##"myprogress.txt"
#define RA_MY_GAME_LIBRARY_FILENAME		RA_DIR_DATA##"mygamelibrary.txt"

#define RA_OVERLAY_BG_FILENAME			RA_DIR_OVERLAY##"overlayBG.png"
#define RA_NEWS_FILENAME				RA_DIR_DATA##"ra_news.txt"
#define RA_TITLES_FILENAME				RA_DIR_DATA##"gametitles.txt"
#define RA_LOG_FILENAME					RA_DIR_DATA##"RALog.txt"


#define RA_HOST_URL						"retroachievements.org"
#define RA_HOST_IMG_URL					"i.retroachievements.org"

#define SIZEOF_ARRAY( ar )	( sizeof( ar ) / sizeof( ar[ 0 ] ) )
#define SAFE_DELETE( x )	{ if( x != nullptr ) { delete x; x = nullptr; } }

typedef unsigned char	BYTE;
typedef unsigned long	DWORD;
typedef int				BOOL;
typedef DWORD			ARGB;

//namespace RA
//{
	template<typename T>
	static inline const T& RAClamp( const T& val, const T& lower, const T& upper )
	{
		return( val < lower ) ? lower : ( ( val > upper ) ? upper : val );
	}
	
	class RARect : public RECT
	{
	public:
		RARect() {}
		RARect( LONG nX, LONG nY, LONG nW, LONG nH )
		{
			left = nX;
			right = nX + nW;
			top = nY;
			bottom = nY + nH;
		}

	public:
		inline int Width() const		{ return( right - left ); }
		inline int Height() const		{ return( bottom - top ); }
	};

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
		Core,
		Unofficial,
		Local,

		NumAchievementSetTypes
	};
	
	typedef std::vector<BYTE> DataStream;
	typedef unsigned long ByteAddress;

	typedef unsigned int AchievementID;
	typedef unsigned int LeaderboardID;
	typedef unsigned int GameID;

	char* DataStreamAsString( DataStream& stream );

	extern void RADebugLogNoFormat( const char* data );
	extern void RADebugLog( const char* sFormat, ... );
	extern BOOL DirectoryExists( const char* sPath );

	const int SERVER_PING_DURATION = 2*60;
//};
//using namespace RA;
	
#define RA_LOG RADebugLog

#ifdef _DEBUG
#undef ASSERT
#define ASSERT( x ) assert( x )
#else
#undef ASSERT
#define ASSERT( x ) {}
#endif
	
#ifndef UNUSED
#define UNUSED( x ) ( x );
#endif

extern std::string Narrow(const wchar_t* wstr);
extern std::string Narrow(const std::wstring& wstr);
extern std::wstring Widen(const char* str);
extern std::wstring Widen(const std::string& str);

//	No-ops to help convert:
//	No-ops to help convert:
extern std::wstring Widen(const wchar_t* wstr);
extern std::wstring Widen(const std::wstring& wstr);
extern std::string Narrow(const char* str);
extern std::string Narrow(const std::string& wstr);



#ifdef UNICODE
#define NativeStr(x) Widen(x)
#define NativeStrType std::wstring
#else
#define NativeStr(x) Narrow(x)
#define NativeStrType std::string
#endif
