#include "OfflineServer.hh"

#include "RA_Json.h"

#include "api\impl\ConnectedServer.hh"

#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

#include <rc_api_editor.h>

namespace ra {
namespace api {
namespace impl {

Login::Response OfflineServer::Login(const Login::Request& request)
{
    Login::Response response;
    response.Result = ApiResult::Success;
    response.Username = request.Username;
    response.ApiToken = "offlineToken";
    return response;
}

Logout::Response OfflineServer::Logout(const Logout::Request&) noexcept
{
    Logout::Response response;
    response.Result = ApiResult::Success;
    return response;
}

FetchGameData::Response OfflineServer::FetchGameData(const FetchGameData::Request& request)
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
    else 
    {
        std::string sContents = "{\"Success\":true,\"PatchData\":";
        std::string sLine;
        while (pData->GetLine(sLine))
        {
            if (!sContents.empty())
                sContents.push_back('\n');

            sContents.append(sLine);
        }
        sContents.push_back('}');

        ra::services::Http::Response httpResponse(ra::services::Http::StatusCode::OK, sContents);
        ConnectedServer::ProcessGamePatchData(response, httpResponse);
    }

    return response;
}

FetchCodeNotes::Response OfflineServer::FetchCodeNotes(const FetchCodeNotes::Request& request)
{
    FetchCodeNotes::Response response;
    rapidjson::Document document;

    // see if the data is available in the cache
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pData = pLocalStorage.ReadText(ra::services::StorageItemType::CodeNotes, std::to_wstring(request.GameId));
    if (pData == nullptr)
    {
        response.Result = ApiResult::Failed;
        response.ErrorMessage = ra::StringPrintf("Code notes for game %u not found in cache", request.GameId);
    }

    std::string sNotes;
    if (!pData->GetLine(sNotes)) // ASSERT: entire JSON block is a single line
    {
        response.Result = ApiResult::Error;
        response.ErrorMessage =
            ra::StringPrintf("%s (%zu)", GetParseError_En(document.GetParseError()), document.GetErrorOffset());
    }
    else
    {
        response.Result = ApiResult::Success;
        sNotes.insert(0, "{\"Success\": true,\"CodeNotes\":");
        sNotes.push_back('}');

        rc_api_fetch_code_notes_response_t api_response;
        const auto nResult = rc_api_process_fetch_code_notes_response(&api_response, sNotes.c_str());
        if (nResult == RC_OK)
            ConnectedServer::ProcessCodeNotes(response, &api_response);

        rc_api_destroy_fetch_code_notes_response(&api_response);
    }

    return response;
}

LatestClient::Response OfflineServer::LatestClient(const LatestClient::Request& request)
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
