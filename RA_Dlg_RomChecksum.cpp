#include "RA_Dlg_RomChecksum.h"

#include "RA_Resource.h"
#include "RA_Core.h"

//static 
BOOL RA_Dlg_RomChecksum::DoModalDialog()
{
	return DialogBox( g_hThisDLLInst, MAKEINTRESOURCE( IDD_RA_ROMCHECKSUM ), g_RAMainWnd, RA_Dlg_RomChecksum::RA_Dlg_RomChecksumProc );
} 

INT_PTR CALLBACK RA_Dlg_RomChecksum::RA_Dlg_RomChecksumProc( HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam )
{
	switch( nMsg )
	{
	case WM_INITDIALOG:
		SetDlgItemText( hDlg, IDC_RA_ROMCHECKSUMTEXT, std::string( std::string( "ROM Checksum: " ) + g_sCurrentROMMD5 ).c_str() );
		break;

	case WM_COMMAND:
		switch( wParam )
		{
		case IDOK:
			EndDialog( hDlg, TRUE );
			return TRUE;
		break;

		case IDC_RA_COPYCHECKSUMCLIPBOARD:
			{
				//	Allocate memory to be managed by the clipboard
				HGLOBAL hMem = GlobalAlloc( GMEM_MOVEABLE, g_sCurrentROMMD5.length() + 1 );
				memcpy( GlobalLock( hMem ), g_sCurrentROMMD5.c_str(), g_sCurrentROMMD5.length() + 1 );
				GlobalUnlock( hMem );

				OpenClipboard( nullptr );
				EmptyClipboard();
				SetClipboardData( CF_TEXT, hMem );
				CloseClipboard();

				//MessageBeep( 0xffffffff );
			}
			return TRUE;
		}
		break;

	case WM_CLOSE:
		EndDialog( hDlg, TRUE );
		return TRUE;
	}

	return FALSE;	//	By default, return false to allow OS to call DefDlgProc itself!
}
