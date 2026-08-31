#include "OverlaySettingsViewModel.hh"

#include "services\IConfiguration.hh"
#include "services\IWindowConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\FileDialogViewModel.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty OverlaySettingsViewModel::MessageLocationProperty("OverlaySettingsViewModel", "MessageLocation", ra::etoi(PopupLocation::BottomLeft));
const IntModelProperty OverlaySettingsViewModel::AchievementTriggerLocationProperty("OverlaySettingsViewModel", "AchievementTriggerLocation", ra::etoi(PopupLocation::BottomLeft));
const BoolModelProperty OverlaySettingsViewModel::ScreenshotAchievementTriggerProperty("OverlaySettingsViewModel", "ScreenshotAchievementTrigger", false);
const IntModelProperty OverlaySettingsViewModel::MasteryLocationProperty("OverlaySettingsViewModel", "MasteryLocation", ra::etoi(PopupLocation::TopMiddle));
const BoolModelProperty OverlaySettingsViewModel::ScreenshotMasteryProperty("OverlaySettingsViewModel", "ScreenshotMastery", false);
const IntModelProperty OverlaySettingsViewModel::LeaderboardStartedLocationProperty("OverlaySettingsViewModel", "LeaderboardStartedLocation", ra::etoi(PopupLocation::BottomLeft));
const IntModelProperty OverlaySettingsViewModel::LeaderboardCanceledLocationProperty("OverlaySettingsViewModel", "LeaderboardCanceledLocation", ra::etoi(PopupLocation::BottomLeft));
const IntModelProperty OverlaySettingsViewModel::LeaderboardTrackerLocationProperty("OverlaySettingsViewModel", "LeaderboardTrackerLocation", ra::etoi(PopupLocation::BottomRight));
const IntModelProperty OverlaySettingsViewModel::LeaderboardScoreboardLocationProperty("OverlaySettingsViewModel", "LeaderboardScoreboardLocation", ra::etoi(PopupLocation::BottomRight));
const IntModelProperty OverlaySettingsViewModel::ActiveChallengeLocationProperty("OverlaySettingsViewModel", "ActiveChallengeLocation", ra::etoi(PopupLocation::BottomRight));
const IntModelProperty OverlaySettingsViewModel::ProgressTrackerLocationProperty("OverlaySettingsViewModel", "ProgressLocation", ra::etoi(PopupLocation::BottomRight));
const StringModelProperty OverlaySettingsViewModel::ScreenshotLocationProperty("OverlaySettingsViewModel", "ScreenshotLocation", L"");

OverlaySettingsViewModel::OverlaySettingsViewModel() noexcept
{
    SetWindowTitle(L"Overlay Settings");

    m_vPopupLocations.Add(ra::etoi(PopupLocation::None), L"None");
    m_vPopupLocations.Add(ra::etoi(PopupLocation::TopLeft), L"Top Left");
    m_vPopupLocations.Add(ra::etoi(PopupLocation::TopMiddle), L"Top Middle");
    m_vPopupLocations.Add(ra::etoi(PopupLocation::TopRight), L"Top Right");
    m_vPopupLocations.Add(ra::etoi(PopupLocation::BottomLeft), L"Bottom Left");
    m_vPopupLocations.Add(ra::etoi(PopupLocation::BottomMiddle), L"Bottom Middle");
    m_vPopupLocations.Add(ra::etoi(PopupLocation::BottomRight), L"Bottom Right");

    m_vPopupLocationsNoMiddle.Add(ra::etoi(PopupLocation::None), L"None");
    m_vPopupLocationsNoMiddle.Add(ra::etoi(PopupLocation::TopLeft), L"Top Left");
    m_vPopupLocationsNoMiddle.Add(ra::etoi(PopupLocation::TopRight), L"Top Right");
    m_vPopupLocationsNoMiddle.Add(ra::etoi(PopupLocation::BottomLeft), L"Bottom Left");
    m_vPopupLocationsNoMiddle.Add(ra::etoi(PopupLocation::BottomRight), L"Bottom Right");
}

