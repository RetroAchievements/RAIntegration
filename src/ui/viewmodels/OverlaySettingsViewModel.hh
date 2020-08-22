#ifndef RA_UI_OVERLAYSETTINGSVIEWMODEL_H
#define RA_UI_OVERLAYSETTINGSVIEWMODEL_H
#pragma once

#include "ui/WindowViewModelBase.hh"

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
    /// The <see cref="ModelProperty" /> for whether or not the achievement triggered notification should be displayed.
    /// </summary>
    static const BoolModelProperty DisplayAchievementTriggerProperty;

    /// <summary>
    /// Gets whether or not the achievement triggered notification should be displayed.
    /// </summary>
    bool DisplayAchievementTrigger() const { return GetValue(DisplayAchievementTriggerProperty); }

    /// <summary>
    /// Sets whether or not the achievement triggered notification should be displayed.
    /// </summary>
    void SetDisplayAchievementTrigger(bool bValue) { SetValue(DisplayAchievementTriggerProperty, bValue); }

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
    /// The <see cref="ModelProperty" /> for whether or not the game mastery notification should be displayed.
    /// </summary>
    static const BoolModelProperty DisplayMasteryProperty;

    /// <summary>
    /// Gets whether or not the game mastery notification should be displayed.
    /// </summary>
    bool DisplayMastery() const { return GetValue(DisplayMasteryProperty); }

    /// <summary>
    /// Sets whether or not the game mastery  notification should be displayed.
    /// </summary>
    void SetDisplayMastery(bool bValue) { SetValue(DisplayMasteryProperty, bValue); }

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
    /// The <see cref="ModelProperty" /> for whether or not the leaderboard started notification should be displayed.
    /// </summary>
    static const BoolModelProperty DisplayLeaderboardStartedProperty;

    /// <summary>
    /// Gets whether or not the leaderboard started notification should be displayed.
    /// </summary>
    bool DisplayLeaderboardStarted() const { return GetValue(DisplayLeaderboardStartedProperty); }

    /// <summary>
    /// Sets whether or not the leaderboard started notification should be displayed.
    /// </summary>
    void SetDisplayLeaderboardStarted(bool bValue) { SetValue(DisplayLeaderboardStartedProperty, bValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the leaderboard canceled notification should be displayed.
    /// </summary>
    static const BoolModelProperty DisplayLeaderboardCanceledProperty;

    /// <summary>
    /// Gets whether or not the leaderboard canceled notification should be displayed.
    /// </summary>
    bool DisplayLeaderboardCanceled() const { return GetValue(DisplayLeaderboardCanceledProperty); }

    /// <summary>
    /// Sets whether or not the leaderboard canceled notification should be displayed.
    /// </summary>
    void SetDisplayLeaderboardCanceled(bool bValue) { SetValue(DisplayLeaderboardCanceledProperty, bValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the value for an active leaderboard should be displayed.
    /// </summary>
    static const BoolModelProperty DisplayLeaderboardValueProperty;

    /// <summary>
    /// Gets whether or not the value for an active leaderboard should be displayed.
    /// </summary>
    bool DisplayLeaderboardValue() const { return GetValue(DisplayLeaderboardValueProperty); }

    /// <summary>
    /// Sets whether or not the value for an active leaderboard should be displayed.
    /// </summary>
    void SetDisplayLeaderboardValue(bool bValue) { SetValue(DisplayLeaderboardValueProperty, bValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the leaderboard scoreboard should be displayed after submitting a leaderboard entry.
    /// </summary>
    static const BoolModelProperty DisplayLeaderboardScoreboardProperty;

    /// <summary>
    /// Gets whether or not the leaderboard scoreboard should be displayed after submitting a leaderboard entry.
    /// </summary>
    bool DisplayLeaderboardScoreboard() const { return GetValue(DisplayLeaderboardScoreboardProperty); }

    /// <summary>
    /// Sets whether or not the leaderboard scoreboard should be displayed after submitting a leaderboard entry.
    /// </summary>
    void SetDisplayLeaderboardScoreboard(bool bValue) { SetValue(DisplayLeaderboardScoreboardProperty, bValue); }

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

protected:
    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_OVERLAYSETTINGSVIEWMODEL_H
