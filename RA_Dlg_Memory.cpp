#include "RA_Dlg_Memory.h"

#include "RA_Achievement.h"
#include "RA_AchievementSet.h"
#include "RA_CodeNotes.h"
#include "RA_Core.h"
#include "RA_httpthread.h"
#include "RA_MemManager.h"
#include "RA_Resource.h"
#include "RA_User.h"


#ifndef ID_OK
#define ID_OK                           1024
#endif
#ifndef ID_CANCEL
#define ID_CANCEL                       1025
#endif

namespace
{
	const size_t MIN_RESULTS_TO_DUMP = 500;
	
	const char* COMP_STR[] = {
		{ "EQUAL" },
		{ "LESS THAN" },
		{ "LESS THAN/EQUAL" },
		{ "GREATER THAN" },
		{ "GREATER THAN/EQUAL" },
		{ "NOT EQUAL" } };
}

Dlg_Memory g_MemoryDialog;

//static 
CodeNotes Dlg_Memory::m_CodeNotes;
HWND Dlg_Memory::m_hWnd = nullptr;

HFONT MemoryViewerControl::m_hViewerFont = nullptr;
SIZE MemoryViewerControl::m_szFontSize;
unsigned int MemoryViewerControl::m_nDataStartXOffset = 0;
unsigned int MemoryViewerControl::m_nAddressOffset = 0;
unsigned int MemoryViewerControl::m_nDataSize = 0;
unsigned int MemoryViewerControl::m_nEditAddress = 0;
unsigned int MemoryViewerControl::m_nEditNibble = 0;
bool MemoryViewerControl::m_bHasCaret = 0;
unsigned int MemoryViewerControl::m_nCaretWidth = 0;
unsigned int MemoryViewerControl::m_nCaretHeight = 0;
unsigned int MemoryViewerControl::m_nDisplayedLines = 0;
unsigned short MemoryViewerControl::m_nActiveMemBank = 0;


std::string ByteAddressToString( ByteAddress nAddr )
{
	static char buffer[16];
	sprintf_s( buffer, "0x%06x", nAddr );
	return std::string( buffer );
}

INT_PTR CALLBACK MemoryViewerControl::s_MemoryDrawProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
	case WM_NCCREATE:
	case WM_NCDESTROY:
		return TRUE;

	case WM_CREATE:
		return TRUE;

	case WM_PAINT:
		RenderMemViewer( hDlg );
		return 0;

	case WM_ERASEBKGND:
		return TRUE;

	case WM_LBUTTONUP:
		OnClick( { GET_X_LPARAM( lParam ), GET_Y_LPARAM( lParam ) } );
		return FALSE;

	case WM_KEYDOWN:
		return( !OnKeyDown( static_cast<UINT>( LOWORD( wParam ) ) ) );

	case WM_CHAR:
		return( !OnEditInput( static_cast<UINT>( LOWORD( wParam ) ) ) );
	}

	return DefWindowProc( hDlg, uMsg, wParam, lParam );
	//return FALSE;
}

bool MemoryViewerControl::OnKeyDown( UINT nChar ) 
{
	int maxNibble = 0;
	if( m_nDataSize == 0 )
		maxNibble = 1;
	else if( m_nDataSize == 1 )
		maxNibble = 3;
	else
		maxNibble = 7;

	bool bShiftHeld = ( GetKeyState(VK_SHIFT) & 0x80000000 ) == 0x80000000;

	switch( nChar )
	{
	case VK_RIGHT:
		if( bShiftHeld )
			moveAddress( ( maxNibble + 1 ) >> 1, 0 );
		else
			moveAddress( 0, 1 );
		return true;

	case VK_LEFT:
		if( bShiftHeld )
			moveAddress( -( ( maxNibble + 1 ) >> 1 ), 0 );
		else
			moveAddress( 0, -1 );
		return true;

	case VK_DOWN:
		moveAddress( 16, 0 );
		return true;

	case VK_UP:
		moveAddress( -16, 0 );
		return true;
	}

	return false;
}

void MemoryViewerControl::moveAddress( int offset, int nibbleOff )
{
	unsigned int maxNibble = 0;
	if(m_nDataSize == 0)
		maxNibble = 1;
	else if(m_nDataSize == 1)
		maxNibble = 3;
	else
		maxNibble = 7;

  if(offset == 0)
  {
    if(nibbleOff == -1)
	{
      m_nEditNibble--;
      if(m_nEditNibble == -1)
	  {
        m_nEditAddress -= (maxNibble + 1) >> 1;
        m_nEditNibble = maxNibble;
      }
      if(m_nAddressOffset == 0 && (m_nEditAddress >= (unsigned int)(m_nDisplayedLines<<4))) {
        m_nEditAddress = 0;
        m_nEditNibble = 0;
		MessageBeep((UINT)-1);
      }
      if(m_nEditAddress < m_nAddressOffset)
        setAddress(m_nAddressOffset - 16);
    } else {
      m_nEditNibble++;
      if(m_nEditNibble > maxNibble) {
        m_nEditNibble = 0;
        m_nEditAddress += (maxNibble + 1) >> 1;
      }
      if(m_nEditAddress < m_nAddressOffset) {
        m_nEditAddress -= (maxNibble + 1) >> 1;
        m_nEditNibble = maxNibble;
	  MessageBeep((UINT)-1);
      }
      if(m_nEditAddress >= (m_nAddressOffset+(m_nDisplayedLines<<4)))
        setAddress(m_nAddressOffset+16);
    }
  } 
  else
  {
    m_nEditAddress += offset;
    if(offset < 0 && m_nEditAddress > (m_nAddressOffset-1+(m_nDisplayedLines<<4)))
	{
      m_nEditAddress -= offset;
	  MessageBeep((UINT)-1);
      return;
    }
    if(offset > 0 && (m_nEditAddress < m_nAddressOffset)) 
	{
      m_nEditAddress -= offset;
	  MessageBeep((UINT)-1);
      return;
    }
    if( m_nEditAddress >= g_MemManager.TotalBankSize() )
	{
      m_nEditAddress -= offset;
	  MessageBeep((UINT)-1);
      return;
    }
    if(m_nEditAddress < m_nAddressOffset)
	{
      if(offset & 15)
        setAddress((m_nAddressOffset+offset-16) & ~15);
      else
        setAddress(m_nAddressOffset+offset);
    }
	else if(m_nEditAddress > (m_nAddressOffset - 1 + (m_nDisplayedLines<<4)))
	{
      if(offset & 15)
        setAddress((m_nAddressOffset+offset+16) & ~15);
      else
        setAddress(m_nAddressOffset+offset);
    }
  }

	SetCaretPos();
}

