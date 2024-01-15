#include "OverlayAchievementsPageViewModel.hh"

#include "RA_StringUtils.h"

#include "api\FetchAchievementInfo.hh"

#include "data\context\GameContext.hh"
#include "data\context\SessionTracker.hh"
#include "data\context\UserContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\IClock.hh"

#include "ui\OverlayTheme.hh"

#include "ui\viewmodels\WindowManager.hh"

#include <rcheevos\src\rc_client_internal.h>

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty OverlayAchievementsPageViewModel::UnlockedAchievementsProperty("OverlayAchievementsPageViewModel", "UnlockedAchievements", L"");

const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::CreatedDateProperty("AchievementViewModel", "CreatedDate", L"");
const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::ModifiedDateProperty("AchievementViewModel", "ModifiedDate", L"");
const StringModelProperty OverlayAchievementsPageViewModel::AchievementViewModel::WonByProperty("AchievementViewModel", "WonBy", L"");

static void SetImage(OverlayListPageViewModel::ItemViewModel& vmItem, const std::string& sBadgeName, bool bLocked)
{
    std::string sImageName = sBadgeName;
    if (bLocked && !ra::StringStartsWith(sBadgeName, "local")) // local images don't have _lock equivalents
    {
        sImageName += "_lock";
        vmItem.SetDisabled(true);
    }
    else
    {
        vmItem.SetDisabled(false);
    }

    vmItem.Image.ChangeReference(ra::ui::ImageType::Badge, sImageName);
}

static void SetAchievement(OverlayListPageViewModel::ItemViewModel& vmItem,
                           const rc_client_achievement_t& pAchievement)
{
    vmItem.SetId(pAchievement.id);
    vmItem.SetLabel(ra::StringPrintf(L"%s (%s %s)", pAchievement.title, pAchievement.points,
                                     (pAchievement.points == 1) ? "point" : "points"));
    vmItem.SetDetail(ra::Widen(pAchievement.description));
    vmItem.SetCollapsed(false);

    std::string sBadgeName = ra::services::AchievementRuntime::GetAchievementBadge(pAchievement);

    switch (pAchievement.state)
    {
        case RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE:
            SetImage(vmItem, sBadgeName, true);

            if (pAchievement.measured_progress[0])
            {
                vmItem.SetProgressString(ra::Widen(pAchievement.measured_progress));
                vmItem.SetProgressPercentage(pAchievement.measured_percent / 100.0f);
            }
            else
            {
                vmItem.SetProgressString(L"");
                vmItem.SetProgressPercentage(0.0f);
            }
            break;

        case RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED:
            SetImage(vmItem, sBadgeName, false);
            vmItem.SetProgressString(L"");
            vmItem.SetProgressPercentage(0.0f);
            break;

        default: // DISABLED/INACTIVE
            SetImage(vmItem, sBadgeName, true);
            vmItem.SetProgressString(L"");
            vmItem.SetProgressPercentage(0.0f);
            break;
    }
}

