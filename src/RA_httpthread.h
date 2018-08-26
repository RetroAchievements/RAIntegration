#ifndef RA_HTTPTHREAD_H
#define RA_HTTPTHREAD_H
#pragma once

#include "RA_Defs.h"

namespace ra {

namespace enum_sizes {

_CONSTANT_VAR NUM_HTTPREQUEST_METHODS{ 3 };
_CONSTANT_VAR NUM_REQUESTTYPES{ 27 };
_CONSTANT_VAR NUM_UPLOADTYPES{ 1 };

} // namespace enum_sizes

// TODO: Determine whether we still need this or put it in the source file instead
enum class HTTPRequestMethod { Post, Get, _TerminateThread };

enum class RequestType : std::size_t
{
    	
    /// <summary>Login</summary>
    Login,

    //	Fetch
    Score,
    News,
    Patch,
    LatestClientPage,
    RichPresence,
    AchievementInfo,
    LeaderboardInfo,
    CodeNotes,
    FriendList,
    BadgeIter,
    Unlocks,
    HashLibrary,
    GamesList,
    AllProgress,
    GameID,

    //	Submit
    Ping,
    PostActivity,
    SubmitAwardAchievement,
    SubmitCodeNote,
    SubmitLeaderboardEntry,
    SubmitAchievementData,
    SubmitTicket,
    SubmitNewTitle,

    //	Media:
    UserPic,
    Badge,


    /// <summary>Special</summary>
    StopThread
};

/// <summary>Client-to-Server upload codes</summary>
enum class UploadType : std::size_t { RequestUploadBadgeImage };

} // namespace ra


extern const char* RequestTypeToString[];

typedef std::map<char, std::string> PostArgs;

extern std::string PostArgsToString(const PostArgs& args);

class RequestObject
{
public:
    RequestObject(ra::RequestType nType, const PostArgs& PostArgs = PostArgs(), const std::string& sData = "") :
        m_nType(nType), m_PostArgs(PostArgs), m_sData(sData)
    {
    }

public:
    const ra::RequestType GetRequestType() const { return m_nType; }
    const PostArgs& GetPostArgs() const { return m_PostArgs; }
    const std::string& GetData() const { return m_sData; }

    std::string& GetResponse() { return m_sResponse; }
    const std::string& GetResponse() const { return m_sResponse; }
    void SetResponse(const std::string& sResponse) { m_sResponse = sResponse; }

    BOOL ParseResponseToJSON(Document& rDocOut);

private:
    const ra::RequestType m_nType;
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
    BOOL PageRequestExists(ra::RequestType nType, const std::string& sData) const;

private:
    std::deque<RequestObject*> m_aRequests;
};

class RAWeb
{
public:
    static void RA_InitializeHTTPThreads();
    static void RA_KillHTTPThreads();

    static void LogJSON(const Document& doc);

    static void CreateThreadedHTTPRequest(ra::RequestType nType, const PostArgs& PostData = PostArgs(), const std::string& sData = "");
    static BOOL HTTPRequestExists(ra::RequestType nType, const std::string& sData);
    static BOOL HTTPResponseExists(ra::RequestType nType, const std::string& sData);

    static BOOL DoBlockingRequest(ra::RequestType nType, const PostArgs& PostData, Document& JSONResponseOut);
    static BOOL DoBlockingRequest(ra::RequestType nType, const PostArgs& PostData, std::string& ResponseOut);

    static BOOL DoBlockingHttpGet(const std::string& sRequestedPage, std::string& ResponseOut, bool bIsImageRequest);
    static BOOL DoBlockingHttpPost(const std::string& sRequestedPage, const std::string& sPostString, std::string& ResponseOut);

    static BOOL DoBlockingImageUpload(ra::UploadType nType, const std::string& sFilename, Document& ResponseOut);

    static DWORD WINAPI HTTPWorkerThread(LPVOID lpParameter);

    static HANDLE Mutex() { return ms_hHTTPMutex; }
    static RequestObject* PopNextHttpResult() { return ms_LastHttpResults.PopNextItem(); }

    static void SetUserAgentString();
    static void SetUserAgent(const std::string& sValue) { sUserAgent = ra::Widen(sValue); }
    static const std::wstring& GetUserAgent() { return sUserAgent; }

private:
    static HANDLE ms_hHTTPMutex;
    static HttpResults ms_LastHttpResults;

    static std::wstring sUserAgent;
};


#endif // !RA_HTTPTHREAD_H
