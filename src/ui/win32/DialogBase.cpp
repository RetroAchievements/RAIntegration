#include "DialogBase.hh"

#include "ui\win32\bindings\ControlBinding.hh"
#include "ui\win32\bindings\GridBinding.hh"

#include "RA_Core.h" // g_RAMainWnd, g_hThisDLLInst

#include "ra_utility.h"

namespace ra {
namespace ui {
namespace win32 {

_Use_decl_annotations_
DialogBase::DialogBase(ra::ui::WindowViewModelBase& vmWindow) noexcept :
    m_vmWindow{ vmWindow },
    m_bindWindow{ vmWindow }
{
}

DialogBase::~DialogBase() noexcept
{
    if (m_hWnd)
    {
        // remove the pointer to the instance from the HWND so the StaticDialogProc doesn't try to redirect
        // the WM_DESTROY message through the instance we're trying to destroy.
        ::SetWindowLongPtr(m_hWnd, DWLP_USER, 0L);

        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
}

_Use_decl_annotations_
_NODISCARD static INT_PTR CALLBACK StaticDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DialogBase* pDialog{};
    GSL_SUPPRESS_TYPE1 pDialog = reinterpret_cast<DialogBase*>(GetWindowLongPtr(hDlg, DWLP_USER));

    if (pDialog == nullptr)
    {
        if (uMsg != WM_INITDIALOG)
            return ::DefWindowProc(hDlg, uMsg, wParam, lParam);

        GSL_SUPPRESS_TYPE1 pDialog = reinterpret_cast<DialogBase*>(lParam);
        if (pDialog == nullptr)
            return ::DefWindowProc(hDlg, uMsg, wParam, lParam);

        ::SetWindowLongPtr(hDlg, DWLP_USER, lParam);
    }

    const INT_PTR result = pDialog->DialogProc(hDlg, uMsg, wParam, lParam);

    if (uMsg == WM_DESTROY)
        ::SetWindowLongPtr(hDlg, DWLP_USER, 0L);

    return result;
}

_Use_decl_annotations_ HWND DialogBase::CreateDialogWindow(const TCHAR* restrict sResourceId,
                                                           IDialogPresenter* const restrict pDialogPresenter) noexcept
{
    GSL_SUPPRESS_TYPE1 m_hWnd = ::CreateDialogParam(g_hThisDLLInst, sResourceId, g_RAMainWnd, StaticDialogProc, reinterpret_cast<LPARAM>(this));
    if (m_hWnd)
        m_pDialogPresenter = pDialogPresenter;

    return m_hWnd;
}

// CreateModalWindow uses DialogBox to run a modal loop. As such, we don't know the HWND without processing the
// associated WM_INITDIALOG message. Use a secondary WNDPROC to capture that so we can associate the DialogBase
// to the HWND's DWLP_USER attribute, then switch back to using the normal StaticDialogProc.
static DialogBase* s_pModalDialog = nullptr;

_Use_decl_annotations_
static INT_PTR CALLBACK StaticModalDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (s_pModalDialog == nullptr)
        return StaticDialogProc(hDlg, uMsg, wParam, lParam);

    const INT_PTR result = s_pModalDialog->DialogProc(hDlg, uMsg, wParam, lParam);

    if (uMsg == WM_INITDIALOG)
    {
        // Once we've seen the WM_INITDIALOG message, we've associated the HWND to the DialogBase and
        // can use the normal StaticDialogProc handler.
        SubclassDialog(hDlg, StaticDialogProc);

        GSL_SUPPRESS_TYPE1 SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(s_pModalDialog));
        s_pModalDialog = nullptr;
    }

