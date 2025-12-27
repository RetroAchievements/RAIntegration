#include "ConnectedServer.hh"

#include "DisconnectedServer.hh"
#include "RA_Defs.h"
#include "util\Log.hh"

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
#include <rc_api_editor.h>
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
            if (ra::StringStartsWith(httpResponse.Content(), "<html>"))
            {
                const auto nStartIndex = httpResponse.Content().find("<title>");
                if (nStartIndex != std::string::npos)
                {
                    const auto nEndIndex = httpResponse.Content().find("</title>");
                    if (nEndIndex != std::string::npos)
                    {
                        pResponse.ErrorMessage =
                            httpResponse.Content().substr(nStartIndex + 7, nEndIndex - nStartIndex - 7);
                    }
                }
            }

            if (pResponse.ErrorMessage.empty())
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
        if (httpResponse.StatusCode() == ra::services::Http::StatusCode::TooManyRequests)
        {
            pResponse.Result = ApiResult::Incomplete;
            return false;
        }

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

static void AppendUrlParam(_Inout_ std::string& sParams, _In_ const char* const sParam, _In_ const std::string& sValue)
{
    if (!sParams.empty() && sParams.back() != '?')
        sParams.push_back('&');

    sParams.append(sParam);
    sParams.push_back('=');
    ra::services::Http::UrlEncodeAppend(sParams, sValue);
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
            break;

        case '<': // HTML, not expected
            if (HandleHttpError(pHttpResponse.StatusCode(), pResponse))
                return false;
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

                sParams.append(api_request.post_data, ptr - api_request.post_data);
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
        if (nResult == RC_MISSING_VALUE && api_response.error_message)
            pResponse.ErrorMessage = ra::StringPrintf("JSON Parse Error: %s", api_response.error_message);
        else
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

            if (nStatusCode == ra::services::Http::StatusCode::TooManyRequests)
            {
                pResponse.Result = ApiResult::Incomplete;
            }
            else
            {
                pResponse.Result = ApiResult::Error;
                RA_LOG_ERR("-- %s Error: %s", sApiName, pResponse.ErrorMessage);
            }
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

static bool DoUpload(const std::string& sHost, const char* _RESTRICT sApiName, const char* _RESTRICT sRequestName,
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

    std::string sExt = ra::Narrow(pFileSystem.GetExtension(sFilePath));
    ra::StringMakeLowercase(sExt);

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

static void HttpResponseToServerResponse(const ra::services::Http::Response& httpResponse,
                                         rc_api_server_response_t* server_response) noexcept
{
    memset(server_response, 0, sizeof(server_response));
    server_response->body = httpResponse.Content().c_str();
    server_response->body_length = httpResponse.Content().length();
    server_response->http_status_code = ra::etoi(httpResponse.StatusCode());
}

FetchUserFriends::Response ConnectedServer::FetchUserFriends(const FetchUserFriends::Request&)
{
    FetchUserFriends::Response response;

    rc_api_fetch_followed_users_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    api_params.username = pUserContext.GetUsername().c_str();
    api_params.api_token = pUserContext.GetApiToken().c_str();

    rc_api_request_t api_request;
    const int result = rc_api_init_fetch_followed_users_request(&api_request, &api_params);
    if (result == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, FetchUserFriends::Name(), httpResponse, response))
        {
            rc_api_fetch_followed_users_response_t api_response;
            rc_api_server_response_t server_response;
            HttpResponseToServerResponse(httpResponse, &server_response);

            const auto nResult = rc_api_process_fetch_followed_users_server_response(&api_response, &server_response);
            if (ValidateResponse(nResult, api_response.response, ResolveHash::Name(), httpResponse.StatusCode(),
                                 response))
            {
                response.Friends.resize(api_response.num_users);
                for (uint32_t i = 0; i < api_response.num_users; ++i)
                {
                    auto& pFriend = response.Friends.at(i);
                    const auto& pUser = api_response.users[i];
                    pFriend.User = pUser.display_name;
                    if (pUser.avatar_url)
                        pFriend.AvatarUrl = pUser.avatar_url;
                    pFriend.Score = pUser.score;
                    if (pUser.recent_activity.description)
                        pFriend.LastActivity = ra::Widen(pUser.recent_activity.description);
                    pFriend.LastActivityContextId = pUser.recent_activity.context_id;
                    if (pUser.recent_activity.context)
                        pFriend.LastActivityContext = ra::Widen(pUser.recent_activity.context);
                    if (pUser.recent_activity.context_image_url)
                        pFriend.LastActivityImageUrl = pUser.recent_activity.context_image_url;
                    pFriend.LastActivityTime = pUser.recent_activity.when;
                }

                response.Result = ApiResult::Success;
            }

            rc_api_destroy_fetch_followed_users_response(&api_response);
        }
    }
    else
    {
        response.Result = ApiResult::Failed;
        response.ErrorMessage = rc_error_str(result);
    }

    rc_api_destroy_request(&api_request);
    return response;
}

