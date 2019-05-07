#ifndef RA_API_UPDATE_ACHIEVEMENT_HH
#define RA_API_UPDATE_ACHIEVEMENT_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class UpdateAchievement
{
public:
    static constexpr const char* const Name() noexcept { return "UpdateAchievement"; }

    struct Response : ApiResponseBase
    {
        unsigned int AchievementId{ 0U };
    };

    struct Request : ApiRequestBase
    {
        unsigned int AchievementId{ 0U };
        unsigned int GameId{ 0U };
        std::wstring Title;
        std::wstring Description;
        std::string Trigger;
        unsigned int Points{ 0U };
        unsigned int Category{ 0U };
        std::string Badge;

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

#endif // !RA_API_UPDATE_ACHIEVEMENT_HH