void OverlayAchievementsPageViewModel::Refresh()
{
    OverlayListPageViewModel::Refresh();

    if (m_pMissableSurface == nullptr)
    {
        #include "..\data\missable.h"
        m_pMissableSurface = LoadDecorator(MISSABLE_PIXELS, 20, 20);

        #include "..\data\progression.h"
        m_pProgressionSurface = LoadDecorator(PROGRESSION_PIXELS, 20, 20);

        #include "..\data\win-condition.h"
        m_pWinConditionSurface = LoadDecorator(WIN_CONDITION_PIXELS, 20, 20);
    }

    // title
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto& sTitle = pGameContext.GameTitle();
    if (sTitle.empty())
        SetTitle(L"No Game Loaded");
    else
        SetTitle(pGameContext.GameTitle());
    m_bHasDetail = true;

    // achievement list
    const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetClient();

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
                    SetAchievement(pvmAchievement, **pAchievement);

                    switch ((*pAchievement)->type)
                    {
                        case RC_CLIENT_ACHIEVEMENT_TYPE_MISSABLE:
                            pvmAchievement.SetDecorator(m_pMissableSurface.get());
                            break;
                        case RC_CLIENT_ACHIEVEMENT_TYPE_PROGRESSION:
                            pvmAchievement.SetDecorator(m_pProgressionSurface.get());
                            break;
                        case RC_CLIENT_ACHIEVEMENT_TYPE_WIN:
                            pvmAchievement.SetDecorator(m_pWinConditionSurface.get());
                            break;
                        default:
                            pvmAchievement.SetDecorator(nullptr);
                            break;
                    }
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
    {
        SetSubTitle(L"No achievements present");
    }
    else if (summary.num_core_achievements > 0)
    {
        SetSubTitle(ra::StringPrintf(L"%u of %u achievements",
            summary.num_unlocked_achievements, summary.num_core_achievements));

        m_sSummary = ra::StringPrintf(L"%d of %d points", summary.points_unlocked, summary.points_core);
    }
    else
    {
        SetSubTitle(ra::StringPrintf(L"%u achievements present", nNumberOfAchievements));
    }

    // playtime
    if (pGameContext.GameId() == 0)
    {
        SetTitleDetail(m_sSummary);
    }
    else
    {
        const auto nPlayTimeSeconds = ra::services::ServiceLocator::Get<ra::data::context::SessionTracker>().GetTotalPlaytime(pGameContext.GameId());
        const auto nPlayTimeMinutes = std::chrono::duration_cast<std::chrono::minutes>(nPlayTimeSeconds).count();
        SetTitleDetail(ra::StringPrintf(L"%s - %dh%02dm", m_sSummary, nPlayTimeMinutes / 60, nPlayTimeMinutes % 60));
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
    SetTitleDetail(ra::StringPrintf(L"%s - %dh%02dm", m_sSummary, nPlayTimeMinutes / 60, nPlayTimeMinutes % 60));

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

const wchar_t* OverlayAchievementsPageViewModel::GetPrevButtonText() const noexcept
{
    if (m_bDetail)
        return L"";

    return L"Recent Games";
}

const wchar_t* OverlayAchievementsPageViewModel::GetNextButtonText() const noexcept
{
    if (m_bDetail)
        return L"";

    return L"Leaderboards";
}

void OverlayAchievementsPageViewModel::RenderDetail(ra::ui::drawing::ISurface& pSurface, int nX, int nY, int nWidth, int nHeight) const
{
    const auto* pAchievement = m_vItems.GetItemAt(GetSelectedItemIndex());
    if (pAchievement == nullptr)
        return;

    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
    const auto nFont = pSurface.LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlayHeader(), ra::ui::FontStyles::Normal);
    const auto nSubFont = pSurface.LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlaySummary(), ra::ui::FontStyles::Normal);
    const auto nDetailFont = pSurface.LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlayDetail(), ra::ui::FontStyles::Normal);
    constexpr auto nAchievementSize = 64;

    pSurface.DrawImage(nX, nY, nAchievementSize, nAchievementSize, pAchievement->Image);

    pSurface.WriteText(nX + nAchievementSize + 12, nY + 4, nFont, pTheme.ColorOverlayText(), pAchievement->GetLabel());
    pSurface.WriteText(nX + nAchievementSize + 12, nY + 4 + 26, nSubFont, pTheme.ColorOverlaySubText(), pAchievement->GetDetail());

    const auto* pDecorator = pAchievement->GetDecorator();
    if (pDecorator != nullptr)
        pSurface.DrawSurface(nWidth - 5 - pDecorator->GetWidth(), nY + 6, *pDecorator);

    const auto pDetail = m_vAchievementDetails.find(pAchievement->GetId());
    if (pDetail == m_vAchievementDetails.end())
        return;

    nY += nAchievementSize + 8;

    const auto szDetail = pSurface.MeasureText(nDetailFont, L"M");
    pSurface.WriteText(nX, nY, nDetailFont, pTheme.ColorOverlaySubText(), L"Created: " + pDetail->second.GetCreatedDate());
    nY += szDetail.Height;
    pSurface.WriteText(nX, nY, nDetailFont, pTheme.ColorOverlaySubText(), L"Modified: " + pDetail->second.GetModifiedDate());
    nY += szDetail.Height + 8;

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

    const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetClient();
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
