#include "RA_Dlg_Login.h"

#include <stdio.h>

#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_httpthread.h"
#include "RA_PopupWindows.h"


INT_PTR CALLBACK LoginProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		SetDlgItemText( hDlg, IDC_RA_USERNAME, g_LocalUser.m_sUsername );

		if( strlen( g_LocalUser.m_sUsername ) > 2 )
		{
			//set focus	IDC_RA_PASSWORD
			HWND hPass = GetDlgItem( hDlg, IDC_RA_PASSWORD );
			SetFocus( hPass );
			return FALSE;	//	Must return FALSE if setting to a non-default active control. WTF?
		}

		return TRUE;
		break;

	case WM_COMMAND:
		switch(wParam)
		{
		case IDOK:
			{
				char user[64];
				char pass[64];
				ZeroMemory( user, 64 );
				ZeroMemory( pass, 64 );

				BOOL bValid = TRUE;
				bValid &= (GetDlgItemText( hDlg, IDC_RA_USERNAME, user, 64 )!=0);
				bValid &= (GetDlgItemText( hDlg, IDC_RA_PASSWORD, pass, 64 )!=0);

				if( !bValid || strlen( user ) < 0 || strlen( pass ) < 0 )
				{
					MessageBox( NULL, "Username/password not valid! Please check and reenter", "Error!", MB_OK );
					return TRUE;
				}

				//	Plaintext password(!)
				char sRequest[512];
				sprintf_s( sRequest, 512, "u=%s&p=%s", user, pass );

				char sResponse[4096];
				ZeroMemory( sResponse, 4096 );
				char* psResponse = sResponse;
				DWORD nBytesRead = 0;

				char bufferFeedback[4096];

				bValid = DoBlockingHttpPost( "requestlogin.php", sRequest, psResponse, 4096, &nBytesRead );

				if( bValid &&
					strncmp( sResponse, "OK:", 3 ) == 0 )
				{
					bool bRememberLogin = ( IsDlgButtonChecked( hDlg, IDC_RA_SAVEPASSWORD )!=BST_UNCHECKED );

					//	Store valid user!
					char* pBuffer = psResponse+3;
					char* pTok = strtok_s( pBuffer, ":", &pBuffer );
					//pBuffer++;	//NO! Not necessary here (Fuck knows why??)
					unsigned int nPoints = strtol( pBuffer, &pBuffer, 10 );
					pBuffer++;
					unsigned int nMessages = strtol( pBuffer, &pBuffer, 10 );

					sprintf_s( bufferFeedback, "Logged in as %s.", user );
					MessageBox( hDlg, bufferFeedback, "Logged in Successfully!", MB_OK );

					g_LocalUser.Login( user, pTok, bRememberLogin, nPoints, nMessages );
					
					g_PopupWindows.AchievementPopups().SuppressNextDeltaUpdate();

					//	Close this dialog
					EndDialog(hDlg, true);
					return TRUE;
				}
				else
				{
					sprintf_s( bufferFeedback, "Failed!\n\nResponse From Server:\n\n%s", sResponse );
					MessageBox( hDlg, bufferFeedback, "Error logging in!", MB_OK );

					return TRUE;
				}
			}
		case IDCANCEL:
			EndDialog(hDlg, TRUE);
			return TRUE;
			break;
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, true);
		return TRUE;
		break;
	}

	return FALSE;
}
