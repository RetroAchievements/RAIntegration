#include "DisconnectedServer.hh"

#include "api\impl\ConnectedServer.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace api {
namespace impl {

LatestClient::Response DisconnectedServer::LatestClient(const LatestClient::Request& request)
{
    // LatestClient call doesn't require being logged in. Dispatch to the ConnectedServer::LatestClient method.
    ConnectedServer serverApi(m_sHost);
    return serverApi.LatestClient(request);
}

} // namespace impl
} // namespace api
} // namespace ra

