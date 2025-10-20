#include "GameAssets.hh"

#include "Exports.hh"
#include "RA_Log.h"

#include "GameContext.hh"

#include "data\models\LocalBadgesModel.hh"
#include "data\models\RichPresenceModel.hh"

#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace data {
namespace context {

ra::data::models::AssetModelBase* GameAssets::FindAsset(ra::data::models::AssetType nType, uint32_t nId)
{
    for (auto& pAsset : *this)
    {
        if (pAsset.GetID() == nId && pAsset.GetType() == nType)
            return &pAsset;
    }

    return nullptr;
}

const ra::data::models::AssetModelBase* GameAssets::FindAsset(ra::data::models::AssetType nType, uint32_t nId) const
{
    for (const auto& pAsset : *this)
    {
        if (pAsset.GetID() == nId && pAsset.GetType() == nType)
            return &pAsset;
    }

    return nullptr;
}

ra::data::models::AchievementModel& GameAssets::NewAchievement()
{
    auto vmAchievement = std::make_unique<ra::data::models::AchievementModel>();
    vmAchievement->SetID(m_nNextLocalId++);
    vmAchievement->SetPoints(0);
    vmAchievement->SetNew();
    vmAchievement->CreateServerCheckpoint();
    vmAchievement->CreateLocalCheckpoint();

    return dynamic_cast<ra::data::models::AchievementModel&>(AddItem(std::move(vmAchievement)));
}

ra::data::models::LeaderboardModel& GameAssets::NewLeaderboard()
{
    auto vmLeaderboard = std::make_unique<ra::data::models::LeaderboardModel>();
    vmLeaderboard->SetID(m_nNextLocalId++);
    vmLeaderboard->SetNew();
    vmLeaderboard->CreateServerCheckpoint();
    vmLeaderboard->CreateLocalCheckpoint();

    return dynamic_cast<ra::data::models::LeaderboardModel&>(AddItem(std::move(vmLeaderboard)));
}

ra::data::models::AssetCategory GameAssets::MostPublishedAssetCategory() const
{
    bool bHasLocalAssets = false;
    bool bHasUnpublishedAssets = false;

    for (const auto& pAsset : *this)
    {
        // we really only care about published achievements and
        // leaderboards. if a set only has published rich presence
        // or code notes, don't consider it a published set.
        switch (pAsset.GetType())
        {
            case ra::data::models::AssetType::Achievement:
            case ra::data::models::AssetType::Leaderboard:
                break;

            default:
                continue;
        }

        switch (pAsset.GetCategory())
        {
            case ra::data::models::AssetCategory::Local:
                bHasLocalAssets = true;
                break;

            case ra::data::models::AssetCategory::Unofficial:
                bHasUnpublishedAssets = true;
                break;

            default:
                // Core, Bonus, or something else that's been published
                return ra::data::models::AssetCategory::Core;
        }
    }

    if (bHasUnpublishedAssets)
        return ra::data::models::AssetCategory::Unofficial;

    if (bHasLocalAssets)
        return ra::data::models::AssetCategory::Local;

    return ra::data::models::AssetCategory::None;
}

void GameAssets::OnBeforeItemRemoved(ModelBase& pModel)
{
    auto* pLocalBadges = dynamic_cast<ra::data::models::LocalBadgesModel*>(FindAsset(ra::data::models::AssetType::LocalBadges, 0));
    if (pLocalBadges)
    {
        const auto* pAchievement = dynamic_cast<const ra::data::models::AchievementModel*>(&pModel);
        if (pAchievement && ra::StringStartsWith(pAchievement->GetBadge(), L"local\\"))
            pLocalBadges->RemoveReference(pAchievement->GetBadge(), pAchievement->IsBadgeCommitted());
    }

    ra::data::DataModelCollection<ra::data::models::AssetModelBase>::OnBeforeItemRemoved(pModel);
}

void GameAssets::ReloadAssets(const std::vector<ra::data::models::AssetModelBase*>& vAssetsToReload)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    if (pGameContext.Subsets().empty()) // server assets haven't been loaded yet
        return;

