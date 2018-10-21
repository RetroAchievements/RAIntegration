#include "RichPresenceDialog.hh"

#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_RichPresence.h"

namespace ra {
namespace ui {
namespace win32 {

bool RichPresenceDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& oViewModel)
{
    return (dynamic_cast<const ra::ui::viewmodels::RichPresenceMonitorViewModel*>(&oViewModel) != nullptr);
}

void RichPresenceDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& oViewModel)
{
    ShowWindow(oViewModel);
}

void RichPresenceDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& oViewModel)
{
    auto& oRichPresenceViewModel = reinterpret_cast<ra::ui::viewmodels::RichPresenceMonitorViewModel&>(oViewModel);

    if (m_pDialog == nullptr)
    {
        m_pDialog.reset(new RichPresenceDialog(oRichPresenceViewModel));
        m_pDialog->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_RICHPRESENCE), this);
    }
    
    if (m_pDialog->ShowDialogWindow())
        m_pDialog->StartTimer();
}

void RichPresenceDialog::Presenter::OnClosed()
{
    m_pDialog.reset();
}

// ------------------------------------
_Use_decl_annotations_
RichPresenceDialog::RichPresenceDialog(ra::ui::viewmodels::RichPresenceMonitorViewModel& vmRichPresenceDisplay) noexcept
    : DialogBase{ vmRichPresenceDisplay }
{
    m_hFont = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, nullptr);

    m_bindWindow.SetInitialPosition(ra::ui::win32::bindings::RelativePosition::After, ra::ui::win32::bindings::RelativePosition::Far, "Rich Presence Monitor");
    m_bindWindow.BindLabel(IDC_RA_RICHPRESENCERESULTTEXT, ra::ui::viewmodels::RichPresenceMonitorViewModel::DisplayStringProperty);
}

RichPresenceDialog::~RichPresenceDialog() noexcept
{
    DeleteFont(m_hFont);
}

static void UpdateDisplayString(ra::ui::WindowViewModelBase& vmWindow)
{
    auto& vmRichPresence = dynamic_cast<ra::ui::viewmodels::RichPresenceMonitorViewModel&>(vmWindow);
    vmRichPresence.UpdateDisplayString();
}

BOOL RichPresenceDialog::OnInitDialog()
{
    SetWindowFont(::GetDlgItem(GetHWND(), IDC_RA_RICHPRESENCERESULTTEXT), m_hFont, FALSE);
    UpdateDisplayString(m_vmWindow);
    return DialogBase::OnInitDialog();
}

void RichPresenceDialog::OnDestroy()
{
    StopTimer();

    DialogBase::OnDestroy();
}

INT_PTR CALLBACK RichPresenceDialog::DialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg)
    {
        case WM_TIMER:
        {
            UpdateDisplayString(m_vmWindow);
            return TRUE;
        }

        default:
            return DialogBase::DialogProc(hDlg, nMsg, wParam, lParam);
    }
}

void RichPresenceDialog::StartTimer()
{
    if (!m_bTimerActive)
    {
        SetTimer(GetHWND(), 1, 1000, nullptr);
        m_bTimerActive = true;
    }
}

void RichPresenceDialog::StopTimer()
{
    if (m_bTimerActive)
    {
        KillTimer(GetHWND(), 1);
        m_bTimerActive = false;
    }
}

} // namespace win32
} // namespace ui
} // namespace ra