void MemoryViewerControl::setAddress( unsigned int address )
{
	m_nAddressOffset = address;
	g_MemoryDialog.SetWatchingAddress( address );

	SetCaretPos();
	Invalidate();
}

void MemoryViewerControl::Invalidate()
{
	HWND hOurDlg = GetDlgItem( g_MemoryDialog.GetHWND(), IDC_RA_MEMTEXTVIEWER );
	if( hOurDlg != NULL )
		InvalidateRect( hOurDlg, NULL, TRUE );
}

void MemoryViewerControl::editData( unsigned int nByteAddress, bool bLowerNibble, unsigned int nNewVal )
{
	//	Immediately invalidate all submissions.
	g_bRAMTamperedWith = true;

	if( bLowerNibble )
	{
		//	We're submitting a new lower nibble:
		//	Fetch existing upper nibble,
		unsigned int nVal = ( g_MemManager.ActiveBankRAMByteRead( nByteAddress ) >> 4 ) << 4;
		//	Merge in given (lower nibble) value,
		nVal |= nNewVal;
		//	Write value:
		g_MemManager.ActiveBankRAMByteWrite( nByteAddress, nVal );
	}
	else
	{
		//	We're submitting a new upper nibble:
		//	Fetch existing lower nibble,
		unsigned int nVal = g_MemManager.ActiveBankRAMByteRead( nByteAddress ) & 0xf;
		//	Merge in given value at upper nibble
		nVal |= ( nNewVal << 4 );
		//	Write value:
		g_MemManager.ActiveBankRAMByteWrite( nByteAddress, nVal );
	}
}

bool MemoryViewerControl::OnEditInput( UINT c )
{
	if( g_MemManager.NumMemoryBanks() == 0 )
		return false;

	if( c > 255 || !RA_GameIsActive() )
	{
		MessageBeep((UINT)-1);
		return false;
	}

	c = tolower(c);

	unsigned int value = 256;

	if(c >= 'a' && c <= 'f')
		value = 10 + (c - 'a');
	else if(c >= '0' && c <= '9')
		value = (c - '0');

	int maxNibble = 0;
	if( m_nDataSize == 0 )
		maxNibble = 1;
	else if( m_nDataSize == 1 )
		maxNibble = 3;
	else
		maxNibble = 7;

    if( value != 256 )
	{
		//value <<= 4*(maxNibble-m_nEditNibble);
		//unsigned int mask = ~(15 << 4*(maxNibble - m_nEditNibble));

		bool bLowerNibble = ( m_nEditNibble % 2 == 1 );
		unsigned int nByteAddress = m_nEditAddress;
		
		if( m_nDataSize == 0 )
		{
			//	8 bit
			//nByteAddress = m_nEditAddress;
		}
		else if( m_nDataSize == 1 )
		{
			//	16 bit
			nByteAddress += ( 1 - (m_nEditNibble>>1) );
		}
		else
		{
			//	32 bit
			nByteAddress += ( 3 - (m_nEditNibble>>1) );
		}

		editData( nByteAddress, bLowerNibble, value );

		moveAddress(0, 1);
		Invalidate();
    }

	return true;
}

void MemoryViewerControl::createEditCaret(int w, int h )
{
	if(!m_bHasCaret || m_nCaretWidth != w || m_nCaretHeight != h) {
		m_bHasCaret = true;
		m_nCaretWidth = w;
		m_nCaretHeight = h;
		::CreateCaret( GetDlgItem( g_MemoryDialog.GetHWND(), IDC_RA_MEMTEXTVIEWER ), (HBITMAP)0, w, h);
	}
}

void MemoryViewerControl::destroyEditCaret()
{
	m_bHasCaret = false;
	DestroyCaret();
}

void MemoryViewerControl::SetCaretPos()
{
	if( g_MemManager.NumMemoryBanks() == 0 )
		return;

	HWND hOurDlg = GetDlgItem( g_MemoryDialog.GetHWND(), IDC_RA_MEMTEXTVIEWER );
	if(GetFocus() != hOurDlg)
	{
		destroyEditCaret();
		return;
	}

	g_MemoryDialog.SetWatchingAddress( m_nEditAddress );

	unsigned int nTopLeft = m_nAddressOffset - 0x40;
	if( g_MemManager.TotalBankSize() > 0 )
		nTopLeft %= g_MemManager.TotalBankSize();
	else
		nTopLeft %= 0x10000;

	//	Doesn't work if we have wrap-around...
	//if(m_nEditAddress < nTopLeft || m_nEditAddress > (nTopLeft-1 + (m_nDisplayedLines<<4)))
	//{
	//	destroyEditCaret();
	//	return;
	//}

	int subAddress = (m_nEditAddress - m_nAddressOffset);

	int topLeft = m_nAddressOffset - 0x40;
	//const int nYSpacing = ( m_nEditAddress-topLeft ) >> 4;
	const int nYSpacing = 4;

	int x = 3 + ( 10*m_szFontSize.cx ) + ( m_nEditNibble*m_szFontSize.cx );
	int y = 3 + ( nYSpacing*m_szFontSize.cy );

	y += m_szFontSize.cy;	//	Account for header

	switch( m_nDataSize )
	{
	case 0:
		x += 3*m_szFontSize.cx*(subAddress & 15);
		break;
	case 1:
		x += 5*m_szFontSize.cx*((subAddress>>1) & 7);
		break;
	case 2:
		x += 9*m_szFontSize.cx*((subAddress>>2) & 3);
		break;
	}

	RECT r;
	GetClientRect(hOurDlg, &r);
	r.right -= 3;
	if(x >= r.right) {
		destroyEditCaret();
		return;
	}
	int w = m_szFontSize.cx;
	if((x+m_szFontSize.cx)>=r.right)
		w = r.right - x;
	createEditCaret(w, m_szFontSize.cy);
	::SetCaretPos(x, y);
	ShowCaret( hOurDlg );
}

