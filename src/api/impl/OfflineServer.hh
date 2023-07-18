#pragma once

#include "ServerBase.hh"

namespace ra {
namespace api {
namespace impl {

class OfflineServer : public ServerBase
{
public:
    const char* Name() const noexcept override { return "offline client"; }

    AwardAchievement::Response AwardAchievement(const AwardAchievement::Request&) override;
    SubmitLeaderboardEntry::Response SubmitLeaderboardEntry(const SubmitLeaderboardEntry::Request&) override;

    FetchGameData::Response FetchGameData(const FetchGameData::Request& request) override;
    FetchCodeNotes::Response FetchCodeNotes(const FetchCodeNotes::Request& request) override;

    LatestClient::Response LatestClient(const LatestClient::Request& request) override;
};

} // namespace impl
} // namespace api
} // namespace ra