void OverlaySettingsViewModel::Initialize()
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    const auto& pWindowConfiguration = ra::services::ServiceLocator::Get<ra::services::IWindowConfiguration>();

    SetMessageLocation(pWindowConfiguration.GetPopupLocation(Popup::Message));

    // set screenshot before location so location is given preference
    SetScreenshotAchievementTrigger(pConfiguration.IsFeatureEnabled(ra::services::Feature::AchievementTriggeredScreenshot));
    SetAchievementTriggerLocation(pWindowConfiguration.GetPopupLocation(Popup::AchievementTriggered));

    SetScreenshotMastery(pConfiguration.IsFeatureEnabled(ra::services::Feature::MasteryNotificationScreenshot));
    SetMasteryLocation(pWindowConfiguration.GetPopupLocation(Popup::Mastery));

    SetLeaderboardStartedLocation(pWindowConfiguration.GetPopupLocation(Popup::LeaderboardStarted));
    SetLeaderboardCanceledLocation(pWindowConfiguration.GetPopupLocation(Popup::LeaderboardCanceled));
    SetLeaderboardTrackerLocation(pWindowConfiguration.GetPopupLocation(Popup::LeaderboardTracker));
    SetLeaderboardScoreboardLocation(pWindowConfiguration.GetPopupLocation(Popup::LeaderboardScoreboard));

    SetActiveChallengeLocation(pWindowConfiguration.GetPopupLocation(Popup::Challenge));
    SetProgressTrackerLocation(pWindowConfiguration.GetPopupLocation(Popup::Progress));

    SetScreenshotLocation(pConfiguration.GetScreenshotDirectory());
}

void OverlaySettingsViewModel::Commit()
{
    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
    auto& pWindowConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IWindowConfiguration>();

    pWindowConfiguration.SetPopupLocation(Popup::Message, GetMessageLocation());
    pWindowConfiguration.SetPopupLocation(Popup::AchievementTriggered, GetAchievementTriggerLocation());
    pConfiguration.SetFeatureEnabled(ra::services::Feature::AchievementTriggeredScreenshot, ScreenshotAchievementTrigger());
    pWindowConfiguration.SetPopupLocation(Popup::Mastery, GetMasteryLocation());
    pConfiguration.SetFeatureEnabled(ra::services::Feature::MasteryNotificationScreenshot, ScreenshotMastery());
    pWindowConfiguration.SetPopupLocation(Popup::LeaderboardStarted, GetLeaderboardStartedLocation());
    pWindowConfiguration.SetPopupLocation(Popup::LeaderboardCanceled, GetLeaderboardCanceledLocation());
    pWindowConfiguration.SetPopupLocation(Popup::LeaderboardTracker, GetLeaderboardTrackerLocation());
    pWindowConfiguration.SetPopupLocation(Popup::LeaderboardScoreboard, GetLeaderboardScoreboardLocation());
    pWindowConfiguration.SetPopupLocation(Popup::Challenge, GetActiveChallengeLocation());
    pWindowConfiguration.SetPopupLocation(Popup::Progress, GetProgressTrackerLocation());

    std::wstring sLocation = ScreenshotLocation();
    if (!sLocation.empty() && sLocation.back() != '\\')
        sLocation.push_back('\\');

    pConfiguration.SetScreenshotDirectory(sLocation);

    pConfiguration.Save();
}

void OverlaySettingsViewModel::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == ScreenshotAchievementTriggerProperty && args.tNewValue)
    {
        if (GetAchievementTriggerLocation() == PopupLocation::None)
            SetAchievementTriggerLocation(ra::itoe<PopupLocation>(AchievementTriggerLocationProperty.GetDefaultValue()));
    }
    else if (args.Property == ScreenshotMasteryProperty && args.tNewValue)
    {
        if (GetMasteryLocation() == PopupLocation::None)
            SetMasteryLocation(ra::itoe<PopupLocation>(MasteryLocationProperty.GetDefaultValue()));
    }

    WindowViewModelBase::OnValueChanged(args);
}

void OverlaySettingsViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == AchievementTriggerLocationProperty && ra::itoe<PopupLocation>(args.tNewValue) == PopupLocation::None)
        SetScreenshotAchievementTrigger(false);
    else if (args.Property == MasteryLocationProperty && ra::itoe<PopupLocation>(args.tNewValue) == PopupLocation::None)
        SetScreenshotMastery(false);

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
