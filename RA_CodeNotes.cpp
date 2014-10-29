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

#include "rapidjson/include/rapidjson/document.h"

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
	
	PostArgs Args;
	Args['g'] = std::to_string( nID );
	RAWeb::CreateThreadedHTTPRequest( RequestCodeNotes, Args );
	return TRUE;
}

//	static
void CodeNotes::s_OnUpdateCB( void* pReqObj )
{
	if( pReqObj == NULL )
		return;

	RequestObject* pObj = static_cast<RequestObject*>( pReqObj );

	if( !pObj->GetSuccess() )
	{
		assert(!"Bad response from requestcodenotes!");
		return;
	}
	
	DataStream Response = pObj->GetResponse();
	//const std::vector<char> Response = static_cast< const std::vector<char> >( pObj->GetResponse() );	//	Copy by val
	
	//BYTE* pIter = &rObj.[3];
	//	JSON TBD:

	Document doc;
	doc.ParseInsitu( DataStreamAsString( Response ) );

	//Response
	//{"Success":true,"CodeNotes":[{"User":"Retromancer","Address":"0x00d008","Note":"''"},{"User":"jadersonic","Address":"0x00d01c","Note":""}]}

	if( !doc.HasParseError() && doc["Success"].GetBool() )
	{
		assert( doc["CodeNotes"].IsArray() );
		const Value& Notes = doc["CodeNotes"];
		for( SizeType i = 0; i < Notes.Size(); ++i )
		{
			const Value& Note = Notes[i];
			const std::string& sUser = Note["User"].GetString();
			const std::string& sAddress = Note["Address"].GetString();
			const std::string& sNote = Note["Note"].GetString();
			RA_LOG( "CodeNote: %s, %s (%s)\n", sAddress.c_str(), sNote.c_str(), sUser.c_str() );
			//	TBD: store?
		}
	}

	std::string sGameID = pObj->GetPostArgs().at('g');
	
	SetCurrentDirectory( g_sHomeDir );

	char sFilename[256];
	sprintf_s( sFilename, 256, "%s%s-Notes2.txt", RA_DIR_DATA, sGameID.c_str() );
	//_WriteBufferToFile( buffer, pIter, rObj.m_nBytesRead-nBytesToSkip );
	_WriteBufferToFile( sFilename, Response.data(), Response.size() );

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

	if( RAUsers::LocalUser.IsLoggedIn() && ( strlen( sAddress ) > 2 ) )
	{ 
		PostArgs args;
		args['u'] = RAUsers::LocalUser.Username();
		args['t'] = RAUsers::LocalUser.Token();
		args['g'] = std::to_string( g_pActiveAchievements->GameID() );
		args['a'] = sAddress;
		args['d'] = sDescription;

		//	faf
		//CreateHTTPRequestThread( "requestsubmitcodenote.php", buffer, HTTPRequest_Post, 0, NULL );
		
		DataStream Response;
		if( RAWeb::DoBlockingHttpPost( "requestsubmitcodenote.php", PostArgsToString( args ), Response ) )
		{
			//	OK!
			MessageBeep( 0xFFFFFFFF );
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

			if( RAUsers::LocalUser.IsLoggedIn() )
			{
				PostArgs args;
				args['u'] = RAUsers::LocalUser.Username();
				args['t'] = RAUsers::LocalUser.Token();
				args['g'] = std::to_string( g_pActiveAchievements->GameID() );
				args['a'] = sAddress;
				args['n'] = "";

				//	faf
				RAWeb::CreateThreadedHTTPRequest( RequestSubmitCodeNote, args );
			}

			return TRUE;
		}

		iter++;
	}
	
	return FALSE;
}