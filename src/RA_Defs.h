#ifndef RA_DEFS_H
#define RA_DEFS_H
#pragma once

// Windows stuff we DO need, they are commented out to show we need them, if for
// some reason you get a compiler error put the offending NO* define here
/*
    #define NOUSER
    #define NOCTLMGR
    #define NOMB
    #define NOWINMESSAGES
    #define NONLS
    #define NOVIRTUALKEYCODES
    #define NOGDI
    #define NOMSG
    #define NOMENUS
    #define NOCOLOR
    #define NOWINSTYLES
    #define NODRAWTEXT
    #define NOSHOWWINDOW
    #define NOTEXTMETRIC
    #define NOWINOFFSETS
    #define NORASTEROPS
    #define NOOPENFILE
*/


// Windows stuff we don't need
#define WIN32_LEAN_AND_MEAN 1
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define OEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOKERNEL
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX  

#include <Windows.h>
#include <WindowsX.h>
#pragma warning(push)
#pragma warning(disable : 4091) // 'typedef ': ignored on left of 'tagGPFIDL_FLAGS' when no variable is declared
#include <ShlObj.h>
#pragma warning(pop)

#include <tchar.h>

#include <sstream>
#include <string_view> 
#include <queue>
#include <map>
#include <ciso646>

#ifndef RA_EXPORTS
#include <cassert> // RapidJSON includes this
//	Version Information is integrated into tags

#else



//NB. These must NOT be accessible from the emulator!
//#define RA_INTEGRATION_VERSION	"0.053"




//	RA-Only
#define RAPIDJSON_HAS_STDSTRING 1

// This is not needed the most recent version
#pragma warning(push, 1)
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

//	RA-Only
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/filestream.h>
#include <rapidjson/error/en.h>
using namespace rapidjson;
extern GetParseErrorFunc GetJSONParseErrorStr;
#pragma warning(pop)
#endif	//RA_EXPORTS


// NB: Do NOT use _NORETURN on setters, varargs (C), C Asserts, or any void
//     function that "returns", [[noreturn]] expects that you throw an exception
//     on failure so if you put [[noreturn]] and "return" will throw a runtime
//     error saying the function call was not saved properly.
#define _NORETURN            [[noreturn]]

// Maybe an extra check just in-case
#if _HAS_CXX17
#define _DEPRECATED          [[deprecated]]
#define _DEPRECATEDR(reason) [[deprecated(reason)]]
#define _FALLTHROUGH         [[fallthrough]]//; you need ';' at the end
#define _UNUSED              [[maybe_unused]]
// constexpr locals can't have inline, just namespace and globals
#define _CONSTANT_VAR        inline constexpr auto
#else
#define _NODISCARD           _Check_return_
#define _DEPRECATED          __declspec(deprecated)
#define _DEPRECATEDR(reason) _CRT_DEPRECATE_TEXT(reason)
#define _FALLTHROUGH         __fallthrough//; you need ';' at the end
#define _UNUSED
#define _CONSTANT_VAR        constexpr auto
#endif // _HAS_CXX17

// TBD: Maybe add some macros for local constexpr and variants without the auto
#define _CONSTANT_FN _CONSTANT_VAR

// TODO: Put these in namespace ra later so we don't have to prefix everything
//       with RA since it should be implied everything here is for RA

// NB: If you want to make absolutely certain these variables/functions are
//     evaluated at compile time, run this under a debugger and if a breakpoint
//     get hit then it was evaluated at compile-time. 
//     

_CONSTANT_VAR RA_KEYS_DLL                    = TEXT("RA_Keys.dll");
_CONSTANT_VAR RA_INTEGRATION_DLL             = TEXT("RA_Integration.dll");
_CONSTANT_VAR RA_PREFERENCES_FILENAME_PREFIX = TEXT("RAPrefs_");
_CONSTANT_VAR RA_UNKNOWN_BADGE_IMAGE_URI     = TEXT("00000");

