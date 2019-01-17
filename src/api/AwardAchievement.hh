#ifndef RA_API_AWARD_ACHIEVEMENT_HH
#define RA_API_AWARD_ACHIEVEMENT_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class AwardAchievement
{
public:
    static constexpr const char* const Name() noexcept { return "AwardAchievement"; }

    struct Response : ApiResponseBase
    {
        unsigned int NewPlayerScore{ 0U };
    };

    struct Request : ApiRequestBase
    {
        unsigned int AchievementId{ 0U };
        bool Hardcore{ false };
        std::string GameHash;

        using Callback = std::function<void(const Response& response)>;

        Response Call() const noexcept;

        void CallAsync(Callback&& callback) const
        {
            ApiRequestBase::CallAsync<Request, Callback>(*this, std::move(callback));
        }

        void CallAsyncWithRetry(Callback&& callback) const
        {
            ApiRequestBase::CallAsyncWithRetry<Request, Callback>(*this, std::move(callback));
        }
    };
};

} // namespace api
} // namespace ra

#endif // !RA_API_AWARD_ACHIEVEMENT_HH
