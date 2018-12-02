#include "DialogBase.hh"

#include "RA_Core.h" // g_RAMainWnd, g_hThisDLLInst

namespace ra {
namespace ui {
namespace win32 {

_Use_decl_annotations_
DialogBase::DialogBase(ra::ui::WindowViewModelBase& vmWindow) :
    m_vmWindow{ vmWindow },
    m_bindWindow{ vmWindow }
{
}

DialogBase::~DialogBase() noexcept
{
    if (m_hWnd)
    {
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
}

_Use_decl_annotations_
_NODISCARD static INT_PTR CALLBACK StaticDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DialogBase* const pDialog = reinterpret_cast<DialogBase*>(GetWindowLongPtr(hDlg, DWLP_USER));

    if (pDialog == nullptr)
        return ::DefWindowProc(hDlg, uMsg, wParam, lParam);

    const INT_PTR result = pDialog->DialogProc(hDlg, uMsg, wParam, lParam);

    if (uMsg == WM_DESTROY)
        ::SetWindowLongPtr(hDlg, DWLP_USER, 0L);

    return result;
}

_Use_decl_annotations_ HWND DialogBase::CreateDialogWindow(const TCHAR* restrict sResourceId,
                                                           IDialogPresenter* const restrict pDialogPresenter)
{
    m_hWnd = ::CreateDialog(g_hThisDLLInst, sResourceId, g_RAMainWnd, StaticDialogProc);
    if (m_hWnd)
    {
        ::SetWindowLongPtr(m_hWnd, DWLP_USER, reinterpret_cast<LONG_PTR>(this));
        m_pDialogPresenter = pDialogPresenter;
        m_bindWindow.SetHWND(m_hWnd);
    }

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
        SetWindowLongPtr(hDlg, DWLP_DLGPROC, reinterpret_cast<LONG_PTR>(&StaticDialogProc));

        SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(s_pModalDialog));
        s_pModalDialog = nullptr;
    }

    return result;
}

_Use_decl_annotations_ void DialogBase::CreateModalWindow(const TCHAR* restrict sResourceId,
                                                          IDialogPresenter* const restrict pDialogPresenter) noexcept
{
    m_pDialogPresenter = pDialogPresenter;
    m_bModal = true;

    s_pModalDialog = this;
    DialogBox(g_hThisDLLInst, sResourceId, g_RAMainWnd, &StaticModalDialogProc);
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
                m_bindWindow.SetHWND(m_hWnd);
            }

            return OnInitDialog();

        case WM_DESTROY:
            OnDestroy();
            return 0;

        case WM_SHOWWINDOW:
            if (static_cast<BOOL>(wParam))
                OnShown();
            return 0;

        case WM_COMMAND:
            return OnCommand(LOWORD(wParam));

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
    if (pClosableDialog)
        pClosableDialog->OnClosed();

    m_pDialogPresenter = nullptr;
}

_Use_decl_annotations_
BOOL DialogBase::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDOK:
            m_vmWindow.SetDialogResult(DialogResult::OK);

            if (m_bModal)
                EndDialog(m_hWnd, IDOK);
            else
                DestroyWindow(m_hWnd);

            return TRUE;

        case IDCLOSE:
        case IDCANCEL:
            m_vmWindow.SetDialogResult(DialogResult::Cancel);

            if (m_bModal)
                EndDialog(m_hWnd, nCommand);
            else
                DestroyWindow(m_hWnd);

            return TRUE;

        default:
            return FALSE;
    }
}

_Use_decl_annotations_
void DialogBase::OnMove(const ra::ui::Position& oNewPosition)
{
    m_bindWindow.OnPositionChanged(oNewPosition);
}

_Use_decl_annotations_
void DialogBase::OnSize(const ra::ui::Size& oNewSize)
{
    m_bindWindow.OnSizeChanged(oNewSize);
}

} // namespace win32
} // namespace ui
} // namespace ra
