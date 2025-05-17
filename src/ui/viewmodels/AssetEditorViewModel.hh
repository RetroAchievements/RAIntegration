#ifndef RA_UI_ASSETEDITOR_VIEW_MODEL_H
#define RA_UI_ASSETEDITOR_VIEW_MODEL_H
#pragma once

#include "TriggerViewModel.hh"

#include "data\Types.hh"
#include "data\models\AchievementModel.hh"
#include "data\models\AssetModelBase.hh"

#include "ui\WindowViewModelBase.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class AssetEditorViewModel : public WindowViewModelBase, 
    protected ViewModelBase::NotifyTarget,
    protected ViewModelCollectionBase::NotifyTarget,
    protected ra::data::DataModelBase::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 AssetEditorViewModel() noexcept;
    ~AssetEditorViewModel();

    AssetEditorViewModel(const AssetEditorViewModel&) noexcept = delete;
    AssetEditorViewModel& operator=(const AssetEditorViewModel&) noexcept = delete;
    AssetEditorViewModel(AssetEditorViewModel&&) noexcept = delete;
    AssetEditorViewModel& operator=(AssetEditorViewModel&&) noexcept = delete;

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not an asset is loaded in the editor.
    /// </summary>
    static const BoolModelProperty IsAssetLoadedProperty;

    /// <summary>
    /// Gets whether or not an asset is loaded in the editor.
    /// </summary>
    bool IsAssetLoaded() const { return GetValue(IsAssetLoadedProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not an AssetValidationError is set.
    /// </summary>
    static const BoolModelProperty HasAssetValidationErrorProperty;

    /// <summary>
    /// Gets whether or not an AssetValidationError is set.
    /// </summary>
    bool HasAssetValidationError() const { return GetValue(HasAssetValidationErrorProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the reason why the asset is not valid.
    /// </summary>
    static const StringModelProperty AssetValidationErrorProperty;

    /// <summary>
    /// Gets the reason why the asset is not valid.
    /// </summary>
    const std::wstring& GetAssetValidationError() const { return GetValue(AssetValidationErrorProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not an AssetValidationWarning is set.
    /// </summary>
    static const BoolModelProperty HasAssetValidationWarningProperty;

    /// <summary>
    /// Gets whether or not an AssetValidationWarning is set.
    /// </summary>
    bool HasAssetValidationWarning() const { return GetValue(HasAssetValidationWarningProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the reason why the asset is not valid.
    /// </summary>
    static const StringModelProperty AssetValidationWarningProperty;

    /// <summary>
    /// Gets the reason why the asset is not valid.
    /// </summary>
    const std::wstring& GetAssetValidationWarning() const { return GetValue(AssetValidationWarningProperty); }

    // ===== Common =====

    /// <summary>
    /// The <see cref="ModelProperty" /> for the asset id.
    /// </summary>
    static const IntModelProperty IDProperty;

    /// <summary>
    /// Gets the asset id.
    /// </summary>
    uint32_t GetID() const { return ra::to_unsigned(GetValue(IDProperty)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the name.
    /// </summary>
    static const StringModelProperty NameProperty;

    /// <summary>
    /// Gets the name to display.
    /// </summary>
    const std::wstring& GetName() const { return GetValue(NameProperty); }

    /// <summary>
    /// Sets the name to display.
    /// </summary>
    void SetName(const std::wstring& sValue) { SetValue(NameProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the description of the asset.
    /// </summary>
    static const StringModelProperty DescriptionProperty;

    /// <summary>
    /// Gets the description of the asset.
    /// </summary>
    const std::wstring& GetDescription() const { return GetValue(DescriptionProperty); }

    /// <summary>
    /// Sets the description of the asset.
    /// </summary>
    void SetDescription(const std::wstring& sValue) { SetValue(DescriptionProperty, sValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the asset category.
    /// </summary>
    static const IntModelProperty CategoryProperty;

    /// <summary>
    /// Gets the asset category.
    /// </summary>
    ra::data::models::AssetCategory GetCategory() const { return ra::itoe<ra::data::models::AssetCategory>(GetValue(CategoryProperty)); }

    /// <summary>
    /// Sets the asset category.
    /// </summary>
    void SetCategory(ra::data::models::AssetCategory nValue) { SetValue(CategoryProperty, ra::etoi(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the asset state.
    /// </summary>
    static const IntModelProperty StateProperty;

    /// <summary>
    /// Gets the asset state.
    /// </summary>
    ra::data::models::AssetState GetState() const { return ra::itoe<ra::data::models::AssetState>(GetValue(StateProperty)); }

    /// <summary>
    /// Sets the asset state.
    /// </summary>
    void SetState(ra::data::models::AssetState nValue) { SetValue(StateProperty, ra::etoi(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for a string to bind to an activation checkbox.
    /// Will be "Waiting" if state is Waiting, otherwise will be "Active".
    /// </summary>
    static const StringModelProperty WaitingLabelProperty;

    // ===== Achievement Specific =====

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not an asset is an achievement.
    /// </summary>
    static const BoolModelProperty IsAchievementProperty;

    /// <summary>
    /// Gets whether or not an asset is an achievement.
    /// </summary>
    bool IsAchievement() const { return GetValue(IsAchievementProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the achievement points.
    /// </summary>
    static const IntModelProperty PointsProperty;

    /// <summary>
    /// Gets the points for the achievement.
    /// </summary>
    int GetPoints() const { return GetValue(PointsProperty); }

    /// <summary>
    /// Sets the points for the achievement.
    /// </summary>
    void SetPoints(int nValue) { SetValue(PointsProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the badge.
    /// </summary>
    static const StringModelProperty BadgeProperty;

    /// <summary>
    /// Gets the name of the badge associated to the achievement.
    /// </summary>
    const std::wstring& GetBadge() const { return GetValue(BadgeProperty); }

    /// <summary>
    /// Sets the name of the badge associated to the achievement.
    /// </summary>
    void SetBadge(const std::wstring& sValue) { SetValue(BadgeProperty, sValue); }

    /// <summary>
    /// Opens the file chooser to select a new badge image.
    /// </summary>
    void SelectBadgeFile();

    /// <summary>
    /// The <see cref="ModelProperty" /> for the achievement type.
    /// </summary>
    static const IntModelProperty AchievementTypeProperty;

    /// <summary>
    /// Gets the achievement type.
    /// </summary>
    ra::data::models::AchievementType GetAchievementType() const
    {
        return ra::itoe<ra::data::models::AchievementType>(GetValue(AchievementTypeProperty));
    }

    /// <summary>
    /// Sets the achievement type.
    /// </summary>
    void SetAchievementType(ra::data::models::AchievementType nValue) { SetValue(AchievementTypeProperty, ra::etoi(nValue)); }

    /// <summary>
    /// Gets the list of achievement types.
    /// </summary>
    const LookupItemViewModelCollection& AchievementTypes() const noexcept { return m_vAchievementTypes; }

    // ===== Leaderboard Specific =====

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not an asset is a leaderboard.
    /// </summary>
    static const BoolModelProperty IsLeaderboardProperty;

    /// <summary>
    /// Gets whether or not an asset is a leaderboard.
    /// </summary>
    bool IsLeaderboard() const { return GetValue(IsLeaderboardProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the asset value format.
    /// </summary>
    static const IntModelProperty ValueFormatProperty;

    /// <summary>
    /// Gets the asset value format.
    /// </summary>
    ra::data::ValueFormat GetValueFormat() const { return ra::itoe<ra::data::ValueFormat>(GetValue(ValueFormatProperty)); }

    /// <summary>
    /// Sets the asset value format.
    /// </summary>
    void SetValueFormat(ra::data::ValueFormat nValue) { SetValue(ValueFormatProperty, ra::etoi(nValue)); }

    /// <summary>
    /// Gets the list of leaderboard formats.
    /// </summary>
    const LookupItemViewModelCollection& Formats() const noexcept
    {
        return m_vFormats;
    }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the formatted value.
    /// </summary>
    static const StringModelProperty FormattedValueProperty;

    /// <summary>
    /// Gets the formatted value.
    /// </summary>
    const std::wstring& GetFormattedValue() const { return GetValue(FormattedValueProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not lower values are better.
    /// </summary>
    static const BoolModelProperty LowerIsBetterProperty;

    /// <summary>
    /// Gets whether or not lower values are better.
    /// </summary>
    bool IsLowerBetter() const { return GetValue(LowerIsBetterProperty); }

    /// <summary>
    /// Sets whether or not lower values are better.
    /// </summary>
    void SetLowerIsBetter(bool nValue) { SetValue(LowerIsBetterProperty, nValue); }

    enum class LeaderboardPart
    {
        None = 0,
        Start,
        Submit,
        Cancel,
        Value
    };

    class LeaderboardPartViewModel : public LookupItemViewModel
    {
    public:
        LeaderboardPartViewModel(int nId, const std::wstring& sLabel) noexcept
            : LookupItemViewModel(nId, sLabel)
        {
        }

        LeaderboardPart GetLeaderboardPart() const { return ra::itoe<LeaderboardPart>(GetValue(IDProperty)); }

        static const IntModelProperty ColorProperty;
        Color GetColor() const { return Color(ra::to_unsigned(GetValue(ColorProperty))); }
        void SetColor(Color value) { SetValue(ColorProperty, ra::to_signed(value.ARGB)); }

        bool m_nPreviousHits = false;
    };

    /// <summary>
    /// Gets the list of leaderboard parts.
    /// </summary>
    const ViewModelCollection<LeaderboardPartViewModel>& LeaderboardParts() const noexcept { return m_vLeaderboardParts; }
    ViewModelCollection<LeaderboardPartViewModel>& LeaderboardParts() noexcept { return m_vLeaderboardParts; }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the selected leaderboard part.
    /// </summary>
    static const IntModelProperty SelectedLeaderboardPartProperty;

    /// <summary>
    /// Gets the selected leaderboard part.
    /// </summary>
    LeaderboardPart GetSelectedLeaderboardPart() const { return ra::itoe<LeaderboardPart>(GetValue(SelectedLeaderboardPartProperty)); }

    /// <summary>
    /// Sets the selected leaderboard part.
    /// </summary>
    void SetSelectedLeaderboardPart(LeaderboardPart nValue) { SetValue(SelectedLeaderboardPartProperty, ra::etoi(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the groups header.
    /// </summary>
    static const StringModelProperty GroupsHeaderProperty;

    // ===== Trigger =====

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the emulator should be paused when the trigger is reset.
    /// </summary>
    static const BoolModelProperty PauseOnResetProperty;

    /// <summary>
    /// Gets whether or not the emulator should be paused when the trigger is reset.
    /// </summary>
    bool IsPauseOnReset() const { return GetValue(PauseOnResetProperty); }

    /// <summary>
    /// Sets whether or not the emulator should be paused when the trigger is reset.
    /// </summary>
    void SetPauseOnReset(bool nValue) { SetValue(PauseOnResetProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the emulator should be paused when the achievement triggers.
    /// </summary>
    static const BoolModelProperty PauseOnTriggerProperty;

    /// <summary>
    /// Gets whether or not the emulator should be paused when the achievement triggers.
    /// </summary>
    bool IsPauseOnTrigger() const { return GetValue(PauseOnTriggerProperty); }

    /// <summary>
    /// Sets whether or not the emulator should be paused when the achievement triggers.
    /// </summary>
    void SetPauseOnTrigger(bool nValue) { SetValue(PauseOnTriggerProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not a trigger is being edited.
    /// </summary>
    static const BoolModelProperty IsTriggerProperty;

    /// <summary>
    /// Gets whether or not a trigger is being edited.
    /// </summary>
    bool IsTrigger() const { return GetValue(IsTriggerProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not condition truthiness should be color-highlighted.
    /// </summary>
    static const BoolModelProperty DebugHighlightsEnabledProperty;

    /// <summary>
    /// Gets whether or not condition truthiness should be color-highlighted.
    /// </summary>
    bool AreDebugHighlightsEnabled() const { return GetValue(DebugHighlightsEnabledProperty); }

    /// <summary>
    /// Sets whether or not condition truthiness should be color-highlighted.
    /// </summary>
    void SetDebugHighlightsEnabled(bool nValue) { SetValue(DebugHighlightsEnabledProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the condition values should be shown in decimal.
    /// </summary>
    static const BoolModelProperty DecimalPreferredProperty;

    /// <summary>
    /// Gets whether or not the condition values should be shown in decimal.
    /// </summary>
    bool IsDecimalPreferred() const { return GetValue(DecimalPreferredProperty); }

    /// <summary>
    /// Sets whether or not the condition values should be shown in decimal.
    /// </summary>
    void SetDecimalPreferred(bool nValue) { SetValue(DecimalPreferredProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the trigger has a Measured target.
    /// </summary>
    static const BoolModelProperty HasMeasuredProperty;

    /// <summary>
    /// Gets whether or not the trigger has a Measured target.
    /// </summary>
    bool HasMeasured() const { return GetValue(HasMeasuredProperty); }

    /// <summary>
    /// Sets whether or not the trigger has a Measured target.
    /// </summary>
    void SetMeasured(bool nValue) { SetValue(HasMeasuredProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the Measured display string.
    /// </summary>
    static const StringModelProperty MeasuredValueProperty;

    /// <summary>
    /// Gets the Measured display string.
    /// </summary>
    const std::wstring& GetMeasuredValue() const { return GetValue(MeasuredValueProperty); }

    // ===== Functions =====

    void LoadAsset(ra::data::models::AssetModelBase* pAsset, bool bForce = false);
    const ra::data::models::AssetModelBase* GetAsset() const noexcept { return m_pAsset; }

    void DoFrame();

    TriggerViewModel& Trigger() noexcept { return m_vmTrigger; }
    const TriggerViewModel& Trigger() const noexcept { return m_vmTrigger; }

protected:
    // ViewModelBase::NotifyTarget
    void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override;

    // ViewModelCollectionBase::NotifyTarget
    void OnViewModelBoolValueChanged(gsl::index, const BoolModelProperty::ChangeArgs& args) override;

    // DataModelBase::NotifyTarget
    void OnDataModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args) override;
    void OnDataModelStringValueChanged(const StringModelProperty::ChangeArgs& args) override;
    void OnDataModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override;

    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

    void UpdateTriggerBinding();
    void OnTriggerChanged(bool bIsLoading);

    void UpdateAssetFrameValues();
    void UpdateDebugHighlights();
    void UpdateMeasuredValue();
    void UpdateLeaderboardTrigger();

    void HandleStateChanged(ra::data::models::AssetState nOldState, ra::data::models::AssetState nNewState);

    TriggerViewModel m_vmTrigger;

    ra::data::models::AssetModelBase* m_pAsset = nullptr;
    bool m_bIgnoreTriggerUpdate = false;

    LookupItemViewModelCollection m_vAchievementTypes;
    LookupItemViewModelCollection m_vFormats;
    ViewModelCollection<LeaderboardPartViewModel> m_vLeaderboardParts;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif RA_UI_ASSETEDITOR_VIEW_MODEL_H
