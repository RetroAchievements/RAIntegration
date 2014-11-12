#include "RA_Dlg_GameTitle.h"

#include <windowsx.h>
#include <stdio.h>

#include "RA_Defs.h"
#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_Achievement.h"
#include "RA_httpthread.h"


Dlg_GameTitle g_GameTitleDialog;

INT_PTR CALLBACK s_GameTitleProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return g_GameTitleDialog.GameTitleProc( hDlg, uMsg, wParam, lParam );
}

INT_PTR Dlg_GameTitle::GameTitleProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	static bool bUpdatingTextboxTitle = false;

 	switch(uMsg)
 	{
 	case WM_INITDIALOG:
	{
		HWND hKnownGamesCbo = GetDlgItem( hDlg, IDC_RA_KNOWNGAMES );
		char sGameTitleTidy[64];
		char buffer[65535];
		char* pBuffer = &buffer[0];
		int nSel = 0;
		const char* NewLine = "\n";
		char* pNextTitle = NULL;

		strcpy_s( sGameTitleTidy, 64, g_GameTitleDialog.m_sEstimatedGameTitle.c_str() );
		Dlg_GameTitle::CleanRomName( sGameTitleTidy, 64 );

		SetDlgItemText( hDlg, IDC_RA_GAMETITLE, sGameTitleTidy );

		//	Load in the checksum
		SetDlgItemText( hDlg, IDC_RA_CHECKSUM, g_GameTitleDialog.m_sMD5.c_str() );

		//	Populate the dropdown
		//	***Do blocking fetch of all game titles.***
		ZeroMemory( buffer, 65535 );
		
		nSel = ComboBox_AddString( hKnownGamesCbo, "<New Title>" );
		ComboBox_SetCurSel( hKnownGamesCbo, nSel );

		PostArgs args;
		args['c'] = std::to_string( g_ConsoleID );

		Document doc;
		if( RAWeb::DoBlockingRequest( RequestGameTitles, args, doc ) )
		{
			const Value& Data = doc["Response"];

			//	For all data responses to this request, populate our m_aGameTitles map
			{
				Value::ConstMemberIterator iter = Data.MemberBegin();
				while( iter != Data.MemberEnd() )
				{
					if( iter->name.IsNull() || iter->value.IsNull() )
					{
						iter++;
						continue;
					}

					const GameID nGameID = std::strtoul( iter->name.GetString(), NULL, 10 );	//	Keys cannot be anything but strings
					const std::string& sTitle = iter->value.GetString();
					m_aGameTitles[sTitle] = nGameID;

					iter++;
				}
			}

			{
				std::map<std::string,GameID>::const_iterator iter = m_aGameTitles.begin();
				while( iter != m_aGameTitles.end() )
				{
					nSel = ComboBox_AddString( hKnownGamesCbo, (*iter).first.c_str() );

					//	Attempt to find this game and select it by default: case insensitive comparison
					if( _stricmp( sGameTitleTidy, (*iter).first.c_str() ) == 0 )
						ComboBox_SetCurSel( hKnownGamesCbo, nSel );

					iter++;
				}

			}
		}

		return TRUE;
	}
	break;
 
 	case WM_COMMAND:
 		switch( LOWORD(wParam) )
 		{
 		case IDOK:
 			{
				//	Fetch input data:
				char sSelectedTitle[512];
				ComboBox_GetText( GetDlgItem( hDlg, IDC_RA_KNOWNGAMES ), sSelectedTitle, 512 );
				
				GameID nGameID = 0;
				if( strcmp( sSelectedTitle, "<New Title>" ) == 0 )
				{
					//	Add a new title!
 					GetDlgItemText( hDlg, IDC_RA_GAMETITLE, sSelectedTitle, 512 );
				}
				else
				{
					//	Existing title
					ASSERT( m_aGameTitles.find( std::string( sSelectedTitle ) ) != m_aGameTitles.end() );
					nGameID = m_aGameTitles[ std::string( sSelectedTitle ) ];
				}

				PostArgs args;
				args['u'] = RAUsers::LocalUser.Username();
				args['t'] = RAUsers::LocalUser.Token();
				args['m'] = m_sMD5;
				args['i'] = sSelectedTitle;
				args['c'] = std::to_string( g_ConsoleID );

				Document doc;
				if( RAWeb::DoBlockingRequest( RequestSubmitNewTitle, args, doc ) && doc.HasMember("Success") && doc["Success"].GetBool() )
				{
					const Value& Response = doc["Response"];

					const GameID nGameID = static_cast<GameID>( Response["GameID"].GetUint() );
					const std::string& sGameTitle = Response["GameTitle"].GetString();

					//	If we're setting the game title here...
					CoreAchievements->SetGameTitle( sGameTitle );
					UnofficialAchievements->SetGameTitle( sGameTitle );
					LocalAchievements->SetGameTitle( sGameTitle );
 
					//	Surely we could set the game ID here too?
					CoreAchievements->SetGameID( nGameID );
					UnofficialAchievements->SetGameID( nGameID );
					LocalAchievements->SetGameID( nGameID );

					g_GameTitleDialog.m_nReturnedGameID = nGameID;

 					//	Close this dialog
 					EndDialog( hDlg, TRUE );
 					return TRUE;
 				}
 				else
 				{
					if( !doc.HasParseError() && doc.HasMember( "Error" ) )
					{
						//Error given
						MessageBox( hDlg,
							( std::string( "Could not add new title: " ) + std::string( doc["Error"].GetString() ) ).c_str(),
							"Errors encountered",
							MB_OK );
					}
					else
					{
 						MessageBox( hDlg, 
							"Cannot contact server!", 
							"Error in connection", 
							MB_OK );
					}
 
 					return TRUE;
 				}
 			}
 		case IDCANCEL:
 			EndDialog( hDlg, TRUE );
 			return TRUE;
 			break;
		case IDC_RA_GAMETITLE:
			switch( HIWORD(wParam) )
			{
				case EN_CHANGE:
				{
					if( !bUpdatingTextboxTitle )	//	Note: use barrier to prevent automatic switching off.
					{
						//	If the user has started to enter a value, set the upper combo to 'new entry'
						HWND hKnownGamesCbo = GetDlgItem( hDlg, IDC_RA_KNOWNGAMES );
						ComboBox_SetCurSel( hKnownGamesCbo, 0 );
					}
				}
				break;
			}
			break;
		case IDC_RA_KNOWNGAMES:
			switch( HIWORD(wParam) )
			{
				case CBN_SELCHANGE:
				{
					//	If the user has selected a value, copy this text to the bottom textbox.
					bUpdatingTextboxTitle = TRUE;

					char sSelectedTitle[512];
 					GetDlgItemText( hDlg, IDC_RA_KNOWNGAMES, sSelectedTitle, 512 );
					SetDlgItemText( hDlg, IDC_RA_GAMETITLE, sSelectedTitle );
					
					bUpdatingTextboxTitle = FALSE;
				}
				break;
			}
			break;
 		}
 		break;
 	case WM_KEYDOWN:
		//IDC_RA_GAMETITLE
		wParam;
		break;
 	case WM_CLOSE:
 		// 		if (Full_Screen)
 		// 		{
 		// 			while (ShowCursor(true) < 0);
 		// 			while (ShowCursor(false) >= 0);
 		// 		}
 
 		EndDialog( hDlg, FALSE );
 		return TRUE;
 		break;
 	}

	return FALSE;
}

