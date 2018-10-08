#include "DialogBase.hh"

#include "RA_Core.h" // g_RAMainWnd, g_hThisDLLInst

namespace ra {
namespace ui {
namespace win32 {

DialogBase::DialogBase(ra::ui::WindowViewModelBase& vmWindow) noexcept
    : m_vmWindow(vmWindow), m_bindWindow(vmWindow)
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

static INT_PTR CALLBACK StaticDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DialogBase* pDialog = reinterpret_cast<DialogBase*>(GetWindowLongPtr(hDlg, DWLP_USER));
    if (pDialog == nullptr)
        return DefWindowProc(hDlg, uMsg, wParam, lParam);

    INT_PTR result = pDialog->DialogProc(hDlg, uMsg, wParam, lParam);

    if (uMsg == WM_DESTROY)
        SetWindowLongPtr(hDlg, DWLP_USER, 0L);

    return result;
}

HWND DialogBase::CreateDialogWindow(LPTSTR sResourceId, IDialogPresenter* pDialogPresenter)
{
    m_hWnd = CreateDialog(g_hThisDLLInst, sResourceId, g_RAMainWnd, &StaticDialogProc);
    if (m_hWnd)
    {
        SetWindowLongPtr(m_hWnd, DWLP_USER, reinterpret_cast<LONG>(this));
        m_pDialogPresenter = pDialogPresenter;
        m_bindWindow.SetHWND(m_hWnd);
    }

    return m_hWnd;
}

INT_PTR CALLBACK DialogBase::DialogProc(_UNUSED HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnInitDialog();
            return TRUE;

        case WM_DESTROY:
            OnDestroy();
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
            return FALSE;
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

BOOL DialogBase::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDOK:
            m_vmWindow.SetDialogResult(ra::ui::DialogResult::OK);
            DestroyWindow(m_hWnd);
            return TRUE;

        case IDCLOSE:
        case IDCANCEL:
            m_vmWindow.SetDialogResult(ra::ui::DialogResult::Cancel);
            DestroyWindow(m_hWnd);
            return TRUE;

        default:
            return FALSE;
    }
}

void DialogBase::OnMove(ra::ui::Position& oNewPosition)
{
    m_bindWindow.OnPositionChanged(oNewPosition);
}

void DialogBase::OnSize(ra::ui::Size& oNewSize)
{
    m_bindWindow.OnSizeChanged(oNewSize);
}

} // namespace win32
} // namespace ui
} // namespace ra
