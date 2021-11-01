#include "ConnectedServer.hh"

#include "DisconnectedServer.hh"
#include "RA_Defs.h"

#include "RA_md5factory.h"

#include "data\context\UserContext.hh"

#include "services\Http.hh"
#include "services\IFileSystem.hh"
#include "services\IHttpRequester.hh"
#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

#include <future>

#include <rapidjson\document.h>

#include <rcheevos\src\rapi\rc_api_common.h> // for parsing cached patchdata response
#include <rc_api_info.h>
#include <rc_api_runtime.h>
#include <rc_api_user.h>

namespace ra {
namespace api {
namespace impl {

ConnectedServer::ConnectedServer(const std::string& sHost)
    : m_sHost(sHost)
{
    rc_api_set_host(sHost.c_str());
}

_NODISCARD static bool HandleHttpError(_In_ const ra::services::Http::StatusCode nStatusCode,
                                       _Inout_ ApiResponseBase& pResponse)
{
    if (nStatusCode != ra::services::Http::StatusCode::OK)
    {
        const auto& pHttpRequester = ra::services::ServiceLocator::Get<ra::services::IHttpRequester>();
        const bool bRetry = pHttpRequester.IsRetryable(ra::etoi(nStatusCode));
        pResponse.Result = bRetry ? ApiResult::Incomplete : ApiResult::Error;
        pResponse.ErrorMessage = ra::BuildString("HTTP error code: ", ra::etoi(nStatusCode));

        const auto& pStatusCodeText = pHttpRequester.GetStatusCodeText(ra::etoi(nStatusCode));
        if (!pStatusCodeText.empty())
            pResponse.ErrorMessage = ra::StringPrintf("%s (%s)", pResponse.ErrorMessage, pStatusCodeText);

        return true;
    }

    return false;
}

_NODISCARD static bool GetJson([[maybe_unused]] _In_ const char* sApiName,
                               _In_ const ra::services::Http::Response& httpResponse,
                               _Inout_ ApiResponseBase& pResponse, _Out_ rapidjson::Document& pDocument)
{
    if (httpResponse.Content().empty())
    {
        pDocument.SetArray();

        if (!HandleHttpError(httpResponse.StatusCode(), pResponse))
        {
            pResponse.ErrorMessage = "Empty JSON response";
            pResponse.Result = ApiResult::Failed;
        }

        RA_LOG_ERR("-- %s: %s", sApiName, pResponse.ErrorMessage.c_str());
        return false;
    }

    RA_LOG_INFO("-- %s Response: %s", sApiName, httpResponse.Content());

    pDocument.Parse(httpResponse.Content());
    if (pDocument.HasParseError())
    {
        if (HandleHttpError(httpResponse.StatusCode(), pResponse))
        {
            RA_LOG_ERR("-- %s: %s", sApiName, pResponse.ErrorMessage.c_str());
            return false;
        }

        RA_LOG_ERR("-- %s: JSON Parse Error encountered!", sApiName);

        pResponse.Result = ApiResult::Error;

        if (pDocument.GetParseError() == rapidjson::kParseErrorValueInvalid && pDocument.GetErrorOffset() == 0)
        {
            // server did not return JSON, check for HTML
            if (!ra::StringStartsWith(httpResponse.Content(), "<"))
            {
                // not HTML, return first line of response as the error message
                std::string sContent = httpResponse.Content();
                const auto nIndex = sContent.find('\n');
                if (nIndex != std::string::npos)
                {
                    sContent.resize(nIndex);
                    ra::TrimLineEnding(sContent);
                }

                if (!sContent.empty())
                    pResponse.ErrorMessage = sContent;
            }
        }

        if (pResponse.ErrorMessage.empty())
        {
            pResponse.ErrorMessage =
                ra::StringPrintf("JSON Parse Error: %s (at %zu)", GetParseError_En(pDocument.GetParseError()), pDocument.GetErrorOffset());
        }

        return false;
    }

    if (pDocument.HasMember("Error"))
    {
        pResponse.ErrorMessage = pDocument["Error"].GetString();
        if (!pResponse.ErrorMessage.empty())
        {
            pResponse.Result = ApiResult::Error;
            RA_LOG_ERR("-- %s Error: %s", sApiName, pResponse.ErrorMessage);
            return false;
        }
    }

    if (pDocument.HasMember("Success") && !pDocument["Success"].GetBool())
    {
        pResponse.Result = ApiResult::Failed;
        RA_LOG_ERR("-- %s Error: Success=false", sApiName);
        return false;
    }

    if (HandleHttpError(httpResponse.StatusCode(), pResponse))
    {
        RA_LOG_ERR("-- %s: %s", sApiName, pResponse.ErrorMessage.c_str());
        return false;
    }

    return true;
}

static void GetRequiredJsonField(_Out_ std::string& sValue, _In_ const rapidjson::Value& pDocument,
                                 _In_ const char* const sField, _Inout_ ApiResponseBase& response)
{
    if (!pDocument.HasMember(sField))
    {
        sValue.clear();

        response.Result = ApiResult::Error;
        if (response.ErrorMessage.empty())
            response.ErrorMessage = ra::StringPrintf("%s not found in response", sField);
    }
    else
    {
        auto& pField = pDocument[sField];
        if (pField.IsString())
            sValue = pField.GetString();
        else
            sValue.clear();
    }
}

static void GetOptionalJsonField(_Out_ std::string& sValue, _In_ const rapidjson::Value& pDocument,
    _In_ const char* const sField, _In_ const char* const sDefaultValue = "")
{
    if (pDocument.HasMember(sField))
    {
        auto& pField = pDocument[sField];
        if (pField.IsString())
            sValue = pField.GetString();
        else
            sValue = sDefaultValue;
    }
    else
    {
        sValue = sDefaultValue;
    }
}

static void GetRequiredJsonField(_Out_ std::wstring& sValue, _In_ const rapidjson::Value& pDocument,
    _In_ const char* const sField, _Inout_ ApiResponseBase& response)
{
    if (!pDocument.HasMember(sField))
    {
        sValue.clear();

        response.Result = ApiResult::Error;
        if (response.ErrorMessage.empty())
            response.ErrorMessage = ra::StringPrintf("%s not found in response", sField);
    }
    else
    {
        auto& pField = pDocument[sField];
        if (pField.IsString())
            sValue = ra::Widen(pField.GetString());
        else
            sValue.clear();
    }
}

static void GetRequiredJsonField(_Out_ unsigned int& nValue, _In_ const rapidjson::Value& pDocument,
    _In_ const char* const sField, _Inout_ ApiResponseBase& response)
{
    if (!pDocument.HasMember(sField))
    {
        nValue = 0;

        response.Result = ApiResult::Error;
        if (response.ErrorMessage.empty())
            response.ErrorMessage = ra::StringPrintf("%s not found in response", sField);
    }
    else
    {
        auto& pField = pDocument[sField];
        if (pField.IsUint())
            nValue = pField.GetUint();
        else
            nValue = 0;
    }
}

static void AppendUrlParam(_Inout_ std::string& sParams, _In_ const char* const sParam, _In_ const std::string& sValue)
{
    if (!sParams.empty() && sParams.back() != '?')
        sParams.push_back('&');

    sParams.append(sParam);
    sParams.push_back('=');
    ra::services::Http::UrlEncodeAppend(sParams, sValue);
}

static bool DoRequest(const std::string& sHost, const char* restrict sApiName, const char* restrict sRequestName,
    const std::string& sInputParams, ApiResponseBase& pResponse, rapidjson::Document& document)
{
    std::string sPostData;

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    AppendUrlParam(sPostData, "u", pUserContext.GetUsername());
    AppendUrlParam(sPostData, "r", sRequestName);
    if (!sInputParams.empty())
    {
        sPostData.push_back('&');
        sPostData.append(sInputParams);
    }
    RA_LOG_INFO("%s Request: %s", sApiName, sPostData.c_str());

    // append token after log so it's not in the log
    AppendUrlParam(sPostData, "t", pUserContext.GetApiToken());

    ra::services::Http::Request httpRequest(ra::StringPrintf("%s/dorequest.php", sHost));
    httpRequest.SetPostData(sPostData);

    const auto httpResponse = httpRequest.Call();
    return GetJson(sApiName, httpResponse, pResponse, document);
}

static bool DoRequestWithoutLog(const rc_api_request_t& api_request, _UNUSED const char* sApiName, ra::services::Http::Response& pHttpResponse, ApiResponseBase& pResponse)
{
    ra::services::Http::Request httpRequest(api_request.url);
    httpRequest.SetPostData(api_request.post_data);
    pHttpResponse = httpRequest.Call();

    if (pHttpResponse.Content().empty())
    {
        if (!HandleHttpError(pHttpResponse.StatusCode(), pResponse))
        {
            pResponse.ErrorMessage = "Empty JSON response";
            pResponse.Result = ApiResult::Failed;
        }

        RA_LOG_ERR("-- %s: %s", sApiName, pResponse.ErrorMessage);
        return false;
    }

    RA_LOG_INFO("-- %s Response: %s", sApiName, pHttpResponse.Content());

    switch (pHttpResponse.Content().at(0))
    {
        case '{': // JSON, expected
        case '<': // HTML, not expected
            break;

        default:
            // not HTML or JSON, return the first line of the response as the error message
            const auto nIndex = pHttpResponse.Content().find('\n');
            std::string sContent = (nIndex != std::string::npos) ? pHttpResponse.Content().substr(0, nIndex) : pHttpResponse.Content();
            ra::TrimLineEnding(sContent);

            if (!sContent.empty())
            {
                pResponse.ErrorMessage = sContent;
                pResponse.Result = ApiResult::Error;
            }
            return false;
    }

    return true;
}

static bool DoRequest(const rc_api_request_t& api_request, const char* sApiName, ra::services::Http::Response& pHttpResponse, ApiResponseBase& pResponse)
{
#ifndef RA_UTEST
    const auto& pLogger = ra::services::ServiceLocator::Get<ra::services::ILogger>();
    if (pLogger.IsEnabled(ra::services::LogLevel::Info))
    {
        // capture the GET parameters
        const char* ptr = api_request.url;
        Expects(ptr != nullptr);
        while (*ptr && *ptr != '?')
            ++ptr;

        std::string sParams;
        if (*ptr == '?')
            sParams = ++ptr;

        // capture the POST parameters
        ptr = api_request.post_data;
        if (ptr)
        {
            // don't log the api token
            while (*ptr && (ptr[0] != 't' || ptr[1] != '='))
                ++ptr;

            if (ptr > api_request.post_data)
            {
                if (!sParams.empty())
                    sParams.push_back('&');

                sParams.append(api_request.post_data, ptr - api_request.post_data - 1);
            }

            while (*ptr && ptr[0] != '&')
                ++ptr;

            if (*ptr)
                sParams.append(ptr);
        }

        pLogger.LogMessage(ra::services::LogLevel::Info, ra::StringPrintf("%s Request: %s", sApiName, sParams));
    }
#endif
    return DoRequestWithoutLog(api_request, sApiName, pHttpResponse, pResponse);
}

static bool ValidateResponse(int nResult, const rc_api_response_t& api_response, _UNUSED const char* sApiName, ra::services::Http::StatusCode nStatusCode, ApiResponseBase& pResponse)
{
    if (nResult != RC_OK)
    {
        /* parse error */
        pResponse.ErrorMessage = ra::StringPrintf("JSON Parse Error: %s", rc_error_str(nResult));
        pResponse.Result = ApiResult::Error;
        RA_LOG_ERR("-- %s %s", sApiName, pResponse.ErrorMessage);
        return false;
    }

    if (!api_response.succeeded)
    {
        if (api_response.error_message && *api_response.error_message)
        {
            pResponse.ErrorMessage = api_response.error_message;
            pResponse.Result = ApiResult::Error;
            RA_LOG_ERR("-- %s Error: %s", sApiName, pResponse.ErrorMessage);
        }
        else
        {
            pResponse.Result = ApiResult::Failed;
            RA_LOG_ERR("-- %s Error: Success=false", sApiName, pResponse.ErrorMessage);
        }

        return false;
    }

    if (HandleHttpError(nStatusCode, pResponse))
    {
        RA_LOG_ERR("-- %s: %s", sApiName, pResponse.ErrorMessage.c_str());
        return false;
    }

    return true;
}

static bool DoUpload(const std::string& sHost, const char* restrict sApiName, const char* restrict sRequestName,
    const std::wstring& sFilePath, ApiResponseBase& pResponse, rapidjson::Document& document)
{
    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    const auto nFileSize = pFileSystem.GetFileSize(sFilePath);
    if (nFileSize < 0)
    {
        pResponse.Result = ra::api::ApiResult::Error;
        pResponse.ErrorMessage = ra::StringPrintf("Could not open %s", sFilePath);
        return false;
    }

    std::string sExt = "png";
    const auto nIndex = sFilePath.find_last_of('.');
    if (nIndex != std::string::npos)
    {
        sExt = ra::Narrow(&sFilePath.at(nIndex + 1));
        ra::StringMakeLowercase(sExt);
    }

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    RA_LOG_INFO("%s Request: file=%s (%zu bytes)", sApiName, sFilePath, nFileSize);

    const char* sBoundary = "---------------------------41184676334";
    std::string sPostData;
    sPostData.reserve(gsl::narrow_cast<size_t>(nFileSize + 512));

    sPostData.append("--");
    sPostData.append(sBoundary);
    sPostData.append("\r\n");
    sPostData.append("Content-Disposition: form-data; name=\"u\"\r\n\r\n");
    sPostData.append(pUserContext.GetUsername());
    sPostData.append("\r\n");

    sPostData.append("--");
    sPostData.append(sBoundary);
    sPostData.append("\r\n");
    sPostData.append("Content-Disposition: form-data; name=\"t\"\r\n\r\n");
    sPostData.append(pUserContext.GetApiToken());
    sPostData.append("\r\n");

    // hackish trick - uploadbadgeimage API expects the filename to be uploadbadgeimage.png
    sPostData.append("--");
    sPostData.append(sBoundary);
    sPostData.append("\r\n");
    sPostData.append("Content-Disposition: form-data; name=\"file\"; filename=\"");
    sPostData.append(sRequestName);
    sPostData.append(ra::StringPrintf(".%s\"\r\n", sExt));
    sPostData.append(ra::StringPrintf("Content-Type: image/%s\r\n\r\n", sExt));

    auto pFile = pFileSystem.OpenTextFile(sFilePath);
    if (pFile != nullptr)
    {
        const auto nLength = sPostData.length();
        sPostData.resize(gsl::narrow_cast<size_t>(nFileSize + nLength));
        uint8_t* pBytes;
        GSL_SUPPRESS_TYPE1 pBytes = reinterpret_cast<uint8_t*>(&sPostData.at(nLength));
        pFile->GetBytes(pBytes, gsl::narrow_cast<size_t>(nFileSize));
    }

    sPostData.append("\r\n");
    sPostData.append("--");
    sPostData.append(sBoundary);
    sPostData.append("--\r\n");

    ra::services::Http::Request httpRequest(ra::StringPrintf("%s/doupload.php", sHost));
    httpRequest.SetContentType(ra::StringPrintf("multipart/form-data; boundary=%s", sBoundary));

    httpRequest.SetPostData(sPostData);

    const auto httpResponse = httpRequest.Call();
    return GetJson(sApiName, httpResponse, pResponse, document);
}

// === APIs ===

Login::Response ConnectedServer::Login(const Login::Request& request)
{
    Login::Response response;

    std::string sPostData;
    rc_api_login_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    api_params.username = request.Username.c_str();
    if (!request.ApiToken.empty())
        api_params.api_token = request.ApiToken.c_str();
    else
        api_params.password = request.Password.c_str();

    rc_api_request_t api_request;
    if (rc_api_init_login_request(&api_request, &api_params) == RC_OK)
    {
        RA_LOG_INFO("Login Request: (using %s)", !request.ApiToken.empty() ? "token" : "password");

        ra::services::Http::Response httpResponse;
        if (DoRequestWithoutLog(api_request, Login::Name(), httpResponse, response))
        {
            rc_api_login_response_t api_response;
            const auto nResult = rc_api_process_login_response(&api_response, httpResponse.Content().c_str());

            if (ValidateResponse(nResult, api_response.response, Login::Name(), httpResponse.StatusCode(), response))
            {
                response.Result = ApiResult::Success;
                response.Username = api_response.username;
                response.ApiToken = api_response.api_token;
                response.Score = api_response.score;
                response.NumUnreadMessages = api_response.num_unread_messages;
            }

            rc_api_destroy_login_response(&api_response);
        }
    }

    rc_api_destroy_request(&api_request);

    return response;
}

Logout::Response ConnectedServer::Logout(const Logout::Request&)
{
    // update the global API pointer to a disconnected API
    ra::services::ServiceLocator::Provide<ra::api::IServer>(std::make_unique<DisconnectedServer>(m_sHost));

    Logout::Response response;
    response.Result = ApiResult::Success;
    return response;
}

StartSession::Response ConnectedServer::StartSession(const StartSession::Request& request)
{
    StartSession::Response response;

    std::string sPostData;
    rc_api_start_session_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    api_params.username = pUserContext.GetUsername().c_str();
    api_params.api_token = pUserContext.GetApiToken().c_str();
    api_params.game_id = request.GameId;

    rc_api_request_t api_request;
    if (rc_api_init_start_session_request(&api_request, &api_params) == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, StartSession::Name(), httpResponse, response))
        {
            rc_api_start_session_response_t api_response;
            const auto nResult = rc_api_process_start_session_response(&api_response, httpResponse.Content().c_str());

            if (ValidateResponse(nResult, api_response.response, StartSession::Name(), httpResponse.StatusCode(), response))
                response.Result = ApiResult::Success;

            rc_api_destroy_start_session_response(&api_response);
        }
    }

