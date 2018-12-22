#include "ApiCall.hh"

#include "api\IServer.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace api {
/* clang-format off */
GSL_SUPPRESS(f.6)
/* clang-format on */
static ra::api::IServer& Server() noexcept
{
    return ra::services::ServiceLocator::GetMutable<ra::api::IServer>();
}

Login::Response Login::Request::Call() const noexcept { return Server().Login(*this); }
Logout::Response Logout::Request::Call() const noexcept { return Server().Logout(*this); }
StartSession::Response StartSession::Request::Call() const noexcept { return Server().StartSession(*this); }
Ping::Response Ping::Request::Call() const noexcept { return Server().Ping(*this); }
ResolveHash::Response ResolveHash::Request::Call() const noexcept { return Server().ResolveHash(*this); }

} // namespace api
} // namespace ra
