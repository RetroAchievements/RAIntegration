#include "OverlayRecentGamesPageViewModel.hh"

#include "RA_StringUtils.h"

#include "api\FetchGameData.hh"
#include "api\impl\OfflineServer.hh"

#include "data\context\GameContext.hh"
#include "data\context\SessionTracker.hh"

#include "services\IThreadPool.hh"

namespace ra {
namespace ui {
namespace viewmodels {

void OverlayRecentGamesPageViewModel::Refresh()
{
    m_sTitle = L"Recent Games";
    OverlayListPageViewModel::Refresh();

    for (gsl::index nIndex = m_vItems.Count() - 1; nIndex >= 0; --nIndex)
        m_vItems.RemoveAt(nIndex);

    std::vector<unsigned int> vPendingGames;

    const auto& pSessionTracker = ra::services::ServiceLocator::Get<ra::data::context::SessionTracker>();
    for (const auto& pGameStats : pSessionTracker.SessionData())
    {
        ItemViewModel& pvmItem = m_vItems.Add();
        pvmItem.SetId(pGameStats.GameId);

        const auto tLastSessionStart = std::chrono::system_clock::to_time_t(pGameStats.LastSessionStart);
        pvmItem.SetDetail(ra::StringPrintf(L"Last played: %s (%s)",
            ra::FormatDate(tLastSessionStart),
            ra::FormatDateRecent(tLastSessionStart)));

        const auto& pIter2 = m_mGameBadges.find(pGameStats.GameId);
        const auto& pIter = m_mGameNames.find(pGameStats.GameId);
        if (pIter != m_mGameNames.end() && pIter2 != m_mGameBadges.end())
        {
            UpdateGameEntry(pvmItem, pIter->second, pIter2->second, pGameStats.TotalPlayTime);
        }
        else
        {
            UpdateGameEntry(pvmItem, L"Loading", "00000", pGameStats.TotalPlayTime);
            vPendingGames.push_back(pGameStats.GameId);
        }

        if (m_vItems.Count() == 20)
            break;
    }

    if (!vPendingGames.empty())
    {
        ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().RunAsync(
            [this, vPendingGames = std::move(vPendingGames)]()
        {
            ra::api::impl::OfflineServer cache;
            for (const auto nGameId : vPendingGames)
            {
                ra::api::FetchGameData::Request request;
                request.GameId = nGameId;
                const auto response = cache.FetchGameData(request);
                if (response.Succeeded())
                {
                    UpdateGameEntry(nGameId, response.Title, response.ImageIcon);
                }
                else
                {
                    request.CallAsync([this, nGameId](const ra::api::FetchGameData::Response & response)
                    {
                        UpdateGameEntry(nGameId, response.Title, response.ImageIcon);
                    });
                }
            }
        });
    }
}

void OverlayRecentGamesPageViewModel::UpdateGameEntry(unsigned int nGameId, const std::wstring& sGameName, const std::string& sGameBadge)
{
    m_mGameNames.emplace(nGameId, sGameName);
    m_mGameBadges.emplace(nGameId, sGameBadge);

    const auto nIndex = m_vItems.FindItemIndex(ItemViewModel::IdProperty, nGameId);
    if (nIndex != -1)
    {
        auto* pvmItem = m_vItems.GetItemAt(nIndex);
        if (pvmItem)
        {
            const auto& pSessionTracker = ra::services::ServiceLocator::Get<ra::data::context::SessionTracker>();
            const auto nPlayTimeSeconds = pSessionTracker.GetTotalPlaytime(nGameId);
            UpdateGameEntry(*pvmItem, sGameName, sGameBadge, nPlayTimeSeconds);

            ForceRedraw();
        }
    }
}

void OverlayRecentGamesPageViewModel::UpdateGameEntry(ItemViewModel& pvmItem,
    const std::wstring& sGameName, const std::string& sGameBadge, std::chrono::seconds nPlayTimeSeconds)
{
    const auto nPlayTimeMinutes = std::chrono::duration_cast<std::chrono::minutes>(nPlayTimeSeconds).count();
    pvmItem.SetLabel(ra::StringPrintf(L"%s - %dh%02dm", sGameName, nPlayTimeMinutes / 60, nPlayTimeMinutes % 60));
    pvmItem.Image.ChangeReference(ra::ui::ImageType::Icon, sGameBadge);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