    return result;
}

_Use_decl_annotations_ void DialogBase::CreateModalWindow(const TCHAR* restrict sResourceId,
                                                          IDialogPresenter* const restrict pDialogPresenter,
                                                          HWND hParentWnd) noexcept
{
    m_pDialogPresenter = pDialogPresenter;
    m_bModal = true;

    s_pModalDialog = this;
    DialogBox(g_hThisDLLInst, sResourceId, hParentWnd, &StaticModalDialogProc);
}

_Use_decl_annotations_
INT_PTR CALLBACK DialogBase::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            if (m_hWnd == nullptr)
            {
                m_hWnd = hDlg;

                // binding the window will resize it, make sure to capture sizes beforehand
                InitializeAnchors();

                m_bindWindow.SetHWND(m_hWnd);
            }

            return OnInitDialog();

        case WM_DESTROY:
            m_vmWindow.SetIsVisible(false);
            OnDestroy();
            return 0;

        case WM_SHOWWINDOW:
            if (wParam)
            {
                m_vmWindow.SetIsVisible(true);
                OnShown();
            }
            else
            {
                m_vmWindow.SetIsVisible(false);
            }
            return 0;

        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                case EN_SETFOCUS:
                {
                    ra::ui::win32::bindings::ControlBinding* pControlBinding;
                    GSL_SUPPRESS_TYPE1 pControlBinding = FindControlBinding(reinterpret_cast<HWND>(lParam));

                    if (pControlBinding)
                        pControlBinding->OnGotFocus();
                    return TRUE;
                }

                case EN_KILLFOCUS:
                {
                    ra::ui::win32::bindings::ControlBinding* pControlBinding;
                    GSL_SUPPRESS_TYPE1 pControlBinding = FindControlBinding(reinterpret_cast<HWND>(lParam));

                    if (pControlBinding)
                        pControlBinding->OnLostFocus();
                    return TRUE;
                }

                case CBN_SELCHANGE:
                case EN_CHANGE:
                {
                    ra::ui::win32::bindings::ControlBinding* pControlBinding;
                    GSL_SUPPRESS_TYPE1 pControlBinding = FindControlBinding(reinterpret_cast<HWND>(lParam));

                    if (pControlBinding)
                        pControlBinding->OnValueChanged();
                    return TRUE;
                }

