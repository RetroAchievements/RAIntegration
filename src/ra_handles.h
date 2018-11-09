#ifndef RA_DELETERS_H
#define RA_DELETERS_H
#pragma once

#include "ra_utility.h"

namespace std {

template<> struct _NODISCARD default_delete<FILE>
{
    void operator()(_Inout_ FILE *restrict stream) const noexcept
    {
        [[maybe_unused]] const auto result = std::fclose(stream);
        assert(result != EOF);
        stream = nullptr;
    }
};

#pragma region USER
template<> struct _NODISCARD default_delete<HMENU__>
{
    void operator()(_In_ HMENU__ *restrict hMenu) const noexcept
    {
        [[maybe_unused]] const auto result = ::DestroyMenu(hMenu);
        assert(result != 0);
    }
};

template<> struct _NODISCARD default_delete<HACCEL__>
{
    void operator()(_In_ HACCEL__ *restrict hAccel) const noexcept
    {
        [[maybe_unused]] const auto result = ::DestroyAcceleratorTable(hAccel);
        assert(result != 0);
    }
};

#ifndef NOWH
template<> struct _NODISCARD default_delete<HHOOK__>
{
    void operator()(_In_ HHOOK__ *restrict hhk) const noexcept
    {
        [[maybe_unused]] const auto result = ::UnhookWindowsHookEx(hhk);
        assert(result != 0);
    }
};
#endif /* !NOWH */
#pragma endregion

#pragma region GDI
// Do not use this with a Window DC (HDC retrieved using GetDC), use ReleaseDC instead
// If managing many different types of Device Context is desired, this can't be specialized.
template<> struct _NODISCARD default_delete<HDC__>
{
    void operator()(_In_ HDC__ *restrict hdc) const noexcept
    {
        [[maybe_unused]] const auto result = ::DeleteDC(hdc);
        assert(result != 0);
    }
};

// Meta-files are a bit complex memory-management wise

template<> struct _NODISCARD default_delete<HBITMAP__>
{
    void operator()(_In_ HBITMAP__ *restrict hbm) const noexcept
    {
        [[maybe_unused]] const auto result = DeleteBitmap(hbm);
        assert(result != 0);
    }
};

template<> struct _NODISCARD default_delete<HBRUSH__>
{
    void operator()(_In_ HBRUSH__ *restrict hbr) const noexcept
    {
        [[maybe_unused]] const auto result = DeleteBrush(hbr);
        assert(result != 0);
    }
};

template<> struct _NODISCARD default_delete<HFONT__>
{
    void operator()(_In_ HFONT__ *restrict hfont) const noexcept
    {
        [[maybe_unused]] const auto result = DeleteFont(hfont);
        assert(result != 0);
    }
};

template<> struct _NODISCARD default_delete<HPALETTE__>
{
    void operator()(_In_ HPALETTE__ *restrict hpal) const noexcept
    {
        [[maybe_unused]] const auto result = DeletePalette(hpal);
        assert(result != 0);
    }
};

template<> struct _NODISCARD default_delete<HPEN__>
{
    void operator()(_In_ HPEN__ *restrict hpen) const noexcept
    {
        [[maybe_unused]] const auto result = DeletePen(hpen);
        assert(result != 0);
    }
};

template<> struct _NODISCARD default_delete<HRGN__>
{
    void operator()(_In_ HRGN__ *restrict hrgn) const noexcept
    {
        [[maybe_unused]] const auto result = DeleteRgn(hrgn);
        assert(result != 0);
    }
};
#pragma endregion

#pragma region COM_OLE
template<> struct _NODISCARD default_delete<ITEMIDLIST>
{
    // There's no real way to check if the pointer is actually invalid (it should be) so an extra annotation was used
    void operator()(_In_ _Post_ptr_invalid_ ITEMIDLIST *restrict pv) const noexcept
    {
        ::CoTaskMemFree(pv);
    }
};
#pragma endregion

#pragma region NT
// can also be used for HMODULE
template<> struct _NODISCARD default_delete<HINSTANCE__>
{
    void operator()(_In_ HINSTANCE__ *restrict hInst) const noexcept
    {
        [[maybe_unused]] const auto result = ::FreeLibrary(hInst);
        assert(result != 0);
    }
};

template<> struct _NODISCARD default_delete<CRITICAL_SECTION>
{
    void operator()(_Inout_ CRITICAL_SECTION *restrict lpCriticalSection) const noexcept
    {
        ::DeleteCriticalSection(lpCriticalSection);
        lpCriticalSection = nullptr;
    }
};

#if _WIN32_WINNT >= 0x0602 /* Windows 8+ */
template<> struct _NODISCARD default_delete<SYNCHRONIZATION_BARRIER>
{
    void operator()(_Inout_ SYNCHRONIZATION_BARRIER *restrict lpBarrier) const noexcept
    {
        ::DeleteSynchronizationBarrier(lpBarrier);
        lpBarrier = nullptr;
    }
};
#endif /* _WIN32_WINNT >= 0x0602 */
#pragma endregion

} /* namespace std */

