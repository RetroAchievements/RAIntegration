#include "GameAssets.hh"

#include "Exports.hh"
#include "RA_Log.h"

#include "GameContext.hh"

#include "data\models\LocalBadgesModel.hh"

#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace data {
namespace context {

ra::data::models::AssetModelBase* GameAssets::FindAsset(ra::data::models::AssetType nType, uint32_t nId)
{
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(Count()); ++nIndex)
    {
        auto* pAsset = GetItemAt(nIndex);
        if (pAsset != nullptr && pAsset->GetID() == nId && pAsset->GetType() == nType)
            return pAsset;
    }

    return nullptr;
}

const ra::data::models::AssetModelBase* GameAssets::FindAsset(ra::data::models::AssetType nType, uint32_t nId) const
{
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(Count()); ++nIndex)
    {
        auto* pAsset = GetItemAt(nIndex);
        if (pAsset != nullptr && pAsset->GetID() == nId && pAsset->GetType() == nType)
            return pAsset;
    }

    return nullptr;
}

ra::data::models::AchievementModel& GameAssets::NewAchievement()
{
    auto vmAchievement = std::make_unique<ra::data::models::AchievementModel>();
    vmAchievement->SetID(m_nNextLocalId++);
    vmAchievement->SetNew();
    vmAchievement->CreateServerCheckpoint();
    vmAchievement->CreateLocalCheckpoint();

    return dynamic_cast<ra::data::models::AchievementModel&>(AddItem(std::move(vmAchievement)));
}

void GameAssets::OnItemsAdded(const std::vector<gsl::index>& vNewIndices)
{
    auto* pLocalBadges = dynamic_cast<ra::data::models::LocalBadgesModel*>(FindAsset(ra::data::models::AssetType::LocalBadges, 0));
    if (pLocalBadges)
    {
        for (gsl::index nIndex : vNewIndices)
        {
            const auto* pAsset = GetItemAt(nIndex);
            const auto* pAchievement = dynamic_cast<const ra::data::models::AchievementModel*>(pAsset);
            if (pAchievement && ra::StringStartsWith(pAchievement->GetBadge(), L"local\\"))
                pLocalBadges->AddReference(pAchievement->GetBadge(), pAchievement->IsBadgeCommitted());
        }
    }

    ra::data::DataModelCollection<ra::data::models::AssetModelBase>::OnItemsAdded(vNewIndices);
}

void GameAssets::OnItemsRemoved(const std::vector<gsl::index>& vDeletedIndices)
{
    auto* pLocalBadges = dynamic_cast<ra::data::models::LocalBadgesModel*>(FindAsset(ra::data::models::AssetType::LocalBadges, 0));
    if (pLocalBadges)
    {
        for (gsl::index nIndex : vDeletedIndices)
        {
            const auto* pAsset = GetItemAt(nIndex);
            const auto* pAchievement = dynamic_cast<const ra::data::models::AchievementModel*>(pAsset);
            if (pAchievement && ra::StringStartsWith(pAchievement->GetBadge(), L"local\\"))
                pLocalBadges->RemoveReference(pAchievement->GetBadge(), pAchievement->IsBadgeCommitted());
        }
    }

    ra::data::DataModelCollection<ra::data::models::AssetModelBase>::OnItemsRemoved(vDeletedIndices);
}