    rc_api_destroy_request(&api_request);

    return response;
}

Ping::Response ConnectedServer::Ping(const Ping::Request& request)
{
    Ping::Response response;

    std::string sPostData;
    rc_api_ping_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    api_params.username = pUserContext.GetUsername().c_str();
    api_params.api_token = pUserContext.GetApiToken().c_str();
    api_params.game_id = request.GameId;

    std::string sRichPresence;
    if (!request.CurrentActivity.empty())
    {
        sRichPresence = ra::Narrow(request.CurrentActivity);
        api_params.rich_presence = sRichPresence.c_str();
    }

    rc_api_request_t api_request;
    if (rc_api_init_ping_request(&api_request, &api_params) == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, Ping::Name(), httpResponse, response))
        {
            rc_api_ping_response_t api_response;
            const auto nResult = rc_api_process_ping_response(&api_response, httpResponse.Content().c_str());

            if (ValidateResponse(nResult, api_response.response, Ping::Name(), httpResponse.StatusCode(), response))
                response.Result = ApiResult::Success;

            rc_api_destroy_ping_response(&api_response);
        }
    }

    rc_api_destroy_request(&api_request);

    return response;
}

FetchUserUnlocks::Response ConnectedServer::FetchUserUnlocks(const FetchUserUnlocks::Request& request)
{
    FetchUserUnlocks::Response response;

    std::string sPostData;
    rc_api_fetch_user_unlocks_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    api_params.username = pUserContext.GetUsername().c_str();
    api_params.api_token = pUserContext.GetApiToken().c_str();
    api_params.game_id = request.GameId;
    api_params.hardcore = request.Hardcore ? 1 : 0;

    rc_api_request_t api_request;
    if (rc_api_init_fetch_user_unlocks_request(&api_request, &api_params) == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, FetchUserUnlocks::Name(), httpResponse, response))
        {
            rc_api_fetch_user_unlocks_response_t api_response;
            const auto nResult = rc_api_process_fetch_user_unlocks_response(&api_response, httpResponse.Content().c_str());

            if (ValidateResponse(nResult, api_response.response, FetchUserUnlocks::Name(), httpResponse.StatusCode(), response))
            {
                response.Result = ApiResult::Success;

                for (unsigned i = 0; i < api_response.num_achievement_ids; ++i)
                    response.UnlockedAchievements.insert(api_response.achievement_ids[i]);
            }

            rc_api_destroy_fetch_user_unlocks_response(&api_response);
        }
    }

    rc_api_destroy_request(&api_request);

    return response;
}

