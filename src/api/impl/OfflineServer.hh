#pragma once

#include "ServerBase.hh"

namespace ra {
namespace api {
namespace impl {

class OfflineServer : public ServerBase
{
public:
    const char* Name() const noexcept override { return "offline client"; }

    GSL_SUPPRESS(f.6) Login::Response Login(const Login::Request& request) noexcept override;
    Logout::Response Logout(_UNUSED const Logout::Request& /*request*/) noexcept override;
};

} // namespace impl
} // namespace api
} // namespace ra
