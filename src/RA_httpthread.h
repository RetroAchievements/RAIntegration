#ifndef RA_HTTPTHREAD_H
#define RA_HTTPTHREAD_H
#pragma once

#include "RA_Defs.h"

typedef void* HANDLE;
typedef void* LPVOID;

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

    //	Special:
    StopThread,

    NumRequestTypes
};

enum UploadType
{
    //	Upload:
    RequestUploadBadgeImage,

    NumUploadTypes
};

const char* RequestTypeToString[];

using PostArgs = std::map<char, std::string>;

extern std::string PostArgsToString(const PostArgs& args);

class RequestObject
{
public:
#pragma warning(push)
#pragma warning(disable : 4514) // unused inline functions, weird
    RequestObject() noexcept {}; // default will throw, but default constructors aren't allowed to throw
    RequestObject(_In_ RequestType nType, _In_ const PostArgs& PostArgs = PostArgs{},
        _In_ const std::string& sData ={}) noexcept :
        m_nType{ nType }, m_PostArgs{ PostArgs }, m_sData{ sData }, m_sResponse{}
    {
    }

    // default will throw
    RequestObject(RequestObject&& b) noexcept :
        m_nType{ std::move_if_noexcept(b.m_nType) },
        m_PostArgs{ std::move_if_noexcept(b.m_PostArgs) },
        m_sData{ std::move_if_noexcept(b.m_sData) },
        m_sResponse{ std::move_if_noexcept(b.m_sResponse) }
    {
        if (b.m_nType != RequestType{})
            b.m_nType = RequestType{};
    }
#pragma warning(pop)
    ~RequestObject() noexcept = default;
    
    RequestObject(const RequestObject&) = delete;
    RequestObject& operator=(const RequestObject&) = delete;
    RequestObject& operator=(RequestObject&&) noexcept = default;
public:
#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions, weird did see them unreferenced
    const RequestType GetRequestType() const { return m_nType; }
    const PostArgs& GetPostArgs() const { return m_PostArgs; }
    const std::string& GetData() const { return m_sData; }

    DataStream& GetResponse() { return m_sResponse; }
    const DataStream& GetResponse() const { return m_sResponse; }
    void SetResponse(const DataStream& sResponse) { m_sResponse = sResponse; }
#pragma warning(pop)

    BOOL ParseResponseToJSON(Document& rDocOut);

private:
    // these can't be const
    RequestType m_nType ={};
    PostArgs m_PostArgs;
    std::string m_sData;

    DataStream m_sResponse;
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
    static void RA_InitializeHTTPThreads();
    static void RA_KillHTTPThreads();

    static void LogJSON(const Document& doc);

    static void CreateThreadedHTTPRequest(RequestType nType, const PostArgs& PostData = PostArgs(), const std::string& sData = "");
    static BOOL HTTPRequestExists(RequestType nType, const std::string& sData);
    static BOOL HTTPResponseExists(RequestType nType, const std::string& sData);

    static BOOL DoBlockingRequest(RequestType nType, const PostArgs& PostData, Document& JSONResponseOut);
    static BOOL DoBlockingRequest(RequestType nType, const PostArgs& PostData, DataStream& ResponseOut);

    static BOOL DoBlockingHttpGet(const std::string& sRequestedPage, DataStream& ResponseOut, bool bIsImageRequest);
    static BOOL DoBlockingHttpPost(const std::string& sRequestedPage, const std::string& sPostString, DataStream& ResponseOut);

    static BOOL DoBlockingImageUpload(UploadType nType, const std::string& sFilename, Document& ResponseOut);

    static DWORD WINAPI HTTPWorkerThread(LPVOID lpParameter);
#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
    static HANDLE Mutex() { return ms_hHTTPMutex; }
    static RequestObject* PopNextHttpResult() { return ms_LastHttpResults.PopNextItem(); }
#pragma warning(pop)
private:
    static HANDLE ms_hHTTPMutex;
    static HttpResults ms_LastHttpResults;
};


#endif // !RA_HTTPTHREAD_H
