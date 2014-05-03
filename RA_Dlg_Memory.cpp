#if defined (RA_VBA)
#include "stdafx.h"
#endif

#include "RA_Dlg_Memory.h"

#include <windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <stdio.h>
#include <assert.h>

#include "RA_Resource.h"
#include "RA_Core.h"
#include "RA_Achievement.h"
#include "RA_httpthread.h"
#include "RA_MemManager.h"
#include "RA_User.h"
#include "RA_CodeNotes.h"


#ifndef ID_OK
#define ID_OK                           1024
#endif
#ifndef ID_CANCEL
#define ID_CANCEL                       1025
#endif

Dlg_Memory g_MemoryDialog;

//HWND MemoryViewerControl::m_hControl = NULL;
HFONT MemoryViewerControl::m_hViewerFont = NULL;
SIZE MemoryViewerControl::m_szFontSize;
unsigned int MemoryViewerControl::m_nDataStartXOffset = 0;
unsigned int MemoryViewerControl::m_nAddressOffset = 0;
unsigned int MemoryViewerControl::m_nAddressSize = 0;
unsigned int MemoryViewerControl::m_nDataSize = 0;
unsigned int MemoryViewerControl::m_nEditAddress = 0;
unsigned int MemoryViewerControl::m_nEditNibble = 0;
bool MemoryViewerControl::m_bHasCaret = 0;
unsigned int MemoryViewerControl::m_nCaretWidth = 0;
unsigned int MemoryViewerControl::m_nCaretHeight = 0;
unsigned int MemoryViewerControl::m_nDisplayedLines = 0;


INT_PTR CALLBACK MemoryViewerControl::s_MemoryDrawProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	//char buffer[1024];
	//sprintf_s( buffer, "uMsg Child = 0x%08x\n", uMsg );
	//OutputDebugString( buffer );

	switch(uMsg)
	{
	case WM_NCCREATE:
	case WM_NCDESTROY:
		return TRUE;

	case WM_PAINT:
		RenderMemViewer( hDlg );
		return FALSE;

	case WM_ERASEBKGND:
		return TRUE;

	case WM_LBUTTONUP:
		{
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

			/*if( GetCursorPos( &pt ) )
			{
				ScreenToClient( hDlg, &pt );*/
			OnClick( pt );
			return 0;
		}

	case WM_KEYDOWN:
		if( OnKeyDown( (UINT)LOWORD( wParam ) ) )
			return 0;
		else
			return 1;

	case WM_CHAR:
		if( OnEditInput( (UINT)wParam&0xffff ) )
			return 0;
		else
			return 1;

		break;
	}

	return DefWindowProc( hDlg, uMsg, wParam, lParam );
}

bool MemoryViewerControl::OnKeyDown( UINT nChar ) 
{
	int maxNibble = 0;
	if(m_nDataSize == 0)
		maxNibble = 1;
	else if(m_nDataSize == 1)
		maxNibble = 3;
	else
		maxNibble = 7;

	bool isShift = (GetKeyState(VK_SHIFT) & 0x80000000) == 0x80000000;

	switch(nChar)
	{
		case VK_RIGHT:
			/*if(editAscii)
				moveAddress(1,0);
			else */if(isShift)
				moveAddress((maxNibble+1)>>1,0);
			else
				moveAddress(0, 1);
		return true;
		case VK_LEFT:
			/*if(editAscii)
				moveAddress(-1, 0);
			else */if(isShift)
				moveAddress(-((maxNibble+1)>>1),0);
			else
				moveAddress(0, -1);
		return true;
		case VK_DOWN:
			moveAddress(16, 0);
		return true;
		case VK_UP:
			moveAddress(-16, 0);
		return true;
		//case VK_TAB:
		//	GetNextDlgTabItem(GetParent(), isShift)->SetFocus();
		//return true;
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
    if( m_nEditAddress >= g_MemManager.RAMTotalSize() )
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
		unsigned int nVal = ( g_MemManager.RAMByte( nByteAddress ) >> 4 ) << 4;
		//	Merge in given (lower nibble) value,
		nVal |= nNewVal;
		//	Write value:
		g_MemManager.RAMByteWrite( nByteAddress, nVal );
	}
	else
	{
		//	We're submitting a new upper nibble:
		//	Fetch existing lower nibble,
		unsigned int nVal = g_MemManager.RAMByte( nByteAddress ) & 0xf;
		//	Merge in given value at upper nibble
		nVal |= (nNewVal<<4);
		//	Write value:
		g_MemManager.RAMByteWrite( nByteAddress, nVal );
	}
}

