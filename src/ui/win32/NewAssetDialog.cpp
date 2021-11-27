#include "NewAssetDialog.hh"

#include "RA_Core.h"
#include "RA_Resource.h"

namespace ra {
namespace ui {
namespace win32 {

bool NewAssetDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const ra::ui::viewmodels::NewAssetViewModel*>(&vmViewModel) != nullptr);
}

void NewAssetDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND hParentWnd)
{
    auto* vmGameChecksum = dynamic_cast<ra::ui::viewmodels::NewAssetViewModel*>(&vmViewModel);
    Expects(vmGameChecksum != nullptr);

    NewAssetDialog oDialog(*vmGameChecksum);
    oDialog.CreateModalWindow(MAKEINTRESOURCE(IDD_RA_NEWASSET), this, hParentWnd);
}

void NewAssetDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& oViewModel)
{
    ShowModal(oViewModel, nullptr);
}

// ------------------------------------

NewAssetDialog::NewAssetDialog(ra::ui::viewmodels::NewAssetViewModel& vmGameChecksum) noexcept
    : DialogBase(vmGameChecksum)
{
}

BOOL NewAssetDialog::OnCommand(WORD nCommand)
{
    if (nCommand == IDC_RA_NEW_ACHIEVEMENT)
    {
        auto* vmNewAsset = dynamic_cast<ra::ui::viewmodels::NewAssetViewModel*>(&m_vmWindow);
        if (vmNewAsset)
            vmNewAsset->NewAchievement();
        return TRUE;
    }

    if (nCommand == IDC_RA_NEW_LEADERBOARD)
    {
        auto* vmNewAsset = dynamic_cast<ra::ui::viewmodels::NewAssetViewModel*>(&m_vmWindow);
        if (vmNewAsset)
            vmNewAsset->NewLeaderboard();
        return TRUE;
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
