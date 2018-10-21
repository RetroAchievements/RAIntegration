#include "DialogBase.hh"

#include "RA_Core.h" // g_RAMainWnd, g_hThisDLLInst

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
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
}

_Use_decl_annotations_
_NODISCARD static INT_PTR CALLBACK StaticDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    const auto pDialog{ reinterpret_cast<DialogBase*>(GetWindowLongPtr(hDlg, DWLP_USER)) };

    // TBD: A dialog should not be calling DefWindowProc, it should return 0 
    if (pDialog == nullptr)
        return ::DefWindowProc(hDlg, uMsg, wParam, lParam);

    const auto result{ pDialog->DialogProc(hDlg, uMsg, wParam, lParam) };

    if (uMsg == WM_DESTROY)
        ::SetWindowLongPtr(hDlg, DWLP_USER, 0L);

    return result;
}

_Use_decl_annotations_
HWND DialogBase::CreateDialogWindow(const LPCTSTR sResourceId, IDialogPresenter* const pDialogPresenter)
{
    m_hWnd = ::CreateDialogParam(g_hThisDLLInst, sResourceId, g_RAMainWnd, StaticDialogProc,
                               reinterpret_cast<LPARAM>(this));
    if (m_hWnd)
    {
        ::SetWindowLongPtr(m_hWnd, DWLP_USER, reinterpret_cast<LONG_PTR>(this));
        m_pDialogPresenter = pDialogPresenter;
        m_bindWindow.SetHWND(m_hWnd);
    }

    return m_hWnd;
}

_Use_decl_annotations_
INT_PTR CALLBACK DialogBase::DialogProc(_UNUSED HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return OnInitDialog();

        case WM_DESTROY:
            OnDestroy();
            return 0;

        case WM_COMMAND:
        {
            const auto id{ ra::to_signed(LOWORD(wParam)) };
            if (id < 1)
                return -1; // Invalid Command ID
            OnCommand(id);
            return 0;
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
    if (pClosableDialog)
        pClosableDialog->OnClosed();

    m_pDialogPresenter = nullptr;
}

_Use_decl_annotations_
void DialogBase::OnCommand(int id)
{
    switch (id)
    {
        case IDOK:
            m_vmWindow.SetDialogResult(DialogResult::OK);
            DestroyWindow(m_hWnd);
            break;

        case IDCLOSE:
        case IDCANCEL:
            m_vmWindow.SetDialogResult(DialogResult::Cancel);
            DestroyWindow(m_hWnd);
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
