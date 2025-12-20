#include "OfflineRcClient.hh"

#include "services\GameIdentifier.hh"
#include "services\ILocalStorage.hh"
#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

#include "util\Log.hh"

#include <rcheevos\src\rc_client_internal.h>

// TODO: Move AchievementRuntimeTests::ServerCallOffline* when moving this class.

namespace ra {
namespace services {
namespace impl {

static std::string GetParam(const ra::services::Http::Request& httpRequest, const std::string& sParam)
{
    std::string sScan = "&" + sParam + "=";
    auto nIndex = httpRequest.GetPostData().find(sScan);

    if (nIndex == std::string::npos)
    {
        const auto nLen = sScan.length() - 1;
        if (httpRequest.GetPostData().length() < nLen)
            return "";

        if (httpRequest.GetPostData().compare(0, nLen, sScan, 1, nLen) != 0)
            return "";

        nIndex = nLen;
    }
    else
    {
        nIndex += sScan.length();
    }

    auto nIndex2 = httpRequest.GetPostData().find('&', nIndex);
    if (nIndex2 == std::string::npos)
        nIndex2 = httpRequest.GetPostData().length();

    return std::string(httpRequest.GetPostData(), nIndex, nIndex2 - nIndex);
}

static ra::services::Http::Response AchievementDataNotFound(const std::string& sGameId)
{
    return ra::services::Http::Response(ra::services::Http::StatusCode::NotFound,
        ra::StringPrintf("{\"Success\":false,\"Error\":\"Achievement data for game %s not found in cache\"}", sGameId));
}

static ra::services::Http::Response HandleOfflineRequest(const ra::services::Http::Request& httpRequest, const std::string sApi)
{
    if (sApi == "ping")
        return ra::services::Http::Response(ra::services::Http::StatusCode::OK, "{\"Success\":true}");

    if (sApi == "achievementsets")
    {
        const auto sGameHash = GetParam(httpRequest, "m");
        const auto nGameId = ra::services::ServiceLocator::GetMutable<ra::services::GameIdentifier>().IdentifyHash(sGameHash);

        // see if the data is available in the cache
        auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
        auto pData = pLocalStorage.ReadText(ra::services::StorageItemType::GameData, std::to_wstring(nGameId));
        if (pData == nullptr)
            return AchievementDataNotFound(std::to_string(nGameId));

        const auto nSize = pData->GetSize();
        std::string sContents;
        sContents.resize(nSize + 1);
        GSL_SUPPRESS_TYPE1 pData->GetBytes(reinterpret_cast<uint8_t*>(sContents.data()), nSize);

        if (sContents.find(",\"Sets\":[") == std::string::npos) // ignore patch response - only return achievementsets response
            return AchievementDataNotFound(std::to_string(nGameId));

        return ra::services::Http::Response(ra::services::Http::StatusCode::OK, sContents);
    }

    if (sApi == "patch")
    {
        const auto sGameId = GetParam(httpRequest, "g");

        // see if the data is available in the cache
        auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
        auto pData = pLocalStorage.ReadText(ra::services::StorageItemType::GameData, ra::Widen(sGameId));
        if (pData == nullptr)
            return AchievementDataNotFound(sGameId);

        std::string sContents = "{\"Success\":true,\"PatchData\":";
        std::string sLine;
        while (pData->GetLine(sLine))
            sContents.append(sLine);
        sContents.push_back('}');

        return ra::services::Http::Response(ra::services::Http::StatusCode::OK, sContents);
    }

    if (sApi == "startsession")
        return ra::services::Http::Response(ra::services::Http::StatusCode::OK, "{\"Success\":true}");

    return ra::services::Http::Response(ra::services::Http::StatusCode::NotImplemented,
        ra::StringPrintf("{\"Success\":false,\"Error\":\"No offline implementation for %s\"}", sApi));
}

void OfflineRcClient::CallApi(const std::string& sApi, const ra::services::Http::Request& pRequest,
    std::function<void(const rc_api_server_response_t&, void*)> fCallback,
    void* pCallbackData) const
{
    ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().RunAsync(
        [httpRequest = pRequest, fCallback, pCallbackData, sApi]()
        {
            ra::services::Http::Response httpResponse = HandleOfflineRequest(httpRequest, sApi);

            if (ra::services::ServiceLocator::Exists<ra::services::ILogger>())
            {
                RA_LOG_INFO("<< %s response (offline) (%d): %s", sApi.c_str(), ra::etoi(httpResponse.StatusCode()), httpResponse.Content().c_str());
            }

            rc_api_server_response_t pResponse;
            memset(&pResponse, 0, sizeof(pResponse));
            pResponse.http_status_code = ra::etoi(httpResponse.StatusCode());
            pResponse.body = httpResponse.Content().c_str();
            pResponse.body_length = httpResponse.Content().length();

            fCallback(pResponse, pCallbackData);
        });
}

} // namespace impl
} // namespace services
} // namespace ra
