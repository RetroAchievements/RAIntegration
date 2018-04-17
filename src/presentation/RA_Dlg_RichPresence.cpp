#include "RA_Dlg_RichPresence.h"

#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_RichPresence.h"

Dlg_RichPresence g_RichPresenceDialog;

INT_PTR CALLBACK Dlg_RichPresence::RichPresenceDialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	switch ( nMsg )
	{
	case WM_INITDIALOG:
	{
		SendMessage( GetDlgItem( hDlg, IDC_RA_RICHPRESENCERESULTTEXT ), WM_SETFONT, (WPARAM)m_hFont, NULL );

		RestoreWindowPosition( hDlg, "Rich Presence Monitor", true, true );
		return TRUE;
	}

	case WM_TIMER:
	{
		std::wstring sRP = Widen( g_RichPresenceInterpretter.GetRichPresenceString() );
		SetDlgItemTextW( m_hRichPresenceDialog, IDC_RA_RICHPRESENCERESULTTEXT, sRP.c_str() );
		return TRUE;
	}

	case WM_COMMAND:
	{
		switch ( LOWORD(wParam) )
		{
		case IDOK:
		case IDCLOSE:
		case IDCANCEL:
			StopTimer();
			EndDialog( hDlg, true );
			return TRUE;

		default:
			return FALSE;
		}
	}

	case WM_DESTROY:
		StopTimer();
		EndDialog( hDlg, true );
		return FALSE;

	case WM_MOVE:
		RememberWindowPosition(hDlg, "Rich Presence Monitor");
		break;

	case WM_SIZE:
		RememberWindowSize(hDlg, "Rich Presence Monitor");
		break;
	}

	return FALSE;
}

//static 
INT_PTR CALLBACK Dlg_RichPresence::s_RichPresenceDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return g_RichPresenceDialog.RichPresenceDialogProc( hDlg, uMsg, wParam, lParam );
}

Dlg_RichPresence::Dlg_RichPresence()
	: m_hRichPresenceDialog( nullptr ),
	  m_bTimerActive( false )
{
	m_hFont = CreateFont( 15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, NULL );
}

Dlg_RichPresence::~Dlg_RichPresence()
{
	DeleteObject( m_hFont );
}

void Dlg_RichPresence::StartMonitoring()
{
	if (g_RichPresenceInterpretter.Enabled())
	{
		StartTimer();
	}
	else
	{
		StopTimer();
	}
}

void Dlg_RichPresence::StartTimer()
{
	if ( !m_bTimerActive )
	{
		SetTimer( m_hRichPresenceDialog, 1, 1000, NULL );
		m_bTimerActive = true;
	}
}

void Dlg_RichPresence::StopTimer()
{
	if ( m_bTimerActive )
	{
		KillTimer( m_hRichPresenceDialog, 1 );
		m_bTimerActive = false;
	}
}