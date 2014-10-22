#pragma once

#include "RA_Defs.h"
#include <deque>
#include <map>
#include <string>
#include <assert.h>

typedef void* HANDLE;

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
	RequestNews,
	RequestPatch,
	RequestLatestClientPage,
	RequestRichPresence,
	RequestAchievementInfo,
	RequestLeaderboardInfo,
	RequestCodeNotes,
	RequestFriendList,
	RequestUserPic,
	RequestBadgeImage,

	//	Submit
	RequestPing,
	RequestPostActivity,
	RequestSubmitAwardAchievement,
	RequestSubmitCodeNote,
	RequestSubmitLeaderboardEntry,
	RequestSubmitAchievementData,
	
	NumRequestTypes
};

extern const char* RequestTypeToString[];

typedef std::map<char, std::string> PostArgs;

class RequestObject
{
public:
	RequestObject( RequestType nType, const PostArgs& PostArgs = PostArgs(), const std::string& sPage = "", const int nUserRef = 0 ) :
		m_nType( nType ), m_PostArgs( PostArgs ), m_sPageURL( sPage ), m_nUserRef( nUserRef )
		{}

public:
	const RequestType GetRequestType() const	{ return m_nType; }
	const PostArgs& GetPostArgs() const			{ return m_PostArgs; }
	const std::string& GetPageURL() const		{ return m_sPageURL; }
	const int GetUserRef() const				{ return m_nUserRef; }
	
	BOOL GetSuccess() const						{ return m_bSuccess; }
	const std::string& GetResponse() const		{ return m_sResponse; }
	size_t GetNumBytesRead() const				{ return m_nBytesRead; }

private:
	const RequestType m_nType;
	const PostArgs m_PostArgs;
	const std::string m_sPageURL;
	const int m_nUserRef;

	BOOL m_bSuccess;
	std::string m_sResponse;
	size_t m_nBytesRead;
};

class HttpResults
{
public:
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

	static BOOL DoBlockingHttpGet( const char* sRequestedPage, char* pBufferOut, DWORD& nBytesRead );
	static BOOL DoBlockingHttpPost( const char* sRequestedPage, const char* sPostString, char* pBufferOut, const unsigned nBufferOutSize, DWORD& nBytesRead );
	static BOOL DoBlockingImageUpload( const char* sRequestedPage, const char* sFilename, char* pBufferOut, DWORD& nBytesRead );

	static DWORD HTTPWorkerThread( LPVOID lpParameter );
};
