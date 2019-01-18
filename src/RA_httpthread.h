#ifndef RA_HTTPTHREAD_H
#define RA_HTTPTHREAD_H
#pragma once

#include "RA_StringUtils.h"
#include "ra_fwd.h"

enum HTTPRequestMethod
{
    Post,
    Get,
    _TerminateThread,

    NumHTTPRequestMethods
};

enum RequestType
{
    //	Fetch
    RequestScore,
    RequestNews,
    RequestRichPresence,
    RequestAchievementInfo,
    RequestLeaderboardInfo,
    RequestCodeNotes,
    RequestFriendList,
    RequestBadgeIter,
    RequestHashLibrary,
    RequestGamesList,
    RequestAllProgress,

    //	Submit
    RequestSubmitCodeNote,
    RequestSubmitLeaderboardEntry,
    RequestSubmitAchievementData,
    RequestSubmitTicket,
    RequestSubmitNewTitle,

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

class RequestObject
{
public:
    RequestObject(RequestType nType, const PostArgs& PostArgs = PostArgs(), const std::string& sData = "") :
        m_nType(nType),
        m_PostArgs(PostArgs),
        m_sData(sData)
    {}

public:
    const RequestType GetRequestType() const noexcept { return m_nType; }
    const PostArgs& GetPostArgs() const noexcept { return m_PostArgs; }
    const std::string& GetData() const noexcept { return m_sData; }

    std::string& GetResponse() noexcept { return m_sResponse; }
    const std::string& GetResponse() const noexcept { return m_sResponse; }
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
    RequestObject* PopNextItem();
    const RequestObject* PeekNextItem() const;
    void PushItem(std::unique_ptr<RequestObject> pOwner);
    void Clear();
    size_t Count() const noexcept;

private:
    std::deque<RequestObject*> m_aRequests;
};

class RAWeb
{
public:
    static void CreateThreadedHTTPRequest(RequestType nType, const PostArgs& PostData = PostArgs(),
                                          const std::string& sData = "");

    static BOOL DoBlockingRequest(RequestType nType, const PostArgs& PostData, rapidjson::Document& JSONResponseOut);
    static BOOL DoBlockingRequest(RequestType nType, const PostArgs& PostData, std::string& ResponseOut);

    static BOOL DoBlockingImageUpload(UploadType nType, const std::string& sFilename, rapidjson::Document& ResponseOut);

    static HANDLE Mutex() noexcept { return ms_hHTTPMutex; }
    static RequestObject* PopNextHttpResult() { return ms_LastHttpResults.PopNextItem(); }

    static void SetUserAgentString();
    static void SetUserAgent(const std::string& sValue) { m_sUserAgent = ra::Widen(sValue); }
    static const std::wstring& GetUserAgent() noexcept { return m_sUserAgent; }

private:
    static HANDLE ms_hHTTPMutex;
    static HttpResults ms_LastHttpResults;
    static time_t ms_tSendNextKeepAliveAt;

    static std::wstring m_sUserAgent;
};

#endif // !RA_HTTPTHREAD_H
