#include "ApiCall.hh"

#include "api\IServer.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace api {

GSL_SUPPRESS(f.6) static ra::api::IServer& Server() noexcept /*Service expected to exist*/
{
    return ra::services::ServiceLocator::GetMutable<ra::api::IServer>();
}

Login::Response Login::Request::Call() const { return Server().Login(*this); }
Logout::Response Logout::Request::Call() const { return Server().Logout(*this); }
StartSession::Response StartSession::Request::Call() const { return Server().StartSession(*this); }
Ping::Response Ping::Request::Call() const { return Server().Ping(*this); }

} // namespace api
} // namespace ra
