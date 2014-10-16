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

//	No game-specific code here please!

HANDLE g_hHTTPMutex;
std::vector<HANDLE> g_vhHTTPThread;
HttpResults HttpRequestQueue;
HttpResults LastHttpResults;

BOOL DirectoryExists( const char* sPath )
{
	DWORD dwAttrib = GetFileAttributes( sPath );

	return( dwAttrib != INVALID_FILE_ATTRIBUTES && ( dwAttrib & FILE_ATTRIBUTE_DIRECTORY ) );
}

BOOL DoBlockingHttpGet( const char* sRequestedPage, char* pBufferOut, const unsigned int nBufferOutSize, DWORD* pBytesRead )
{
	RA_LOG( __FUNCTION__ ": (%08x) GET from %s...\n", GetCurrentThreadId(), sRequestedPage );

	BOOL bResults = FALSE, bSuccess = FALSE;
	HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;

	WCHAR wBuffer[1024];
	size_t nTemp;
	char* sDataDestOffset = &pBufferOut[0];
	DWORD nBytesToRead = 0;
	DWORD nBytesFetched = 0;

	char sClientName[1024];
	sprintf_s( sClientName, 1024, "Retro Achievements Client %s %s", g_sClientName, g_sClientVersion );
	WCHAR wClientNameBuffer[1024];
	mbstowcs_s( &nTemp, wClientNameBuffer, 1024, sClientName, strlen(sClientName)+1 );

	// Use WinHttpOpen to obtain a session handle.
	hSession = WinHttpOpen( wClientNameBuffer, 
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, 
		WINHTTP_NO_PROXY_BYPASS, 0);

	// Specify an HTTP server.
	if( hSession != NULL )
	{
		if( strncmp( sRequestedPage, "Badge", 5 ) == 0 )
			hConnect = WinHttpConnect( hSession, RA_IMG_HOST_W, INTERNET_DEFAULT_HTTP_PORT, 0);
		else
			hConnect = WinHttpConnect( hSession, RA_HOST_W, INTERNET_DEFAULT_HTTP_PORT, 0);

		// Create an HTTP Request handle.
		if( hConnect != NULL )
		{
			mbstowcs_s( &nTemp, wBuffer, 1024, sRequestedPage, strlen(sRequestedPage)+1 );

			hRequest = WinHttpOpenRequest( hConnect, 
				L"GET", 
				wBuffer, 
				NULL, 
				WINHTTP_NO_REFERER, 
				WINHTTP_DEFAULT_ACCEPT_TYPES,
				0);

			// Send a Request.
			if( hRequest != NULL )
			{
				bResults = WinHttpSendRequest( hRequest, 
					L"Content-Type: application/x-www-form-urlencoded",	//	WOOHOO!
					//L"Content-Type: text/plain",	//<-- NOPE!!
					0, 
					WINHTTP_NO_REQUEST_DATA,
					0, 
					0,
					0);

				if( WinHttpReceiveResponse( hRequest, NULL ) )
				{
					nBytesToRead = 0;
					(*pBytesRead) = 0;
					WinHttpQueryDataAvailable( hRequest, &nBytesToRead );

					while( nBytesToRead > 0 )
					{
						char sHttpReadData[8192];
						ZeroMemory( sHttpReadData, 8192 );

						assert( nBytesToRead <= 8192 );
						if( nBytesToRead <= 8192 )
						{
							nBytesFetched = 0;
							if( WinHttpReadData( hRequest, &sHttpReadData, nBytesToRead, &nBytesFetched ) )
							{
								assert( nBytesToRead == nBytesFetched );

								//Read: parse buffer
								memcpy( sDataDestOffset, sHttpReadData, nBytesFetched );

								sDataDestOffset += nBytesFetched;
								(*pBytesRead) += nBytesFetched;
							}
						}

						bSuccess = TRUE;

						WinHttpQueryDataAvailable( hRequest, &nBytesToRead );
					}

					RA_LOG( __FUNCTION__ ": (%08x) success! Read %d bytes...\n", GetCurrentThreadId(), (*pBytesRead) );
				}
			}
		}
	}


	// Close open handles.
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);

	return bSuccess;
}

