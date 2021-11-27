#ifndef RA_API_UPDATE_LEADERBOARD_HH
#define RA_API_UPDATE_LEADERBOARD_HH
#pragma once

#include "ApiCall.hh"

#include "data\Types.hh"

namespace ra {
namespace api {

class UpdateLeaderboard
{
public:
    static constexpr const char* const Name() noexcept { return "UpdateLeaderboard"; }

    struct Response : ApiResponseBase
    {
        unsigned int LeaderboardId{ 0U };
    };

    struct Request : ApiRequestBase
    {
        unsigned int LeaderboardId{ 0U };
        unsigned int GameId{ 0U };
        std::wstring Title;
        std::wstring Description;
        std::string StartTrigger;
        std::string SubmitTrigger;
        std::string CancelTrigger;
        std::string ValueDefinition;
        ra::data::ValueFormat Format{ ra::data::ValueFormat::Value };
        bool LowerIsBetter{ false };

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

#endif // !RA_API_UPDATE_LEADERBOARD_HH
