#pragma once

// Note to self (SyrianBallaS/Samer) don't touch includes until making a pch
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
//#include <memory>
//#include <thread>
//#include <mutex>

#ifndef RA_EXPORTS

//	Version Information is integrated into tags

#else
// if you really want to use null instead of nullptr like .net
#ifndef null
#define null nullptr
#endif // !null

// Maybe an extra check just in-case

#if _HAS_CXX17
#define _DEPRECATED          [[deprecated]]
#define _DEPRECATEDR(reason) [[deprecated(reason)]]
#define _FALLTHROUGH         [[fallthrough]]//; you need ';' at the end
#define _NODISCARD           [[nodiscard]]
#define _NORETURN            [[noreturn]]
#define _UNUSED              [[maybe_unused]]

// Disables the use of const_casts, if you get an error, it's not a literal
// type. You could use it on functions but they will need a deduction guide
// That would probably be better with a forwarding function
#define _CONSTANT_VAR inline constexpr auto
#define _CONSTANT     inline constexpr



#endif // _HAS_CXX17
//NB. These must NOT be accessible from the emulator!
//#define RA_INTEGRATION_VERSION	"0.053"

//	RA-Only
#define RAPIDJSON_HAS_STDSTRING 1
// This shit man...
#pragma warning(push, 1)
// This is not needed the most recent version
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
//	RA-Only
#include "rapidjson/include/rapidjson/document.h"
#include "rapidjson/include/rapidjson/reader.h"
#include "rapidjson/include/rapidjson/writer.h"
#include "rapidjson/include/rapidjson/filestream.h"
#include "rapidjson/include/rapidjson/stringbuffer.h"
#include "rapidjson/include/rapidjson/error/en.h"
#pragma warning(pop)
using namespace rapidjson;
extern GetParseErrorFunc GetJSONParseErrorStr;



using namespace std::string_literals;
//using namespace std::chrono_literals; we could use this later
namespace ra {}
using namespace ra;
#define _RA ::ra::
#define _DETAIL ::ra::detail::
#endif	//RA_EXPORTS


#define RA_KEYS_DLL						"RA_Keys.dll"
#define RA_PREFERENCES_FILENAME_PREFIX	"RAPrefs_"
#define RA_UNKNOWN_BADGE_IMAGE_URI		"00000"

#define RA_DIR_OVERLAY					".\\Overlay\\"
#define RA_DIR_BASE						".\\RACache\\"
#define RA_DIR_DATA						RA_DIR_BASE##"Data\\"
#define RA_DIR_BADGE					RA_DIR_BASE##"Badge\\"
#define RA_DIR_USERPIC					RA_DIR_BASE##"UserPic\\"
#define RA_DIR_BOOKMARKS				RA_DIR_BASE##"Bookmarks\\"

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

