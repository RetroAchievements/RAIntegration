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
    auto response = serverApi->Login(request);

    // if successful, update the global IServer instance to the connected API
    if (response.Result == ApiResult::Success)
        ra::services::ServiceLocator::Provide<ra::api::IServer>(std::move(serverApi));

    // pass the server API response back to the caller
    return response;
}

LatestClient::Response DisconnectedServer::LatestClient(const LatestClient::Request& request) noexcept
{
    // LatestClient call doesn't require being logged in. Dispatch to the ConnectedServer::LatestClient method.
    ConnectedServer serverApi(m_sHost);
    return serverApi.LatestClient(request);
}

} // namespace impl
} // namespace api
} // namespace ra

