#ifndef RA_DEFS_H
#define RA_DEFS_H
#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WindowsX.h>
#include <ShlObj.h>
#include <tchar.h>
#include <cassert>
#include <sstream> // has string
#include <queue> // has deque, vector, algorithm
#include <map>
#include <array>




#ifndef RA_EXPORTS

//	Version Information is integrated into tags

#else


// Maybe an extra check just in-case

#define _NORETURN            [[noreturn]]

#if _HAS_CXX17
#define _DEPRECATED          [[deprecated]]
#define _DEPRECATEDR(reason) [[deprecated(reason)]]
#define _FALLTHROUGH         [[fallthrough]]//; you need ';' at the end
#define _UNUSED              [[maybe_unused]]


#endif // _HAS_CXX17
//NB. These must NOT be accessible from the emulator!
//#define RA_INTEGRATION_VERSION	"0.053"




//	RA-Only
#define RAPIDJSON_HAS_STDSTRING 1

// This is not needed the most recent version
#pragma warning(push, 1)
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

//	RA-Only
#define RAPIDJSON_HAS_STDSTRING 1
#pragma warning(push, 1)
// This is not needed the most recent version
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
//	RA-Only
#include <rapidjson/document.h> // has reader.h
#include <rapidjson/writer.h> // has stringbuffer.h
#include <rapidjson/filestream.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>
#pragma warning(pop)

using namespace rapidjson;
extern GetParseErrorFunc GetJSONParseErrorStr;
#pragma warning(pop)


using namespace std::string_literals;
//using namespace std::chrono_literals; we could use this later
namespace ra {}
using namespace ra;

#endif	//RA_EXPORTS
#define _NORETURN            [[noreturn]]

