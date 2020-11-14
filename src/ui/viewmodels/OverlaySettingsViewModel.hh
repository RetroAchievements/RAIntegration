#ifndef RA_UI_OVERLAYSETTINGSVIEWMODEL_H
#define RA_UI_OVERLAYSETTINGSVIEWMODEL_H
#pragma once

#include "ui/WindowViewModelBase.hh"
#include "ui/viewmodels/LookupItemViewModel.hh"
#include "ui/viewmodels/PopupViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class OverlaySettingsViewModel : public WindowViewModelBase
{
public:
    GSL_SUPPRESS_F6 OverlaySettingsViewModel() noexcept;

    /// <summary>
    /// Sets the initial values of each checkbox from the current configuration.
    /// </summary>
    void Initialize();

    /// <summary>
    /// Updates the current configuration with values from the checkboxes.
    /// </summary>
    void Commit();

    /// <summary>
    /// The <see cref="ModelProperty" /> for where the message notification should be displayed.
    /// </summary>
    static const IntModelProperty MessageLocationProperty;

    /// <summary>
    /// Gets the location where message notification popups should be displayed.
    /// </summary>
    PopupLocation GetMessageLocation() const { return ra::itoe<PopupLocation>(GetValue(MessageLocationProperty)); }

    /// <summary>
    /// Sets the location where message notification popups should be displayed.
    /// </summary>
    void SetMessageLocation(PopupLocation nValue) { SetValue(MessageLocationProperty, ra::etoi(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for where the achievement triggered notification should be displayed.
    /// </summary>
    static const IntModelProperty AchievementTriggerLocationProperty;

    /// <summary>
    /// Gets the location where achievement triggered notification popups should be displayed.
    /// </summary>
    PopupLocation GetAchievementTriggerLocation() const { return ra::itoe<PopupLocation>(GetValue(AchievementTriggerLocationProperty)); }

    /// <summary>
    /// Sets the location where achievement triggered notification popups should be displayed.
    /// </summary>
    void SetAchievementTriggerLocation(PopupLocation nValue) { SetValue(AchievementTriggerLocationProperty, ra::etoi(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the achievement triggered notification should be screenshoted.
    /// </summary>
    static const BoolModelProperty ScreenshotAchievementTriggerProperty;

    /// <summary>
    /// Gets whether or not the achievement triggered notification should be screenshoted.
    /// </summary>
    bool ScreenshotAchievementTrigger() const { return GetValue(ScreenshotAchievementTriggerProperty); }

    /// <summary>
    /// Sets whether or not the achievement triggered notification should be screenshoted.
    /// </summary>
    void SetScreenshotAchievementTrigger(bool bValue) { SetValue(ScreenshotAchievementTriggerProperty, bValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for where the mastery notification should be displayed.
    /// </summary>
    static const IntModelProperty MasteryLocationProperty;

    /// <summary>
    /// Gets the location where mastery notification popups should be displayed.
    /// </summary>
    PopupLocation GetMasteryLocation() const { return ra::itoe<PopupLocation>(GetValue(MasteryLocationProperty)); }

    /// <summary>
    /// Sets the location where mastery notification popups should be displayed.
    /// </summary>
    void SetMasteryLocation(PopupLocation nValue) { SetValue(MasteryLocationProperty, ra::etoi(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the game mastery notification should be screenshoted.
    /// </summary>
    static const BoolModelProperty ScreenshotMasteryProperty;

    /// <summary>
    /// Gets whether or not the game mastery notification should be screenshoted.
    /// </summary>
    bool ScreenshotMastery() const { return GetValue(ScreenshotMasteryProperty); }

    /// <summary>
    /// Sets whether or not the game mastery notification should be screenshoted.
    /// </summary>
    void SetScreenshotMastery(bool bValue) { SetValue(ScreenshotMasteryProperty, bValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for where the leaderboard started notification should be displayed.
    /// </summary>
    static const IntModelProperty LeaderboardStartedLocationProperty;

    /// <summary>
    /// Gets the location where leaderboard started notification popups should be displayed.
    /// </summary>
    PopupLocation GetLeaderboardStartedLocation() const { return ra::itoe<PopupLocation>(GetValue(LeaderboardStartedLocationProperty)); }

    /// <summary>
    /// Sets the location where leaderboard started notification popups should be displayed.
    /// </summary>
    void SetLeaderboardStartedLocation(PopupLocation nValue) { SetValue(LeaderboardStartedLocationProperty, ra::etoi(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for where the leaderboard canceled notification should be displayed.
    /// </summary>
    static const IntModelProperty LeaderboardCanceledLocationProperty;

    /// <summary>
    /// Gets the location where leaderboard canceled notification popups should be displayed.
    /// </summary>
    PopupLocation GetLeaderboardCanceledLocation() const { return ra::itoe<PopupLocation>(GetValue(LeaderboardCanceledLocationProperty)); }

    /// <summary>
    /// Sets the location where leaderboard canceled notification popups should be displayed.
    /// </summary>
    void SetLeaderboardCanceledLocation(PopupLocation nValue) { SetValue(LeaderboardCanceledLocationProperty, ra::etoi(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for where the leaderboard tracker should be displayed.
    /// </summary>
    static const IntModelProperty LeaderboardTrackerLocationProperty;

    /// <summary>
    /// Gets the location where leaderboard tracker popups should be displayed.
    /// </summary>
    PopupLocation GetLeaderboardTrackerLocation() const { return ra::itoe<PopupLocation>(GetValue(LeaderboardTrackerLocationProperty)); }

    /// <summary>
    /// Sets the location where leaderboard tracker popups should be displayed.
    /// </summary>
    void SetLeaderboardTrackerLocation(PopupLocation nValue) { SetValue(LeaderboardTrackerLocationProperty, ra::etoi(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for where the leaderboard scoreboard should be displayed.
    /// </summary>
    static const IntModelProperty LeaderboardScoreboardLocationProperty;

    /// <summary>
    /// Gets the location where leaderboard scoreboard popups should be displayed.
    /// </summary>
    PopupLocation GetLeaderboardScoreboardLocation() const { return ra::itoe<PopupLocation>(GetValue(LeaderboardScoreboardLocationProperty)); }

    /// <summary>
    /// Sets the location where leaderboard scoreboard popups should be displayed.
    /// </summary>
    void SetLeaderboardScoreboardLocation(PopupLocation nValue) { SetValue(LeaderboardScoreboardLocationProperty, ra::etoi(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the location to write screenshots.
    /// </summary>
    static const StringModelProperty ScreenshotLocationProperty;

    /// <summary>
    /// Gets the location to write screenshots.
    /// </summary>
    const std::wstring& ScreenshotLocation() const { return GetValue(ScreenshotLocationProperty); }

    /// <summary>
    /// Sets the location to write screenshots.
    /// </summary>
    void SetScreenshotLocation(const std::wstring& sValue) { SetValue(ScreenshotLocationProperty, sValue); }

    /// <summary>
    /// Opens the folder browser dialog to select a <see cref="ScreenshotLocation" />.
    /// </summary>
    void BrowseLocation();

    /// <summary>
    /// Gets the list of popup locations.
    /// </summary>
    const LookupItemViewModelCollection& PopupLocations() const noexcept
    {
        return m_vPopupLocations;
    }

    /// <summary>
    /// Gets the list of popup locations without middle options.
    /// </summary>
    const LookupItemViewModelCollection& PopupLocationsNoMiddle() const noexcept
    {
        return m_vPopupLocationsNoMiddle;
    }

protected:
    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

private:
    LookupItemViewModelCollection m_vPopupLocations;
    LookupItemViewModelCollection m_vPopupLocationsNoMiddle;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_OVERLAYSETTINGSVIEWMODEL_H
