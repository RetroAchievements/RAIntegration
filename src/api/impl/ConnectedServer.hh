#pragma once

#include "ServerBase.hh"

namespace ra {
namespace api {
namespace impl {

class ConnectedServer : public ServerBase
{
public:
    explicit ConnectedServer(const std::string& sHost) : m_sHost(sHost) {}

    const char* Name() const noexcept override { return m_sHost.c_str(); }

    Login::Response Login(const Login::Request& request) override;
    Logout::Response Logout(const Logout::Request&) override;
    StartSession::Response StartSession(const StartSession::Request& request) override;
    Ping::Response Ping(const Ping::Request& request) override;
    FetchUserUnlocks::Response FetchUserUnlocks(const FetchUserUnlocks::Request& request) override;
    AwardAchievement::Response AwardAchievement(const AwardAchievement::Request& request) override;
    SubmitLeaderboardEntry::Response SubmitLeaderboardEntry(const SubmitLeaderboardEntry::Request& request) override;
    ResolveHash::Response ResolveHash(const ResolveHash::Request& request) override;
    FetchGameData::Response FetchGameData(const FetchGameData::Request& request) override;
    FetchCodeNotes::Response FetchCodeNotes(const FetchCodeNotes::Request& request) override;
    UpdateCodeNote::Response UpdateCodeNote(const UpdateCodeNote::Request& request) override;
    DeleteCodeNote::Response DeleteCodeNote(const DeleteCodeNote::Request& request) override;
    FetchAchievementInfo::Response FetchAchievementInfo(const FetchAchievementInfo::Request& request) override;
    FetchLeaderboardInfo::Response FetchLeaderboardInfo(const FetchLeaderboardInfo::Request& request) override;
    LatestClient::Response LatestClient(const LatestClient::Request& request) override;
    FetchGamesList::Response FetchGamesList(const FetchGamesList::Request& request) override;
    SubmitNewTitle::Response SubmitNewTitle(const SubmitNewTitle::Request& request) override;
    SubmitTicket::Response SubmitTicket(const SubmitTicket::Request& request) override;

    static void ProcessGamePatchData(FetchGameData::Response &response, const rapidjson::Value& PatchData);
    static void ProcessCodeNotes(FetchCodeNotes::Response &response, const rapidjson::Value& CodeNotes);

private:
    const std::string m_sHost;
};

} // namespace impl
} // namespace api
} // namespace ra