void MemoryViewerControl::OnClick( POINT point )
{
	if( g_MemManager.NumMemoryBanks() == 0 )
		return;

	HWND hOurDlg = GetDlgItem( g_MemoryDialog.GetHWND(), IDC_RA_MEMTEXTVIEWER );

	int x = point.x;
	int y = point.y - m_szFontSize.cy;	//	Adjust for header
	int line = ((y-3)/m_szFontSize.cy);	

	if( line == -1 )
		return;	//	clicked on header

	int rowLengthPx = m_nDataStartXOffset;
	int inc = 1;
	int sub = 3*m_szFontSize.cx;

	switch( m_nDataSize )
	{
		case 0:
			rowLengthPx += 47*m_szFontSize.cx;
			inc = 1;					//	increment mem offset by 1 each subset
			sub = 3*m_szFontSize.cx;	//	2 char set plus one char space
			break;
		case 1:
			rowLengthPx += 39*m_szFontSize.cx;
			inc = 2;					//	increment mem offset by 2 each subset
			sub = 5*m_szFontSize.cx;	//	4 char set plus one char space
			break;
		case 2:
			rowLengthPx += 35*m_szFontSize.cx;
			inc = 4;					//	increment mem offset by 4 each subset
			sub = 9*m_szFontSize.cx;	//	8 char set plus one char space
			break;
	}

	unsigned int nTopLeft = m_nAddressOffset - 0x40;//( m_nAddressOffset - ( m_nAddressOffset % 0xf ) );

	m_nEditAddress = ( nTopLeft + (line<<4) );

	if( g_MemManager.TotalBankSize() > 0 )
		m_nEditAddress %= g_MemManager.TotalBankSize();
	else
		m_nEditAddress %= 0x10000;

	if( x >= (int)m_nDataStartXOffset && x < rowLengthPx )
	{
		x -= m_nDataStartXOffset;
		m_nEditNibble = 0;

		while(x > 0)
		{
			//	Adjust x by one subset til we find out the correct offset:
			x -= sub;
			if( x >= 0 )
				m_nEditAddress += inc;
			else
				m_nEditNibble = (x + sub)/m_szFontSize.cx;
		}

	} 
	else 
	{
		return;
	}

	unsigned int maxNibble = 0;
	if(m_nDataSize == 0)
		maxNibble = 1;
	else if(m_nDataSize == 1)
		maxNibble = 3;
	else
		maxNibble = 7;

	if(m_nEditNibble > maxNibble)
		m_nEditNibble = maxNibble;
	SetFocus( hOurDlg );
	SetCaretPos();
}

