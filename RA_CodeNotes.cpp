#include "RA_CodeNotes.h"

#include <Windows.h>
#include <string>
#include <utility>
#include <vector>

#include "RA_Core.h"
#include "RA_Dlg_Memory.h"
#include "RA_Achievement.h"
#include "RA_httpthread.h"
#include "RA_MemManager.h"
#include "RA_User.h"

namespace
{
	const char Divider = ':';
	const char EndLine = '\n';
}

CodeNotes::CodeNotes()
{
}

CodeNotes::~CodeNotes()
{
}

void CodeNotes::Clear()
{
	m_sCodeNotes.clear();
}


void CodeNotes::Sort()
{
	for( size_t i = 0; i < m_sCodeNotes.size(); ++i )
	{
		BOOL bComplete = TRUE;
		for( size_t j = i; j < m_sCodeNotes.size(); ++j )
		{
			if( strcmp( m_sCodeNotes[j].m_sAddress.c_str(), m_sCodeNotes[i].m_sAddress.c_str() ) < 0 )
			{
				bComplete = FALSE;

				CodeNoteObj temp = m_sCodeNotes[i];
				m_sCodeNotes[i] = m_sCodeNotes[j];
				m_sCodeNotes[j] = temp;

				//std::swap<CodeNotePair>( m_sCodeNotes[j], m_sCodeNotes[i] );
				//swap
			}
		}

		if( bComplete )
			break;
	}
}

BOOL CodeNotes::Update( unsigned int nID )
{
	if( nID == 0 )
		return FALSE;
	
	char buffer[256];
	sprintf_s( buffer, 256, "g=%d", nID );

	CreateHTTPRequestThread( "requestcodenotes.php", buffer, HTTPRequest_Post, 0, &CodeNotes::s_OnUpdateCB );
	
	return TRUE;
}

//	static
void CodeNotes::s_OnUpdateCB(void* pObj)
{
	if( pObj == NULL )
		return;

	RequestObject* pRObj = (RequestObject*)(pObj);
	RequestObject& rObj = *pRObj;

	if( strncmp( rObj.m_sResponse, "OK:", 3 ) )
	{
		assert(!"Bad response from requestcodenotes!");
		return;
	}
	
	char* pIter = &rObj.m_sResponse[3];
	char* pGameID = strtok_s( pIter, ":", &pIter );

	char buffer[256];
	sprintf_s( buffer, 256, "%s%s-Notes2.txt", RA_DIR_DATA, pGameID );

	const unsigned int nBytesToSkip = (pIter-rObj.m_sResponse);

	SetCurrentDirectory( g_sHomeDir );
	_WriteBufferToFile( buffer, pIter, rObj.m_nBytesRead-nBytesToSkip );

	g_MemoryDialog.RepopulateMemNotesFromFile();
}

size_t CodeNotes::Load( const char* sFile )
{
	Clear();

	FILE* pFile = NULL;
	if( fopen_s( &pFile, sFile, "r" ) == 0 )
	{
		//	Get game title:
		DWORD nCharsRead = 0;
		do
		{
			char sAuthor[64];
			char sAddress[64];
			char sNote[512];

			if( !_ReadTil( Divider, sAuthor, 64, &nCharsRead, pFile ) )
				break;

			//	Turn colon into a endstring
			sAuthor[nCharsRead-1] = '\0';

			if( !_ReadTil( Divider, sAddress, 64, &nCharsRead, pFile ) )
				break;

			//	Turn colon into a endstring
			sAddress[nCharsRead-1] = '\0';

			if( !_ReadTil( '#', sNote, 512, &nCharsRead, pFile ) )
				break;
			
			//	Turn newline into a endstring
			sNote[nCharsRead-1] = '\0';

			char sAddressFixed[64];
			if( g_MemManager.RAMTotalSize() > 65536 )
				sprintf_s( sAddressFixed, 64, "0x%s", sAddress+2 ); 
			else
				sprintf_s( sAddressFixed, 64, "0x%s", sAddress+4 );	//	Adjust the 6-figure readout
			
			m_sCodeNotes.push_back( CodeNoteObj( sAuthor, sAddressFixed, sNote ) );
		}
		while ( !feof( pFile ) );

		fclose( pFile );
	}
	else
	{
		//	Create?
	}

	return m_sCodeNotes.size();
} 

