#include "RA_httpthread.h"

#include <vector>
#include <Windows.h>	//	GetFileAttributes
#include <winhttp.h>
#include <fstream>
#include <sstream>
#include <time.h>

#include "RA_Defs.h"
#include "RA_Core.h"
#include "RA_User.h"
#include "RA_Achievement.h"
#include "RA_Dlg_Memory.h"
#include "RA_RichPresence.h"


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
	"RequestFriendList"
	"RequestBadgeIter",
	"RequestGameTitles",
	"RequestUnlocks",
	"RequestHashLibrary",
	"RequestGamesList",
	"RequestAllProgress",

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

	"STOP_THREAD",
	"",
};
static_assert( SIZEOF_ARRAY( RequestTypeToString ) == NumRequestTypes, "Must match up!" );

const char* RequestTypeToPost[] =
{
	"login",
	"score",
	"news",
	"patch",
	"",						//	TBD RequestLatestClientPage
	"richpresencepatch",
	"achievementwondata",
	"lbinfo",
	"codenotes2",
	"getfriendlist",
	"badgeiter",
	"gametitles",
	"unlocks",
	"hashlibrary",
	"gameslist",
	"allprogress",

	"",						//	TBD RequestPing (ping.php)
	"postactivity",
	"awardachievement",
	"submitcodenote",
	"submitlbentry",
	"uploadachievement",
	"submitticket",
	"submittitle",
	
	"",						//	TBD RequestUserPic
	"",						//	TBD RequestBadge

	"",						//	STOP_THREAD
};
static_assert( SIZEOF_ARRAY( RequestTypeToPost ) == NumRequestTypes, "Must match up!" );

const char* UploadTypeToString[] = 
{
	"RequestUploadBadgeImage",
};
static_assert( SIZEOF_ARRAY( UploadTypeToString ) == NumUploadTypes, "Must match up!" );

const char* UploadTypeToPost[] =
{
	"uploadbadgeimage",
};
static_assert( SIZEOF_ARRAY( UploadTypeToPost ) == NumUploadTypes, "Must match up!" );

//	No game-specific code here please!

std::vector<HANDLE> g_vhHTTPThread;
HttpResults HttpRequestQueue;

HANDLE RAWeb::g_hHTTPMutex = NULL;
HttpResults RAWeb::LastHttpResults;


BOOL RequestObject::ParseResponseToJSON( Document& rDocOut )
{
	rDocOut.ParseInsitu( DataStreamAsString( GetResponse() ) );

	if( rDocOut.HasParseError() )
	{
		RA_LOG( "Parse issue on response, %s (%s)\n", rDocOut.GetParseError(), RequestTypeToString[m_nType] );
	}

	return !rDocOut.HasParseError();
}