    const auto nPrimarySubsetId = pGameContext.Subsets().front().AchievementSetID();

    auto* pRichPresence = dynamic_cast<ra::data::models::RichPresenceModel*>(FindAsset(ra::data::models::AssetType::RichPresence, 0));
    if (pRichPresence != nullptr)
        pRichPresence->ReloadRichPresenceScript();

    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pData = pLocalStorage.ReadText(ra::services::StorageItemType::UserAchievements, std::to_wstring(pGameContext.ActiveGameId()));
    if (pData == nullptr)
    {
        // no local file found. reset non-local items to their server state
        BeginUpdate();
        for (auto* pAsset : vAssetsToReload)
        {
            if (pAsset != nullptr && pAsset->GetCategory() != ra::data::models::AssetCategory::Local)
                pAsset->RestoreServerCheckpoint();
        }
        EndUpdate();

        return;
    }

    std::vector<ra::data::models::AssetModelBase*> vRemainingAssetsToReload(vAssetsToReload);
    const bool bReloadAll = vAssetsToReload.empty();
    if (bReloadAll)
    {
        // when reloading all, populate vRemainingAssetsToReload with all local assets
        // or modified assets so they'll be discarded if no longer in the local file
        for (auto& pAsset : *this)
        {
            if (pAsset.GetChanges() != ra::data::models::AssetChanges::None ||
                pAsset.GetCategory() == ra::data::models::AssetCategory::Local)
            {
                switch (pAsset.GetType())
                {
                    // ignore LocalBadges container
                    case ra::data::models::AssetType::LocalBadges:
                        continue;

                    // ignore RichPresence model (it's not actually stored in the XXX-User file)
                    case ra::data::models::AssetType::RichPresence:
                        continue;

                    // ignore CodeNotes model (it's actually a collection of notes)
                    case ra::data::models::AssetType::CodeNotes:
                        continue;
                }

                vRemainingAssetsToReload.push_back(&pAsset);
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
        unsigned nSubsetId = 0;

        ra::Tokenizer pTokenizer(sLine);
        switch (pTokenizer.PeekChar())
        {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                // achievements start with a number (no prefix)
                nType = ra::data::models::AssetType::Achievement;
                break;

            case 'L':
                nType = ra::data::models::AssetType::Leaderboard;
                pTokenizer.Consume('L');
                break;

            case 'N':
                nType = ra::data::models::AssetType::CodeNotes;
                pTokenizer.Consume('N');
                break;

            default:
                continue;
        }

        nId = pTokenizer.ReadNumber();
        nSubsetId = pTokenizer.Consume('|') ? pTokenizer.ReadNumber() : nPrimarySubsetId;

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
                if (pAssetToReset != nullptr && pAssetToReset->GetID() == nId && pAssetToReset->GetType() == nType)
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
                    auto mAchievement = std::make_unique<ra::data::models::AchievementModel>();
                    mAchievement->SetID(nId);
                    mAchievement->SetCategory(ra::data::models::AssetCategory::Local);
                    mAchievement->CreateServerCheckpoint();

                    pAsset = &Append(std::move(mAchievement));
                    break;
                }

                case ra::data::models::AssetType::Leaderboard:
                {
                    auto mLeaderboard = std::make_unique<ra::data::models::LeaderboardModel>();
                    mLeaderboard->SetID(nId);
                    mLeaderboard->SetCategory(ra::data::models::AssetCategory::Local);
                    mLeaderboard->CreateServerCheckpoint();

                    pAsset = &Append(std::move(mLeaderboard));
                    break;
                }

                case ra::data::models::AssetType::CodeNotes:
                    pAsset = FindCodeNotes();
                    if (pAsset)
                        pAsset->Deserialize(pTokenizer);

                    continue;
            }

            if (pAsset)
            {
                pAsset->DeserializeLocalCheckpoint(pTokenizer);

                if (nId == 0)
                    vUnnumberedAssets.push_back(pAsset);
            }
        }

