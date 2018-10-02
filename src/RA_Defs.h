#ifndef RA_DEFS_H
#define RA_DEFS_H
#pragma once

#include "windows_nodefines.h"
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
#include "RA_StringUtils.h"

#if !RA_EXPORTS
#include <cassert> 
//	Version Information is integrated into tags

#else

#include "RA_Log.h"

#include "RA_Json.h"

using namespace std::string_literals; // u8 (UTF-8), u (UTF-16, char16_t), U (UTF-32, char32_t) prefixes and s (std::basic_string) suffix
#endif	// RA_EXPORTS

_CONSTANT_VAR RA_KEYS_DLL{ "RA_Keys.dll" };
_CONSTANT_VAR RA_PREFERENCES_FILENAME_PREFIX{ L"RAPrefs_" };
_CONSTANT_VAR RA_DIR_OVERLAY{ L"Overlay\\" };
_CONSTANT_VAR RA_DIR_BASE{ L"RACache\\" };
_CONSTANT_VAR RA_DIR_DATA{ L"RACache\\Data\\" };
_CONSTANT_VAR RA_DIR_BADGE{ L"RACache\\Badge\\" };
_CONSTANT_VAR RA_DIR_USERPIC{ L"RACache\\UserPic\\" };
_CONSTANT_VAR RA_DIR_BOOKMARKS{ L"RACache\\Bookmarks\\" };

_CONSTANT_VAR RA_GAME_HASH_FILENAME{ L"RACache\\Data\\gamehashlibrary.txt" };
_CONSTANT_VAR RA_GAME_LIST_FILENAME{ L"RACache\\Data\\gametitles.txt" };
_CONSTANT_VAR RA_MY_PROGRESS_FILENAME{ L"RACache\\Data\\myprogress.txt" };
_CONSTANT_VAR RA_MY_GAME_LIBRARY_FILENAME{ L"RACache\\Data\\mygamelibrary.txt" };

_CONSTANT_VAR RA_NEWS_FILENAME{ L"RACache\\Data\\ra_news.txt" };
_CONSTANT_VAR RA_TITLES_FILENAME{ L"RACache\\Data\\gametitles.txt" };
_CONSTANT_VAR RA_LOG_FILENAME{ L"RACache\\Data\\RALog.txt" };

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

extern BOOL DirectoryExists(const char* sPath);

const int SERVER_PING_DURATION = 2 * 60;
//};
//using namespace RA;

#ifdef _DEBUG
#ifndef RA_UTEST
#undef ASSERT
_CONSTANT_FN ASSERT(_In_ bool bExpr) noexcept { assert(bExpr); }
#else
#undef ASSERT
_CONSTANT_FN ASSERT(_UNUSED bool) noexcept {}
#endif
#else
#undef ASSERT
_CONSTANT_FN ASSERT(_UNUSED bool) noexcept {}
#endif

#ifndef UNUSED
template<typename T>
_CONSTANT_FN UNUSED(_UNUSED T) noexcept {}
#endif

#endif // !RA_DEFS_H
