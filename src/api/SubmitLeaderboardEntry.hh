#ifndef RA_API_SUBMIT_LEADERBOARD_ENTRY_HH
#define RA_API_SUBMIT_LEADERBOARD_ENTRY_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class SubmitLeaderboardEntry
{
public:
    static constexpr const char* const Name() noexcept { return "SubmitLeaderboardEntry"; }

    struct Response : ApiResponseBase
    {
        unsigned int Score{ 0U };
        unsigned int BestScore{ 0U };
        unsigned int NewRank{ 0U };
        unsigned int NumEntries{ 0U };

        struct Entry
        {
            unsigned int Rank{ 0U };
            std::string User;
            unsigned int Score{ 0U };
        };

        std::vector<Entry> TopEntries;
    };

    struct Request : ApiRequestBase
    {
        unsigned int LeaderboardId{ 0U };
        unsigned int Score{ 0U };
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

#endif // !RA_API_SUBMIT_LEADERBOARD_ENTRY_HH