namespace ra {
/* Users shouldn't use these directly as it'll seem more complicated than it is, use the type aliases instead */
namespace detail {

enum class _NODISCARD IconHandle
{
    Icon,
    Cursor
};

// HCURSOR's object has the same type as HICON since they're "polymorphic" (kind of)
template<IconHandle ih>
struct _NODISCARD IconDeleter
{
    void operator()(_In_ HICON__ *restrict hIconOrCursor) const noexcept
    {
        BOOL result = 0;
        switch (ih)
        {
            case IconHandle::Icon:
                result = ::DestroyIcon(hIconOrCursor);
                break;
            case IconHandle::Cursor:
                result = ::DestroyCursor(hIconOrCursor);
        }
        assert(result != 0);
    }
};

enum class _NODISCARD WindowType
{
    App,
    Control,
    ModelessDialog,
    ModalDialog // only modal dialog has a different deleter
};

template<WindowType wt>
struct _NODISCARD WindowDeleter
{
    void operator()(_In_ HWND__ *restrict hWnd) const noexcept
    {
        BOOL result = 0;
        switch (wt)
        {
            case WindowType::App:     [[fallthrough]]; /* fallthrough to Control */
            case WindowType::Control: [[fallthrough]]; /* fallthrough to ModelessDialog */
            case WindowType::ModelessDialog:
                result = ::DestroyWindow(hWnd);
                break;
            case WindowType::ModalDialog:
                result = ::EndDialog(hWnd, IDCLOSE); /*nResult is the same result from DialogBox but it doesn't matter */
        }
        assert(result != 0);
    }
};

struct _NODISCARD InternetDeleter
{
    void operator()(_In_ void *restrict hInternet) const noexcept
    {
        [[maybe_unused]] const auto result = ::WinHttpCloseHandle(hInternet);
        assert(result != 0);
    }
};

struct _NODISCARD GlobalDeleter
{
    void operator()(_In_ void *restrict hMem) const noexcept
    {
        [[maybe_unused]] const auto result = ::GlobalFree(hMem);
        assert(result != nullptr);
    }
};

struct _NODISCARD LocalDeleter
{
    void operator()(_In_ void *restrict hMem) const noexcept
    {
        [[maybe_unused]] const auto result = ::LocalFree(hMem);
        assert(result != nullptr);
    }
};

/*
    We say NT because these might not exist in other Operating Systems (most do)
    A lot of this stuff exists in standard C/C++ but are here in case they are used (some are).
*/
enum class _NODISCARD NTKernelObjType
{
    AccessToken,
    ChangeNotification,
    Event,
    EventLog,
    File,
    FindFile,
    Heap,
    Job,
    Mailslot,
    Mutex,
    Pipe,
    Process,
    Semaphore,
    Thread,
    Timer, /* not the same as a timer for windows, this is mainly used for sync timeouts (WaitForSingleObject)*/
    TimerQueue,
    Object /* Default */
};

template<NTKernelObjType ntk_t>
struct _NODISCARD NTKernelDeleter
{
    void operator()(_In_ void *restrict hObject) const noexcept
    {
        BOOL result = 0;
        // many kernel objects use CloseHandle so a lot of cases will be skipped
        // Some kernel objects were not included because they needed more than one parameter when destroying
        switch (ntk_t)
        {
            case NTKernelObjType::ChangeNotification:
                result = ::FindCloseChangeNotification(hObject);
                break;
            case NTKernelObjType::EventLog:
                result = ::CloseEventLog(hObject);
                break;
            case NTKernelObjType::FindFile:
                result = ::FindClose(hObject);
                break;
            case NTKernelObjType::Heap:
                result = ::HeapDestroy(hObject);
                break;
            case NTKernelObjType::TimerQueue:
                result = ::DeleteTimerQueue(hObject);
                break;
            case NTKernelObjType::AccessToken: [[fallthrough]]; // all the way to Object
            case NTKernelObjType::File:        [[fallthrough]];
            case NTKernelObjType::Job:         [[fallthrough]];
            case NTKernelObjType::Mailslot:    [[fallthrough]];
            case NTKernelObjType::Mutex:       [[fallthrough]];
            case NTKernelObjType::Pipe:        [[fallthrough]];
            case NTKernelObjType::Process:     [[fallthrough]];
            case NTKernelObjType::Semaphore:   [[fallthrough]];
            case NTKernelObjType::Thread:      [[fallthrough]];
            case NTKernelObjType::Timer:       [[fallthrough]];
            case NTKernelObjType::Object: /*tags above are meant for type aliasing but most aren't used right now */
                result = ::CloseHandle(hObject);
        }
        assert(result != 0);
    }
};

} /* namespace detail */

// For these types all you need to do is specify the creation function

// The deleters for this group are std::default_delete specializations
// Valid creation function will be in the front as a comment

// The ones with specialized std::default_delete should work with std::make_unique, but currently don't in MSVC
// Example: CFileH file{std::fopen(filename, mode)};
using CFileH      = std::unique_ptr<FILE>; // std::fopen, _wfopen, ra::fopen_s (fopen_s and _wfopen_s will not work)
using ItemIdListH = std::unique_ptr<ITEMIDLIST>; // SHBrowseForFolder 

#pragma region USER
using MenuH  = std::unique_ptr<HMENU__>;  // CreateMenu, CreatePopupMenu, LoadMenu, LoadMenuIndirect
using AccelH = std::unique_ptr<HACCEL__>; // CreateAcceleratorTable

using IconH = std::unique_ptr<
    HICON__,
    detail::IconDeleter<detail::IconHandle::Icon>
>; // CreateIconIndirect, LoadIcon, LoadImage

using CursorH = std::unique_ptr<
    HICON__,
    detail::IconDeleter<detail::IconHandle::Cursor>
>; // CreateCursor, LoadCursor, LoadImage

#ifndef NOWH
using DefeWindowPosH = std::unique_ptr<HDWP__>; // BeginDeferWindowPos
#endif /* !NOWH */

using AppWndH = std::unique_ptr<
    HWND__,
    detail::WindowDeleter<detail::WindowType::App>
>; // CreateWindow, CreateWindowEx, CreateMDIWindow

using WndCtrlH = std::unique_ptr<
    HWND__,
    detail::WindowDeleter<detail::WindowType::Control>
>; // CreateWindow, CreateWindowEx

using ModelessDlgH = std::unique_ptr<
    HWND__,
    detail::WindowDeleter<detail::WindowType::ModelessDialog>
>; // CreateDialogParam, CreateDialogIndirectParam

using ModalDlgH = std::unique_ptr<
    HWND__,
    detail::WindowDeleter<detail::WindowType::ModalDialog>
>; // DialogBoxParam, DialogBoxIndirectParam

#ifndef NODEFERWINDOWPOS
using DeferWindowPosH = std::unique_ptr<HDWP__>; // BeginDeferWindowPos
#endif /* !NODEFERWINDOWPOS */
#pragma endregion

#pragma region GDI
// Note that none of the shared functions are in the lists (GetStock*, etc.)
using BitmapH = std::unique_ptr<HBITMAP__>; // CreateBitmap, CreateBitmapIndirect, CreateCompatibleBitmap,
                                            // CreateDIBitmap, CreateDIBSection, CreateDiscardableBitmap

using BrushH = std::unique_ptr<HBRUSH__>; // CreateBrushIndirect, CreateDIBPatternBrush, CreateDIBPatternBrushPt,
                                          // CreateHatchBrush, CreatePatternBrush, CreateSolidBrush

using DCH      = std::unique_ptr<HDC__>;      // CreateDC, CreateCompatibleDC (not GetDC!)
using FontH    = std::unique_ptr<HFONT__>;    // CreateFont, CreateFontIndirect
using PaletteH = std::unique_ptr<HPALETTE__>; // CreatePalette
using PenH     = std::unique_ptr<HPEN__>;     // CreatePen, CreatePenIndirect, ExtCreatePen

using RgnH = std::unique_ptr<HRGN__>; // CombineRgn, CreateEllipticRgn, CreateEllipticRgnIndirect, CreatePolygonRgn,
                                      // CreatePolyPolygonRgn, CreateRectRgn, CreateRectRgnIndirect, CreateRoundRectRgn,
                                      // ExtCreateRegion, PathToRegion
#pragma endregion

#pragma region NT Kernel (ntoskernel, http, etc.)
using InstanceH = std::unique_ptr<HINSTANCE__>; // LoadLibrary, GetModuleHandle
using InternetH = std::unique_ptr<void, detail::InternetDeleter>; // WinHttpConnect, WinHttpOpen, WinHttpOpenRequest,
using GlobalH   = std::unique_ptr<void, detail::GlobalDeleter>;   // GlobalAlloc, GlobalReAlloc
using LocalH    = std::unique_ptr<void, detail::LocalDeleter>;    // LocalAlloc, LocalReAlloc

// We really shouldn't have to use HANDLE so only some are specified
using FileH     = std::unique_ptr<void, detail::NTKernelDeleter<detail::NTKernelObjType::File>>;     // CreateFile
using FindFileH = std::unique_ptr<void, detail::NTKernelDeleter<detail::NTKernelObjType::FindFile>>; // FindFirstFile
using HandleH   = std::unique_ptr<void, detail::NTKernelDeleter<detail::NTKernelObjType::Object>>;   // Various  
#pragma endregion

// Non-member Creation Procs (only for the ones currently used)
// Everything here is assumed to be unique, so pointers are restrict qualified
#pragma region Resource Creation Procedures
template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>>
_NODISCARD inline auto make_cfile(_In_z_ const CharT *restrict filename,
                                  _In_z_ const CharT *restrict open_mode)
{
    return CFileH{ ra::fopen_s(filename, open_mode) };
}

_NODISCARD inline auto make_itemidlist(_In_ BROWSEINFO *restrict lpbi)
{
    return ItemIdListH{ ::SHBrowseForFolder(lpbi) };
}

// These regions are to get to stuff faster by collapsing
#pragma region USER
_NODISCARD inline auto make_menu() { return MenuH{ ::CreatePopupMenu() }; }

// Optional params are moved to the end and use non-deprecated SAL instead, as long as the

_NODISCARD inline auto make_control(_In_ DWORD dwExStyle,
                                    _In_ DWORD dwStyle,
                                    _In_ int X,
                                    _In_ int Y,
                                    _In_ int nWidth,
                                    _In_ int nHeight,
                                    _In_opt_z_ const TCHAR *restrict lpClassName  = _T(""),
                                    _In_opt_z_ const TCHAR *restrict lpWindowName = _T(""),
                                    _In_opt_ HWND__ *restrict hWndParent          = nullptr,
                                    _In_opt_ HMENU__ *restrict hMenu              = nullptr,
                                    _In_opt_ HINSTANCE__ *restrict hInstance      = nullptr,
                                    _In_opt_ void *restrict lpParam               = nullptr)
{
    return WndCtrlH{ ::CreateWindowEx(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent,
                                      hMenu, hInstance, lpParam) };
}

_NODISCARD inline auto make_modeless_dialog(_In_z_ const TCHAR *restrict lpTemplateName,
                                            _In_opt_ HINSTANCE__ *restrict hInstance = nullptr,
                                            _In_opt_ HWND__ *restrict hWndParent     = nullptr,
                                            _In_opt_ DLGPROC lpDialogFunc            = nullptr,
                                            _In_opt_ LPARAM dwInitParam              = 0L)
{
    return ModelessDlgH{ ::CreateDialogParam(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam) };
}

_NODISCARD inline auto make_modal_dialog(_In_z_ const TCHAR *restrict lpTemplateName,
                                         _In_opt_ HINSTANCE__ *restrict hInstance = nullptr,
                                         _In_opt_ HWND__ *restrict hWndParent     = nullptr,
                                         _In_opt_ DLGPROC lpDialogFunc            = nullptr,
                                         _In_opt_ LPARAM dwInitParam              = 0L)
{
    [[maybe_unused]] const auto result = ::DialogBoxParam(hInstance, lpTemplateName, hWndParent, lpDialogFunc,
                                                          dwInitParam);
    assert((result != 0) && (result != -1)); // hWndParent is invalid; Failure for any other reason
    const auto classAtom = ::GetClassLongPtr(hWndParent, GCW_ATOM); // modals have the same window class as parent by default
    assert(classAtom != 0);
    // Only non-hard-coded way we could figure out

    // TBD: Every window has a Window Class but not all of them have names, the class name
    //      speeds up the search but might cause overhead if it has no name.
    return ModalDlgH{ ::FindWindow(MAKEINTATOM(classAtom), lpTemplateName) };
}
#pragma endregion

#pragma region GDI
_NODISCARD inline auto make_bitmap(_In_ HDC__* const restrict hdc, _In_ int cx, _In_ int cy)
{
    return BitmapH{ ::CreateCompatibleBitmap(hdc, cx, cy) };
}

_NODISCARD inline auto make_bitmap(_In_ const BITMAPINFO *restrict pbmi,
                                   _In_ UINT usage,
                                   _In_ DWORD offset,
                                   _When_((pbmi->bmiHeader.biBitCount != 0),
                                          _Outptr_result_bytebuffer_(_Inexpressible_(GDI_DIBSIZE((pbmi->bmiHeader)))))
                                   _When_((pbmi->bmiHeader.biBitCount == 0),
                                          _Outptr_result_bytebuffer_((pbmi->bmiHeader).biSizeImage))
                                   void *restrict *restrict ppvBits,
                                   _In_opt_ HDC__ *restrict hdc = nullptr,
                                   _In_opt_ void *restrict hSection = nullptr)
{
    return BitmapH{ ::CreateDIBSection(hdc, pbmi, usage, ppvBits, hSection, offset) };
}

_NODISCARD inline auto make_brush(_In_ COLORREF color) { return BrushH{ ::CreateSolidBrush(color) }; }
_NODISCARD inline auto make_memorydc(_In_opt_ HDC__ *restrict hdc) { return DCH{ ::CreateCompatibleDC(hdc) }; }
_NODISCARD inline auto make_font(_In_ int cHeight, _In_ int cWidth,
                                 _In_ int cEscapement, _In_ int cOrientation,
                                 _In_ int cWeight, _In_ DWORD bItalic,
                                 _In_ DWORD bUnderline, _In_ DWORD bStrikeOut,
                                 _In_ DWORD iCharSet, _In_ DWORD iOutPrecision,
                                 _In_ DWORD iClipPrecision, _In_ DWORD iQuality,
                                 _In_ DWORD iPitchAndFamily, _In_opt_ const TCHAR *restrict pszFaceName = _T(""))
{
    return FontH{ ::CreateFont(cHeight, cWidth, cEscapement, cOrientation, cWeight, bItalic, bUnderline,
                               bStrikeOut, iCharSet, iOutPrecision, iClipPrecision, iQuality, iPitchAndFamily,
                               pszFaceName) };
}

_NODISCARD inline auto make_pen(_In_ int iStyle, _In_ int cWidth, _In_ COLORREF color)
{
    return PenH{ ::CreatePen(iStyle, cWidth, color) };
}
#pragma endregion

// There can be more but this is good enough for now
#pragma region KERNEL
_NODISCARD inline auto make_library(_In_z_ const TCHAR *restrict lpLibFileName)
{
    return InstanceH{ ::LoadLibrary(lpLibFileName) };
}

_NODISCARD inline auto make_module(_In_opt_z_ const TCHAR *restrict lpModuleName = _T(""))
{
    return InstanceH{ ::GetModuleHandle(lpModuleName) };
}

_NODISCARD inline auto make_global(_In_ UINT uFlags, _In_ SIZE_T dwBytes)
{
    return GlobalH{ ::GlobalAlloc(uFlags, dwBytes) };
}

// Uses the Windows API instead of the C-Runtime
_NODISCARD inline auto make_wfile(_In_z_   const TCHAR *restrict lpFileName,
                                  _In_     DWORD dwDesiredAccess,
                                  _In_     DWORD dwShareMode,
                                  _In_     DWORD dwCreationDisposition,
                                  _In_     DWORD dwFlagsAndAttributes,
                                  _In_opt_ SECURITY_ATTRIBUTES *restrict lpSecurityAttributes = nullptr,
                                  _In_opt_ void *restrict hTemplateFile = nullptr)
{
    return FileH{ ::CreateFile(lpFileName ,dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition,
                               dwFlagsAndAttributes, hTemplateFile) };
}

// Bit too complicated to make it a template, will make specializations instead
_NODISCARD inline auto make_findfile(_In_ const wchar_t *restrict lpFileName,
                                     _Out_ WIN32_FIND_DATAW *restrict lpFindFileData)
{
    return FindFileH{ ::FindFirstFileW(lpFileName , lpFindFileData) };
}

_NODISCARD inline auto make_findfile(_In_ const TCHAR *restrict lpFileName,
                                     _Out_ WIN32_FIND_DATA *restrict lpFindFileData)
{
    return FindFileH{ ::FindFirstFile(lpFileName, lpFindFileData) };
}
#pragma endregion
#pragma endregion

} /* namespace ra */

#endif /* !RA_DELETERS_H */