void MemoryViewerControl::RenderMemViewer( HWND hTarget )
{
	PAINTSTRUCT ps;
	HDC dc = BeginPaint( hTarget, &ps );

	HDC hMemDC = CreateCompatibleDC( dc );

	RECT rect;
	GetClientRect( hTarget, &rect );
	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top - 6;

	//	Pick font
	if( m_hViewerFont == NULL )
		m_hViewerFont = (HFONT)GetStockObject( SYSTEM_FIXED_FONT );
	HGDIOBJ hOldFont = SelectObject( hMemDC, m_hViewerFont );

	HBITMAP hBitmap = CreateCompatibleBitmap( dc, w, rect.bottom - rect.top );
	HGDIOBJ hOldBitmap = SelectObject( hMemDC, hBitmap );

	GetTextExtentPoint32( hMemDC, L"0", 1, &m_szFontSize );

	//	Fill white:
	HBRUSH hBrush = (HBRUSH)GetStockObject( WHITE_BRUSH );
	FillRect( hMemDC, &rect, hBrush );
	DrawEdge( hMemDC, &rect, EDGE_ETCHED, BF_RECT );

	BOOL bGroup32 = ( IsDlgButtonChecked( g_MemoryDialog.GetHWND(), IDC_RA_MEMVIEW32BIT ) == BST_CHECKED );
	BOOL bGroup16 = bGroup32 | ( IsDlgButtonChecked( g_MemoryDialog.GetHWND(), IDC_RA_MEMVIEW16BIT ) == BST_CHECKED );

	m_nDataSize = ( bGroup32 ? 2 : ( bGroup16 ? 1 : 0 ) );

	const char* sHeader = bGroup32 ? "          0        4        8        c"
		: bGroup16 ?				 "          0    2    4    6    8    a    c    e"
		:							 "          0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f";

	int lines = h / m_szFontSize.cy;
	lines -= 1;	//	Watch out for header
	m_nDisplayedLines = lines;

	//	Infer the offset from the watcher window
	wchar_t sCurrentAddrWide[ 16 ];
	ComboBox_GetText( GetDlgItem( g_MemoryDialog.GetHWND(), IDC_RA_WATCHING ), sCurrentAddrWide, 16 );
	unsigned int nWatchedAddress = strtol( Narrow( sCurrentAddrWide ).c_str(), nullptr, 16 );
	m_nAddressOffset = ( nWatchedAddress - ( nWatchedAddress & 0xf ) );

	int addr = m_nAddressOffset;
	addr -= ( 0x40 );	//	Offset will be this quantity (push up four lines)...
	//addr &= 0xffffff;	//	This should be mem size!!!
	if( m_nActiveMemBank < g_MemManager.NumMemoryBanks() )				///CHECK THIS ITS DODGY ##SD
		addr &= ( g_MemManager.TotalBankSize() - 1 );

	int line = 0;

	SetTextColor( hMemDC, RGB( 0, 0, 0 ) );

	unsigned int data[ 32 ];
	ZeroMemory( data, 32 * sizeof( unsigned int ) );

	RECT r;
	r.top = 3;
	r.left = 3;
	r.bottom = r.top + m_szFontSize.cy;
	r.right = rect.right - 3;

	//	Draw header:
	DrawText( hMemDC, Widen( sHeader ).c_str(), strlen( sHeader ), &r, DT_TOP | DT_LEFT | DT_NOPREFIX );

	//	Adjust for header:
	r.top += m_szFontSize.cy;
	r.bottom += m_szFontSize.cy;

	for( int i = 0; i < lines; i++ )
	{
		//char sLocalAddr[64];
		char buffer[ 4096 ];
		sprintf_s( buffer, 4096, "0x%06x", addr );

		DrawText( hMemDC, Widen( buffer ).c_str(), strlen( buffer ), &r, DT_TOP | DT_LEFT | DT_NOPREFIX );

		r.left += 10 * m_szFontSize.cx;
		m_nDataStartXOffset = r.left;

		for( int j = 0; j < 16; ++j )
		{
			if( m_nActiveMemBank < g_MemManager.NumMemoryBanks() )
			{
				if( static_cast<size_t>( addr + j ) < g_MemManager.TotalBankSize() )
					data[ j ] = g_MemManager.ActiveBankRAMByteRead( addr + j );
			}
		}
		//readData(addr, 16, data);

		if( m_nDataSize == 0 )	//	8-bit
		{
			for( int j = 0; j < 16; j++ )
			{
				const CodeNotes::CodeNoteObj* pSavedNote = g_MemoryDialog.Notes().FindCodeNote( addr + j );

				if( addr + j == nWatchedAddress )
					SetTextColor( hMemDC, RGB( 255, 0, 0 ) );
				else if( pSavedNote != NULL )
					SetTextColor( hMemDC, RGB( 0, 0, 255 ) );
				else
					SetTextColor( hMemDC, RGB( 0, 0, 0 ) );

				sprintf_s( buffer, 4096, "%02x", data[ j ] );
				DrawText( hMemDC, Widen( buffer ).c_str(), strlen( buffer ), &r, DT_TOP | DT_LEFT | DT_NOPREFIX );
				r.left += 3 * m_szFontSize.cx;
			}
		}
		else if( m_nDataSize == 1 )	//	16-bit
		{
			for( int j = 0; j < 16; j += 2 )
			{
				const CodeNotes::CodeNoteObj* pSavedNote = g_MemoryDialog.Notes().FindCodeNote( addr + j );

				if( ( ( addr + j ) - ( addr + j ) % 2 ) == nWatchedAddress )
					SetTextColor( hMemDC, RGB( 255, 0, 0 ) );
				else if( pSavedNote != NULL )
					SetTextColor( hMemDC, RGB( 0, 0, 255 ) );
				else
					SetTextColor( hMemDC, RGB( 0, 0, 0 ) );

				sprintf_s( buffer, 4096, "%04x", data[ j ] | data[ j + 1 ] << 8 );
				DrawText( hMemDC, Widen( buffer ).c_str(), strlen( buffer ), &r, DT_TOP | DT_LEFT | DT_NOPREFIX );
				r.left += 5 * m_szFontSize.cx;
			}
		}
		else if( m_nDataSize == 2 )	//	32-bit
		{
			for( int j = 0; j < 16; j += 4 )
			{
				if( ( ( addr + j ) - ( addr + j ) % 4 ) == nWatchedAddress )
					SetTextColor( hMemDC, RGB( 255, 0, 0 ) );
				else
					SetTextColor( hMemDC, RGB( 0, 0, 0 ) );

				sprintf_s( buffer, 4096, "%08x", data[ j ] | data[ j + 1 ] << 8 | data[ j + 2 ] << 16 | data[ j + 3 ] << 24 );
				DrawText( hMemDC, Widen( buffer ).c_str(), strlen( buffer ), &r, DT_TOP | DT_LEFT | DT_NOPREFIX );
				r.left += 9 * m_szFontSize.cx;
			}
		}

		SetTextColor( hMemDC, RGB( 0, 0, 0 ) );

		line = r.left;
		r.left += m_szFontSize.cx;

		addr += 16;

		if( m_nActiveMemBank < g_MemManager.NumMemoryBanks() )
			addr &= ( g_MemManager.TotalBankSize() - 1 );

		r.top += m_szFontSize.cy;
		r.bottom += m_szFontSize.cy;
		r.left = 3;
	}

	{
		HPEN hPen = CreatePen( PS_SOLID, 1, RGB( 0, 0, 0 ) );
		HGDIOBJ hOldPen = SelectObject( hMemDC, hPen );

		MoveToEx( hMemDC, 3 + m_szFontSize.cx * 9, 3 + m_szFontSize.cy, NULL );
		LineTo( hMemDC, 3 + m_szFontSize.cx * 9, 3 + ( ( m_nDisplayedLines + 1 ) * m_szFontSize.cy ) );

		SelectObject( hMemDC, hOldPen );
		DeleteObject( hPen );
	}

	SelectObject( hMemDC, hOldFont );

	BitBlt( dc, 0, 0, w, rect.bottom - rect.top, hMemDC, 0, 0, SRCCOPY );

	SelectObject( hMemDC, hOldBitmap );
	DeleteDC( hMemDC );
	DeleteObject( hBitmap );

	EndPaint( hTarget, &ps );
}

void Dlg_Memory::Init()
{
	WNDCLASS wc;
	ZeroMemory( &wc, sizeof( wc ) );
	wc.style = CS_PARENTDC | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
	wc.lpfnWndProc = (WNDPROC)MemoryViewerControl::s_MemoryDrawProc;
	wc.hInstance = g_hThisDLLInst;
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)GetStockObject( WHITE_BRUSH );
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"MemoryViewerControl";

	RegisterClass( &wc );
}

//static
INT_PTR CALLBACK Dlg_Memory::s_MemoryProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return g_MemoryDialog.MemoryProc( hDlg, uMsg, wParam, lParam );
}

void Dlg_Memory::ClearLogOutput()
{
	ListBox_ResetContent( GetDlgItem( m_hWnd, IDC_RA_MEM_LIST ) );
}

void Dlg_Memory::AddLogLine( const std::string& sNextLine )
{
	ListBox_AddString( GetDlgItem( m_hWnd, IDC_RA_MEM_LIST ), Widen( sNextLine ).c_str() );
}