AwardAchievement::Response ConnectedServer::AwardAchievement(const AwardAchievement::Request& request)
{
    AwardAchievement::Response response;

    std::string sPostData;
    rc_api_award_achievement_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    api_params.username = pUserContext.GetUsername().c_str();
    api_params.api_token = pUserContext.GetApiToken().c_str();

    if (!request.GameHash.empty())
        api_params.game_hash = request.GameHash.c_str();

    api_params.achievement_id = request.AchievementId;
    api_params.hardcore = request.Hardcore ? 1 : 0;

    rc_api_request_t api_request;
    if (rc_api_init_award_achievement_request(&api_request, &api_params) == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, AwardAchievement::Name(), httpResponse, response))
        {
            rc_api_award_achievement_response_t api_response;
            const auto nResult = rc_api_process_award_achievement_response(&api_response, httpResponse.Content().c_str());

            if (ValidateResponse(nResult, api_response.response, AwardAchievement::Name(), httpResponse.StatusCode(), response))
            {
                response.Result = ApiResult::Success;
                response.NewPlayerScore = api_response.new_player_score;
            }

            rc_api_destroy_award_achievement_response(&api_response);
        }
    }

    rc_api_destroy_request(&api_request);

    return response;
}

SubmitLeaderboardEntry::Response ConnectedServer::SubmitLeaderboardEntry(const SubmitLeaderboardEntry::Request& request)
{
    SubmitLeaderboardEntry::Response response;

    std::string sPostData;
    rc_api_submit_lboard_entry_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    api_params.username = pUserContext.GetUsername().c_str();
    api_params.api_token = pUserContext.GetApiToken().c_str();

    api_params.leaderboard_id = request.LeaderboardId;
    api_params.score = request.Score;

    if (!request.GameHash.empty())
        api_params.game_hash = request.GameHash.c_str();

    rc_api_request_t api_request;
    if (rc_api_init_submit_lboard_entry_request(&api_request, &api_params) == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, SubmitLeaderboardEntry::Name(), httpResponse, response))
        {
            rc_api_submit_lboard_entry_response_t api_response;
            const auto nResult = rc_api_process_submit_lboard_entry_response(&api_response, httpResponse.Content().c_str());

            if (ValidateResponse(nResult, api_response.response, SubmitLeaderboardEntry::Name(), httpResponse.StatusCode(), response))
            {
                response.Result = ApiResult::Success;
                response.Score = api_response.submitted_score;
                response.BestScore = api_response.best_score;
                response.NewRank = api_response.new_rank;
                response.NumEntries = api_response.num_entries;

                for (unsigned i = 0; i < api_response.num_top_entries; ++i)
                {
                    const auto* pSrcEntry = &api_response.top_entries[i];
                    auto& entry = response.TopEntries.emplace_back();
                    entry.Rank = pSrcEntry->rank;
                    entry.Score = pSrcEntry->score;
                    entry.User = pSrcEntry->username;
                }
            }

            rc_api_destroy_submit_lboard_entry_response(&api_response);
        }
    }

    rc_api_destroy_request(&api_request);

    return response;
}

