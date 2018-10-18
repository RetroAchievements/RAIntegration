#ifndef RA_HTTPTHREAD_H
#define RA_HTTPTHREAD_H
#pragma once

#include "ra_fwd.h"
#include "RA_StringUtils.h"

enum HTTPRequestMethod
{
    Post,
    Get,
    _TerminateThread,

    NumHTTPRequestMethods
};

enum RequestType
{
    //	Login
    RequestLogin,

    //	Fetch
    RequestScore,
    RequestNews,
    RequestPatch,
    RequestLatestClientPage,
    RequestRichPresence,
    RequestAchievementInfo,
    RequestLeaderboardInfo,
    RequestCodeNotes,
    RequestFriendList,
    RequestBadgeIter,
    RequestUnlocks,
    RequestHashLibrary,
    RequestGamesList,
    RequestAllProgress,
    RequestGameID,

    //	Submit
    RequestPing,
    RequestPostActivity,
    RequestSubmitAwardAchievement,
    RequestSubmitCodeNote,
    RequestSubmitLeaderboardEntry,
    RequestSubmitAchievementData,
    RequestSubmitTicket,
    RequestSubmitNewTitle,

    //	Media:
    RequestUserPic,
    RequestBadge,


    NumRequestTypes
};

enum UploadType
{
    //	Upload:
    RequestUploadBadgeImage,

    NumUploadTypes
};

extern const char* RequestTypeToString[];

typedef std::map<char, std::string> PostArgs;

extern std::string PostArgsToString(const PostArgs& args);

void AppendNTVersion(std::string &sUserAgent);

void AppendIntegrationVersion(std::string &sUserAgent);

class RequestObject
{
public:
    RequestObject(RequestType nType, const PostArgs& PostArgs = PostArgs(), const std::string& sData = "") :
        m_nType(nType), m_PostArgs(PostArgs), m_sData(sData)
    {
    }

public:
    const RequestType GetRequestType() const { return m_nType; }
    const PostArgs& GetPostArgs() const { return m_PostArgs; }
    const std::string& GetData() const { return m_sData; }

    std::string& GetResponse() { return m_sResponse; }
    const std::string& GetResponse() const { return m_sResponse; }
    void SetResponse(const std::string& sResponse) { m_sResponse = sResponse; }

    BOOL ParseResponseToJSON(rapidjson::Document& rDocOut);

private:
    const RequestType m_nType;
    const PostArgs m_PostArgs;
    const std::string m_sData;

    std::string m_sResponse;
};

class HttpResults
{
public:
    //	Caller must manage: SAFE_DELETE when finished
    RequestObject * PopNextItem();
    const RequestObject* PeekNextItem() const;
    void PushItem(RequestObject* pObj);
    void Clear();
    size_t Count() const;
    BOOL PageRequestExists(RequestType nType, const std::string& sData) const;

private:
    std::deque<RequestObject*> m_aRequests;
};

class RAWeb
{
public:
    static void LogJSON(const rapidjson::Document& doc);

    static void CreateThreadedHTTPRequest(RequestType nType, const PostArgs& PostData = PostArgs(), const std::string& sData = "");
    static BOOL HTTPResponseExists(RequestType nType, const std::string& sData);

    static BOOL DoBlockingRequest(RequestType nType, const PostArgs& PostData, rapidjson::Document& JSONResponseOut);
    static BOOL DoBlockingRequest(RequestType nType, const PostArgs& PostData, std::string& ResponseOut);

    static BOOL DoBlockingHttpGet(const std::string& sRequestedPage, std::string& ResponseOut, bool bIsImageRequest);
    static BOOL DoBlockingHttpPost(const std::string& sRequestedPage, const std::string& sPostString, std::string& ResponseOut);

    static BOOL DoBlockingImageUpload(UploadType nType, const std::string& sFilename, rapidjson::Document& ResponseOut);

    static void SendKeepAlive();

    static HANDLE Mutex() { return ms_hHTTPMutex; }
    static RequestObject* PopNextHttpResult() { return ms_LastHttpResults.PopNextItem(); }

    static void SetUserAgentString();
    static void SetUserAgent(const std::string& sValue) { m_sUserAgent = ra::Widen(sValue); }
    static const std::wstring& GetUserAgent() { return m_sUserAgent; }

private:
    static HANDLE ms_hHTTPMutex;
    static HttpResults ms_LastHttpResults;
    static time_t ms_tSendNextKeepAliveAt;

    static std::wstring m_sUserAgent;
};


#endif // !RA_HTTPTHREAD_H
