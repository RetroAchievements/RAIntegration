#include "MemoryViewerControlBinding.hh"

#include "ra_fwd.h"
#include "ra_utility.h"

#include "services\IClipboard.hh"
#include "ui/EditorTheme.hh"
#include "ui/drawing/gdi/GDISurface.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

constexpr int MEMVIEW_MARGIN = 4;
constexpr UINT WM_USER_INVALIDATE = WM_USER + 1;

INT_PTR CALLBACK MemoryViewerControlBinding::WndProc(HWND hControl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_PAINT:
            RenderMemViewer();
            return 0;

        case WM_ERASEBKGND:
            // we'll repaint the entire control area in WM_PAINT, so don't need to clear first.
            return TRUE;

        case WM_MOUSEWHEEL:
            if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
                ScrollUp();
            else if (GET_WHEEL_DELTA_WPARAM(wParam) < 0)
                ScrollDown();
            return FALSE;

        case WM_LBUTTONUP:
            OnClick({ GET_X_LPARAM(lParam) - MEMVIEW_MARGIN, GET_Y_LPARAM(lParam) - MEMVIEW_MARGIN });
            return FALSE;

        case WM_KEYDOWN:
            return (!OnKeyDown(static_cast<UINT>(LOWORD(wParam))));

        case WM_CHAR:
            return (!OnEditInput(static_cast<UINT>(LOWORD(wParam))));

        case WM_SETFOCUS:
            OnGotFocus();
            return FALSE;

        case WM_KILLFOCUS:
            OnLostFocus();
            return FALSE;

        case WM_GETDLGCODE:
            return DLGC_WANTCHARS | DLGC_WANTARROWS;

        case WM_USER_INVALIDATE:
            Invalidate();
            return FALSE;
    }

    return ControlBinding::WndProc(hControl, uMsg, wParam, lParam);
}

void MemoryViewerControlBinding::RegisterControlClass() noexcept
{
    static bool bClassRegistered = false;
    if (!bClassRegistered)
    {
        WNDCLASSEX wc{ sizeof(WNDCLASSEX) };
        wc.style = ra::to_unsigned(CS_PARENTDC | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS);
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = GetStockBrush(WHITE_BRUSH);
        wc.lpszClassName = TEXT("MemoryViewerControl");

        if (!RegisterClassEx(&wc))
        {
            // if the class already exists, assume the DLL was shutdown and restarted. Unregister the old one and reregister the new one
            bool bSuccess = false;
            if (GetLastError() == ERROR_CLASS_ALREADY_EXISTS)
            {
                if (UnregisterClass(wc.lpszClassName, wc.hInstance))
                {
                    if (RegisterClassEx(&wc))
                        bSuccess = true;
                }
            }

            if (!bSuccess)
            {
                MessageBox(nullptr, TEXT("Failed to register memory viewer control"), TEXT("Error"), MB_OK);
                return;
            }
        }

        bClassRegistered = true;
    }
}

void MemoryViewerControlBinding::SetHWND(DialogBase& pDialog, HWND hControl)
{
    ControlBinding::SetHWND(pDialog, hControl);

    SubclassWndProc();

    RECT rcRect;
    GetWindowRect(hControl, &rcRect);

    const ra::ui::Size szSize{ rcRect.right - rcRect.left, rcRect.bottom - rcRect.top };
    OnSizeChanged(szSize);
}

void MemoryViewerControlBinding::ScrollUp()
{
    if (m_pViewModel.IsAddressFixed())
        return;

    m_pViewModel.SetFirstAddress(m_pViewModel.GetFirstAddress() - 32);
    Invalidate();
}

void MemoryViewerControlBinding::ScrollDown()
{
    if (m_pViewModel.IsAddressFixed())
        return;

    m_pViewModel.SetFirstAddress(m_pViewModel.GetFirstAddress() + 32);
    Invalidate();
}

bool MemoryViewerControlBinding::OnKeyDown(UINT nChar)
{
    bool bHandled = false;

    // multiple properties may change while navigating, we'll do a single Invalidate after we're done
    m_bSuppressMemoryViewerInvalidate = true;

    if (!m_pViewModel.IsAddressFixed())
        bHandled = HandleNavigation(nChar);
    if (!bHandled)
        bHandled = HandleShortcut(nChar);

    m_bSuppressMemoryViewerInvalidate = false;

    if (bHandled)
        Invalidate();

    return bHandled;
}

