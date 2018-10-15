#include "RA_httpthread.h"

#include "RA_Core.h"
#include "RA_User.h"

#include "RA_BuildVer.h"
#include "RA_AchievementSet.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_Dlg_Memory.h"
#include "RA_Dlg_MemBookmark.h"
#include "RA_GameData.h"
#include "RA_RichPresence.h"

#include "services\IConfiguration.hh"
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

    "RequestUserPic",
    "RequestBadge",
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

    "_requestuserpic_",     //  TBD RequestUserPic
    "_requestbadge_",       //  TBD RequestBadge
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

//BOOL RAWeb::DoBlockingHttpGet( const std::string& sRequestedPage, DataStream& ResponseOut )
//{
//  RA_LOG( __FUNCTION__ ": (%08x) GET from %s...\n", GetCurrentThreadId(), sRequestedPage );
//
//  BOOL bResults = FALSE, bSuccess = FALSE;
//  HINTERNET hSession = nullptr, hConnect = nullptr, hRequest = nullptr;
//
//  WCHAR wBuffer[1024];
//  size_t nTemp;
//  //BYTE* sDataDestOffset = &pBufferOut[0];
//  DWORD nBytesToRead = 0;
//  DWORD nBytesFetched = 0;
//
//  char sClientName[1024];
//  sprintf_s( sClientName, 1024, "Retro Achievements Client %s %s", g_sClientName, g_sClientVersion );
//  WCHAR wClientNameBuffer[1024];
//  mbstowcs_s( &nTemp, wClientNameBuffer, 1024, sClientName, strlen( sClientName )+1 );
//
//  // Use WinHttpOpen to obtain a session handle.
//  hSession = WinHttpOpen( wClientNameBuffer, 
//      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
//      WINHTTP_NO_PROXY_NAME, 
//      WINHTTP_NO_PROXY_BYPASS, 0);
//
//  // Specify an HTTP server.
//  if( hSession != nullptr )
//  {
//      if( strncmp( sRequestedPage, "Badge", 5 ) == 0 )
//          hConnect = WinHttpConnect( hSession, RA_IMG_HOST_W, INTERNET_DEFAULT_HTTP_PORT, 0 );
//      else
//          hConnect = WinHttpConnect( hSession, RA_HOST_W, INTERNET_DEFAULT_HTTP_PORT, 0 );
//
//      // Create an HTTP Request handle.
//      if( hConnect != nullptr )
//      {
//          mbstowcs_s( &nTemp, wBuffer, 1024, sRequestedPage, strlen(sRequestedPage)+1 );
//
//          hRequest = WinHttpOpenRequest( hConnect, 
//              L"GET", 
//              wBuffer, 
//              nullptr, 
//              WINHTTP_NO_REFERER, 
//              WINHTTP_DEFAULT_ACCEPT_TYPES,
//              0);
//
//          // Send a Request.
//          if( hRequest != nullptr )
//          {
//              bResults = WinHttpSendRequest( hRequest, 
//                  L"Content-Type: application/x-www-form-urlencoded",
//                  0, 
//                  WINHTTP_NO_REQUEST_DATA,
//                  0, 
//                  0,
//                  0 );
//
//              if( WinHttpReceiveResponse( hRequest, nullptr ) )
//              {
//                  nBytesToRead = 0;
//                  nBytesRead = 0;
//                  WinHttpQueryDataAvailable( hRequest, &nBytesToRead );
//
//                  std::stringstream sstr;
//
//                  while( nBytesToRead > 0 )
//                  {
//                      char sHttpReadData[8192];
//                      ZeroMemory( sHttpReadData, 8192 );
//
//                      assert( nBytesToRead <= 8192 );
//                      if( nBytesToRead <= 8192 )
//                      {
//                          nBytesFetched = 0;
//                          if( WinHttpReadData( hRequest, &sHttpReadData, nBytesToRead, &nBytesFetched ) )
//                          {
//                              assert( nBytesToRead == nBytesFetched );
//
//                              //Read: parse buffer
//                              //memcpy( sDataDestOffset, sHttpReadData, nBytesFetched );
//                              //sDataDestOffset += nBytesFetched;
//
//                              sstr.write( sHttpReadData, nBytesFetched );
//                              nBytesRead += nBytesFetched;
//                          }
//                      }
//
//                      bSuccess = TRUE;
//
//                      WinHttpQueryDataAvailable( hRequest, &nBytesToRead );
//                  }
//
//                  RA_LOG( __FUNCTION__ ": (%08x) success! Read %d bytes...\n", GetCurrentThreadId(), nBytesRead );
//              }
//          }
//      }
//  }
//
//  // Close open handles.
//  if( hRequest != nullptr )
//      WinHttpCloseHandle( hRequest );
//  if( hConnect != nullptr )
//      WinHttpCloseHandle( hConnect );
//  if( hSession != nullptr )
//      WinHttpCloseHandle( hSession );
//
//  return bSuccess;
//}

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

    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms724832(v=vs.85).aspx
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms724429(v=vs.85).aspx
    // https://github.com/DarthTon/Blackbone/blob/master/contrib/VersionHelpers.h
