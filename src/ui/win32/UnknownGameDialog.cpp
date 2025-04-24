#include "UnknownGameDialog.hh"

#include "RA_Core.h"
#include "RA_Resource.h"

namespace ra {
namespace ui {
namespace win32 {

bool UnknownGameDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const ra::ui::viewmodels::UnknownGameViewModel*>(&vmViewModel) != nullptr);
}

void UnknownGameDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND hParentWnd)
{
    auto* vmUnknownGame = dynamic_cast<ra::ui::viewmodels::UnknownGameViewModel*>(&vmViewModel);
    Expects(vmUnknownGame != nullptr);

    UnknownGameDialog oDialog(*vmUnknownGame);
    oDialog.CreateModalWindow(MAKEINTRESOURCE(IDD_RA_GAMETITLESEL), this, hParentWnd);
}

void UnknownGameDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& oViewModel) { ShowModal(oViewModel, nullptr); }

// ------------------------------------

UnknownGameDialog::UnknownGameDialog(ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame) :
    DialogBase(vmUnknownGame),
    m_bindExistingTitle(vmUnknownGame),
    m_bindNewTitle(vmUnknownGame)
{
    m_bindWindow.SetInitialPosition(RelativePosition::Center, RelativePosition::Center);
    m_bindWindow.BindLabel(IDC_RA_PROBLEMHEADER, ra::ui::viewmodels::UnknownGameViewModel::ProblemHeaderProperty);

    m_bindExistingTitle.BindItems(vmUnknownGame.GameTitles());
    m_bindExistingTitle.BindSelectedItem(ra::ui::viewmodels::UnknownGameViewModel::SelectedGameIdProperty);
    m_bindWindow.BindEnabled(IDC_RA_KNOWNGAMES, ra::ui::viewmodels::UnknownGameViewModel::IsSelectedGameEnabledProperty);

    m_bindNewTitle.BindText(ra::ui::viewmodels::UnknownGameViewModel::NewGameNameProperty,
        ra::ui::win32::bindings::TextBoxBinding::UpdateMode::KeyPress);
    m_bindWindow.BindEnabled(IDC_RA_GAMETITLE, ra::ui::viewmodels::UnknownGameViewModel::IsSelectedGameEnabledProperty);

    m_bindWindow.BindLabel(IDC_RA_GAMENAME, ra::ui::viewmodels::UnknownGameViewModel::EstimatedGameNameProperty);
    m_bindWindow.BindLabel(IDC_RA_SYSTEMNAME, ra::ui::viewmodels::UnknownGameViewModel::SystemNameProperty);
    m_bindWindow.BindLabel(IDC_RA_CHECKSUM, ra::ui::viewmodels::UnknownGameViewModel::ChecksumProperty);

    m_bindWindow.BindEnabled(IDC_RA_LINK, ra::ui::viewmodels::UnknownGameViewModel::IsAssociateEnabledProperty);
    m_bindWindow.BindEnabled(IDOK, ra::ui::viewmodels::UnknownGameViewModel::IsSelectedGameEnabledProperty);
}

BOOL UnknownGameDialog::OnInitDialog()
{
    m_bindExistingTitle.SetControl(*this, IDC_RA_KNOWNGAMES);
    m_bindNewTitle.SetControl(*this, IDC_RA_GAMETITLE);

    return DialogBase::OnInitDialog();
}

BOOL UnknownGameDialog::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDOK:
        {
            auto* vmUnknownGame = dynamic_cast<ra::ui::viewmodels::UnknownGameViewModel*>(&m_vmWindow);
            if (vmUnknownGame && vmUnknownGame->BeginTest())
                SetDialogResult(ra::ui::DialogResult::OK);
            return TRUE;
        }

        case IDC_RA_LINK:
        {
            auto* vmUnknownGame = dynamic_cast<ra::ui::viewmodels::UnknownGameViewModel*>(&m_vmWindow);
            if (vmUnknownGame && vmUnknownGame->Associate())
                SetDialogResult(ra::ui::DialogResult::OK);
            return TRUE;
        }

        case IDC_RA_COPYCHECKSUMCLIPBOARD:
        {
            const auto* vmUnknownGame = dynamic_cast<ra::ui::viewmodels::UnknownGameViewModel*>(&m_vmWindow);
            if (vmUnknownGame)
                vmUnknownGame->CopyChecksumToClipboard();
            return TRUE;
        }
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