FetchUserFriends::Response ConnectedServer::FetchUserFriends(const FetchUserFriends::Request&)
{
    FetchUserFriends::Response response;
    rapidjson::Document document;
    std::string sPostData;

    if (DoRequest(m_sHost, FetchUserFriends::Name(), "getfriendlist", sPostData, response, document))
    {
        response.Result = ApiResult::Success;

        if (!document.HasMember("Friends"))
        {
            response.Result = ApiResult::Error;
            response.ErrorMessage = ra::StringPrintf("%s not found in response", "Friends");
        }
        else
        {
            const auto& pFriends = document["Friends"].GetArray();
            for (const auto& pFriend : pFriends)
            {
                FetchUserFriends::Response::Friend oFriend;
                GetRequiredJsonField(oFriend.User, pFriend, "Friend", response);
                GetRequiredJsonField(oFriend.LastActivity, pFriend, "LastSeen", response);

                // if server's LastSeen is empty or "Unknown", it returns "_"
                if (oFriend.LastActivity == L"_")
                    oFriend.LastActivity.clear();

                std::string sPoints;
                GetRequiredJsonField(sPoints, pFriend, "RAPoints", response);
                oFriend.Score = std::stoi(sPoints);

                response.Friends.emplace_back(oFriend);
            }
        }
    }

    return response;
}

