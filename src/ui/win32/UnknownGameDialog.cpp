#include "UnknownGameDialog.hh"

#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_RichPresence.h"

namespace ra {
namespace ui {
namespace win32 {

bool UnknownGameDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const ra::ui::viewmodels::UnknownGameViewModel*>(&vmViewModel) != nullptr);
}

void UnknownGameDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto& vmLogin = reinterpret_cast<ra::ui::viewmodels::UnknownGameViewModel&>(vmViewModel);

    UnknownGameDialog oDialog(vmLogin);
    oDialog.CreateModalWindow(MAKEINTRESOURCE(IDD_RA_GAMETITLESEL), this);
}

void UnknownGameDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& oViewModel) { ShowModal(oViewModel); }

// ------------------------------------

UnknownGameDialog::UnknownGameDialog(ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame) :
    DialogBase(vmUnknownGame),
    m_bindExistingTitle(vmUnknownGame),
    m_bindNewTitle(vmUnknownGame)
{
    m_bindWindow.SetInitialPosition(RelativePosition::Center, RelativePosition::Center);

    m_bindExistingTitle.BindItems(vmUnknownGame.GameTitles());
    m_bindExistingTitle.BindSelectedItem(ra::ui::viewmodels::UnknownGameViewModel::SelectedGameIdProperty);

    m_bindNewTitle.BindText(ra::ui::viewmodels::UnknownGameViewModel::NewGameNameProperty,
        ra::ui::win32::bindings::TextBoxBinding::UpdateMode::KeyPress);

    m_bindWindow.BindLabel(IDC_RA_CHECKSUM, ra::ui::viewmodels::UnknownGameViewModel::ChecksumProperty);
}

BOOL UnknownGameDialog::OnInitDialog()
{
    m_bindExistingTitle.SetControl(*this, IDC_RA_KNOWNGAMES);
    m_bindNewTitle.SetControl(*this, IDC_RA_GAMETITLE);

    return DialogBase::OnInitDialog();
}

BOOL UnknownGameDialog::OnCommand(WORD nCommand)
{
    if (nCommand == IDOK)
    {
        auto& vmUnknownGame = reinterpret_cast<ra::ui::viewmodels::UnknownGameViewModel&>(m_vmWindow);
        if (!vmUnknownGame.Associate())
            return TRUE;
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
