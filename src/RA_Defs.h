#ifndef RA_DEFS_H
#define RA_DEFS_H
#pragma once

#include "RA_StringUtils.h"

#if !(RA_EXPORTS || RA_UTEST)
#include "windows_nodefines.h"
#include <Windows.h>
#include <WindowsX.h>

#pragma warning(push)
#pragma warning(disable : 4091)
#include <ShlObj.h>
#pragma warning(pop)

#include <tchar.h>

#include <cassert>

#include <array> // algorithm, iterator, tuple
#include <map>
#include <queue>   // deque, vector, algorithm
#include <sstream> // string

// Version Information is integrated into tags
#else

#include "RA_Json.h"
#include "RA_Log.h"

// RA-Only
using namespace std::string_literals;
_CONSTANT_VAR BUFSIZ            = 512U;        // ANSI buffer size
_CONSTANT_VAR BYTESTRING_BUFSIZ = BUFSIZ * 4U; // should be the same for MultiByte
_CONSTANT_VAR WIDESTRING_BUFSIZ = BUFSIZ * 8U;
_CONSTANT_VAR MAX_BUFSIZ        = BUFSIZ * 128U; // Try not to use this one
#endif // RA_EXPORTS

_CONSTANT_VAR RA_DIR_OVERLAY   = L"Overlay\\";
_CONSTANT_VAR RA_DIR_BASE      = L"RACache\\";
_CONSTANT_VAR RA_DIR_DATA      = L"RACache\\Data\\";
_CONSTANT_VAR RA_DIR_BADGE     = L"RACache\\Badge\\";
_CONSTANT_VAR RA_DIR_USERPIC   = L"RACache\\UserPic\\";
_CONSTANT_VAR RA_DIR_BOOKMARKS = L"RACache\\Bookmarks\\";

_CONSTANT_VAR RA_GAME_HASH_FILENAME       = L"RACache\\Data\\gamehashlibrary.txt";
_CONSTANT_VAR RA_GAME_LIST_FILENAME       = L"RACache\\Data\\gametitles.txt";
_CONSTANT_VAR RA_MY_PROGRESS_FILENAME     = L"RACache\\Data\\myprogress.txt";
_CONSTANT_VAR RA_MY_GAME_LIBRARY_FILENAME = L"RACache\\Data\\mygamelibrary.txt";

_CONSTANT_VAR RA_NEWS_FILENAME   = L"RACache\\Data\\ra_news.txt";
_CONSTANT_VAR RA_TITLES_FILENAME = L"RACache\\Data\\gametitles.txt";
_CONSTANT_VAR RA_LOG_FILENAME    = L"RACache\\Data\\RALog.txt";

// NB: This only works on C arrays, but we shouldn't use them; std::array is more of an array wrapper
template<typename Array, typename = std::enable_if_t<std::is_array_v<Array>>>
_NODISCARD _CONSTANT_FN SIZEOF_ARRAY(const Array& arr) noexcept
{
    return (sizeof(arr) / sizeof(*arr));
}

template<typename NullablePointer, typename = std::enable_if_t<ra::is_nullable_pointer_v<NullablePointer>>>
_CONSTANT_FN SAFE_DELETE(NullablePointer np, bool bIsDynArray = false) noexcept
{
    if (bIsDynArray)
        delete[] np;
    else
        delete np;
    np = nullptr;
}

class RARect : public RECT
{
public:
    RARect() {}
    RARect(LONG nX, LONG nY, LONG nW, LONG nH)
    {
        left   = nX;
        right  = nX + nW;
        top    = nY;
        bottom = nY + nH;
    }

public:
    inline int Width() const { return (right - left); }
    inline int Height() const { return (bottom - top); }
};

class ResizeContent
{
public:
    enum class AlignType
    {
        Right,
        Bottom,
        BottomRight
    };

public:
    HWND hwnd{};
    POINT pLT{};
    POINT pRB{};
    AlignType nAlignType{};
    int nDistanceX{};
    int nDistanceY{};
    bool bResize{};

    explicit ResizeContent(_In_ HWND contentHwnd, _In_ AlignType newAlignType, _In_ bool isResize) noexcept :
        hwnd{contentHwnd},
        nAlignType{newAlignType},
        bResize{isResize}
    {
        RARect rect;
        auto check = ::GetWindowRect(hwnd, &rect);
        assert(check != 0);

        pLT = {rect.left, rect.top};
        pRB = {rect.right, rect.bottom};

        HWND__* const restrict parentHwnd = ::GetParent(contentHwnd);

        check = ::ScreenToClient(parentHwnd, &pLT);
        assert(check != 0);
        check = ::ScreenToClient(parentHwnd, &pRB);
        assert(check != 0);

        check = ::GetWindowRect(parentHwnd, &rect);
        assert(check != 0);
        nDistanceX = rect.Width() - pLT.x;
        nDistanceY = rect.Height() - pLT.y;

        if (bResize)
        {
            nDistanceX -= (pRB.x - pLT.x);
            nDistanceY -= (pRB.y - pLT.y);
        }
    }

    void Resize(_In_ int width, _In_ int height) noexcept
    {
        int xPos = 0, yPos = 0;

        switch (nAlignType)
        {
            case AlignType::Right:
                xPos = width - nDistanceX - (bResize ? pLT.x : 0);
                yPos = bResize ? (pRB.y - pLT.x) : pLT.y;
                break;
            case AlignType::Bottom:
                xPos = bResize ? (pRB.x - pLT.x) : pLT.x;
                yPos = height - nDistanceY - (bResize ? pLT.y : 0);
                break;
            case AlignType::BottomRight:
                xPos = width - nDistanceX - (bResize ? pLT.x : 0);
                yPos = height - nDistanceY - (bResize ? pLT.y : 0);
                break;
            default:
                xPos = bResize ? (pRB.x - pLT.x) : pLT.x;
                yPos = bResize ? (pRB.y - pLT.x) : pLT.y;
        }

        if (BOOL check = 0; !bResize)
        {
            check = ::SetWindowPos(hwnd, nullptr, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            assert(check != 0);
        }
        else
        {
            check = ::SetWindowPos(hwnd, nullptr, 0, 0, xPos, yPos, SWP_NOMOVE | SWP_NOZORDER);
            assert(check != 0);
        }
    }
};

#ifdef _DEBUG
#ifndef RA_UTEST
#undef ASSERT
#define ASSERT(x) assert(x)
#else
#undef ASSERT
#define ASSERT(x) (void)0
#endif
#else
#undef ASSERT
#define ASSERT(x) (void)0
#endif

#ifndef UNUSED
#define UNUSED(x) (x);
#endif

namespace ra {
_NODISCARD std::string ByteAddressToString(_In_ ByteAddress nAddr, _In_ std::streamsize nPrecision = 6LL,
                                           _In_ bool bShowBase = false);
} // namespace ra

#if _MBCS
_CONSTANT_VAR RA_MAX_PATH{ra::to_unsigned(_MAX_PATH)}; // MultiByte max path
#elif _UNICODE
_CONSTANT_VAR RA_MAX_PATH{32767U}; // Unicode max path
#else
#error Unknown character set detected!
#endif /* _MBCS */

#endif // !RA_DEFS_H