ResolveHash::Response ConnectedServer::ResolveHash(const ResolveHash::Request& request)
{
    ResolveHash::Response response;

    std::string sPostData;
    rc_api_resolve_hash_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    api_params.game_hash = request.Hash.c_str();

    rc_api_request_t api_request;
    if (rc_api_init_resolve_hash_request(&api_request, &api_params) == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, ResolveHash::Name(), httpResponse, response))
        {
            rc_api_resolve_hash_response_t api_response;
            const auto nResult = rc_api_process_resolve_hash_response(&api_response, httpResponse.Content().c_str());

            if (ValidateResponse(nResult, api_response.response, ResolveHash::Name(), httpResponse.StatusCode(), response))
            {
                response.Result = ApiResult::Success;
                response.GameId = api_response.game_id;
            }

            rc_api_destroy_resolve_hash_response(&api_response);
        }
    }

    rc_api_destroy_request(&api_request);

    return response;
}

FetchGameData::Response ConnectedServer::FetchGameData(const FetchGameData::Request& request)
{
    FetchGameData::Response response;

    std::string sPostData;
    rc_api_fetch_game_data_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    api_params.username = pUserContext.GetUsername().c_str();
    api_params.api_token = pUserContext.GetApiToken().c_str();
    api_params.game_id = request.GameId;

    rc_api_request_t api_request;
    if (rc_api_init_fetch_game_data_request(&api_request, &api_params) == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, FetchGameData::Name(), httpResponse, response))
        {
            // process it
            ProcessGamePatchData(response, httpResponse);

            if (response.Result == ApiResult::Success)
            {
                // extract the PatchData and store a copy in the cache for offline mode
                rc_api_response_t api_response{};
                rc_json_field_t fields[] = {
                    {"Success"},
                    {"Error"},
                    {"PatchData"}
                };

                if (rc_json_parse_response(&api_response, httpResponse.Content().c_str(), fields, sizeof(fields) / sizeof(fields[0])) == RC_OK &&
                    fields[2].value_start && fields[2].value_end)
                {
                    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
                    auto pData = pLocalStorage.WriteText(ra::services::StorageItemType::GameData, std::to_wstring(request.GameId));
                    if (pData != nullptr)
                    {
                        std::string sPatchData;
                        sPatchData.append(fields[2].value_start, fields[2].value_end - fields[2].value_start);
                        pData->Write(sPatchData);
                    }
                }
            }
        }
    }

    rc_api_destroy_request(&api_request);

    return response;
}

