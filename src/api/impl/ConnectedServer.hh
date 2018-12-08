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

    GSL_SUPPRESS(f.6) Login::Response Login(const Login::Request& request) noexcept override;
    GSL_SUPPRESS(f.6) Logout::Response Logout(_UNUSED const Logout::Request& /*request*/) noexcept override;
    GSL_SUPPRESS(f.6) StartSession::Response StartSession(const StartSession::Request& request) noexcept override;
    GSL_SUPPRESS(f.6) Ping::Response Ping(const Ping::Request& request) noexcept override;
    GSL_SUPPRESS(f.6) ResolveHash::Response ResolveHash(const ResolveHash::Request& request) noexcept override;

private:
    const std::string m_sHost;
};

} // namespace impl
} // namespace api
} // namespace ra