#ifndef NTSTATUS
    #define NTSTATUS long
#endif
    {
        using fnRtlGetVersion = NTSTATUS(NTAPI*)(PRTL_OSVERSIONINFOEXW lpVersionInformation);

        const auto ntModule{ ::GetModuleHandleW(L"ntdll.dll") };
        if (!ntModule)
            return;

        RTL_OSVERSIONINFOEXW osVersion{ sizeof(OSVERSIONINFOEX) };
        const auto RtlGetVersion{
            reinterpret_cast<fnRtlGetVersion>(::GetProcAddress(ntModule, "RtlGetVersion"))
        };
        if (RtlGetVersion)
        {
            RtlGetVersion(&osVersion);
            sUserAgent.append("WindowsNT ");
            sUserAgent.append(std::to_string(osVersion.dwMajorVersion));
            sUserAgent.append(".");
            sUserAgent.append(std::to_string(osVersion.dwMinorVersion));
        }
    }

    sUserAgent.append(") Integration/");
    {
        std::string buffer;
        buffer.reserve(64);
        sprintf_s(buffer.data(), 64U, "%d.%d.%d.%d", RA_INTEGRATION_VERSION_MAJOR, RA_INTEGRATION_VERSION_MINOR,
                    RA_INTEGRATION_VERSION_REVISION, RA_INTEGRATION_VERSION_MODIFIED);
        sUserAgent.append(buffer);
    }

    {
        // We should already assume it's there, we just need to know where the '-' is
        _CONSTANT_LOC posFound{ std::string_view{ RA_INTEGRATION_VERSION_PRODUCT }.find('-') };
        constexpr std::string_view sAppend{ RA_INTEGRATION_VERSION_PRODUCT };
        sUserAgent.append(sAppend, posFound);
    }

    RA_LOG("User-Agent: %s", sUserAgent.c_str());

    SetUserAgent(sUserAgent);
}

void RAWeb::LogJSON(const rapidjson::Document& doc)
{
    //  DebugLog:
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer{ buffer };
    doc.Accept(writer);

    //  buffer may contain percentage literals!
    const auto& pLogger = ra::services::ServiceLocator::Get<ra::services::ILogger>();
    if (pLogger.IsEnabled(ra::services::LogLevel::Info))
        pLogger.LogMessage(ra::services::LogLevel::Info, buffer.GetString());
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
    PostArgs args = PostData;               //  Take a copy
    args['r'] = RequestTypeToPost[nType];   //  Embed request type

    switch (nType)
    {
        case RequestUserPic:
            return DoBlockingHttpGet(std::string("UserPic/" + PostData.at('u') + ".png"), Response, false); //  UserPic needs migrating to S3...
        case RequestBadge:
            return DoBlockingHttpGet(std::string("Badge/" + PostData.at('b') + ".png"), Response, true);
        case RequestLogin:
            return DoBlockingHttpPost("login_app.php", PostArgsToString(args), Response);
        default:
            return DoBlockingHttpPost("dorequest.php", PostArgsToString(args), Response);
    }
}

