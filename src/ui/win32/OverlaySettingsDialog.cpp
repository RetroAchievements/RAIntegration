#include "OverlaySettingsDialog.hh"

#include "RA_Core.h"
#include "RA_Resource.h"

namespace ra {
namespace ui {
namespace win32 {

bool OverlaySettingsDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const ra::ui::viewmodels::OverlaySettingsViewModel*>(&vmViewModel) != nullptr);
}

void OverlaySettingsDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND hParentWnd)
{
    auto& vmSettings = reinterpret_cast<ra::ui::viewmodels::OverlaySettingsViewModel&>(vmViewModel);

    OverlaySettingsDialog oDialog(vmSettings);
    oDialog.CreateModalWindow(MAKEINTRESOURCE(IDD_RA_OVERLAYSETTINGS), this, hParentWnd);
}

void OverlaySettingsDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& oViewModel)
{
    ShowModal(oViewModel, nullptr);
}

// ------------------------------------

OverlaySettingsDialog::OverlaySettingsDialog(ra::ui::viewmodels::OverlaySettingsViewModel& vmSettings)
    : DialogBase(vmSettings),
      m_bindDisplayAchievementTrigger(vmSettings),
      m_bindScreenshotAchievementTrigger(vmSettings),
      m_bindDisplayMastery(vmSettings),
      m_bindScreenshotMastery(vmSettings),
      m_bindDisplayLeaderboardStarted(vmSettings),
      m_bindDisplayLeaderboardCanceled(vmSettings),
      m_bindDisplayLeaderboardValue(vmSettings),
      m_bindDisplayLeaderboardScoreboards(vmSettings),
      m_bindScreenshotLocation(vmSettings)
{
    m_bindWindow.SetInitialPosition(RelativePosition::Center, RelativePosition::Center);

    m_bindDisplayAchievementTrigger.BindCheck(ra::ui::viewmodels::OverlaySettingsViewModel::DisplayAchievementTriggerProperty);
    m_bindScreenshotAchievementTrigger.BindCheck(ra::ui::viewmodels::OverlaySettingsViewModel::ScreenshotAchievementTriggerProperty);
    m_bindDisplayMastery.BindCheck(ra::ui::viewmodels::OverlaySettingsViewModel::DisplayMasteryProperty);
    m_bindScreenshotMastery.BindCheck(ra::ui::viewmodels::OverlaySettingsViewModel::ScreenshotMasteryProperty);
    m_bindDisplayLeaderboardStarted.BindCheck(ra::ui::viewmodels::OverlaySettingsViewModel::DisplayLeaderboardStartedProperty);
    m_bindDisplayLeaderboardCanceled.BindCheck(ra::ui::viewmodels::OverlaySettingsViewModel::DisplayLeaderboardCanceledProperty);
    m_bindDisplayLeaderboardValue.BindCheck(ra::ui::viewmodels::OverlaySettingsViewModel::DisplayLeaderboardValueProperty);
    m_bindDisplayLeaderboardScoreboards.BindCheck(ra::ui::viewmodels::OverlaySettingsViewModel::DisplayLeaderboardScoreboardProperty);

    m_bindScreenshotLocation.BindText(ra::ui::viewmodels::OverlaySettingsViewModel::ScreenshotLocationProperty);
}

BOOL OverlaySettingsDialog::OnInitDialog()
{
    m_bindDisplayAchievementTrigger.SetControl(*this, IDC_RA_DISPLAY_TRIGGER);
    m_bindScreenshotAchievementTrigger.SetControl(*this, IDC_RA_SCREENSHOT_TRIGGER);
    m_bindDisplayMastery.SetControl(*this, IDC_RA_DISPLAY_MASTERY);
    m_bindScreenshotMastery.SetControl(*this, IDC_RA_SCREENSHOT_MASTERY);
    m_bindDisplayLeaderboardStarted.SetControl(*this, IDC_RA_DISPLAY_LBOARD_START);
    m_bindDisplayLeaderboardCanceled.SetControl(*this, IDC_RA_DISPLAY_LBOARD_CANCEL);
    m_bindDisplayLeaderboardValue.SetControl(*this, IDC_RA_DISPLAY_LBOARD_VALUE);
    m_bindDisplayLeaderboardScoreboards.SetControl(*this, IDC_RA_DISPLAY_LBOARD_SCOREBOARD);

    m_bindScreenshotLocation.SetControl(*this, IDC_RA_SCREENSHOT_LOCATION);

    return DialogBase::OnInitDialog();
}

BOOL OverlaySettingsDialog::OnCommand(WORD nCommand)
{
    if (nCommand == IDC_RA_BROWSE)
    {
        auto& vmSettings = reinterpret_cast<ra::ui::viewmodels::OverlaySettingsViewModel&>(m_vmWindow);
        vmSettings.BrowseLocation();
        return TRUE;
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