BOOL DoBlockingImageUpload( const char* sRequestedPage, const char* sFilename, char* pBufferOut, const unsigned int nBufferOutSize, DWORD* pCharsRead )
{
	RA_LOG( __FUNCTION__ ": (%08x) uploading \"%s\" to %s...\n", GetCurrentThreadId(), sFilename, sRequestedPage );

	BOOL bSuccess = FALSE;
	BOOL  bResults = FALSE;
	HINTERNET hSession = NULL,
		hConnect = NULL,
		hRequest = NULL;

	WCHAR wBuffer[1024];
	size_t nTemp;

	char sClientName[1024];
	sprintf_s( sClientName, 1024, "Retro Achievements Client %s %s", g_sClientName, g_sClientVersion );
	WCHAR wClientNameBuffer[1024];
	mbstowcs_s( &nTemp, wClientNameBuffer, 1024, sClientName, strlen(sClientName)+1 );

	// Use WinHttpOpen to obtain a session handle.
	hSession = WinHttpOpen( wClientNameBuffer, 
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, 
		WINHTTP_NO_PROXY_BYPASS, 0);

	// Specify an HTTP server.
	if( hSession != NULL )
	{
		hConnect = WinHttpConnect( hSession, RA_HOST_W, INTERNET_DEFAULT_HTTP_PORT, 0);
	}

	if( hConnect != NULL )
	{
		mbstowcs_s( &nTemp, wBuffer, 1024, sRequestedPage, strlen(sRequestedPage)+1 );

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
		
		int nResult = WinHttpAddRequestHeaders(hRequest, contentType, (unsigned long)-1, WINHTTP_ADDREQ_FLAG_ADD_IF_NEW);
		if( nResult != 0 )
		{
			// Add the photo to the stream
			std::ifstream f( sFilename, std::ios::binary );

			std::ostringstream sb_ascii;
			//sb_ascii << str;									
			sb_ascii << "--" << mimeBoundary << "\r\n";															//	--Boundary
			sb_ascii << "Content-Disposition: form-data; name=\"file\"; filename=\"" << sFilename << "\"\r\n";	//	Item header
			sb_ascii << "\r\n";																					//	Spacing
			sb_ascii << f.rdbuf();																				//	Binary content
			sb_ascii << "\r\n";																					//	Spacing
			sb_ascii << "--" << mimeBoundary << "--\r\n";														//	--Boundary--

			const std::string str = sb_ascii.str();

			bSuccess = WinHttpSendRequest(
				hRequest,
				WINHTTP_NO_ADDITIONAL_HEADERS,
				0,
				(void*)str.c_str(),
				static_cast<unsigned long>(str.length()),
				static_cast<unsigned long>(str.length()),
				0);
		}

		if( WinHttpReceiveResponse( hRequest, NULL ) )
		{
			char* sDataDestOffset = &pBufferOut[0];

			DWORD nBytesToRead = 0;
			(*pCharsRead) = 0;
			WinHttpQueryDataAvailable( hRequest, &nBytesToRead );

			//	Note: success is much earlier, as 0 bytes read is VALID
			//	i.e. fetch achievements for new game will return 0 bytes.
			bSuccess = TRUE;

			while( nBytesToRead > 0 )
			{
				char sHttpReadData[8192];
				ZeroMemory( sHttpReadData, 8192 );

				assert( nBytesToRead <= 8192 );
				if( nBytesToRead <= 8192 )
				{
					DWORD nBytesFetched = 0;
					if( WinHttpReadData( hRequest, &sHttpReadData, nBytesToRead, &nBytesFetched ) )
					{
						assert( nBytesToRead == nBytesFetched );

						//Read: parse buffer
						memcpy( sDataDestOffset, sHttpReadData, nBytesFetched );

						sDataDestOffset += nBytesFetched;
						(*pCharsRead) += nBytesFetched;
					}
				}

				WinHttpQueryDataAvailable( hRequest, &nBytesToRead );
			}

			RA_LOG( __FUNCTION__ ": success! Returned %d bytes.", nBytesToRead );
		}
	}

	return bSuccess;
}

