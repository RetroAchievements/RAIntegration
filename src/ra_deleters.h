#ifndef RA_DELETERS_H
#define RA_DELETERS_H

// we need _NODISCARD (instead of straight up [[nodiscard]]) to get rid of browsing errors. it's defined in yvals_core.h

// No need to specify the deleter as it's already taken care of
// Specializations of std::default_delete can work but have to be in an alias or user-defined type
// The void* and the ones that need enums can't be specialized

// Deletion results are marked [[maybe_unused]] because assert does nothing in release mode

namespace std {

template<> struct _NODISCARD default_delete<FILE>
{
    void operator()(_In_ FILE* const __restrict stream) const noexcept
    {
        [[maybe_unused]] const auto result = std::fclose(stream);
        assert(result != EOF);
    }
};

template<> struct _NODISCARD default_delete<HMENU__>
{
    void operator()(_In_ HMENU__* const __restrict hMenu) const noexcept
    {
        [[maybe_unused]] const auto result = ::DestroyMenu(hMenu);
        assert(result != 0);
    }
};

// can also be used for HMODULE
template<> struct _NODISCARD default_delete<HINSTANCE__>
{
    void operator()(_In_ HINSTANCE__* const  __restrict hInst) const noexcept
    {
        [[maybe_unused]] const auto result = ::FreeLibrary(hInst);
        assert(result != 0);
    }
};

template<> struct _NODISCARD default_delete<HACCEL__>
{
    void operator()(_In_ HACCEL__* const  __restrict hAccel) const noexcept
    {
        [[maybe_unused]] const auto result = ::DestroyAcceleratorTable(hAccel);
        assert(result != 0);
    }
};

// Do not use this with a Window DC
template<> struct _NODISCARD default_delete<HDC__>
{
    void operator()(_In_ HDC__* const __restrict hdc) const noexcept
    {
        [[maybe_unused]] const auto result = ::DeleteDC(hdc);
        assert(result != 0);
    }
};

} /* namespace std */

namespace ra {
/* Users shouldn't use these directly as it'll seem more complicated than it is, use the type aliases instead */
namespace detail {

// Handles have to be typed out the way they are or restrict won't work
// We can remove the const in front if setting to nullptr is desired

struct _NODISCARD InternetDeleter
{
    void operator()(_In_ void* const __restrict hInternet) const noexcept
    {
        [[maybe_unused]] const auto result = ::WinHttpCloseHandle(hInternet);
        assert(result != FALSE);
    }
};

// Can be used with HLOCAL as well since there is no local heap
struct _NODISCARD GlobalDeleter
{
    void operator()(_In_ void* const __restrict hMem) const noexcept
    {
        [[maybe_unused]] const auto result = ::GlobalFree(hMem);
        assert(result == nullptr);
    }
};

enum class _NODISCARD IconHandle
{
    Icon,
    Cursor
};

// HCURSOR's object has the same type as HICON since they're "polymorphic" (kind of)
template<IconHandle ih>
struct _NODISCARD IconDeleter
{
    void operator()(_In_ HICON__* const  __restrict hIconOrCursor) const noexcept
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
    ModelessDialog, // only modal dialog has a different deleter
    ModalDialog
};

template<WindowType wt>
struct _NODISCARD WindowDeleter
{
    void operator()(_In_ HWND__* const __restrict hWnd) const noexcept
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
                result = ::EndDialog(hWnd, IDCLOSE);
        }
        assert(result != 0);
    }
};

enum class _NODISCARD GdiType
{
    Bitmap,
    Brush,
    Font,
    Palette,
    Pen,
    Rgn,
    Obj
};

template<GdiType gdi_t>
struct _NODISCARD GdiDeleter
{
    void operator()(_In_ void* const __restrict ho) const noexcept
    {
        BOOL result = 0;
        switch (gdi_t)
        {
            case GdiType::Bitmap:
                result = DeleteBitmap(ho);
                break;
            case GdiType::Brush:
                result = DeleteBrush(ho);
                break;
            case GdiType::Font:
                result = DeleteFont(ho);
                break;
            case GdiType::Palette:
                result = DeletePalette(ho);
                break;
            case GdiType::Pen:
                result = DeletePen(ho);
                break;
            case GdiType::Rgn:
                result = DeleteRgn(ho);
                break;
            case GdiType::Obj:
                result = ::DeleteObject(ho);
        }
        assert(result != 0);
    }
};

