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
    auto& vmSettings = dynamic_cast<ra::ui::viewmodels::OverlaySettingsViewModel&>(vmViewModel);

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
      m_bindAchievementTriggerLocation(vmSettings),
      m_bindScreenshotAchievementTrigger(vmSettings),
      m_bindDisplayMastery(vmSettings),
      m_bindScreenshotMastery(vmSettings),
      m_bindDisplayLeaderboardStarted(vmSettings),
      m_bindDisplayLeaderboardCanceled(vmSettings),
      m_bindDisplayLeaderboardValue(vmSettings),
      m_bindDisplayLeaderboardScoreboards(vmSettings),
      m_bindChallengeNotifications(vmSettings),
      m_bindProgressTrackers(vmSettings),
      m_bindScreenshotLocation(vmSettings),
      m_bindInformationLocation(vmSettings)
{
    m_bindWindow.SetInitialPosition(RelativePosition::Center, RelativePosition::Center);

    m_bindAchievementTriggerLocation.BindItems(vmSettings.PopupLocations());
    m_bindAchievementTriggerLocation.BindSelectedItem(ra::ui::viewmodels::OverlaySettingsViewModel::AchievementTriggerLocationProperty);

    m_bindScreenshotAchievementTrigger.BindCheck(ra::ui::viewmodels::OverlaySettingsViewModel::ScreenshotAchievementTriggerProperty);

    m_bindDisplayMastery.BindItems(vmSettings.PopupLocations());
    m_bindDisplayMastery.BindSelectedItem(ra::ui::viewmodels::OverlaySettingsViewModel::MasteryLocationProperty);
    m_bindScreenshotMastery.BindCheck(ra::ui::viewmodels::OverlaySettingsViewModel::ScreenshotMasteryProperty);

    m_bindDisplayLeaderboardStarted.BindItems(vmSettings.PopupLocations());
    m_bindDisplayLeaderboardStarted.BindSelectedItem(ra::ui::viewmodels::OverlaySettingsViewModel::LeaderboardStartedLocationProperty);
    m_bindDisplayLeaderboardCanceled.BindItems(vmSettings.PopupLocations());
    m_bindDisplayLeaderboardCanceled.BindSelectedItem(ra::ui::viewmodels::OverlaySettingsViewModel::LeaderboardCanceledLocationProperty);
    m_bindDisplayLeaderboardValue.BindItems(vmSettings.PopupLocations());
    m_bindDisplayLeaderboardValue.BindSelectedItem(ra::ui::viewmodels::OverlaySettingsViewModel::LeaderboardTrackerLocationProperty);
    m_bindDisplayLeaderboardScoreboards.BindItems(vmSettings.PopupLocationsNoMiddle());
    m_bindDisplayLeaderboardScoreboards.BindSelectedItem(ra::ui::viewmodels::OverlaySettingsViewModel::LeaderboardScoreboardLocationProperty);

    m_bindChallengeNotifications.BindItems(vmSettings.PopupLocationsNoMiddle());
    m_bindChallengeNotifications.BindSelectedItem(ra::ui::viewmodels::OverlaySettingsViewModel::ActiveChallengeLocationProperty);
    m_bindProgressTrackers.BindItems(vmSettings.PopupLocationsNoMiddle());
    m_bindProgressTrackers.BindSelectedItem(ra::ui::viewmodels::OverlaySettingsViewModel::ProgressTrackerLocationProperty);

    m_bindInformationLocation.BindItems(vmSettings.PopupLocations());
    m_bindInformationLocation.BindSelectedItem(ra::ui::viewmodels::OverlaySettingsViewModel::MessageLocationProperty);

    m_bindScreenshotLocation.BindText(ra::ui::viewmodels::OverlaySettingsViewModel::ScreenshotLocationProperty);
}

BOOL OverlaySettingsDialog::OnInitDialog()
{
    m_bindAchievementTriggerLocation.SetControl(*this, IDC_RA_DISPLAY_TRIGGER);
    m_bindScreenshotAchievementTrigger.SetControl(*this, IDC_RA_SCREENSHOT_TRIGGER);
    m_bindDisplayMastery.SetControl(*this, IDC_RA_DISPLAY_MASTERY);
    m_bindScreenshotMastery.SetControl(*this, IDC_RA_SCREENSHOT_MASTERY);
    m_bindDisplayLeaderboardStarted.SetControl(*this, IDC_RA_DISPLAY_LBOARD_START);
    m_bindDisplayLeaderboardCanceled.SetControl(*this, IDC_RA_DISPLAY_LBOARD_CANCEL);
    m_bindDisplayLeaderboardValue.SetControl(*this, IDC_RA_DISPLAY_LBOARD_VALUE);
    m_bindDisplayLeaderboardScoreboards.SetControl(*this, IDC_RA_DISPLAY_LBOARD_SCOREBOARD);
    m_bindChallengeNotifications.SetControl(*this, IDC_RA_DISPLAY_CHALLENGE_INDICATOR);
    m_bindProgressTrackers.SetControl(*this, IDC_RA_DISPLAY_PROGRESS_INDICATOR);
    m_bindInformationLocation.SetControl(*this, IDC_RA_DISPLAY_INFORMATION);

    m_bindScreenshotLocation.SetControl(*this, IDC_RA_SCREENSHOT_LOCATION);

    return DialogBase::OnInitDialog();
}

BOOL OverlaySettingsDialog::OnCommand(WORD nCommand)
{
    if (nCommand == IDC_RA_BROWSE)
    {
        auto& vmSettings = dynamic_cast<ra::ui::viewmodels::OverlaySettingsViewModel&>(m_vmWindow);
        vmSettings.BrowseLocation();
        return TRUE;
    }

    return DialogBase::OnCommand(nCommand);
}

} // namespace win32
} // namespace ui
} // namespace ra
