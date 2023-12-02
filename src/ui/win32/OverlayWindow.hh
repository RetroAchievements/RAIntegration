#ifndef RA_UI_WIN32_OVERLAYWINDOW_H
#define RA_UI_WIN32_OVERLAYWINDOW_H
#pragma once

#include "ui\viewmodels\OverlayViewModel.hh"
#include "ui\win32\IDialogPresenter.hh"

namespace ra {
namespace ui {
namespace win32 {

class OverlayWindow
{
public:
    void CreateOverlayWindow(HWND hWnd);
    void DestroyOverlayWindow();

    void OnOverlayMoved() noexcept;

    void Render();

    DWORD OverlayWindowThread();
private:
    void CreateOverlayWindow();
    void UpdateOverlayPosition() noexcept;

    bool m_bErase = false;
    bool m_bOverlayMoved = false;

    HWND m_hWnd{};
    HWND m_hOverlayWnd{};
    HWINEVENTHOOK m_hEventHook{};
    HANDLE m_hOverlayWndThread{};
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_OVERLAYWINDOW_H