        if (pAsset)
            pAsset->SetSubsetID(nSubsetId);
    }

    // assign IDs for any assets where one was not available
    for (auto* pAsset : vUnnumberedAssets)
    {
        if (pAsset != nullptr)
            pAsset->SetID(m_nNextLocalId++);
    }

    // any items still in the source list no longer exist in the file. if the item is on the server,
    // just reset to the server state. otherwise, delete the item.
    for (auto* pAsset : vRemainingAssetsToReload)
    {
        if (pAsset == nullptr)
            continue;

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
    auto pData = pLocalStorage.WriteText(ra::services::StorageItemType::UserAchievements, std::to_wstring(pGameContext.ActiveGameId()));
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

    uint32_t nPrimarySubsetId = 0;
    if (pGameContext.Subsets().empty())
    {
        pData->WriteLine(pGameContext.GameTitle());
    }
    else
    {
        nPrimarySubsetId = pGameContext.Subsets().front().AchievementSetID();
        pData->WriteLine(pGameContext.Subsets().front().Title());
    }

    bool bHasDeleted = false;
    for (auto& pAsset : *this)
    {
        switch (pAsset.GetChanges())
        {
            case ra::data::models::AssetChanges::None:
                // non-modified committed items don't need to be serialized
                // ASSERT: unmodified local items should report as Unpublished
                continue;

            case ra::data::models::AssetChanges::Unpublished:
                // always write unpublished changes
                break;

            case ra::data::models::AssetChanges::Deleted:
                // never write deleted objects - we'll remove them after we update the file
                bHasDeleted = true;
                continue;

            default:
                // if the item is modified, check to see if it's selected
                bool bUpdate = vAssetsToSave.empty();
                if (!bUpdate)
                {
                    const auto nID = pAsset.GetID();
                    const auto nType = pAsset.GetType();
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
                    pAsset.UpdateLocalCheckpoint();

                    // reverted back to committed state, no need to serialize
                    if (pAsset.GetChanges() == ra::data::models::AssetChanges::None)
                        continue;
                }
                else if (pAsset.GetCategory() != ra::data::models::AssetCategory::Local)
                {
                    // if there's no unpublished changes and the item is not selected, ignore it
                    if (!pAsset.HasUnpublishedChanges())
                        continue;
                }
                else if (pAsset.GetChanges() == ra::data::models::AssetChanges::New)
                {
                    // unselected new item has no local state to maintain
                    continue;
                }
                break;
        }

        // serialize the item
        switch (pAsset.GetType())
        {
            case ra::data::models::AssetType::Achievement:
                break;

            case ra::data::models::AssetType::Leaderboard:
                pData->Write("L");
                break;

            case ra::data::models::AssetType::CodeNotes:
                pData->Write("N");
                break;

            case ra::data::models::AssetType::RichPresence:
                continue;
        }

        pData->Write(std::to_string(pAsset.GetID()));

        const auto nSubsetId = pAsset.GetSubsetID();
        if (nSubsetId > 0 && nSubsetId != nPrimarySubsetId)
        {
            pData->Write("|");
            pData->Write(std::to_string(nSubsetId));
        }

        pAsset.Serialize(*pData);
        pData->WriteLine();
    }

    RA_LOG_INFO("Wrote user assets file");

    if (bHasDeleted)
    {
        for (gsl::index nIndex = gsl::narrow_cast<gsl::index>(Count()) - 1; nIndex >= 0; --nIndex)
        {
            auto* pItem = GetItemAt(nIndex);
            if (pItem != nullptr && pItem->GetChanges() == ra::data::models::AssetChanges::Deleted)
                RemoveAt(nIndex);
        }
    }
}

} // namespace context
} // namespace data
} // namespace ra
