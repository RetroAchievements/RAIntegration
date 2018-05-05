#ifndef RA_DEFS_H
#define RA_DEFS_H
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




#endif // _HAS_CXX17
//NB. These must NOT be accessible from the emulator!
//#define RA_INTEGRATION_VERSION	"0.053"

//	RA-Only
#define RAPIDJSON_HAS_STDSTRING 1
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

#endif	//RA_EXPORTS
#define _CONSTANT_VAR inline constexpr auto
#define _CONSTANT     inline constexpr

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
static inline const T& RAClamp(const T& val, const T& lower, const T& upper)
{
    return(val < lower) ? lower : ((val > upper) ? upper : val);
}

class RARect : public RECT
{
public:
    RARect() {}
    RARect(LONG nX, LONG nY, LONG nW, LONG nH)
    {
        left = nX;
        right = nX + nW;
        top = nY;
        bottom = nY + nH;
    }

public:
    inline int Width() const { return(right - left); }
    inline int Height() const { return(bottom - top); }
};

class RASize
{
public:
    RASize() : m_nWidth(0), m_nHeight(0) {}
    RASize(const RASize& rhs) : m_nWidth(rhs.m_nWidth), m_nHeight(rhs.m_nHeight) {}
    RASize(int nW, int nH) : m_nWidth(nW), m_nHeight(nH) {}

public:
    inline int Width() const { return m_nWidth; }
    inline int Height() const { return m_nHeight; }
    inline void SetWidth(int nW) { m_nWidth = nW; }
    inline void SetHeight(int nH) { m_nHeight = nH; }

private:
    int m_nWidth;
    int m_nHeight;
};

const RASize RA_BADGE_PX(64, 64);
const RASize RA_USERPIC_PX(64, 64);

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

    ResizeContent(HWND parentHwnd, HWND contentHwnd, AlignType newAlignType, bool isResize)
    {
        hwnd = contentHwnd;
        nAlignType = newAlignType;
        bResize = isResize;

        RARect rect;
        GetWindowRect(hwnd, &rect);

        pLT.x = rect.left;	pLT.y = rect.top;
        pRB.x = rect.right; pRB.y = rect.bottom;

        ScreenToClient(parentHwnd, &pLT);
        ScreenToClient(parentHwnd, &pRB);

        GetWindowRect(parentHwnd, &rect);
        nDistanceX = rect.Width() - pLT.x;
        nDistanceY = rect.Height() - pLT.y;

        if (bResize)
        {
            nDistanceX -= (pRB.x - pLT.x);
            nDistanceY -= (pRB.y - pLT.y);
        }
    }

    void Resize(int width, int height)
    {
        int xPos, yPos;

        switch (nAlignType)
        {
        case ResizeContent::ALIGN_RIGHT:
            xPos = width - nDistanceX - (bResize ? pLT.x : 0);
            yPos = bResize ? (pRB.y - pLT.x) : pLT.y;
            break;
        case ResizeContent::ALIGN_BOTTOM:
            xPos = bResize ? (pRB.x - pLT.x) : pLT.x;
            yPos = height - nDistanceY - (bResize ? pLT.y : 0);
            break;
        case ResizeContent::ALIGN_BOTTOM_RIGHT:
            xPos = width - nDistanceX - (bResize ? pLT.x : 0);
            yPos = height - nDistanceY - (bResize ? pLT.y : 0);
            break;
        default:
            xPos = bResize ? (pRB.x - pLT.x) : pLT.x;
            yPos = bResize ? (pRB.y - pLT.x) : pLT.y;
            break;
        }

        if (!bResize)
            SetWindowPos(hwnd, NULL, xPos, yPos, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
        else
            SetWindowPos(hwnd, NULL, 0, 0, xPos, yPos, SWP_NOMOVE | SWP_NOZORDER);
    }
};

enum AchievementSetType
{
    Core,
    Unofficial,
    Local,