ResolveHash::Response ConnectedServer::ResolveHash(const ResolveHash::Request& request)
{
    ResolveHash::Response response;

    rc_api_resolve_hash_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    api_params.game_hash = request.Hash.c_str();

    rc_api_request_t api_request;
    const int result = rc_api_init_resolve_hash_request(&api_request, &api_params);
    if (result == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, ResolveHash::Name(), httpResponse, response))
        {
            rc_api_resolve_hash_response_t api_response;
            rc_api_server_response_t server_response;
            HttpResponseToServerResponse(httpResponse, &server_response);

            const auto nResult = rc_api_process_resolve_hash_server_response(&api_response, &server_response);

            if (ValidateResponse(nResult, api_response.response, ResolveHash::Name(), httpResponse.StatusCode(), response))
            {
                response.Result = ApiResult::Success;
                response.GameId = api_response.game_id;
            }

            rc_api_destroy_resolve_hash_response(&api_response);
        }
    }
    else
    {
        response.Result = ApiResult::Failed;
        response.ErrorMessage = rc_error_str(result);
    }

    rc_api_destroy_request(&api_request);
    return response;
}

FetchCodeNotes::Response ConnectedServer::FetchCodeNotes(const FetchCodeNotes::Request& request)
{
    FetchCodeNotes::Response response;

    rc_api_fetch_code_notes_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    api_params.game_id = request.GameId;

    rc_api_request_t api_request;
    const int result = rc_api_init_fetch_code_notes_request(&api_request, &api_params);
    if (result == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, FetchCodeNotes::Name(), httpResponse, response))
        {
            rc_api_fetch_code_notes_response_t api_response;
            rc_api_server_response_t server_response;
            HttpResponseToServerResponse(httpResponse, &server_response);

            const auto nResult = rc_api_process_fetch_code_notes_server_response(&api_response, &server_response);

            if (ValidateResponse(nResult, api_response.response, FetchCodeNotes::Name(), httpResponse.StatusCode(), response))
            {
                // store a copy in the cache for offline mode
                auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
                auto pData = pLocalStorage.WriteText(ra::services::StorageItemType::CodeNotes, std::to_wstring(request.GameId));
                if (pData != nullptr)
                {
                    std::string sContent(httpResponse.Content());
                    auto nIndex = sContent.find('[');
                    sContent.erase(0, nIndex);
                    nIndex = sContent.find_last_of(']');
                    sContent.erase(nIndex + 1);
                    pData->Write(sContent);
                }

                response.Result = ApiResult::Success;

                ProcessCodeNotes(response, &api_response);
            }

            rc_api_destroy_fetch_code_notes_response(&api_response);
        }
    }
    else
    {
        response.Result = ApiResult::Failed;
        response.ErrorMessage = rc_error_str(result);
    }

    rc_api_destroy_request(&api_request);
    return response;
}

