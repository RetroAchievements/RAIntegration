#include "OverlayLeaderboardsPageViewModel.hh"

#include "api\FetchLeaderboardInfo.hh"

#include "data\GameContext.hh"
#include "data\UserContext.hh"

#include "ui\OverlayTheme.hh"

namespace ra {
namespace ui {
namespace viewmodels {

void OverlayLeaderboardsPageViewModel::Refresh()
{
    m_sTitle = L"Leaderboards";
    m_sDetailTitle = L"Leaderboard Info";
    OverlayListPageViewModel::Refresh();

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

    // title
    SetListTitle(pGameContext.GameTitle());

    // leaderboard list
    size_t nNumberOfLeaderboards = 0;

    pGameContext.EnumerateLeaderboards([this, &nNumberOfLeaderboards](const RA_Leaderboard& pLeaderboard)
    {
        ItemViewModel* pvmAchievement = m_vItems.GetItemAt(nNumberOfLeaderboards);
        if (pvmAchievement == nullptr)
            pvmAchievement = &m_vItems.Add();

        pvmAchievement->SetId(pLeaderboard.ID());
        pvmAchievement->SetLabel(ra::Widen(pLeaderboard.Title()));
        pvmAchievement->SetDetail(ra::Widen(pLeaderboard.Description()));
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
    const auto nFont = pSurface.LoadFont(OverlayViewModel::FONT_TO_USE, OverlayViewModel::FONT_SIZE_HEADER, ra::ui::FontStyles::Normal);
    const auto nSubFont = pSurface.LoadFont(OverlayViewModel::FONT_TO_USE, OverlayViewModel::FONT_SIZE_SUMMARY, ra::ui::FontStyles::Normal);

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

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    const auto* pLeaderboard = pGameContext.FindLeaderboard(vmItem.GetId());
    if (pLeaderboard == nullptr)
        return;

    auto& vmLeaderboard = m_vLeaderboardRanks.emplace(vmItem.GetId(), ViewModelCollection<ItemViewModel>()).first->second;

    ra::api::FetchLeaderboardInfo::Request request;
    request.LeaderboardId = vmItem.GetId();
    request.FirstEntry = 1;
    request.NumEntries = 10;
    request.CallAsync([this, nId = vmItem.GetId()](const ra::api::FetchLeaderboardInfo::Response& response)
    {
        auto pIter = m_vLeaderboardRanks.find(nId);
        if (pIter == m_vLeaderboardRanks.end())
            return;

        if (static_cast<ra::LeaderboardID>(m_vItems.GetItemAt(GetSelectedItemIndex())->GetId()) != nId)
            return;

        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
        RA_Leaderboard* pLeaderboard = pGameContext.FindLeaderboard(nId);
        if (!pLeaderboard)
            return;

        const auto& sUsername = ra::services::ServiceLocator::Get<ra::data::UserContext>().GetUsername();
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
