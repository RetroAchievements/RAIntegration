#include "ApiCall.hh"

#include "api\IServer.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace api {

static ra::api::IServer& Server()
{
    return ra::services::ServiceLocator::GetMutable<ra::api::IServer>();
}

FetchUserFriends::Response FetchUserFriends::Request::Call() const { return Server().FetchUserFriends(*this); }
ResolveHash::Response ResolveHash::Request::Call() const { return Server().ResolveHash(*this); }
FetchCodeNotes::Response FetchCodeNotes::Request::Call() const { return Server().FetchCodeNotes(*this); }
UpdateCodeNote::Response UpdateCodeNote::Request::Call() const { return Server().UpdateCodeNote(*this); }
DeleteCodeNote::Response DeleteCodeNote::Request::Call() const { return Server().DeleteCodeNote(*this); }
UpdateAchievement::Response UpdateAchievement::Request::Call() const { return Server().UpdateAchievement(*this); }
FetchAchievementInfo::Response FetchAchievementInfo::Request::Call() const { return Server().FetchAchievementInfo(*this); }
UpdateLeaderboard::Response UpdateLeaderboard::Request::Call() const { return Server().UpdateLeaderboard(*this); }
FetchLeaderboardInfo::Response FetchLeaderboardInfo::Request::Call() const { return Server().FetchLeaderboardInfo(*this); }
UpdateRichPresence::Response UpdateRichPresence::Request::Call() const { return Server().UpdateRichPresence(*this); }
LatestClient::Response LatestClient::Request::Call() const { return Server().LatestClient(*this); }
SubmitNewTitle::Response SubmitNewTitle::Request::Call() const { return Server().SubmitNewTitle(*this); }
FetchBadgeIds::Response FetchBadgeIds::Request::Call() const { return Server().FetchBadgeIds(*this); }
UploadBadge::Response UploadBadge::Request::Call() const { return Server().UploadBadge(*this); }

} // namespace api
} // namespace ra