#pragma warning(push)
#pragma warning(disable : 5045)
void ConnectedServer::ProcessCodeNotes(FetchCodeNotes::Response& response, const void* api_response)
{
    const rc_api_fetch_code_notes_response_t* fetch_code_notes_response =
        static_cast<const rc_api_fetch_code_notes_response_t*>(api_response);
    const rc_api_code_note_t* note = fetch_code_notes_response->notes;
    const rc_api_code_note_t* stop = fetch_code_notes_response->notes + fetch_code_notes_response->num_notes;
    for (; note < stop; ++note)
    {
        auto& pNote = response.Notes.emplace_back();
        pNote.Author = note->author;
        pNote.Address = note->address;
        pNote.Note = ra::Widen(note->note);

        ra::NormalizeLineEndings(pNote.Note);
    }
}
#pragma warning(pop)

static void SetCodeNote(ApiResponseBase& response, const char* sApiName,
    unsigned nGameId, ra::data::ByteAddress nAddress, const char* sNote)
{
    rc_api_update_code_note_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    api_params.username = pUserContext.GetUsername().c_str();
    api_params.api_token = pUserContext.GetApiToken().c_str();

    api_params.game_id = nGameId;
    api_params.address = nAddress;
    api_params.note = sNote;

    rc_api_request_t api_request;
    const int result = rc_api_init_update_code_note_request(&api_request, &api_params);
    if (result == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, sApiName, httpResponse, response))
        {
            rc_api_update_code_note_response_t api_response;
            const auto nResult = rc_api_process_update_code_note_response(&api_response, httpResponse.Content().c_str());

            if (ValidateResponse(nResult, api_response.response, sApiName, httpResponse.StatusCode(), response))
            {
                response.Result = ApiResult::Success;
            }

            rc_api_destroy_update_code_note_response(&api_response);
        }
    }
    else
    {
        response.Result = ApiResult::Failed;
        response.ErrorMessage = rc_error_str(result);
    }

    rc_api_destroy_request(&api_request);
}

UpdateCodeNote::Response ConnectedServer::UpdateCodeNote(const UpdateCodeNote::Request& request)
{
    UpdateCodeNote::Response response;

    const std::string sNote = ra::Narrow(request.Note);
    SetCodeNote(response, UpdateCodeNote::Name(), request.GameId, request.Address, sNote.c_str());

    return response;
}

DeleteCodeNote::Response ConnectedServer::DeleteCodeNote(const DeleteCodeNote::Request& request)
{
    DeleteCodeNote::Response response;

    SetCodeNote(response, DeleteCodeNote::Name(), request.GameId, request.Address, nullptr);

    return response;
}

UpdateAchievement::Response ConnectedServer::UpdateAchievement(const UpdateAchievement::Request& request)
{
    UpdateAchievement::Response response;

    rc_api_update_achievement_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    api_params.username = pUserContext.GetUsername().c_str();
    api_params.api_token = pUserContext.GetApiToken().c_str();

    const std::string sTitle = ra::Narrow(request.Title);
    const std::string sDescription = ra::Narrow(request.Description);

    api_params.achievement_id = request.AchievementId;
    api_params.game_id = request.GameId;
    api_params.title = sTitle.c_str();
    api_params.description = sDescription.c_str();
    api_params.trigger = request.Trigger.c_str();
    api_params.points = request.Points;
    api_params.category = request.Category;
    api_params.badge = request.Badge.c_str();
    api_params.type = request.Type;

    rc_api_request_t api_request;
    const int result = rc_api_init_update_achievement_request(&api_request, &api_params);
    if (result == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, UpdateAchievement::Name(), httpResponse, response))
        {
            rc_api_update_achievement_response_t api_response;
            rc_api_server_response_t server_response;
            HttpResponseToServerResponse(httpResponse, &server_response);

            const auto nResult = rc_api_process_update_achievement_server_response(&api_response, &server_response);

            if (ValidateResponse(nResult, api_response.response, UpdateAchievement::Name(), httpResponse.StatusCode(), response))
            {
                response.Result = ApiResult::Success;
                response.AchievementId = api_response.achievement_id;
            }

            rc_api_destroy_update_achievement_response(&api_response);
        }
    }
    else
    {
        response.Result = ApiResult::Failed;
        response.ErrorMessage = rc_error_str(result);
    }

    rc_api_destroy_request(&api_request);
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
    const int result = rc_api_init_fetch_achievement_info_request(&api_request, &api_params);
    if (result == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, FetchAchievementInfo::Name(), httpResponse, response))
        {
            rc_api_fetch_achievement_info_response_t api_response;
            rc_api_server_response_t server_response;
            HttpResponseToServerResponse(httpResponse, &server_response);

            const auto nResult = rc_api_process_fetch_achievement_info_server_response(&api_response, &server_response);

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
    else
    {
        response.Result = ApiResult::Failed;
        response.ErrorMessage = rc_error_str(result);
    }

    rc_api_destroy_request(&api_request);
    return response;
}

