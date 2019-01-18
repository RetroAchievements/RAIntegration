#ifndef RA_API_FETCH_USER_UNLOCKS_HH
#define RA_API_FETCH_USER_UNLOCKS_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class FetchUserUnlocks
{
public:
    static constexpr const char* const Name() noexcept { return "FetchUserUnlocks"; }

    struct Response : ApiResponseBase
    {
        std::set<unsigned int> UnlockedAchievements;
    };

    struct Request : ApiRequestBase
    {
        unsigned int GameId{ 0U };
        bool Hardcore{ false };

        using Callback = std::function<void(const Response& response)>;

        Response Call() const noexcept;

        void CallAsync(Callback&& callback) const
        {
            ApiRequestBase::CallAsync<Request, Callback>(*this, std::move(callback));
        }
    };
};

} // namespace api
} // namespace ra

#endif // !RA_API_FETCH_USER_UNLOCKS_HH
