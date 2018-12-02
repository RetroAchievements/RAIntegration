#include "OfflineServer.hh"

namespace ra {
namespace api {
namespace impl {

Login::Response OfflineServer::Login(const Login::Request& request)
{
    
    Login::Response response;
    response.Result = ApiResult::Success;
    response.Username = request.Username;
    response.ApiToken = "offlineToken";
    return std::move(response);
}

Logout::Response OfflineServer::Logout(_UNUSED const Logout::Request& request) noexcept
{
    Logout::Response response;
    response.Result = ApiResult::Success;
    return std::move(response);
}

} // namespace impl
} // namespace api
} // namespace ra
