#include "RA_httpthread.h"

#include "RA_Core.h"
#include "RA_User.h"

#include "RA_BuildVer.h"
#include "RA_AchievementSet.h"
#include "RA_Dlg_AchEditor.h"
#include "RA_Dlg_Memory.h"
#include "RA_Dlg_MemBookmark.h"

#include "data\EmulatorContext.hh"
#include "data\GameContext.hh"

#include "services\Http.hh"
#include "services\IConfiguration.hh"
#include "services\IHttpRequester.hh"
#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

#include <winhttp.h>

const char* RequestTypeToString[] =
{
    "RequestScore",
    "RequestNews",
    "RequestRichPresence",
    "RequestHashLibrary",
    "RequestAllProgress",
};
static_assert(SIZEOF_ARRAY(RequestTypeToString) == NumRequestTypes, "Must match up!");

const char* RequestTypeToPost[] =
{
    "score",
    "news",
    "richpresencepatch",
    "hashlibrary",
    "allprogress",
};
static_assert(SIZEOF_ARRAY(RequestTypeToPost) == NumRequestTypes, "Must match up!");

//  No game-specific code here please!

std::vector<HANDLE> g_vhHTTPThread;

HANDLE RAWeb::ms_hHTTPMutex = nullptr;
HttpResults RAWeb::ms_LastHttpResults;
time_t RAWeb::ms_tSendNextKeepAliveAt = time(nullptr);

std::wstring RAWeb::m_sUserAgent = ra::Widen("RetroAchievements Toolkit " RA_INTEGRATION_VERSION_PRODUCT);

BOOL RequestObject::ParseResponseToJSON(rapidjson::Document& rDocOut)
{
    rDocOut.Parse(GetResponse().c_str());

    if (rDocOut.HasParseError())
        RA_LOG("Possible parse issue on response, %s (%s)\n", rapidjson::GetParseError_En(rDocOut.GetParseError()),
               gsl::at(RequestTypeToString, m_nType));

    return !rDocOut.HasParseError();
}

static void AppendIntegrationVersion(_Inout_ std::string& sUserAgent)
{
    sUserAgent.append(ra::StringPrintf("%d.%d.%d.%d", RA_INTEGRATION_VERSION_MAJOR,
                                       RA_INTEGRATION_VERSION_MINOR, RA_INTEGRATION_VERSION_PATCH,
                                       RA_INTEGRATION_VERSION_REVISION));
    
    if constexpr (_CONSTANT_LOC pos{ std::string_view{ RA_INTEGRATION_VERSION_PRODUCT }.find('-') }; pos != std::string_view::npos)
    {
        constexpr std::string_view sAppend{ RA_INTEGRATION_VERSION_PRODUCT };
        sUserAgent.append(sAppend, pos);
    }
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

        fnRtlGetVersion RtlGetVersion{};
        GSL_SUPPRESS_TYPE1 RtlGetVersion =
            reinterpret_cast<fnRtlGetVersion>(::GetProcAddress(ntModule, "RtlGetVersion"));

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

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    const auto& sClientName = pEmulatorContext.GetClientName();
    if (!sClientName.empty())
    {
        sUserAgent.append(sClientName);
        sUserAgent.append("/");
    }
    else
    {
        sUserAgent.append("UnknownClient/");
    }
    sUserAgent.append(pEmulatorContext.GetClientVersion());
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

    sLogPage = "dorequest.php";
    sUrl += "/";
    sUrl += sLogPage;
    sLogPage += "?r=";
    sLogPage += gsl::at(RequestTypeToPost, nType);

    RA_LOG("POST to %s&%s", sLogPage.c_str(), sPostData.c_str());

    if (!sPostData.empty())
        sPostData.push_back('&');
    sPostData += "r=";
    sPostData += gsl::at(RequestTypeToPost, nType);

    ra::services::Http::Request request(sUrl);
    request.SetPostData(sPostData);
    auto response = request.Call();

    if (response.StatusCode() != ra::services::Http::StatusCode::OK)
    {
        RA_LOG("Error %u from %s: %s", response.StatusCode(), sLogPage.c_str(), response.Content().c_str());
        Response.clear();
        return false;
    }

    Response = response.Content();

    RA_LOG("Received %zu bytes from %s: %s", response.Content().length(), sLogPage.c_str(), response.Content().c_str());

    return TRUE;
}

//  Adds items to the httprequest queue
void RAWeb::CreateThreadedHTTPRequest(RequestType nType, const PostArgs& PostData, const std::string& sData)
{
    ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().RunAsync([nType, PostData, sData]() mutable
    {
        auto pObj = std::make_unique<RequestObject>(nType, PostData, sData);
        std::string sResponse;
        DoBlockingRequest(pObj->GetRequestType(), pObj->GetPostArgs(), sResponse);

        pObj->SetResponse(sResponse);
        ms_LastHttpResults.PushItem(std::move(pObj)); // pointer gets corrupted if released here
    });
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

void HttpResults::PushItem(std::unique_ptr<RequestObject> pObj)
{
    WaitForSingleObject(RAWeb::Mutex(), INFINITE);
    {
        m_aRequests.push_front(pObj.release());
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

size_t HttpResults::Count() const noexcept
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
