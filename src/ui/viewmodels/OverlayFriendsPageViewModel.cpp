#include "OverlayFriendsPageViewModel.hh"

#include "context\IRCClient.hh"
#include "context\UserContext.hh"

#include "services\ServiceLocator.hh"

#include "util\Strings.hh"

#include "ui\IImageRepository.hh"

#include <rcheevos/include/rc_api_user.h>

namespace ra {
namespace ui {
namespace viewmodels {

void OverlayFriendsPageViewModel::Refresh()
{
    SetTitle(L"Following");
    OverlayListPageViewModel::Refresh();

    const auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
    const auto tNow = pClock.UpTime();
    if (tNow - m_tLastUpdated >= std::chrono::minutes(2))
    {
        m_tLastUpdated = tNow;

        const auto& pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>();
        rc_api_fetch_followed_users_request_t api_params;
        memset(&api_params, 0, sizeof(api_params));
        const auto& pUserContext = ra::services::ServiceLocator::Get<ra::context::UserContext>();
        api_params.username = pUserContext.GetUsername().c_str();
        api_params.api_token = pUserContext.GetApiToken().c_str();

        rc_api_request_t request;
        const auto nResult = rc_api_init_fetch_followed_users_request_hosted(&request, &api_params, pClient.GetHost());
        Expects(nResult == RC_OK);

        pClient.DispatchRequest(request, [this](const rc_api_server_response_t& api_response, void*) {
            rc_api_fetch_followed_users_response_t response;
            const auto nResult = rc_api_process_fetch_followed_users_server_response(&response, &api_response);
            if (nResult != RC_OK)
            {
                SetSubTitle(ra::util::String::Widen(ra::context::IRcClient::GetErrorMessage(nResult, response.response)));
            }
            else
            {
                auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();

                const auto* pFriend = response.users;
                const auto* pStop = pFriend + response.num_users;
                for (; pFriend < pStop; ++pFriend) {
                    pImageRepository.FetchImage(ra::ui::ImageType::UserPic, pFriend->display_name, pFriend->avatar_url, pFriend->avatar_last_updated);

                    auto sFriendName = ra::util::String::Widen(pFriend->display_name);
                    sFriendName.push_back(' ');

                    ItemViewModel* pvmFriend = nullptr;
                    for (gsl::index nIndex = 0; nIndex < ra::to_signed(m_vItems.Count()); ++nIndex)
                    {
                        const auto& pItemLabel = m_vItems.GetItemValue(nIndex, ItemViewModel::LabelProperty);
                        if (ra::util::String::StartsWith(pItemLabel, sFriendName))
                        {
                            pvmFriend = m_vItems.GetItemAt(nIndex);
                            break;
                        }
                    }

                    if (pvmFriend == nullptr)
                    {
                        pvmFriend = &m_vItems.Add();
                        Ensures(pvmFriend != nullptr);
                        pvmFriend->SetId(gsl::narrow_cast<int>(m_vItems.Count())); // prevent it from looking like a header
                        pvmFriend->Image.ChangeReference(ra::ui::ImageType::UserPic, pFriend->display_name);
                    }

                    pvmFriend->SetLabel(ra::util::String::Printf(L"%s (%u)", pFriend->display_name, pFriend->score));
                    pvmFriend->SetDetail(ra::util::String::Widen(pFriend->recent_activity.description));
                }

                OverlayListPageViewModel::Refresh();
                ForceRedraw();
            }
        }, nullptr);
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
