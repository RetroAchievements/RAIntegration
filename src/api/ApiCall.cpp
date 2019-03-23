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
FetchLeaderboardInfo::Response FetchLeaderboardInfo::Request::Call() const { return Server().FetchLeaderboardInfo(*this); }
LatestClient::Response LatestClient::Request::Call() const { return Server().LatestClient(*this); }
FetchGamesList::Response FetchGamesList::Request::Call() const { return Server().FetchGamesList(*this); }
SubmitNewTitle::Response SubmitNewTitle::Request::Call() const { return Server().SubmitNewTitle(*this); }

} // namespace api
} // namespace ra
