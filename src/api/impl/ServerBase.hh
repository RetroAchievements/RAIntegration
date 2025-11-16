#ifndef SERVERBASE_HH
#define SERVERBASE_HH
#pragma once

#include "api/IServer.hh"

#include "util\Strings.hh"

#include <string>

namespace ra {
namespace api {
namespace impl {

class ServerBase : public IServer
{
public:
    // === user functions ===

    FetchUserFriends::Response FetchUserFriends(const FetchUserFriends::Request&) override
    {
        return UnsupportedApi<FetchUserFriends::Response>(FetchUserFriends::Name());
    }

    // === game functions ===

    ResolveHash::Response ResolveHash(const ResolveHash::Request&) override
    {
        return UnsupportedApi<ResolveHash::Response>(ResolveHash::Name());
    }

    FetchCodeNotes::Response FetchCodeNotes(const FetchCodeNotes::Request&) override
    {
        return UnsupportedApi<FetchCodeNotes::Response>(FetchCodeNotes::Name());
    }

    UpdateCodeNote::Response UpdateCodeNote(const UpdateCodeNote::Request&) override
    {
        return UnsupportedApi<UpdateCodeNote::Response>(UpdateCodeNote::Name());
    }

    DeleteCodeNote::Response DeleteCodeNote(const DeleteCodeNote::Request&) override
    {
        return UnsupportedApi<DeleteCodeNote::Response>(DeleteCodeNote::Name());
    }

    UpdateAchievement::Response UpdateAchievement(const UpdateAchievement::Request&) override
    {
        return UnsupportedApi<UpdateAchievement::Response>(UpdateAchievement::Name());
    }

    FetchAchievementInfo::Response FetchAchievementInfo(const FetchAchievementInfo::Request&) override
    {
        return UnsupportedApi<FetchAchievementInfo::Response>(FetchAchievementInfo::Name());
    }

    UpdateLeaderboard::Response UpdateLeaderboard(const UpdateLeaderboard::Request&) override
    {
        return UnsupportedApi<UpdateLeaderboard::Response>(UpdateLeaderboard::Name());
    }

    FetchLeaderboardInfo::Response FetchLeaderboardInfo(const FetchLeaderboardInfo::Request&) override
    {
        return UnsupportedApi<FetchLeaderboardInfo::Response>(FetchLeaderboardInfo::Name());
    }

    UpdateRichPresence::Response UpdateRichPresence(const UpdateRichPresence::Request&) override
    {
        return UnsupportedApi<UpdateRichPresence::Response>(UpdateRichPresence::Name());
    }

    // === other functions ===

    LatestClient::Response LatestClient(const LatestClient::Request&) override
    {
        return UnsupportedApi<LatestClient::Response>(LatestClient::Name());
    }

    FetchGamesList::Response FetchGamesList(const FetchGamesList::Request&) override
    {
        return UnsupportedApi<FetchGamesList::Response>(FetchGamesList::Name());
    }

    SubmitNewTitle::Response SubmitNewTitle(const SubmitNewTitle::Request&) override
    {
        return UnsupportedApi<SubmitNewTitle::Response>(SubmitNewTitle::Name());
    }

    FetchBadgeIds::Response FetchBadgeIds(const FetchBadgeIds::Request&) override
    {
        return UnsupportedApi<FetchBadgeIds::Response>(FetchBadgeIds::Name());
    }

    UploadBadge::Response UploadBadge(const UploadBadge::Request&) override
    {
        return UnsupportedApi<UploadBadge::Response>(UploadBadge::Name());
    }

protected:
    template<typename TResponse>
    inline typename TResponse UnsupportedApi(const char* const _RESTRICT apiName) const
    {
        static_assert(std::is_base_of<ApiResponseBase, TResponse>::value, "TResponse must derive from ApiResponseBase");

        TResponse response;
        response.Result = ApiResult::Unsupported;
        response.ErrorMessage = ra::StringPrintf("%s is not supported by %s.", apiName, Name());
        return response;
    }
};

} // namespace impl
} // namespace api
} // namespace ra

#endif /* !SERVERBASE_HH */
