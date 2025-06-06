#ifndef RA_API_FETCH_USER_FRIENDS_HH
#define RA_API_FETCH_USER_FRIENDS_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class FetchUserFriends
{
public:
    static constexpr const char* const Name() noexcept { return "FetchUserFriends"; }

    struct Response : ApiResponseBase
    {
        struct Friend
        {
            std::string User;
            std::string AvatarUrl;
            uint32_t Score{ 0U };
            std::wstring LastActivity;
            time_t LastActivityTime{ 0 };
            uint32_t LastActivityContextId{ 0U };
            std::wstring LastActivityContext;
            std::string LastActivityImageUrl;
        };

        std::vector<Friend> Friends;
    };

    struct Request : ApiRequestBase
    {
        using Callback = std::function<void(const Response& response)>;

        Response Call() const;

        void CallAsync(Callback&& callback) const
        {
            ApiRequestBase::CallAsync<Request, Callback>(*this, std::move(callback));
        }
    };
};

} // namespace api
} // namespace ra

#endif // !RA_API_FETCH_USER_FRIENDS_HH
