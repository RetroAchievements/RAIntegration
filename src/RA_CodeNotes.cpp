#include "RA_CodeNotes.h"

#include <Windows.h>

#include "RA_Core.h"
#include "RA_httpthread.h"
#include "RA_Dlg_Memory.h"
#include "RA_User.h"
#include "RA_Achievement.h"
#include "RA_AchievementSet.h"
#include "RA_GameData.h"
#include <fstream>


void CodeNotes::Clear()
{
	m_CodeNotes.clear();
}

size_t CodeNotes::Load( const std::string& sFile )
{
	Clear();
	
	SetCurrentDirectory(NativeStr( g_sHomeDir ).c_str());

	std::ifstream ifile{ sFile, std::ios::binary };

	// constructor calls open automatically and throws an exception, should rarely if not ever happen

	Document doc;
	IStreamWrapper isw{ ifile };
	doc.ParseStream(isw);
	if (!doc.HasParseError())

	{
		// You know that assert gets compiled out in release mode right?
		ASSERT(doc["CodeNotes"].IsArray());

		// use auto when you can to prevent implicit conversions (you get warnings on higher levels)
		const auto& NoteArray = doc["CodeNotes"];

		for (auto& i : NoteArray.GetArray())
		{
			// what? that's the first I ever heard that
			if (i["Note"].IsNull())
				continue;

			const std::string& sNote = i["Note"].GetString();
			if (sNote.length() < 2)
				continue;

			const std::string& sAddr = i["Address"].GetString();
			auto nAddr = static_cast<ByteAddress>(std::strtoul(sAddr.c_str(), nullptr, 16));

            // Moved comments to the top becaue of indentation complaints
			//	Author?
			const std::string& sAuthor = i["User"].GetString();	

			// Your CodeNoteObj would need a move constructor for this to work properly, here's a better approach
			auto code_pair{ std::make_pair(nAddr, CodeNoteObj{ sAuthor, sNote }) };
			m_CodeNotes.insert(code_pair);
		} // end for

	}

	return m_CodeNotes.size();
} 

BOOL CodeNotes::Save( const std::string& sFile )
{
	return FALSE;
	//	All saving should be cloud-based!
}

BOOL CodeNotes::ReloadFromWeb( GameID nID )
{
	if( nID == 0 )
		return FALSE;
	
	PostArgs args;
	args['g'] = std::to_string( nID );
	RAWeb::CreateThreadedHTTPRequest( RequestCodeNotes, args );
	return TRUE;
}

//	static
void CodeNotes::OnCodeNotesResponse( Document& doc )
{
	//	Persist then reload
	const GameID nGameID = doc["GameID"].GetUint();

	SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());
	_WriteBufferToFile( std::string( RA_DIR_DATA ) + std::to_string( nGameID ) + "-Notes2.txt", doc );

	g_MemoryDialog.RepopulateMemNotesFromFile();
}

void CodeNotes::Add( const ByteAddress& nAddr, const std::string& sAuthor, const std::string& sNote )
{
	if( m_CodeNotes.find( nAddr ) == m_CodeNotes.end() )
		m_CodeNotes.insert( std::map<ByteAddress,CodeNoteObj>::value_type( nAddr, CodeNoteObj( sAuthor, sNote ) ) );
	else
		m_CodeNotes.at( nAddr ).SetNote( sNote );

	if( RAUsers::LocalUser().IsLoggedIn() )
	{ 
		PostArgs args;
		args['u'] = RAUsers::LocalUser().Username();
		args['t'] = RAUsers::LocalUser().Token();
		args['g'] = std::to_string( g_pCurrentGameData->GetGameID() );
		args['m'] = std::to_string( nAddr );
		args['n'] = sNote;

		Document doc;
		if( RAWeb::DoBlockingRequest( RequestSubmitCodeNote, args, doc ) )
		{
			//	OK!
			MessageBeep( 0xFFFFFFFF );
		}
		else
		{
			MessageBox( g_RAMainWnd, _T( "Could not save note! Please check you are online and retry." ), _T( "Error!" ), MB_OK|MB_ICONWARNING );
		}
	}
}

BOOL CodeNotes::Remove( const ByteAddress& nAddr )
{
	if( m_CodeNotes.find( nAddr ) == m_CodeNotes.end() )
	{
		RA_LOG( "Already deleted this code note? (%d), nAddr " );
		return FALSE;
	}

	m_CodeNotes.erase( nAddr );
	
	if( RAUsers::LocalUser().IsLoggedIn() )
	{
		PostArgs args;
		args['u'] = RAUsers::LocalUser().Username();
		args['t'] = RAUsers::LocalUser().Token();
		args['g'] = std::to_string(g_pCurrentGameData->GetGameID());
		args['m'] = std::to_string( nAddr );
		args['n'] = "";

		//	faf
		RAWeb::CreateThreadedHTTPRequest( RequestSubmitCodeNote, args );
	}
	
	return TRUE;
}
