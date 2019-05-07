#ifndef SERVERBASE_HH
#define SERVERBASE_HH
#pragma once

#include "api/IServer.hh"

#include "RA_StringUtils.h"
#include "RA_StringUtils.h"

#include <string>

namespace ra {
namespace api {
namespace impl {

class ServerBase : public IServer
{
public:
    // === user functions ===
    Login::Response Login(const Login::Request&) override
    {
        return UnsupportedApi<Login::Response>(Login::Name());
    }

    Logout::Response Logout(const Logout::Request&) override
    {
        return UnsupportedApi<Logout::Response>(Logout::Name());
    }

    StartSession::Response StartSession(const StartSession::Request&) override
    {
        return UnsupportedApi<StartSession::Response>(StartSession::Name());
    }

    Ping::Response Ping(const Ping::Request&) override
    {
        return UnsupportedApi<Ping::Response>(Ping::Name());
    }

    FetchUserUnlocks::Response FetchUserUnlocks(const FetchUserUnlocks::Request&) override
    {
        return UnsupportedApi<FetchUserUnlocks::Response>(FetchUserUnlocks::Name());
    }

    AwardAchievement::Response AwardAchievement(const AwardAchievement::Request&) override
    {
        return UnsupportedApi<AwardAchievement::Response>(AwardAchievement::Name());
    }

    SubmitLeaderboardEntry::Response SubmitLeaderboardEntry(const SubmitLeaderboardEntry::Request&) override
    {
        return UnsupportedApi<SubmitLeaderboardEntry::Response>(SubmitLeaderboardEntry::Name());
    }

    // === game functions ===

    ResolveHash::Response ResolveHash(const ResolveHash::Request&) override
    {
        return UnsupportedApi<ResolveHash::Response>(ResolveHash::Name());
    }

    FetchGameData::Response FetchGameData(const FetchGameData::Request&) override
    {
        return UnsupportedApi<FetchGameData::Response>(FetchGameData::Name());
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

    FetchLeaderboardInfo::Response FetchLeaderboardInfo(const FetchLeaderboardInfo::Request&) override
    {
        return UnsupportedApi<FetchLeaderboardInfo::Response>(FetchLeaderboardInfo::Name());
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

    SubmitTicket::Response SubmitTicket(const SubmitTicket::Request&) override
    {
        return UnsupportedApi<SubmitTicket::Response>(SubmitTicket::Name());
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
    inline typename TResponse UnsupportedApi(const char* const restrict apiName) const
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