//BOOL RAWeb::DoBlockingHttpGet( const std::string& sRequestedPage, DataStream& ResponseOut )
//{
//	RA_LOG( __FUNCTION__ ": (%08x) GET from %s...\n", GetCurrentThreadId(), sRequestedPage );
//
//	BOOL bResults = FALSE, bSuccess = FALSE;
//	HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
//
//	WCHAR wBuffer[1024];
//	size_t nTemp;
//	//BYTE* sDataDestOffset = &pBufferOut[0];
//	DWORD nBytesToRead = 0;
//	DWORD nBytesFetched = 0;
//
//	char sClientName[1024];
//	sprintf_s( sClientName, 1024, "Retro Achievements Client %s %s", g_sClientName, g_sClientVersion );
//	WCHAR wClientNameBuffer[1024];
//	mbstowcs_s( &nTemp, wClientNameBuffer, 1024, sClientName, strlen( sClientName )+1 );
//
//	// Use WinHttpOpen to obtain a session handle.
//	hSession = WinHttpOpen( wClientNameBuffer, 
//		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
//		WINHTTP_NO_PROXY_NAME, 
//		WINHTTP_NO_PROXY_BYPASS, 0);
//
//	// Specify an HTTP server.
//	if( hSession != NULL )
//	{
//		if( strncmp( sRequestedPage, "Badge", 5 ) == 0 )
//			hConnect = WinHttpConnect( hSession, RA_IMG_HOST_W, INTERNET_DEFAULT_HTTP_PORT, 0 );
//		else
//			hConnect = WinHttpConnect( hSession, RA_HOST_W, INTERNET_DEFAULT_HTTP_PORT, 0 );
//
//		// Create an HTTP Request handle.
//		if( hConnect != NULL )
//		{
//			mbstowcs_s( &nTemp, wBuffer, 1024, sRequestedPage, strlen(sRequestedPage)+1 );
//
//			hRequest = WinHttpOpenRequest( hConnect, 
//				L"GET", 
//				wBuffer, 
//				NULL, 
//				WINHTTP_NO_REFERER, 
//				WINHTTP_DEFAULT_ACCEPT_TYPES,
//				0);
//
//			// Send a Request.
//			if( hRequest != NULL )
//			{
//				bResults = WinHttpSendRequest( hRequest, 
//					L"Content-Type: application/x-www-form-urlencoded",
//					0, 
//					WINHTTP_NO_REQUEST_DATA,
//					0, 
//					0,
//					0 );
//
//				if( WinHttpReceiveResponse( hRequest, NULL ) )
//				{
//					nBytesToRead = 0;
//					nBytesRead = 0;
//					WinHttpQueryDataAvailable( hRequest, &nBytesToRead );
//
//					std::stringstream sstr;
//
//					while( nBytesToRead > 0 )
//					{
//						char sHttpReadData[8192];
//						ZeroMemory( sHttpReadData, 8192 );
//
//						assert( nBytesToRead <= 8192 );
//						if( nBytesToRead <= 8192 )
//						{
//							nBytesFetched = 0;
//							if( WinHttpReadData( hRequest, &sHttpReadData, nBytesToRead, &nBytesFetched ) )
//							{
//								assert( nBytesToRead == nBytesFetched );
//
//								//Read: parse buffer
//								//memcpy( sDataDestOffset, sHttpReadData, nBytesFetched );
//								//sDataDestOffset += nBytesFetched;
//
//								sstr.write( sHttpReadData, nBytesFetched );
//								nBytesRead += nBytesFetched;
//							}
//						}
//
//						bSuccess = TRUE;
//
//						WinHttpQueryDataAvailable( hRequest, &nBytesToRead );
//					}
//
//					RA_LOG( __FUNCTION__ ": (%08x) success! Read %d bytes...\n", GetCurrentThreadId(), nBytesRead );
//				}
//			}
//		}
//	}
//
//	// Close open handles.
//	if( hRequest != NULL )
//		WinHttpCloseHandle( hRequest );
//	if( hConnect != NULL )
//		WinHttpCloseHandle( hConnect );
//	if( hSession != NULL )
//		WinHttpCloseHandle( hSession );
//
//	return bSuccess;
//}

BOOL RAWeb::DoBlockingRequest( RequestType nType, const PostArgs& PostData, Document& JSONResponseOut )
{
	DataStream response;
	if( DoBlockingRequest( nType, PostData, response ) )
	{
		JSONResponseOut.ParseInsitu( DataStreamAsString( response ) );
		return( !JSONResponseOut.HasParseError() );
	}
	else
	{
		return FALSE;
	}
}

BOOL RAWeb::DoBlockingRequest( RequestType nType, const PostArgs& PostData, DataStream& ResponseOut )
{
	PostArgs args = PostData;	//	Take a copy
	args['r'] = RequestTypeToPost[nType];
	return DoBlockingHttpPost( "dorequest.php", PostArgsToString( args ), ResponseOut );
}