_CONSTANT_VAR RA_DIR_OVERLAY                 = TEXT(".\\Overlay\\");
_CONSTANT_VAR RA_DIR_BASE                    = TEXT(".\\RACache\\");

_CONSTANT_VAR RA_DIR_DATA                    = TEXT(".\\RACache\\Data\\");
_CONSTANT_VAR RA_DIR_BADGE                   = TEXT(".\\RACache\\Badge\\");
_CONSTANT_VAR RA_DIR_USERPIC                 = TEXT(".\\RACache\\UserPic\\");
_CONSTANT_VAR RA_DIR_BOOKMARKS               = TEXT(".\\RACache\\Bookmarks\\");

_CONSTANT_VAR RA_GAME_HASH_FILENAME          = TEXT(".\\RACache\\Data\\gamehashlibrary.txt");
_CONSTANT_VAR RA_GAME_LIST_FILENAME          = TEXT(".\\RACache\\Data\\gametitles.txt");
_CONSTANT_VAR RA_MY_PROGRESS_FILENAME        = TEXT(".\\RACache\\Data\\myprogress.txt");
_CONSTANT_VAR RA_MY_GAME_LIBRARY_FILENAME    = TEXT(".\\RACache\\Data\\mygamelibrary.txt");

_CONSTANT_VAR RA_OVERLAY_BG_FILENAME         = TEXT(".\\Overlay\\overlayBG.png");
_CONSTANT_VAR RA_NEWS_FILENAME               = TEXT(".\\RACache\\Data\\ra_news.txt");
_CONSTANT_VAR RA_TITLES_FILENAME             = TEXT(".\\RACache\\Data\\gametitles.txt");
_CONSTANT_VAR RA_LOG_FILENAME                = TEXT(".\\RACache\\Data\\RALog.txt");

_CONSTANT_VAR RA_HOST_URL                    = TEXT("retroachievements.org");
_CONSTANT_VAR RA_HOST_IMG_URL                = TEXT("i.retroachievements.org");

// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365530(v=vs.85).aspx
#if _MBCS
_CONSTANT_VAR RA_MAX_PATH = _MAX_PATH;
#elif _UNICODE
_CONSTANT_VAR RA_MAX_PATH = 32'767;
#else
#error Illegal character-set detected! Please specify a character-set!
#endif // _MBCS


template<typename Array, class = std::enable_if_t<std::is_array_v<Array>>>
_CONSTANT_FN SIZEOF_ARRAY(_In_ const Array& ar) noexcept
->decltype(sizeof(ar) / sizeof(ar[0])) { return { sizeof(ar) / sizeof(ar[0]) }; }

template<typename Pointer, class = std::enable_if_t<std::is_pointer_v<Pointer>>>
_CONSTANT_FN SAFE_DELETE(_In_ Pointer x) noexcept
{
    if (x)
    {
        delete x;
        x = nullptr;
    }
}


typedef unsigned char	BYTE;
typedef unsigned long	DWORD;
typedef int				BOOL;
typedef DWORD			ARGB;

//namespace RA
//{

_NODISCARD inline constexpr bool operator==(_In_ const RECT& a, _In_ const RECT& b) noexcept
{
    return{ a.left   == b.left   and
            a.top    == b.top    and
            a.right  == b.right  and
            a.bottom == b.bottom };
}

// What should we do for less-than? Compare the area, quadrants?

// We can make this into a literal type by removing inheritance
class RARect
{
public:
    // Constructors are similar to SetRect
    inline constexpr RARect(_In_ const RECT& rect) noexcept : m_Rect{ rect } { }
    inline constexpr RARect(_In_ const POINT& XY, _In_ const POINT& WH) noexcept :
        m_Rect{ XY.x,         // left
                XY.y,         // top
                XY.x + WH.x,  // right
                XY.y + WH.y } // bottom
    {
    }

    inline constexpr RARect(_In_ LONG nX, _In_ LONG nY, _In_ LONG nW, _In_ LONG nH) noexcept :
        m_Rect{ nX,       // left
                nY,       // top
                nX + nW,  // right
                nY + nH } // bottom
    {
    }

