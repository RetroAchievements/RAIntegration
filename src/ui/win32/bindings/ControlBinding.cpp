#include "ControlBinding.hh"

#include "data\context\EmulatorContext.hh"

#include "services\ServiceLocator.hh"
#include "services\IClock.hh"

#include "ui\win32\Desktop.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

ControlBinding::~ControlBinding() noexcept
{
    DisableBinding();

    if (m_pOriginalWndProc)
    {
        SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, (LONG_PTR)m_pOriginalWndProc);
        SetWindowLongPtr(m_hWnd, GWLP_USERDATA, 0);
    }
}

void ControlBinding::ForceRepaint(HWND hWnd)
{
    InvalidateRect(hWnd, nullptr, FALSE);

    // When using SDL, the Windows message queue is never empty (there's a flood of WM_PAINT messages for the
    // SDL window). InvalidateRect only generates a WM_PAINT when the message queue is empty, so we have to
    // explicitly generate (and dispatch) a WM_PAINT message by calling UpdateWindow.
    switch (ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>().GetEmulatorId())
    {
        case RA_Libretro:
        case RA_Oricutron:
            UpdateWindow(hWnd);
            break;
    }
}

_NODISCARD static INT_PTR CALLBACK ControlBindingWndProc(HWND hControl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    ControlBinding* pBinding{};
    GSL_SUPPRESS_TYPE1 pBinding = reinterpret_cast<ControlBinding*>(GetWindowLongPtr(hControl, GWLP_USERDATA));

    if (pBinding != nullptr)
        return pBinding->WndProc(hControl, uMsg, wParam, lParam);

    return 0;
}

void ControlBinding::SubclassWndProc() noexcept
{
    if (m_pOriginalWndProc != nullptr)
        return;

    GSL_SUPPRESS_TYPE1 SetWindowLongPtr(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    GSL_SUPPRESS_TYPE1 m_pOriginalWndProc = reinterpret_cast<WNDPROC>(
        SetWindowLongPtr(m_hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&ControlBindingWndProc)));
}

_Use_decl_annotations_
INT_PTR CALLBACK ControlBinding::WndProc(HWND hControl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return CallWindowProc(m_pOriginalWndProc, hControl, uMsg, wParam, lParam);
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