    NumAchievementSetTypes
};






extern void RADebugLogNoFormat(const char* data);
extern void RADebugLog(const char* sFormat, ...);
extern BOOL DirectoryExists(const char* sPath);

const int SERVER_PING_DURATION = 2 * 60;
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








namespace ra {

using tstring       = std::basic_string<TCHAR>;
using DataStream    = std::basic_string<BYTE>;

using ByteAddress   = std::size_t;
using DataPos       = std::size_t;
using AchievementID = std::size_t;
using LeaderboardID = std::size_t;
using GameID        = std::size_t;

// forgot to add this
// if you are already in ra or type "using namespace ra" that's good enough
// This must be an inline namespace
inline namespace int_literals
{

// Don't feel like casting non-stop
// There's also standard literals for strings and clock types

// N.B.: The parameter can only be either "unsigned long long", char16_t, or char32_t

/// <summary>
///   <para>
///     Use it if you need an unsigned integer without the need for explicit
///     narrowing conversions.
///   </para>
///   <para>
///     usage: <c>auto some_int{21_z};</c> where <c>21</c> is
///            <paramref name="n" />.
///   </para>
/// </summary>
/// <param name="n">The integer.</param>
/// <returns>The integer as <c>std::size_t</c>.</returns>
_CONSTANT_VAR operator""_z(unsigned long long n) noexcept {
    return static_cast<std::size_t>(n);
} // end operator""_z


/// <summary>
///   <para>
///     Use it if you need an integer without the need for explicit narrowing
///     conversions.
///   </para>
///   <para>
///     usage: <c>auto some_int{1_i};</c> where <c>1</c> is
///            <paramref name="n" />.
///   </para>
/// </summary>
/// <param name="n">The integer.</param>
/// <returns>The integer as <c>std::intptr_t</c>.</returns>
_CONSTANT_VAR operator""_i(unsigned long long n) noexcept {
    return static_cast<std::intptr_t>(n);
} // end operator""_i

  // We need one for DWORD, because it doesn't match LPDWORD for some stuff
_CONSTANT_VAR operator""_dw(unsigned long long n) noexcept {
    return static_cast<_CSTD DWORD>(n);
} // end operator""_dw

// streamsize varies as well
/// <summary>
/// A literal
/// </summary>
/// <param name="n">The n.</param>
/// <returns></returns>
_CONSTANT_VAR operator""_ss(unsigned long long n) noexcept {
    return static_cast<std::streamsize>(n);
} // end operator""_ss

// Literal for unsigned short
_CONSTANT_VAR operator""_hu(unsigned long long n) noexcept {
    return static_cast<std::uint16_t>(n);
} // end operator""_hu


_CONSTANT_VAR operator""_dp(unsigned long long n) noexcept {
    return static_cast<DataPos>(n);
} // end operator""_dp

// If you follow the standard every alias is considered mutually exclusive, if not don't worry about it
// These are here if you follow the standards (starts with ISO C++11/C11) rvalue conversions


_CONSTANT_VAR operator""_ba(unsigned long long n) noexcept
{
    return static_cast<ByteAddress>(n);
} // end operator""_ba


_CONSTANT_VAR operator""_achid(unsigned long long n) noexcept {
    return static_cast<AchievementID>(n);
} // end operator""_achid


_CONSTANT_VAR operator""_lbid(unsigned long long n) noexcept {
    return static_cast<LeaderboardID>(n);
} // end operator""_lbid


_CONSTANT_VAR operator""_gameid(unsigned long long n) noexcept {
    return static_cast<GameID>(n);
} // end operator""_gameid

} // inline namespace int_literals



std::string DataStreamAsString(const DataStream& stream);
std::string Narrow(const std::wstring& wstr);
std::string Narrow(const wchar_t* wstr);
std::wstring Widen(const std::string& str);
std::wstring Widen(const char* str);





//	No-ops to help convert:
std::wstring Widen(const wchar_t* wstr);
std::wstring Widen(const std::wstring& wstr);
std::string Narrow(const char* str);
std::string Narrow(const std::string& wstr);
std::string ByteAddressToString(ByteAddress nAddr);


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
    return convert_string<DataStream>(str);
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


// Stuff in the detail namespace are things people using the RA_Integration API
// shouldn't use
namespace detail {

// There helper variable templates that are much easier to use, using these
// explicitly is not recommended because it can lead to user errors.

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

// TODO: Have implementations of concepts that are required for std::basic_string

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
_CONSTANT_VAR is_char_v = detail::is_char<CharT>::value;


// is_string helper variable template
template<typename StringType>
_CONSTANT_VAR is_string_v = detail::is_string<StringType>::value;

// is_equality_comparable helper variable template
template<typename EqualityComparable>
_CONSTANT_VAR is_equality_comparable_v = detail::is_equality_comparable<EqualityComparable>::value;

// is_lessthan_comparable helper variable template
template<typename LessThanComparable>
_CONSTANT_VAR is_lessthan_comparable_v = detail::is_lessthan_comparable<LessThanComparable>::value;

// is_nothrow_equality_comparable helper variable template
template<typename EqualityComparable>
_CONSTANT_VAR is_nothrow_equality_comparable_v = detail::is_nothrow_equality_comparable<EqualityComparable>::value;

template<typename LessThanComparable>
_CONSTANT_VAR is_nothrow_lessthan_comparable_v = detail::is_nothrow_lessthan_comparable<LessThanComparable>::value;

// We probably don't need this now but it'll be useful later when using STL filestreams instead of C filestreams
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
    typename Traits = std::char_traits<CharT>,
    class = std::enable_if_t<is_char_v<CharT>>
>
typename Traits::pos_type filesize(std::basic_string<CharT>& filename) noexcept {
    // It's always the little things...
    using file_type = std::basic_fstream<CharT>;
    file_type file{ filename, std::ios::in | std::ios::ate | std::ios::binary };
    return file.tellg();
} // end function filesize

