#pragma once

#include "OfflineServer.hh"

namespace ra {
namespace api {
namespace impl {

class DisconnectedServer : public OfflineServer
{
public:
    explicit DisconnectedServer(const std::string& sHost) : m_sHost(sHost) {}

    const char* Name() const noexcept override { return "disconnected client"; }

    GSL_SUPPRESS_F6 Login::Response Login(const Login::Request& request) noexcept override;
    const std::string& Host() const noexcept { return m_sHost; }

    GSL_SUPPRESS_F6 LatestClient::Response LatestClient(const LatestClient::Request& request) noexcept override;

private:
    const std::string m_sHost;
};

} // namespace impl
} // namespace api
} // namespace ra