UpdateLeaderboard::Response ConnectedServer::UpdateLeaderboard(const UpdateLeaderboard::Request& request)
{
    UpdateLeaderboard::Response response;

    rc_api_update_leaderboard_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    api_params.username = pUserContext.GetUsername().c_str();
    api_params.api_token = pUserContext.GetApiToken().c_str();

    const std::string sTitle = ra::Narrow(request.Title);
    const std::string sDescription = ra::Narrow(request.Description);

    api_params.leaderboard_id = request.LeaderboardId;
    api_params.game_id = request.GameId;
    api_params.title = sTitle.c_str();
    api_params.description = sDescription.c_str();
    api_params.start_trigger = request.StartTrigger.c_str();
    api_params.submit_trigger = request.SubmitTrigger.c_str();
    api_params.cancel_trigger = request.CancelTrigger.c_str();
    api_params.value_definition = request.ValueDefinition.c_str();
    api_params.lower_is_better = request.LowerIsBetter ? 1 : 0;
    api_params.format = ValueFormatToString(request.Format);

    rc_api_request_t api_request;
    const int result = rc_api_init_update_leaderboard_request(&api_request, &api_params);
    if (result == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, UpdateLeaderboard::Name(), httpResponse, response))
        {
            rc_api_update_leaderboard_response_t api_response;
            rc_api_server_response_t server_response;
            HttpResponseToServerResponse(httpResponse, &server_response);

            const auto nResult = rc_api_process_update_leaderboard_server_response(&api_response, &server_response);

            if (ValidateResponse(nResult, api_response.response, UpdateLeaderboard::Name(), httpResponse.StatusCode(), response))
            {
                response.Result = ApiResult::Success;
                response.LeaderboardId = api_response.leaderboard_id;
            }

            rc_api_destroy_update_leaderboard_response(&api_response);
        }
    }
    else
    {
        response.Result = ApiResult::Failed;
        response.ErrorMessage = rc_error_str(result);
    }

    rc_api_destroy_request(&api_request);
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
    const int result = rc_api_init_fetch_leaderboard_info_request(&api_request, &api_params);
    if (result == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, FetchLeaderboardInfo::Name(), httpResponse, response))
        {
            rc_api_fetch_leaderboard_info_response_t api_response;
            rc_api_server_response_t server_response;
            HttpResponseToServerResponse(httpResponse, &server_response);

            const auto nResult = rc_api_process_fetch_leaderboard_info_server_response(&api_response, &server_response);

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
    else
    {
        response.Result = ApiResult::Failed;
        response.ErrorMessage = rc_error_str(result);
    }

    rc_api_destroy_request(&api_request);
    return response;
}

