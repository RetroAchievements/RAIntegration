#include "ApiCall.hh"

#include "ra_fwd.h"

#include "api\IServer.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace api {

static ra::api::IServer& Server()
{
    return ra::services::ServiceLocator::GetMutable<ra::api::IServer>();
}

Login::Response Login::Request::Call() const { return Server().Login(*this); }
Logout::Response Logout::Request::Call() const { return Server().Logout(*this); }
StartSession::Response StartSession::Request::Call() const { return Server().StartSession(*this); }
Ping::Response Ping::Request::Call() const { return Server().Ping(*this); }
FetchUserUnlocks::Response FetchUserUnlocks::Request::Call() const { return Server().FetchUserUnlocks(*this); }
AwardAchievement::Response AwardAchievement::Request::Call() const { return Server().AwardAchievement(*this); }
SubmitLeaderboardEntry::Response SubmitLeaderboardEntry::Request::Call() const { return Server().SubmitLeaderboardEntry(*this); }
ResolveHash::Response ResolveHash::Request::Call() const { return Server().ResolveHash(*this); }
FetchGameData::Response FetchGameData::Request::Call() const { return Server().FetchGameData(*this); }
FetchCodeNotes::Response FetchCodeNotes::Request::Call() const { return Server().FetchCodeNotes(*this); }
UpdateCodeNote::Response UpdateCodeNote::Request::Call() const { return Server().UpdateCodeNote(*this); }
DeleteCodeNote::Response DeleteCodeNote::Request::Call() const { return Server().DeleteCodeNote(*this); }
UpdateAchievement::Response UpdateAchievement::Request::Call() const { return Server().UpdateAchievement(*this); }
FetchAchievementInfo::Response FetchAchievementInfo::Request::Call() const { return Server().FetchAchievementInfo(*this); }
FetchLeaderboardInfo::Response FetchLeaderboardInfo::Request::Call() const { return Server().FetchLeaderboardInfo(*this); }
LatestClient::Response LatestClient::Request::Call() const { return Server().LatestClient(*this); }
FetchGamesList::Response FetchGamesList::Request::Call() const { return Server().FetchGamesList(*this); }
SubmitNewTitle::Response SubmitNewTitle::Request::Call() const { return Server().SubmitNewTitle(*this); }
SubmitTicket::Response SubmitTicket::Request::Call() const { return Server().SubmitTicket(*this); }
FetchBadgeIds::Response FetchBadgeIds::Request::Call() const { return Server().FetchBadgeIds(*this); }
UploadBadge::Response UploadBadge::Request::Call() const { return Server().UploadBadge(*this); }

} // namespace api
} // namespace ra