void ConnectedServer::ProcessGamePatchData(ra::api::FetchGameData::Response &response, const ra::services::Http::Response& httpResponse)
{
    rc_api_fetch_game_data_response_t api_response;
    const auto nResult = rc_api_process_fetch_game_data_response(&api_response, httpResponse.Content().c_str());

    if (ValidateResponse(nResult, api_response.response, FetchGameData::Name(), httpResponse.StatusCode(), response))
    {
        response.Result = ApiResult::Success;
        response.Title = ra::Widen(api_response.title);
        response.ConsoleId = api_response.console_id;
        response.ImageIcon = api_response.image_name;
        response.RichPresence = api_response.rich_presence_script;

        for (unsigned i = 0; i < api_response.num_achievements; ++i)
        {
            const auto* pSrcAchievement = &api_response.achievements[i];
            auto& pAchievement = response.Achievements.emplace_back();
            pAchievement.Id = pSrcAchievement->id;
            pAchievement.Title = pSrcAchievement->title;
            pAchievement.Description = pSrcAchievement->description;
            pAchievement.CategoryId = pSrcAchievement->category;
            pAchievement.Points = pSrcAchievement->points;
            pAchievement.Definition = pSrcAchievement->definition;
            pAchievement.Author = pSrcAchievement->author;
            pAchievement.BadgeName = pSrcAchievement->badge_name;
            pAchievement.Created = pSrcAchievement->created;
            pAchievement.Updated = pSrcAchievement->updated;
        }

        for (unsigned i = 0; i < api_response.num_leaderboards; ++i)
        {
            const auto* pSrcLeaderboard = &api_response.leaderboards[i];
            auto& pLeaderboard = response.Leaderboards.emplace_back();
            pLeaderboard.Id = pSrcLeaderboard->id;
            pLeaderboard.Title = pSrcLeaderboard->title;
            pLeaderboard.Description = pSrcLeaderboard->description;
            pLeaderboard.Definition = pSrcLeaderboard->definition;
            pLeaderboard.Format = pSrcLeaderboard->format;
        }
    }

    rc_api_destroy_fetch_game_data_response(&api_response);
}

FetchCodeNotes::Response ConnectedServer::FetchCodeNotes(const FetchCodeNotes::Request& request)
{
    FetchCodeNotes::Response response;
    rapidjson::Document document;
    std::string sPostData;

    AppendUrlParam(sPostData, "g", std::to_string(request.GameId));

    if (DoRequest(m_sHost, FetchCodeNotes::Name(), "codenotes2", sPostData, response, document))
    {
        if (!document.HasMember("CodeNotes"))
        {
            response.Result = ApiResult::Error;
            response.ErrorMessage = ra::StringPrintf("%s not found in response", "CodeNotes");
        }
        else
        {
            response.Result = ApiResult::Success;

            const auto& CodeNotes = document["CodeNotes"];

            // store a copy in the cache for offline mode
            auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
            auto pData = pLocalStorage.WriteText(ra::services::StorageItemType::CodeNotes, std::to_wstring(request.GameId));
            if (pData != nullptr)
            {
                rapidjson::Document codeNotes;
                codeNotes.CopyFrom(CodeNotes, document.GetAllocator());
                SaveDocument(codeNotes, *pData.get());
            }

            // process it
            ProcessCodeNotes(response, CodeNotes);
        }
    }

    return response;
}

void ConnectedServer::ProcessCodeNotes(ra::api::FetchCodeNotes::Response &response, const rapidjson::Value& CodeNotes)
{
    for (const auto& codeNote : CodeNotes.GetArray())
    {
        auto& pNote = response.Notes.emplace_back();
        GetRequiredJsonField(pNote.Author, codeNote, "User", response);

        std::string sAddress;
        GetRequiredJsonField(sAddress, codeNote, "Address", response);
        pNote.Address = ra::ByteAddressFromString(sAddress);

        GetRequiredJsonField(pNote.Note, codeNote, "Note", response);

        // empty notes were deleted on the server - don't bother including them in the response
        if (pNote.Note.empty())
            response.Notes.pop_back();
    }
}

UpdateCodeNote::Response ConnectedServer::UpdateCodeNote(const UpdateCodeNote::Request& request)
{
    UpdateCodeNote::Response response;
    rapidjson::Document document;
    std::string sPostData;

    AppendUrlParam(sPostData, "g", std::to_string(request.GameId));
    AppendUrlParam(sPostData, "m", std::to_string(request.Address));
    AppendUrlParam(sPostData, "n", ra::Narrow(request.Note));

    if (DoRequest(m_sHost, UpdateCodeNote::Name(), "submitcodenote", sPostData, response, document))
    {
        response.Result = ApiResult::Success;
    }

    return response;
}

DeleteCodeNote::Response ConnectedServer::DeleteCodeNote(const DeleteCodeNote::Request& request)
{
    DeleteCodeNote::Response response;
    rapidjson::Document document;
    std::string sPostData;

    AppendUrlParam(sPostData, "g", std::to_string(request.GameId));
    AppendUrlParam(sPostData, "m", std::to_string(request.Address));

    if (DoRequest(m_sHost, DeleteCodeNote::Name(), "submitcodenote", sPostData, response, document))
    {
        response.Result = ApiResult::Success;
    }

    return response;
}

