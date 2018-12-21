#include "GameChecksumDialog.hh"

#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_RichPresence.h"

namespace ra {
namespace ui {
namespace win32 {

bool GameChecksumDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const ra::ui::viewmodels::GameChecksumViewModel*>(&vmViewModel) != nullptr);
}

void GameChecksumDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto& vmGameChecksum = reinterpret_cast<ra::ui::viewmodels::GameChecksumViewModel&>(vmViewModel);

    GameChecksumDialog oDialog(vmGameChecksum);
    oDialog.CreateModalWindow(MAKEINTRESOURCE(IDD_RA_ROMCHECKSUM), this);
}

void GameChecksumDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& oViewModel)
{
    ShowModal(oViewModel);
}

// ------------------------------------

GameChecksumDialog::GameChecksumDialog(ra::ui::viewmodels::GameChecksumViewModel& vmGameChecksum)
    : DialogBase(vmGameChecksum)
{
    m_bindWindow.SetInitialPosition(RelativePosition::Center, RelativePosition::Center);
    m_bindWindow.BindLabel(IDC_RA_ROMCHECKSUMTEXT, ra::ui::viewmodels::GameChecksumViewModel::ChecksumProperty);
}

BOOL GameChecksumDialog::OnCommand(WORD nCommand)
{
    if (nCommand == IDC_RA_COPYCHECKSUMCLIPBOARD)
    {
        const auto& vmGameChecksum = reinterpret_cast<ra::ui::viewmodels::GameChecksumViewModel&>(m_vmWindow);
        vmGameChecksum.CopyChecksumToClipboard();
        return TRUE;
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
