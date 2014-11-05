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
	RequestUserPic,
	RequestBadge,
	RequestBadgeIter,
	RequestGameTitles,
	RequestUnlocks,

	//	Submit
	RequestPing,
	RequestPostActivity,
	RequestSubmitAwardAchievement,
	RequestSubmitCodeNote,
	RequestSubmitLeaderboardEntry,
	RequestSubmitAchievementData,
	RequestSubmitTicket,
	RequestSubmitNewTitle,
	
	
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

std::string PostArgsToString( const PostArgs& args )
{
	std::string str = "";
	PostArgs::const_iterator iter = args.begin();
	while( iter != args.end() )
	{
		if( iter == args.begin() )
			str += "?";
		else
			str += "&";

		str += (*iter).first;
		str += "=";
		str += (*iter).second;

		iter++;
	}
	return str;
}

class RequestObject
{
public:
	RequestObject( RequestType nType, const PostArgs& PostArgs = PostArgs(), const std::string& sPage = "", const int nUserRef = 0 ) :
		m_nType( nType ), m_PostArgs( PostArgs ), m_sPageURL( sPage ), m_nUserRef( nUserRef )
		{}

public:
	const RequestType GetRequestType() const		{ return m_nType; }
	const PostArgs& GetPostArgs() const				{ return m_PostArgs; }
	const std::string& GetPageURL() const			{ return m_sPageURL; }
	const int GetUserRef() const					{ return m_nUserRef; }
	
	BOOL GetSuccess() const							{ return m_bSuccess; }
	DataStream& GetResponse()						{ return m_sResponse; }

	void SetResult( BOOL bSuccess, const DataStream& sResponse );

	BOOL ParseResponseToJSON( Document& rDocOut );

private:
	const RequestType m_nType;
	const PostArgs m_PostArgs;
	const std::string m_sPageURL;
	const int m_nUserRef;

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
	BOOL PageRequestExists( const char* sPageName ) const;

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

	static void CreateThreadedHTTPRequest( RequestType nType, const PostArgs& PostData = PostArgs(), const std::string& sCustomPageURL = "", int nUserRef = 0 );
	static BOOL HTTPRequestExists( const char* sRequestPageName );
	
	static BOOL DoBlockingRequest( RequestType nType, const PostArgs& PostData, Document& JSONResponseOut );
	static BOOL DoBlockingRequest( RequestType nType, const PostArgs& PostData, DataStream& ResponseOut );

	//static BOOL DoBlockingHttpGet( const std::string& sRequestedPage, DataStream& ResponseOut );
	static BOOL DoBlockingHttpPost( const std::string& sRequestedPage, const std::string& sPostString, DataStream& ResponseOut );

	static BOOL DoBlockingImageUpload( UploadType nType, const std::string& sFilename, Document& ResponseOut );
	//static BOOL DoBlockingImageUpload( UploadType nType, const std::string& sFilename, DataStream& ResponseOut );

	static DWORD __stdcall HTTPWorkerThread( LPVOID lpParameter );
};