using ARGB = DWORD;

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

	class ResizeContent
	{
	public:
		enum AlignType
		{
			NO_ALIGN,
			ALIGN_RIGHT,
			ALIGN_BOTTOM,
			ALIGN_BOTTOM_RIGHT
		};

	public:
		HWND hwnd;
		POINT pLT;
		POINT pRB;
		AlignType nAlignType;
		int nDistanceX;
		int nDistanceY;
		bool bResize;

		ResizeContent( HWND parentHwnd, HWND contentHwnd, AlignType newAlignType, bool isResize )
		{
			hwnd = contentHwnd;
			nAlignType = newAlignType;
			bResize = isResize;
			
			RARect rect;
			GetWindowRect( hwnd, &rect );

			pLT.x = rect.left;	pLT.y = rect.top;
			pRB.x = rect.right; pRB.y = rect.bottom;

			ScreenToClient( parentHwnd, &pLT );
			ScreenToClient( parentHwnd, &pRB );

			GetWindowRect ( parentHwnd, &rect );
			nDistanceX = rect.Width() - pLT.x;
			nDistanceY = rect.Height() - pLT.y;
			
			if ( bResize )
			{
				nDistanceX -= (pRB.x - pLT.x);
				nDistanceY -= (pRB.y - pLT.y);
			}
		}

		void Resize(int width, int height)
		{
			int xPos, yPos;

			switch ( nAlignType )
			{
				case ResizeContent::ALIGN_RIGHT:
					xPos = width - nDistanceX - ( bResize ? pLT.x : 0 );
					yPos = bResize ? ( pRB.y - pLT.x ) : pLT.y;
					break;
				case ResizeContent::ALIGN_BOTTOM:
					xPos = bResize ? ( pRB.x - pLT.x ) : pLT.x;
					yPos = height - nDistanceY - ( bResize ? pLT.y : 0 );
					break;
				case ResizeContent::ALIGN_BOTTOM_RIGHT:
					xPos = width - nDistanceX - ( bResize ? pLT.x : 0 );
					yPos = height - nDistanceY - ( bResize ? pLT.y : 0 );
					break;
				default:
					xPos = bResize ? ( pRB.x - pLT.x ) : pLT.x;
					yPos = bResize ? ( pRB.y - pLT.x ) : pLT.y;
					break;
			}

			if ( !bResize )
				SetWindowPos( hwnd, NULL, xPos, yPos, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER );
			else
				SetWindowPos( hwnd, NULL, 0, 0, xPos, yPos, SWP_NOMOVE | SWP_NOZORDER );
		}
	};

	enum AchievementSetType
	{
		Core,
		Unofficial,
		Local,

		NumAchievementSetTypes
	};
	
	


	

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






#ifdef UNICODE
#define NativeStr(x) Widen(x)
#define NativeStrType std::wstring
#else
#define NativeStr(x) Narrow(x)
#define NativeStrType std::string
#endif

