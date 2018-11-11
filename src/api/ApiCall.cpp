#include "ApiCall.hh"

#include "api\IServer.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace api {

static ra::api::IServer& Server() noexcept
{
    return ra::services::ServiceLocator::GetMutable<ra::api::IServer>();
}

Login::Response Login::Request::Call() const noexcept
{
    return Server().Login(*this);
}

Logout::Response Logout::Request::Call() const noexcept
{
    return Server().Logout(*this);
}

} // namespace api
} // namespace ra
