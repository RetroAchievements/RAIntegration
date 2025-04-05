#include "OverlayLeaderboardsPageViewModel.hh"

#include "RA_StringUtils.h"

#include "api\FetchLeaderboardInfo.hh"

#include "data\context\GameContext.hh"
#include "data\context\UserContext.hh"

#include "services\AchievementRuntime.hh"

#include "ui\OverlayTheme.hh"

#include "ui\viewmodels\WindowManager.hh"

#include <rcheevos\src\rc_client_internal.h>

namespace ra {
namespace ui {
namespace viewmodels {

static void SetLeaderboard(OverlayListPageViewModel::ItemViewModel& vmItem,
                           const rc_client_leaderboard_t& pLeaderboard)
{
    vmItem.SetId(pLeaderboard.id);
    vmItem.SetLabel(ra::Widen(pLeaderboard.title));
    vmItem.SetDetail(ra::Widen(pLeaderboard.description));
    vmItem.SetCollapsed(false);

    switch (pLeaderboard.state)
    {
        case RC_CLIENT_LEADERBOARD_STATE_ACTIVE:
            vmItem.SetProgressString(L"");
            vmItem.SetProgressPercentage(0.0f);
            vmItem.SetDisabled(false);
            break;

        case RC_CLIENT_LEADERBOARD_STATE_TRACKING:
            vmItem.SetProgressString(ra::Widen(pLeaderboard.tracker_value));
            vmItem.SetProgressPercentage(-1.0f);
            vmItem.SetDisabled(false);
            break;

        case RC_CLIENT_LEADERBOARD_STATE_DISABLED:
            vmItem.SetProgressString(L"");
            vmItem.SetProgressPercentage(0.0f);
            vmItem.SetDisabled(true);
            break;

        default: // INACTIVE
            vmItem.SetProgressString(L"");
            vmItem.SetProgressPercentage(0.0f);
            vmItem.SetDisabled(false);
            break;
    }
}

void OverlayLeaderboardsPageViewModel::Refresh()
{
    OverlayListPageViewModel::Refresh();

    // title
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    SetTitle(pGameContext.GameTitle());
    m_bHasDetail = true;

    // leaderboard list
    const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetClient();

    std::vector<rc_client_subset_info_t*> vDeactivatedSubsets;
    const auto& pAssetList = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().AssetList;
    if (pAssetList.GetCategoryFilter() == ra::ui::viewmodels::AssetListViewModel::CategoryFilter::Core)
    {
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

    const auto* pLeaderboardList = rc_client_create_leaderboard_list(pClient, RC_CLIENT_LEADERBOARD_LIST_GROUPING_TRACKING);

    for (auto* pSubset : vDeactivatedSubsets)
        pSubset->active = 1;

    size_t nIndex = 0;
    size_t nNumberOfLeaderboards = 0;
    m_mHeaderKeys.clear();
    m_vItems.BeginUpdate();

    const auto* pBucket = pLeaderboardList->buckets;
    if (pBucket != nullptr)
    {
        const bool bCanCollapseHeaders = GetCanCollapseHeaders();
        const rc_client_leaderboard_bucket_t* pBucketStop = pBucket + pLeaderboardList->num_buckets;
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
                    bCollapsed = pIter->second;
            }

            pvmHeader.SetCollapsed(bCollapsed);

            if (!bCollapsed)
            {
                const auto* pLeaderboard = pBucket->leaderboards;
                Expects(pLeaderboard != nullptr);
                const rc_client_leaderboard_t* const* pLeaderboardStop = pLeaderboard + pBucket->num_leaderboards;
                for (; pLeaderboard < pLeaderboardStop; ++pLeaderboard)
                {
                    auto& pvmLeaderboard = GetNextItem(&nIndex);
                    SetLeaderboard(pvmLeaderboard, **pLeaderboard);
                }
            }

            nNumberOfLeaderboards += pBucket->num_leaderboards;
        }
    }

    while (m_vItems.Count() > nIndex)
        m_vItems.RemoveAt(m_vItems.Count() - 1);

    m_vItems.EndUpdate();

