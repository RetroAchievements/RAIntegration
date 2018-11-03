#include "ConnectedServer.hh"

#include "DisconnectedServer.hh"
#include "RA_Defs.h"

#include "RA_User.h"

#include "services\Http.hh"
#include "services\ServiceLocator.hh"

#include <future>

#include <rapidjson\document.h>

namespace ra {
namespace api {
namespace impl {

_NODISCARD static bool HandleHttpError(_In_ const ra::services::Http::Response& httpResponse,
                                       _Inout_ ApiResponseBase& pResponse) noexcept
{
    if (httpResponse.StatusCode() != ra::services::Http::StatusCode::OK)
    {
        pResponse.Result = ApiResult::Error;
        pResponse.ErrorMessage = "HTTP error code " + std::to_string(ra::etoi(httpResponse.StatusCode()));
        return true;
    }

    return false;
}

_NODISCARD static bool GetJson(_In_ const char* sApiName, _In_ const ra::services::Http::Response& httpResponse,
                               _Inout_ ApiResponseBase& pResponse, _Out_ rapidjson::Document& pDocument) noexcept
{
    if (HandleHttpError(httpResponse, pResponse))
    {
        pDocument.SetArray();

        RA_LOG_ERR("-- %s: %s", sApiName, pResponse.ErrorMessage.c_str());
        return false;
    }

    if (httpResponse.Content().empty())
    {
        pDocument.SetArray();

        RA_LOG_ERR("-- %s: Empty JSON response", sApiName);
        pResponse.Result = ApiResult::Failed;
        pResponse.ErrorMessage = "Empty JSON response";
        return false;
    }

    RA_LOG_INFO("-- %s Response: %s", sApiName, httpResponse.Content().c_str());

    pDocument.Parse(httpResponse.Content());
    if (pDocument.HasParseError())
    {
        RA_LOG_ERR("-- %s: JSON Parse Error encountered!", sApiName);

        pResponse.Result = ApiResult::Error;
        pResponse.ErrorMessage = std::string(GetParseError_En(pDocument.GetParseError())) + " (" +
                                 std::to_string(pDocument.GetErrorOffset()) + ")";
        return false;
    }

    if (pDocument.HasMember("Error"))
    {
        pResponse.Result = ApiResult::Error;
        pResponse.ErrorMessage = pDocument["Error"].GetString();
        RA_LOG_ERR("-- %s Error: %s", sApiName, pResponse.ErrorMessage.c_str());
        return false;
    }

    if (pDocument.HasMember("Success") && !pDocument["Success"].GetBool())
    {
        pResponse.Result = ApiResult::Failed;
        RA_LOG_ERR("-- %s Error: Success=false", sApiName);
        return false;
    }

    return true;
}

static void GetRequiredJsonField(_Out_ std::string& sValue, _In_ const rapidjson::Document& pDocument,
                                 _In_ const char* const sField, _Inout_ ApiResponseBase& response) noexcept
{
    if (!pDocument.HasMember(sField))
    {
        sValue.clear();

        response.Result = ApiResult::Error;
        if (response.ErrorMessage.empty())
            response.ErrorMessage = std::string(sField) + " not found in response";
    }
    else
    {
        sValue = pDocument[sField].GetString();
    }
}

static void GetOptionalJsonField(_Out_ unsigned int& nValue, _In_ const rapidjson::Document& pDocument,
                                 _In_ const char* sField, _In_ unsigned int nDefaultValue = 0) noexcept
{
    if (pDocument.HasMember(sField))
        nValue = pDocument[sField].GetUint();
    else
        nValue = nDefaultValue;
}

static void AppendUrlParam(_Inout_ std::string& sParams, _In_ const char* const sParam,
                           _In_ const std::string& sValue) noexcept
{
    if (!sParams.empty() && sParams.back() != '?')
        sParams.push_back('&');

    sParams.append(sParam);
    sParams.push_back('=');
    ra::services::Http::UrlEncodeAppend(sParams, sValue);
}

Login::Response ConnectedServer::Login(const Login::Request& request) noexcept
{
    ra::services::Http::Request httpRequest(m_sHost + "/login_app.php");

    std::string sPostData;
    AppendUrlParam(sPostData, "u", request.Username);
    if (!request.ApiToken.empty())
    {
        AppendUrlParam(sPostData, "t", request.ApiToken);
        RA_LOG_INFO("%s Request: %s", Login::Name(), sPostData.c_str());
    }
    else
    {
        // intentionally do not log password
        RA_LOG_INFO("%s Request: %s", Login::Name(), sPostData.c_str());
        AppendUrlParam(sPostData, "p", request.Password);
    }
    httpRequest.SetPostData(sPostData);

    ra::services::Http::Response httpResponse = httpRequest.Call();

    Login::Response response;
    rapidjson::Document document;
    if (GetJson(Login::Name(), httpResponse, response, document))
    {
        response.Result = ApiResult::Success;
        GetRequiredJsonField(response.Username, document, "User", response);
        GetRequiredJsonField(response.ApiToken, document, "Token", response);
        GetOptionalJsonField(response.Score, document, "Score");
        GetOptionalJsonField(response.NumUnreadMessages, document, "Messages");
    }

    return std::move(response);
}

Logout::Response ConnectedServer::Logout(_UNUSED const Logout::Request& request) noexcept
{
    // update the global API pointer to a disconnected API
    ra::services::ServiceLocator::Provide<ra::api::IServer>(new DisconnectedServer(m_sHost));

    Logout::Response response;
    response.Result = ApiResult::Success;
    return std::move(response);
}

static bool DoRequest(const std::string& sHost, const char* sApiName, const char* sRequestName,
                      const std::string& sInputParams, ApiResponseBase& pResponse, rapidjson::Document& document)
{
    std::string sPostData;

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::UserContext>();
    AppendUrlParam(sPostData, "u", pUserContext.GetUsername());
    AppendUrlParam(sPostData, "t", pUserContext.GetApiToken());
    AppendUrlParam(sPostData, "r", sRequestName);
    if (!sInputParams.empty())
    {
        sPostData.push_back('&');
        sPostData.append(sInputParams);
    }
    RA_LOG_INFO("%s Request: %s", sApiName, sPostData.c_str());

    ra::services::Http::Request httpRequest(sHost + "/dorequest.php");
    httpRequest.SetPostData(sPostData);

    const auto httpResponse = httpRequest.Call();
    return GetJson(sApiName, httpResponse, pResponse, document);
}

StartSession::Response ConnectedServer::StartSession(_UNUSED const StartSession::Request& request) noexcept
{
    StartSession::Response response;
    rapidjson::Document document;
    std::string sPostData;

    // activity type enum (only 3 is used )
    // 1 = earned achievement - handled by awardachievement
    // 2 = logged in - handled by login
    // 3 = started playing
    // 4 = uploaded achievement - handled by uploadachievement
    // 5 = modified achievmeent - handled by uploadachievement
    AppendUrlParam(sPostData, "a", "3");

    AppendUrlParam(sPostData, "m", std::to_string(request.GameId));

    if (DoRequest(m_sHost, StartSession::Name(), "postactivity", sPostData, response, document))
        response.Result = ApiResult::Success;

    return std::move(response);
}

Ping::Response ConnectedServer::Ping(_UNUSED const Ping::Request& request) noexcept
{
    Ping::Response response;
    rapidjson::Document document;
    std::string sPostData;

    AppendUrlParam(sPostData, "g", std::to_string(request.GameId));
    if (!request.CurrentActivity.empty())
        AppendUrlParam(sPostData, "m", ra::Narrow(request.CurrentActivity));

    if (DoRequest(m_sHost, Ping::Name(), "ping", sPostData, response, document))
        response.Result = ApiResult::Success;

    return std::move(response);
}

} // namespace impl
} // namespace api
} // namespace ra
