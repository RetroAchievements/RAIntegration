#include "ConnectedServer.hh"

#include "DisconnectedServer.hh"
#include "RA_Defs.h"

#include "RA_md5factory.h"
#include "RA_User.h"

#include "services\Http.hh"
#include "services\IFileSystem.hh"
#include "services\IHttpRequester.hh"
#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

#include <future>

#include <rapidjson\document.h>

namespace ra {
namespace api {
namespace impl {

_NODISCARD static bool HandleHttpError(_In_ const ra::services::Http::Response& httpResponse,
                                       _Inout_ ApiResponseBase& pResponse)
{
    if (httpResponse.StatusCode() != ra::services::Http::StatusCode::OK)
    {
        const auto& pHttpRequester = ra::services::ServiceLocator::Get<ra::services::IHttpRequester>();
        const bool bRetry = pHttpRequester.IsRetryable(ra::etoi(httpResponse.StatusCode()));
        pResponse.Result = bRetry ? ApiResult::Incomplete : ApiResult::Error;
        pResponse.ErrorMessage = ra::BuildString("HTTP error code: ", ra::etoi(httpResponse.StatusCode()));
        return true;
    }

    return false;
}

_NODISCARD static bool GetJson([[maybe_unused]] _In_ const char* sApiName,
                               _In_ const ra::services::Http::Response& httpResponse,
                               _Inout_ ApiResponseBase& pResponse, _Out_ rapidjson::Document& pDocument)
{
    /*this function can throw std::bad_alloc (from the strings allocator) but very low chance*/
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

    RA_LOG_INFO("-- %s Response: %s", sApiName, httpResponse.Content());

    pDocument.Parse(httpResponse.Content());
    if (pDocument.HasParseError())
    {
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
        pResponse.Result = ApiResult::Error;
        pResponse.ErrorMessage = pDocument["Error"].GetString();
        RA_LOG_ERR("-- %s Error: %s", sApiName, pResponse.ErrorMessage);
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

static void GetOptionalJsonField(_Out_ unsigned int& nValue, _In_ const rapidjson::Value& pDocument,
                                 _In_ const char* sField, _In_ unsigned int nDefaultValue = 0)
{
    if (pDocument.HasMember(sField))
    {
        auto& pField = pDocument[sField];
        if (pField.IsNumber())
            nValue = pDocument[sField].GetUint();
        else
            nValue = nDefaultValue;
    }
    else
    {
        nValue = nDefaultValue;
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

    ra::services::Http::Request httpRequest(ra::StringPrintf("%s/dorequest.php", sHost));
    httpRequest.SetPostData(sPostData);

    const auto httpResponse = httpRequest.Call();
    return GetJson(sApiName, httpResponse, pResponse, document);
}

static bool DoUpload(const std::string& sHost, const char* restrict sApiName, const char* restrict sRequestName,
    const std::wstring& sFilePath, ApiResponseBase& pResponse, rapidjson::Document& document)
{
    const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    auto nFileSize = pFileSystem.GetFileSize(sFilePath);
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
        std::transform(sExt.begin(), sExt.end(), sExt.begin(), [](char c) {
            return static_cast<char>(::tolower(c));
        });
    }

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::UserContext>();
    RA_LOG_INFO("%s Request: file=%s (%zu bytes)", sApiName, sFilePath, nFileSize);

    const char* sBoundary = "---------------------------41184676334";
    std::string sPostData;
    sPostData.reserve(static_cast<size_t>(nFileSize + 512));

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
        const int nLength = sPostData.length();
        sPostData.resize(static_cast<size_t>(nFileSize + nLength));
        pFile->GetBytes(&sPostData.at(nLength), static_cast<size_t>(nFileSize));
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
    ra::services::Http::Request httpRequest(ra::StringPrintf("%s/login_app.php", m_sHost));

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

Ping::Response ConnectedServer::Ping(const Ping::Request& request)
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

FetchUserUnlocks::Response ConnectedServer::FetchUserUnlocks(const FetchUserUnlocks::Request& request)
{
    FetchUserUnlocks::Response response;
    rapidjson::Document document;
    std::string sPostData;

    AppendUrlParam(sPostData, "g", std::to_string(request.GameId));
    AppendUrlParam(sPostData, "h", request.Hardcore ? "1" : "0");

    if (DoRequest(m_sHost, FetchUserUnlocks::Name(), "unlocks", sPostData, response, document))
    {
        if (!document.HasMember("UserUnlocks"))
        {
            response.Result = ApiResult::Error;
            response.ErrorMessage = ra::StringPrintf("%s not found in response", "UserUnlocks");
        }
        else
        {
            response.Result = ApiResult::Success;

            const auto& UserUnlocks{ document["UserUnlocks"] };
            for (const auto& unlocked : UserUnlocks.GetArray())
                response.UnlockedAchievements.insert(unlocked.GetUint());
        }
    }

    return std::move(response);
}

AwardAchievement::Response ConnectedServer::AwardAchievement(const AwardAchievement::Request& request)
{
    AwardAchievement::Response response;
    rapidjson::Document document;
    std::string sPostData;

    AppendUrlParam(sPostData, "a", std::to_string(request.AchievementId));
    AppendUrlParam(sPostData, "h", request.Hardcore ? "1" : "0");
    if (!request.GameHash.empty())
        AppendUrlParam(sPostData, "m", request.GameHash);

    if (DoRequest(m_sHost, AwardAchievement::Name(), "awardachievement", sPostData, response, document))
    {
        response.Result = ApiResult::Success;
        GetRequiredJsonField(response.NewPlayerScore, document, "Score", response);
    }

    return std::move(response);
}

SubmitLeaderboardEntry::Response ConnectedServer::SubmitLeaderboardEntry(const SubmitLeaderboardEntry::Request& request)
{
    SubmitLeaderboardEntry::Response response;
    rapidjson::Document document;
    std::string sPostData;

    AppendUrlParam(sPostData, "i", std::to_string(request.LeaderboardId));
    AppendUrlParam(sPostData, "s", std::to_string(request.Score));

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::UserContext>();
    std::string sValidationSignature = ra::StringPrintf("%u%s%u", request.LeaderboardId, pUserContext.GetUsername(), request.LeaderboardId);
    AppendUrlParam(sPostData, "v", RAGenerateMD5(sValidationSignature));

    if (!request.GameHash.empty())
        AppendUrlParam(sPostData, "m", request.GameHash);

    if (DoRequest(m_sHost, SubmitLeaderboardEntry::Name(), "submitlbentry", sPostData, response, document))
    {
        response.Result = ApiResult::Success;

        if (!document.HasMember("Response"))
        {
            response.Result = ApiResult::Error;
            response.ErrorMessage = ra::StringPrintf("%s not found in response", "Response");
        }
        else
        {
            const auto& submitResponse = document["Response"];
            GetRequiredJsonField(response.Score, submitResponse, "Score", response);
            GetRequiredJsonField(response.BestScore, submitResponse, "BestScore", response);

            if (!submitResponse.HasMember("RankInfo"))
            {
                response.Result = ApiResult::Error;
                response.ErrorMessage = ra::StringPrintf("%s not found in response", "RankInfo");
            }
            else
            {
                const auto& pRankInfo = submitResponse["RankInfo"];
                GetRequiredJsonField(response.NewRank, pRankInfo, "Rank", response);
                std::string sNumEntries;
                GetRequiredJsonField(sNumEntries, pRankInfo, "NumEntries", response);
                response.NumEntries = std::stoi(sNumEntries);
            }

            if (submitResponse.HasMember("TopEntries"))
            {
                const auto& pTopEntries = submitResponse["TopEntries"].GetArray();
                response.TopEntries.reserve(pTopEntries.Size());
                for (const auto& pEntry : pTopEntries)
                {
                    SubmitLeaderboardEntry::Response::Entry entry;
                    GetRequiredJsonField(entry.Rank, pEntry, "Rank", response);
                    GetRequiredJsonField(entry.User, pEntry, "User", response);
                    GetRequiredJsonField(entry.Score, pEntry, "Score", response);
                    response.TopEntries.emplace_back(entry);
                }
            }
        }
    }

    return std::move(response);
}

ResolveHash::Response ConnectedServer::ResolveHash(const ResolveHash::Request& request)
{
    ResolveHash::Response response;
    rapidjson::Document document;
    std::string sPostData;

    AppendUrlParam(sPostData, "m", request.Hash);

    if (DoRequest(m_sHost, ResolveHash::Name(), "gameid", sPostData, response, document))
    {
        response.Result = ApiResult::Success;
        GetRequiredJsonField(response.GameId, document, "GameID", response);
    }

    return std::move(response);
}

FetchGameData::Response ConnectedServer::FetchGameData(const FetchGameData::Request& request)
{
    FetchGameData::Response response;
    rapidjson::Document document;
    std::string sPostData;

    AppendUrlParam(sPostData, "g", std::to_string(request.GameId));

    if (DoRequest(m_sHost, FetchGameData::Name(), "patch", sPostData, response, document))
    {
        if (!document.HasMember("PatchData"))
        {
            response.Result = ApiResult::Error;
            response.ErrorMessage = ra::StringPrintf("%s not found in response", "PatchData");
        }
        else
        {
            response.Result = ApiResult::Success;

            const auto& PatchData = document["PatchData"];

            // store a copy in the cache for offline mode
            auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
            auto pData = pLocalStorage.WriteText(ra::services::StorageItemType::GameData, std::to_wstring(request.GameId));
            if (pData != nullptr)
            {
                rapidjson::Document patchData;
                patchData.CopyFrom(PatchData, document.GetAllocator());
                SaveDocument(patchData, *pData.get());
            }

            // process it
            ProcessGamePatchData(response, PatchData);
        }
    }

    return std::move(response);
}

void ConnectedServer::ProcessGamePatchData(ra::api::FetchGameData::Response &response, const rapidjson::Value& PatchData)
{
    GetRequiredJsonField(response.Title, PatchData, "Title", response);
    GetRequiredJsonField(response.ConsoleId, PatchData, "ConsoleID", response);
    GetOptionalJsonField(response.ForumTopicId, PatchData, "ForumTopicID");
    GetOptionalJsonField(response.Flags, PatchData, "Flags");
    GetOptionalJsonField(response.ImageIcon, PatchData, "ImageIcon");
    GetOptionalJsonField(response.RichPresence, PatchData, "RichPresencePatch");

    if (!response.ImageIcon.empty())
    {
        // ImageIcon value will be "/Images/001234.png" - extract the "001234"
        auto nIndex = response.ImageIcon.find_last_of('/');
        if (nIndex != std::string::npos)
            response.ImageIcon.erase(0, nIndex + 1);
        nIndex = response.ImageIcon.find_last_of('.');
        if (nIndex != std::string::npos)
            response.ImageIcon.erase(nIndex);
    }

    const auto& AchievementsData{ PatchData["Achievements"] };
    for (const auto& achData : AchievementsData.GetArray())
    {
        auto& pAchievement = response.Achievements.emplace_back();
        GetRequiredJsonField(pAchievement.Id, achData, "ID", response);
        GetRequiredJsonField(pAchievement.Title, achData, "Title", response);
        GetRequiredJsonField(pAchievement.Description, achData, "Description", response);
        GetRequiredJsonField(pAchievement.CategoryId, achData, "Flags", response);
        GetOptionalJsonField(pAchievement.Points, achData, "Points");
        GetRequiredJsonField(pAchievement.Definition, achData, "MemAddr", response);
        GetOptionalJsonField(pAchievement.Author, achData, "Author");
        GetRequiredJsonField(pAchievement.BadgeName, achData, "BadgeName", response);

        unsigned int time;
        GetRequiredJsonField(time, achData, "Created", response);
        pAchievement.Created = static_cast<time_t>(time);
        GetRequiredJsonField(time, achData, "Modified", response);
        pAchievement.Updated = static_cast<time_t>(time);
    }

    const auto& LeaderboardsData{ PatchData["Leaderboards"] };
    for (const auto& lbData : LeaderboardsData.GetArray())
    {
        auto& pLeaderboard = response.Leaderboards.emplace_back();
        GetRequiredJsonField(pLeaderboard.Id, lbData, "ID", response);
        GetRequiredJsonField(pLeaderboard.Title, lbData, "Title", response);
        GetRequiredJsonField(pLeaderboard.Description, lbData, "Description", response);
        GetRequiredJsonField(pLeaderboard.Definition, lbData, "Mem", response);
        GetOptionalJsonField(pLeaderboard.Format, lbData, "Format", "VALUE");
    }
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

    return std::move(response);
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

    return std::move(response);
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

    return std::move(response);
}

FetchAchievementInfo::Response ConnectedServer::FetchAchievementInfo(const FetchAchievementInfo::Request& request)
{
    FetchAchievementInfo::Response response;
    rapidjson::Document document;
    std::string sPostData;

    AppendUrlParam(sPostData, "a", std::to_string(request.AchievementId));
    if (request.FirstEntry > 1)
        AppendUrlParam(sPostData, "o", std::to_string(request.FirstEntry - 1));
    AppendUrlParam(sPostData, "c", std::to_string(request.NumEntries));

    if (request.FriendsOnly)
        AppendUrlParam(sPostData, "f", "1");

    if (DoRequest(m_sHost, FetchAchievementInfo::Name(), "achievementwondata", sPostData, response, document))
    {
        if (!document.HasMember("Response"))
        {
            response.Result = ApiResult::Error;
            response.ErrorMessage = ra::StringPrintf("%s not found in response", "Response");
        }
        else
        {
            response.Result = ApiResult::Success;

            const auto& AchievmentInfo = document["Response"];

            GetRequiredJsonField(response.GameId, AchievmentInfo, "GameID", response);
            GetRequiredJsonField(response.EarnedBy, AchievmentInfo, "NumEarned", response);
            GetRequiredJsonField(response.NumPlayers, AchievmentInfo, "TotalPlayers", response);

            if (AchievmentInfo.HasMember("RecentWinner")) // RecentWinner will not be returned if there are no winners
            {
                const auto& pEntries = AchievmentInfo["RecentWinner"];
                if (!pEntries.IsArray())
                {
                    response.Result = ApiResult::Error;
                    response.ErrorMessage = ra::StringPrintf("%s not an array", "RecentWinner");
                }
                else
                {
                    const auto& pEntriesArray = pEntries.GetArray();
                    response.Entries.reserve(pEntriesArray.Size());
                    for (const auto& pEntry : pEntriesArray)
                    {
                        FetchAchievementInfo::Response::Entry entry;
                        GetRequiredJsonField(entry.User, pEntry, "User", response);
                        GetRequiredJsonField(entry.Points, pEntry, "RAPoints", response);
                        unsigned int nTime;
                        GetRequiredJsonField(nTime, pEntry, "DateAwarded", response);
                        entry.DateAwarded = nTime;
                        response.Entries.emplace_back(entry);
                    }
                }
            }
        }
    }

    return std::move(response);
}

FetchLeaderboardInfo::Response ConnectedServer::FetchLeaderboardInfo(const FetchLeaderboardInfo::Request& request)
{
    FetchLeaderboardInfo::Response response;
    rapidjson::Document document;
    std::string sPostData;

    AppendUrlParam(sPostData, "i", std::to_string(request.LeaderboardId));
    if (request.FirstEntry > 1)
        AppendUrlParam(sPostData, "o", std::to_string(request.FirstEntry - 1));
    AppendUrlParam(sPostData, "c", std::to_string(request.NumEntries));

    if (DoRequest(m_sHost, FetchLeaderboardInfo::Name(), "lbinfo", sPostData, response, document))
    {
        if (!document.HasMember("LeaderboardData"))
        {
            response.Result = ApiResult::Error;
            response.ErrorMessage = ra::StringPrintf("%s not found in response", "LeaderboardData");
        }
        else
        {
            response.Result = ApiResult::Success;

            const auto& LeaderboardData = document["LeaderboardData"];

            GetRequiredJsonField(response.GameId, LeaderboardData, "GameID", response);
            GetRequiredJsonField(response.ConsoleId, LeaderboardData, "ConsoleID", response);
            unsigned int nLowerIsBetter;
            GetRequiredJsonField(nLowerIsBetter, LeaderboardData, "LowerIsBetter", response);
            response.LowerIsBetter = (nLowerIsBetter != 0);

            if (LeaderboardData.HasMember("Entries"))
            {
                const auto& pEntries = LeaderboardData["Entries"];
                if (!pEntries.IsArray())
                {
                    response.Result = ApiResult::Error;
                    response.ErrorMessage = ra::StringPrintf("%s not an array", "Entries");
                }
                else
                {
                    const auto& pEntriesArray = pEntries.GetArray();
                    response.Entries.reserve(pEntriesArray.Size());
                    for (const auto& pEntry : pEntriesArray)
                    {
                        FetchLeaderboardInfo::Response::Entry entry;
                        GetRequiredJsonField(entry.Rank, pEntry, "Rank", response);
                        GetRequiredJsonField(entry.User, pEntry, "User", response);
                        GetRequiredJsonField(entry.Score, pEntry, "Score", response);
                        unsigned int nTime;
                        GetRequiredJsonField(nTime, pEntry, "DateSubmitted", response);
                        entry.DateSubmitted = nTime;
                        response.Entries.emplace_back(entry);
                    }
                }
            }
        }
    }

    return std::move(response);
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
    }

    return response;
}

FetchGamesList::Response ConnectedServer::FetchGamesList(const FetchGamesList::Request& request)
{
    FetchGamesList::Response response;
    rapidjson::Document document;
    std::string sPostData;

    AppendUrlParam(sPostData, "c", std::to_string(request.ConsoleId));

    if (DoRequest(m_sHost, FetchGamesList::Name(), "gameslist", sPostData, response, document))
    {
        if (!document.HasMember("Response"))
        {
            response.Result = ApiResult::Error;
            if (response.ErrorMessage.empty())
                response.ErrorMessage = ra::BuildString("Response", " not found in response");
        }
        else
        {
            response.Result = ApiResult::Success;

            auto& Data = document["Response"];
            for (auto iter = Data.MemberBegin(); iter != Data.MemberEnd(); ++iter)
            {
                if (!iter->name.IsString())
                {
                    response.Result = ApiResult::Error;
                    if (response.ErrorMessage.empty())
                        response.ErrorMessage = "Non-string key found in response";

                    break;
                }

                if (!iter->value.IsString())
                {
                    response.Result = ApiResult::Error;
                    if (response.ErrorMessage.empty())
                        response.ErrorMessage = ra::BuildString("Non-string value found in response for key ", iter->name.GetString());

                    break;
                }

                char* pEnd;
                const auto nGameId = strtoul(iter->name.GetString(), &pEnd, 10);
                if (nGameId == 0 || (pEnd && *pEnd))
                {
                    response.Result = ApiResult::Error;
                    if (response.ErrorMessage.empty())
                        response.ErrorMessage = ra::BuildString("Invalid game ID: ", iter->name.GetString());

                    break;
                }

                response.Games.emplace_back(nGameId, ra::Widen(iter->value.GetString()));
            }
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
                response.ErrorMessage = ra::BuildString("Response", " not found in response");
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
                response.ErrorMessage = ra::BuildString("Response", " not found in response");
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
                response.ErrorMessage = ra::BuildString("Response", " not found in response");
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
