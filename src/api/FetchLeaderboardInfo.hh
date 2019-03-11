#ifndef RA_API_FETCH_LEADERBOARD_INFO_HH
#define RA_API_FETCH_LEADERBOARD_INFO_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class FetchLeaderboardInfo
{
public:
    static constexpr const char* const Name() noexcept { return "FetchLeaderboardInfo"; }

    struct Response : ApiResponseBase
    {
        unsigned int GameId{ 0U };
        unsigned int ConsoleId{ 0U };
        bool LowerIsBetter{ false };

        struct Entry
        {
            unsigned int Rank{ 0U };
            std::string User;
            unsigned int Score{ 0U };
            time_t DateSubmitted{};
        };

        std::vector<Entry> Entries;
    };

    struct Request : ApiRequestBase
    {
        unsigned int LeaderboardId{ 0U };
        unsigned int FirstEntry{ 0U };
        unsigned int NumEntries{ 0U };

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

#endif // !RA_API_FETCH_LEADERBOARD_INFO_HH
