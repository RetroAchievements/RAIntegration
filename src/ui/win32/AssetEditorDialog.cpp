#include "AssetEditorDialog.hh"

#include "RA_Resource.h"

#include "data\EmulatorContext.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "ui\win32\bindings\GridAddressColumnBinding.hh"
#include "ui\win32\bindings\GridLookupColumnBinding.hh"
#include "ui\win32\bindings\GridNumberColumnBinding.hh"
#include "ui\win32\bindings\GridTextColumnBinding.hh"

using ra::ui::viewmodels::AssetEditorViewModel;
using ra::ui::viewmodels::AssetViewModelBase;
using ra::ui::viewmodels::AchievementViewModel;
using ra::ui::win32::bindings::GridColumnBinding;

namespace ra {
namespace ui {
namespace win32 {

bool AssetEditorDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const AssetEditorViewModel*>(&vmViewModel) != nullptr);
}

void AssetEditorDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND)
{
    ShowWindow(vmViewModel);
}

void AssetEditorDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto* vmMemoryBookmarks = dynamic_cast<AssetEditorViewModel*>(&vmViewModel);
    Expects(vmMemoryBookmarks != nullptr);

    if (m_pDialog == nullptr)
    {
        m_pDialog.reset(new AssetEditorDialog(*vmMemoryBookmarks));
        if (!m_pDialog->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_ACHIEVEMENTEDITOR), this))
            RA_LOG_ERR("Could not create Asset Editor dialog!");
    }

    m_pDialog->ShowDialogWindow();
}

void AssetEditorDialog::Presenter::OnClosed() noexcept { m_pDialog.reset(); }

// ------------------------------------

AssetEditorDialog::AssetEditorDialog(AssetEditorViewModel& vmAssetList)
    : DialogBase(vmAssetList),
      m_bindID(vmAssetList),
      m_bindName(vmAssetList),
      m_bindDescription(vmAssetList),
      m_bindBadge(vmAssetList),
      m_bindPoints(vmAssetList),
      m_bindPauseOnReset(vmAssetList),
      m_bindPauseOnTrigger(vmAssetList)
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::After, "Achievement Editor");

    m_bindID.BindValue(AssetEditorViewModel::IDProperty);
    m_bindName.BindText(AssetEditorViewModel::NameProperty);
    m_bindDescription.BindText(AssetEditorViewModel::DescriptionProperty);
    m_bindBadge.BindText(AssetEditorViewModel::BadgeProperty);
    m_bindPoints.BindValue(AssetEditorViewModel::PointsProperty);

    m_bindPauseOnReset.BindCheck(AssetEditorViewModel::PauseOnResetProperty);
    m_bindPauseOnTrigger.BindCheck(AssetEditorViewModel::PauseOnTriggerProperty);

    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_LISTACHIEVEMENTS, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);

    SetMinimumSize(400, 188);
}

BOOL AssetEditorDialog::OnInitDialog()
{
    m_bindID.SetControl(*this, IDC_RA_ACH_ID);
    m_bindName.SetControl(*this, IDC_RA_ACH_TITLE);
    m_bindDescription.SetControl(*this, IDC_RA_ACH_DESC);
    m_bindBadge.SetControl(*this, IDC_RA_BADGENAME);
    m_bindPoints.SetControl(*this, IDC_RA_ACH_POINTS);

    return DialogBase::OnInitDialog();
}

BOOL AssetEditorDialog::OnCommand(WORD nCommand)
{
    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
