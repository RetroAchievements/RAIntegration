#include "OverlayWindow.hh"

#include "data\EmulatorContext.hh"

#include "services\IClock.hh"

#include "ui\OverlayTheme.hh"
#include "ui\drawing\gdi\GDISurface.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\win32\bindings\ControlBinding.hh"

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

        case WM_DESTROY:
            ra::services::ServiceLocator::GetMutable<OverlayWindow>().DestroyOverlayWindow();
            return 0;

        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
    }
}

static void CALLBACK HandleWinEvent(HWINEVENTHOOK, DWORD nEvent, HWND, LONG nObjectId, LONG, DWORD, DWORD)
{
    if (nEvent == EVENT_OBJECT_LOCATIONCHANGE && nObjectId == CHILDID_SELF)
        ra::services::ServiceLocator::GetMutable<OverlayWindow>().OnOverlayMoved();
}

static DWORD WINAPI OverlayWindowThreadStart(LPVOID)
{
    return ra::services::ServiceLocator::GetMutable<OverlayWindow>().OverlayWindowThread();
}

void OverlayWindow::CreateOverlayWindow(HWND hWnd)
{
    switch (ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().GetEmulatorId())
    {
        case EmulatorID::RA_Gens:
            return;
    }

    if (m_hOverlayWnd && IsWindow(m_hOverlayWnd))
    {
        // overlay window exists, just update the parent (if necessary)
        if (m_hWnd != hWnd)
        {
            m_hWnd = hWnd;
            SetParent(m_hOverlayWnd, hWnd);
            UpdateOverlayPosition();
        }
        return;
    }

    // overlay window does not exist, create it
    m_hWnd = hWnd;

    const auto hParentThread = GetWindowThreadProcessId(m_hWnd, nullptr);
    const auto hCurrentThread = GetCurrentThreadId();
    if (hParentThread == hCurrentThread)
    {
        // we're on the thread that owns the parent window, it's message pump will serve us
        CreateOverlayWindow();
    }
    else
    {
        // if the window was destroyed, it should have taken the thread with it
        assert(m_hOverlayWndThread == nullptr);

        // we're not on the thread that owns the parent window, so there may not even be a message pump.
        // create our own message pump thread and create/manage the overlay window there.
        m_hOverlayWndThread = CreateThread(nullptr, 0, OverlayWindowThreadStart, nullptr, 0, nullptr);
    }
}

DWORD OverlayWindow::OverlayWindowThread()
{
    CreateOverlayWindow();

    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);

        if (msg.message == WM_DESTROY)
        {
            m_hOverlayWnd = nullptr;
            break;
        }
    }

    m_hOverlayWndThread = nullptr;
    return gsl::narrow_cast<DWORD>(msg.wParam);
}

