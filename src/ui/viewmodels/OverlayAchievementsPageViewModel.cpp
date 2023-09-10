#include "OverlayAchievementsPageViewModel.hh"

#include "RA_StringUtils.h"

#include "api\FetchAchievementInfo.hh"

#include "data\context\GameContext.hh"
#include "data\context\SessionTracker.hh"
#include "data\context\UserContext.hh"

#include "services\IClock.hh"
#include "services\RcheevosClient.hh"

#include "ui\OverlayTheme.hh"

#include "ui\viewmodels\WindowManager.hh"

#include <rcheevos\src\rcheevos\rc_client_internal.h>

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty OverlayAchievementsPageViewModel::UnlockedAchievementsProperty("OverlayAchievementsPageViewModel", "UnlockedAchievements", L"");

const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::CreatedDateProperty("AchievementViewModel", "CreatedDate", L"");
const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::ModifiedDateProperty("AchievementViewModel", "ModifiedDate", L"");
const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::WonByProperty("AchievementViewModel", "WonBy", L"");

static void SetImage(OverlayListPageViewModel::ItemViewModel& vmItem, const char* sBadgeName, bool bLocked)
{
    std::string sImageName = sBadgeName;
    if (bLocked)
        sImageName += "_lock";

    vmItem.Image.ChangeReference(ra::ui::ImageType::Badge, sImageName);
    vmItem.SetDisabled(bLocked);
}

static void SetAchievement(OverlayListPageViewModel::ItemViewModel& vmItem,
                           const rc_client_achievement_bucket_t& pBucket,
                           const rc_client_achievement_t& pAchievement)
{
    vmItem.SetId(pAchievement.id);
    vmItem.SetLabel(ra::StringPrintf(L"%s (%s %s)", pAchievement.title, pAchievement.points,
                                     (pAchievement.points == 1) ? "point" : "points"));
    vmItem.SetDetail(ra::Widen(pAchievement.description));
    vmItem.SetCollapsed(false);

    switch (pAchievement.state)
    {
        case RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE:
            if (pBucket.subset_id == ra::data::context::GameAssets::LocalSubsetId) // local achievements never appear disabled
                SetImage(vmItem, pAchievement.badge_name, false);
            else
                SetImage(vmItem, pAchievement.badge_name, true);

            if (pAchievement.measured_progress[0])
            {
                vmItem.SetProgressString(ra::Widen(pAchievement.measured_progress));
                vmItem.SetProgressPercentage(pAchievement.measured_percent);
            }
            else
            {
                vmItem.SetProgressString(L"");
                vmItem.SetProgressPercentage(0.0f);
            }
            break;

        case RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED:
            SetImage(vmItem, pAchievement.badge_name, false);
            vmItem.SetProgressString(L"");
            vmItem.SetProgressPercentage(0.0f);
            break;

        default: // DISABLED/INACTIVE
            SetImage(vmItem, pAchievement.badge_name, true);
            vmItem.SetProgressString(L"");
            vmItem.SetProgressPercentage(0.0f);
            break;
    }
}