UpdateRichPresence::Response ConnectedServer::UpdateRichPresence(const UpdateRichPresence::Request& request)
{
    UpdateRichPresence::Response response;

    rc_api_update_rich_presence_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    api_params.username = pUserContext.GetUsername().c_str();
    api_params.api_token = pUserContext.GetApiToken().c_str();

    api_params.game_id = request.GameId;
    api_params.script = request.Script.c_str();

    rc_api_request_t api_request;
    const int result = rc_api_init_update_rich_presence_request(&api_request, &api_params);
    if (result == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, UpdateRichPresence::Name(), httpResponse, response))
        {
            rc_api_update_rich_presence_response_t api_response;
            rc_api_server_response_t server_response;
            HttpResponseToServerResponse(httpResponse, &server_response);

            const auto nResult = rc_api_process_update_rich_presence_server_response(&api_response, &server_response);

            if (ValidateResponse(nResult, api_response.response, UpdateRichPresence::Name(), httpResponse.StatusCode(), response))
                response.Result = ApiResult::Success;

            rc_api_destroy_update_rich_presence_response(&api_response);
        }
    }
    else
    {
        response.Result = ApiResult::Failed;
        response.ErrorMessage = rc_error_str(result);
    }

    rc_api_destroy_request(&api_request);
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

    rc_api_fetch_games_list_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    api_params.console_id = request.ConsoleId;

    rc_api_request_t api_request;
    const int result = rc_api_init_fetch_games_list_request(&api_request, &api_params);
    if (result == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, FetchGamesList::Name(), httpResponse, response))
        {
            rc_api_fetch_games_list_response_t api_response;
            rc_api_server_response_t server_response;
            HttpResponseToServerResponse(httpResponse, &server_response);

            const auto nResult = rc_api_process_fetch_games_list_server_response(&api_response, &server_response);

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
    else
    {
        response.Result = ApiResult::Failed;
        response.ErrorMessage = rc_error_str(result);
    }

    rc_api_destroy_request(&api_request);
    return response;
}

SubmitNewTitle::Response ConnectedServer::SubmitNewTitle(const SubmitNewTitle::Request& request)
{
    SubmitNewTitle::Response response;

    rc_api_add_game_hash_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::context::UserContext>();
    api_params.username = pUserContext.GetUsername().c_str();
    api_params.api_token = pUserContext.GetApiToken().c_str();

    const std::string sGameName = ra::Narrow(request.GameName);
    const std::string sDescription = ra::Narrow(request.Description);

    api_params.console_id = request.ConsoleId;
    api_params.hash = request.Hash.c_str();
    api_params.title = sGameName.c_str();
    api_params.game_id = request.GameId;

    if (!sDescription.empty())
        api_params.hash_description = sDescription.c_str();

    rc_api_request_t api_request;
    const int result = rc_api_init_add_game_hash_request(&api_request, &api_params);
    if (result == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, SubmitNewTitle::Name(), httpResponse, response))
        {
            rc_api_add_game_hash_response_t api_response;
            rc_api_server_response_t server_response;
            HttpResponseToServerResponse(httpResponse, &server_response);

            const auto nResult = rc_api_process_add_game_hash_server_response(&api_response, &server_response);

            if (ValidateResponse(nResult, api_response.response, SubmitNewTitle::Name(), httpResponse.StatusCode(), response))
            {
                response.Result = ApiResult::Success;
                response.GameId = api_response.game_id;
            }

            rc_api_destroy_add_game_hash_response(&api_response);
        }
    }
    else
    {
        response.Result = ApiResult::Failed;
        response.ErrorMessage = rc_error_str(result);
    }

    rc_api_destroy_request(&api_request);
    return response;
}

FetchBadgeIds::Response ConnectedServer::FetchBadgeIds(const FetchBadgeIds::Request&)
{
    FetchBadgeIds::Response response;

    rc_api_fetch_badge_range_request_t api_params;
    memset(&api_params, 0, sizeof(api_params));

    rc_api_request_t api_request;
    const int result = rc_api_init_fetch_badge_range_request(&api_request, &api_params);
    if (result == RC_OK)
    {
        ra::services::Http::Response httpResponse;
        if (DoRequest(api_request, FetchBadgeIds::Name(), httpResponse, response))
        {
            rc_api_fetch_badge_range_response_t api_response;
            rc_api_server_response_t server_response;
            HttpResponseToServerResponse(httpResponse, &server_response);

            const auto nResult = rc_api_process_fetch_badge_range_server_response(&api_response, &server_response);

            if (ValidateResponse(nResult, api_response.response, FetchBadgeIds::Name(), httpResponse.StatusCode(), response))
            {
                response.Result = ApiResult::Success;
                response.FirstID = api_response.first_badge_id;
                response.NextID = api_response.next_badge_id;
            }

            rc_api_destroy_fetch_badge_range_response(&api_response);
        }
    }
    else
    {
        response.Result = ApiResult::Failed;
        response.ErrorMessage = rc_error_str(result);
    }

    rc_api_destroy_request(&api_request);
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
            if (response.Result == ApiResult::None)
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