BOOL RAWeb::DoBlockingHttpGet(const std::string& sRequestedPage, std::string& ResponseOut, bool bIsImageRequest)
{
    BOOL bSuccess = FALSE;

    RA_LOG(__FUNCTION__ ": (%04x) GET to %s...\n", GetCurrentThreadId(), sRequestedPage.c_str());
    ResponseOut.clear();

    const char* sHostName = bIsImageRequest ? "i.retroachievements.org" : _RA_HostName();

    size_t nTemp;

    // Use WinHttpOpen to obtain a session handle.
    HINTERNET hSession = WinHttpOpen(GetUserAgent().c_str(),
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    // Specify an HTTP server.
    if (hSession != nullptr)
    {
        HINTERNET hConnect = WinHttpConnect(hSession, ra::Widen(sHostName).c_str(), INTERNET_DEFAULT_HTTP_PORT, 0);

        // Create an HTTP Request handle.
        if (hConnect != nullptr)
        {
            WCHAR wBuffer[1024];
            mbstowcs_s(&nTemp, wBuffer, 1024, sRequestedPage.c_str(), strlen(sRequestedPage.c_str()) + 1);

            HINTERNET hRequest = WinHttpOpenRequest(hConnect,
                L"GET",
                wBuffer,
                nullptr,
                WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                0);

            // Send a Request.
            if (hRequest != nullptr)
            {
                if (WinHttpSendRequest(hRequest,
                    L"Content-Type: application/x-www-form-urlencoded",
                    0,
                    WINHTTP_NO_REQUEST_DATA, //WINHTTP_NO_REQUEST_DATA,
                    0,
                    0,
                    0) == 0)
                {
                    ::WinHttpCloseHandle(hRequest);
                    ::WinHttpCloseHandle(hConnect);
                    ::WinHttpCloseHandle(hSession);
                    return static_cast<BOOL>(ra::to_signed(::GetLastError()));
                }

                if (WinHttpReceiveResponse(hRequest, nullptr))
                {
                    DWORD nBytesToRead = 0;
                    WinHttpQueryDataAvailable(hRequest, &nBytesToRead);

                    //  Note: success is much earlier, as 0 bytes read is VALID
                    //  i.e. fetch achievements for new game will return 0 bytes.
                    bSuccess = TRUE;

                    while (nBytesToRead > 0)
                    {
                        //if( nBytesToRead <= 32 )
                        {
                            DWORD nBytesFetched = 0UL;
                            auto pData{ std::make_unique<char[]>(nBytesToRead) };
                            if (WinHttpReadData(hRequest, pData.get(), nBytesToRead, &nBytesFetched))
                            {
                                ASSERT(nBytesToRead == nBytesFetched);
                                ResponseOut.insert(ResponseOut.end(), pData.get(), pData.get() + nBytesFetched);
                            }
                            else
                            {
                                RA_LOG("Assumed timed out connection?!");
                                break;  //Timed out?
                            }
                        }

                        WinHttpQueryDataAvailable(hRequest, &nBytesToRead);
                    }

                    if (ResponseOut.size() > 0)
                        ResponseOut.push_back('\0');    //  EOS for parsing

                    RA_LOG(__FUNCTION__ ": success! %s Returned %zu bytes.", sRequestedPage.c_str(), ResponseOut.size());
                }

            }

            if (hRequest != nullptr)
                WinHttpCloseHandle(hRequest);
        }

        if (hConnect != nullptr)
            WinHttpCloseHandle(hConnect);
    }

    if (hSession != nullptr)
        WinHttpCloseHandle(hSession);

    return bSuccess;
}

BOOL RAWeb::DoBlockingHttpPost(const std::string& sRequestedPage, const std::string& sPostString, std::string& ResponseOut)
{
    BOOL bSuccess = FALSE;
    ResponseOut.clear();

    if (sRequestedPage.compare("login_app.php") == 0)
    {
        //  Special case: DO NOT LOG raw user credentials!
        RA_LOG(__FUNCTION__ ": (%04x) POST to %s (LOGIN)...\n", GetCurrentThreadId(), sRequestedPage.c_str());
    }
    else
    {
        RA_LOG(__FUNCTION__ ": (%04x) POST to %s?%s...\n", GetCurrentThreadId(), sRequestedPage.c_str(), sPostString.c_str());
    }

    HINTERNET hSession = WinHttpOpen(GetUserAgent().c_str(),
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession != nullptr)
    {
        HINTERNET hConnect = WinHttpConnect(hSession, ra::Widen(_RA_HostName()).c_str(), INTERNET_DEFAULT_HTTP_PORT, 0);
        if (hConnect != nullptr)
        {
            HINTERNET hRequest = WinHttpOpenRequest(hConnect,
                L"POST",
                ra::Widen(sRequestedPage).c_str(),
                nullptr,
                WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                0);
            if (hRequest != nullptr)
            {
                BOOL bSendSuccess = WinHttpSendRequest(hRequest,
                    L"Content-Type: application/x-www-form-urlencoded",
                    0,
                    reinterpret_cast<LPVOID>(const_cast<char*>(sPostString.data())), //WINHTTP_NO_REQUEST_DATA,
                    strlen(sPostString.c_str()),
                    strlen(sPostString.c_str()),
                    0);

                if (bSendSuccess && WinHttpReceiveResponse(hRequest, nullptr))
                {
                    //  Note: success is much earlier, as 0 bytes read is VALID
                    //  i.e. fetch achievements for new game will return 0 bytes.
                    bSuccess = TRUE;

                    DWORD nBytesToRead = 0;
                    WinHttpQueryDataAvailable(hRequest, &nBytesToRead);
                    while (nBytesToRead > 0)
                    {
                        {
                            DWORD nBytesFetched = 0;
                            auto pData{ std::make_unique<char[]>(nBytesToRead) };
                            if (WinHttpReadData(hRequest, pData.get(), nBytesToRead, &nBytesFetched))
                            {
                                ASSERT(nBytesToRead == nBytesFetched);
                                ResponseOut.insert(ResponseOut.end(), pData.get(), pData.get() + nBytesFetched);
                            }
                            else
                            {
                                RA_LOG("Assumed timed out connection?!");
                                break;  //Timed out?
                            }
                        }

                        WinHttpQueryDataAvailable(hRequest, &nBytesToRead);
                    }

                    if (ResponseOut.size() > 0)
                        ResponseOut.push_back('\0');    //  EOS for parsing

                    if (sPostString.find("r=login") != std::string::npos)
                    {
                        //  Special case: DO NOT LOG raw user credentials!
                        RA_LOG("... " __FUNCTION__ ": (%04x) LOGIN Success: %u bytes read\n", GetCurrentThreadId(), ResponseOut.size());
                    }
                    else
                    {
                        RA_LOG("-> " __FUNCTION__ ": (%04x) POST to %s?%s Success: %u bytes read\n", GetCurrentThreadId(), sRequestedPage.c_str(), sPostString.c_str(), ResponseOut.size());
                    }
                }

                WinHttpCloseHandle(hRequest);
            }

            WinHttpCloseHandle(hConnect);
        }

        WinHttpCloseHandle(hSession);
    }

    //  Debug logging...
    if (ResponseOut.size() > 0)
    {
        rapidjson::Document doc;
        doc.Parse(ResponseOut.c_str());

        if (doc.HasParseError())
        {
            RA_LOG("Cannot parse JSON!\n");
            RA_LOG(ResponseOut.c_str());
            RA_LOG("\n");
        }
        else
        {
            LogJSON(doc);
        }
    }
    else
    {
        RA_LOG(__FUNCTION__ ": (%04x) Empty JSON Response\n", GetCurrentThreadId());
    }

    return bSuccess;
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

BOOL RAWeb::HTTPResponseExists(RequestType nType, const std::string& sData)
{
    return ms_LastHttpResults.PageRequestExists(nType, sData);
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

    PostArgs args;
    args['u'] = RAUsers::LocalUser().Username();
    args['t'] = RAUsers::LocalUser().Token();
    args['g'] = std::to_string(g_pCurrentGameData->GetGameID());

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
            {
                args['m'] = sRPResponse;
            }
            else if (g_pActiveAchievements && g_pActiveAchievements->NumAchievements() > 0)
            {
                args['m'] = "Earning Achievements";
            }
            else
            {
                char buffer[128];
                snprintf(buffer, sizeof(buffer), "Playing %s", g_pCurrentGameData->GameTitle().c_str());
                args['m'] = buffer;
            }
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

BOOL HttpResults::PageRequestExists(RequestType nType, const std::string& sData) const
{
    BOOL bRetVal = FALSE;
    WaitForSingleObject(RAWeb::Mutex(), INFINITE);
    {
        std::deque<RequestObject*>::const_iterator iter = m_aRequests.begin();
        while (iter != m_aRequests.end())
        {
            const RequestObject* pObj = (*iter);
            if (pObj->GetRequestType() == nType &&
                pObj->GetData().compare(sData) == 0)
            {
                bRetVal = TRUE;
                break;
            }

            iter++;
        }
    }
    ReleaseMutex(RAWeb::Mutex());

    return bRetVal;
}

std::string PostArgsToString(const PostArgs& args)
{
    std::string str = "";
    PostArgs::const_iterator iter = args.begin();
    while (iter != args.end())
    {
        if (iter == args.begin())
            str += "";//?
        else
            str += "&";

        str += (*iter).first;
        str += "=";
        str += (*iter).second;

        iter++;
    }

    //  Replace all spaces with '+' (RFC 1738)
    std::replace(str.begin(), str.end(), ' ', '+');

    return str;
}