enum class _NODISCARD NTKernelObjType /* We say NT because these might not exist in other Operating Systems (most do) */
{
    ChangeNotification,
    File,
    FindFile,
    Job,
    Mailslot,
    Mutex, // All threads must be closed before closing the mutex (while locked) or an access violation will occur
    Pipe, // anonymous pipes and named pipes are deallocated the same
    Process,
    Thread,
    TimerQueue,
    Object /* Default */
};

template<NTKernelObjType ntk_t>
struct _NODISCARD NTKernelDeleter
{
    void operator()(_In_ void* const __restrict hObject) const noexcept
    {
        BOOL result = 0;
        // many kernel objects use CloseHandle so a lot of cases will be skipped
        // Some kernel objects were not included because they needed more than one parameter when destroying
        switch (ntk_t)
        {
            case NTKernelObjType::ChangeNotification:
                result = ::FindCloseChangeNotification(hObject);
                break;
            case NTKernelObjType::FindFile:
                result = ::FindClose(hObject);
                break;
            case NTKernelObjType::TimerQueue:
                result = ::DeleteTimerQueue(hObject);
                break;
            case NTKernelObjType::File:     [[fallthrough]]; // all the way to Object
            case NTKernelObjType::Job:      [[fallthrough]];
            case NTKernelObjType::Mailslot: [[fallthrough]];
            case NTKernelObjType::Mutex:    [[fallthrough]];
            case NTKernelObjType::Pipe:     [[fallthrough]];
            case NTKernelObjType::Process:  [[fallthrough]];
            case NTKernelObjType::Thread:   [[fallthrough]];
            case NTKernelObjType::Object: /*tags above are meant for type aliasing but most aren't used right now */
                result = ::CloseHandle(hObject);
        }
        assert(result != 0);
    }
};

} /* namespace detail */

// For these types all you need to do is specify the creation function
using CFileH    = std::unique_ptr<FILE>; // there's an HFILE as well so we put CFile
using MenuH     = std::unique_ptr<HMENU__>;
using InstanceH = std::unique_ptr<HINSTANCE__>;
using AccelH    = std::unique_ptr<HACCEL__>;
using DCH       = std::unique_ptr<HDC__>; // Do not use with a Window DC
using InternetH = std::unique_ptr<void, detail::InternetDeleter>;
using GlobalH   = std::unique_ptr<void, detail::GlobalDeleter>;

// Only for use if HICON is initialized with CreateIconFromResourceEx (if called without the LR_SHARED flag),
// CreateIconIndirect, and CopyIcon. HCURSOR cannot be shared either
using IconH   = std::unique_ptr<HICON__, detail::IconDeleter<detail::IconHandle::Icon>>;
using CursorH = std::unique_ptr<HICON__, detail::IconDeleter<detail::IconHandle::Cursor>>;

using AppWndH      = std::unique_ptr<HWND__, detail::WindowDeleter<detail::WindowType::App>>;
using ModelessDlgH = std::unique_ptr<HWND__, detail::WindowDeleter<detail::WindowType::ModelessDialog>>;
using ModalDlgH    = std::unique_ptr<HWND__, detail::WindowDeleter<detail::WindowType::ModalDialog>>;

using BitmapH  = std::unique_ptr<HBITMAP__, detail::GdiDeleter<detail::GdiType::Bitmap>>;
using BrushH   = std::unique_ptr<HBRUSH__, detail::GdiDeleter<detail::GdiType::Brush>>;
using FontH    = std::unique_ptr<HFONT__, detail::GdiDeleter<detail::GdiType::Font>>;
using PaletteH = std::unique_ptr<HPALETTE__, detail::GdiDeleter<detail::GdiType::Palette>>;
using PenH     = std::unique_ptr<HPEN__, detail::GdiDeleter<detail::GdiType::Pen>>;
using RgnH     = std::unique_ptr<HRGN__, detail::GdiDeleter<detail::GdiType::Rgn>>;
using GdiObjH  = std::unique_ptr<void, detail::GdiDeleter<detail::GdiType::Obj>>;

using FileH     = std::unique_ptr<void, detail::NTKernelDeleter<detail::NTKernelObjType::File>>;
using FindFileH = std::unique_ptr<void, detail::NTKernelDeleter<detail::NTKernelObjType::FindFile>>;
using HandleH   = std::unique_ptr<void, detail::NTKernelDeleter<detail::NTKernelObjType::Object>>;

} /* namespace ra */

#endif /* !RA_DELETERS_H */