UpdateAchievement::Response ConnectedServer::UpdateAchievement(const UpdateAchievement::Request& request)
{
    UpdateAchievement::Response response;
    rapidjson::Document document;
    std::string sPostData;

    AppendUrlParam(sPostData, "a", std::to_string(request.AchievementId));
    AppendUrlParam(sPostData, "g", std::to_string(request.GameId));
    AppendUrlParam(sPostData, "n", ra::Narrow(request.Title));
    AppendUrlParam(sPostData, "d", ra::Narrow(request.Description));
    AppendUrlParam(sPostData, "m", request.Trigger);
    AppendUrlParam(sPostData, "z", std::to_string(request.Points));
    AppendUrlParam(sPostData, "f", std::to_string(request.Category));
    AppendUrlParam(sPostData, "b", request.Badge);

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    const std::string sPostCode = ra::StringPrintf("%sSECRET%uSEC%s%uRE2%u",
        pUserContext.GetUsername(), request.AchievementId, request.Trigger, request.Points, request.Points * 3);
    const std::string sPostCodeHash = RAGenerateMD5(sPostCode);
    AppendUrlParam(sPostData, "h", sPostCodeHash);

    if (DoRequest(m_sHost, UpdateAchievement::Name(), "uploadachievement", sPostData, response, document))
    {
        response.Result = ApiResult::Success;
        GetRequiredJsonField(response.AchievementId, document, "AchievementID", response);
    }

    return response;
}

FetchAchievementInfo::Response ConnectedServer::FetchAchievementInfo(const FetchAchievementInfo::Request& request)
{
    FetchAchievementInfo::Response response;
    std::string sPostData;

    rc_api_fetch_achievement_info_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    api_params.username = pUserContext.GetUsername().c_str();
    api_params.api_token = pUserContext.GetApiToken().c_str();

    api_params.achievement_id = request.AchievementId;
    api_params.first_entry = request.FirstEntry;
    api_params.count = request.NumEntries;
    if (request.FriendsOnly)
        api_params.friends_only = 1;

    rc_api_request_t api_request;
    if (rc_api_init_fetch_achievement_info_request(&api_request, &api_params) == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, FetchAchievementInfo::Name(), httpResponse, response))
        {
            rc_api_fetch_achievement_info_response_t api_response;
            const auto nResult = rc_api_process_fetch_achievement_info_response(&api_response, httpResponse.Content().c_str());

            if (ValidateResponse(nResult, api_response.response, FetchAchievementInfo::Name(), httpResponse.StatusCode(), response))
            {
                response.Result = ApiResult::Success;

                response.GameId = api_response.game_id;
                response.EarnedBy = api_response.num_awarded;
                response.NumPlayers = api_response.num_players;

                response.Entries.reserve(api_response.num_recently_awarded);
                for (unsigned i = 0; i < api_response.num_recently_awarded; ++i)
                {
                    const auto* pAwarded = &api_response.recently_awarded[i];
                    auto& pEntry = response.Entries.emplace_back();
                    pEntry.User = pAwarded->username;
                    pEntry.DateAwarded = pAwarded->awarded;
                }
            }

            rc_api_destroy_fetch_achievement_info_response(&api_response);
        }
    }

    return response;
}

FetchLeaderboardInfo::Response ConnectedServer::FetchLeaderboardInfo(const FetchLeaderboardInfo::Request& request)
{
    FetchLeaderboardInfo::Response response;
    std::string sPostData;

    rc_api_fetch_leaderboard_info_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    api_params.leaderboard_id = request.LeaderboardId;
    if (!request.AroundUser.empty())
        api_params.username = request.AroundUser.c_str();
    else
        api_params.first_entry = request.FirstEntry;

    api_params.count = request.NumEntries;

    rc_api_request_t api_request;
    if (rc_api_init_fetch_leaderboard_info_request(&api_request, &api_params) == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, FetchLeaderboardInfo::Name(), httpResponse, response))
        {
            rc_api_fetch_leaderboard_info_response_t api_response;
            const auto nResult = rc_api_process_fetch_leaderboard_info_response(&api_response, httpResponse.Content().c_str());

            if (ValidateResponse(nResult, api_response.response, FetchLeaderboardInfo::Name(), httpResponse.StatusCode(), response))
            {
                response.Result = ApiResult::Success;

                response.GameId = api_response.game_id;
                response.LowerIsBetter = api_response.lower_is_better;

                response.Entries.reserve(api_response.num_entries);
                for (unsigned i = 0; i < api_response.num_entries; ++i)
                {
                    const auto* pApiEntry = &api_response.entries[i];
                    auto& pEntry = response.Entries.emplace_back();
                    pEntry.Rank = pApiEntry->rank;
                    pEntry.Score = pApiEntry->score;
                    pEntry.User = pApiEntry->username;
                    pEntry.DateSubmitted = pApiEntry->submitted;
                }
            }

            rc_api_destroy_fetch_leaderboard_info_response(&api_response);
        }
    }

    return response;
}