BOOL DoBlockingHttpPost( const char* sRequestedPage, const char* sPostString, char* pBufferOut, const unsigned int nBufferOutSize, DWORD* pCharsRead )
{
	RA_LOG( __FUNCTION__ ": (%08x) POST to %s?%s...\n", GetCurrentThreadId(), sRequestedPage, sPostString );

	BOOL bResults = FALSE, bSuccess = FALSE;
	HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;

	WCHAR wBuffer[1024];
	size_t nTemp;
	char sHttpReadData[8192];
	char* sDataDestOffset = &pBufferOut[0];
	DWORD nBytesToRead = 0;
	DWORD nBytesFetched = 0;
	int nRemainingBuffer = 0;

	char sClientName[1024];
	sprintf_s( sClientName, 1024, "Retro Achievements Client %s %s", g_sClientName, g_sClientVersion );
	WCHAR wClientNameBuffer[1024];
	mbstowcs_s( &nTemp, wClientNameBuffer, 1024, sClientName, strlen(sClientName)+1 );

 	// Use WinHttpOpen to obtain a session handle.
 	hSession = WinHttpOpen( wClientNameBuffer, 
 		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
 		WINHTTP_NO_PROXY_NAME, 
 		WINHTTP_NO_PROXY_BYPASS, 0);
 
 	// Specify an HTTP server.
	if( hSession != NULL )
	{
 		hConnect = WinHttpConnect( hSession, RA_HOST_W, INTERNET_DEFAULT_HTTP_PORT, 0);
 
 		// Create an HTTP Request handle.
 		if( hConnect != NULL )
		{
			mbstowcs_s( &nTemp, wBuffer, 1024, sRequestedPage, strlen(sRequestedPage)+1 );

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
 					L"Content-Type: application/x-www-form-urlencoded",	//	WOOHOO!
 					//L"Content-Type: text/plain",	//<-- NOPE!!
 					0, 
 					(LPVOID)sPostString, //WINHTTP_NO_REQUEST_DATA,
 					strlen(sPostString), 
 					strlen(sPostString),
 					0);

				if( WinHttpReceiveResponse( hRequest, NULL ) )
				{
					char* sDataDestOffset = &pBufferOut[0];

					nBytesToRead = 0;
					(*pCharsRead) = 0;
					WinHttpQueryDataAvailable( hRequest, &nBytesToRead );

					//	Note: success is much earlier, as 0 bytes read is VALID
					//	i.e. fetch achievements for new game will return 0 bytes.
					bSuccess = TRUE;

					while( nBytesToRead > 0 )
					{
						ZeroMemory( sHttpReadData, 8192 );

						assert( nBytesToRead <= 8192 );
						if( nBytesToRead <= 8192 )
						{
							nBytesFetched = 0;
							if( WinHttpReadData( hRequest, &sHttpReadData, nBytesToRead, &nBytesFetched ) )
							{
								assert( nBytesToRead == nBytesFetched );

								//Read: parse buffer

								nRemainingBuffer = nBufferOutSize - (*pCharsRead);

								if( nRemainingBuffer < 0 )
								{
									//	ignore: we aren't handling this any more: just keep reading then close.
								}
								else if( nRemainingBuffer < (int)nBytesFetched )
								{
									// 10000 buffer
									//	fetched 8000, written it.
									//	fetched 10000 - must write 2000.

									memcpy( sDataDestOffset, sHttpReadData, nRemainingBuffer );
									sDataDestOffset += nRemainingBuffer;
									(*pCharsRead) += nRemainingBuffer;
								}
								else //if( nBytesFetched > nBufferOutSize )
								{
									memcpy( sDataDestOffset, sHttpReadData, nBytesFetched );
									sDataDestOffset += nBytesFetched;
									(*pCharsRead) += nBytesFetched;
								}

							}
						}

						WinHttpQueryDataAvailable( hRequest, &nBytesToRead );
					}

					RA_LOG( "DoBlockingHttpPost: POST to %s Success: %d bytes read\n", sRequestedPage, nBytesFetched );

				}
 			}
 		}
	}
 
 
 	// Close open handles.
 	if (hRequest) WinHttpCloseHandle(hRequest);
 	if (hConnect) WinHttpCloseHandle(hConnect);
 	if (hSession) WinHttpCloseHandle(hSession);

	return bSuccess;
}


BOOL HTTPRequestExists( const char* sRequestPageName )
{
	return HttpRequestQueue.PageRequestExists( sRequestPageName );
}

