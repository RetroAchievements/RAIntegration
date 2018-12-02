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
    Logout::Response Logout(_UNUSED const Logout::Request& request) override;
    StartSession::Response StartSession(_UNUSED const StartSession::Request& request) override;
    Ping::Response Ping(_UNUSED const Ping::Request& request) override;

private:
    const std::string m_sHost;
};

} // namespace impl
} // namespace api
} // namespace ra