LatestClient::Response ConnectedServer::LatestClient(const LatestClient::Request& request)
{
    LatestClient::Response response;
    rapidjson::Document document;
    std::string sPostData;

    // LatestClient doesn't require User/Password, so the next few lines are a subset of DoRequest
    AppendUrlParam(sPostData, "r", "latestclient");
    AppendUrlParam(sPostData, "e", std::to_string(request.EmulatorId));
    RA_LOG_INFO("%s Request: %s", LatestClient::Name(), sPostData.c_str());

    ra::services::Http::Request httpRequest(ra::StringPrintf("%s/dorequest.php", m_sHost));
    httpRequest.SetPostData(sPostData);

    const auto httpResponse = httpRequest.Call();
    if (GetJson(LatestClient::Name(), httpResponse, response, document))
    {
        response.Result = ApiResult::Success;
        GetRequiredJsonField(response.LatestVersion, document, "LatestVersion", response);
        GetOptionalJsonField(response.MinimumVersion, document, "MinimumVersion");
        if (response.MinimumVersion.empty())
            response.MinimumVersion = response.LatestVersion;
    }

    return response;
}

FetchGamesList::Response ConnectedServer::FetchGamesList(const FetchGamesList::Request& request)
{
    FetchGamesList::Response response;
    std::string sPostData;

    rc_api_fetch_games_list_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    api_params.console_id = request.ConsoleId;

    rc_api_request_t api_request;
    if (rc_api_init_fetch_games_list_request(&api_request, &api_params) == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, FetchGamesList::Name(), httpResponse, response))
        {
            rc_api_fetch_games_list_response_t api_response;
            const auto nResult = rc_api_process_fetch_games_list_response(&api_response, httpResponse.Content().c_str());

            if (ValidateResponse(nResult, api_response.response, FetchGamesList::Name(), httpResponse.StatusCode(), response))
            {
                response.Result = ApiResult::Success;

                response.Games.reserve(api_response.num_entries);
                for (unsigned i = 0; i < api_response.num_entries; ++i)
                {
                    const auto* pEntry = &api_response.entries[i];
                    response.Games.emplace_back(pEntry->id, ra::Widen(pEntry->name));
                }
            }

            rc_api_destroy_fetch_games_list_response(&api_response);
        }
    }

    return response;
}

SubmitNewTitle::Response ConnectedServer::SubmitNewTitle(const SubmitNewTitle::Request& request)
{
    SubmitNewTitle::Response response;
    rapidjson::Document document;
    std::string sPostData;

    AppendUrlParam(sPostData, "c", std::to_string(request.ConsoleId));
    AppendUrlParam(sPostData, "m", request.Hash);
    AppendUrlParam(sPostData, "i", ra::Narrow(request.GameName));

    if (request.GameId)
        AppendUrlParam(sPostData, "g", std::to_string(request.GameId));

    if (!request.Description.empty())
        AppendUrlParam(sPostData, "d", ra::Narrow(request.Description));

    if (DoRequest(m_sHost, SubmitNewTitle::Name(), "submitgametitle", sPostData, response, document))
    {
        if (!document.HasMember("Response"))
        {
            response.Result = ApiResult::Error;
            if (response.ErrorMessage.empty())
                response.ErrorMessage = ra::StringPrintf("%s not found in response", "Response");
        }
        else
        {
            response.Result = ApiResult::Success;
            GetRequiredJsonField(response.GameId, document["Response"], "GameID", response);
        }
    }

    return response;
}

SubmitTicket::Response ConnectedServer::SubmitTicket(const SubmitTicket::Request& request)
{
    SubmitTicket::Response response;
    rapidjson::Document document;
    std::string sPostData;

    std::string sAchievementIds;
    for (auto nAchievementId : request.AchievementIds)
    {
        sAchievementIds.append(std::to_string(nAchievementId));
        sAchievementIds.push_back(',');
    }
    sAchievementIds.pop_back();

    AppendUrlParam(sPostData, "i", sAchievementIds);
    AppendUrlParam(sPostData, "m", request.GameHash);
    AppendUrlParam(sPostData, "p", std::to_string(ra::etoi(request.Problem)));
    AppendUrlParam(sPostData, "n", request.Comment);

    if (DoRequest(m_sHost, SubmitTicket::Name(), "submitticket", sPostData, response, document))
    {
        if (!document.HasMember("Response"))
        {
            response.Result = ApiResult::Error;
            if (response.ErrorMessage.empty())
                response.ErrorMessage = ra::StringPrintf("%s not found in response", "Response");
        }
        else
        {
            response.Result = ApiResult::Success;
            GetRequiredJsonField(response.TicketsCreated, document["Response"], "Added", response);
        }
    }

    return response;
}

FetchBadgeIds::Response ConnectedServer::FetchBadgeIds(const FetchBadgeIds::Request&)
{
    FetchBadgeIds::Response response;
    rapidjson::Document document;
    std::string sPostData;

    if (DoRequest(m_sHost, FetchBadgeIds::Name(), "badgeiter", sPostData, response, document))
    {
        GetRequiredJsonField(response.FirstID, document, "FirstBadge", response);
        GetRequiredJsonField(response.NextID, document, "NextBadge", response);
    }

    return response;
}

UploadBadge::Response ConnectedServer::UploadBadge(const UploadBadge::Request& request)
{
    UploadBadge::Response response;
    rapidjson::Document document;
    std::string sPostData;

    if (DoUpload(m_sHost, UploadBadge::Name(), "uploadbadgeimage", request.ImageFilePath, response, document))
    {
        if (!document.HasMember("Response"))
        {
            response.Result = ApiResult::Error;
            if (response.ErrorMessage.empty())
                response.ErrorMessage = ra::StringPrintf("%s not found in response", "Response");
        }
        else
        {
            response.Result = ApiResult::Success;
            GetRequiredJsonField(response.BadgeId, document["Response"], "BadgeIter", response);
        }
    }

    return response;
}

} // namespace impl
} // namespace api
} // namespace ra
