#ifndef RA_UI_ASSETLIST_VIEW_MODEL_H
#define RA_UI_ASSETLIST_VIEW_MODEL_H
#pragma once

#include "ui\WindowViewModelBase.hh"
#include "ui\ViewModelCollection.hh"
#include "ui\viewmodels\LookupItemViewModel.hh"

#include "AssetViewModelBase.hh"

#include "AchievementViewModel.hh"

#include "data\Types.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class AssetListViewModel : public WindowViewModelBase,
    protected ViewModelCollectionBase::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 AssetListViewModel() noexcept;
    ~AssetListViewModel();

    AssetListViewModel(const AssetListViewModel&) noexcept = delete;
    AssetListViewModel& operator=(const AssetListViewModel&) noexcept = delete;
    AssetListViewModel(AssetListViewModel&&) noexcept = delete;
    AssetListViewModel& operator=(AssetListViewModel&&) noexcept = delete;

    /// <summary>
    /// The <see cref="ModelProperty" /> for the game ID.
    /// </summary>
    static const IntModelProperty GameIdProperty;

    /// <summary>
    /// Gets the game ID.
    /// </summary>
    unsigned GetGameId() const { return ra::to_unsigned(GetValue(GameIdProperty)); }

    /// <summary>
    /// Sets the game ID.
    /// </summary>
    void SetGameId(unsigned nValue) { SetValue(GameIdProperty, ra::to_signed(nValue)); }

    // <summary>
    /// The <see cref="ModelProperty" /> for the core achievement count.
    /// </summary>
    static const IntModelProperty AchievementCountProperty;

    /// <summary>
    /// Gets the total number of core achievements.
    /// </summary>
    int GetAchievementCount() const { return GetValue(AchievementCountProperty); }

    // <summary>
    /// The <see cref="ModelProperty" /> for the total core achievement points.
    /// </summary>
    static const IntModelProperty TotalPointsProperty;

    /// <summary>
    /// Gets the total number of points associated to core achievements.
    /// </summary>
    int GetTotalPoints() const { return GetValue(TotalPointsProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the whether processing is active.
    /// </summary>
    static const BoolModelProperty IsProcessingActiveProperty;

    /// <summary>
    /// Gets whether processing is active.
    /// </summary>
    bool IsProcessingActive() const { return GetValue(IsProcessingActiveProperty); }

    /// <summary>
    /// Sets whether processing is active.
    /// </summary>
    void SetProcessingActive(bool bValue) { SetValue(IsProcessingActiveProperty, bValue); }

    /// <summary>
    /// Gets the list of asset states.
    /// </summary>
    const LookupItemViewModelCollection& States() const noexcept
    {
        return m_vStates;
    }

    /// <summary>
    /// Gets the list of asset categories.
    /// </summary>
    const LookupItemViewModelCollection& Categories() const noexcept
    {
        return m_vCategories;
    }

    /// <summary>
    /// Gets the list of asset modification states.
    /// </summary>
    const LookupItemViewModelCollection& Changes() const noexcept
    {
        return m_vChanges;
    }

    static const StringModelProperty ActivateButtonTextProperty;
    const std::wstring& GetActivateButtonText() const { return GetValue(ActivateButtonTextProperty); }
    void ActivateSelected();

    static const StringModelProperty SaveButtonTextProperty;
    const std::wstring& GetSaveButtonText() const { return GetValue(SaveButtonTextProperty); }
    static const BoolModelProperty CanSaveProperty;
    bool CanSave() const { return GetValue(CanSaveProperty); }
    void SaveSelected();

    static const StringModelProperty PublishButtonTextProperty;
    const std::wstring& GetPublishButtonText() const { return GetValue(PublishButtonTextProperty); }
    static const BoolModelProperty CanPublishProperty;
    bool CanPublish() const { return GetValue(CanPublishProperty); }
    void PublishSelected();

    static const StringModelProperty RefreshButtonTextProperty;
    const std::wstring& GetRefreshButtonText() const { return GetValue(RefreshButtonTextProperty); }
    static const BoolModelProperty CanRefreshProperty;
    bool CanRefresh() const { return GetValue(CanRefreshProperty); }
    void RefreshSelected();

    static const StringModelProperty RevertButtonTextProperty;
    const std::wstring& GetRevertButtonText() const { return GetValue(RevertButtonTextProperty); }
    static const BoolModelProperty CanRevertProperty;
    bool CanRevert() const { return GetValue(CanRevertProperty); }
    void RevertSelected();

    void CreateNew() noexcept;

    static const BoolModelProperty CanCloneProperty;
    bool CanClone() const { return GetValue(CanCloneProperty); }
    void CloneSelected();

    void DoFrame();

    ViewModelCollection<AssetViewModelBase>& Assets() noexcept { return m_vAssets; }
    const ViewModelCollection<AssetViewModelBase>& Assets() const noexcept { return m_vAssets; }

    AchievementViewModel* FindAchievement(ra::AchievementID nId)
    {
        return dynamic_cast<AchievementViewModel*>(FindAsset(AssetType::Achievement, nId));
    }
    const AchievementViewModel* FindAchievement(ra::AchievementID nId) const
    {
        return dynamic_cast<const AchievementViewModel*>(FindAsset(AssetType::Achievement, nId));
    }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the filter category.
    /// </summary>
    static const IntModelProperty FilterCategoryProperty;

    /// <summary>
    /// Gets the filter category.
    /// </summary>
    AssetCategory GetFilterCategory() const { return ra::itoe<AssetCategory>(GetValue(FilterCategoryProperty)); }

    /// <summary>
    /// Sets the filter category.
    /// </summary>
    void SetFilterCategory(AssetCategory nValue) { SetValue(FilterCategoryProperty, ra::etoi(nValue)); }

    class AssetSummaryViewModel : public LookupItemViewModel
    {
    public:
        AssetType GetType() const { return ra::itoe<AssetType>(GetValue(AssetViewModelBase::TypeProperty)); }
        void SetType(AssetType nValue) { SetValue(AssetViewModelBase::TypeProperty, ra::etoi(nValue)); }

        AssetCategory GetCategory() const { return ra::itoe<AssetCategory>(GetValue(AssetViewModelBase::CategoryProperty)); }
        void SetCategory(AssetCategory nValue) { SetValue(AssetViewModelBase::CategoryProperty, ra::etoi(nValue)); }

        int GetPoints() const { return GetValue(AchievementViewModel::PointsProperty); }
        void SetPoints(int nValue) { SetValue(AchievementViewModel::PointsProperty, nValue); }

        AssetState GetState() const { return ra::itoe<AssetState>(GetValue(AssetViewModelBase::StateProperty)); }
        void SetState(AssetState nValue) { SetValue(AssetViewModelBase::StateProperty, ra::etoi(nValue)); }

        AssetChanges GetChanges() const { return ra::itoe<AssetChanges>(GetValue(AssetViewModelBase::ChangesProperty)); }
        void SetChanges(AssetChanges nValue) { SetValue(AssetViewModelBase::ChangesProperty, ra::etoi(nValue)); }
    };

    ViewModelCollection<AssetSummaryViewModel>& FilteredAssets() noexcept { return m_vFilteredAssets; }
    const ViewModelCollection<AssetSummaryViewModel>& FilteredAssets() const noexcept { return m_vFilteredAssets; }

    void OpenEditor(const AssetSummaryViewModel* pAsset);

private:
    // ViewModelCollectionBase::NotifyTarget
    void OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) override;
    void OnViewModelStringValueChanged(gsl::index nIndex, const StringModelProperty::ChangeArgs& args) override;
    void OnViewModelAdded(gsl::index nIndex) override;
    void OnViewModelRemoved(gsl::index nIndex) override;
    void OnViewModelChanged(gsl::index nIndex) override;
    void OnEndViewModelCollectionUpdate() override;

    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

    class FilteredListMonitor : public ViewModelCollectionBase::NotifyTarget
    {
    public:
        AssetListViewModel *m_pOwner = nullptr;

        void OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;
    };
    FilteredListMonitor m_pFilteredListMonitor;

    AssetViewModelBase* FindAsset(AssetType nType, ra::AchievementID nId);
    const AssetViewModelBase* FindAsset(AssetType nType, ra::AchievementID nId) const;

    bool HasSelection(AssetType nAssetType) const;
    void GetSelectedAssets(std::vector<AssetViewModelBase*>& vSelectedAssets);

    void UpdateTotals();
    void UpdateButtons();
    void DoUpdateButtons();
    std::atomic_bool m_bNeedToUpdateButtons = false;

    bool MatchesFilter(const AssetViewModelBase& pAsset);
    void AddOrRemoveFilteredItem(const AssetViewModelBase& pAsset);
    gsl::index GetFilteredAssetIndex(const AssetViewModelBase& pAsset) const;
    void ApplyFilter();

    ViewModelCollection<AssetViewModelBase> m_vAssets;
    ViewModelCollection<AssetSummaryViewModel> m_vFilteredAssets;

    LookupItemViewModelCollection m_vStates;
    LookupItemViewModelCollection m_vCategories;
    LookupItemViewModelCollection m_vChanges;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif RA_UI_ASSETLIST_VIEW_MODEL_H
