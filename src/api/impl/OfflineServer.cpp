#include "OfflineServer.hh"

#include "RA_Json.h"

#include "api\impl\ConnectedServer.hh"

#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

#include <rc_api_editor.h>

namespace ra {
namespace api {
namespace impl {

LatestClient::Response OfflineServer::LatestClient(const LatestClient::Request&)
{
    // all versions are newer than 0.0.0.0, and are therefore valid/allowed
    LatestClient::Response response;
    response.LatestVersion = response.MinimumVersion = "0.0.0.0";
    response.Result = ApiResult::Success;
    return response;
}

} // namespace impl
} // namespace api
} // namespace ra