bool MemoryViewerControlBinding::HandleNavigation(UINT nChar)
{
    const bool bShiftHeld = (GetKeyState(VK_SHIFT) < 0);
    const bool bControlHeld = (GetKeyState(VK_CONTROL) < 0);

    switch (nChar)
    {
        case VK_RIGHT:
            if (bShiftHeld || bControlHeld)
                m_pViewModel.AdvanceCursorWord();
            else
                m_pViewModel.AdvanceCursor();
            return true;

        case VK_LEFT:
            if (bShiftHeld || bControlHeld)
                m_pViewModel.RetreatCursorWord();
            else
                m_pViewModel.RetreatCursor();
            return true;

        case VK_DOWN:
            if (bControlHeld)
                m_pViewModel.SetFirstAddress(m_pViewModel.GetFirstAddress() + 0x10);
            else
                m_pViewModel.AdvanceCursorLine();
            return true;

        case VK_UP:
            if (bControlHeld)
                m_pViewModel.SetFirstAddress(m_pViewModel.GetFirstAddress() - 0x10);
            else
                m_pViewModel.RetreatCursorLine();
            return true;

        case VK_PRIOR: // Page up (!)
            m_pViewModel.RetreatCursorPage();
            return true;

        case VK_NEXT: // Page down (!)
            m_pViewModel.AdvanceCursorPage();
            return true;

        case VK_HOME:
            if (bControlHeld)
            {
                m_pViewModel.SetFirstAddress(0);
                m_pViewModel.SetAddress(0);
            }
            else
            {
                m_pViewModel.SetAddress(m_pViewModel.GetAddress() & ~0x0F);
            }
            return true;

        case VK_END:
            if (bControlHeld)
            {
                const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
                const auto nTotalBytes = gsl::narrow<ra::ByteAddress>(pEmulatorContext.TotalMemorySize());

                m_pViewModel.SetFirstAddress(nTotalBytes & ~0x0F);
                m_pViewModel.SetAddress(nTotalBytes - 1);
            }
            else
            {
                switch (m_pViewModel.GetSize())
                {
                    case MemSize::ThirtyTwoBit:
                        m_pViewModel.SetAddress((m_pViewModel.GetAddress() & ~0x0F) | 0x0C);
                        break;

                    case MemSize::SixteenBit:
                        m_pViewModel.SetAddress((m_pViewModel.GetAddress() & ~0x0F) | 0x0E);
                        break;

                    default:
                        m_pViewModel.SetAddress(m_pViewModel.GetAddress() | 0x0F);
                        break;
                }
            }
            return true;

        default:
            return false;
    }
}

bool MemoryViewerControlBinding::HandleShortcut(UINT nChar)
{
    const bool bShiftHeld = (GetKeyState(VK_SHIFT) < 0);
    const bool bControlHeld = (GetKeyState(VK_CONTROL) < 0);

    switch (nChar)
    {
        case 'C':
            if (bControlHeld)
            {
                OnCopy();
            }
            return true;

        case 'V':
            if (bControlHeld and !m_pViewModel.IsReadOnly())
            {
                return OnPaste(bShiftHeld);
            }
            return true;

        default:
            return false;
    }
}

bool MemoryViewerControlBinding::OnEditInput(UINT c)
{
    // multiple properties may change while typing, we'll do a single Invalidate after we're done
    m_bSuppressMemoryViewerInvalidate = true;
    const bool bResult = m_pViewModel.OnChar(gsl::narrow_cast<char>(c));
    m_bSuppressMemoryViewerInvalidate = false;

    if (bResult)
        Invalidate();

    return bResult;
}

void MemoryViewerControlBinding::OnClick(POINT point)
{
    if (m_pViewModel.IsAddressFixed())
        return;

    // multiple properties may change while typing, we'll do a single Invalidate after we're done
    m_bSuppressMemoryViewerInvalidate = true;
    m_pViewModel.OnClick(point.x, point.y);
    m_bSuppressMemoryViewerInvalidate = false;

    SetFocus(m_hWnd);

    Invalidate();
}

void MemoryViewerControlBinding::OnGotFocus()
{
    m_pViewModel.OnGotFocus();
}

void MemoryViewerControlBinding::OnLostFocus()
{
    m_pViewModel.OnLostFocus();
}

void MemoryViewerControlBinding::OnSizeChanged(const ra::ui::Size& pNewSize)
{
    m_pViewModel.OnResized(pNewSize.Width - MEMVIEW_MARGIN * 2, pNewSize.Height - MEMVIEW_MARGIN * 2);
}

void MemoryViewerControlBinding::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) noexcept
{
    if (args.Property == ra::ui::viewmodels::MemoryViewerViewModel::AddressProperty ||
        args.Property == ra::ui::viewmodels::MemoryViewerViewModel::SizeProperty)
    {
        // these properties affect the rendered image, immediately invalidate in case the emulator
        // is paused - in which case, the update from DoFrame will not occur
        PostMessage(m_hWnd, WM_USER_INVALIDATE, 0, 0);
    }
}