BOOL RAWeb::DoBlockingHttpPost( const std::string& sRequestedPage, const std::string& sPostString, DataStream& ResponseOut )
{
	if( sPostString.find( "r=login" ) != std::string::npos )
	{
		//	Special case: DO NOT LOG raw user credentials!
		RA_LOG( __FUNCTION__ ": (%04x) POST to %s...\n", GetCurrentThreadId(), sRequestedPage.c_str() );
	}
	else
	{
		RA_LOG( __FUNCTION__ ": (%04x) POST to %s%s...\n", GetCurrentThreadId(), sRequestedPage.c_str(), sPostString.c_str() );
	}
	
	ResponseOut.clear();

	BOOL bResults = FALSE, bSuccess = FALSE;
	HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;

	WCHAR wBuffer[1024];
	size_t nTemp;
	DWORD nBytesToRead = 0;
	DWORD nBytesFetched = 0;
	int nRemainingBuffer = 0;

	char sClientName[1024];
	sprintf_s( sClientName, 1024, "Retro Achievements Client %s %s", g_sClientName, g_sClientVersion );
	WCHAR wClientNameBuffer[1024];
	mbstowcs_s( &nTemp, wClientNameBuffer, 1024, sClientName, strlen( sClientName )+1 );

 	// Use WinHttpOpen to obtain a session handle.
 	hSession = WinHttpOpen( wClientNameBuffer, 
 		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
 		WINHTTP_NO_PROXY_NAME, 
 		WINHTTP_NO_PROXY_BYPASS, 0);
 
 	// Specify an HTTP server.
	if( hSession != NULL )
	{
 		hConnect = WinHttpConnect( hSession, RA_HOST_W, INTERNET_DEFAULT_HTTP_PORT, 0 );
 
 		// Create an HTTP Request handle.
 		if( hConnect != NULL )
		{
			mbstowcs_s( &nTemp, wBuffer, 1024, sRequestedPage.c_str(), strlen( sRequestedPage.c_str() )+1 );

 			hRequest = WinHttpOpenRequest( hConnect, 
 				L"POST", 
 				wBuffer, 
 				NULL, 
 				WINHTTP_NO_REFERER, 
 				WINHTTP_DEFAULT_ACCEPT_TYPES,
 				0);
 
 			// Send a Request.
 			if( hRequest != NULL )
 			{
 				bResults = WinHttpSendRequest( hRequest, 
 					L"Content-Type: application/x-www-form-urlencoded",
 					0, 
 					(LPVOID)sPostString.c_str(), //WINHTTP_NO_REQUEST_DATA,
 					strlen( sPostString.c_str() ), 
 					strlen( sPostString.c_str() ),
 					0);

				if( WinHttpReceiveResponse( hRequest, NULL ) )
				{
					//BYTE* sDataDestOffset = &pBufferOut[0];

					nBytesToRead = 0;
					WinHttpQueryDataAvailable( hRequest, &nBytesToRead );

					//	Note: success is much earlier, as 0 bytes read is VALID
					//	i.e. fetch achievements for new game will return 0 bytes.
					bSuccess = TRUE;

					while( nBytesToRead > 0 )
					{
						//ZeroMemory( sHttpReadData, 8192 );
						DataStream sHttpReadData;
						sHttpReadData.reserve( 8192 );

						assert( nBytesToRead <= 8192 );
						if( nBytesToRead <= 8192 )
						{
							nBytesFetched = 0;
							if( WinHttpReadData( hRequest, sHttpReadData.data(), nBytesToRead, &nBytesFetched ) )
							{
								assert( nBytesToRead == nBytesFetched );
								ResponseOut.insert( ResponseOut.end(), sHttpReadData.begin(), sHttpReadData.end() );
							}
						}

						WinHttpQueryDataAvailable( hRequest, &nBytesToRead );
					}

					RA_LOG( "DoBlockingHttpPost: POST to %s Success: %d bytes read\n", sRequestedPage, ResponseOut.size() );

				}
 			}
 		}
	}
 
	// Close open handles.
	if( hRequest != NULL )
		WinHttpCloseHandle( hRequest );
	if( hConnect != NULL )
		WinHttpCloseHandle( hConnect );
	if( hSession != NULL )
		WinHttpCloseHandle( hSession );

	return bSuccess;
}

