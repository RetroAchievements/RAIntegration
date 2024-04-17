#include "OverlayRecentGamesPageViewModel.hh"

#include "RA_StringUtils.h"

#include "api\impl\OfflineServer.hh"

#include "data\context\GameContext.hh"
#include "data\context\SessionTracker.hh"
#include "data\context\UserContext.hh"

#include "services\Http.hh"
#include "services\IConfiguration.hh"
#include "services\ILocalStorage.hh"
#include "services\IThreadPool.hh"

#include <rcheevos\include\rc_api_runtime.h>

namespace ra {
namespace ui {
namespace viewmodels {

void OverlayRecentGamesPageViewModel::Refresh()
{
    SetTitle(L"Recent Games");
    OverlayListPageViewModel::Refresh();

    for (gsl::index nIndex = m_vItems.Count() - 1; nIndex >= 0; --nIndex)
        m_vItems.RemoveAt(nIndex);

    std::vector<unsigned int> vPendingGames;
    const bool bOffline = ra::services::ServiceLocator::Get<ra::services::IConfiguration>()
        .IsFeatureEnabled(ra::services::Feature::Offline);

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
        else if (bOffline)
        {
            UpdateGameEntry(pvmItem, L"Unknown", "00000", pGameStats.TotalPlayTime);
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
            const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
            rc_api_fetch_game_data_request_t api_params;
            memset(&api_params, 0, sizeof(api_params));
            api_params.username = pUserContext.GetUsername().c_str();
            api_params.api_token = pUserContext.GetApiToken().c_str();

            auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
            for (const auto nGameId : vPendingGames)
            {
                auto pData = pLocalStorage.ReadText(ra::services::StorageItemType::GameData, std::to_wstring(nGameId));
                if (pData != nullptr)
                {
                    std::string sContents = "{\"Success\":true,\"PatchData\":";
                    std::string sLine;
                    while (pData->GetLine(sLine))
                    {
                        if (!sContents.empty())
                            sContents.push_back('\n');

                        sContents.append(sLine);
                    }
                    sContents.push_back('}');

                    rc_api_fetch_game_data_response_t response;
                    if (rc_api_process_fetch_game_data_response(&response, sContents.c_str()) == RC_OK)
                        UpdateGameEntry(nGameId, ra::Widen(response.title), response.image_name);
                }
                else
                {
                    api_params.game_id = nGameId;

                    rc_api_request_t request;
                    if (rc_api_init_fetch_game_data_request(&request, &api_params) != RC_OK)
                        continue;

                    ra::services::Http::Request httpRequest(request.url);
                    httpRequest.SetContentType(request.content_type);
                    httpRequest.SetPostData(request.post_data);

                    httpRequest.CallAsync([this, nGameId](const ra::services::Http::Response& pResponse)
                    {
                        rc_api_fetch_game_data_response_t response;
                        rc_api_server_response_t server_response;

                        memset(&server_response, 0, sizeof(server_response));
                        server_response.body = pResponse.Content().c_str();
                        server_response.body_length = pResponse.Content().length();
                        server_response.http_status_code = ra::etoi(pResponse.StatusCode());

                        if (rc_api_process_fetch_game_data_server_response(&response, &server_response) == RC_OK &&
                            response.response.succeeded)
                        {
                            UpdateGameEntry(nGameId, ra::Widen(response.title), response.image_name);
                        }
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