//	Adds items to the httprequest queue
BOOL CreateHTTPRequestThread( const char* sRequestedPage, const char* sPostString, enum HTTPRequestType nType, int nUserRef, cb_OnReceive pfOnReceive )
{
	RA_LOG( __FUNCTION__ " %s\n", sRequestedPage );

	RequestObject* pObj = (RequestObject*)malloc( sizeof(RequestObject) );
	if( pObj == NULL )
		return FALSE;

	ZeroMemory( pObj, sizeof(RequestObject) );

	strcpy_s( pObj->m_sRequestPageName, 1024, sRequestedPage );
	strcpy_s( pObj->m_sRequestPost, 1024, sPostString );
	strcpy_s( pObj->m_sResponse, 1024, "" );
	ZeroMemory( pObj->m_sResponse, 32768 );	//	Just to be sure
	pObj->m_nUserRef = nUserRef;
	pObj->m_nReqType = nType;
	pObj->m_pfCallbackOnReceive = pfOnReceive;
	
	HttpRequestQueue.PushItem( pObj );

	RA_LOG( __FUNCTION__ " added, queue is at %d\n", HttpRequestQueue.Count() );

	return TRUE;
}

//	Takes items from the http request queue, and posts them to the last http results queue.
DWORD WINAPI HTTPWorkerThread( LPVOID lpParameter )
{
	time_t nSendNextKeepAliveAt = time( NULL ) + RA_SERVER_POLL_DURATION;

	bool bActive = true;
	bool bDoPingKeepAlive = ( (int)lpParameter ) == 0;

	while( bActive )
	{
		RequestObject* pObj = HttpRequestQueue.PopNextItem();
		if( pObj != NULL )
		{
			DWORD nRead = 0;
			char* pStrOut = (char*)pObj->m_sResponse;

			BOOL bRetVal = FALSE;

			switch( pObj->m_nReqType )
			{
			case HTTPRequest_Post:
				bRetVal = DoBlockingHttpPost( pObj->m_sRequestPageName, pObj->m_sRequestPost, pStrOut, 32768, &nRead );
				break;
			case HTTPRequest_Get:
				bRetVal = DoBlockingHttpGet( pObj->m_sRequestPageName, pStrOut, 32768, &nRead );
				break;
			case HTTPRequest_StopThread:
				bActive = false;
				bDoPingKeepAlive = false;
				break;
			default:
				assert(0);
				break;
			}

			pObj->m_nBytesRead = nRead;
			pObj->m_bResponse = bRetVal;

			//	As a worker thread, we CANNOT directly use callbacks: we are not in the correct thread!
			//if( pObj->m_nUserRef != MAINTHREAD_CB &&
			//	pObj->m_pfCallbackOnReceive != NULL )
			//{
			//	pObj->m_pfCallbackOnReceive( pObj );
			//}

			if( bActive )
			{
				//	Push object over to results queue - let app deal with them now.
				LastHttpResults.PushItem( pObj );
			}
			else
			{
				//	Take ownership and free(): we caused the 'pop' earlier, so we have responsibility to
				//	 either pass to LastHttpResults, or deal with it here.
				free( pObj );
				pObj = NULL;
			}
		}

		if( bDoPingKeepAlive )
		{
			//	Post a pingback once every few minutes to keep the server aware of our presence
			if( time( NULL ) > nSendNextKeepAliveAt )
			{
				nSendNextKeepAliveAt += RA_SERVER_POLL_DURATION;

				//	Post a keepalive packet:
				if( g_LocalUser.m_bIsLoggedIn )
				{
					char sPostString[4096];
					sprintf_s( sPostString, 4096, "u=%s", g_LocalUser.Username() );
					
					if( RA_GameIsActive() )
					{
						char sMessage[1024];
						
						if( ( g_MemoryDialog.GetHWND() != NULL ) && ( IsWindowVisible( g_MemoryDialog.GetHWND() ) ) )
						{
							sprintf_s( sMessage, "Developing Achievements" );
						}
						else
						{
							const std::string& sRPResponse = g_RichPresenceInterpretter.GetRichPresenceString();
							if( sRPResponse.size() == 0 )
							{
								sprintf_s( sMessage, "Earning Achievements" );
							}
							else
							{
								sprintf_s( sMessage, "%s", sRPResponse.c_str() );
							}
						}

						char sActivityStr[2048];
						sprintf_s( sActivityStr, 2048, "&g=%d&m=%s",
							CoreAchievements->GameID(),
							sMessage );	//	TBD

						//	concat
						strcat_s( sPostString, 4096, sActivityStr );
					}
					
					CreateHTTPRequestThread( "ping.php", sPostString, HTTPRequest_Post, 0, NULL );
				}
			}
		}

		if( HttpRequestQueue.Count() > 0 )
			RA_LOG( __FUNCTION__ " (%08x) request queue is at %d\n", GetCurrentThreadId(), HttpRequestQueue.Count() );

		Sleep(100);
	}

	//	Delete and empty queue - allocated data is within!
	LastHttpResults.Clear();

	return 0;
}

