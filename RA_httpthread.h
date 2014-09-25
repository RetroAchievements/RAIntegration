#pragma once

#include "RA_Defs.h"
#include <deque>
#include <assert.h>

typedef void* HANDLE;

enum HTTPRequestType
{
	HTTPRequest_Post,
	HTTPRequest_Get,
	HTTPRequest_StopThread,

	HTTPRequest__Max
};

typedef void (*cb_OnReceive)(void* pObj);

class RequestObject
{
public:
	void Clean();

public:
	char m_sRequestPageName[256];
	char m_sRequestPost[256];
	char m_sResponse[32768];
	enum HTTPRequestType m_nReqType;
	BOOL m_bResponse;
	int m_nBytesRead;
	int m_nUserRef;
	cb_OnReceive m_pfCallbackOnReceive;
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


extern HANDLE g_hHTTPMutex;
extern HttpResults LastHttpResults;

void RA_InitializeHTTPThreads();
void RA_KillHTTPThreads();
BOOL DirectoryExists( const char* sPath );

BOOL CreateHTTPRequestThread( const char* sRequestedPage, const char* sPostString, enum HTTPRequestType nType, int nUserRef, cb_OnReceive pfOnReceive );
BOOL HTTPRequestExists( const char* sRequestPageName );

BOOL DoBlockingHttpGet( const char* sRequestedPage, char* pBufferOut, const unsigned int nBufferOutSize, unsigned long* rBytesRead );
BOOL DoBlockingHttpPost( const char* sRequestedPage, const char* sPostString, char* pBufferOut, const unsigned nBufferOutSize, DWORD* rCharsRead );
BOOL DoBlockingImageUpload( const char* sRequestedPage, const char* sFilename, char* pBufferOut, const unsigned int nBufferOutSize, DWORD* rCharsRead );