BOOL DoBlockingImageUpload( UploadType nType, const std::string& sFilename, DataStream& ResponseOut )
{
	const std::string sRequestedPage = "doupload.php";
	const std::string sRTarget = RequestTypeToPost[nType]; //"uploadbadgeimage";

	RA_LOG( __FUNCTION__ ": (%04x) uploading \"%s\" to %s...\n", GetCurrentThreadId(), sFilename.c_str(), sRequestedPage.c_str() );

	BOOL bSuccess = FALSE;
	HINTERNET hConnect = NULL, hRequest = NULL;

	char sClientName[1024];
	sprintf_s( sClientName, 1024, "Retro Achievements Client %s %s", g_sClientName, g_sClientVersion );

	size_t nTemp;
	WCHAR wClientNameBuffer[1024];
	mbstowcs_s( &nTemp, wClientNameBuffer, 1024, sClientName, strlen(sClientName)+1 );

	// Use WinHttpOpen to obtain a session handle.
	HINTERNET hSession = WinHttpOpen( wClientNameBuffer, 
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, 
		WINHTTP_NO_PROXY_BYPASS, 0);

	// Specify an HTTP server.
	if( hSession != NULL )
		hConnect = WinHttpConnect( hSession, RA_HOST_W, INTERNET_DEFAULT_HTTP_PORT, 0 );

	if( hConnect != NULL )
	{
		WCHAR wBuffer[1024];
		mbstowcs_s( &nTemp, wBuffer, 1024, sRequestedPage.c_str(), strlen( sRequestedPage.c_str() )+1 );

		hRequest = WinHttpOpenRequest( hConnect, 
			L"POST", 
			wBuffer, 
			NULL, 
			WINHTTP_NO_REFERER, 
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			0);
	}

	if( hRequest != NULL )
	{
		//---------------------------41184676334
		const char* mimeBoundary = "---------------------------41184676334";
		const wchar_t* contentType = L"Content-Type: multipart/form-data; boundary=---------------------------41184676334\r\n";
		
		int nResult = WinHttpAddRequestHeaders( hRequest, contentType, (unsigned long)-1, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW );
		if( nResult != 0 )
		{
			// Add the photo to the stream
			std::ifstream f( sFilename, std::ios::binary );

			std::ostringstream sb_ascii;
			//sb_ascii << str;									
			sb_ascii << "--" << mimeBoundary << "\r\n";															//	--Boundary
			sb_ascii << "Content-Disposition: form-data; name=\"file\"; filename=\"" << sFilename << "\"\r\n";	//	Item header    'file'
			sb_ascii << "\r\n";																					//	Spacing
			sb_ascii << f.rdbuf();																				//	Binary content
			sb_ascii << "\r\n";																					//	Spacing
			sb_ascii << "--" << mimeBoundary << "--\r\n";														//	--Boundary--

			//	## EXPERIMENTAL ##
			sb_ascii << "Content-Disposition: form-data; name=\"r\"\r\n";										//	Item header    'r'
			sb_ascii << "\r\n";																					//	Spacing
			sb_ascii << sRTarget << "\r\n";																		//	Binary content
			sb_ascii << "\r\n";																					//	Spacing
			sb_ascii << "--" << mimeBoundary << "--\r\n";														//	--Boundary--

			const std::string str = sb_ascii.str();

			bSuccess = WinHttpSendRequest(
				hRequest,
				WINHTTP_NO_ADDITIONAL_HEADERS,
				0,
				(void*)str.c_str(),
				static_cast<unsigned long>( str.length() ),
				static_cast<unsigned long>( str.length() ),
				0);
		}

		if( WinHttpReceiveResponse( hRequest, NULL ) )
		{
			//BYTE* sDataDestOffset = &pBufferOut[0];

			DWORD nBytesToRead = 0;
			WinHttpQueryDataAvailable( hRequest, &nBytesToRead );

			//	Note: success is much earlier, as 0 bytes read is VALID
			//	i.e. fetch achievements for new game will return 0 bytes.
			bSuccess = TRUE;

			while( nBytesToRead > 0 )
			{
				DataStream sHttpReadData;
				sHttpReadData.reserve( 8192 );

				assert( nBytesToRead <= 8192 );
				if( nBytesToRead <= 8192 )
				{
					DWORD nBytesFetched = 0;
					if( WinHttpReadData( hRequest, sHttpReadData.data(), nBytesToRead, &nBytesFetched ) )
					{
						assert( nBytesToRead == nBytesFetched );
						ResponseOut.insert( ResponseOut.end(), sHttpReadData.begin(), sHttpReadData.end() );
					}
				}

				WinHttpQueryDataAvailable( hRequest, &nBytesToRead );
			}

			RA_LOG( __FUNCTION__ ": success! Returned %d bytes.", ResponseOut.size() );
		}
	}

	return bSuccess;
}

