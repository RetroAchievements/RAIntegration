#pragma once

#include "RA_Defs.h"
#include <deque>
#include <map>
#include <string>
#include <vector>
#include <assert.h>

typedef void* HANDLE;
typedef void* LPVOID;

enum HTTPRequestType
{
	HTTPRequest_Post,
	HTTPRequest_Get,
	HTTPRequest_StopThread,

	HTTPRequest__Max
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
	RequestGameTitles,
	RequestUnlocks,
	RequestHashLibrary,
	RequestGamesList,
	RequestAllProgress,

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

extern const char* RequestTypeToString[];

typedef std::map<char, std::string> PostArgs;

extern std::string PostArgsToString( const PostArgs& args );

class RequestObject
{
public:
	RequestObject( RequestType nType, const PostArgs& PostArgs = PostArgs(), const std::string& sData = "" ) :
		m_nType( nType ), m_PostArgs( PostArgs ), m_sData( sData )
		{}

public:
	const RequestType GetRequestType() const						{ return m_nType; }
	const PostArgs& GetPostArgs() const								{ return m_PostArgs; }
	const std::string& GetData() const								{ return m_sData; }
	
	BOOL GetSuccess() const											{ return m_bSuccess; }
	DataStream& GetResponse()										{ return m_sResponse; }
	const DataStream& GetResponse() const							{ return m_sResponse; }

	void SetResult( BOOL bSuccess, const DataStream& sResponse )	{ m_bSuccess = bSuccess; m_sResponse = sResponse; }

	BOOL ParseResponseToJSON( Document& rDocOut );

private:
	const RequestType m_nType;
	const PostArgs m_PostArgs;
	const std::string m_sData;

	BOOL m_bSuccess;
	DataStream m_sResponse;
};

class HttpResults
{
public:
	//	Caller must manage: SAFE_DELETE when finished
	RequestObject* PopNextItem();
	const RequestObject* PeekNextItem() const;
	void PushItem( RequestObject* pObj );
	void Clear();
	size_t Count() const;
	BOOL PageRequestExists( RequestType nType, const std::string& sData ) const;

private:
	std::deque<RequestObject*> m_aRequests;
};

class RAWeb
{
public:
	static HANDLE g_hHTTPMutex;
	static HttpResults LastHttpResults;

	static void RA_InitializeHTTPThreads();
	static void RA_KillHTTPThreads();

	static void CreateThreadedHTTPRequest( RequestType nType, const PostArgs& PostData = PostArgs(), const std::string& sData = "" );
	static BOOL HTTPRequestExists( RequestType nType, const std::string& sData );
	
	static BOOL DoBlockingRequest( RequestType nType, const PostArgs& PostData, Document& JSONResponseOut );
	static BOOL DoBlockingRequest( RequestType nType, const PostArgs& PostData, DataStream& ResponseOut );

	//static BOOL DoBlockingHttpGet( const std::string& sRequestedPage, DataStream& ResponseOut );
	static BOOL DoBlockingHttpPost( const std::string& sRequestedPage, const std::string& sPostString, DataStream& ResponseOut );

	static BOOL DoBlockingImageUpload( UploadType nType, const std::string& sFilename, Document& ResponseOut );

	static DWORD __stdcall HTTPWorkerThread( LPVOID lpParameter );
};
