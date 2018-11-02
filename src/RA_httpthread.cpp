#include "RA_httpthread.h"

#include "RA_Core.h"
#include "RA_User.h"

#include "RA_BuildVer.h"
#include "RA_AchievementSet.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_Dlg_Memory.h"
#include "RA_Dlg_MemBookmark.h"
#include "RA_RichPresence.h"

#include "data\GameContext.hh"

#include "services\Http.hh"
#include "services\IConfiguration.hh"
#include "services\IHttpRequester.hh"
#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

const char* RequestTypeToString[] =
{
    "RequestLogin",

    "RequestScore",
    "RequestNews",
    "RequestPatch",
    "RequestLatestClientPage",
    "RequestRichPresence",
    "RequestAchievementInfo",
    "RequestLeaderboardInfo",
    "RequestCodeNotes",
    "RequestFriendList",
    "RequestBadgeIter",
    "RequestUnlocks",
    "RequestHashLibrary",
    "RequestGamesList",
    "RequestAllProgress",
    "RequestGameID",

    "RequestPing",
    "RequestPostActivity",
    "RequestSubmitAwardAchievement",
    "RequestSubmitCodeNote",
    "RequestSubmitLeaderboardEntry",
    "RequestSubmitAchievementData",
    "RequestSubmitTicket",
    "RequestSubmitNewTitleEntry",
};
static_assert(SIZEOF_ARRAY(RequestTypeToString) == NumRequestTypes, "Must match up!");

const char* RequestTypeToPost[] =
{
    "login",
    "score",
    "news",
    "patch",
    "latestclient",
    "richpresencepatch",
    "achievementwondata",
    "lbinfo",
    "codenotes2",
    "getfriendlist",
    "badgeiter",
    "unlocks",
    "hashlibrary",
    "gameslist",
    "allprogress",
    "gameid",

    "ping",
    "postactivity",
    "awardachievement",
    "submitcodenote",
    "submitlbentry",
    "uploadachievement",
    "submitticket",
    "submitgametitle",
};
static_assert(SIZEOF_ARRAY(RequestTypeToPost) == NumRequestTypes, "Must match up!");

const char* UploadTypeToString[] =
{
    "RequestUploadBadgeImage",
};
static_assert(SIZEOF_ARRAY(UploadTypeToString) == NumUploadTypes, "Must match up!");

const char* UploadTypeToPost[] =
{
    "uploadbadgeimage",
};
static_assert(SIZEOF_ARRAY(UploadTypeToPost) == NumUploadTypes, "Must match up!");

//  No game-specific code here please!

std::vector<HANDLE> g_vhHTTPThread;

HANDLE RAWeb::ms_hHTTPMutex = nullptr;
HttpResults RAWeb::ms_LastHttpResults;
time_t RAWeb::ms_tSendNextKeepAliveAt = time(nullptr);

PostArgs PrevArgs;

std::wstring RAWeb::m_sUserAgent = ra::Widen("RetroAchievements Toolkit " RA_INTEGRATION_VERSION_PRODUCT);

BOOL RequestObject::ParseResponseToJSON(rapidjson::Document& rDocOut)
{
    rDocOut.Parse(GetResponse().c_str());

    if (rDocOut.HasParseError())
        RA_LOG("Possible parse issue on response, %s (%s)\n", rapidjson::GetParseError_En(rDocOut.GetParseError()), RequestTypeToString[m_nType]);

    return !rDocOut.HasParseError();
}

static void AppendIntegrationVersion(_Inout_ std::string& sUserAgent)
{
    sUserAgent.append(ra::StringPrintf("%d.%d.%d.%d", RA_INTEGRATION_VERSION_MAJOR,
                                       RA_INTEGRATION_VERSION_MINOR, RA_INTEGRATION_VERSION_REVISION,
                                       RA_INTEGRATION_VERSION_MODIFIED));

    _CONSTANT_LOC posFound{ std::string_view{ RA_INTEGRATION_VERSION_PRODUCT }.find('-') };
    constexpr std::string_view sAppend{ RA_INTEGRATION_VERSION_PRODUCT };
    sUserAgent.append(sAppend, posFound);

}

