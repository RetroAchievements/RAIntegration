#include "RA_Dlg_GameTitle.h"

//#include <windowsx.h>
//#include <stdio.h>
//#include <sstream>

#include "RA_Defs.h"
#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_Achievement.h"
#include "RA_AchievementSet.h"
#include "RA_httpthread.h"
#include "RA_GameData.h"


Dlg_GameTitle g_GameTitleDialog;

INT_PTR CALLBACK s_GameTitleProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return g_GameTitleDialog.GameTitleProc( hDlg, uMsg, wParam, lParam );
}

INT_PTR Dlg_GameTitle::GameTitleProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	static bool bUpdatingTextboxTitle = false;

	switch( uMsg )
	{
	case WM_INITDIALOG:
	{
		const HWND hKnownGamesCbo = GetDlgItem( hDlg, IDC_RA_KNOWNGAMES );
		std::string sGameTitleTidy = Dlg_GameTitle::CleanRomName( g_GameTitleDialog.m_sEstimatedGameTitle );
		SetDlgItemText( hDlg, IDC_RA_GAMETITLE, NativeStr( sGameTitleTidy ).c_str() );

		//	Load in the checksum
		SetDlgItemText( hDlg, IDC_RA_CHECKSUM, NativeStr( g_GameTitleDialog.m_sMD5 ).c_str() );

		//	Populate the dropdown
		//	***Do blocking fetch of all game titles.***
		int nSel = ComboBox_AddString( hKnownGamesCbo, Widen( "<New Title>" ).c_str() );
		ComboBox_SetCurSel( hKnownGamesCbo, nSel );

		PostArgs args;
		args[ 'c' ] = std::to_string( g_ConsoleID );

		Document doc;
		if( RAWeb::DoBlockingRequest( RequestGamesList, args, doc ) )
		{
			const Value& Data = doc[ "Response" ];

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

					const GameID nGameID = std::strtoul( iter->name.GetString(), nullptr, 10 );	//	Keys cannot be anything but strings
					const std::string& sTitle = iter->value.GetString();
					m_aGameTitles[ sTitle ] = nGameID;

					iter++;
				}
			}

			{
				std::map<std::string, GameID>::const_iterator iter = m_aGameTitles.begin();
				while( iter != m_aGameTitles.end() )
				{
					const std::string& sTitle = iter->first;

					nSel = ComboBox_AddString( hKnownGamesCbo, Widen( sTitle ).c_str() );

					//	Attempt to find this game and select it by default: case insensitive comparison
					if (sGameTitleTidy.compare(sTitle) == 0)
					{
						ComboBox_SetCurSel(hKnownGamesCbo, nSel);
					}

					iter++;
				}

			}
		}

		return TRUE;
	}
		break;

	case WM_COMMAND:
		switch( LOWORD( wParam ) )
		{
		case IDOK:
		{
			//	Fetch input data:
			TCHAR sSelectedTitleWide[ 512 ];
			ComboBox_GetText( GetDlgItem( hDlg, IDC_RA_KNOWNGAMES ), sSelectedTitleWide, 512 );
			std::string sSelectedTitle = Narrow( sSelectedTitleWide );

			GameID nGameID = 0;
			if( strcmp( sSelectedTitle.c_str(), "<New Title>" ) == 0 )
			{
				//	Add a new title!
				GetDlgItemText( hDlg, IDC_RA_GAMETITLE, sSelectedTitleWide, 512 );
				sSelectedTitle = Narrow(sSelectedTitleWide);
			}
			else
			{
				//	Existing title
				ASSERT( m_aGameTitles.find( std::string( sSelectedTitle ) ) != m_aGameTitles.end() );
				nGameID = m_aGameTitles[ std::string( sSelectedTitle ) ];
			}

			PostArgs args;
			args[ 'u' ] = RAUsers::LocalUser().Username();
			args[ 't' ] = RAUsers::LocalUser().Token();
			args[ 'm' ] = m_sMD5;
			args[ 'i' ] = sSelectedTitle;
			args[ 'c' ] = std::to_string( g_ConsoleID );

			Document doc;
			if( RAWeb::DoBlockingRequest( RequestSubmitNewTitle, args, doc ) && doc.HasMember( "Success" ) && doc[ "Success" ].GetBool() )
			{
				const Value& Response = doc[ "Response" ];

				const GameID nGameID = static_cast<GameID>( Response[ "GameID" ].GetUint() );
				const std::string& sGameTitle = Response[ "GameTitle" ].GetString();

				//	If we're setting the game title here...
				//	 surely we could set the game ID here too?
				g_pCurrentGameData->SetGameTitle( sGameTitle );
				g_pCurrentGameData->SetGameID( nGameID );

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
								NativeStr( std::string( "Could not add new title: " ) + std::string( doc[ "Error" ].GetString() ) ).c_str(),
								TEXT("Errors encountered"),
								MB_OK );
				}
				else
				{
					MessageBox( hDlg,
								TEXT("Cannot contact server!"),
								TEXT("Error in connection"),
								MB_OK );
				}

				return TRUE;
			}
		}

		case IDCANCEL:
			EndDialog( hDlg, TRUE );
			return TRUE;

		case IDC_RA_GAMETITLE:
			switch( HIWORD( wParam ) )
			{
			case EN_CHANGE:
				if( !bUpdatingTextboxTitle )	//	Note: use barrier to prevent automatic switching off.
				{
					//	If the user has started to enter a value, set the upper combo to 'new entry'
					HWND hKnownGamesCbo = GetDlgItem( hDlg, IDC_RA_KNOWNGAMES );
					ComboBox_SetCurSel( hKnownGamesCbo, 0 );
				}
				break;
			}
			break;

		case IDC_RA_KNOWNGAMES:
			switch( HIWORD( wParam ) )
			{
			case CBN_SELCHANGE:
			{
				//	If the user has selected a value, copy this text to the bottom textbox.
				bUpdatingTextboxTitle = TRUE;

				TCHAR sSelectedTitle[ 512 ];
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
		break;

	case WM_CLOSE:
		EndDialog( hDlg, FALSE );
		return TRUE;
	}

	return FALSE;
}

void Dlg_GameTitle::DoModalDialog( HINSTANCE hInst, HWND hParent, std::string& sMD5InOut, std::string& sEstimatedGameTitleInOut, GameID& nGameIDOut )
{
	if( sMD5InOut.length() == 0 )
		return;

	if( !RAUsers::LocalUser().IsLoggedIn() )
		return;

	g_GameTitleDialog.m_sMD5 = sMD5InOut;
	g_GameTitleDialog.m_sEstimatedGameTitle = sEstimatedGameTitleInOut;
	g_GameTitleDialog.m_nReturnedGameID = 0; 

	DialogBox( hInst, MAKEINTRESOURCE(IDD_RA_GAMETITLESEL), hParent, s_GameTitleProc );
	
	sMD5InOut = g_GameTitleDialog.m_sMD5;
	sEstimatedGameTitleInOut = g_GameTitleDialog.m_sEstimatedGameTitle;
	nGameIDOut = g_GameTitleDialog.m_nReturnedGameID;
}

//	static
std::string Dlg_GameTitle::CleanRomName( const std::string& sTryName )
{
	std::stringstream sstr;

	//	Scan through, reform sRomNameRef using all logical characters
	int nCharsAdded = 0;

	for( size_t i = 0; i < sTryName.length(); ++i )
	{
		if( sTryName[ i ] == '\0' )
			break;

		if( __isascii( sTryName[ i ] ) )
			sstr << sTryName[ i ];
	}

	return sstr.str();
}