unsigned int Dlg_GameTitle::DoModalDialog( HINSTANCE hInst, HWND hParent, const std::string& sMD5, const std::string& sEstimatedGameTitle )
{
	if( sMD5.length() == 0 )
		return 0;

	if( !RAUsers::LocalUser.IsLoggedIn() )
		return 0;

	g_GameTitleDialog.m_sMD5 = sMD5;
	g_GameTitleDialog.m_sEstimatedGameTitle = sEstimatedGameTitle;
	g_GameTitleDialog.m_nReturnedGameID = 0; 

	DialogBox( hInst, MAKEINTRESOURCE(IDD_RA_GAMETITLESEL), hParent, s_GameTitleProc );

	return g_GameTitleDialog.m_nReturnedGameID;
}

//	static
void Dlg_GameTitle::CleanRomName( char* sRomNameRef, unsigned int nLen )
{
	if( strlen( sRomNameRef ) < 2 )
		return;

	unsigned int i = 0;
	int nCharsAdded = 0;
	char* buffer = (char*)malloc( nLen );

	for( i = 0; i < nLen-1; ++i )
	{
		if( sRomNameRef[i] != ' ' || 
			( sRomNameRef[i+1] != ' ' && sRomNameRef[i+1] != '\0' ) )
		{
			//	Attempt to avoid any double-spaces
			buffer[nCharsAdded] = sRomNameRef[i];
			nCharsAdded++;
		}
	}

	buffer[nCharsAdded] = '\0';

	strcpy_s( sRomNameRef, nLen, buffer );

	free( buffer );
	buffer = NULL;
}
