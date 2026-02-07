#ifndef RA_API_ISERVER_HH
#define RA_API_ISERVER_HH
#pragma once

#include "api/DeleteCodeNote.hh"
#include "api/FetchAchievementInfo.hh"
#include "api/FetchBadgeIds.hh"
#include "api/FetchCodeNotes.hh"
#include "api/FetchLeaderboardInfo.hh"
#include "api/FetchUserFriends.hh"
#include "api/LatestClient.hh"
#include "api/ResolveHash.hh"
#include "api/SubmitTicket.hh"
#include "api/UpdateAchievement.hh"
#include "api/UpdateCodeNote.hh"
#include "api/UpdateLeaderboard.hh"
#include "api/UpdateRichPresence.hh"
#include "api/UploadBadge.hh"

namespace ra {
namespace api {

class IServer
{
public:
    virtual const char* Name() const noexcept = 0;

    // === user functions ===
    virtual FetchUserFriends::Response FetchUserFriends(const FetchUserFriends::Request& request) = 0;

    // === game functions ===
    virtual ResolveHash::Response ResolveHash(const ResolveHash::Request& request) = 0;
    virtual FetchCodeNotes::Response FetchCodeNotes(const FetchCodeNotes::Request& request) = 0;
    virtual UpdateCodeNote::Response UpdateCodeNote(const UpdateCodeNote::Request& request) = 0;
    virtual DeleteCodeNote::Response DeleteCodeNote(const DeleteCodeNote::Request& request) = 0;
    virtual UpdateAchievement::Response UpdateAchievement(const UpdateAchievement::Request& request) = 0;
    virtual FetchAchievementInfo::Response FetchAchievementInfo(const FetchAchievementInfo::Request& request) = 0;
    virtual UpdateLeaderboard::Response UpdateLeaderboard(const UpdateLeaderboard::Request& request) = 0;
    virtual FetchLeaderboardInfo::Response FetchLeaderboardInfo(const FetchLeaderboardInfo::Request& request) = 0;
    virtual UpdateRichPresence::Response UpdateRichPresence(const UpdateRichPresence::Request& request) = 0;

    // === other functions ===
    virtual LatestClient::Response LatestClient(const LatestClient::Request& request) = 0;
    virtual FetchBadgeIds::Response FetchBadgeIds(const FetchBadgeIds::Request& request) = 0;
    virtual UploadBadge::Response UploadBadge(const UploadBadge::Request& request) = 0;

protected:
    IServer() noexcept = default;

public:
    virtual ~IServer() noexcept = default;
    IServer(const IServer&) = delete;
    IServer& operator=(const IServer&) = delete;
    IServer(IServer&&) noexcept = delete;
    IServer& operator=(IServer&&) noexcept = delete;
};

} // namespace api
} // namespace ra

#endif !RA_API_ISERVER_HH
