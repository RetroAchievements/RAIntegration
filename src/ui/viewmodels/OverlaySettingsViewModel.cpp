#include "OverlaySettingsViewModel.hh"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\FileDialogViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const BoolModelProperty OverlaySettingsViewModel::DisplayAchievementTriggerProperty("OverlaySettingsViewModel", "DisplayAchievementTrigger", true);
const BoolModelProperty OverlaySettingsViewModel::ScreenshotAchievementTriggerProperty("OverlaySettingsViewModel", "ScreenshotAchievementTrigger", false);
const BoolModelProperty OverlaySettingsViewModel::DisplayMasteryProperty("OverlaySettingsViewModel", "DisplayMastery", true);
const BoolModelProperty OverlaySettingsViewModel::ScreenshotMasteryProperty("OverlaySettingsViewModel", "ScreenshotMastery", false);
const BoolModelProperty OverlaySettingsViewModel::DisplayLeaderboardStartedProperty("OverlaySettingsViewModel", "DisplayLeaderboardStarted", true);
const BoolModelProperty OverlaySettingsViewModel::DisplayLeaderboardCanceledProperty("OverlaySettingsViewModel", "DisplayLeaderboardCanceled", true);
const BoolModelProperty OverlaySettingsViewModel::DisplayLeaderboardValueProperty("OverlaySettingsViewModel", "DisplayLeaderboardValue", true);
const BoolModelProperty OverlaySettingsViewModel::DisplayLeaderboardScoreboardProperty("OverlaySettingsViewModel", "DisplayLeaderboardScoreboard", true);
const StringModelProperty OverlaySettingsViewModel::ScreenshotLocationProperty("OverlaySettingsViewModel", "ScreenshotLocation", L"");

OverlaySettingsViewModel::OverlaySettingsViewModel() noexcept
{
    SetWindowTitle(L"Overlay Settings");
}

void OverlaySettingsViewModel::Initialize()
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    SetDisplayAchievementTrigger(pConfiguration.IsFeatureEnabled(ra::services::Feature::AchievementTriggeredNotifications));
    SetScreenshotAchievementTrigger(pConfiguration.IsFeatureEnabled(ra::services::Feature::AchievementTriggeredScreenshot));
    SetDisplayMastery(pConfiguration.IsFeatureEnabled(ra::services::Feature::MasteryNotification));
    SetScreenshotMastery(pConfiguration.IsFeatureEnabled(ra::services::Feature::MasteryNotificationScreenshot));
    SetDisplayLeaderboardStarted(pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardNotifications));
    SetDisplayLeaderboardCanceled(pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardCancelNotifications));
    SetDisplayLeaderboardValue(pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardCounters));
    SetDisplayLeaderboardScoreboard(pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardScoreboards));

    SetScreenshotLocation(pConfiguration.GetScreenshotDirectory());
}

void OverlaySettingsViewModel::Commit()
{
    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();

    pConfiguration.SetFeatureEnabled(ra::services::Feature::AchievementTriggeredNotifications, DisplayAchievementTrigger());
    pConfiguration.SetFeatureEnabled(ra::services::Feature::AchievementTriggeredScreenshot, ScreenshotAchievementTrigger());
    pConfiguration.SetFeatureEnabled(ra::services::Feature::MasteryNotification, DisplayMastery());
    pConfiguration.SetFeatureEnabled(ra::services::Feature::MasteryNotificationScreenshot, ScreenshotMastery());
    pConfiguration.SetFeatureEnabled(ra::services::Feature::LeaderboardNotifications, DisplayLeaderboardStarted());
    pConfiguration.SetFeatureEnabled(ra::services::Feature::LeaderboardCancelNotifications, DisplayLeaderboardCanceled());
    pConfiguration.SetFeatureEnabled(ra::services::Feature::LeaderboardCounters, DisplayLeaderboardValue());
    pConfiguration.SetFeatureEnabled(ra::services::Feature::LeaderboardScoreboards, DisplayLeaderboardScoreboard());

    std::wstring sLocation = ScreenshotLocation();
    if (!sLocation.empty() && sLocation.back() != '\\')
        sLocation.push_back('\\');

    pConfiguration.SetScreenshotDirectory(sLocation);

    pConfiguration.Save();
}

void OverlaySettingsViewModel::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == DisplayAchievementTriggerProperty && !args.tNewValue)
        SetScreenshotAchievementTrigger(false);
    else if (args.Property == ScreenshotAchievementTriggerProperty && args.tNewValue)
        SetDisplayAchievementTrigger(true);
    else if (args.Property == DisplayMasteryProperty && !args.tNewValue)
        SetScreenshotMastery(false);
    else if (args.Property == ScreenshotMasteryProperty && args.tNewValue)
        SetDisplayMastery(true);

    WindowViewModelBase::OnValueChanged(args);
}

void OverlaySettingsViewModel::BrowseLocation()
{
    ra::ui::viewmodels::FileDialogViewModel vmFolder;
    vmFolder.SetWindowTitle(L"Select Screenshot Location");
    vmFolder.SetInitialDirectory(ScreenshotLocation());

    if (vmFolder.ShowSelectFolderDialog(*this) == ra::ui::DialogResult::OK)
    {
        std::wstring sLocation = vmFolder.GetFileName();
        if (!sLocation.empty() && sLocation.back() != '\\')
            sLocation.push_back('\\');

        SetScreenshotLocation(sLocation);
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