void GameAssets::ReloadAssets(const std::vector<ra::data::models::AssetModelBase*>& vAssetsToReload)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();

    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pData = pLocalStorage.ReadText(ra::services::StorageItemType::UserAchievements, std::to_wstring(pGameContext.GameId()));
    if (pData == nullptr)
        return;

    std::vector<ra::data::models::AssetModelBase*> vRemainingAssetsToReload(vAssetsToReload);
    const bool bReloadAll = vAssetsToReload.empty();
    if (bReloadAll)
    {
        // when reloading all, populate vRemainingAssetsToReload with all local assets
        // or modified assets so they'll be discarded if no longer in the local file
        for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(Count()); ++nIndex)
        {
            auto* pAsset = GetItemAt(nIndex);
            if (pAsset)
            {
                if (pAsset->GetChanges() != ra::data::models::AssetChanges::None ||
                    pAsset->GetCategory() == ra::data::models::AssetCategory::Local)
                {
                    vRemainingAssetsToReload.push_back(pAsset);
                }
            }
        }
    }

    std::string sLine;
    pData->GetLine(sLine); // version used to create the file
    pData->GetLine(sLine); // game title

    BeginUpdate();

    std::vector<ra::data::models::AssetModelBase*> vUnnumberedAssets;
    while (pData->GetLine(sLine))
    {
        ra::data::models::AssetType nType = ra::data::models::AssetType::None;
        unsigned nId = 0;

        ra::Tokenizer pTokenizer(sLine);
        switch (pTokenizer.PeekChar())
        {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                // achievements start with a number (no prefix)
                nType = ra::data::models::AssetType::Achievement;
                nId = pTokenizer.ReadNumber();
                break;
        }

        if (nType == ra::data::models::AssetType::None)
            continue;
        if (!pTokenizer.Consume(':'))
            continue;

        // find the asset to merge
        ra::data::models::AssetModelBase* pAsset = nullptr;
        if (nId != 0)
        {
            if (nId >= m_nNextLocalId)
                m_nNextLocalId = nId + 1;

            for (auto pIter = vRemainingAssetsToReload.begin(); pIter != vRemainingAssetsToReload.end(); ++pIter)
            {
                auto* pAssetToReset = *pIter;
                if (pAssetToReset->GetID() == nId && pAssetToReset->GetType() == nType)
                {
                    pAsset = pAssetToReset;
                    vRemainingAssetsToReload.erase(pIter);
                    break;
                }
            }

            // when reloading all, if the asset wasn't modified it won't be in vRemainingAssetsToReload
            // but we still want to merge the item. see if it exists at all.
            if (!pAsset && bReloadAll && nId < FirstLocalId)
                pAsset = FindAsset(nType, nId);
        }

        if (pAsset)
        {
            // merge the data from the file
            pAsset->ResetLocalCheckpoint(pTokenizer);
        }
        else if (bReloadAll)
        {
            // non-existant item, only process it if reloading all
            switch (nType)
            {
                case ra::data::models::AssetType::Achievement:
                {
                    auto vmAchievement = std::make_unique<ra::data::models::AchievementModel>();
                    vmAchievement->SetID(nId);
                    vmAchievement->SetCategory(ra::data::models::AssetCategory::Local);
                    vmAchievement->CreateServerCheckpoint();

                    pAsset = &Append(std::move(vmAchievement));
                    break;
                }
            }

            if (pAsset)
            {
                pAsset->Deserialize(pTokenizer);
                pAsset->CreateLocalCheckpoint();

                if (nId == 0)
                    vUnnumberedAssets.push_back(pAsset);
            }
        }
    }

    // assign IDs for any assets where one was not available
    for (auto* pAsset : vUnnumberedAssets)
        pAsset->SetID(m_nNextLocalId++);

    // any items still in the source list no longer exist in the file. if the item is on the server,
    // just reset to the server state. otherwise, delete the item.
    for (auto* pAsset : vRemainingAssetsToReload)
    {
        if (pAsset->GetCategory() == ra::data::models::AssetCategory::Local)
        {
            for (gsl::index nIndex = gsl::narrow_cast<gsl::index>(Count()) - 1; nIndex >= 0; --nIndex)
            {
                if (GetItemAt(nIndex) == pAsset)
                {
                    RemoveAt(nIndex);
                    break;
                }
            }
        }
        else
        {
            pAsset->RestoreServerCheckpoint();
        }
    }

    EndUpdate();
}

void GameAssets::SaveAssets(const std::vector<ra::data::models::AssetModelBase*>& vAssetsToSave)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pData = pLocalStorage.WriteText(ra::services::StorageItemType::UserAchievements, std::to_wstring(pGameContext.GameId()));
    if (pData == nullptr)
    {
        RA_LOG_ERR("Failed to create user assets file");
        return;
    }

#ifdef RA_UTEST
    pData->WriteLine("0.0.0.0");
#else
    pData->WriteLine(_RA_IntegrationVersion()); // version used to create the file
#endif

    pData->WriteLine(pGameContext.GameTitle());

    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(Count()); ++nIndex)
    {
        auto* pItem = GetItemAt(nIndex);
        if (pItem == nullptr)
            continue;

        switch (pItem->GetChanges())
        {
            case ra::data::models::AssetChanges::None:
                // non-modified committed items don't need to be serialized
                // ASSERT: unmodified local items should report as Unpublished
                continue;

            case ra::data::models::AssetChanges::Unpublished:
                // always write unpublished changes
                break;

            default:
                // if the item is modified, check to see if it's selected
                bool bUpdate = vAssetsToSave.empty();
                if (!bUpdate)
                {
                    const auto nID = pItem->GetID();
                    const auto nType = pItem->GetType();
                    for (const auto& pScan : vAssetsToSave)
                    {
                        if (pScan->GetID() == nID && pScan->GetType() == nType)
                        {
                            bUpdate = true;
                            break;
                        }
                    }
                }

                // if it's selected for update, commit the changes before flushing
                if (bUpdate)
                {
                    pItem->UpdateLocalCheckpoint();

                    // reverted back to committed state, no need to serialize
                    if (pItem->GetChanges() == ra::data::models::AssetChanges::None)
                        continue;
                }
                else if (pItem->GetCategory() != ra::data::models::AssetCategory::Local)
                {
                    // if there's no unpublished changes and the item is not selected, ignore it
                    if (!pItem->HasUnpublishedChanges())
                        continue;
                }
                else if (pItem->GetChanges() == ra::data::models::AssetChanges::New)
                {
                    // unselected new item has no local state to maintain
                    continue;
                }
                break;
        }

        // serialize the item
        pData->Write(std::to_string(pItem->GetID()));
        pItem->Serialize(*pData);
        pData->WriteLine();
    }

    RA_LOG_INFO("Wrote user assets file");
}

} // namespace context
} // namespace data
} // namespace ra
