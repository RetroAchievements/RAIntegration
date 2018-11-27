#ifndef RA_HTTPTHREAD_H
#define RA_HTTPTHREAD_H
#pragma once

#include "ra_fwd.h"
#include "RA_StringUtils.h"

enum class HTTPRequestMethod
{
    Post,
    Get,
    _TerminateThread
};

enum class RequestType : std::size_t
{
    // Fetch
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

    // Submit
    SubmitAwardAchievement,
    SubmitCodeNote,
    SubmitLeaderboardEntry,
    SubmitAchievementData,
    SubmitTicket,
    SubmitNewTitle,
};

enum class UploadType
{
    // Upload:
    BadgeImage
};

inline constexpr std::array<const char*, 21> RequestTypeToString
{
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

    "RequestSubmitAwardAchievement",
    "RequestSubmitCodeNote",
    "RequestSubmitLeaderboardEntry",
    "RequestSubmitAchievementData",
    "RequestSubmitTicket",
    "RequestSubmitNewTitleEntry",
};

using PostArgs = std::map<char, std::string>;

extern std::string PostArgsToString(const PostArgs& args);

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

private:
    std::deque<RequestObject*> m_aRequests;
};

class RAWeb
{
public:
    static void CreateThreadedHTTPRequest(RequestType nType, const PostArgs& PostData = PostArgs(), const std::string& sData = "");

    static BOOL DoBlockingRequest(RequestType nType, const PostArgs& PostData, rapidjson::Document& JSONResponseOut);
    static BOOL DoBlockingRequest(RequestType nType, const PostArgs& PostData, std::string& ResponseOut);

    static BOOL DoBlockingImageUpload(UploadType nType, const std::string& sFilename, rapidjson::Document& ResponseOut);

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
