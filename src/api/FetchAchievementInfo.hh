#ifndef RA_API_FETCH_ACHIEVEMENT_INFO_HH
#define RA_API_FETCH_ACHIEVEMENT_INFO_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class FetchAchievementInfo
{
public:
    static constexpr const char* const Name() noexcept { return "FetchAchievementInfo"; }

    struct Response : ApiResponseBase
    {
        unsigned int GameId{ 0U };
        unsigned int EarnedBy{ 0U };
        unsigned int NumPlayers{ 0U };

        struct Entry
        {
            std::string User;
            unsigned int Points{ 0U };
            time_t DateAwarded{};
        };

        std::vector<Entry> Entries;
    };

    struct Request : ApiRequestBase
    {
        unsigned int AchievementId{ 0U };
        unsigned int FirstEntry{ 1U };
        unsigned int NumEntries{ 10U };
        bool FriendsOnly{ false };

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

#endif // !RA_API_FETCH_ACHIEVEMENT_INFO_HH