void RA_InitializeHTTPThreads()
{
	RA_LOG( __FUNCTION__ " called\n" );

	DWORD dwThread;
	g_hHTTPMutex = CreateMutex( NULL, FALSE, NULL );

	for( size_t i = 0; i < g_nNumHTTPThreads; ++i )
	{
		HANDLE hThread = CreateThread( NULL, 0, HTTPWorkerThread, (void*)i, 0 , &dwThread );
		assert( hThread != NULL );
		if( hThread != NULL )
		{
			g_vhHTTPThread.push_back( hThread );
			RA_LOG( __FUNCTION__ " Adding HTTP thread %d\n", i );
		}
	}
}

void RA_KillHTTPThreads()
{
	RA_LOG( __FUNCTION__ " called\n" );

	for( size_t i = 0; i < g_vhHTTPThread.size(); ++i )
	{
		//	Create 5 of these:
		CreateHTTPRequestThread( "", "", HTTPRequest_StopThread, 0, NULL );
	}
	
	for( size_t i = 0; i < g_vhHTTPThread.size(); ++i )
	{
		//	Wait for 5 responses:
		DWORD nResult = WaitForSingleObject( g_vhHTTPThread[i], INFINITE );
		RA_LOG( __FUNCTION__ " ended, result %d\n", nResult );
	}
}

void RequestObject::Clean()
{
	m_nReqType = HTTPRequest_Post;
	m_sRequestPageName[0] = '\0';
	m_sRequestPost[0] = '\0';
	m_sResponse[0] = '\0';
	m_bResponse = FALSE;
	m_nBytesRead = 0;
	m_nUserRef = 0;
	m_pfCallbackOnReceive = NULL;
}

//////////////////////////////////////////////////////////////////////////

RequestObject* HttpResults::PopNextItem()
{
	RequestObject* pRetVal = NULL;
	WaitForSingleObject( g_hHTTPMutex, INFINITE );
	{
		if( m_aRequests.size() > 0 )
		{
			pRetVal = m_aRequests.front();
			m_aRequests.pop_front();
		}
	}
	ReleaseMutex( g_hHTTPMutex );

	return pRetVal;
}

const RequestObject* HttpResults::PeekNextItem() const
{
	RequestObject* pRetVal = NULL;
	WaitForSingleObject( g_hHTTPMutex, INFINITE );
	{
		pRetVal = m_aRequests.front();
	}
	ReleaseMutex( g_hHTTPMutex );
		
	return pRetVal; 
}

void HttpResults::PushItem( RequestObject* pObj )
{ 
	WaitForSingleObject( g_hHTTPMutex, INFINITE );
	{
		m_aRequests.push_front( pObj ); 
	}
	ReleaseMutex( g_hHTTPMutex );
}

void HttpResults::Clear()
{ 
	WaitForSingleObject( g_hHTTPMutex, INFINITE );
	{
		while( !m_aRequests.empty() )
		{
			RequestObject* pObj = m_aRequests.front();
			m_aRequests.pop_front();
			free( pObj );	//	Must free after pop! Otherwise mem leak!
			pObj = NULL;
		}
	}
	ReleaseMutex( g_hHTTPMutex );
}

size_t HttpResults::Count() const
{ 
	size_t nCount = 0;
	WaitForSingleObject( g_hHTTPMutex, INFINITE );
	{
		nCount = m_aRequests.size();
	}
	ReleaseMutex( g_hHTTPMutex );

	return nCount;
}

BOOL HttpResults::PageRequestExists( const char* sPageName ) const
{
	BOOL bRetVal = FALSE;
	WaitForSingleObject( g_hHTTPMutex, INFINITE );
	{
		std::deque<RequestObject*>::const_iterator iter = m_aRequests.begin();
		while( iter != m_aRequests.end() )
		{
			const RequestObject* pObj = (*iter);
			if( _stricmp( pObj->m_sRequestPageName, sPageName ) == 0 )
			{
				bRetVal = TRUE;
				break;
			}

			iter++;
		}
	}
	ReleaseMutex( g_hHTTPMutex );

	return bRetVal;
}