static void AppendNTVersion(_Inout_ std::string& sUserAgent)
{
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms724832(v=vs.85).aspx
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms724429(v=vs.85).aspx
    // https://github.com/DarthTon/Blackbone/blob/master/contrib/VersionHelpers.h
#ifndef NTSTATUS
    using NTSTATUS = __success(return >= 0) LONG;
#endif
    if (const auto ntModule{ ::GetModuleHandleW(L"ntdll.dll") }; ntModule)
    {
        RTL_OSVERSIONINFOEXW osVersion{ sizeof(RTL_OSVERSIONINFOEXW) };
        using fnRtlGetVersion = NTSTATUS(NTAPI*)(PRTL_OSVERSIONINFOEXW);
        const auto RtlGetVersion{
            reinterpret_cast<fnRtlGetVersion>(::GetProcAddress(ntModule, "RtlGetVersion"))
        };
        if (RtlGetVersion)
        {
            RtlGetVersion(&osVersion);
            if (osVersion.dwMajorVersion > 0UL)
            {
                sUserAgent.append(ra::StringPrintf("WindowsNT %lu.%lu", osVersion.dwMajorVersion,
                                                   osVersion.dwMinorVersion));
            }
        }
    }
}

void RAWeb::SetUserAgentString()
{
    std::string sUserAgent;
    sUserAgent.reserve(128U);

    if (g_sClientName != nullptr)
    {
        sUserAgent.append(g_sClientName);
        sUserAgent.append("/");
    }
    else
    {
        sUserAgent.append("UnknownClient/");
    }
    sUserAgent.append(g_sClientVersion);
    sUserAgent.append(" (");

    AppendNTVersion(sUserAgent);
    sUserAgent.append(") Integration/");
    AppendIntegrationVersion(sUserAgent);

    RA_LOG("User-Agent: %s", sUserAgent.c_str());

    ra::services::ServiceLocator::GetMutable<ra::services::IHttpRequester>().SetUserAgent(sUserAgent);

    SetUserAgent(sUserAgent);
}

BOOL RAWeb::DoBlockingRequest(RequestType nType, const PostArgs& PostData, rapidjson::Document& JSONResponseOut)
{
    std::string response;
    if (DoBlockingRequest(nType, PostData, response))
    {
        if (response.size() > 0)
        {
            JSONResponseOut.Parse(response.c_str());
            //LogJSON( JSONResponseOut );   //  Already logged during DoBlockingRequest()?

            if (JSONResponseOut.HasParseError())
            {
                RA_LOG("JSON Parse Error encountered!\n");
            }

            return(!JSONResponseOut.HasParseError());
        }
    }

    return FALSE;
}

BOOL RAWeb::DoBlockingRequest(RequestType nType, const PostArgs& PostData, std::string& Response)
{
    std::string sUrl = _RA_HostName();
    std::string sPostData = PostArgsToString(PostData);
    std::string sLogPage;

    if (nType == RequestLogin)
    {
        sLogPage = "login_app.php";
        sUrl += "/";
        sUrl += sLogPage;

        RA_LOG("POST to %s", sLogPage.c_str()); // do not log user credentials (sPostData)
    }
    else
    {
        sLogPage = "dorequest.php";
        sUrl += "/";
        sUrl += sLogPage;
        sLogPage += "?r=";
        sLogPage += RequestTypeToPost[nType];

        RA_LOG("POST to %s&%s", sLogPage.c_str(), sPostData.c_str());

        if (!sPostData.empty())
            sPostData.push_back('&');
        sPostData += "r=";
        sPostData += RequestTypeToPost[nType];
    }

    ra::services::Http::Request request(sUrl);
    request.SetPostData(sPostData);
    auto response = request.Call();

    if (response.StatusCode() != 200)
    {
        RA_LOG("Error %u from %s: %s", response.StatusCode(), sLogPage.c_str(), response.Content().c_str());
        Response.clear();
        return false;
    }

    Response = response.Content();

    RA_LOG("Received %zu bytes from %s: %s", response.Content().length(), sLogPage.c_str(), response.Content().c_str());

    return TRUE;
}

