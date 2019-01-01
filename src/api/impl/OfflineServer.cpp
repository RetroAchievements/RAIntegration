#include "OfflineServer.hh"

#include "RA_Json.h"

#include "api\impl\ConnectedServer.hh"

#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace api {
namespace impl {

Login::Response OfflineServer::Login(const Login::Request& request) noexcept
{
    
    Login::Response response;
    response.Result = ApiResult::Success;
    response.Username = request.Username;
    response.ApiToken = "offlineToken";
    return std::move(response);
}

Logout::Response OfflineServer::Logout(_UNUSED const Logout::Request& /*request*/) noexcept
{
    Logout::Response response;
    response.Result = ApiResult::Success;
    return std::move(response);
}

FetchGameData::Response OfflineServer::FetchGameData(const FetchGameData::Request& request) noexcept
{
    FetchGameData::Response response;
    rapidjson::Document document;

    // see if the data is available in the cache
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pData = pLocalStorage.ReadText(ra::services::StorageItemType::GameData, std::to_wstring(request.GameId));
    if (pData == nullptr)
    {
        response.Result = ApiResult::Failed;
        response.ErrorMessage = ra::StringPrintf("Achievement data for game %u not found in cache", request.GameId);
    }
    else if (!LoadDocument(document, *pData.get()))
    {
        response.Result = ApiResult::Error;
        response.ErrorMessage =
            ra::StringPrintf("%s (%zu)", GetParseError_En(document.GetParseError()), document.GetErrorOffset());
    }
    else
    {
        response.Result = ApiResult::Success;
        ConnectedServer::ProcessGamePatchData(response, document);
    }

    return std::move(response);
}

} // namespace impl
} // namespace api
} // namespace ra
