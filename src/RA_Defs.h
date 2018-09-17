#ifndef RA_DEFS_H
#define RA_DEFS_H
#pragma once

// Windows stuff we DO need, they are commented out to show we need them, if for
// some reason you get a compiler error put the offending NO* define here
/*
    #define NOCOLOR
    #define NOCLIPBOARD - gave an error when put in the pch
    #define NOCTLMGR
    #define NODRAWTEXT
    #define NOGDI
    #define NOMB
    #define NOMENUS
    #define NOMSG
    #define NONLS
    #define NOOPENFILE
    #define NORASTEROPS
    #define NOSHOWWINDOW
    #define NOTEXTMETRIC
    #define NOUSER
    #define NOVIRTUALKEYCODES
    #define NOWINMESSAGES
    #define NOWINOFFSETS
    #define NOWINSTYLES
*/


// Windows stuff we don't need
#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS
#define NOSYSMETRICS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define OEMRESOURCE
#define NOATOM
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

struct IUnknown;
#include <Windows.h>
#include <WindowsX.h>

#pragma warning(push)
#pragma warning(disable : 4091) // 'typedef ': ignored on left of 'tagGPFIDL_FLAGS' when no variable is declared
#include <ShlObj.h>
#pragma warning(pop)

#include <tchar.h>

#include <map>
#include <array> // algorithm, iterator, tuple
#include <sstream> // string
#include <queue> // deque, vector, algorithm
#include "ra_utility.h"


#if !RA_EXPORTS
#include <cassert> 
//	Version Information is integrated into tags

#else

#include "RA_Log.h"

#include "RA_Json.h"

//	RA-Only
using namespace std::string_literals;
#endif	// RA_EXPORTS

#define RA_DIR_OVERLAY					L"Overlay\\"
#define RA_DIR_BASE						L"RACache\\"
#define RA_DIR_DATA						RA_DIR_BASE L"Data\\"
#define RA_DIR_BADGE					RA_DIR_BASE L"Badge\\"
#define RA_DIR_USERPIC					RA_DIR_BASE L"UserPic\\"
#define RA_DIR_BOOKMARKS				RA_DIR_BASE L"Bookmarks\\"

#define RA_GAME_HASH_FILENAME			RA_DIR_DATA L"gamehashlibrary.txt"
#define RA_GAME_LIST_FILENAME			RA_DIR_DATA L"gametitles.txt"
#define RA_MY_PROGRESS_FILENAME			RA_DIR_DATA L"myprogress.txt"
#define RA_MY_GAME_LIBRARY_FILENAME		RA_DIR_DATA L"mygamelibrary.txt"

#define RA_NEWS_FILENAME				RA_DIR_DATA L"ra_news.txt"
#define RA_TITLES_FILENAME				RA_DIR_DATA L"gametitles.txt"
#define RA_LOG_FILENAME					RA_DIR_DATA L"RALog.txt"


#define SIZEOF_ARRAY( ar )	( sizeof( ar ) / sizeof( ar[ 0 ] ) )
#define SAFE_DELETE( x )	{ if( x != nullptr ) { delete x; x = nullptr; } }



//namespace RA
//{
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

extern BOOL DirectoryExists(const char* sPath);

const int SERVER_PING_DURATION = 2 * 60;
//};
//using namespace RA;

#ifdef _DEBUG
#ifndef RA_UTEST
#undef ASSERT
#define ASSERT( x ) assert( x )
#else
#undef ASSERT
#define ASSERT( x ) {}
#endif
#else
#undef ASSERT
#define ASSERT( x ) {}
#endif

#ifndef UNUSED
#define UNUSED( x ) ( x );
#endif

#include "RA_StringUtils.h"

namespace ra {
_NODISCARD std::string ByteAddressToString(_In_ ra::ByteAddress nAddr);
}

#endif // !RA_DEFS_H