INT_PTR Dlg_Memory::MemoryProc( HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam )
{
	switch( nMsg )
	{
	case WM_TIMER:
		{
			SetDlgItemText( hDlg, IDC_RA_MEMBITS_TITLE, L"" );
			SetDlgItemText( hDlg, IDC_RA_MEMBITS, L"" );

			//	Force this through to invalidate mem viewer:
			Invalidate();

			if( ( g_MemManager.NumMemoryBanks() == 0 ) || ( g_MemManager.TotalBankSize() == 0 ) )
				return FALSE;

			bool bView8Bit = ( SendDlgItemMessage( hDlg, IDC_RA_MEMVIEW8BIT, BM_GETCHECK, 0, 0 ) == BST_CHECKED );
			if( !bView8Bit )
				return FALSE;
			
			wchar_t bufferWide[ 1024 ];
			GetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_WATCHING, bufferWide, 1024 );
			const std::string buffer = Narrow( bufferWide );
			if( ( buffer.length() >= 3 ) && buffer[ 0 ] == '0' && buffer[ 1 ] == 'x' )
			{
				ByteAddress nAddr = static_cast<ByteAddress>( strtol( buffer.c_str() + 2, nullptr, 16 ) );
				unsigned char nVal = g_MemManager.ActiveBankRAMByteRead( nAddr );

				wchar_t sDesc[ 32 ];
				wsprintf( sDesc,	L"      %d %d %d %d %d %d %d %d",
					static_cast<int>( ( nVal & ( 1 << 7 ) ) != 0 ),
					static_cast<int>( ( nVal & ( 1 << 6 ) ) != 0 ),
					static_cast<int>( ( nVal & ( 1 << 5 ) ) != 0 ),
					static_cast<int>( ( nVal & ( 1 << 4 ) ) != 0 ),
					static_cast<int>( ( nVal & ( 1 << 3 ) ) != 0 ),
					static_cast<int>( ( nVal & ( 1 << 2 ) ) != 0 ),
					static_cast<int>( ( nVal & ( 1 << 1 ) ) != 0 ),
					static_cast<int>( ( nVal & ( 1 << 0 ) ) != 0 ) );

				SetDlgItemText( hDlg, IDC_RA_MEMBITS_TITLE, L"Bits: 7 6 5 4 3 2 1 0" );
				SetDlgItemText( hDlg, IDC_RA_MEMBITS, sDesc );
			}
		}
		return FALSE;

	case WM_INITDIALOG:
		{
			g_MemoryDialog.m_hWnd = hDlg;

 			RECT rc;
 			GetWindowRect( g_RAMainWnd, &rc );
			SetWindowPos( hDlg, NULL, rc.right, rc.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW );

			CheckDlgButton( hDlg, IDC_RA_CBO_GIVENVAL, BST_UNCHECKED );
			CheckDlgButton( hDlg, IDC_RA_CBO_LASTKNOWNVAL, BST_CHECKED );
			EnableWindow( GetDlgItem( hDlg, IDC_RA_TESTVAL ), FALSE );

			for( size_t i = 0; i < NumComparisonTypes; ++i )
				ComboBox_AddString( GetDlgItem( hDlg, IDC_RA_CBO_CMPTYPE ), Widen( COMPARISONTYPE_STR[ i ] ).c_str() );

			ComboBox_SetCurSel( GetDlgItem( hDlg, IDC_RA_CBO_CMPTYPE ), 0 );

			//	Every 16ms, update timer proc
			SetTimer(hDlg, 1, 16, (TIMERPROC)s_MemoryProc);

			EnableWindow( GetDlgItem( hDlg, IDC_RA_DOTEST ), g_MemManager.NumCandidates() > 0 );

			SetDlgItemText( hDlg, IDC_RA_WATCHING, L"0x0000" );

			SendMessage( GetDlgItem( hDlg, IDC_RA_MEMBITS ), WM_SETFONT, reinterpret_cast<WPARAM>( GetStockObject( SYSTEM_FIXED_FONT ) ), TRUE );
			SendMessage( GetDlgItem( hDlg, IDC_RA_MEMBITS_TITLE ), WM_SETFONT, reinterpret_cast<WPARAM>( GetStockObject( SYSTEM_FIXED_FONT ) ), TRUE );

			//	8-bit by default:
			CheckDlgButton( hDlg, IDC_RA_CBO_4BIT, BST_UNCHECKED );
			CheckDlgButton( hDlg, IDC_RA_CBO_8BIT, BST_CHECKED );
			CheckDlgButton( hDlg, IDC_RA_CBO_16BIT, BST_UNCHECKED );
			CheckDlgButton( hDlg, IDC_RA_CBO_32BIT, BST_UNCHECKED );

			CheckDlgButton( hDlg, IDC_RA_MEMVIEW8BIT, BST_CHECKED );
			CheckDlgButton( hDlg, IDC_RA_MEMVIEW16BIT, BST_UNCHECKED );
			CheckDlgButton( hDlg, IDC_RA_MEMVIEW32BIT, BST_UNCHECKED );

			MemoryProc( hDlg, WM_COMMAND, IDC_RA_CBO_8BIT, 0 );		//	Imitate a buttonpress of '8-bit'
			g_MemoryDialog.OnLoad_NewRom();

			//	Fetch banks
			ClearBanks();
			std::vector<size_t> bankIDs = g_MemManager.GetBankIDs();
			for( size_t i = 0; i < bankIDs.size(); ++i )
				AddBank( bankIDs[ i ] );

			return TRUE;
		}

	case WM_COMMAND:
	{
		switch( LOWORD( wParam ) )
		{
			case IDC_RA_DOTEST:
			{
				if( g_MemManager.NumMemoryBanks() == 0 )
					return TRUE;	//	Ignored

				if( g_MemManager.TotalBankSize() == 0 )
					return TRUE;	//	Handled

				ComparisonType nCmpType = static_cast<ComparisonType>( ComboBox_GetCurSel( GetDlgItem( hDlg, IDC_RA_CBO_CMPTYPE ) ) );

				ClearLogOutput();

				unsigned int nValueQuery = 0;

				{
					wchar_t bufferWide[ 1024 ];
					if( GetDlgItemText( hDlg, IDC_RA_TESTVAL, bufferWide, 1024 ) )
					{
						std::string buffer = Narrow( bufferWide );
						//	Read hex or dec
						if( buffer[ 0 ] == '0' && buffer[ 1 ] == 'x' )
							nValueQuery = static_cast<unsigned int>( std::strtoul( buffer.c_str() + 2, NULL, 16 ) );
						else
							nValueQuery = static_cast<unsigned int>( std::strtoul( buffer.c_str(), NULL, 10 ) );
					}
				}


				std::string str(
					"Filtering for " + std::string( COMP_STR[ nCmpType ] ) +
					( ( g_MemManager.UseLastKnownValue() ) ? " last known value..." : std::to_string( nValueQuery ) ) );

				AddLogLine( str );

				//////////////////////////////////////////////////////////////////////////
				bool bResultsFound = false;
				size_t nResults = g_MemManager.Compare( nCmpType, nValueQuery, bResultsFound );
				//////////////////////////////////////////////////////////////////////////


				std::stringstream sstr;
				if( !bResultsFound )
					sstr << "Found *ZERO* matches: restoring old results set ( " << nResults << " results )!";
				else
					sstr << "Found " << nResults << " matches!";

				AddLogLine( sstr.str() );

				if( nResults < MIN_RESULTS_TO_DUMP )
				{
					for( size_t i = 0; i < nResults; ++i )
					{
						const DWORD nCandidateAddr = g_MemManager.ValidMemAddrFound( i );

						char buffer[ 1024 ];

						char sMem[ 16 ];
						sprintf_s( sMem, 16, "0x%06x", nCandidateAddr );

						if( g_MemManager.MemoryComparisonSize() == ThirtyTwoBit )
						{
							sprintf_s( buffer, 1024, "%s: 0x%04x", sMem, g_MemManager.ActiveBankRAMByteRead( nCandidateAddr ) | ( g_MemManager.ActiveBankRAMByteRead( nCandidateAddr + 1 ) << 8 ) );
						}
						if( g_MemManager.MemoryComparisonSize() == SixteenBit )
						{
							sprintf_s( buffer, 1024, "%s: 0x%04x", sMem, g_MemManager.ActiveBankRAMByteRead( nCandidateAddr ) | ( g_MemManager.ActiveBankRAMByteRead( nCandidateAddr + 1 ) << 8 ) );
						}
						else if( g_MemManager.MemoryComparisonSize() == EightBit )
						{
							sprintf_s( buffer, 1024, "%s: 0x%02x", sMem, g_MemManager.ActiveBankRAMByteRead( nCandidateAddr ) );
						}
						else if( g_MemManager.MemoryComparisonSize() == Nibble_Lower || g_MemManager.MemoryComparisonSize() == Nibble_Upper )
						{
							if( g_MemManager.GetCandidate( i ).m_bUpperNibble )
								sprintf_s( buffer, 1024, "%s: 0x%01x", sMem, ( g_MemManager.ActiveBankRAMByteRead( nCandidateAddr ) >> 4 ) & 0xf );
							else
								sprintf_s( buffer, 1024, "%s: 0x%01x", sMem, g_MemManager.ActiveBankRAMByteRead( nCandidateAddr ) & 0xf );
						}
						else
						{
							//?
							buffer[ 0 ] = '\0';
						}

						//	Append memory note if we can find one:
						const CodeNotes::CodeNoteObj* pSavedNote = m_CodeNotes.FindCodeNote( nCandidateAddr );
						if( ( pSavedNote != NULL ) && ( pSavedNote->Note().length() > 0 ) )
							strcat_s( buffer, std::string( " (" + pSavedNote->Note() + ")" ).c_str() );	//	##SD check...

						AddLogLine( buffer );
					}
				}

				EnableWindow( GetDlgItem( hDlg, IDC_RA_DOTEST ), g_MemManager.NumCandidates() > 0 );
			}
			return TRUE;

			case IDC_RA_MEMVIEW8BIT:
			case IDC_RA_MEMVIEW16BIT:
			case IDC_RA_MEMVIEW32BIT:
				Invalidate();	//	Cause the MemoryViewerControl to refresh
				MemoryViewerControl::destroyEditCaret();
				return FALSE;

			case IDC_RA_CBO_4BIT:
			case IDC_RA_CBO_8BIT:
			case IDC_RA_CBO_16BIT:
			case IDC_RA_CBO_32BIT:
			{
				bool b4BitSet = ( SendDlgItemMessage( hDlg, IDC_RA_CBO_4BIT, BM_GETCHECK, 0, 0 ) == BST_CHECKED );
				bool b8BitSet = ( SendDlgItemMessage( hDlg, IDC_RA_CBO_8BIT, BM_GETCHECK, 0, 0 ) == BST_CHECKED );
				bool b16BitSet = ( SendDlgItemMessage( hDlg, IDC_RA_CBO_16BIT, BM_GETCHECK, 0, 0 ) == BST_CHECKED );
				bool b32BitSet = ( SendDlgItemMessage( hDlg, IDC_RA_CBO_32BIT, BM_GETCHECK, 0, 0 ) == BST_CHECKED );

				ComparisonVariableSize nCompSize = Nibble_Lower;	//	or upper, doesn't really matter
				if( b4BitSet )
					nCompSize = Nibble_Lower;
				else if( b8BitSet )
					nCompSize = EightBit;
				else if( b16BitSet )
					nCompSize = SixteenBit;
				else //if( b32BitSet )
					nCompSize = ThirtyTwoBit;

				g_MemManager.Reset( 0, nCompSize );

				ClearLogOutput();
				AddLogLine( "Cleared: (" + std::string( COMPARISONVARIABLESIZE_STR[ nCompSize ] ) + ") mode. Aware of " + std::to_string( g_MemManager.NumCandidates() ) + " RAM locations." );
				EnableWindow( GetDlgItem( hDlg, IDC_RA_DOTEST ), g_MemManager.NumCandidates() > 0 );

				return FALSE;
			}

			case ID_OK:
				EndDialog( hDlg, TRUE );
				return TRUE;

			case IDC_RA_CBO_GIVENVAL:
			case IDC_RA_CBO_LASTKNOWNVAL:
				EnableWindow( GetDlgItem( hDlg, IDC_RA_TESTVAL ), ( IsDlgButtonChecked( hDlg, IDC_RA_CBO_GIVENVAL ) == BST_CHECKED ) );
				g_MemManager.SetUseLastKnownValue( IsDlgButtonChecked( hDlg, IDC_RA_CBO_GIVENVAL ) == BST_UNCHECKED );
				return TRUE;

			case IDC_RA_ADDNOTE:
			{
				HWND hMemWatch = GetDlgItem( hDlg, IDC_RA_WATCHING );
				HWND hMemDescription = GetDlgItem( hDlg, IDC_RA_MEMSAVENOTE );

				wchar_t sAddressWide[ 16 ];
				ComboBox_GetText( hMemWatch, sAddressWide, 16 );
				const std::string sAddress = Narrow( sAddressWide );

				if( sAddress[ 0 ] != '0' || sAddress[ 1 ] != 'x' )
					return FALSE;

				wchar_t sNewNoteWide[ 512 ];
				GetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, sNewNoteWide, 512 );
				const std::string sNewNote = Narrow( sNewNoteWide );

				const ByteAddress nAddr = static_cast<ByteAddress>( std::strtoul( sAddress.c_str() + 2, nullptr, 16 ) );
				const CodeNotes::CodeNoteObj* pSavedNote = m_CodeNotes.FindCodeNote( nAddr );
				if( ( pSavedNote != nullptr ) && ( pSavedNote->Note().length() > 0 ) )
				{
					if( pSavedNote->Note().compare( sNewNote ) != 0 )	//	New note is different
					{
						char sWarning[ 4096 ];
						sprintf_s( sWarning, 4096,
								   "Address 0x%04x already stored with note:\n\n"
								   "%s\n"
								   "by %s\n"
								   "\n\n"
								   "Would you like to overwrite with\n\n"
								   "%s",
								   nAddr,
								   pSavedNote->Note().c_str(),
								   pSavedNote->Author().c_str(),
								   sNewNote.c_str() );

						if( MessageBox( hDlg, Widen( sWarning ).c_str(), L"Warning: overwrite note?", MB_YESNO ) == IDYES )
							m_CodeNotes.Add( nAddr, RAUsers::LocalUser().Username(), sNewNote );
					}
					else
					{
						//	Already exists and is added exactly as described. Ignore.
					}
				}
				else
				{
					//	Doesn't yet exist: add it newly!
					m_CodeNotes.Add( nAddr, RAUsers::LocalUser().Username(), sNewNote );
					ComboBox_AddString( hMemWatch, Widen( sAddress ).c_str() );
				}

				return FALSE;
			}

			case IDC_RA_REMNOTE:
			{
				HWND hMemWatch = GetDlgItem( hDlg, IDC_RA_WATCHING );
				HWND hMemDescription = GetDlgItem( hDlg, IDC_RA_MEMSAVENOTE );

				wchar_t sAddressWide[ 16 ];
				ComboBox_GetText( hMemWatch, sAddressWide, 16 );
				const std::string sAddress = Narrow( sAddressWide );

				wchar_t sDescriptionWide[ 1024 ];
				GetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, sDescriptionWide, 1024 );
				const std::string sDescription = Narrow( sDescriptionWide );

				ByteAddress nAddr = static_cast<ByteAddress>( std::strtoul( sAddress.c_str() + 2, nullptr, 16 ) );

				m_CodeNotes.Remove( nAddr );

				SetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, L"" );

				int nIndex = ComboBox_FindString( hMemWatch, -1, Widen( sAddress ).c_str() );
				if( nIndex != CB_ERR )
					ComboBox_DeleteString( hMemWatch, nIndex );

				ComboBox_SetText( hMemWatch, L"" );

				return FALSE;
			}

			case IDC_RA_MEM_LIST:
			{
				wchar_t sSelectedString[ 1024 ];
				HWND hListbox = GetDlgItem( hDlg, IDC_RA_MEM_LIST );
				ListBox_GetText( hListbox, ListBox_GetCurSel( hListbox ), sSelectedString );

				if( wcslen( sSelectedString ) > 6 &&
					sSelectedString[ 0 ] == '0' &&
					sSelectedString[ 1 ] == 'x' )
				{
					char nString[1024];
					wcstombs(nString, sSelectedString, 1024);
					nString[ 8 ] = '\0';
					ByteAddress nAddr = static_cast<ByteAddress>( std::strtoul(nString + 2, NULL, 16 ) );	//	NB. Hex
					ComboBox_SetText( GetDlgItem( hDlg, IDC_RA_WATCHING ), Widen(nString).c_str() );

					const CodeNotes::CodeNoteObj* pSavedNote = m_CodeNotes.FindCodeNote( nAddr );
					if( ( pSavedNote != nullptr ) && ( pSavedNote->Note().length() > 0 ) )
						SetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, Widen( pSavedNote->Note() ).c_str() );
					else
						SetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, L"" );

					Invalidate();
				}

				return FALSE;
			}

			case IDC_RA_WATCHING:
				switch( HIWORD( wParam ) )
				{
					case CBN_SELCHANGE:
					{
						HWND hMemWatch = GetDlgItem( hDlg, IDC_RA_WATCHING );
						int nSel = ComboBox_GetCurSel( hMemWatch );
						if( nSel != CB_ERR )
						{
							wchar_t sAddr[ 64 ];
							if( ComboBox_GetLBText( hMemWatch, nSel, sAddr ) > 0 )
							{
								ByteAddress nAddr = static_cast<ByteAddress>( std::strtoul( Narrow( sAddr ).c_str() + 2, nullptr, 16 ) );
								const CodeNotes::CodeNoteObj* pSavedNote = m_CodeNotes.FindCodeNote( nAddr );
								if( pSavedNote != NULL && pSavedNote->Note().length() > 0 )
									SetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, Widen( pSavedNote->Note() ).c_str() );
							}
						}

						Invalidate();
						return TRUE;
					}
					case CBN_EDITCHANGE:
						OnWatchingMemChange();
						return TRUE;

					default:
						return FALSE;
						//return DefWindowProc( hDlg, nMsg, wParam, lParam );
				}

			case IDC_RA_MEMBANK:
				switch( HIWORD( wParam ) )
				{
					case LBN_SELCHANGE:
					{
						RA_LOG( "Sel detected!" );
						HWND hMemBanks = GetDlgItem( m_hWnd, IDC_RA_MEMBANK );
						int nSelectedIdx = ComboBox_GetCurSel( hMemBanks );
						
						unsigned short nBankID = static_cast<unsigned short>( ComboBox_GetItemData( hMemBanks, nSelectedIdx ) );

						MemoryViewerControl::m_nActiveMemBank = nBankID;
						g_MemManager.ChangeActiveMemBank( nBankID );

						InvalidateRect( m_hWnd, NULL, TRUE );	//	Force redraw on mem viewer
						break;
					}
				}

				return TRUE;

			default:
				return FALSE;	//	unhandled
		}
	}

	case WM_CLOSE:
		EndDialog( hDlg, 0 );
		return TRUE;

	default:
		return FALSE;	//	unhandled
	}
}