  // All puporse string converter
  /// <summary>
  ///   Converts <paramref name="in" /> into an
  ///   <typeparamref name="OutputString" />. To use this you have to at least
  ///   specfiy <typeparamref name="OutputString" />. It won't work between
  ///   Mutli-Byte to UTF-16, Use Widen() instead.
  /// </summary>
  /// <typeparam name="InputString">
  ///   The type of string to be converted.
  /// </typeparam>
  /// <typeparam name="OutputString">
  ///   The type of string to be converted to.
  /// </typeparam>
  /// <param name="in">The string to be converted.</param>
  /// <returns>An <typeparamref name="OutputString" />.</returns>
  /// <exception cref="std::ios_base::failure">
  ///   If this was not thrown explicitly, there are no side effects. The runtime
  ///   throws this exception if any bit in the stream is not
  ///   <see cref="std::ios_base::good_bit" />.
  /// </exception>
template<
    typename OutputString,
    typename InputString,
    class = std::enable_if_t<(is_string_v<InputString> && is_string_v<OutputString>) &&
    (!std::is_same_v<InputString, OutputString>) && (!std::is_same_v<OutputString, std::wstring>)>
>
OutputString convert_string(_In_ const InputString& in) noexcept {
    using out_char_t       = typename OutputString::value_type;
    using out_stringstream = std::basic_ostringstream<out_char_t>;

    out_stringstream oss;

    for (auto& i : in) {
        oss << i;
    }

    return oss.str();
} // end function convert_string

} // namespace ra

#ifdef UNICODE
#define NativeStr(x) ra::Widen(x)
#define NativeStrType std::wstring
#else
#define NativeStr(x) ra::Narrow(x)
#define NativeStrType std::string
#endif

#endif // !RA_DEFS_H