#if _HAS_CXX17
#define _DEPRECATED          [[deprecated]]
#define _DEPRECATEDR(reason) [[deprecated(reason)]]
#define _FALLTHROUGH         [[fallthrough]]//; you need ';' at the end
#define _UNUSED              [[maybe_unused]]
#define _CONSTANT_VAR        inline constexpr auto
#define _CONSTANT            inline constexpr
#else
#define _DEPRECATED          
#define _DEPRECATEDR(reason) 
#define _FALLTHROUGH         
#define _NODISCARD           
#define _UNUSED              
#define _CONSTANT_VAR constexpr auto
#define _CONSTANT     constexpr
#endif // _HAS_CXX17


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
            SetWindowPos(hwnd, nullptr, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        else
            SetWindowPos(hwnd, nullptr, 0, 0, xPos, yPos, SWP_NOMOVE | SWP_NOZORDER);
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
_NODISCARD _CONSTANT_VAR operator""_z(unsigned long long n) noexcept {
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
_NODISCARD _CONSTANT_VAR operator""_i(unsigned long long n) noexcept {
    return static_cast<std::intptr_t>(n);
} // end operator""_i

  // We need one for DWORD, because it doesn't match LPDWORD for some stuff
_NODISCARD _CONSTANT_VAR operator""_dw(unsigned long long n) noexcept {
    return static_cast<_CSTD DWORD>(n);
} // end operator""_dw

// streamsize varies as well
/// <summary>
/// A literal
/// </summary>
/// <param name="n">The n.</param>
/// <returns></returns>
_NODISCARD _CONSTANT_VAR operator""_ss(unsigned long long n) noexcept {
    return static_cast<std::streamsize>(n);
} // end operator""_ss

// Literal for unsigned short
_NODISCARD _CONSTANT_VAR operator""_hu(unsigned long long n) noexcept {
    return static_cast<std::uint16_t>(n);
} // end operator""_hu


_NODISCARD _CONSTANT_VAR operator""_dp(unsigned long long n) noexcept {
    return static_cast<DataPos>(n);
} // end operator""_dp

// If you follow the standard every alias is considered mutually exclusive, if not don't worry about it
// These are here if you follow the standards (starts with ISO C++11/C11) rvalue conversions


_NODISCARD _CONSTANT_VAR operator""_ba(unsigned long long n) noexcept
{
    return static_cast<ByteAddress>(n);
} // end operator""_ba


_NODISCARD _CONSTANT_VAR operator""_achid(unsigned long long n) noexcept {
    return static_cast<AchievementID>(n);
} // end operator""_achid


_NODISCARD _CONSTANT_VAR operator""_lbid(unsigned long long n) noexcept {
    return static_cast<LeaderboardID>(n);
} // end operator""_lbid


_NODISCARD _CONSTANT_VAR operator""_gameid(unsigned long long n) noexcept {
    return static_cast<GameID>(n);
} // end operator""_gameid

} // inline namespace int_literals



_NODISCARD std::string DataStreamAsString(const DataStream& stream);
_NODISCARD std::string Narrow(const std::wstring& wstr);
_NODISCARD std::string Narrow(std::wstring&& wstr) noexcept;
_NODISCARD std::string Narrow(const wchar_t* wstr);
_NODISCARD std::wstring Widen(const std::string& str);
_NODISCARD std::wstring Widen(std::string&& str) noexcept;
_NODISCARD std::wstring Widen(const char* str);




//	No-ops to help convert:
_NODISCARD std::wstring Widen(const wchar_t* wstr);
_NODISCARD std::wstring Widen(const std::wstring& wstr);
_NODISCARD std::string Narrow(const char* str);
_NODISCARD std::string Narrow(const std::string& wstr);
_NODISCARD std::string ByteAddressToString(ByteAddress nAddr);


template<typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
_NODISCARD _CONSTANT_VAR etoi(Enum e) -> std::underlying_type_t<Enum>
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

// function alias template
template<typename Enum>
_NODISCARD _CONSTANT_VAR to_integral = etoi<Enum>;

// We'll just force it to be a regular byte string.
_NODISCARD DataStream to_datastream(_In_ const std::string& str) noexcept;


  // This should save some pain...
template<typename SignedType, class = std::enable_if_t<std::is_signed_v<SignedType>>>
_Use_decl_annotations_
_NODISCARD _CONSTANT_VAR to_unsigned(_In_ SignedType st) noexcept -> std::make_unsigned_t<SignedType>
{
    using unsigned_t = std::make_unsigned_t<SignedType>;
    return static_cast<unsigned_t>(st);
}

template<typename UnsignedType, class = std::enable_if_t<std::is_unsigned_v<UnsignedType>>>
_Use_decl_annotations_
_NODISCARD _CONSTANT_VAR to_signed(_In_ UnsignedType ust) noexcept -> std::make_signed_t<UnsignedType>
{
    using signed_t = std::make_signed_t<UnsignedType>;
    return static_cast<signed_t>(ust);
}


// Stuff in the detail namespace are things people using the RA_Integration API
// shouldn't use directly.
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
/// <typeparam name="CharT">A type to be evaluated</typeparam>
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
_NODISCARD _CONSTANT_VAR is_string_v = detail::is_string<StringType>::value;

// is_equality_comparable helper variable template
template<typename EqualityComparable>
_NODISCARD _CONSTANT_VAR is_equality_comparable_v = detail::is_equality_comparable<EqualityComparable>::value;

// is_lessthan_comparable helper variable template
template<typename LessThanComparable>
_NODISCARD _CONSTANT_VAR is_lessthan_comparable_v = detail::is_lessthan_comparable<LessThanComparable>::value;

// is_nothrow_equality_comparable helper variable template
template<typename EqualityComparable>
_NODISCARD _CONSTANT_VAR is_nothrow_equality_comparable_v = detail
::is_nothrow_equality_comparable<EqualityComparable>::value;

template<typename LessThanComparable>
_NODISCARD _CONSTANT_VAR is_nothrow_lessthan_comparable_v = detail
::is_nothrow_lessthan_comparable<LessThanComparable>::value;

namespace detail {

/// <summary>
///   Casts <typeparamref name="InputString" /> into an
///   <typeparamref name="OutputString" />. This won't work with multi-byte
///   strings, only byte strings. It can be used to convert <c>std::string</c> to
///   and from <c>std::wstring</c> but that's only if they are byte strings.
/// </summary>
/// <typeparam name="OutputString">
///   The string type specified for conversion.
/// </typeparam>
/// <typeparam name="InputString">
///   The string type to be converted to, as long as an intellisense error
///   doesn't occur, it should be fine.
/// </typeparam>
/// <param name="str">The string to be converted.</param>
/// <returns>
///   <typeparamref name="InputString" /> as an
///   <typeparamref name="OutputString" />.
/// </returns>
/// <exception cref="std::ios_base::failure">
///   If this exception is thrown, this operation as no effect and the
///   <typeparamref name="InputString" /> is in a valid state.
/// </exception>
/// <remarks>
///   At least <typeparamref name="OutputString" /> needs to be specified and
///   <typeparamref name="InputString" /> must not be the same as
///   <typeparamref name="OutputString" />. Use <see cref="ra::Widen" /> or
///   <see cref="ra::Narrow" /> if you need a Multi-byte to Unicode string
///   conversion.
/// </remarks>
template<
    typename OutputString,
    typename InputString,
    class = std::enable_if_t<(not std::is_same_v<OutputString, InputString>) and
    (is_string_v<OutputString> and is_string_v<InputString>) and
    (!std::is_same_v<OutputString, std::u16string> and !std::is_same_v<InputString, std::u16string>) and
    (!std::is_same_v<OutputString, std::u32string> and !std::is_same_v<InputString, std::u32string>)>
>
_NODISCARD OutputString string_cast(const InputString& str) noexcept
{
    if (str.empty())
        return OutputString{};

    using out_ostringstream = std::basic_ostringstream<typename OutputString::value_type>;
    out_ostringstream oss;

    for (auto& i : str)
    {
        if (std::is_same_v<OutputString, std::string> and std::is_same_v<InputString, DataStream>)
            oss << static_cast<std::string::value_type>(i); 
        else if (std::is_same_v<OutputString, DataStream> and std::is_same_v<InputString, std::string>)
            oss << static_cast<DataStream::value_type>(i);
        else if (std::is_same_v<OutputString, std::wstring> and std::is_same_v<InputString, std::string>)
            oss << std::wcout.widen(i);
        else if (std::is_same_v<OutputString, std::string> and std::is_same_v<InputString, std::wstring>)
            oss << std::wcout.narrow(i);
    }
    return oss.str();
} // end function string_cast

} // namespace detail


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
_Use_decl_annotations_
_NODISCARD typename Traits::pos_type filesize(_In_ std::basic_string<CharT>& filename) noexcept {
    // It's always the little things...
    using file_type = std::basic_fstream<CharT>;
    file_type file{ filename, std::ios::in | std::ios::ate | std::ios::binary };
    return file.tellg();
} // end function filesize

} // namespace ra




#ifdef UNICODE
#define NativeStr(x) ra::Widen(x)
#define NativeStrType std::wstring
#else
#define NativeStr(x) ra::Narrow(x)
#define NativeStrType std::string
#endif

#endif // !RA_DEFS_H
