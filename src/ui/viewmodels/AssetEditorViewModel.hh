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
    /// The <see cref="ModelProperty" /> for whether or not the emulator should be paused when the achievement triggers.
    /// </summary>
    static const BoolModelProperty AssetLoadedProperty;

    /// <summary>
    /// Gets whether or not the emulator should be paused when the achievement triggers.
    /// </summary>
    bool IsAssetLoaded() const { return GetValue(AssetLoadedProperty); }

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

    // ===== Achievement Specific =====

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

    // ===== Functions =====

    void LoadAsset(ra::data::models::AssetModelBase* pAsset);

    void DoFrame();

    TriggerViewModel& Trigger() noexcept { return m_vmTrigger; }
    const TriggerViewModel& Trigger() const noexcept { return m_vmTrigger; }

protected:
    // ViewModelBase::NotifyTarget
    void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override;

    // DataModelBase::NotifyTarget
    void OnDataModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args) override;
    void OnDataModelStringValueChanged(const StringModelProperty::ChangeArgs& args) override;
    void OnDataModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override;

    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

    void UpdateTriggerBinding();
    void OnTriggerChanged();

    TriggerViewModel m_vmTrigger;

    ra::data::models::AssetModelBase* m_pAsset = nullptr;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif RA_UI_ASSETEDITOR_VIEW_MODEL_H
