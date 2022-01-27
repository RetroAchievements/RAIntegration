#include "OverlayLeaderboardsPageViewModel.hh"

#include "RA_StringUtils.h"

#include "api\FetchLeaderboardInfo.hh"

#include "data\context\GameContext.hh"
#include "data\context\UserContext.hh"

#include "ui\OverlayTheme.hh"

namespace ra {
namespace ui {
namespace viewmodels {

static void SetLeaderboard(OverlayListPageViewModel::ItemViewModel& vmItem, const ra::data::models::LeaderboardModel& vmLeaderboard)
{
    vmItem.SetId(vmLeaderboard.GetID());
    vmItem.SetLabel(vmLeaderboard.GetName());
    vmItem.SetDetail(vmLeaderboard.GetDescription());
    vmItem.SetDisabled(vmLeaderboard.GetState() == ra::data::models::AssetState::Disabled);
}

void OverlayLeaderboardsPageViewModel::Refresh()
{
    m_sTitle = L"Leaderboards";
    m_sDetailTitle = L"Leaderboard Info";
    OverlayListPageViewModel::Refresh();

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();

    // title
    SetListTitle(pGameContext.GameTitle());

    // leaderboard list
    std::vector<const ra::data::models::LeaderboardModel*> vActiveLeaderboards;
    std::vector<const ra::data::models::LeaderboardModel*> vLocalLeaderboards;
    std::vector<const ra::data::models::LeaderboardModel*> vCoreLeaderboards;
    std::vector<const ra::data::models::LeaderboardModel*> vUnsupportedLeaderboards;
    for (gsl::index nIndex = 0; nIndex < gsl::narrow_cast<gsl::index>(pGameContext.Assets().Count()); ++nIndex)
    {
        const auto* pLeaderboard = dynamic_cast<const ra::data::models::LeaderboardModel*>(pGameContext.Assets().GetItemAt(nIndex));
        if (pLeaderboard != nullptr && !pLeaderboard->IsHidden())
        {
            switch (pLeaderboard->GetState())
            {
                case ra::data::models::AssetState::Primed:
                    vActiveLeaderboards.push_back(pLeaderboard);
                    continue;

                case ra::data::models::AssetState::Disabled:
                    vUnsupportedLeaderboards.push_back(pLeaderboard);
                    continue;

                default:
                    break;
            }

            switch (pLeaderboard->GetCategory())
            {
                case ra::data::models::AssetCategory::Core:
                    vCoreLeaderboards.push_back(pLeaderboard);
                    break;

                default:
                    if (AssetAppearsInFilter(*pLeaderboard))
                        vLocalLeaderboards.push_back(pLeaderboard);
                    break;
            }
        }
    }

    size_t nIndex = 0;
    size_t nNumberOfLeaderboards = 0;

    if (!vActiveLeaderboards.empty())
    {
        auto& pvmHeader = GetNextItem(&nIndex);
        SetHeader(pvmHeader, L"Active Leaderboards");

        for (const auto* vmLeaderboard : vActiveLeaderboards)
        {
            if (vmLeaderboard == nullptr)
                continue;

            auto& pvmLeaderboard = GetNextItem(&nIndex);
            SetLeaderboard(pvmLeaderboard, *vmLeaderboard);
        }

        nNumberOfLeaderboards += vActiveLeaderboards.size();
    }

    if (!vLocalLeaderboards.empty())
    {
        if (nIndex > 0 || !vCoreLeaderboards.empty() || !vUnsupportedLeaderboards.empty())
        {
            auto& pvmHeader = GetNextItem(&nIndex);
            SetHeader(pvmHeader, L"Inactive Local Leaderboards");
        }

        for (const auto* vmLeaderboard : vLocalLeaderboards)
        {
            if (vmLeaderboard == nullptr)
                continue;

            auto& pvmLeaderboard = GetNextItem(&nIndex);
            SetLeaderboard(pvmLeaderboard, *vmLeaderboard);
        }

        nNumberOfLeaderboards += vLocalLeaderboards.size();
    }

    if (!vCoreLeaderboards.empty())
    {
        if (nIndex > 0 || !vUnsupportedLeaderboards.empty())
        {
            auto& pvmHeader = GetNextItem(&nIndex);
            SetHeader(pvmHeader, L"Inactive Leaderboards");
        }

        for (const auto* vmLeaderboard : vCoreLeaderboards)
        {
            if (vmLeaderboard == nullptr)
                continue;

            auto& pvmLeaderboard = GetNextItem(&nIndex);
            SetLeaderboard(pvmLeaderboard, *vmLeaderboard);
        }

        nNumberOfLeaderboards += vCoreLeaderboards.size();
    }

    if (!vUnsupportedLeaderboards.empty())
    {
        if (nIndex > 0)
        {
            auto& pvmHeader = GetNextItem(&nIndex);
            SetHeader(pvmHeader, L"Unsupported Leaderboards");
        }

        for (const auto* vmLeaderboard : vUnsupportedLeaderboards)
        {
            if (vmLeaderboard == nullptr)
                continue;

            auto& pvmLeaderboard = GetNextItem(&nIndex);
            SetLeaderboard(pvmLeaderboard, *vmLeaderboard);
        }

        nNumberOfLeaderboards += vUnsupportedLeaderboards.size();
    }

    while (m_vItems.Count() > nIndex)
        m_vItems.RemoveAt(m_vItems.Count() - 1);

    // summary
    if (nNumberOfLeaderboards == 0)
        SetSummary(L"No leaderboards present");
    else
        SetSummary(ra::StringPrintf(L"%u leaderboards present", nNumberOfLeaderboards));
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

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pLeaderboard = pGameContext.Assets().FindLeaderboard(vmItem.GetId());
    if (pLeaderboard == nullptr)
        return;

    m_vLeaderboardRanks.emplace(vmItem.GetId(), ViewModelCollection<ItemViewModel>());

    ra::api::FetchLeaderboardInfo::Request request;
    request.LeaderboardId = vmItem.GetId();
    request.AroundUser = ra::services::ServiceLocator::Get<ra::data::context::UserContext>().GetUsername();
    request.NumEntries = 11;
    request.CallAsync([this, nId = vmItem.GetId()](const ra::api::FetchLeaderboardInfo::Response& response)
    {
        const auto pIter = m_vLeaderboardRanks.find(nId);
        if (pIter == m_vLeaderboardRanks.end())
            return;

        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
        const auto* pLeaderboard = pGameContext.Assets().FindLeaderboard(nId);
        if (!pLeaderboard)
            return;

        const auto& sUsername = ra::services::ServiceLocator::Get<ra::data::context::UserContext>().GetUsername();
        auto& vmLeaderboard = pIter->second;
        for (const auto& pEntry : response.Entries)
        {
            auto& vmEntry = vmLeaderboard.Add();
            vmEntry.SetId(pEntry.Rank);
            vmEntry.SetLabel(ra::Widen(pEntry.User));
            vmEntry.SetDetail(ra::Widen(pLeaderboard->FormatScore(pEntry.Score)));

            if (pEntry.User == sUsername)
                vmEntry.SetDisabled(true);
        }

        ForceRedraw();
    });
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