    RARect(const RARect&) = delete;
    RARect& operator=(const RARect&) = delete;
    inline constexpr RARect(RARect&&) noexcept = default;
    _NODISCARD inline constexpr RARect& operator=(RARect&&) noexcept = default;

    inline constexpr RARect() noexcept = default; 

    _NODISCARD explicit constexpr operator RECT() const noexcept { return m_Rect; }
    _NODISCARD explicit constexpr operator bool() const noexcept { return std::empty(*this); }
    // This one can't be const
    _NODISCARD explicit constexpr operator LPRECT() noexcept { return &m_Rect; }

public:
    _NODISCARD inline constexpr LONG Width() const noexcept { return{ m_Rect.right - m_Rect.left }; }
    _NODISCARD inline constexpr LONG Height() const noexcept { return{ m_Rect.bottom - m_Rect.top }; }

    _NODISCARD inline constexpr LONG Area() const noexcept { return{ Width()*Height() }; }
    _NODISCARD inline constexpr bool empty() const noexcept { return{ Area() == 0L }; }

    // Cloning just in-case we need a "copy", similar to CopyRect
    _NORETURN inline constexpr void Clone(_Out_ RARect& rect) noexcept { rect.m_Rect = this->m_Rect; }
private:
    RECT m_Rect = RECT{};

    // Similar to EqualRect
    _NODISCARD inline constexpr bool friend operator==(_In_ const RARect& a, _In_ const RARect& b) noexcept
    {
        return{ a.m_Rect == b.m_Rect };
    }
};

// We can make a generic one later
_NODISCARD inline constexpr bool operator==(_In_ const SIZE& a, _In_ const SIZE& b) noexcept
{
    return{ (a.cx == b.cx) and (a.cy == b.cy) };
}

class RASize
{
public:
    ~RASize() noexcept = default;
    RASize(const RASize&) = delete;
    RASize operator=(const RASize&) = delete;
    inline constexpr RASize(RASize&&) noexcept = default;
    _NODISCARD inline constexpr RASize& operator=(RASize&&) noexcept = default;
    inline constexpr RASize(_In_ int nW, _In_ int nH) noexcept : m_nWidth{ nW }, m_nHeight{ nH } {}
    inline constexpr RASize() noexcept = default;

    
    inline constexpr operator SIZE() const noexcept { return SIZE{ m_nWidth, m_nHeight }; }
    inline constexpr operator LPSIZE() const noexcept {
        SIZE mySize{ m_nWidth, m_nHeight };
        return &mySize;
    }

public:
    _NODISCARD _CONSTANT_FN Width() const noexcept { return m_nWidth; }
    _NODISCARD _CONSTANT_FN Height() const noexcept { return m_nHeight; }
    _CONSTANT_FN SetWidth(_In_ int nW) noexcept { m_nWidth = nW; }
    _CONSTANT_FN SetHeight(_In_ int nH) noexcept { m_nHeight = nH; }
    _NODISCARD _CONSTANT_FN Area() const noexcept { return m_nWidth*m_nHeight; }
    _NODISCARD _CONSTANT_FN empty() const noexcept { return (Area() == 0); }
    inline constexpr operator bool() const noexcept { return std::empty(*this); }

    // Just in-case we need a "copy"
    _NORETURN _CONSTANT_FN Clone(_Out_ RASize& sz) const noexcept {
        sz.m_nWidth  = this->m_nWidth;
        sz.m_nHeight = this->m_nHeight;
    }
private:
    int m_nWidth{ 0 };
    int m_nHeight{ 0 };

    _NODISCARD inline constexpr bool friend operator==(_In_ const RASize& a, _In_ const RASize& b) noexcept
    {
        return{ (a.m_nWidth == b.m_nWidth) and (a.m_nHeight == b.m_nHeight) };
    }
};

inline constexpr RASize RA_BADGE_PX{ 64, 64 };
inline constexpr RASize RA_USERPIC_PX{ 64, 64 };