    // summary
    if (nNumberOfLeaderboards == 0)
        SetSubTitle(L"No leaderboards present");
    else
        SetSubTitle(ra::StringPrintf(L"%u leaderboards present", nNumberOfLeaderboards));
}

bool OverlayLeaderboardsPageViewModel::OnHeaderClicked(ItemViewModel& vmItem)
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

const wchar_t* OverlayLeaderboardsPageViewModel::GetPrevButtonText() const noexcept
{
    if (m_bDetail)
        return L"";

    return L"Achievements";
}

const wchar_t* OverlayLeaderboardsPageViewModel::GetNextButtonText() const noexcept
{
    if (m_bDetail)
        return L"";

    return L"Following";
}

void OverlayLeaderboardsPageViewModel::RenderDetail(ra::ui::drawing::ISurface& pSurface, int nX, int nY, _UNUSED int nWidth, int nHeight) const
{
    const auto* pLeaderboard = m_vItems.GetItemAt(GetSelectedItemIndex());
    if (pLeaderboard == nullptr)
        return;

    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
    const auto nFont = pSurface.LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlayHeader(), ra::ui::FontStyles::Normal);
    const auto nSubFont = pSurface.LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlaySummary(), ra::ui::FontStyles::Normal);

    pSurface.WriteText(nX, nY + 4, nFont, pTheme.ColorOverlayText(), pLeaderboard->GetLabel());
    pSurface.WriteText(nX, nY + 4 + 26, nSubFont, pTheme.ColorOverlaySubText(), pLeaderboard->GetDetail());
    nY += 64;

    const auto pDetail = m_vLeaderboardRanks.find(pLeaderboard->GetId());
    if (pDetail == m_vLeaderboardRanks.end())
        return;

    gsl::index nIndex = 0;
    while (nY + 20 < nHeight)
    {
        const auto* pEntry = pDetail->second.GetItemAt(nIndex);
        if (pEntry == nullptr)
            break;

        const auto nColor = pEntry->IsDisabled() ? pTheme.ColorOverlaySelectionText() : pTheme.ColorOverlaySubText();
        pSurface.WriteText(nX, nY, nSubFont, nColor, std::to_wstring(pEntry->GetId()));
        pSurface.WriteText(nX + 60, nY, nSubFont, nColor, pEntry->GetLabel());
        pSurface.WriteText(nX + 260, nY, nSubFont, nColor, pEntry->GetDetail());

        ++nIndex;
        nY += 20;
    }
}

void OverlayLeaderboardsPageViewModel::FetchItemDetail(ItemViewModel& vmItem)
{
    if (m_vLeaderboardRanks.find(vmItem.GetId()) != m_vLeaderboardRanks.end()) // already populated
        return;

    const auto& pClient = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>().GetClient();
    const auto nLeaderboardId = vmItem.GetId();
    GSL_SUPPRESS_TYPE1 const auto* pLeaderboard =
        reinterpret_cast<const rc_client_leaderboard_info_t*>(rc_client_get_leaderboard_info(pClient, nLeaderboardId));
    if (pLeaderboard == nullptr)
        return;

    m_vLeaderboardRanks.emplace(nLeaderboardId, ViewModelCollection<ItemViewModel>());

    ra::api::FetchLeaderboardInfo::Request request;
    request.LeaderboardId = nLeaderboardId;
    request.AroundUser = ra::services::ServiceLocator::Get<ra::data::context::UserContext>().GetUsername();
    request.NumEntries = 11;
    request.CallAsync([this, nId = nLeaderboardId, nFormat = pLeaderboard->format](const ra::api::FetchLeaderboardInfo::Response& response)
    {
        const auto pIter = m_vLeaderboardRanks.find(nId);
        if (pIter == m_vLeaderboardRanks.end())
            return;

        char sBuffer[64] = "";
        const auto& sUsername = ra::services::ServiceLocator::Get<ra::data::context::UserContext>().GetDisplayName();
        auto& vmLeaderboard = pIter->second;
        for (const auto& pEntry : response.Entries)
        {
            auto& vmEntry = vmLeaderboard.Add();
            vmEntry.SetId(pEntry.Rank);
            vmEntry.SetLabel(ra::Widen(pEntry.User));

            rc_format_value(sBuffer, sizeof(sBuffer), pEntry.Score, nFormat);
            vmEntry.SetDetail(ra::Widen(sBuffer));

            if (pEntry.User == sUsername)
                vmEntry.SetDisabled(true);
        }

        ForceRedraw();
    });
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
