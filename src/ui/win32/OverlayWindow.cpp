#include "OverlayWindow.hh"

#include "services\IClock.hh"

#include "ui\drawing\gdi\GDISurface.hh"

#include "ui\viewmodels\OverlayManager.hh"

namespace ra {
namespace ui {
namespace win32 {

static LRESULT CALLBACK OverlayWndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_PAINT:
            ra::services::ServiceLocator::GetMutable<OverlayWindow>().Render();
            return 0;

        case WM_SIZE:
            return SIZE_RESTORED;

        case WM_NCHITTEST:
            return HTNOWHERE;

        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
    }
}

static void CALLBACK HandleWinEvent(HWINEVENTHOOK, DWORD nEvent, HWND, LONG nObjectId, LONG, DWORD, DWORD)
{
    if (nEvent == EVENT_OBJECT_LOCATIONCHANGE && nObjectId == CHILDID_SELF)
        ra::services::ServiceLocator::GetMutable<OverlayWindow>().UpdateOverlayPosition();
}

void OverlayWindow::CreateOverlayWindow(HWND hWnd)
{
    if (m_hOverlayWnd)
        return;

    m_hWnd = hWnd;

    RECT rcMainWindowClientArea;
    GetClientRect(m_hWnd, &rcMainWindowClientArea);

    const COLORREF nTransparentColor = RGB(ra::ui::Color::Transparent.Channel.R, ra::ui::Color::Transparent.Channel.G, ra::ui::Color::Transparent.Channel.B);
    constexpr int nAlpha = (255 * 90 / 100); // 90% transparency

    // Create the overlay window
    WNDCLASSA wndEx;
    memset(&wndEx, 0, sizeof(wndEx));
    wndEx.cbWndExtra = sizeof(wndEx);
    wndEx.lpszClassName = "RA_WND_CLASS";
    wndEx.lpfnWndProc = OverlayWndProc;
    wndEx.hbrBackground = CreateSolidBrush(nTransparentColor);
    GSL_SUPPRESS_TYPE1 wndEx.hInstance = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(m_hWnd, GWLP_HINSTANCE));
    RegisterClass(&wndEx);

    m_hOverlayWnd = CreateWindowEx(
        (WS_EX_NOACTIVATE | WS_EX_TRANSPARENT | WS_EX_LAYERED),
        wndEx.lpszClassName,
        "TransparentOverlayWindow",
        (WS_POPUP),
        CW_USEDEFAULT, CW_USEDEFAULT, rcMainWindowClientArea.right, rcMainWindowClientArea.bottom,
        m_hWnd, nullptr, wndEx.hInstance, nullptr);

    SetLayeredWindowAttributes(m_hOverlayWnd, nTransparentColor, nAlpha, LWA_ALPHA | LWA_COLORKEY);

    SetParent(m_hWnd, m_hOverlayWnd);

    ShowWindow(m_hOverlayWnd, SW_SHOWNOACTIVATE);

    // watch for location/size changes in the parent window
    m_hEventHook = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, nullptr, HandleWinEvent, GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
    UpdateOverlayPosition();

    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    pOverlayManager.SetRenderRequestHandler([this]() noexcept {
        InvalidateRect(m_hOverlayWnd, nullptr, FALSE);
    });
}

void OverlayWindow::DestroyOverlayWindow() noexcept
{
    if (m_hEventHook)
    {
        UnhookWinEvent(m_hEventHook);
        m_hEventHook = nullptr;
    }

    if (m_hOverlayWnd)
    {
        DestroyWindow(m_hOverlayWnd);
        m_hOverlayWnd = nullptr;
    }

    m_hWnd = nullptr;
}

void OverlayWindow::UpdateOverlayPosition() noexcept
{
    RECT rcWindowClientArea, rcOverlay;
    GetClientRect(m_hOverlayWnd, &rcOverlay);

    GetClientRect(m_hWnd, &rcWindowClientArea);
    const int nWidth = rcWindowClientArea.right - rcWindowClientArea.left;
    const int nHeight = rcWindowClientArea.bottom - rcWindowClientArea.top;

    // get the screen position of the client rect
    GSL_SUPPRESS_TYPE1 ClientToScreen(m_hWnd, reinterpret_cast<POINT*>(&rcWindowClientArea.left));
    rcWindowClientArea.right = rcWindowClientArea.left + nWidth;
    rcWindowClientArea.bottom = rcWindowClientArea.top + nHeight;

    // move the layered window over the client window
    MoveWindow(m_hOverlayWnd, rcWindowClientArea.left, rcWindowClientArea.top, nWidth, nHeight, FALSE);

    if (rcOverlay.right - rcOverlay.left != nWidth || rcOverlay.bottom - rcOverlay.top != nHeight)
    {
        m_bErase = true;
        InvalidateRect(m_hOverlayWnd, nullptr, false);
    }
}

void OverlayWindow::Render()
{
    RECT rcClientArea;
    ::GetClientRect(m_hWnd, &rcClientArea);

    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(m_hOverlayWnd, &ps);
    ra::ui::drawing::gdi::GDISurface pSurface(hDC, rcClientArea);

    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();

    if (m_bErase)
    {
        m_bErase = false;

        pSurface.FillRectangle(ps.rcPaint.left, ps.rcPaint.top,
            ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top, ra::ui::Color::Transparent);
        pOverlayManager.Render(pSurface, true);
    }
    else
    {
        pOverlayManager.Render(pSurface, false);
    }

    EndPaint(m_hOverlayWnd, &ps);
}

} // namespace win32
} // namespace ui
} // namespace ra
