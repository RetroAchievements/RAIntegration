#include "OverlayFriendsPageViewModel.hh"

#include "RA_StringUtils.h"

#include "api\FetchUserFriends.hh"

#include "data\context\UserContext.hh"

namespace ra {
namespace ui {
namespace viewmodels {

void OverlayFriendsPageViewModel::Refresh()
{
    m_sTitle = L"Friends";
    OverlayListPageViewModel::Refresh();

    const auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
    const auto tNow = pClock.UpTime();
    if (tNow - m_tLastUpdated >= std::chrono::minutes(2))
    {
        m_tLastUpdated = tNow;

        ra::api::FetchUserFriends::Request request;
        request.CallAsync([this](const ra::api::FetchUserFriends::Response& response)
        {
            if (response.Failed())
            {
                SetSummary(ra::Widen(response.ErrorMessage));
                return;
            }

            for (const auto& pFriend : response.Friends)
            {
                auto sFriendName = ra::Widen(pFriend.User);
                sFriendName.push_back(' ');

                ItemViewModel* pvmFriend = nullptr;
                for (gsl::index nIndex = 0; nIndex < ra::to_signed(m_vItems.Count()); ++nIndex)
                {
                    const auto& pItemLabel = m_vItems.GetItemValue(nIndex, ItemViewModel::LabelProperty);
                    if (ra::StringStartsWith(pItemLabel, sFriendName))
                    {
                        pvmFriend = m_vItems.GetItemAt(nIndex);
                        break;
                    }
                }

                if (pvmFriend == nullptr)
                {
                    pvmFriend = &m_vItems.Add();
                    pvmFriend->Image.ChangeReference(ra::ui::ImageType::UserPic, pFriend.User);
                    Ensures(pvmFriend != nullptr);
                }

                pvmFriend->SetLabel(ra::StringPrintf(L"%s (%u)", pFriend.User, pFriend.Score));
                pvmFriend->SetDetail(pFriend.LastActivity);
            }

            OverlayListPageViewModel::Refresh();
            ForceRedraw();
        });
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
