#ifndef RA_DEFS_H
#define RA_DEFS_H
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WindowsX.h>
#include <ShlObj.h>
#include <tchar.h>
#include <cassert>
#include <sstream>
#include <queue>
#include <map>
#include <array>
#include <functional>

#ifdef WIN32_LEAN_AND_MEAN
#include <MMSystem.h>
#include <ShellAPI.h>
#endif // WIN32_LEAN_AND_MEAN




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
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/filestream.h>
#include <rapidjson/error/en.h>
using namespace rapidjson;
extern GetParseErrorFunc GetJSONParseErrorStr;
#pragma warning(pop)
#endif	//RA_EXPORTS


inline constexpr auto RA_KEYS_DLL                    = TEXT("RA_Keys.dll");
inline constexpr auto RA_PREFERENCES_FILENAME_PREFIX = TEXT("RAPrefs_");
inline constexpr auto RA_UNKNOWN_BADGE_IMAGE_URI	 = TEXT("00000");

inline constexpr auto RA_DIR_OVERLAY                 = TEXT(".\\Overlay\\");
inline constexpr auto RA_DIR_BASE	                 = TEXT(".\\RACache\\");

inline constexpr auto RA_DIR_DATA                    = TEXT(".\\RACache\\Data\\");
inline constexpr auto RA_DIR_BADGE                   = TEXT(".\\RACache\\Badge\\");
inline constexpr auto RA_DIR_USERPIC                 = TEXT(".\\RACache\\UserPic\\");
inline constexpr auto RA_DIR_BOOKMARKS               = TEXT(".\\RACache\\Bookmarks\\");

inline constexpr auto RA_GAME_HASH_FILENAME	         = TEXT(".\\RACache\\Data\\gamehashlibrary.txt");
inline constexpr auto RA_GAME_LIST_FILENAME	         = TEXT(".\\RACache\\Data\\gametitles.txt");
inline constexpr auto RA_MY_PROGRESS_FILENAME        = TEXT(".\\RACache\\Data\\myprogress.txt");
inline constexpr auto RA_MY_GAME_LIBRARY_FILENAME    = TEXT(".\\RACache\\Data\\mygamelibrary.txt");

inline constexpr auto RA_OVERLAY_BG_FILENAME         = TEXT(".\\Overlay\\overlayBG.png");
inline constexpr auto RA_NEWS_FILENAME               = TEXT(".\\RACache\\Data\\ra_news.txt");
inline constexpr auto RA_TITLES_FILENAME             = TEXT(".\\RACache\\Data\\gametitles.txt");
inline constexpr auto RA_LOG_FILENAME                = TEXT(".\\RACache\\Data\\RALog.txt");

inline constexpr auto RA_HOST_URL                    = TEXT("retroachievements.org");
inline constexpr auto RA_HOST_IMG_URL                = TEXT("i.retroachievements.org");


template<typename Array, class = std::enable_if_t<std::is_array_v<Array>>>
inline constexpr auto SIZEOF_ARRAY(const Array& ar) noexcept->decltype(sizeof(ar) / sizeof(ar[0]))
{
    return { sizeof(ar) / sizeof(ar[0]) };
}

template<typename Pointer, class = std::enable_if_t<std::is_pointer_v<Pointer>>>
inline constexpr auto SAFE_DELETE(Pointer x) noexcept
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

typedef std::vector<BYTE> DataStream;
typedef unsigned long ByteAddress;

typedef unsigned int AchievementID;
typedef unsigned int LeaderboardID;
typedef unsigned int GameID;

char* DataStreamAsString(DataStream& stream);

extern void RADebugLogNoFormat(const char* data);
extern void RADebugLog(const char* sFormat, ...);
extern BOOL DirectoryExists(const char* sPath);

const int SERVER_PING_DURATION = 2 * 60;
//};
//using namespace RA;


inline constexpr auto& RA_LOG = RADebugLog;

#ifdef _DEBUG
#undef ASSERT
inline auto ASSERT(bool x) noexcept->decltype(assert(x)) { assert(x); }
#else
#undef ASSERT
inline auto ASSERT(_UNUSED bool x) noexcept->void {}
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

typedef std::basic_string<TCHAR> tstring;


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


NativeString NativeStr(NonNativeCString str) noexcept;
NativeString NativeStr(const NonNativeString& str) noexcept;
NativeString NativeStr(NonNativeString&& str) noexcept;
NativeString NativeStr(const NativeString& str) noexcept;
NativeString NativeStr(NativeCString str) noexcept;



#endif // !RA_DEFS_H
