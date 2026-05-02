#include "OverlayFriendsPageViewModel.hh"

#include "util\Strings.hh"

#include "api\FetchUserFriends.hh"

#include "context\UserContext.hh"

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

        ra::api::FetchUserFriends::Request request;
        request.CallAsync([this](const ra::api::FetchUserFriends::Response& response)
        {
            if (response.Failed())
            {
                SetSubTitle(ra::util::String::Widen(response.ErrorMessage));
                return;
            }

            auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();

            for (const auto& pFriend : response.Friends)
            {
                auto sFriendName = ra::util::String::Widen(pFriend.User);
                sFriendName.push_back(' ');

                if (!pImageRepository.IsImageAvailable(ra::ui::ImageType::UserPic, pFriend.User))
                    pImageRepository.FetchImage(ra::ui::ImageType::UserPic, pFriend.User, pFriend.AvatarUrl);

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
                    pvmFriend->Image.ChangeReference(ra::ui::ImageType::UserPic, pFriend.User);
                }

                pvmFriend->SetLabel(ra::util::String::Printf(L"%s (%u)", pFriend.User, pFriend.Score));
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