BOOL RAWeb::DoBlockingImageUpload( UploadType nType, const std::string& sFilename, Document& ResponseOut )
{
	DataStream response;
	if( ::DoBlockingImageUpload( nType, sFilename, response ) )
	{
		ResponseOut.ParseInsitu( DataStreamAsString( response ) );
		if( !ResponseOut.HasParseError() )
		{
			return TRUE;
		}
		else
		{
			RA_LOG( __FUNCTION__ " (%d, %s) has parse error: %s\n", nType, sFilename.c_str(), ResponseOut.GetParseError() );
			return FALSE;
		}
	}
	else
	{
		RA_LOG( __FUNCTION__ " (%d, %s) could not connect?\n", nType, sFilename.c_str() );
		return FALSE;
	}
}
//
//BOOL RAWeb::HTTPRequestExists( const char* sRequestPageName )
//{
//	return HttpRequestQueue.PageRequestExists( sRequestPageName );
//}

BOOL RAWeb::HTTPRequestExists( RequestType nType, const std::string& sData )
{
	return HttpRequestQueue.PageRequestExists( nType, sData );
}

//	Adds items to the httprequest queue
void RAWeb::CreateThreadedHTTPRequest( RequestType nType, const PostArgs& PostData, const std::string& sData )
{
	HttpRequestQueue.PushItem( new RequestObject( nType, PostData, sData ) );
	RA_LOG( __FUNCTION__ " added '%s', ('%s'), queue (%d)\n", RequestTypeToString[nType], sData.c_str(), HttpRequestQueue.Count() );
}

//////////////////////////////////////////////////////////////////////////

void RAWeb::RA_InitializeHTTPThreads()
{
	RA_LOG( __FUNCTION__ " called\n" );

	DWORD dwThread;
	RAWeb::g_hHTTPMutex = CreateMutex( NULL, FALSE, NULL );

	for( size_t i = 0; i < g_nNumHTTPThreads; ++i )
	{
		HANDLE hThread = CreateThread( NULL, 0, RAWeb::HTTPWorkerThread, (void*)i, 0 , &dwThread );
		assert( hThread != NULL );
		if( hThread != NULL )
		{
			g_vhHTTPThread.push_back( hThread );
			RA_LOG( __FUNCTION__ " Adding HTTP thread %d\n", i );
		}
	}
}

//	Takes items from the http request queue, and posts them to the last http results queue.
DWORD RAWeb::HTTPWorkerThread( LPVOID lpParameter )
{
	time_t nSendNextKeepAliveAt = time( NULL ) + SERVER_PING_DURATION;

	BOOL bThreadActive = true;
	BOOL bDoPingKeepAlive = ( (int)lpParameter ) == 0;

	while( bThreadActive )
	{
		RequestObject* pObj = HttpRequestQueue.PopNextItem();
		if( pObj != NULL )
		{
			BOOL bSuccess = FALSE;
			DataStream Response;

			switch( pObj->GetRequestType() )
			{
			case RequestLogin:
					//	TBD
				break;

			case StopThread:
				bThreadActive = FALSE;
				bDoPingKeepAlive = FALSE;
				break;

			default:
				DoBlockingRequest( pObj->GetRequestType(), pObj->GetPostArgs(), Response );
				break;
			}

			pObj->SetResult( bSuccess, Response );

			if( bThreadActive )
			{
				//	Push object over to results queue - let app deal with them now.
				RAWeb::LastHttpResults.PushItem( pObj );
			}
			else
			{
				//	Take ownership and delete(): we caused the 'pop' earlier, so we have responsibility to
				//	 either pass to LastHttpResults, or deal with it here.
				SAFE_DELETE( pObj );
			}
		}

		if( bDoPingKeepAlive )
		{
			//	Post a pingback once every few minutes to keep the server aware of our presence
			if( time( NULL ) > nSendNextKeepAliveAt )
			{
				nSendNextKeepAliveAt += SERVER_PING_DURATION;

				//	Post a keepalive packet:
				if( RAUsers::LocalUser.IsLoggedIn() )
				{
					PostArgs args;
					args['u'] = RAUsers::LocalUser.Username();

					if( RA_GameIsActive() )
					{
						if( g_MemoryDialog.IsActive() )
						{
							args['m'] = "Developing Achievements";
						}
						else
						{
							const std::string& sRPResponse = g_RichPresenceInterpretter.GetRichPresenceString();
							if( sRPResponse.size() == 0 )
								args['m'] = "Earning Achievements";
							else
								args['m'] = sRPResponse;
						}
					}
					
					RAWeb::CreateThreadedHTTPRequest( RequestPing, args );
				}
			}
		}

		if( HttpRequestQueue.Count() > 0 )
			RA_LOG( __FUNCTION__ " (%08x) request queue is at %d\n", GetCurrentThreadId(), HttpRequestQueue.Count() );

		Sleep( 100 );
	}

	//	Delete and empty queue - allocated data is within!
	RAWeb::LastHttpResults.Clear();

	return 0;
}