                case 0:
                {
                    // a command triggered by the keyboard will not move focus to the associated button. ensure any
                    // lostfocus logic is applied for the currently focused control before executing the command.
                    auto* pControlBinding = FindControlBinding(GetFocus());
                    if (pControlBinding)
                        pControlBinding->OnLostFocus();

                    GSL_SUPPRESS_TYPE1 pControlBinding = FindControlBinding(reinterpret_cast<HWND>(lParam));
                    if (pControlBinding)
                        pControlBinding->OnCommand();
                    break;
                }
            }
            return OnCommand(LOWORD(wParam));

        case WM_NOTIFY:
        {
            LPNMHDR pnmHdr;
            GSL_SUPPRESS_TYPE1{ pnmHdr = reinterpret_cast<LPNMHDR>(lParam); }
            switch (pnmHdr->code)
            {
                case LVN_ITEMCHANGED:
                {
                    ra::ui::win32::bindings::GridBinding* pGridBinding;
                    GSL_SUPPRESS_TYPE1 pGridBinding = reinterpret_cast<ra::ui::win32::bindings::GridBinding*>(
                        FindControlBinding(pnmHdr->hwndFrom));

                    if (pGridBinding)
                    {
                        LPNMLISTVIEW pnmListView;
                        GSL_SUPPRESS_TYPE1{ pnmListView = reinterpret_cast<LPNMLISTVIEW>(pnmHdr); }
                        pGridBinding->OnLvnItemChanged(pnmListView);
                    }

                    return 0;
                }

                case LVN_COLUMNCLICK:
                {
                    ra::ui::win32::bindings::GridBinding* pGridBinding;
                    GSL_SUPPRESS_TYPE1 pGridBinding = reinterpret_cast<ra::ui::win32::bindings::GridBinding*>(
                        FindControlBinding(pnmHdr->hwndFrom));

                    if (pGridBinding)
                    {
                        LPNMLISTVIEW pnmListView;
                        GSL_SUPPRESS_TYPE1{ pnmListView = reinterpret_cast<LPNMLISTVIEW>(pnmHdr); }
                        pGridBinding->OnLvnColumnClick(pnmListView);
                    }

                    return 0;
                }

                case NM_CLICK:
                {
                    ra::ui::win32::bindings::GridBinding* pGridBinding;
                    GSL_SUPPRESS_TYPE1 pGridBinding = reinterpret_cast<ra::ui::win32::bindings::GridBinding*>(
                        FindControlBinding(pnmHdr->hwndFrom));

                    if (pGridBinding)
                    {
                        const NMITEMACTIVATE* pnmItemActivate;
                        GSL_SUPPRESS_TYPE1{ pnmItemActivate = reinterpret_cast<const NMITEMACTIVATE*>(lParam); }
                        pGridBinding->OnNmClick(pnmItemActivate);
                    }

                    return 0;
                }

                case NM_DBLCLK:
                {
                    ra::ui::win32::bindings::GridBinding* pGridBinding;
                    GSL_SUPPRESS_TYPE1 pGridBinding = reinterpret_cast<ra::ui::win32::bindings::GridBinding*>(
                        FindControlBinding(pnmHdr->hwndFrom));

                    if (pGridBinding)
                    {
                        const NMITEMACTIVATE* pnmItemActivate;
                        GSL_SUPPRESS_TYPE1{ pnmItemActivate = reinterpret_cast<const NMITEMACTIVATE*>(lParam); }
                        pGridBinding->OnNmDblClick(pnmItemActivate);
                    }

                    return 0;
                }

                case NM_SETFOCUS:
                {
                    ra::ui::win32::bindings::ControlBinding* pControlBinding;
                    GSL_SUPPRESS_TYPE1 pControlBinding = FindControlBinding(reinterpret_cast<HWND>(pnmHdr->hwndFrom));

                    if (pControlBinding)
                        pControlBinding->OnGotFocus();
                    return 0;
                }

                case NM_KILLFOCUS:
                {
                    ra::ui::win32::bindings::ControlBinding* pControlBinding;
                    GSL_SUPPRESS_TYPE1 pControlBinding = FindControlBinding(reinterpret_cast<HWND>(pnmHdr->hwndFrom));

                    if (pControlBinding)
                        pControlBinding->OnLostFocus();
                    return 0;
                }
            }
        }

        case WM_MOVE:
        {
            ra::ui::Position oPosition{ LOWORD(lParam), HIWORD(lParam) };
            OnMove(oPosition);
            return 0;
        }

        case WM_SIZE:
        {
            ra::ui::Size oSize{ LOWORD(lParam), HIWORD(lParam) };
            OnSize(oSize);
            return 0;
        }

        default:
            return 0;
    }
}

void DialogBase::OnDestroy()
{
    m_bindWindow.SetHWND(nullptr);
    m_hWnd = nullptr;

    auto* pClosableDialog = dynamic_cast<IClosableDialogPresenter*>(m_pDialogPresenter);
    m_pDialogPresenter = nullptr;

    // WARNING: the main reason a presenter wants to be notified of a dialog closing is to destroy the dialog
    // "this" may no longer be valid after this call, so don't use it!
    if (pClosableDialog)
        pClosableDialog->OnClosed();
}

_Use_decl_annotations_
BOOL DialogBase::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDOK:
            SetDialogResult(DialogResult::OK);
            return TRUE;

        case IDCLOSE:
        case IDCANCEL:
            SetDialogResult(DialogResult::Cancel);
            return TRUE;

        default:
            return FALSE;
    }
}

void DialogBase::SetDialogResult(DialogResult nResult)
{
    m_vmWindow.SetDialogResult(nResult);

    if (m_bModal)
        EndDialog(m_hWnd, 0); // DialogBox call in CreateModalWindow() ignores return value
    else
        DestroyWindow(m_hWnd);
}

_Use_decl_annotations_
void DialogBase::OnMove(const ra::ui::Position& oNewPosition)
{
    m_bindWindow.OnPositionChanged(oNewPosition);
}

