#include "ControlBinding.hh"

#include "data\EmulatorContext.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

void ControlBinding::ForceRepaint(HWND hWnd)
{
    InvalidateRect(hWnd, nullptr, FALSE);

    // When using SDL, the Windows message queue is never empty (there's a flood of WM_PAINT messages for the
    // SDL window). InvalidateRect only generates a WM_PAINT when the message queue is empty, so we have to
    // explicitly generate (and dispatch) a WM_PAINT message by calling UpdateWindow.
    switch (ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().GetEmulatorId())
    {
        case RA_Libretro:
        case RA_Oricutron:
            UpdateWindow(hWnd);
            break;
    }
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
