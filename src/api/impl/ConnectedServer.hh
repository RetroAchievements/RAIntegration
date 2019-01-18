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

    GSL_SUPPRESS_F6 Login::Response Login(const Login::Request& request) noexcept override;
    GSL_SUPPRESS_F6 Logout::Response Logout(_UNUSED const Logout::Request& /*request*/) noexcept override;
    GSL_SUPPRESS_F6 StartSession::Response StartSession(const StartSession::Request& request) noexcept override;
    GSL_SUPPRESS_F6 Ping::Response Ping(const Ping::Request& request) noexcept override;
    GSL_SUPPRESS_F6 FetchUserUnlocks::Response FetchUserUnlocks(const FetchUserUnlocks::Request& request) noexcept override;
    GSL_SUPPRESS_F6 AwardAchievement::Response AwardAchievement(const AwardAchievement::Request& request) noexcept override;
    GSL_SUPPRESS_F6 ResolveHash::Response ResolveHash(const ResolveHash::Request& request) noexcept override;
    GSL_SUPPRESS_F6 FetchGameData::Response FetchGameData(const FetchGameData::Request& request) noexcept override;
    GSL_SUPPRESS_F6 LatestClient::Response LatestClient(const LatestClient::Request& request) noexcept override;

    static void ProcessGamePatchData(FetchGameData::Response &response, const rapidjson::Value& PatchData);

private:
    const std::string m_sHost;
};

} // namespace impl
} // namespace api
} // namespace ra
