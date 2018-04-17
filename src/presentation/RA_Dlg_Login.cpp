#include "RA_Dlg_Login.h"

#include <stdio.h>
#include <string>

#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_httpthread.h"
#include "RA_PopupWindows.h"

//static 
BOOL RA_Dlg_Login::DoModalLogin()
{
	return DialogBox( g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_LOGIN), g_RAMainWnd, RA_Dlg_Login::RA_Dlg_LoginProc );
} 

INT_PTR CALLBACK RA_Dlg_Login::RA_Dlg_LoginProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
	case WM_INITDIALOG:

		SetDlgItemText( hDlg, IDC_RA_USERNAME, NativeStr( RAUsers::LocalUser().Username() ).c_str() );
		if( RAUsers::LocalUser().Username().length() > 2 )
		{
			HWND hPass = GetDlgItem( hDlg, IDC_RA_PASSWORD );
			SetFocus( hPass );
			return FALSE;	//	Must return FALSE if setting to a non-default active control.
		}
		else
		{
			return TRUE;
		}

	case WM_COMMAND:
		switch( wParam )
		{
		case IDOK:
			{
				BOOL bValid = TRUE;
				TCHAR sUserEntry[ 64 ];
				bValid &= ( GetDlgItemText( hDlg, IDC_RA_USERNAME, sUserEntry, 64 ) > 0 );
				TCHAR sPassEntry[ 64 ];
				bValid &= ( GetDlgItemText( hDlg, IDC_RA_PASSWORD, sPassEntry, 64 ) > 0 );

				if( !bValid || lstrlen( sUserEntry ) < 0 || lstrlen( sPassEntry ) < 0 )
				{
					MessageBox( nullptr, TEXT("Username/password not valid! Please check and reenter"), TEXT("Error!"), MB_OK );
					return TRUE;
				}

				PostArgs args;
				args[ 'u' ] = Narrow( sUserEntry );
				args[ 'p' ] = Narrow( sPassEntry );		//	Plaintext password(!)
				
				Document doc;
				if( RAWeb::DoBlockingRequest( RequestLogin, args, doc ) )
				{
					std::string sResponse;
					std::string sResponseTitle;

					if( doc[ "Success" ].GetBool() )
					{
						const std::string& sUser = doc[ "User" ].GetString();
						const std::string& sToken = doc[ "Token" ].GetString();
						const unsigned int nPoints = doc[ "Score" ].GetUint();
						const unsigned int nUnreadMessages = doc[ "Messages" ].GetUint();

						bool bRememberLogin = ( IsDlgButtonChecked( hDlg, IDC_RA_SAVEPASSWORD ) != BST_UNCHECKED );

						RAUsers::LocalUser().ProcessSuccessfulLogin( sUser, sToken, nPoints, nUnreadMessages, bRememberLogin );

						sResponse = "Logged in as " + sUser + ".";
						sResponseTitle = "Logged in Successfully!";

						//g_PopupWindows.AchievementPopups().SuppressNextDeltaUpdate();
					}
					else
					{
						sResponse = std::string( "Failed!\r\n"
												 "Response From Server:\r\n" ) +
									doc[ "Error" ].GetString();
						sResponseTitle = "Error logging in!";
					}

					MessageBox( hDlg, NativeStr( sResponse ).c_str(), NativeStr( sResponseTitle ).c_str(), MB_OK );

					//	If we are now logged in
					if( RAUsers::LocalUser().IsLoggedIn() )
					{
						//	Close this dialog
						EndDialog( hDlg, TRUE );
					}

					return TRUE;
				}
				else
				{
					if (!doc.HasParseError() && doc.HasMember("Error"))
					{
						MessageBox(hDlg, NativeStr(std::string("Server error: ") + std::string(doc["Error"].GetString())).c_str(), TEXT("Error!"), MB_OK);
					}
					else
					{
						MessageBox(hDlg, TEXT("Unknown error connecting... please try again!"), TEXT("Error!"), MB_OK);
					}

					return TRUE;	//	==Handled
				}
			}
		break;

		case IDCANCEL:
			EndDialog( hDlg, TRUE );
			return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog( hDlg, TRUE );
		return TRUE;
	}

	return FALSE;
}