// Let's see it in action
static_assert(RA_BADGE_PX == RA_USERPIC_PX);


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
    inline constexpr ResizeContent() noexcept = default;
    // This one can't be constexpr
    ResizeContent(_In_ HWND parentHwnd, _In_ HWND contentHwnd, _In_ AlignType newAlignType,
        _In_ bool isResize) noexcept;
    // TODO: make this defaulted once the RAII wrapper for HWND gets in
    ~ResizeContent() noexcept = default;
    ResizeContent(const ResizeContent&) = delete;
    ResizeContent& operator=(const ResizeContent&) = delete;
    inline constexpr ResizeContent(ResizeContent&&) noexcept = default;
    _NODISCARD inline constexpr ResizeContent& operator=(ResizeContent&&) noexcept = default;

    _NORETURN void Resize(_In_ int width, _In_ int height) noexcept;

private:
    // So we don't have to define a destructor, plus if it doesn't use default_delete we have initialize it explicitly
    std::unique_ptr<HWND__, decltype(&::DestroyWindow)> hwnd{ nullptr, &::DestroyWindow };
    POINT pLT = POINT{};
    POINT pRB = POINT{};
    AlignType nAlignType = AlignType{};
    int nDistanceX{ 0 };
    int nDistanceY{ 0 };
    bool bResize{ false };
};

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

_NODISCARD char* DataStreamAsString(_Inout_ DataStream& stream);

void RADebugLogNoFormat(_In_z_ const char* data);
void RADebugLog(_In_z_ const char* sFormat, ...);
BOOL DirectoryExists(_In_z_ const char* sPath);

_CONSTANT_VAR SERVER_PING_DURATION{ 2 * 60 };
//};
//using namespace RA;


_CONSTANT_VAR& RA_LOG = RADebugLog;

#ifdef _DEBUG
#ifndef RA_UTEST
#undef ASSERT
inline auto ASSERT(_In_ bool x) noexcept->decltype(assert(x)) { assert(x); }
#else
#undef ASSERT
inline auto ASSERT(_UNUSED _In_ bool x) noexcept->void {}
#endif
#else
#undef ASSERT
#define ASSERT( x ) {}
#endif



_NODISCARD std::string Narrow(_In_z_ const wchar_t* wstr);
_NODISCARD std::string Narrow(_In_ const std::wstring& wstr);
_NODISCARD std::wstring Widen(_In_z_ const char* str);
_NODISCARD std::wstring Widen(_In_ const std::string& str);

//	No-ops to help convert:
//	No-ops to help convert:
_NODISCARD std::wstring Widen(_In_z_ const wchar_t* wstr);
_NODISCARD std::wstring Widen(_In_ const std::wstring& wstr);
_NODISCARD std::string Narrow(_In_z_ const char* str);
_NODISCARD std::string Narrow(_In_ const std::string& wstr);

typedef std::basic_string<TCHAR> tstring;
using tstring_view = std::basic_string_view<TCHAR>;

#ifdef UNICODE
using NonNativeString  = std::string;
using NonNativeCString = NonNativeString::const_pointer;

using NativeString     = std::wstring;
using NativeCharType   = NativeString::value_type;
using NativeCString    = NativeString::const_pointer;
#else
using NonNativeString  = std::wstring;
using NonNativeCString = NonNativeString::const_pointer;

using NativeString     = std::string;
using NativeCharType   = NativeString::value_type;
using NativeCString    = NativeString::const_pointer;
#endif // UNICODE


_NODISCARD NativeString NativeStr(_In_z_ NonNativeCString str) noexcept;
_NODISCARD NativeString NativeStr(_In_ const NonNativeString& str) noexcept;
_NODISCARD NativeString NativeStr(_In_ NonNativeString&& str) noexcept;
_NODISCARD NativeString NativeStr(_In_ NativeString&& str) noexcept;
_NODISCARD NativeString NativeStr(_In_ const NativeString& str) noexcept;
_NODISCARD NativeString NativeStr(_In_z_ NativeCString str) noexcept;



#endif // !RA_DEFS_H