void OverlayWindow::CreateOverlayWindow()
{
    RECT rcMainWindowClientArea;
    GetClientRect(m_hWnd, &rcMainWindowClientArea);

    const COLORREF nTransparentColor = RGB(ra::ui::Color::Transparent.Channel.R, ra::ui::Color::Transparent.Channel.G, ra::ui::Color::Transparent.Channel.B);

    // Create the overlay window
    WNDCLASSEX wndEx;
    memset(&wndEx, 0, sizeof(wndEx));
    wndEx.cbSize = sizeof(wndEx);
    wndEx.lpszClassName = TEXT("RA_OVERLAY_WND_CLASS");

    static bool bClassRegistered = false;
    if (!bClassRegistered)
    {
        wndEx.lpfnWndProc = OverlayWndProc;
        wndEx.hbrBackground = CreateSolidBrush(nTransparentColor);
        wndEx.hInstance = GetModuleHandle(nullptr);
        if (!RegisterClassEx(&wndEx))
        {
            // if the class already exists, assume the DLL was shutdown and restarted. Unregister the old one and reregister the new one
            bool bSuccess = false;
            if (GetLastError() == ERROR_CLASS_ALREADY_EXISTS)
            {
                if (UnregisterClass(wndEx.lpszClassName, wndEx.hInstance))
                {
                    if (RegisterClassEx(&wndEx))
                        bSuccess = true;
                }
            }

            if (!bSuccess)
            {
                MessageBox(m_hWnd, TEXT("Failed to register overlay window class"), TEXT("Error"), MB_OK);
                return;
            }
        }

        bClassRegistered = true;
    }

    m_hOverlayWnd = CreateWindowEx(
        (WS_EX_NOACTIVATE | WS_EX_TRANSPARENT | WS_EX_LAYERED),
        wndEx.lpszClassName,
        "TransparentOverlayWindow",
        (WS_POPUP),
        CW_USEDEFAULT, CW_USEDEFAULT, rcMainWindowClientArea.right, rcMainWindowClientArea.bottom,
        m_hWnd, nullptr, wndEx.hInstance, nullptr);

    if (m_hOverlayWnd == nullptr)
    {
        MessageBox(m_hWnd, TEXT("Failed to create overlay window"), TEXT("Error"), MB_OK);
        return;
    }

    if (ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>().Transparent())
    {
        constexpr auto nAlpha = (255 * 90 / 100); // 90% opacity
        SetLayeredWindowAttributes(m_hOverlayWnd, nTransparentColor, nAlpha, LWA_ALPHA | LWA_COLORKEY);
    }
    else
    {
        SetLayeredWindowAttributes(m_hOverlayWnd, nTransparentColor, 0, LWA_COLORKEY);
    }

    m_bErase = true;
    ShowWindow(m_hOverlayWnd, SW_HIDE);

    // watch for location/size changes in the parent window
    m_hEventHook = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, nullptr, HandleWinEvent, GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
    UpdateOverlayPosition();

    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    pOverlayManager.SetRenderRequestHandler([this]() noexcept {
        ra::ui::win32::bindings::ControlBinding::ForceRepaint(m_hOverlayWnd);
    });

    pOverlayManager.SetShowRequestHandler([this]() noexcept {
        UpdateOverlayPosition();
        ::ShowWindow(m_hOverlayWnd, SW_SHOWNOACTIVATE);
    });

    pOverlayManager.SetHideRequestHandler([this]() noexcept {
        ::ShowWindow(m_hOverlayWnd, SW_HIDE);
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
        // clear out m_hOverlayWnd so we don't end up infinitely recursing
        HWND hOverlayWnd = m_hOverlayWnd;
        m_hOverlayWnd = nullptr;

        if (m_hOverlayWndThread)
        {
            // use PostMessage instead of calling DestroyWindow directly to ensure we kick out of the loop -
            // GetMessage does not get involved when using DestroyWindow directly.
            PostMessage(hOverlayWnd, WM_DESTROY, 0, 0);

            // Wait for the thread to finish
            WaitForSingleObject(m_hOverlayWndThread, 1000);
            m_hOverlayWndThread = nullptr;
        }
        else
        {
            // we didn't create a message pump, so we don't have to clean it up. just destroy the window.
            DestroyWindow(hOverlayWnd);
        }
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
    m_bOverlayMoved = false;
    MoveWindow(m_hOverlayWnd, rcWindowClientArea.left, rcWindowClientArea.top, nWidth, nHeight, FALSE);

    // if the size changed, we want to redraw everything on the next repaint
    if (rcOverlay.right - rcOverlay.left != nWidth || rcOverlay.bottom - rcOverlay.top != nHeight)
    {
        m_bErase = true;
        InvalidateRect(m_hOverlayWnd, nullptr, false);
    }
}

void OverlayWindow::OnOverlayMoved() noexcept
{
    if (!m_bOverlayMoved)
    {
        m_bOverlayMoved = true;
        InvalidateRect(m_hOverlayWnd, nullptr, false);
    }
}

void OverlayWindow::Render()
{
    auto& pOverlayManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>();
    if (!m_bErase && !pOverlayManager.NeedsRender())
        return;

    if (m_bOverlayMoved)
        UpdateOverlayPosition();

    RECT rcClientArea;
    ::GetClientRect(m_hWnd, &rcClientArea);

    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(m_hOverlayWnd, &ps);
    ra::ui::drawing::gdi::GDISurface pSurface(hDC, rcClientArea);

    if (m_bErase)
    {
        // redraw everything
        m_bErase = false;

        pSurface.FillRectangle(ps.rcPaint.left, ps.rcPaint.top,
            ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top, ra::ui::Color::Transparent);
        pOverlayManager.Render(pSurface, true);
    }
    else
    {
        // only redraw the stuff that changes
        pOverlayManager.Render(pSurface, false);
    }

    EndPaint(m_hOverlayWnd, &ps);
}

} // namespace win32
} // namespace ui
} // namespace ra