namespace ra {
using cstring       = const char*;
using cwstring      = const wchar_t*;
using tstring       = std::basic_string<TCHAR>;

template<typename CharT>
using string_t = std::basic_string<CharT>;

using DataStream     = std::basic_string<BYTE>;
using ustring        = DataStream;
using uofstream      = std::basic_ofstream<BYTE>;
using uifstream      = std::basic_ifstream<BYTE>;
using uostringstream = std::basic_ostringstream<BYTE>;
using uistringstream = std::basic_istringstream<BYTE>;
using uostream       = std::basic_ostream<BYTE>;
using uistream       = std::basic_istream<BYTE>;

using ByteAddress   = std::size_t;
using AchievementID = std::size_t;
using LeaderboardID = std::size_t;
using GameID        = std::size_t;

// forgot to add this
// if you are already in ra or type "using namespace ra" that's good enough
inline namespace int_literals
{

// Don't feel like casting non-stop
// There's also standard literals for strings on clock types

// Use it if you need an unsigned int 
// Not using _s because that's the literal for std::string
// usage: auto a{19_z};
_CONSTANT_VAR operator""_z(unsigned long long n) noexcept
{
    return static_cast<std::size_t>(n);
} // end operator""_z

  // Use it if you need a signed int
  // usage: auto a{9_i};
_CONSTANT_VAR operator""_i(unsigned long long n) noexcept
{
    return static_cast<std::intptr_t>(n);
} // end operator""_i

  // We need one for DWORD, because it doesn't match LPDWORD for some stuff
_CONSTANT_VAR operator""_dw(unsigned long long n) noexcept
{
    return static_cast<_CSTD DWORD>(n);
} // end operator""_dw

  // streamsize varies as well
_CONSTANT_VAR operator""_ss(unsigned long long n) noexcept
{
    return static_cast<std::streamsize>(n);
} // end operator""_ss


} // inline namespace int_literals


inline namespace conversions {

// Noticed some weird uses of rvalues returning as lvalues, whatever they aren't used anywhere

std::string DataStreamAsString(const DataStream& stream);
extern std::string Narrow(const std::wstring& wstr);
extern std::string Narrow(const wchar_t* wstr);
extern std::wstring Widen(const std::string& str);
extern std::wstring Widen(const char* str);

// There was some weird shit with TCHAR in some files
template<typename CharT, class = std::enable_if_t<typeid(CharT) != typeid(char)>>
std::string Narrow(const string_t<CharT>& str) {
    std::ostringstream oss;

    for (auto& i : str) {
        oss << static_cast<char>(i);
    }
    return oss.str();
}

template<typename CharT, class = std::enable_if_t<typeid(CharT) != typeid(wchar_t)>>
std::wstring Widen(const string_t<CharT>& str) {
    std::wostringstream oss;

    for (auto& i : str) {
        oss << static_cast<wchar_t>(i);
    }
    return oss.str();
}

//	No-ops to help convert:
extern std::wstring Widen(const wchar_t* wstr);
extern std::wstring Widen(const std::wstring& wstr);
extern std::string Narrow(const char* str);
extern std::string Narrow(const std::string& wstr);

template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_CONSTANT_VAR etoi(Enum e) -> std::underlying_type_t<Enum>
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

// function alias template
template<typename Enum>
_CONSTANT_VAR to_integral = etoi<Enum>;

/// <summary>
///   Converts a standard string with a signed character type to a
///   <c>DataStream</c>.
/// </summary>
/// <param name="str">The string.</param>
/// <typeparam name="CharT">
///   The character type, must be signed or you will get an intellisense error.
/// </typeparam>
/// <returns><paramref name="str" /> as a <c>DataStream</c>.</returns>
/// <remarks>
///   <c>DataStream</c> is just an alias for an unsigned standard string.
/// </remarks>
template<
    typename CharT,
    class = std::enable_if_t<is_char_v<CharT> && std::is_signed_v<CharT>>
>
DataStream stodata_stream(const std::basic_string<CharT>& str) noexcept
{
    uostringstream doss;
    
    for (auto& i : str)
        doss << static_cast<typename DataStream::value_type>(i);

    auto dstr = doss.str();

    return dstr;
} // end function stodata_stream


  // This should save some pain...
template<typename SignedType, class = std::enable_if_t<std::is_signed_v<SignedType>>>
_CONSTANT_VAR to_unsigned(SignedType st) noexcept -> std::make_unsigned_t<SignedType>
{
    using unsigned_t = std::make_unsigned_t<SignedType>;
    return static_cast<unsigned_t>(st);
}

template<typename UnsignedType, class = std::enable_if_t<std::is_unsigned_v<UnsignedType>>>
_CONSTANT_VAR to_signed(UnsignedType ust) noexcept -> std::make_signed_t<UnsignedType>
{
    using signed_t = std::make_signed_t<UnsignedType>;
    return static_cast<signed_t>(ust);
}

} // inline namespace conversions


namespace detail {

// A lot of these are just extra compile-time validation to ensure types are used correctly
// Like you will get an intellisense error instead of a runtime error

/// <summary>
///   A check to tell whether a type is a known character type. If this
///   function was reached, <typeparamref name="CharT" /> is a known character
///   type.
/// </summary>
/// <typeparam name="CharT">A type to be evalualted</typeparam>
template<typename CharT>
struct is_char :
    std::bool_constant<std::is_integral_v<CharT> &&
    (std::is_same_v<CharT, char> || std::is_same_v<CharT, wchar_t> ||
        std::is_same_v<CharT, char16_t> || std::is_same_v<CharT, char32_t> ||
        std::is_same_v<CharT, unsigned char>)> {};

// for the hell of it
template<
    typename StringType,
    class = std::void_t<>
>
struct is_string : std::false_type {};

// There really should be a buttload of checks but that'll take too long for now
template<typename StringType>
struct is_string<StringType, std::enable_if_t<
    is_char<typename std::char_traits<typename StringType::value_type>::char_type>::value>>
    : std::true_type {};

template<
    typename EqualityComparable,
    class = std::void_t<>
>
struct is_equality_comparable : std::false_type {};


template<typename EqualityComparable>
struct is_equality_comparable<EqualityComparable,
    std::enable_if_t<std::is_convertible_v<
    decltype(std::declval<EqualityComparable>() == std::declval<EqualityComparable>()), bool>
    >> : std::true_type {};

template<
    typename LessThanComparable,
    class = std::void_t<>
>
struct is_lessthan_comparable : std::false_type {};


template<typename LessThanComparable>
struct is_lessthan_comparable<LessThanComparable,
    std::enable_if_t<std::is_convertible_v<
    decltype(std::declval<LessThanComparable>() < std::declval<LessThanComparable>()), bool>
    >> : std::true_type{};

// nothrows: for each one of these the operator has to be marked with noexcept to pass
template<typename EqualityComparable>
struct is_nothrow_equality_comparable :
    std::bool_constant<noexcept(is_equality_comparable<EqualityComparable>::value)> {};

template<typename LessThanComparable>
struct is_nothrow_lessthan_comparable :
    std::bool_constant<noexcept(is_lessthan_comparable<LessThanComparable>::value)> {};

} // namespace detail


// is_char helper variable template
template<typename CharT>
_CONSTANT_VAR is_char_v = _DETAIL is_char<CharT>::value;


// is_string helper variable template
template<typename StringType>
_CONSTANT_VAR is_string_v = _DETAIL is_string<StringType>::value;

// is_equality_comparable helper variable template
template<typename EqualityComparable>
_CONSTANT_VAR is_equality_comparable_v = _DETAIL is_equality_comparable<EqualityComparable>::value;

// is_lessthan_comparable helper variable template
template<typename LessThanComparable>
_CONSTANT_VAR is_lessthan_comparable_v = _DETAIL is_lessthan_comparable<LessThanComparable>::value;

// is_nothrow_equality_comparable helper variable template
template<typename EqualityComparable>
_CONSTANT_VAR is_nothrow_equality_comparable_v = _DETAIL is_nothrow_equality_comparable<EqualityComparable>::value;

template<typename LessThanComparable>
_CONSTANT_VAR is_nothrow_lessthan_comparable_v = _DETAIL is_nothrow_lessthan_comparable<LessThanComparable>::value;

// These are down here for demo purposes only to get the idea
// These are here just to test that the validations works
// if they don't work then the type you are using is an oxymoron
//static_assert(is_char_v<BYTE>);
//static_assert(is_string_v<DataStream>);
//static_assert(is_equality_comparable_v<std::vector<int>>);
//static_assert(is_nothrow_equality_comparable_v<std::shared_ptr<int>>);
//static_assert(is_lessthan_comparable_v<std::move_iterator<int>>);
//static_assert(is_nothrow_lessthan_comparable_v<std::thread>);


// We probably don't need this now but it'll be useful if we upgrade rapidjson
/// <summary>Calculates the size of any standard fstream.</summary>
/// <param name="filename">The filename.</param>
/// <typeparam name="CharT">
///   The character type, it should be auto deduced if valid. Otherwise you'll
///   get an intellisense error.
/// </typeparam>
/// <typeparam name="Traits">
///   The character traits of <typeparamref name="CharT" />.
/// </typeparam>
/// <returns>The size of the file stream.</returns>
template<
    typename CharT,
    typename Traits = _STD char_traits<CharT>,
    class = _STD enable_if_t<is_char_v<CharT>>
>
typename Traits::pos_type filesize(string_t<CharT>& filename) noexcept {
    // It's always the little things...
    using file_type = _STD basic_fstream<CharT>;
    file_type file{ filename, _STD ios::in | _STD ios::ate | _STD ios::binary };
    return file.tellg();
} // end function filesize


} // namespace ra
