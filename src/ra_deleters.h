#ifndef RA_DELETERS_H
#define RA_DELETERS_H
#pragma once

// specialization of std::default_delete doesn't work anymore

namespace ra {

struct CFileDeleter { void operator()(_In_ FILE* const fp) const noexcept { std::fclose(fp); } };
struct MenuDeleter { void operator()(_In_ HMENU__* const hMenu) const noexcept { ::DestroyMenu(hMenu); } };

// For these types all you need to do is specify the creation function
using CFileH = std::unique_ptr<FILE, CFileDeleter>; // there's an HFILE as well so we put CFile
using MenuH = std::unique_ptr<HMENU__, MenuDeleter>;

} /* namespace ra */

namespace std {



template<>
struct default_delete<HINSTANCE__> { void operator()(_In_ HINSTANCE hInst) noexcept { ::FreeLibrary(hInst); } };

template<>
struct default_delete<HACCEL__> { void operator()(_In_ HACCEL hAccel) noexcept { ::DestroyAcceleratorTable(hAccel); } };



// Only for use if hIcon is initialized with CreateIconFromResourceEx (if called without the LR_SHARED flag),
// CreateIconIndirect, and CopyIcon. HCURSOR will need a custom class
template<>
struct default_delete<HICON__> { void operator()(_In_ HICON hIcon) noexcept { ::DestroyIcon(hIcon); } };


// If hWnd is a handle to a dialog, it must be created as modeless such as with CreateDialog
template<>
struct default_delete<HWND__> { void operator()(_In_ HWND hWnd) noexcept { ::DestroyWindow(hWnd); } };

// HDC will need a custom class

template<>
struct default_delete<HBITMAP__> { void operator()(_In_ HBITMAP hbm) noexcept { DeleteBitmap(hbm); } };

template<>
struct default_delete<HBRUSH__> { void operator()(_In_ HBRUSH hbr) noexcept { DeleteBrush(hbr); } };

template<>
struct default_delete<HFONT__> { void operator()(_In_ HFONT hfont) noexcept { DeleteFont(hfont); } };

template<>
struct default_delete<HPALETTE__> { void operator()(_In_ HPALETTE hpal) noexcept { DeletePalette(hpal); } };

template<>
struct default_delete<HRGN__> { void operator()(_In_ HRGN hrgn) noexcept { DeleteRgn(hrgn); } };

} /* namespace std */

#endif /* !RA_DELETERS_H */