void Dlg_Memory::OnWatchingMemChange()
{
	wchar_t sAddrWide[ 1024 ];
	GetDlgItemText( m_hWnd, IDC_RA_WATCHING, sAddrWide, 1024 );
	std::string sAddr = Narrow( sAddrWide );
	ByteAddress nAddr = static_cast<ByteAddress>( std::strtoul( sAddr.c_str() + 2, nullptr, 16 ) );

	const CodeNotes::CodeNoteObj* pSavedNote = m_CodeNotes.FindCodeNote( nAddr );
	SetDlgItemText( m_hWnd, IDC_RA_MEMSAVENOTE, Widen( ( pSavedNote != nullptr ) ? pSavedNote->Note() : "" ).c_str() );

	MemoryViewerControl::destroyEditCaret();

	Invalidate();
}

void Dlg_Memory::RepopulateMemNotesFromFile()
{
	HWND hMemWatch = GetDlgItem( g_MemoryDialog.m_hWnd, IDC_RA_WATCHING );
	if( hMemWatch != NULL )
	{
		SetDlgItemText( hMemWatch, IDC_RA_MEMSAVENOTE, L"" );

		while( ComboBox_DeleteString( hMemWatch, 0 ) != CB_ERR ) {}

		GameID nGameID = g_pActiveAchievements->GetGameID();
		if( nGameID != 0 )
		{
			char sNotesFilename[1024];
			sprintf_s( sNotesFilename, 1024, "%s%d-Notes2.txt", RA_DIR_DATA, nGameID );
			size_t nSize = m_CodeNotes.Load( sNotesFilename );

			//	Issue a fetch instead!
			std::map<ByteAddress, CodeNotes::CodeNoteObj>::const_iterator iter = m_CodeNotes.FirstNote();
			while( iter != m_CodeNotes.EndOfNotes() )
			{
				const std::string sAddr = ByteAddressToString( iter->first );
				ComboBox_AddString( hMemWatch, Widen( sAddr ).c_str() );
				iter++;
			}

			if( nSize > 0 )
			{
				//	Select the first one.
				ComboBox_SetCurSel( hMemWatch, 0 );

				//	Note: as this is sorted, we should grab the desc again
				wchar_t sAddrWide[ 64 ];
				ComboBox_GetLBText( hMemWatch, 0, sAddrWide );
				const std::string sAddr = Narrow( sAddrWide );

				ByteAddress nAddr = static_cast<ByteAddress>( std::strtoul( sAddr.c_str() + 2, nullptr, 16 ) );
				const CodeNotes::CodeNoteObj* pSavedNote = m_CodeNotes.FindCodeNote( nAddr );
				if( ( pSavedNote != nullptr ) && ( pSavedNote->Note().length() > 0 ) )
					SetDlgItemText( m_hWnd, IDC_RA_MEMSAVENOTE, Widen( pSavedNote->Note() ).c_str() );
			}
		}
	}
}

