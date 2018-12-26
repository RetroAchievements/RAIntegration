#include "ApiCall.hh"

#include "ra_fwd.h"

#include "api\IServer.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace api {

static ra::api::IServer& Server() noexcept
{
    GSL_SUPPRESS_F6
    return ra::services::ServiceLocator::GetMutable<ra::api::IServer>();
}

Login::Response Login::Request::Call() const noexcept { return Server().Login(*this); }
Logout::Response Logout::Request::Call() const noexcept { return Server().Logout(*this); }
StartSession::Response StartSession::Request::Call() const noexcept { return Server().StartSession(*this); }
Ping::Response Ping::Request::Call() const noexcept { return Server().Ping(*this); }
ResolveHash::Response ResolveHash::Request::Call() const noexcept { return Server().ResolveHash(*this); }
LatestClient::Response LatestClient::Request::Call() const noexcept { return Server().LatestClient(*this); }

} // namespace api
} // namespace ra
