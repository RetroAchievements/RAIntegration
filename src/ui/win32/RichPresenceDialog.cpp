#include "RichPresenceDialog.hh"

#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_RichPresence.h"

namespace ra {
namespace ui {
namespace win32 {

bool RichPresenceDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& oViewModel) noexcept
{
    return (dynamic_cast<const ra::ui::viewmodels::RichPresenceMonitorViewModel*>(&oViewModel) != nullptr);
}

void RichPresenceDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel)
{
    ShowWindow(vmViewModel);
}

void RichPresenceDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto& vmRichPresenceViewModel = reinterpret_cast<ra::ui::viewmodels::RichPresenceMonitorViewModel&>(vmViewModel);

    if (m_pDialog == nullptr)
    {
        m_pDialog.reset(new RichPresenceDialog(vmRichPresenceViewModel));
        if (!m_pDialog->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_RICHPRESENCE), this))
            RA_LOG_ERR("Could not create RichPresence dialog!");
    }

    m_pDialog->ShowDialogWindow();
}

void RichPresenceDialog::Presenter::OnClosed() noexcept { m_pDialog.reset(); }

// ------------------------------------
_Use_decl_annotations_
RichPresenceDialog::RichPresenceDialog(ra::ui::viewmodels::RichPresenceMonitorViewModel& vmRichPresenceDisplay)
    : DialogBase(vmRichPresenceDisplay)
{
    m_hFont = CreateFont(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, nullptr);

    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::Far, "Rich Presence Monitor");
    m_bindWindow.BindLabel(IDC_RA_RICHPRESENCERESULTTEXT,
                           ra::ui::viewmodels::RichPresenceMonitorViewModel::DisplayStringProperty);
}

RichPresenceDialog::~RichPresenceDialog() noexcept
{
    DeleteFont(m_hFont);
}


BOOL RichPresenceDialog::OnInitDialog()
{
    SetWindowFont(::GetDlgItem(GetHWND(), IDC_RA_RICHPRESENCERESULTTEXT), m_hFont, FALSE);
    return DialogBase::OnInitDialog();
}

void RichPresenceDialog::OnShown()
{
    auto& vmRichPresence = dynamic_cast<ra::ui::viewmodels::RichPresenceMonitorViewModel&>(m_vmWindow);
    vmRichPresence.StartMonitoring();

    DialogBase::OnShown();
}

void RichPresenceDialog::OnDestroy()
{
    auto& vmRichPresence = dynamic_cast<ra::ui::viewmodels::RichPresenceMonitorViewModel&>(m_vmWindow);
    vmRichPresence.StopMonitoring();

    DialogBase::OnDestroy();
}

} // namespace win32
} // namespace ui
} // namespace ra
