#include "OverlayLeaderboardsPageViewModel.hh"

#include "RA_StringUtils.h"

#include "api\FetchLeaderboardInfo.hh"

#include "data\context\GameContext.hh"
#include "data\context\UserContext.hh"

#include "ui\OverlayTheme.hh"

namespace ra {
namespace ui {
namespace viewmodels {

void OverlayLeaderboardsPageViewModel::Refresh()
{
    m_sTitle = L"Leaderboards";
    m_sDetailTitle = L"Leaderboard Info";
    OverlayListPageViewModel::Refresh();

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();

    // title
    SetListTitle(pGameContext.GameTitle());

    // leaderboard list
    size_t nNumberOfLeaderboards = 0;

    pGameContext.EnumerateLeaderboards([this, &nNumberOfLeaderboards](const RA_Leaderboard& pLeaderboard)
    {
        ItemViewModel* pvmLeaderboard = m_vItems.GetItemAt(nNumberOfLeaderboards);
        if (pvmLeaderboard == nullptr)
        {
            pvmLeaderboard = &m_vItems.Add();
            Ensures(pvmLeaderboard != nullptr);
        }

        pvmLeaderboard->SetId(pLeaderboard.ID());
        pvmLeaderboard->SetLabel(ra::Widen(pLeaderboard.Title()));
        pvmLeaderboard->SetDetail(ra::Widen(pLeaderboard.Description()));
        ++nNumberOfLeaderboards;
        return true;
    });

    while (m_vItems.Count() > nNumberOfLeaderboards)
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
    const auto* pLeaderboard = pGameContext.FindLeaderboard(vmItem.GetId());
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
        const RA_Leaderboard* pLeaderboard = pGameContext.FindLeaderboard(nId);
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