void RAWeb::RA_KillHTTPThreads()
{
	RA_LOG( __FUNCTION__ " called\n" );

	for( size_t i = 0; i < g_vhHTTPThread.size(); ++i )
	{
		//	Create n of these:
		RAWeb::CreateThreadedHTTPRequest( RequestType::StopThread );
	}
	
	for( size_t i = 0; i < g_vhHTTPThread.size(); ++i )
	{
		//	Wait for n responses:
		DWORD nResult = WaitForSingleObject( g_vhHTTPThread[i], INFINITE );
		RA_LOG( __FUNCTION__ " ended, result %d\n", nResult );
	}
}

//////////////////////////////////////////////////////////////////////////

RequestObject* HttpResults::PopNextItem()
{
	RequestObject* pRetVal = NULL;
	WaitForSingleObject( RAWeb::g_hHTTPMutex, INFINITE );
	{
		if( m_aRequests.size() > 0 )
		{
			pRetVal = m_aRequests.front();
			m_aRequests.pop_front();
		}
	}
	ReleaseMutex( RAWeb::g_hHTTPMutex );

	return pRetVal;
}

const RequestObject* HttpResults::PeekNextItem() const
{
	RequestObject* pRetVal = NULL;
	WaitForSingleObject( RAWeb::g_hHTTPMutex, INFINITE );
	{
		pRetVal = m_aRequests.front();
	}
	ReleaseMutex( RAWeb::g_hHTTPMutex );
		
	return pRetVal; 
}

void HttpResults::PushItem( RequestObject* pObj )
{ 
	WaitForSingleObject( RAWeb::g_hHTTPMutex, INFINITE );
	{
		m_aRequests.push_front( pObj ); 
	}
	ReleaseMutex( RAWeb::g_hHTTPMutex );
}

void HttpResults::Clear()
{ 
	WaitForSingleObject( RAWeb::g_hHTTPMutex, INFINITE );
	{
		while( !m_aRequests.empty() )
		{
			RequestObject* pObj = m_aRequests.front();
			m_aRequests.pop_front();
			SAFE_DELETE( pObj );
		}
	}
	ReleaseMutex( RAWeb::g_hHTTPMutex );
}

size_t HttpResults::Count() const
{ 
	size_t nCount = 0;
	WaitForSingleObject( RAWeb::g_hHTTPMutex, INFINITE );
	{
		nCount = m_aRequests.size();
	}
	ReleaseMutex( RAWeb::g_hHTTPMutex );

	return nCount;
}

BOOL HttpResults::PageRequestExists( RequestType nType, const std::string& sData ) const
{
	BOOL bRetVal = FALSE;
	WaitForSingleObject( RAWeb::g_hHTTPMutex, INFINITE );
	{
		std::deque<RequestObject*>::const_iterator iter = m_aRequests.begin();
		while( iter != m_aRequests.end() )
		{
			const RequestObject* pObj = (*iter);
			if( pObj->GetRequestType() == nType &&
				pObj->GetData().compare( sData ) == 0 )
			{
				bRetVal = TRUE;
				break;
			}

			iter++;
		}
	}
	ReleaseMutex( RAWeb::g_hHTTPMutex );

	return bRetVal;
}

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
