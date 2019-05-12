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

    Login::Response Login(const Login::Request& request) override;
    const std::string& Host() const noexcept { return m_sHost; }

    LatestClient::Response LatestClient(const LatestClient::Request& request) override;

private:
    const std::string m_sHost;
};

} // namespace impl
} // namespace api
} // namespace ra
