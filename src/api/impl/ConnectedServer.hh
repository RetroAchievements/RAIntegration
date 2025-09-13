#pragma once

#include "ServerBase.hh"

#include "services\Http.hh"

namespace ra {
namespace api {
namespace impl {

class ConnectedServer : public ServerBase
{
public:
    explicit ConnectedServer(const std::string& sHost);

    const char* Name() const noexcept override { return m_sHost.c_str(); }

    FetchUserFriends::Response FetchUserFriends(const FetchUserFriends::Request& request) override;
    ResolveHash::Response ResolveHash(const ResolveHash::Request& request) override;
    FetchCodeNotes::Response FetchCodeNotes(const FetchCodeNotes::Request& request) override;
    UpdateCodeNote::Response UpdateCodeNote(const UpdateCodeNote::Request& request) override;
    DeleteCodeNote::Response DeleteCodeNote(const DeleteCodeNote::Request& request) override;
    UpdateAchievement::Response UpdateAchievement(const UpdateAchievement::Request& request) override;
    FetchAchievementInfo::Response FetchAchievementInfo(const FetchAchievementInfo::Request& request) override;
    UpdateLeaderboard::Response UpdateLeaderboard(const UpdateLeaderboard::Request& request) override;
    FetchLeaderboardInfo::Response FetchLeaderboardInfo(const FetchLeaderboardInfo::Request& request) override;
    UpdateRichPresence::Response UpdateRichPresence(const UpdateRichPresence::Request& request) override;
    LatestClient::Response LatestClient(const LatestClient::Request& request) override;
    FetchGamesList::Response FetchGamesList(const FetchGamesList::Request& request) override;
    SubmitNewTitle::Response SubmitNewTitle(const SubmitNewTitle::Request& request) override;
    FetchBadgeIds::Response FetchBadgeIds(const FetchBadgeIds::Request& request) override;
    UploadBadge::Response UploadBadge(const UploadBadge::Request& request) override;

    static void ProcessCodeNotes(FetchCodeNotes::Response &response, const void* api_response);

private:
    const std::string m_sHost;
};

} // namespace impl
} // namespace api
} // namespace ra
