#include "OfflineServer.hh"

#include "RA_Json.h"

#include "api\impl\ConnectedServer.hh"

#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

#include <rc_api_editor.h>

namespace ra {
namespace api {
namespace impl {

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
        return response;
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