void Dlg_Memory::OnLoad_NewRom()
{
	m_CodeNotes.ReloadFromWeb( g_pActiveAchievements->GetGameID() );
	
	SetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_MEM_LIST, L"" );
	SetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_MEMSAVENOTE, L"" );
	if( g_pActiveAchievements->GetGameID() == 0 )
		SetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_WATCHING, L"" );
	else
		SetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_WATCHING, L"Loading..." );

	RepopulateMemNotesFromFile();

	MemoryViewerControl::destroyEditCaret();
}

void Dlg_Memory::Invalidate()
{
	MemoryViewerControl::Invalidate();
}

void Dlg_Memory::SetWatchingAddress( unsigned int nAddr )
{
	char buffer[ 32 ];
	sprintf_s( buffer, 32, "0x%06x", nAddr );
	SetDlgItemText( g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, Widen( buffer ).c_str() );

	OnWatchingMemChange();
}

BOOL Dlg_Memory::IsActive() const
{
	return( g_MemoryDialog.GetHWND() != NULL ) && ( IsWindowVisible( g_MemoryDialog.GetHWND() ) );
}

void Dlg_Memory::ClearBanks()
{
	if( m_hWnd == nullptr )
		return;

	HWND hMemBanks = GetDlgItem( m_hWnd, IDC_RA_MEMBANK );
	while( ComboBox_DeleteString( hMemBanks, 0 ) != CB_ERR ) {}
}

void Dlg_Memory::AddBank( size_t nBankID )
{
	if( m_hWnd == nullptr )
		return;

	HWND hMemBanks = GetDlgItem( m_hWnd, IDC_RA_MEMBANK );
	int nIndex = ComboBox_AddString( hMemBanks, Widen( std::to_string( nBankID ) ).c_str() );
	if( nIndex != CB_ERR )
	{
		ComboBox_SetItemData( hMemBanks, nIndex, nBankID );
	}

	//	Select first element by default ('0')
	ComboBox_SetCurSel( hMemBanks, 0 );
}