void MemoryViewerControlBinding::OnCopy()
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    const auto nValue = pEmulatorContext.ReadMemory(m_pViewModel.GetAddress(), m_pViewModel.GetSize());
    std::wstring sValue = ra::data::MemSizeFormat(nValue, m_pViewModel.GetSize(), MemFormat::Hex);

    ra::services::ServiceLocator::Get<ra::services::IClipboard>().SetText(ra::Widen(sValue));
}

bool MemoryViewerControlBinding::OnPaste(bool bShiftHeld)
{
    auto nAddress = m_pViewModel.GetAddress();
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    std::wstring sClipboardText = ra::services::ServiceLocator::Get<ra::services::IClipboard>().GetText();
    const auto nBytesForSize = ra::data::MemSizeBytes(m_pViewModel.GetSize());
    std::vector<std::uint32_t> vValues;
    std::wstring sError;

    if (sClipboardText.empty())
        return false;

    // Split the clipboard value into substrings matching the current size and check if they're valid hexadecimal values
    // If the clipboard text is smaller than a single write, it will be treated as a padded value:
    //    C => 0C, 000C, or 0000000C (depending on currently selected viewer size)
    // If shift is held, and the clipboard string is bigger than a single write, multiple writes will occur.
    // If the last write is not a complete chunk, it will be padded in the same way as described above:
    //    12345678C => 12 34 56 78 0C, 1234 5678 000C, or 12345678 0000000C
    for (size_t i = 0; i < sClipboardText.size(); i += nBytesForSize * 2)
    {
        std::wstring sSubString = sClipboardText.substr(i, nBytesForSize * 2);
        unsigned int nValue;

        if (!ra::ParseHex(sSubString, 0xFFFFFFFF, nValue, sError))
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Paste value failed", sError);
            return false;
        }

        // Single mode writes only the first value,
        if (!bShiftHeld)
        {
            pEmulatorContext.WriteMemory(nAddress, m_pViewModel.GetSize(), nValue);
            return true;
        }

        vValues.push_back(nValue);
    }

    // Multi mode (shift) writes every values
    for (auto nValue : vValues)
    {
        pEmulatorContext.WriteMemory(nAddress, m_pViewModel.GetSize(), nValue);
        if (!bShiftHeld)
            break;
        nAddress += nBytesForSize;
    }

    return true;
}

void MemoryViewerControlBinding::Invalidate()
{
    if (m_pViewModel.NeedsRedraw() && !m_bSuppressMemoryViewerInvalidate)
        ControlBinding::ForceRepaint(m_hWnd);
}

void MemoryViewerControlBinding::RenderMemViewer()
{
    m_pViewModel.UpdateRenderImage();

    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(m_hWnd, &ps);

    RECT rcClient;
    GetClientRect(m_hWnd, &rcClient);

    const auto& pRenderImage = m_pViewModel.GetRenderImage();
    const auto& pEditorTheme = ra::services::ServiceLocator::Get<ra::ui::EditorTheme>();
    HBRUSH hBackground = CreateSolidBrush(RGB(pEditorTheme.ColorBackground().Channel.R,
                                              pEditorTheme.ColorBackground().Channel.G,
                                              pEditorTheme.ColorBackground().Channel.B));

    // left margin
    RECT rcFill{ rcClient.left, rcClient.top, rcClient.left + MEMVIEW_MARGIN, rcClient.bottom - 1 };
    FillRect(hDC, &rcFill, hBackground);

    // right margin
    rcFill.left = rcClient.left + MEMVIEW_MARGIN + pRenderImage.GetWidth();
    rcFill.right = rcClient.right - 1;
    FillRect(hDC, &rcFill, hBackground);

    // top margin
    rcFill.left = rcClient.left;
    rcFill.bottom = rcClient.top + MEMVIEW_MARGIN;
    FillRect(hDC, &rcFill, hBackground);

    // bottom margin
    rcFill.top = rcClient.top + MEMVIEW_MARGIN + pRenderImage.GetHeight();
    rcFill.bottom = rcClient.bottom - 1;
    FillRect(hDC, &rcFill, hBackground);

    // frame
    FrameRect(hDC, &rcClient, GetSysColorBrush(COLOR_3DSHADOW));

    // content
    ra::ui::drawing::gdi::GDISurface pSurface(hDC, rcClient);
    pSurface.DrawSurface(MEMVIEW_MARGIN, MEMVIEW_MARGIN, pRenderImage);

    DeleteObject(hBackground);
    EndPaint(m_hWnd, &ps);
}

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra
