#include "AchievementSetModel.hh"

#include "context/IRcClient.hh"

#include "data/models/GameAssets.hh"

#include "util/Strings.hh"

#include <rcheevos/src/rc_client_internal.h>

namespace ra {
namespace data {
namespace models {

const IntModelProperty AchievementSetModel::TypeProperty("AchievementSetModel", "Type", ra::etoi(AchievementSetType::Core));
const IntModelProperty AchievementSetModel::IDProperty("AchievementSetModel", "ID", 0);
const IntModelProperty AchievementSetModel::BackingGameIDProperty("AchievementSetModel", "BackingGameID", 0);
const StringModelProperty AchievementSetModel::TitleProperty("AchievementSetModel", "Name", L"");

struct AchievementSetModel::SubsetInfo
{
    rc_client_subset_info_t* oPublishedSubsetInfo {}; // the actual published data
    rc_client_subset_info_t oPublishedRuntimeInfo {}; // the possibly modified published data
    rc_client_subset_info_t oLocalRuntimeInfo {};     // the local only data
    std::vector<rc_client_achievement_info_t> vPublishedAchievements;
    std::vector<rc_client_achievement_info_t> vLocalAchievements;
    std::vector<rc_client_leaderboard_info_t> vPublishedLeaderboards;
    std::vector<rc_client_leaderboard_info_t> vLocalLeaderboards;
};

AchievementSetModel::AchievementSetModel() noexcept
{
    // must be declared here so unique_ptr can correctly free the forwrd declared SubsetInfo object
}

AchievementSetModel::~AchievementSetModel()
{
    // must be declared here so unique_ptr can correctly free the forward declared SubsetInfo object
}

void AchievementSetModel::Initialize(uint32_t nId, uint32_t nBackingGameId, const std::wstring& sName, AchievementSetType nType)
{
    SetValue(IDProperty, nId);
    SetValue(BackingGameIDProperty, nBackingGameId);
    SetValue(TitleProperty, sName);
    SetValue(TypeProperty, nType);
}

static void SyncSubset(rc_client_subset_info_t& pSubsetInfo,
    std::vector<rc_client_achievement_info_t>& vAchievements,
    std::vector<rc_client_leaderboard_info_t>& vLeaderboards,
    std::vector<ra::data::models::AchievementModel*> vAchievementModels,
    std::vector<ra::data::models::LeaderboardModel*> vLeaderboardModels)
{
    bool bChanged = vAchievements.size() != vAchievementModels.size();
    if (!bChanged)
    {
        for (size_t i = 0; i < vAchievementModels.size(); ++i)
        {
            if (vAchievements.at(i).trigger != vAchievementModels.at(i)->GetRuntimeTrigger())
            {
                bChanged = true;
                break;
            }
        }
    }

    if (bChanged)
    {
        std::vector<rc_client_achievement_info_t> vNewAchievements;

        if (!vAchievementModels.empty())
        {
            vNewAchievements.resize(vAchievementModels.size());
            rc_client_achievement_info_t* pDst = &vNewAchievements.at(0);

            size_t nFirstUnmatched = 0;
            for (auto* pAchievementModel : vAchievementModels)
            {
                const auto nId = pAchievementModel->GetID();

                const rc_client_achievement_info_t* pAchievementInfo = pAchievementModel->GetRuntimeAchievementInfo();
                if (pAchievementInfo == nullptr)
                {
                    for (size_t i = nFirstUnmatched; i < pSubsetInfo.public_.num_achievements; ++i) {
                        if (pSubsetInfo.achievements[i].public_.id == nId) {
                            pAchievementInfo = &pSubsetInfo.achievements[i];
                            if (i == nFirstUnmatched)
                                ++nFirstUnmatched;
                            break;
                        }
                    }
                }

                // if a source was found, copy from it
                if (pAchievementInfo != nullptr)
                    memcpy(pDst, pAchievementInfo, sizeof(*pDst));

                // point at the local copy
                pAchievementModel->SetLocalAchievementInfo(*pDst);

                // if the record differs from the server value, incorporate any local changes
                if (pDst->public_.id != nId || pAchievementModel->GetChanges() != AssetChanges::None)
                    pAchievementModel->SyncToLocalAchievementInfo();

                ++pDst;
            }
        }

        vAchievements.swap(vNewAchievements);
        pSubsetInfo.achievements = vAchievements.data();
        pSubsetInfo.public_.num_achievements = gsl::narrow_cast<uint32_t>(vAchievements.size());
    }

    bChanged = vLeaderboards.size() != vLeaderboardModels.size();
    if (!bChanged)
    {
        for (size_t i = 0; i < vLeaderboardModels.size(); ++i)
        {
            if (vLeaderboards.at(i).lboard != vLeaderboardModels.at(i)->GetRuntimeLeaderboard())
            {
                bChanged = true;
                break;
            }
        }
    }

    if (bChanged)
    {
        std::vector<rc_client_leaderboard_info_t> vNewLeaderboards;

        if (!vLeaderboardModels.empty())
        {
            vNewLeaderboards.resize(vLeaderboardModels.size());
            rc_client_leaderboard_info_t* pDst = &vNewLeaderboards.at(0);

            size_t nFirstUnmatched = 0;
            for (auto* pLeaderboardModel : vLeaderboardModels)
            {
                const auto nId = pLeaderboardModel->GetID();

                const rc_client_leaderboard_info_t* pLeaderboardInfo = pLeaderboardModel->GetRuntimeLeaderboardInfo();
                if (pLeaderboardInfo == nullptr)
                {
                    for (size_t i = nFirstUnmatched; i < pSubsetInfo.public_.num_leaderboards; ++i) {
                        if (pSubsetInfo.leaderboards[i].public_.id == nId) {
                            pLeaderboardInfo = &pSubsetInfo.leaderboards[i];
                            if (i == nFirstUnmatched)
                                ++nFirstUnmatched;
                            break;
                        }
                    }
                }

                // if a source was found, copy from it
                if (pLeaderboardInfo != nullptr)
                    memcpy(pDst, pLeaderboardInfo, sizeof(*pDst));

                // point at the local copy
                pLeaderboardModel->SetLocalLeaderboardInfo(*pDst);

                // if the record differs from the server value, incorporate any local changes
                if (pDst->public_.id != nId || pLeaderboardModel->GetChanges() != AssetChanges::None)
                    pLeaderboardModel->SyncToLocalLeaderboardInfo();

                ++pDst;
            }
        }

        vLeaderboards.swap(vNewLeaderboards);
        pSubsetInfo.leaderboards = vLeaderboards.data();
        pSubsetInfo.public_.num_leaderboards = gsl::narrow_cast<uint32_t>(vLeaderboards.size());
    }

    pSubsetInfo.active = !vAchievements.empty() || !vLeaderboards.empty();
}

void AchievementSetModel::SyncToRuntime(rc_client_subset_info_t& pSubset, GameAssets& pAssets)
{
    if (m_pInfo == nullptr)
    {
       m_pInfo = std::make_unique<SubsetInfo>();
       memcpy(&m_pInfo->oPublishedSubsetInfo, &pSubset, sizeof(pSubset));
       memcpy(&m_pInfo->oPublishedRuntimeInfo, &pSubset, sizeof(pSubset));
       memcpy(&m_pInfo->oLocalRuntimeInfo, &pSubset, sizeof(pSubset));
       m_pInfo->oLocalRuntimeInfo.public_.id = LocalId;
       m_pInfo->oLocalRuntimeInfo.public_.title = "Local";
       m_pInfo->oLocalRuntimeInfo.public_.num_achievements = 0;
       m_pInfo->oLocalRuntimeInfo.public_.num_leaderboards = 0;
       m_pInfo->oLocalRuntimeInfo.mastery = 0;
    }

    // find the assets that belong to this subset
    std::vector<ra::data::models::AchievementModel*> vCoreAchievements;
    std::vector<ra::data::models::AchievementModel*> vLocalAchievements;
    std::vector<ra::data::models::LeaderboardModel*> vCoreLeaderboards;
    std::vector<ra::data::models::LeaderboardModel*> vLocalLeaderboards;

    for (auto& pAsset : pAssets)
    {
        if (pAsset.GetSubsetID() != pSubset.public_.id)
            continue;

        if (pAsset.GetChanges() == ra::data::models::AssetChanges::Deleted)
            continue;

        auto* vmAchievement = dynamic_cast<ra::data::models::AchievementModel*>(&pAsset);
        if (vmAchievement != nullptr)
        {
            if (vmAchievement->GetCategory() == ra::data::models::AssetCategory::Local)
                vLocalAchievements.push_back(vmAchievement);
            else
                vCoreAchievements.push_back(vmAchievement);

            continue;
        }

        auto* vmLeaderboard = dynamic_cast<ra::data::models::LeaderboardModel*>(&pAsset);
        if (vmLeaderboard != nullptr)
        {
            if (vmLeaderboard->GetCategory() == ra::data::models::AssetCategory::Local)
                vLocalLeaderboards.push_back(vmLeaderboard);
            else
                vCoreLeaderboards.push_back(vmLeaderboard);

            continue;
        }
    }

    // sync the models into the runtime
    SyncSubset(m_pInfo->oPublishedRuntimeInfo,
               m_pInfo->vPublishedAchievements, m_pInfo->vPublishedLeaderboards,
               vCoreAchievements, vCoreLeaderboards);

    SyncSubset(m_pInfo->oLocalRuntimeInfo,
               m_pInfo->vLocalAchievements, m_pInfo->vLocalLeaderboards,
               vLocalAchievements, vLocalLeaderboards);
}

rc_client_subset_info_t* AchievementSetModel::GetPublishedSubsetInfo() const
{
    return m_pInfo ? &m_pInfo->oPublishedRuntimeInfo : nullptr;
}

rc_client_subset_info_t* AchievementSetModel::GetLocalSubsetInfo() const
{
    return m_pInfo ? &m_pInfo->oLocalRuntimeInfo : nullptr;
}

} // namespace models
} // namespace data
} // namespace ra
