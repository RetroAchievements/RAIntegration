#ifndef RA_DELETERS_H
#define RA_DELETERS_H

// specialization of std::default_delete doesn't work anymore
// No need to specify the deleter as it's already taken care of

namespace ra {
/* Users shouldn't use these directly as it'll seem more complicated than it is, use the type aliases instead */
namespace detail {

// Handles have to types the way they are or restrict won't work
// We can remove the const in front if setting to nullptr is desired

struct CFileDeleter { void operator()(_In_ FILE* const __restrict fp) const noexcept { std::fclose(fp); } };
struct MenuDeleter { void operator()(_In_ HMENU__* const __restrict hMenu) const noexcept { ::DestroyMenu(hMenu); } };

// can also be used for HMODULE
struct InstanceDeleter
{
    void operator()(_In_ HINSTANCE__* const  __restrict hInst) const noexcept
    {
        ::FreeLibrary(hInst);
    }
};

struct AcceleratorDeleter
{
    void operator()(_In_ HACCEL__* const  __restrict hAccel) noexcept
    {
        ::DestroyAcceleratorTable(hAccel);
    }
};

// Do not use this with a Window DC
struct DcDeleter { void operator()(_In_ HDC__* const __restrict hdc) const noexcept { ::DeleteDC(hdc); } };

struct InternetDeleter
{
    void operator()(_In_ void* const __restrict hInternet) const noexcept { ::WinHttpCloseHandle(hInternet); }
};

// Can be used with HLOCAL as well since there is no local heap
struct GlobalDeleter { void operator()(_In_ void* const __restrict hMem) const noexcept { ::GlobalFree(hMem); } };

enum class IconHandle
{
    Icon,
    Cursor
};

// HCURSOR's object has the same type as HICON since they're "polymorphic" (kind of)
template<IconHandle ih>
struct IconDeleter
{
    void operator()(_In_ HICON__* const  __restrict hIconOrCursor) const noexcept
    {
        switch (ih)
        {
            case IconHandle::Icon:
                ::DestroyIcon(hIconOrCursor);
                break;
            case IconHandle::Cursor:
                ::DestroyCursor(hIconOrCursor);
        }
    }
};

enum class WindowType
{
    App,
    Control,
    ModelessDialog, // only modal dialog has a different deleter
    ModalDialog
};

template<WindowType wt>
struct WindowDeleter
{
    void operator()(_In_ HWND__* const __restrict hWnd) const noexcept
    {
        switch (wt)
        {
            case WindowType::App:     [[fallthrough]]; /* fallthrough to Control */
            case WindowType::Control: [[fallthrough]]; /* fallthrough to ModelessDialog */
            case WindowType::ModelessDialog:
                ::DestroyWindow(hWnd);
                break;
            case WindowType::ModalDialog:
                ::EndDialog(hWnd, IDCLOSE);
        }
    }
};

enum class GdiType
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
struct GdiDeleter
{
    void operator()(_In_ void* const __restrict ho) const noexcept
    {
        switch (gdi_t)
        {
            case GdiType::Bitmap:
                DeleteBitmap(ho);
                break;
            case GdiType::Brush:
                DeleteBrush(ho);
                break;
            case GdiType::Font:
                DeleteFont(ho);
                break;
            case GdiType::Palette:
                DeletePalette(ho);
                break;
            case GdiType::Pen:
                DeletePen(ho);
                break;
            case GdiType::Rgn:
                DeleteRgn(ho);
                break;
            case GdiType::Obj:
                ::DeleteObject(ho);
        }
    }
};

enum class NTKernelObjType /* We say NT because these might not exist in other Operating Systems (most do) */
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
struct NTKernelDeleter
{
    void operator()(_In_ void* const __restrict hObject) const noexcept
    {
        // many kernel objects use CloseHandle so a lot of cases will be skipped
        // Some kernel objects were not included because they needed more than one parameter when destroying
        switch (ntk_t)
        {
            case NTKernelObjType::ChangeNotification:
                ::FindCloseChangeNotification(hObject);
                break;
            case NTKernelObjType::FindFile:
                ::FindClose(hObject);
                break;
            case NTKernelObjType::TimerQueue:
                ::DeleteTimerQueue(hObject);
                break;
            case NTKernelObjType::File:     [[fallthrough]]; // all the way to Object
            case NTKernelObjType::Job:      [[fallthrough]];
            case NTKernelObjType::Mailslot: [[fallthrough]];
            case NTKernelObjType::Mutex:    [[fallthrough]];
            case NTKernelObjType::Pipe:     [[fallthrough]];
            case NTKernelObjType::Process:  [[fallthrough]];
            case NTKernelObjType::Thread:   [[fallthrough]];
            case NTKernelObjType::Object: /*tags above are meant for type aliasing but most aren't used right now */
                ::CloseHandle(hObject);
        }
    }
};

} /* namespace detail */

// For these types all you need to do is specify the creation function
using CFileH    = std::unique_ptr<FILE, detail::CFileDeleter>; // there's an HFILE as well so we put CFile
using MenuH     = std::unique_ptr<HMENU__, detail::MenuDeleter>;
using InstanceH = std::unique_ptr<HINSTANCE__, detail::InstanceDeleter>;
using AccelH    = std::unique_ptr<HACCEL__, detail::AcceleratorDeleter>;
using DCH       = std::unique_ptr<HDC__, detail::DcDeleter>; // Do not use with a Window DC
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
