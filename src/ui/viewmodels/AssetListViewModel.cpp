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
const IntModelProperty AssetListViewModel::FilterCategoryProperty("AssetListViewModel", "FilterCategory", ra::etoi(AssetCategory::Core));

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
    else if (args.Property == AssetViewModelBase::CategoryProperty)
    {
        UpdateTotals();

        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr)
            AddOrRemoveFilteredItem(*pItem);
    }
}

void AssetListViewModel::OnViewModelAdded(_UNUSED gsl::index nIndex)
{
    if (!m_vAssets.IsUpdating())
    {
        UpdateTotals();

        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr)
            AddOrRemoveFilteredItem(*pItem);
    }
}

void AssetListViewModel::OnViewModelRemoved(_UNUSED gsl::index nIndex)
{
    if (!m_vAssets.IsUpdating())
    {
        UpdateTotals();

        // the item has already been removed from the vAssets collection, and all we know about it
        // is the index where it was located. scan through the vFilteredAssets collection and remove 
        // any that no longer exist in the vAssets collection.
        for (gsl::index nFilteredIndex = 0; nFilteredIndex < gsl::narrow_cast<gsl::index>(m_vFilteredAssets.Count()); ++nFilteredIndex)
        {
            auto* pItem = m_vFilteredAssets.GetItemAt(nIndex);
            if (pItem != nullptr)
            {
                if (!FindAsset(pItem->GetType(), pItem->GetId()))
                {
                    m_vFilteredAssets.RemoveAt(nIndex);
                    break;
                }
            }
        }
    }
}

void AssetListViewModel::OnViewModelChanged(_UNUSED gsl::index nIndex)
{
    if (!m_vAssets.IsUpdating())
    {
        UpdateTotals();

        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr)
            AddOrRemoveFilteredItem(*pItem);
    }
}

void AssetListViewModel::OnEndViewModelCollectionUpdate()
{
    UpdateTotals();
    ApplyFilter();
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

void AssetListViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == FilterCategoryProperty)
        ApplyFilter();
}

void AssetListViewModel::ApplyFilter()
{
    m_vFilteredAssets.BeginUpdate();

    // first pass: remove any filtered items no longer in the source collection
    for (gsl::index nIndex = gsl::narrow_cast<gsl::index>(m_vFilteredAssets.Count()) -1; nIndex >= 0; --nIndex)
    {
        auto* pItem = m_vFilteredAssets.GetItemAt(nIndex);
        if (pItem != nullptr)
        {
            if (!FindAsset(pItem->GetType(), pItem->GetId()))
                m_vFilteredAssets.RemoveAt(nIndex);
        }
    }

    // second pass: update visibility of each item in the source collection
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vAssets.Count()); ++nIndex)
    {
        auto* pItem = m_vAssets.GetItemAt(nIndex);
        if (pItem != nullptr)
            AddOrRemoveFilteredItem(*pItem);
    }

    m_vFilteredAssets.EndUpdate();
}

bool AssetListViewModel::MatchesFilter(const AssetViewModelBase& pAsset)
{
    if (pAsset.GetCategory() != GetFilterCategory())
        return false;

    return true;
}

void AssetListViewModel::AddOrRemoveFilteredItem(const AssetViewModelBase& pAsset)
{
    const auto nIndex = GetFilteredAssetIndex(pAsset);

    if (MatchesFilter(pAsset))
    {
        if (nIndex < 0)
        {
            auto pSummary = std::make_unique<AssetSummaryViewModel>();
            pSummary->SetId(ra::to_signed(pAsset.GetID()));
            pSummary->SetLabel(pAsset.GetName());
            pSummary->SetType(pAsset.GetType());
            pSummary->SetCategory(pAsset.GetCategory());
            pSummary->SetChanges(pAsset.GetChanges());

            const auto* pAchievement = dynamic_cast<const AchievementViewModel*>(&pAsset);
            if (pAchievement != nullptr)
                pSummary->SetPoints(pAchievement->GetPoints());

            m_vFilteredAssets.Append(std::move(pSummary));
        }
    }
    else
    {
        if (nIndex >= 0)
            m_vFilteredAssets.RemoveAt(nIndex);
    }
}

gsl::index AssetListViewModel::GetFilteredAssetIndex(const AssetViewModelBase& pAsset) const
{
    const auto nId = ra::to_signed(pAsset.GetID());
    const auto nType = pAsset.GetType();

    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vFilteredAssets.Count()); ++nIndex)
    {
        auto* pItem = m_vFilteredAssets.GetItemAt(nIndex);
        if (pItem != nullptr && pItem->GetId() == nId && pItem->GetType() == nType)
            return nIndex;
    }

    return -1;
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
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(m_vFilteredAssets.Count()); ++nIndex)
    {
        const auto* pItem = m_vFilteredAssets.GetItemAt(nIndex);
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

        if (bHasSelection)
        {
            const auto nFilteredIndex = GetFilteredAssetIndex(*pItem);
            if (nFilteredIndex >= 0 && !m_vFilteredAssets.GetItemAt(nFilteredIndex)->IsSelected())
            {
                // TODO: still need to write local version even if it's not selected,
                // even if it hasn't been modified
                continue;
            }
        }

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
