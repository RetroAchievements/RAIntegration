#include "DisconnectedServer.hh"

#include "api\impl\ConnectedServer.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace api {
namespace impl {

Login::Response DisconnectedServer::Login(const Login::Request& request) noexcept
{
    // use the normal ServerApi to attempt to connect
    auto serverApi = std::make_unique<ConnectedServer>(m_sHost);
    Login::Response response = serverApi->Login(request);

    // if successful, update the global IServer instance to the connected API
    if (response.Result == ApiResult::Success)
        ra::services::ServiceLocator::Provide<ra::api::IServer>(std::move(serverApi));

    // pass the server API response back to the caller
    return response;
}

} // namespace impl
} // namespace api
} // namespace ra