void OverlayAchievementsPageViewModel::Refresh()
{
    m_sTitle = L"Achievements";
    m_sDetailTitle = L"Achievement Info";
    OverlayListPageViewModel::Refresh();

    // title
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    SetListTitle(pGameContext.GameTitle());

    // achievement list
    const auto& pClient = ra::services::ServiceLocator::Get<ra::services::RcheevosClient>().GetClient();

    std::vector<rc_client_subset_info_t*> vDeactivatedSubsets;
    int nCategory = RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE_AND_UNOFFICIAL;
    const auto& pAssetList = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().AssetList;
    if (pAssetList.GetFilterCategory() == ra::ui::viewmodels::AssetListViewModel::FilterCategory::Core)
    {
        nCategory = RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE;

        // disable local subset while we build the list
        auto* pSubset = pClient->game ? pClient->game->subsets : nullptr;
        for (; pSubset; pSubset = pSubset->next)
        {
            if (!pSubset->active)
                continue;

            if (pSubset->public_.id == ra::data::context::GameAssets::LocalSubsetId)
            {
                vDeactivatedSubsets.push_back(pSubset);
                pSubset->active = 0;
            }
        }
    }

    auto* pAchievementList = rc_client_create_achievement_list(pClient, nCategory,
        RC_CLIENT_ACHIEVEMENT_LIST_GROUPING_PROGRESS);

    for (auto* pSubset : vDeactivatedSubsets)
        pSubset->active = 1;

    size_t nIndex = 0;
    size_t nNumberOfAchievements = 0;
    m_mHeaderKeys.clear();
    m_vItems.BeginUpdate();

    const auto* pBucket = pAchievementList->buckets;
    if (pBucket != nullptr)
    {
        const bool bCanCollapseHeaders = GetCanCollapseHeaders();
        const auto* pBucketStop = pBucket + pAchievementList->num_buckets;
        for (; pBucket < pBucketStop; ++pBucket)
        {
            auto& pvmHeader = GetNextItem(&nIndex);
            SetHeader(pvmHeader, ra::Widen(pBucket->label));

            bool bCollapsed = false;
            if (bCanCollapseHeaders)
            {
                const auto nKey = pBucket->subset_id << 5 | pBucket->bucket_type;
                m_mHeaderKeys[nIndex - 1] = nKey;
                const auto pIter = m_mCollapseState.find(nKey);
                if (pIter != m_mCollapseState.end())
                {
                    bCollapsed = pIter->second;
                }
                else
                {
                    switch (pBucket->bucket_type)
                    {
                        case RC_CLIENT_ACHIEVEMENT_BUCKET_UNLOCKED:
                        case RC_CLIENT_ACHIEVEMENT_BUCKET_UNOFFICIAL:
                            bCollapsed = true;
                            break;
                    }
                }
            }

            pvmHeader.SetCollapsed(bCollapsed);

            if (!bCollapsed)
            {
                const auto* pAchievement = pBucket->achievements;
                Expects(pAchievement != nullptr);
                const rc_client_achievement_t* const* pAchievementStop = pAchievement + pBucket->num_achievements;
                for (; pAchievement < pAchievementStop; ++pAchievement)
                {
                    auto& pvmAchievement = GetNextItem(&nIndex);
                    SetAchievement(pvmAchievement, *pBucket, **pAchievement);
                }
            }

            nNumberOfAchievements += pBucket->num_achievements;
        }
    }

    while (m_vItems.Count() > nIndex)
        m_vItems.RemoveAt(m_vItems.Count() - 1);

    m_vItems.EndUpdate();

    free(pAchievementList);

    // summary
    rc_client_user_game_summary_t summary;
    rc_client_get_user_game_summary(pClient, &summary);
    if (nNumberOfAchievements == 0)
        m_sSummary = L"No achievements present";
    else if (summary.num_core_achievements > 0)
        m_sSummary = ra::StringPrintf(L"%u of %u won (%u/%u)",
            summary.num_unlocked_achievements, summary.num_core_achievements,
            summary.points_unlocked, summary.points_core);
    else
        m_sSummary = ra::StringPrintf(L"%u achievements present", nNumberOfAchievements);

    // playtime
    if (pGameContext.GameId() == 0)
    {
        SetSummary(m_sSummary);
    }
    else
    {
        const auto nPlayTimeSeconds = ra::services::ServiceLocator::Get<ra::data::context::SessionTracker>().GetTotalPlaytime(pGameContext.GameId());
        const auto nPlayTimeMinutes = std::chrono::duration_cast<std::chrono::minutes>(nPlayTimeSeconds).count();
        SetSummary(ra::StringPrintf(L"%s - %dh%02dm", m_sSummary, nPlayTimeMinutes / 60, nPlayTimeMinutes % 60));
        m_fElapsed = static_cast<double>(nPlayTimeSeconds.count() % 60);
    }

    EnsureSelectedItemIndexValid();
}

bool OverlayAchievementsPageViewModel::Update(double fElapsed)
{
    const bool bUpdated = OverlayListPageViewModel::Update(fElapsed);

    if (m_fElapsed < 60.0)
        return bUpdated;

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto nPlayTimeSeconds = ra::services::ServiceLocator::Get<ra::data::context::SessionTracker>().GetTotalPlaytime(pGameContext.GameId());
    const auto nPlayTimeMinutes = std::chrono::duration_cast<std::chrono::minutes>(nPlayTimeSeconds).count();
    SetSummary(ra::StringPrintf(L"%s - %dh%02dm", m_sSummary, nPlayTimeMinutes / 60, nPlayTimeMinutes % 60));

    do
    {
        m_fElapsed -= 60.0;
    } while (m_fElapsed > 60.0);

    return true;
}

bool OverlayAchievementsPageViewModel::OnHeaderClicked(ItemViewModel& vmItem)
{
    for (const auto pair : m_mHeaderKeys)
    {
        const auto* vmHeader = m_vItems.GetItemAt(pair.first);
        if (vmHeader == &vmItem)
        {
            m_mCollapseState[pair.second] = !vmItem.IsCollapsed();
            Refresh();
            return true;
        }
    }

    return false;
}