bool MemoryViewerControl::OnEditInput( UINT c )
{
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

		//switch(m_nDataSize)
		//{
		//case 0:
		//	editData( m_nEditAddress, 8, mask, value );
		//	break;
		//case 1:
		//	editData( m_nEditAddress, 16, mask, value );
		//	break;
		//case 2:
		//	editData( m_nEditAddress, 32, mask, value );
		//	break;
		//}

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
	HWND hOurDlg = GetDlgItem( g_MemoryDialog.GetHWND(), IDC_RA_MEMTEXTVIEWER );
	if(GetFocus() != hOurDlg)
	{
		destroyEditCaret();
		return;
	}

	g_MemoryDialog.SetWatchingAddress( m_nEditAddress );

	unsigned int nTopLeft = m_nAddressOffset - 0x40;
	if( g_MemManager.RAMTotalSize() > 0 )
		nTopLeft %= g_MemManager.RAMTotalSize();
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

	if( g_MemManager.RAMTotalSize() > 0 )
		m_nEditAddress %= g_MemManager.RAMTotalSize();
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
		m_hViewerFont = (HFONT)GetStockObject(SYSTEM_FIXED_FONT);
	HGDIOBJ hOldFont = SelectObject( hMemDC, m_hViewerFont );

	HBITMAP hBitmap = CreateCompatibleBitmap( dc, w, rect.bottom - rect.top );
	HGDIOBJ hOldBitmap = SelectObject( hMemDC, hBitmap );

	GetTextExtentPoint32( hMemDC, "0", 1, &m_szFontSize );

	//	Fill white:
	HBRUSH hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
	FillRect( hMemDC, &rect, hBrush );
	DrawEdge( hMemDC, &rect, EDGE_ETCHED, BF_RECT );

	BOOL bGroup32 = (BOOL)SendDlgItemMessage( g_MemoryDialog.GetHWND(), IDC_RA_MEMVIEW32BIT, BM_GETCHECK, (WPARAM)0, 0);
	BOOL bGroup16 = bGroup32 | (BOOL)SendDlgItemMessage( g_MemoryDialog.GetHWND(), IDC_RA_MEMVIEW16BIT, BM_GETCHECK, (WPARAM)0, 0);
	
	//if( g_MemManager.RAMTotalSize() == 0 )
	//	break;
	
	BOOL bUseLongAddresses = ( g_MemManager.RAMTotalSize() > 65536 );
	const DWORD nOKAddressLength = ( bUseLongAddresses ? 8 : 6 );

	m_nDataSize = ( bGroup32 ? 2 : ( bGroup16 ? 1 : 0 ) );
	m_nAddressSize = ( g_MemManager.RAMTotalSize() <= 65536 );	//	??

	char* sHeader = bGroup32 ? "          0        4        8        c"
		: bGroup16 ? "          0    2    4    6    8    a    c    e"
		: "          0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f";

	int lines = h / m_szFontSize.cy;
	lines -= 1;	//	Watch out for header
	m_nDisplayedLines = lines;
	
	//	Infer the offset from the watcher window
	char sCurrentAddr[16];
	ComboBox_GetText( GetDlgItem( g_MemoryDialog.GetHWND(), IDC_RA_WATCHING ), sCurrentAddr, 16 );
	unsigned int nWatchedAddress = strtol( sCurrentAddr, NULL, 16 );
	m_nAddressOffset = ( nWatchedAddress - ( nWatchedAddress & 0xf ) );

	int addr = m_nAddressOffset;

	//	Push reader up 4 lines, so center current watch var.
	//int iter = 0;
	//while( addr > 0 && iter++ < 4 )
		addr -= (0x40);

	//if( addr < 0 )
	//	addr = 0;

	if(m_nAddressSize)
		addr &= 0xffff;
	else
		addr &= 0xffffff;

	int line = 0;

	SetTextColor( hMemDC, RGB(0,0,0) );

	unsigned int data[32];

	RECT r;
	r.top = 3;
	r.left = 3;
	r.bottom = r.top+m_szFontSize.cy;
	r.right = rect.right-3;

	//	Draw header:
	DrawText( hMemDC, sHeader, strlen( sHeader ), &r, DT_TOP | DT_LEFT | DT_NOPREFIX );

	//	Adjust for header:
	r.top += m_szFontSize.cy;
	r.bottom += m_szFontSize.cy;

	for( int i = 0; i < lines; i++ )
	{
		char sLocalAddr[64];
		char buffer[4096];

		if( m_nAddressSize )
			sprintf_s( buffer, 4096, "0x%04x", addr );
		else
			sprintf_s( buffer, 4096, "0x%06x", addr );

		DrawText( hMemDC, buffer, strlen( buffer ), &r, DT_TOP | DT_LEFT | DT_NOPREFIX );

		r.left += 10*m_szFontSize.cx;
		m_nDataStartXOffset = r.left;

		for( int j = 0; j < 16; ++j )
			data[j] = g_MemManager.RAMByte(addr+j);
		//readData(addr, 16, data);


		if( m_nDataSize == 0 )	//	8-bit
		{
			for( int j = 0; j < 16; j++ )
			{
				sprintf_s( sLocalAddr, 64, m_nAddressSize ? "0x%04x" : "0x%06x", addr+j );

				if( addr+j == nWatchedAddress )
					SetTextColor( hMemDC, RGB(255,0,0) );
				else if( g_MemoryDialog.m_pCodeNotes->Exists( sLocalAddr, NULL, NULL ) )
					SetTextColor( hMemDC, RGB(0,0,255) );
				else
					SetTextColor( hMemDC, RGB(0,0,0) );

				sprintf_s( buffer, 4096, "%02x", data[j] );
				DrawText( hMemDC, buffer, strlen( buffer ), &r, DT_TOP | DT_LEFT | DT_NOPREFIX );
				r.left += 3*m_szFontSize.cx;
			}
		}
		else if( m_nDataSize == 1 )	//	16-bit
		{
			for( int j = 0; j < 16; j += 2 )
			{
				sprintf_s( sLocalAddr, 64, m_nAddressSize ? "0x%04x" : "0x%06x", addr+j );

				if( ((addr+j) - (addr+j)%2) == nWatchedAddress )
					SetTextColor( hMemDC, RGB(255,0,0) );
				else if( g_MemoryDialog.m_pCodeNotes->Exists( sLocalAddr, NULL, NULL ) )
					SetTextColor( hMemDC, RGB(0,0,255) );
				else
					SetTextColor( hMemDC, RGB(0,0,0) );

				sprintf_s( buffer, 4096, "%04x", data[j] | data[j+1]<<8 );
				DrawText( hMemDC, buffer, strlen( buffer ), &r, DT_TOP | DT_LEFT | DT_NOPREFIX );
				r.left += 5*m_szFontSize.cx;
			}      
		}
		else if( m_nDataSize == 2 )	//	32-bit
		{
			for( int j = 0; j < 16; j += 4 )
			{
				if( ((addr+j) - (addr+j)%4) == nWatchedAddress )
					SetTextColor( hMemDC, RGB(255,0,0) );
				else
					SetTextColor( hMemDC, RGB(0,0,0) );

				sprintf_s( buffer, 4096, "%08x", data[j] | data[j+1]<<8 | data[j+2] << 16 | data[j+3] << 24 );
				DrawText( hMemDC, buffer, strlen( buffer ), &r, DT_TOP | DT_LEFT | DT_NOPREFIX );
				r.left += 9*m_szFontSize.cx;
			}            
		}
    
		SetTextColor( hMemDC, RGB(0,0,0) );

		line = r.left;

		r.left += m_szFontSize.cx;

		//	ASCII render:

		//beginAscii = r.left;
		////buffer.Empty();
		//buffer[0] = '\0';

		//char* pIter = &buffer[0];

		//for(j = 0; j < 16; j++)
		//{
		//	char c = data[j];
		//	if(c >= 32 && c <= 127)
		//	{
		//		(*pIter) = c;
		//	}
		//	else
		//	{
		//		(*pIter) = '.';
		//	}
		//	pIter++;
		//}
		//DrawText( hMemDC, buffer, 16, &r, DT_TOP | DT_LEFT | DT_NOPREFIX );


		addr += 16;

		if(m_nAddressSize)
			addr &= 0xffff;
		else
			addr &= 0xffffff;

		r.top += m_szFontSize.cy;
		r.bottom += m_szFontSize.cy;
		r.left = 3;
	}

	{
		HPEN hPen = CreatePen( PS_SOLID, 1, RGB(0,0,0) );
		HGDIOBJ hOldPen = SelectObject( hMemDC, hPen );

		MoveToEx( hMemDC, 3+m_szFontSize.cx*9, 3+m_szFontSize.cy, NULL );
		LineTo( hMemDC, 3+m_szFontSize.cx*9, 3+( (m_nDisplayedLines+1) * m_szFontSize.cy ) );

		//MoveToEx( hMemDC, line, 3, NULL );
		//LineTo( hMemDC, line, 3+displayedLines*m_szFontSize.cy );

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


Dlg_Memory::Dlg_Memory()
{
	m_pCodeNotes = new CodeNotes();
	m_hWnd = NULL;
}

Dlg_Memory::~Dlg_Memory()
{
	if( m_pCodeNotes != NULL )
	{
		delete( m_pCodeNotes );
		m_pCodeNotes = NULL;
	}
}

void Dlg_Memory::Init()
{
    WNDCLASS wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.style = CS_PARENTDC | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
    wc.lpfnWndProc = (WNDPROC)MemoryViewerControl::s_MemoryDrawProc;
    wc.hInstance = g_hThisDLLInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "MemoryViewerControl";

	RegisterClass( &wc );
}

//static
INT_PTR CALLBACK Dlg_Memory::s_MemoryProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return g_MemoryDialog.MemoryProc( hDlg, uMsg, wParam, lParam );
}

INT_PTR Dlg_Memory::MemoryProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	//char buffer[1024];
	//sprintf_s( buffer, "uMsg = 0x%08x\n", uMsg );
	//OutputDebugString( buffer );

	switch(uMsg)
	{
	case WM_TIMER:
		{
			const int BufferSize = 2048;
			char sOutputBuffer[BufferSize];
			DWORD iterAddr = 0;
			DWORD nAddrOffsetOdd = 0;
			DWORD n = 0;

			SetDlgItemText( hDlg, IDC_RA_MEMBITS_TITLE, "" );
			SetDlgItemText( hDlg, IDC_RA_MEMBITS, "" );

			if( g_MemManager.RAMTotalSize() == 0 )
				break;

			BOOL bView8Bit = (BOOL)SendDlgItemMessage( hDlg, IDC_RA_MEMVIEW8BIT, BM_GETCHECK, (WPARAM)0, 0);
			if( !bView8Bit )
				break;

			BOOL bUseLongAddresses = ( g_MemManager.RAMTotalSize() > 65536 );
			const DWORD nOKAddressLength = ( bUseLongAddresses ? 8 : 6 );

			GetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_WATCHING, sOutputBuffer, 14 );
			if( strlen( sOutputBuffer ) == nOKAddressLength )
			{
				char* sData = sOutputBuffer+2;
				DWORD nAddr = strtol( sData, NULL, 16 );
				unsigned short nVal = (unsigned short)( g_MemManager.RAMByte(nAddr) );

				const char* sTitle =	"Bits: 7 6 5 4 3 2 1 0";
				char sDesc[32];
				sprintf_s( sDesc, 32,	"      %d %d %d %d %d %d %d %d",
					(int)( (nVal&(1<<7))!=0 ),
					(int)( (nVal&(1<<6))!=0 ),
					(int)( (nVal&(1<<5))!=0 ),
					(int)( (nVal&(1<<4))!=0 ),
					(int)( (nVal&(1<<3))!=0 ),
					(int)( (nVal&(1<<2))!=0 ),
					(int)( (nVal&(1<<1))!=0 ),
					(int)( (nVal&(1<<0))!=0 ) );

				SetDlgItemText( hDlg, IDC_RA_MEMBITS_TITLE, sTitle );
				SetDlgItemText( hDlg, IDC_RA_MEMBITS, sDesc );
			}




			//GetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_WATCHING, sOutputBuffer, 14 );
			//if( strlen( sOutputBuffer ) == nOKAddressLength )
			//{
			//	SendDlgItemMessage( g_MemoryDialog.m_hWnd, IDC_RA_WATCH, WM_CLEAR, (WPARAM)0, (LPARAM)0 );

			//	char* sData = sOutputBuffer+2;
			//	DWORD nAddr = strtol( sData, NULL, 16 );

			//	char* sHeader = bGroup32 ? "0        4        8        c"
			//		: bGroup16 ? "0    2    4    6    8    a    c    e"
			//		: "0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f";

			//	sprintf_s( sOutputBuffer, BufferSize, "%s%s        %s\\line\n", 
			//		"{\\rtf1\\ansi\\deff0"
			//		"{\\colortbl;\\red0\\green0\\blue0;\\red255\\green0\\blue0;}",
			//		(bUseLongAddresses ? "  " : ""), sHeader );

			//	for( DWORD i = 0; i < 128; i++ )
			//	{
			//		iterAddr = nAddr + (i - (64+15));
			//		if( iterAddr < 0 )
			//			continue;

			//		if( iterAddr > (g_MemManager.RAMTotalSize() - 16) )
			//			continue;

			//		if( iterAddr%16 != 0 )
			//			continue;


			//		if( bUseLongAddresses )
			//			sprintf_s( sOutputBuffer, BufferSize, "%s0x%06x:", sOutputBuffer, iterAddr );
			//		else //if( g_MemManager.RAMSize() <= 65536 )
			//			sprintf_s( sOutputBuffer, BufferSize, "%s0x%04x:", sOutputBuffer, iterAddr );

			//		nAddrOffsetOdd = (nAddr%2 == 0) ? 0 : 1;

			//		unsigned int nInterval = bGroup32 ? 4 : bGroup16 ? 2 : 1;

			//		for( n = 0; n < 16; n+=nInterval )
			//		{
			//			//const DWORD nOffset = (iterAddr+n - nAddr) + nAddrOffsetOdd;
			//			const DWORD nOffset = (iterAddr+n - nAddr);
			//			
			//			if( nOffset == 0 )
			//				strcat_s( sOutputBuffer, BufferSize, "\\cf2\n" );
			//			
			//			strcat_s( sOutputBuffer, BufferSize, " " );

			//			if( bGroup32 )
			//			{
			//				sprintf_s( sOutputBuffer, BufferSize, "%s%02x", sOutputBuffer, (unsigned short)( g_MemManager.RAMByte(iterAddr+n+3) ) );
			//				sprintf_s( sOutputBuffer, BufferSize, "%s%02x", sOutputBuffer, (unsigned short)( g_MemManager.RAMByte(iterAddr+n+2) ) );
			//				sprintf_s( sOutputBuffer, BufferSize, "%s%02x", sOutputBuffer, (unsigned short)( g_MemManager.RAMByte(iterAddr+n+1) ) );
			//				sprintf_s( sOutputBuffer, BufferSize, "%s%02x", sOutputBuffer, (unsigned short)( g_MemManager.RAMByte(iterAddr+n) ) );
			//			}
			//			else if( bGroup16 )
			//			{
			//				sprintf_s( sOutputBuffer, BufferSize, "%s%02x", sOutputBuffer, (unsigned short)( g_MemManager.RAMByte(iterAddr+n+1) ) );
			//				sprintf_s( sOutputBuffer, BufferSize, "%s%02x", sOutputBuffer, (unsigned short)( g_MemManager.RAMByte(iterAddr+n) ) );
			//			}
			//			else //if( bGroup8 )
			//			{
			//				sprintf_s( sOutputBuffer, BufferSize, "%s%02x", sOutputBuffer, (unsigned short)( g_MemManager.RAMByte(iterAddr+n) ) );
			//			}

			//			if( nOffset == 0 )
			//				strcat_s( sOutputBuffer, BufferSize, "\\cf1\n" );

			//		}

			//		strcat_s( sOutputBuffer, BufferSize, "\\line\n" );

			//		i += 15;
			//	}

			//	strcat_s( sOutputBuffer, BufferSize, "}" );
			//	SetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_WATCH, sOutputBuffer );
			//}
		}
		break;
	case WM_INITDIALOG:
		{
			char buffer[1024];
			//	Setup fixed-width font for edit box 'watch'
			//SendMessage( GetDlgItem(hDlg, IDC_RA_WATCH), WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), TRUE );
			SendMessage( GetDlgItem(hDlg, IDC_RA_MEMBITS), WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), TRUE );
			SendMessage( GetDlgItem(hDlg, IDC_RA_MEMBITS_TITLE), WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), TRUE );

			g_MemoryDialog.m_hWnd = hDlg;

 			RECT rc;
 			GetWindowRect( g_RAMainWnd, &rc );
 			int dx1 = (rc.right - rc.left);
 			int dy1 = (rc.bottom - rc.top);
 			SetWindowPos(hDlg, NULL, rc.right, rc.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

			if( g_MemManager.m_bUseLastKnownValue )
			{
				SendDlgItemMessage( hDlg, IDC_RA_CBO_GIVENVAL, BM_SETCHECK, (WPARAM)0, (LONG)0 );
				SendDlgItemMessage( hDlg, IDC_RA_CBO_LASTKNOWNVAL, BM_SETCHECK, (WPARAM)1, (LONG)0 );
				EnableWindow( GetDlgItem( hDlg, IDC_RA_TESTVAL ), FALSE );
			}
			else
			{
				SendDlgItemMessage( hDlg, IDC_RA_CBO_GIVENVAL, BM_SETCHECK, (WPARAM)1, (LONG)0 );
				SendDlgItemMessage( hDlg, IDC_RA_CBO_LASTKNOWNVAL, BM_SETCHECK, (WPARAM)0, (LONG)0 );
				EnableWindow( GetDlgItem( hDlg, IDC_RA_TESTVAL ), TRUE );
			}

			if( g_MemManager.m_nComparisonSizeMode == CMP_SZ_4BIT_LOWER || g_MemManager.m_nComparisonSizeMode == CMP_SZ_4BIT_UPPER )
			{
				SendDlgItemMessage( hDlg, IDC_RA_CBO_4BIT, BM_SETCHECK, (WPARAM)1, (LONG)0 );
				SendDlgItemMessage( hDlg, IDC_RA_CBO_8BIT, BM_SETCHECK, (WPARAM)0, (LONG)0 );
				SendDlgItemMessage( hDlg, IDC_RA_CBO_16BIT, BM_SETCHECK, (WPARAM)0, (LONG)0 );
				SendDlgItemMessage( hDlg, IDC_RA_CBO_32BIT, BM_SETCHECK, (WPARAM)0, (LONG)0 );
			}
			else if( g_MemManager.m_nComparisonSizeMode == CMP_SZ_8BIT )
			{
				SendDlgItemMessage( hDlg, IDC_RA_CBO_4BIT, BM_SETCHECK, (WPARAM)0, (LONG)0 );
				SendDlgItemMessage( hDlg, IDC_RA_CBO_8BIT, BM_SETCHECK, (WPARAM)1, (LONG)0 );
				SendDlgItemMessage( hDlg, IDC_RA_CBO_16BIT, BM_SETCHECK, (WPARAM)0, (LONG)0 );
				SendDlgItemMessage( hDlg, IDC_RA_CBO_32BIT, BM_SETCHECK, (WPARAM)0, (LONG)0 );
			}
			else if( g_MemManager.m_nComparisonSizeMode == CMP_SZ_16BIT )
			{
				SendDlgItemMessage( hDlg, IDC_RA_CBO_4BIT, BM_SETCHECK, (WPARAM)0, (LONG)0 );
				SendDlgItemMessage( hDlg, IDC_RA_CBO_8BIT, BM_SETCHECK, (WPARAM)0, (LONG)0 );
				SendDlgItemMessage( hDlg, IDC_RA_CBO_16BIT, BM_SETCHECK, (WPARAM)1, (LONG)0 );
				SendDlgItemMessage( hDlg, IDC_RA_CBO_32BIT, BM_SETCHECK, (WPARAM)0, (LONG)0 );
			}
			else //if( g_MemManager.m_nComparisonSizeMode == CMP_SZ_32BIT )
			{
				SendDlgItemMessage( hDlg, IDC_RA_CBO_4BIT, BM_SETCHECK, (WPARAM)0, (LONG)0 );
				SendDlgItemMessage( hDlg, IDC_RA_CBO_8BIT, BM_SETCHECK, (WPARAM)0, (LONG)0 );
				SendDlgItemMessage( hDlg, IDC_RA_CBO_16BIT, BM_SETCHECK, (WPARAM)0, (LONG)0 );
				SendDlgItemMessage( hDlg, IDC_RA_CBO_32BIT, BM_SETCHECK, (WPARAM)1, (LONG)0 );
			}

			sprintf_s( buffer, 1024, "Restored: Aware of %d mem locations.", g_MemManager.m_NumCandidates );
			SendDlgItemMessage( hDlg, IDC_RA_MEM_LIST, LB_ADDSTRING, (WPARAM)0, (LONG)(LPTSTR)buffer );

			SendDlgItemMessage( hDlg, IDC_RA_CBO_CMPTYPE, CB_ADDSTRING, (WPARAM)0, (LONG)(LPTSTR)"==" );	//	EQ
			SendDlgItemMessage( hDlg, IDC_RA_CBO_CMPTYPE, CB_ADDSTRING, (WPARAM)0, (LONG)(LPTSTR)"<" );	//	LT
			SendDlgItemMessage( hDlg, IDC_RA_CBO_CMPTYPE, CB_ADDSTRING, (WPARAM)0, (LONG)(LPTSTR)"<=" );	//	LTE
			SendDlgItemMessage( hDlg, IDC_RA_CBO_CMPTYPE, CB_ADDSTRING, (WPARAM)0, (LONG)(LPTSTR)">" );	//	GT
			SendDlgItemMessage( hDlg, IDC_RA_CBO_CMPTYPE, CB_ADDSTRING, (WPARAM)0, (LONG)(LPTSTR)">=" );	//	GTE
			SendDlgItemMessage( hDlg, IDC_RA_CBO_CMPTYPE, CB_ADDSTRING, (WPARAM)0, (LONG)(LPTSTR)"!=" );	//	NEQ

			SendDlgItemMessage( hDlg, IDC_RA_CBO_CMPTYPE, CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );

			SetTimer( g_MemoryDialog.m_hWnd, 1, 50, (TIMERPROC)s_MemoryProc );

			EnableWindow( GetDlgItem( hDlg, IDC_RA_DOTEST ), g_MemManager.m_NumCandidates > 0 );

			SetDlgItemText( hDlg, IDC_RA_WATCHING, "0x8000" );

			SendDlgItemMessage( hDlg, IDC_RA_CBO_4BIT, BM_SETCHECK, (WPARAM)0, 0);
			SendDlgItemMessage( hDlg, IDC_RA_CBO_8BIT, BM_SETCHECK, (WPARAM)1, 0);
			SendDlgItemMessage( hDlg, IDC_RA_CBO_16BIT, BM_SETCHECK, (WPARAM)0, 0);

			SendDlgItemMessage( hDlg, IDC_RA_MEMVIEW8BIT, BM_SETCHECK, (WPARAM)0, 0);
			SendDlgItemMessage( hDlg, IDC_RA_MEMVIEW16BIT, BM_SETCHECK, (WPARAM)1, 0);
			SendDlgItemMessage( hDlg, IDC_RA_MEMVIEW32BIT, BM_SETCHECK, (WPARAM)0, 0);

			//Dlg_Memory_MemoryProc( hDlg, WM_COMMAND, ID_RESET, 0 );
			MemoryProc( hDlg, WM_COMMAND, IDC_RA_CBO_8BIT, 0 );

			g_MemoryDialog.OnLoad_NewRom();
		}
		return TRUE;
		break;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_RA_DOTEST:
			{
				if( g_MemManager.RAMTotalSize() == 0 )
					break;

				unsigned int i = 0;
				char buffer[1024];
				unsigned int nQuery = 0;
				size_t nResults = 0;

				const char* CmpStrings[] = {
					{ "EQUAL" },
					{ "LESS THAN" },
					{ "LESS THAN/EQUAL" },
					{ "GREATER THAN" },
					{ "GREATER THAN/EQUAL" },
					{ "NOT EQUAL" } };

				int nCmpType = (int)( SendDlgItemMessage( hDlg, IDC_RA_CBO_CMPTYPE, CB_GETCURSEL, (WPARAM)0, (LPARAM)0 ) );

				if( g_MemManager.m_pCoreRAM == NULL )
					break;

				SendDlgItemMessage( hDlg, IDC_RA_MEM_LIST, LB_RESETCONTENT, (WPARAM)0, (LPARAM)0 );

				if( GetDlgItemText( hDlg, IDC_RA_TESTVAL, buffer, 14 ) )
					nQuery = (unsigned int)atoi( buffer );

				if( g_MemManager.m_bUseLastKnownValue )
				{
					sprintf_s( buffer, 1024, "Filtering for %s last known value...", CmpStrings[nCmpType] );
				}
				else
				{
					sprintf_s( buffer, 1024, "Filtering for %s %d...", CmpStrings[nCmpType], nQuery );
				}

				SendDlgItemMessage( hDlg, IDC_RA_MEM_LIST, LB_ADDSTRING, (WPARAM)0, (LONG)(LPTSTR)buffer );

				//////////////////////////////////////////////////////////////////////////
				BOOL bResultsFound = FALSE;
				nResults = g_MemManager.Compare( nCmpType, nQuery, bResultsFound );
				//////////////////////////////////////////////////////////////////////////

				if( !bResultsFound )
				{
					sprintf_s( buffer, 1024, "Found *ZERO* matches: restoring old results set!" );
				}
				else
				{
					sprintf_s( buffer, 1024, "Found %d matches!", nResults );
				}

				SendDlgItemMessage( hDlg, IDC_RA_MEM_LIST, LB_ADDSTRING, (WPARAM)0, (LONG)(LPTSTR)buffer );

				if( nResults < 250 )
				{
					for( i = 0; i < nResults; ++i )
					{
						const DWORD nCandidateAddr = g_MemManager.ValidMemAddrFound( i );

						char sMem[16];

						if( g_MemManager.RAMTotalSize() > 65536 )
							sprintf_s( sMem, 16, "0x%06x", nCandidateAddr );
						else
							sprintf_s( sMem, 16, "0x%04x", nCandidateAddr );


						// 						if( g_MemManager.GetComparisonSize() == CMP_SZ_32BIT )
						// 						{
						// 							sprintf_s( TempBuffer, "0x%04x: 0x%08x", g_MemManager.ValidMemAddrFound( i ), Ram_68k[g_MemManager.ValidMemAddrFound( i )] );
						// 						}

						if( g_MemManager.m_nComparisonSizeMode == CMP_SZ_16BIT )
						{
							sprintf_s( buffer, 1024, "%s: 0x%04x", sMem, g_MemManager.RAMByte( nCandidateAddr ) | ( g_MemManager.RAMByte( nCandidateAddr + 1 ) << 8 ) );
						}
						else if( g_MemManager.m_nComparisonSizeMode == CMP_SZ_8BIT )
						{
							sprintf_s( buffer, 1024, "%s: 0x%02x", sMem, g_MemManager.RAMByte( nCandidateAddr ));
						}
						else if( g_MemManager.m_nComparisonSizeMode == CMP_SZ_4BIT_LOWER || g_MemManager.m_nComparisonSizeMode == CMP_SZ_4BIT_UPPER )
						{
							if( g_MemManager.m_Candidates[i].m_bUpperNibble )
								sprintf_s( buffer, 1024, "%s: 0x%01x", sMem, (g_MemManager.RAMByte( nCandidateAddr )>>4)&0xf );
							else
								sprintf_s( buffer, 1024, "%s: 0x%01x", sMem, g_MemManager.RAMByte( nCandidateAddr )&0xf );
						}
						else
						{
							//?
						}

						char sDesc[1024];
						char sAuthor[1024];
						if( m_pCodeNotes->Exists( sMem, sAuthor, sDesc, 1024 ) )
						{
							strcat_s( buffer, " (" );
							strcat_s( buffer, sDesc );
							strcat_s( buffer, ")" );
						}

						SendDlgItemMessage( hDlg, IDC_RA_MEM_LIST, LB_ADDSTRING, (WPARAM)0, (LONG)(LPTSTR)buffer );
					}
				}

				{
					HWND hFilterButton = GetDlgItem( hDlg, IDC_RA_DOTEST );
					EnableWindow( hFilterButton, g_MemManager.m_NumCandidates > 0 );
				}

				break;
			}

			return TRUE;
			break;

		case IDC_RA_MEMVIEW8BIT:
		case IDC_RA_MEMVIEW16BIT:
		case IDC_RA_MEMVIEW32BIT:
			{
				Invalidate();	//	Cause the MemoryViewerControl to refresh
				MemoryViewerControl::destroyEditCaret();
				//LRESULT n4BitSet = SendDlgItemMessage( hDlg, IDC_RA_MEMVIEW8BIT, BM_GETCHECK, (WPARAM)0, 0);
				//LRESULT n8BitSet = SendDlgItemMessage( hDlg, IDC_RA_MEMVIEW16BIT, BM_GETCHECK, (WPARAM)0, 0);
				//LRESULT n16BitSet = SendDlgItemMessage( hDlg, IDC_RA_MEMVIEW32BIT, BM_GETCHECK, (WPARAM)0, 0);
			}
			break;

		case IDC_RA_CBO_4BIT:
		case IDC_RA_CBO_8BIT:
		case IDC_RA_CBO_16BIT:
			//case IDC_RA_CBO_32BIT:
			//case ID_RESET:
			{
				LRESULT n4BitSet = SendDlgItemMessage( hDlg, IDC_RA_CBO_4BIT, BM_GETCHECK, (WPARAM)0, 0);
				LRESULT n8BitSet = SendDlgItemMessage( hDlg, IDC_RA_CBO_8BIT, BM_GETCHECK, (WPARAM)0, 0);
				LRESULT n16BitSet = SendDlgItemMessage( hDlg, IDC_RA_CBO_16BIT, BM_GETCHECK, (WPARAM)0, 0);
				//LRESULT n32BitSet = SendDlgItemMessage( hDlg, IDC_RA_CBO_32BIT, BM_GETCHECK, (WPARAM)0, 0);
				const char* sMode = n16BitSet ? "16-bit" : n8BitSet ? "8-bit" : "4-bit";
				char TempBuffer[1024];

				CompVariableSize cmpSize = CMP_SZ_4BIT_LOWER;	//	or upper, doesn't really matter
				if( n8BitSet )
					cmpSize = CMP_SZ_8BIT;
				else if( n16BitSet )
					cmpSize = CMP_SZ_16BIT;

				g_MemManager.Reset( cmpSize );

				SendDlgItemMessage( hDlg, IDC_RA_MEM_LIST, LB_RESETCONTENT, (WPARAM)0, 0 );
				sprintf_s( TempBuffer, 1024, "Cleared: (%s) mode. Aware of %d RAM locations.", 
					sMode, g_MemManager.m_NumCandidates );
				SendDlgItemMessage( hDlg, IDC_RA_MEM_LIST, LB_ADDSTRING, (WPARAM)0, (LONG)(LPTSTR)TempBuffer );

				{
					HWND hFilterButton = GetDlgItem( hDlg, IDC_RA_DOTEST );
					EnableWindow( hFilterButton, g_MemManager.m_NumCandidates > 0 );
				}
			}

			break;

		case ID_OK:
			EndDialog( hDlg, TRUE );

			return TRUE;
			break;

		case IDC_RA_CBO_GIVENVAL:
		case IDC_RA_CBO_LASTKNOWNVAL:
			{
				HWND hEditBox = GetDlgItem( hDlg, IDC_RA_TESTVAL );
				LRESULT nUseGivenVal = SendDlgItemMessage( hDlg, IDC_RA_CBO_GIVENVAL, BM_GETCHECK, (WPARAM)0, 0);
				if( nUseGivenVal==BST_CHECKED )
				{ 
					SendDlgItemMessage( hDlg, IDC_RA_CBO_GIVENVAL, BM_SETCHECK, (WPARAM)1, 0 );
					SendDlgItemMessage( hDlg, IDC_RA_CBO_LASTKNOWNVAL, BM_SETCHECK, (WPARAM)0, 0 );
					EnableWindow( hEditBox, TRUE );
				}
				else
				{
					SendDlgItemMessage( hDlg, IDC_RA_CBO_GIVENVAL, BM_SETCHECK, (WPARAM)0, 0 );
					SendDlgItemMessage( hDlg, IDC_RA_CBO_LASTKNOWNVAL, BM_SETCHECK, (WPARAM)1, 0 );
					EnableWindow( hEditBox, FALSE );
				}
				g_MemManager.m_bUseLastKnownValue = (nUseGivenVal==BST_UNCHECKED);
			}

			return TRUE;
			break;

		case IDC_RA_ADDNOTE:
			{
 				HWND hMemWatch = GetDlgItem( hDlg, IDC_RA_WATCHING );
  				char sAddress[16];
  				ComboBox_GetText( hMemWatch, (LPSTR)sAddress, 16 );
 
 				char sDescription[512];
 				HWND hMemDescription = GetDlgItem( hDlg, IDC_RA_MEMSAVENOTE );
 				GetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, (LPSTR)sDescription, 512 );
 				
 				BOOL bDoSave = FALSE;

				char sAuthor[512];
 				char sOldDesc[512];
 				if( m_pCodeNotes->Exists( sAddress, sAuthor, sOldDesc, 512 ) )
 				{
 					if( strcmp( sDescription, sOldDesc ) != 0 )
 					{
 						char sWarning[4096];
 						sprintf_s( sWarning, 4096, "Address %s already stored with note:\n\n"
 							"%s\n"
							"by %s\n"
							"\n\n"
 							"Would you like to overwrite with\n\n"
 							"%s",
 							sAddress, sOldDesc, sAuthor, sDescription );
 						
 						if( MessageBox( hDlg, sWarning, "Warning: overwrite note?", MB_YESNO ) == IDYES )
 						{
 							m_pCodeNotes->Add( g_LocalUser.Username(), sAddress, sDescription );
 							//ComboBox_AddString( hMemWatch, (LPSTR)sAddress );	//	Already added!
 							
 							bDoSave = TRUE;
 						}
 					}
 					else
 					{
 						//	Already exists and is added exactly as described. Ignore.
 					}
 				}
 				else
 				{
 					//	Doesn't yet exist: add it newly!
 					m_pCodeNotes->Add( g_LocalUser.Username(), sAddress, sDescription );
 					ComboBox_AddString( hMemWatch, (LPSTR)sAddress );
 					
 					bDoSave = TRUE;
 				}
 
 				if( bDoSave )
 				{
 					char buffer[1024];
 					int nGameID = g_pActiveAchievements->GameID();
 					sprintf_s( buffer, 1024, "%s%d-Notes2.txt", RA_DIR_DATA, nGameID );
 					m_pCodeNotes->Save( buffer );
 				}

				break;
			}
		case IDC_RA_REMNOTE:
			{
 				HWND hMemWatch = GetDlgItem( hDlg, IDC_RA_WATCHING );
 				char sAddress[16];
 				ComboBox_GetText( hMemWatch, (LPSTR)sAddress, 16 );
 
 				char sDescription[1024];
 				HWND hMemDescription = GetDlgItem( hDlg, IDC_RA_MEMSAVENOTE );
 				GetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, (LPSTR)sDescription, 1024 );
 
 				m_pCodeNotes->Remove( sAddress );
 				char buffer[1024];
 
 				int nGameID = g_pActiveAchievements->GameID();
 				sprintf_s( buffer, 1024, "%s%d-Notes2.txt", RA_DIR_DATA, nGameID );
 				m_pCodeNotes->Save( buffer );
 
 				SetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, "" );
 
 				int nIndex = ComboBox_FindString( hMemWatch, -1, sAddress );
 				if( nIndex != CB_ERR )
 					ComboBox_DeleteString( hMemWatch, nIndex );
 
 				ComboBox_SetText( hMemWatch, "" );

				break;
			}
		case IDC_RA_MEM_LIST:
			{
				char sSelectedString[1024];

				HWND hLbx = GetDlgItem( hDlg, IDC_RA_MEM_LIST );
				ListBox_GetText( hLbx, ListBox_GetCurSel(hLbx), (LPCTSTR)sSelectedString );

				if( strlen( sSelectedString ) > 4 &&
					sSelectedString[0] == '0' &&
					sSelectedString[1] == 'x' )
				{
					if( g_MemManager.RAMTotalSize() > 65536 )
						sSelectedString[8] = '\0';
					else //if( g_MemManager.RAMSize() <= 
						sSelectedString[6] = '\0';

					{
						HWND hMemWatch = GetDlgItem( hDlg, IDC_RA_WATCHING );
						ComboBox_SetText( hMemWatch, (LPCTSTR)sSelectedString );
					}

					char sAuthor[1024];
					char sDesc[1024];
 					if( m_pCodeNotes->Exists( sSelectedString, sAuthor, sDesc, 1024 ) )
 					{
 						SetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, sDesc );
 					}
 					else
 					{
 						SetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, "" );
 					}

					Invalidate();
				}
				break;
			}
		case IDC_RA_WATCHING:
			{
				HWND hMemWatch = GetDlgItem( hDlg, IDC_RA_WATCHING );
				if( HIWORD(wParam) == CBN_SELCHANGE )
				{
					int nSel = ComboBox_GetCurSel( hMemWatch );
					if( nSel != CB_ERR )
					{
						char sAddr[64];
						if( ComboBox_GetLBText( hMemWatch, nSel, sAddr ) > 0 )
						{
							char sDesc[1024];
							char sAuthor[1024];
 							if( m_pCodeNotes->Exists( sAddr, sAuthor, sDesc, 1024 ) )
 							{
 								SetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, sDesc );
 							}
						}	
					}

					Invalidate();
				}
				else if( HIWORD(wParam) == CBN_EDITCHANGE )
				{
					OnWatchingMemChange();
				}
			}
			break;
		}
		break;

	//case WM_SIZE:
	//	//	Resize control elements:

	//	break;

	case WM_CLOSE:
		EndDialog( hDlg, TRUE );
		return TRUE;
		break;
	}

	return 0;
	//return DefWindowProc( hDlg, uMsg, wParam, lParam );
}