BOOL CodeNotes::Save( const char* sFile )
{
	unsigned int nCharsRead = 0;

	Sort();

	FILE* pFile = NULL;
	if( fopen_s( &pFile, sFile, "w" ) == 0 )
	{
		std::vector< CodeNoteObj >::const_iterator iter = m_sCodeNotes.begin();
		while( iter != m_sCodeNotes.end() )
		{
			const CodeNoteObj& NextItem = (*iter);
			fwrite( NextItem.m_sAuthor.c_str(), sizeof(char), NextItem.m_sAuthor.length(), pFile );
			fwrite( &Divider, sizeof(char), 1, pFile );
			fwrite( NextItem.m_sAddress.c_str(), sizeof(char), NextItem.m_sAddress.length(), pFile );
			fwrite( &Divider, sizeof(char), 1, pFile );
			fwrite( NextItem.m_sNote.c_str(), sizeof(char), NextItem.m_sNote.length(), pFile );
			fwrite( &EndLine, sizeof(char), 1, pFile );

			iter++;
		}

		fclose( pFile );

		return TRUE;
	}
	else
	{
		//	Create?
		return FALSE;
	}
}

BOOL CodeNotes::Exists( const char* sAddress, char* sAuthorOut, char* sDescriptionOut, const size_t nMaxLen )
{
	std::vector< CodeNoteObj >::const_iterator iter;
	iter = m_sCodeNotes.begin();

	while( iter != m_sCodeNotes.end() )
	{
		const CodeNoteObj& NextItem = (*iter);
		if( strcmp( sAddress, NextItem.m_sAddress.c_str() ) == 0 )
		{
			if( sAuthorOut )
				strcpy_s( sAuthorOut, nMaxLen, NextItem.m_sAuthor.c_str() );
			if( sDescriptionOut )
				strcpy_s( sDescriptionOut, nMaxLen, NextItem.m_sNote.c_str() );
			return TRUE;
		}
		iter++;
	}

	return FALSE;
}

BOOL CodeNotes::ExistsRef( const char* sAddress, std::string*& psDescOut )
{
	std::vector< CodeNoteObj >::iterator iter;
	iter = m_sCodeNotes.begin();

	while( iter != m_sCodeNotes.end() )
	{
		CodeNoteObj& rNextItem = (*iter);
		if( strcmp( sAddress, rNextItem.m_sAddress.c_str() ) == 0 )
		{
			psDescOut = &rNextItem.m_sNote;
			return TRUE;
		}
		
		iter++;
	}

	return FALSE;
}

void CodeNotes::Add( const char* sAuthor, const char* sAddress, const char* sDescription )
{
	std::string* psDesc = NULL;
	if( ExistsRef( sAddress, psDesc ) )
	{
		//	Update it:
		(*psDesc) = sDescription;
	}
	else
	{
		m_sCodeNotes.push_back( CodeNoteObj( sAuthor, sAddress, sDescription ) );
	}

	if( g_LocalUser.m_bIsLoggedIn && ( strlen( sAddress ) > 2 ) )
	{ 
		char buffer[1024];
		sprintf_s( buffer, 1024, "u=%s&t=%s&g=%d&a=%s&n=%s", 
			g_LocalUser.Username(),
			g_LocalUser.m_sToken,
			g_pActiveAchievements->GameID(),
			sAddress,
			sDescription );

		//	faf
		//CreateHTTPRequestThread( "requestsubmitcodenote.php", buffer, HTTPRequest_Post, 0, NULL );
		
		char bufferResponse[1024];
		DWORD nResponseSize = 0;

		if( DoBlockingHttpPost( "requestsubmitcodenote.php", buffer, bufferResponse, 1024, &nResponseSize ) )
		{
			//	OK!
			MessageBeep( 0xFFFFFFFF );
			//MessageBox( g_RAMainWnd, "Saved OK!", "OK!", MB_OK );
		}
		else
		{
			MessageBox( g_RAMainWnd, "Could not save note! Please check you are online and retry.", "Error!", MB_OK|MB_ICONWARNING );
		}
	}
}

BOOL CodeNotes::Remove( const char* sAddress )
{
	std::vector< CodeNoteObj >::iterator iter;
	iter = m_sCodeNotes.begin();

	while( iter != m_sCodeNotes.end() )
	{
		CodeNoteObj& rNextItem = (*iter);
		if( strcmp( rNextItem.m_sAddress.c_str(), sAddress ) == 0 )
		{
			m_sCodeNotes.erase( (iter) );

			if( g_LocalUser.m_bIsLoggedIn )
			{
				char buffer[1024];
				sprintf_s( buffer, 1024, "u=%s&t=%s&g=%d&a=%s&n=''", 
					g_LocalUser.Username(),
					g_LocalUser.m_sToken,
					g_pActiveAchievements->GameID(),
					sAddress );

				//	faf
				CreateHTTPRequestThread( "requestsubmitcodenote.php", buffer, HTTPRequest_Post, 0, NULL );
			}

			return TRUE;
		}

		iter++;
	}
	
	return FALSE;
}