void OverlayAchievementsPageViewModel::RenderDetail(ra::ui::drawing::ISurface& pSurface, int nX, int nY, _UNUSED int nWidth, int nHeight) const
{
    const auto* pAchievement = m_vItems.GetItemAt(GetSelectedItemIndex());
    if (pAchievement == nullptr)
        return;

    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
    const auto nFont = pSurface.LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlayHeader(), ra::ui::FontStyles::Normal);
    const auto nSubFont = pSurface.LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlaySummary(), ra::ui::FontStyles::Normal);
    constexpr auto nAchievementSize = 64;

    pSurface.DrawImage(nX, nY, nAchievementSize, nAchievementSize, pAchievement->Image);

    pSurface.WriteText(nX + nAchievementSize + 12, nY + 4, nFont, pTheme.ColorOverlayText(), pAchievement->GetLabel());
    pSurface.WriteText(nX + nAchievementSize + 12, nY + 4 + 26, nSubFont, pTheme.ColorOverlaySubText(), pAchievement->GetDetail());
    nY += nAchievementSize + 8;

    const auto pDetail = m_vAchievementDetails.find(pAchievement->GetId());
    if (pDetail == m_vAchievementDetails.end())
        return;

    pSurface.WriteText(nX, nY, nSubFont, pTheme.ColorOverlaySubText(), L"Created: " + pDetail->second.GetCreatedDate());
    pSurface.WriteText(nX, nY + 26, nSubFont, pTheme.ColorOverlaySubText(), L"Modified: " + pDetail->second.GetModifiedDate());
    nY += 60;

    const auto sWonBy = pDetail->second.GetWonBy();
    if (sWonBy.empty())
        return;

    pSurface.WriteText(nX, nY, nSubFont, pTheme.ColorOverlaySubText(), sWonBy);
    pSurface.WriteText(nX, nY + 30, nFont, pTheme.ColorOverlayText(), L"Recent Winners:");
    nY += 60;

    gsl::index nIndex = 0;
    while (nY + 20 < nHeight)
    {
        const auto* pWinner = pDetail->second.RecentWinners.GetItemAt(nIndex);
        if (pWinner == nullptr)
            break;

        const auto nColor = pWinner->IsDisabled() ? pTheme.ColorOverlaySelectionText() : pTheme.ColorOverlaySubText();
        pSurface.WriteText(nX, nY, nSubFont, nColor, pWinner->GetLabel());
        pSurface.WriteText(nX + 200, nY, nSubFont, nColor, pWinner->GetDetail());

        ++nIndex;
        nY += 20;
    }
}

void OverlayAchievementsPageViewModel::FetchItemDetail(ItemViewModel& vmItem)
{
    if (m_vAchievementDetails.find(vmItem.GetId()) != m_vAchievementDetails.end()) // already populated
        return;

    const auto& pClient = ra::services::ServiceLocator::Get<ra::services::RcheevosClient>().GetClient();
    const auto nAchievementID = vmItem.GetId();
    GSL_SUPPRESS_TYPE1 const auto* pAchievement =
        reinterpret_cast<const rc_client_achievement_info_t*>(rc_client_get_achievement_info(pClient, nAchievementID));
    if (pAchievement == nullptr)
        return;

    auto& vmAchievement = m_vAchievementDetails.emplace(nAchievementID, AchievementViewModel{}).first->second;
    vmAchievement.SetCreatedDate(ra::Widen(ra::FormatDateTime(pAchievement->created_time)));
    vmAchievement.SetModifiedDate(ra::Widen(ra::FormatDateTime(pAchievement->updated_time)));

    if (nAchievementID >= ra::data::context::GameAssets::FirstLocalId)
    {
        vmAchievement.SetWonBy(L"Local Achievement");
        return;
    }

    ra::api::FetchAchievementInfo::Request request;
    request.AchievementId = nAchievementID;
    request.FirstEntry = 1;
    request.NumEntries = 10;
    request.CallAsync([this, nId = nAchievementID](const ra::api::FetchAchievementInfo::Response& response)
    {
        const auto pIter = m_vAchievementDetails.find(nId);
        if (pIter == m_vAchievementDetails.end())
            return;

        auto& vmAchievement = pIter->second;
        if (!response.Succeeded())
        {
            vmAchievement.SetWonBy(ra::Widen(response.ErrorMessage));
            return;
        }

        vmAchievement.SetWonBy(ra::StringPrintf(L"Won by %u of %u (%1.0f%%)", response.EarnedBy, response.NumPlayers,
            (static_cast<double>(response.EarnedBy) * 100) / response.NumPlayers));

        const auto& sUsername = ra::services::ServiceLocator::Get<ra::data::context::UserContext>().GetDisplayName();
        for (const auto& pWinner : response.Entries)
        {
            auto& vmWinner = vmAchievement.RecentWinners.Add();
            vmWinner.SetLabel(ra::Widen(pWinner.User));
            vmWinner.SetDetail(ra::StringPrintf(L"%s (%s)", ra::FormatDateTime(pWinner.DateAwarded), ra::FormatDateRecent(pWinner.DateAwarded)));

            if (pWinner.User == sUsername)
                vmWinner.SetDisabled(true);
        }

        ForceRedraw();
    });
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