_Use_decl_annotations_
void DialogBase::OnSize(const ra::ui::Size& oNewSize)
{
    UpdateAnchoredControls();

    m_bindWindow.OnSizeChanged(oNewSize);
}

void DialogBase::InitializeAnchors() noexcept
{
    if (m_vControlAnchors.empty())
        return;

    // client rect will always be (0,0)-(width,height)
    // but child window positions will always be screen-relative, so translate the client rect
    // to screen coordinates
    RECT rcWindow;
    ::GetClientRect(m_hWnd, &rcWindow);

    POINT pTopLeft = { rcWindow.left, rcWindow.top };
    ::ClientToScreen(m_hWnd, &pTopLeft);
    ::OffsetRect(&rcWindow, pTopLeft.x, pTopLeft.y);

    for (auto& info : m_vControlAnchors)
    {
        auto hControl = ::GetDlgItem(m_hWnd, info.nIDDlgItem);
        if (!hControl)
            continue;

        RECT rcControl;
        ::GetWindowRect(hControl, &rcControl);

        info.nMarginLeft = rcControl.left - rcWindow.left;
        info.nMarginTop = rcControl.top - rcWindow.top;
        info.nMarginRight = rcWindow.right - rcControl.right;
        info.nMarginBottom = rcWindow.bottom - rcControl.bottom;
    }
}

void DialogBase::UpdateAnchoredControls()
{
    if (m_vControlAnchors.empty())
        return;

    RECT rcWindow;
    ::GetClientRect(m_hWnd, &rcWindow);
    const int nWindowWidth = rcWindow.right - rcWindow.left;
    const int nWindowHeight = rcWindow.bottom - rcWindow.top;

    for (const auto& info : m_vControlAnchors)
    {
        auto hControl = ::GetDlgItem(m_hWnd, info.nIDDlgItem);
        if (!hControl)
            continue;

        // GetClientRect will always return screen coordinates
        // since we're just after the width/height, that's fine
        RECT rcControl;
        ::GetWindowRect(hControl, &rcControl);

        int nLeft = info.nMarginLeft;
        int nTop = info.nMarginTop;
        int nWidth = rcControl.right - rcControl.left;
        int nHeight = rcControl.bottom - rcControl.top;

#pragma warning(push)
#pragma warning(disable : 4063) // case invalid for entry not in enum: "Left | Right" and "Top | Bottom"

        using namespace ra::bitwise_ops;
        switch (info.nAnchor & (Anchor::Left | Anchor::Right))
        {
            case Anchor::None:
                nLeft = (nWindowWidth - nWidth) / 2;
                break;

            default:
            case Anchor::Left:
                break;

            case Anchor::Right:
                nLeft = nWindowWidth - info.nMarginRight - nWidth;
                break;

            case Anchor::Left | Anchor::Right:
                nWidth = nWindowWidth - info.nMarginLeft - info.nMarginRight;
                break;
        }

        switch (info.nAnchor & (Anchor::Top | Anchor::Bottom))
        {
            case Anchor::None:
                nTop = (nWindowHeight - nHeight) / 2;
                break;

            default:
            case Anchor::Top:
                break;

            case Anchor::Bottom:
                nTop = nWindowHeight - info.nMarginBottom - nHeight;
                break;

            case Anchor::Top | Anchor::Bottom:
                nHeight = nWindowHeight - info.nMarginTop - info.nMarginBottom;
                break;
        }

#pragma warning(pop)

        ::MoveWindow(hControl, nLeft, nTop, nWidth, nHeight, FALSE);

        if (nWidth != rcControl.right - rcControl.left || nHeight != rcControl.bottom - rcControl.top)
        {
            auto* pBinding = FindControlBinding(hControl);
            if (pBinding)
            {
                ra::ui::Size pNewSize{ nWidth, nHeight };
                pBinding->OnSizeChanged(pNewSize);
            }
        }
    }

    ::InvalidateRect(m_hWnd, nullptr, TRUE);
}

} // namespace win32
} // namespace ui
} // namespace ra