BOOL DoBlockingImageUpload(UploadType nType, const std::string& sFilename, std::string& ResponseOut)
{
    ASSERT(nType == UploadType::RequestUploadBadgeImage); // Others not yet supported, see "r=" below

    const std::string sRequestedPage = "doupload.php";
    const std::string sRTarget = UploadTypeToPost[nType]; //"uploadbadgeimage";

    RA_LOG(__FUNCTION__ ": (%04x) uploading \"%s\" to %s...\n", GetCurrentThreadId(), sFilename.c_str(), sRequestedPage.c_str());

    BOOL bSuccess = FALSE;
    HINTERNET hConnect = nullptr, hRequest = nullptr;

    size_t nTemp;

    // Use WinHttpOpen to obtain a session handle.
    HINTERNET hSession = WinHttpOpen(RAWeb::GetUserAgent().c_str(),
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    // Specify an HTTP server.
    if (hSession != nullptr)
    {
        hConnect = WinHttpConnect(hSession, ra::Widen(_RA_HostName()).c_str(), INTERNET_DEFAULT_HTTP_PORT, 0);
    }

    if (hConnect != nullptr)
    {
        WCHAR wBuffer[1024];
        mbstowcs_s(&nTemp, wBuffer, 1024, sRequestedPage.c_str(), strlen(sRequestedPage.c_str()) + 1);

        hRequest = WinHttpOpenRequest(hConnect,
            L"POST",
            wBuffer,
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            0);
    }

    if (hRequest != nullptr)
    {
        //---------------------------41184676334
        const char* mimeBoundary = "---------------------------41184676334";
        const wchar_t* contentType = L"Content-Type: multipart/form-data; boundary=---------------------------41184676334\r\n";

        int nResult = WinHttpAddRequestHeaders(hRequest, contentType, (unsigned long)-1, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW);
        if (nResult != 0)
        {
            // Add the photo to the stream
            std::ifstream f(sFilename, std::ios::binary);

            std::string sRTargetAndExtension = sRTarget + sFilename.substr(sFilename.length() - 4);

            std::ostringstream sb_ascii;
            //sb_ascii << str;                                  
            sb_ascii << "--" << mimeBoundary << "\r\n";                                                         //  --Boundary
            sb_ascii << "Content-Disposition: form-data; name=\"file\"; filename=\"" << sRTargetAndExtension << "\"\r\n";   //  Item header    'file' - Hijacking to determine request type!
            sb_ascii << "\r\n";                                                                                 //  Spacing
            sb_ascii << f.rdbuf();                                                                              //  Binary content
            sb_ascii << "\r\n";                                                                                 //  Spacing
            sb_ascii << "--" << mimeBoundary << "--\r\n";                                                       //  --Boundary--

                                                                                                                //  ## EXPERIMENTAL ##
                                                                                                                //sb_ascii << "Content-Disposition: form-data; name=\"r\"\r\n";                                     //  Item header    'r'
                                                                                                                //sb_ascii << "\r\n";                                                                                   //  Spacing
                                                                                                                //sb_ascii << sRTarget << "\r\n";                                                                       //  Binary content
                                                                                                                //sb_ascii << "\r\n";                                                                                   //  Spacing
                                                                                                                //sb_ascii << "--" << mimeBoundary << "--\r\n";                                                     //  --Boundary--

            const std::string str = sb_ascii.str();

            //  Inject type of request

            bSuccess = WinHttpSendRequest(
                hRequest,
                WINHTTP_NO_ADDITIONAL_HEADERS,
                0,
                (void*)str.c_str(),
                static_cast<unsigned long>(str.length()),
                static_cast<unsigned long>(str.length()),
                0);
        }

        if (WinHttpReceiveResponse(hRequest, nullptr))
        {
            //BYTE* sDataDestOffset = &pBufferOut[0];

            DWORD nBytesToRead = 0;
            WinHttpQueryDataAvailable(hRequest, &nBytesToRead);

            //  Note: success is much earlier, as 0 bytes read is VALID
            //  i.e. fetch achievements for new game will return 0 bytes.
            bSuccess = nBytesToRead > 0;

            while (nBytesToRead > 0)
            {
                ASSERT(nBytesToRead <= 8192);
                if (nBytesToRead <= 8192)
                {
                    DWORD nBytesFetched = 0;

                    auto pData{ std::make_unique<char[]>(nBytesToRead) };
                    if (WinHttpReadData(hRequest, pData.get(), nBytesToRead, &nBytesFetched))
                    {
                        ASSERT(nBytesToRead == nBytesFetched);
                        ResponseOut.insert(ResponseOut.end(), pData.get(), pData.get() + nBytesFetched);
                    }
                }

                WinHttpQueryDataAvailable(hRequest, &nBytesToRead);
            }

            if (ResponseOut.size() > 0)
                ResponseOut.push_back('\0');    //  EOS for parsing

            RA_LOG(__FUNCTION__ ": success! Returned %u bytes.", ResponseOut.size());
        }
    }

    return bSuccess;
}

BOOL RAWeb::DoBlockingImageUpload(UploadType nType, const std::string& sFilename, rapidjson::Document& ResponseOut)
{
    std::string response;
    if (::DoBlockingImageUpload(nType, sFilename, response))
    {
        ResponseOut.Parse(response.c_str());
        if (!ResponseOut.HasParseError())
        {
            return TRUE;
        }
        else
        {
            RA_LOG(__FUNCTION__ " (%d, %s) has parse error: %s\n", nType, sFilename.c_str(), GetParseError_En(ResponseOut.GetParseError()));
            return FALSE;
        }
    }
    else
    {
        RA_LOG(__FUNCTION__ " (%d, %s) could not connect?\n", nType, sFilename.c_str());
        return FALSE;
    }
}

//  Adds items to the httprequest queue
void RAWeb::CreateThreadedHTTPRequest(RequestType nType, const PostArgs& PostData, const std::string& sData)
{
    auto* pObj = new RequestObject(nType, PostData, sData);

    ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().RunAsync([pObj]()
    {
        std::string sResponse;
        DoBlockingRequest(pObj->GetRequestType(), pObj->GetPostArgs(), sResponse);

        pObj->SetResponse(sResponse);
        ms_LastHttpResults.PushItem(pObj);
    });
}

//////////////////////////////////////////////////////////////////////////

void RAWeb::SendKeepAlive()
{
    if (!RAUsers::LocalUser().IsLoggedIn())
        return;

    //  Post a pingback once every few minutes to keep the server aware of our presence
    time_t tNow = time(nullptr);
    if (tNow < ms_tSendNextKeepAliveAt)
        return;

    ms_tSendNextKeepAliveAt = tNow + SERVER_PING_DURATION;
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

    if (pGameContext.GameId() == 0)
        return;

    PostArgs args;
    args['u'] = RAUsers::LocalUser().Username();
    args['t'] = RAUsers::LocalUser().Token();
    args['g'] = std::to_string(pGameContext.GameId());

    if (RA_GameIsActive())
    {
        if (g_MemoryDialog.IsActive() || g_AchievementEditorDialog.IsActive() || g_MemBookmarkDialog.IsActive())
        {
            if (!g_pActiveAchievements || g_pActiveAchievements->NumAchievements() == 0)
                args['m'] = "Developing Achievements";
            else if (_RA_HardcoreModeIsActive())
                args['m'] = "Inspecting Memory in Hardcore mode";
            else if (g_nActiveAchievementSet == AchievementSet::Type::Core)
                args['m'] = "Fixing Achievements";
            else
                args['m'] = "Developing Achievements";
        }
        else
        {
            const std::string& sRPResponse = g_RichPresenceInterpreter.GetRichPresenceString();
            if (!sRPResponse.empty())
                args['m'] = sRPResponse;
            else if (g_pActiveAchievements && g_pActiveAchievements->NumAchievements() > 0)
                args['m'] = "Earning Achievements";
            else
                args['m'] = "Playing " + ra::Narrow(pGameContext.GameTitle());
        }
    }

    //  Scott: Temporarily removed; Ping and RP are merged at current
    //   and if we don't constantly poll the server, the players are dropped
    //   from 'currently playing'.
    //if (args['m'] != PrevArgs['m'] || args['g'] != PrevArgs['g'])
    {
        RAWeb::CreateThreadedHTTPRequest(RequestPing, args);
        PrevArgs = args;
    }
}

//////////////////////////////////////////////////////////////////////////

RequestObject* HttpResults::PopNextItem()
{
    RequestObject* pRetVal = nullptr;
    WaitForSingleObject(RAWeb::Mutex(), INFINITE);
    {
        if (m_aRequests.size() > 0)
        {
            pRetVal = m_aRequests.front();
            m_aRequests.pop_front();
        }
    }
    ReleaseMutex(RAWeb::Mutex());

    return pRetVal;
}

const RequestObject* HttpResults::PeekNextItem() const
{
    RequestObject* pRetVal = nullptr;
    WaitForSingleObject(RAWeb::Mutex(), INFINITE);
    {
        pRetVal = m_aRequests.front();
    }
    ReleaseMutex(RAWeb::Mutex());

    return pRetVal;
}

void HttpResults::PushItem(RequestObject* pObj)
{
    WaitForSingleObject(RAWeb::Mutex(), INFINITE);
    {
        m_aRequests.push_front(pObj);
    }
    ReleaseMutex(RAWeb::Mutex());
}

void HttpResults::Clear()
{
    WaitForSingleObject(RAWeb::Mutex(), INFINITE);
    {
        while (!m_aRequests.empty())
        {
            RequestObject* pObj = m_aRequests.front();
            m_aRequests.pop_front();
            SAFE_DELETE(pObj);
        }
    }
    ReleaseMutex(RAWeb::Mutex());
}

size_t HttpResults::Count() const
{
    size_t nCount = 0;
    WaitForSingleObject(RAWeb::Mutex(), INFINITE);
    {
        nCount = m_aRequests.size();
    }
    ReleaseMutex(RAWeb::Mutex());

    return nCount;
}

std::string PostArgsToString(const PostArgs& args)
{
    std::string str;
    str.reserve(args.size() * 32); // estimate 32 bytes per arg/value pair

    for (auto& arg : args)
    {
        if (!str.empty())
            str.push_back('&');

        str += arg.first;
        str.push_back('=');
        ra::services::Http::UrlEncodeAppend(str, arg.second);
    }

    return str;
}
