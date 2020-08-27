#ifndef RA_UI_ASSETLIST_VIEW_MODEL_H
#define RA_UI_ASSETLIST_VIEW_MODEL_H
#pragma once

#include "ui\ViewModelBase.hh"
#include "ui\ViewModelCollection.hh"

#include "AssetViewModelBase.hh"

#include "AchievementViewModel.hh"

#include "data\Types.hh"

namespace ra {
namespace ui {
namespace viewmodels {

class AssetListViewModel : public ViewModelBase
{
public:
    /// <summary>
    /// The <see cref="ModelProperty" /> for the game ID.
    /// </summary>
    static const IntModelProperty GameIdProperty;

    /// <summary>
    /// Gets the game ID.
    /// </summary>
    int GetGameId() const { return GetValue(GameIdProperty); }

    // <summary>
    /// The <see cref="ModelProperty" /> for the achievement count.
    /// </summary>
    static const IntModelProperty AchievementCountProperty;

    /// <summary>
    /// Gets the total number of filtered achievements.
    /// </summary>
    int GetAchievementCount() const { return GetValue(AchievementCountProperty); }

    // <summary>
    /// The <see cref="ModelProperty" /> for the total achievement points.
    /// </summary>
    static const IntModelProperty TotalPointsProperty;

    /// <summary>
    /// Gets the total number of points associated to filtered achievements.
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


    static const StringModelProperty ActivateButtonTextProperty;
    const std::wstring& GetActivateButtonText() const { return GetValue(ActivateButtonTextProperty); }
    void ActivateSelected() noexcept;

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
    void RevertSelected() noexcept;

    void CreateNew() noexcept;

    static const BoolModelProperty CanCloneProperty;
    bool CanClone() const { return GetValue(CanCloneProperty); }
    void CloneSelected();


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

private:
    AssetViewModelBase* FindAsset(AssetType nType, ra::AchievementID nId);
    const AssetViewModelBase* FindAsset(AssetType nType, ra::AchievementID nId) const;

    bool HasSelection(AssetType nAssetType) const;

    ViewModelCollection<AssetViewModelBase> m_vAssets;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif RA_UI_ASSETLIST_VIEW_MODEL_H