void Dlg_Memory::OnWatchingMemChange()
{
	char sAddr[1024];
	char sAuthor[1024];
	GetDlgItemText( m_hWnd, IDC_RA_WATCHING, sAddr, 1024 );

	char sDesc[1024];
	if( m_pCodeNotes->Exists( sAddr, sAuthor, sDesc, 1024 ) )
	{
		SetDlgItemText( m_hWnd, IDC_RA_MEMSAVENOTE, sDesc );
	}
	else
	{
		SetDlgItemText( m_hWnd, IDC_RA_MEMSAVENOTE, "" );
	}

	MemoryViewerControl::destroyEditCaret();

	Invalidate();
}

void Dlg_Memory::RepopulateMemNotesFromFile()
{
	//char sNotesFilename[1024];
	HWND hMemWatch = GetDlgItem( g_MemoryDialog.m_hWnd, IDC_RA_WATCHING );
	if( hMemWatch != NULL )
	{
		while( ComboBox_DeleteString( hMemWatch, 0 ) > 0 )
		{
		}

		int nGameID = g_pActiveAchievements->m_nGameID;
		if( nGameID != 0 )
		{
			char sNotesFilename[1024];
			sprintf_s( sNotesFilename, 1024, "%s%d-Notes2.txt", RA_DIR_DATA, nGameID );
			size_t nSize = m_pCodeNotes->Load( sNotesFilename );

			//	Issue a fetch instead!

			for( size_t i = 0; i < nSize; ++i )
			{
				const char* sAddress = m_pCodeNotes->GetAddress( i );
				ComboBox_AddString( hMemWatch, sAddress );
			}

			if( nSize > 0 )
			{
				//	Select the first one.
				ComboBox_SetCurSel( hMemWatch, 0 );

				//	Note: as this is sorted, we should grab the desc again
				char sAddr[64];
				char sDesc[1024];
				char sAuthor[1024];
				ComboBox_GetLBText( hMemWatch, 0, sAddr );
				if( m_pCodeNotes->Exists( sAddr, sAuthor, sDesc, 1024 ) )
					SetDlgItemText( m_hWnd, IDC_RA_MEMSAVENOTE, sDesc );
			}
		}
	}
}

void Dlg_Memory::OnLoad_NewRom()
{
	m_pCodeNotes->Update( g_pActiveAchievements->m_nGameID );
	
	SetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_MEM_LIST, "" );
	SetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_MEMSAVENOTE, "" );
	if( g_pActiveAchievements->m_nGameID == 0 )
		SetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_WATCHING, "" );
	else
		SetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_WATCHING, "Loading..." );

	RepopulateMemNotesFromFile();

	MemoryViewerControl::destroyEditCaret();
}

void Dlg_Memory::Invalidate()
{
	MemoryViewerControl::Invalidate();
}

void Dlg_Memory::SetWatchingAddress( unsigned int nAddr )
{
	char buffer[32];
	if( g_MemManager.RAMTotalSize() <= 65536 )
		sprintf_s( buffer, 32, "0x%04x", nAddr );
	else
		sprintf_s( buffer, 32, "0x%06x", nAddr );

	SetDlgItemText( g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, buffer );

	OnWatchingMemChange();
}
