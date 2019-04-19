#ifndef RA_API_ISERVER_HH
#define RA_API_ISERVER_HH
#pragma once

#include "api/AwardAchievement.hh"
#include "api/FetchAchievementInfo.hh"
#include "api/FetchCodeNotes.hh"
#include "api/FetchGameData.hh"
#include "api/FetchGamesList.hh"
#include "api/FetchLeaderboardInfo.hh"
#include "api/FetchUserUnlocks.hh"
#include "api/LatestClient.hh"
#include "api/Login.hh"
#include "api/Logout.hh"
#include "api/Ping.hh"
#include "api/ResolveHash.hh"
#include "api/StartSession.hh"
#include "api/SubmitLeaderboardEntry.hh"
#include "api/SubmitNewTitle.hh"
#include "api/SubmitTicket.hh"

namespace ra {
namespace api {

class IServer
{
public:
    virtual const char* Name() const noexcept = 0;

    // === user functions ===
    virtual Login::Response Login(const Login::Request& request) = 0;
    virtual Logout::Response Logout(const Logout::Request& request) = 0;
    virtual StartSession::Response StartSession(const StartSession::Request& request) = 0;
    virtual Ping::Response Ping(const Ping::Request& request) = 0;
    virtual FetchUserUnlocks::Response FetchUserUnlocks(const FetchUserUnlocks::Request& request) = 0;
    virtual AwardAchievement::Response AwardAchievement(const AwardAchievement::Request& request) = 0;
    virtual SubmitLeaderboardEntry::Response SubmitLeaderboardEntry(const SubmitLeaderboardEntry::Request& request) = 0;

    // === game functions ===
    virtual ResolveHash::Response ResolveHash(const ResolveHash::Request& request) = 0;
    virtual FetchGameData::Response FetchGameData(const FetchGameData::Request& request) = 0;
    virtual FetchCodeNotes::Response FetchCodeNotes(const FetchCodeNotes::Request& request) = 0;
    virtual FetchAchievementInfo::Response FetchAchievementInfo(const FetchAchievementInfo::Request& request) = 0;
    virtual FetchLeaderboardInfo::Response FetchLeaderboardInfo(const FetchLeaderboardInfo::Request& request) = 0;

    // === other functions ===
    virtual LatestClient::Response LatestClient(const LatestClient::Request& request) = 0;
    virtual FetchGamesList::Response FetchGamesList(const FetchGamesList::Request& request) = 0;
    virtual SubmitNewTitle::Response SubmitNewTitle(const SubmitNewTitle::Request& request) = 0;
    virtual SubmitTicket::Response SubmitTicket(const SubmitTicket::Request& request) = 0;

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
