#include "AssetListViewModel.hh"

#include "data\GameContext.hh"

#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

#include "Exports.hh"
#include "RA_Log.h"

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty AssetListViewModel::GameIdProperty("AssetListViewModel", "GameId", 0);
const IntModelProperty AssetListViewModel::AchievementCountProperty("AssetListViewModel", "AchievementCount", 0);
const IntModelProperty AssetListViewModel::TotalPointsProperty("AssetListViewModel", "TotalPoints", 0);
const BoolModelProperty AssetListViewModel::IsProcessingActiveProperty("AssetListViewModel", "IsProcessingActive", true);
const StringModelProperty AssetListViewModel::ActivateButtonTextProperty("AssetListViewModel", "ActivateButtonText", L"_Activate All");
const StringModelProperty AssetListViewModel::SaveButtonTextProperty("AssetListViewModel", "SaveButtonText", L"_Save All");
const StringModelProperty AssetListViewModel::PublishButtonTextProperty("AssetListViewModel", "SaveButtonText", L"_Publish All");
const StringModelProperty AssetListViewModel::RefreshButtonTextProperty("AssetListViewModel", "SaveButtonText", L"_Refresh All");
const StringModelProperty AssetListViewModel::RevertButtonTextProperty("AssetListViewModel", "SaveButtonText", L"Re_vert All");
const BoolModelProperty AssetListViewModel::CanSaveProperty("AssetListViewModel", "CanSave", true);
const BoolModelProperty AssetListViewModel::CanPublishProperty("AssetListViewModel", "CanPublish", true);
const BoolModelProperty AssetListViewModel::CanRefreshProperty("AssetListViewModel", "CanRefresh", true);
const BoolModelProperty AssetListViewModel::CanCloneProperty("AssetListViewModel", "CanClone", true);

AssetListViewModel::AssetListViewModel() noexcept
{
    SetWindowTitle(L"Assets List");

    m_vStates.Add(ra::etoi(AssetState::Inactive), L"Inactive");
    m_vStates.Add(ra::etoi(AssetState::Waiting), L"Waiting");
    m_vStates.Add(ra::etoi(AssetState::Active), L"Active");
    m_vStates.Add(ra::etoi(AssetState::Paused), L"Paused");
    m_vStates.Add(ra::etoi(AssetState::Triggered), L"Triggered");

    m_vCategories.Add(ra::etoi(AssetCategory::Core), L"Core");
    m_vCategories.Add(ra::etoi(AssetCategory::Unofficial), L"Unofficial");
    m_vCategories.Add(ra::etoi(AssetCategory::Local), L"Local");

    m_vChanges.Add(ra::etoi(AssetChanges::None), L"");
    m_vChanges.Add(ra::etoi(AssetChanges::Modified), L"Modified");
    m_vChanges.Add(ra::etoi(AssetChanges::Unpublished), L"Unpublished");

    m_vAssets.AddNotifyTarget(*this);
}

static bool IsTalliedAchievement(const AchievementViewModel& pAchievement)
{
    return (pAchievement.GetCategory() == AssetCategory::Core);
}

void AssetListViewModel::OnViewModelIntValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == AchievementViewModel::PointsProperty)
    {
        auto* pAchievement = dynamic_cast<const AchievementViewModel*>(m_vAssets.GetItemAt(nIndex));
        if (pAchievement != nullptr && IsTalliedAchievement(*pAchievement))
        {
            auto nPoints = GetTotalPoints();
            nPoints += args.tNewValue - args.tOldValue;
            SetValue(TotalPointsProperty, nPoints);
        }
    }
}

void AssetListViewModel::OnViewModelAdded(_UNUSED gsl::index nIndex)
{
    if (!m_vAssets.IsUpdating())
        UpdateTotals();
}

void AssetListViewModel::OnViewModelRemoved(_UNUSED gsl::index nIndex)
{
    if (!m_vAssets.IsUpdating())
        UpdateTotals();
}

void AssetListViewModel::OnViewModelChanged(_UNUSED gsl::index nIndex)
{
    if (!m_vAssets.IsUpdating())
        UpdateTotals();
}

void AssetListViewModel::OnEndViewModelCollectionUpdate()
{
    UpdateTotals();
}

void AssetListViewModel::UpdateTotals()
{
    int nAchievementCount = 0;
    int nTotalPoints = 0;

    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vAssets.Count()); ++nIndex)
    {
        auto* pAchievement = dynamic_cast<const AchievementViewModel*>(m_vAssets.GetItemAt(nIndex));
        if (pAchievement != nullptr && IsTalliedAchievement(*pAchievement))
        {
            ++nAchievementCount;
            nTotalPoints += pAchievement->GetPoints();
        }
    }

    SetValue(AchievementCountProperty, nAchievementCount);
    SetValue(TotalPointsProperty, nTotalPoints);
}


AssetViewModelBase* AssetListViewModel::FindAsset(AssetType nType, ra::AchievementID nId)
{
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vAssets.Count()); ++nIndex)
    {
        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr && pItem->GetID() == nId && pItem->GetType() == nType)
            return pItem;
    }

    return nullptr;
}

const AssetViewModelBase* AssetListViewModel::FindAsset(AssetType nType, ra::AchievementID nId) const
{
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vAssets.Count()); ++nIndex)
    {
        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr && pItem->GetID() == nId && pItem->GetType() == nType)
            return pItem;
    }

    return nullptr;
}

bool AssetListViewModel::HasSelection(AssetType nAssetType) const
{
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vAssets.Count()); ++nIndex)
    {
        const auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr && pItem->IsSelected())
        {
            if (nAssetType == AssetType::None || nAssetType == pItem->GetType())
                return true;
        }
    }

    return false;
}

void AssetListViewModel::ActivateSelected() noexcept
{

}

void AssetListViewModel::SaveSelected()
{
    if (!CanSave())
        return;

    const bool bHasSelection = HasSelection(AssetType::Achievement);

    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pData = pLocalStorage.WriteText(ra::services::StorageItemType::UserAchievements, std::to_wstring(GetGameId()));
    if (pData == nullptr)
    {
        RA_LOG_ERR("Failed to create user assets file");
        return;
    }

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    pData->WriteLine(_RA_IntegrationVersion()); // version used to create the file
    pData->WriteLine(pGameContext.GameTitle());

    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vAssets.Count()); ++nIndex)
    {
        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem == nullptr)
            continue;

        if (pItem->GetChanges() == AssetChanges::None)
            continue;

        if (bHasSelection && !pItem->IsSelected())
            continue;

        // serialize the item
        pData->Write(std::to_string(pItem->GetID()));
        pItem->Serialize(*pData);
        pData->WriteLine();

        // reset the modified state
        pItem->UpdateLocalCheckpoint();
    }

    RA_LOG_INFO("Wrote user assets file");
}

void AssetListViewModel::PublishSelected()
{
    if (!CanPublish())
        return;
}

void AssetListViewModel::RefreshSelected()
{
    if (!CanRefresh())
        return;
}

void AssetListViewModel::RevertSelected() noexcept
{

}

void AssetListViewModel::CreateNew() noexcept
{

}

void AssetListViewModel::CloneSelected()
{
    if (!CanClone())
        return;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
