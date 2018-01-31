#include "RA_Dlg_Memory.h"

#include "RA_Achievement.h"
#include "RA_AchievementSet.h"
#include "RA_CodeNotes.h"
#include "RA_Core.h"
#include "RA_GameData.h"
#include "RA_httpthread.h"
#include "RA_MemManager.h"
#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_Dlg_MemBookmark.h"

#ifndef ID_OK
#define ID_OK                           1024
#endif
#ifndef ID_CANCEL
#define ID_CANCEL                       1025
#endif

namespace
{
	const size_t MIN_RESULTS_TO_DUMP = 500000;
	const size_t MIN_SEARCH_PAGE_SIZE = 50;

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

unsigned int m_nPage = 0;

// Dialog Resizing
int nDlgMemoryMinX;
int nDlgMemoryMinY;
int nDlgMemViewerGapY;

std::string ByteAddressToString(ByteAddress nAddr)
{
	static char buffer[16];
	sprintf_s(buffer, "0x%06x", nAddr);
	return std::string(buffer);
}

INT_PTR CALLBACK MemoryViewerControl::s_MemoryDrawProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCCREATE:
	case WM_NCDESTROY:
		return TRUE;

	case WM_CREATE:
		return TRUE;

	case WM_PAINT:
		RenderMemViewer(hDlg);
		return 0;

	case WM_ERASEBKGND:
		return TRUE;

	case WM_MOUSEWHEEL:
		if (GET_WHEEL_DELTA_WPARAM(wParam) > 0 && m_nAddressOffset > (0x40))
			setAddress(m_nAddressOffset - 32);
		else if (GET_WHEEL_DELTA_WPARAM(wParam) < 0 && m_nAddressOffset + (0x40) < g_MemManager.TotalBankSize())
			setAddress(m_nAddressOffset + 32);
		return FALSE;

	case WM_LBUTTONUP:
		OnClick({ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) });
		return FALSE;

	case WM_KEYDOWN:
		return(!OnKeyDown(static_cast<UINT>(LOWORD(wParam))));

	case WM_CHAR:
		return(!OnEditInput(static_cast<UINT>(LOWORD(wParam))));
	}

	return DefWindowProc(hDlg, uMsg, wParam, lParam);
	//return FALSE;
}

bool MemoryViewerControl::OnKeyDown(UINT nChar)
{
	int maxNibble = 0;
	if (m_nDataSize == 0)
		maxNibble = 1;
	else if (m_nDataSize == 1)
		maxNibble = 3;
	else
		maxNibble = 7;

	bool bShiftHeld = (GetKeyState(VK_SHIFT) & 0x80000000) == 0x80000000;

	switch (nChar)
	{
	case VK_RIGHT:
		if (bShiftHeld)
			moveAddress((maxNibble + 1) >> 1, 0);
		else
			moveAddress(0, 1);
		return true;

	case VK_LEFT:
		if (bShiftHeld)
			moveAddress(-((maxNibble + 1) >> 1), 0);
		else
			moveAddress(0, -1);
		return true;

	case VK_DOWN:
		moveAddress(0x10, 0);
		return true;

	case VK_UP:
		moveAddress(-0x10, 0);
		return true;

	case VK_PRIOR:	//	Page up (!)
		moveAddress(-(int)(m_nDisplayedLines << 4), 0);
		return true;

	case VK_NEXT:	//	Page down (!)
		moveAddress((m_nDisplayedLines << 4), 0);
		return true;

	case VK_HOME:
		m_nEditAddress = 0;
		m_nEditNibble = 0;
		setAddress(0);
		return true;

	case VK_END:
		m_nEditAddress = g_MemManager.TotalBankSize() - 0x10;
		m_nEditNibble = 0;
		setAddress(m_nEditAddress);
		return true;
	}

	return false;
}

void MemoryViewerControl::moveAddress(int offset, int nibbleOff)
{
	unsigned int maxNibble = 0;
	if (m_nDataSize == 0)
		maxNibble = 1;
	else if (m_nDataSize == 1)
		maxNibble = 3;
	else
		maxNibble = 7;

	if (offset == 0)
	{
		if (nibbleOff == -1)
		{
			//	Going left
			m_nEditNibble--;
			if (m_nEditAddress == 0 && m_nEditNibble == -1)
			{
				m_nEditNibble = 0;
				MessageBeep((UINT)-1);
				return;
			}
			if (m_nEditNibble == -1)
			{
				m_nEditAddress -= (maxNibble + 1) >> 1;
				m_nEditNibble = maxNibble;
			}
		}
		else
		{
			//	Going right
			m_nEditNibble++;
			if (m_nEditNibble > maxNibble)
			{
				m_nEditNibble = 0;
				m_nEditAddress += (maxNibble + 1) >> 1;
			}
			if (m_nEditAddress >= g_MemManager.TotalBankSize())
			{
				//	Undo this movement.
				m_nEditAddress -= (maxNibble + 1) >> 1;
				m_nEditNibble = maxNibble;
				MessageBeep((UINT)-1);
				return;
			}
		}
	}
	else
	{
		m_nEditAddress += offset;

		if (offset < 0)
		{

			if (m_nEditAddress > (m_nAddressOffset - 1 + (m_nDisplayedLines << 4)) && (signed)m_nEditAddress < (0x10))
			{
				m_nEditAddress -= offset;
				MessageBeep((UINT)-1);
				return;
			}
		}
		else
		{
			if (m_nEditAddress >= g_MemManager.TotalBankSize())
			{
				m_nEditAddress -= offset;
				MessageBeep((UINT)-1);
				return;
			}
		}
	}

	if (m_nEditAddress + (0x40) < m_nAddressOffset)
		setAddress((m_nEditAddress & ~(0xf)) + (0x40));
	else if (m_nEditAddress >= (m_nAddressOffset + (m_nDisplayedLines << 4) - (0x40)))
		setAddress((m_nEditAddress & ~(0xf)) - (m_nDisplayedLines << 4) + (0x50));

	SetCaretPos();
}

void MemoryViewerControl::setAddress(unsigned int address)
{
	m_nAddressOffset = address;
	//g_MemoryDialog.SetWatchingAddress( address );

	SetCaretPos();
	Invalidate();
}

void MemoryViewerControl::Invalidate()
{
	HWND hOurDlg = GetDlgItem(g_MemoryDialog.GetHWND(), IDC_RA_MEMTEXTVIEWER);
	if (hOurDlg != NULL)
		InvalidateRect(hOurDlg, NULL, TRUE);
}

void MemoryViewerControl::editData(unsigned int nByteAddress, bool bLowerNibble, unsigned int nNewVal)
{
	//	Immediately invalidate all submissions.
	g_bRAMTamperedWith = true;

	if (bLowerNibble)
	{
		//	We're submitting a new lower nibble:
		//	Fetch existing upper nibble,
		unsigned int nVal = (g_MemManager.ActiveBankRAMByteRead(nByteAddress) >> 4) << 4;
		//	Merge in given (lower nibble) value,
		nVal |= nNewVal;
		//	Write value:
		g_MemManager.ActiveBankRAMByteWrite(nByteAddress, nVal);
	}
	else
	{
		//	We're submitting a new upper nibble:
		//	Fetch existing lower nibble,
		unsigned int nVal = g_MemManager.ActiveBankRAMByteRead(nByteAddress) & 0xf;
		//	Merge in given value at upper nibble
		nVal |= (nNewVal << 4);
		//	Write value:
		g_MemManager.ActiveBankRAMByteWrite(nByteAddress, nVal);
	}
}

bool MemoryViewerControl::OnEditInput(UINT c)
{
	if (g_MemManager.NumMemoryBanks() == 0)
		return false;

	if (c > 255 || !RA_GameIsActive())
	{
		MessageBeep((UINT)-1);
		return false;
	}

	c = tolower(c);

	unsigned int value = 256;

	if (c >= 'a' && c <= 'f')
		value = 10 + (c - 'a');
	else if (c >= '0' && c <= '9')
		value = (c - '0');

	int maxNibble = 0;
	if (m_nDataSize == 0)
		maxNibble = 1;
	else if (m_nDataSize == 1)
		maxNibble = 3;
	else
		maxNibble = 7;

	if (value != 256)
	{
		//value <<= 4*(maxNibble-m_nEditNibble);
		//unsigned int mask = ~(15 << 4*(maxNibble - m_nEditNibble));

		bool bLowerNibble = (m_nEditNibble % 2 == 1);
		unsigned int nByteAddress = m_nEditAddress;

		if (m_nDataSize == 0)
		{
			//	8 bit
			//nByteAddress = m_nEditAddress;
		}
		else if (m_nDataSize == 1)
		{
			//	16 bit
			nByteAddress += (1 - (m_nEditNibble >> 1));
		}
		else
		{
			//	32 bit
			nByteAddress += (3 - (m_nEditNibble >> 1));
		}

		editData(nByteAddress, bLowerNibble, value);

		if (g_MemBookmarkDialog.GetHWND() != nullptr)
			g_MemBookmarkDialog.UpdateBookmarks( TRUE );

		moveAddress(0, 1);
		Invalidate();
	}

	return true;
}

void MemoryViewerControl::createEditCaret(int w, int h)
{
	if (!m_bHasCaret || m_nCaretWidth != w || m_nCaretHeight != h) {
		m_bHasCaret = true;
		m_nCaretWidth = w;
		m_nCaretHeight = h;
		::CreateCaret(GetDlgItem(g_MemoryDialog.GetHWND(), IDC_RA_MEMTEXTVIEWER), (HBITMAP)0, w, h);
	}
}

void MemoryViewerControl::destroyEditCaret()
{
	m_bHasCaret = false;
	DestroyCaret();
}

void MemoryViewerControl::SetCaretPos()
{
	if (g_MemManager.NumMemoryBanks() == 0)
		return;

	HWND hOurDlg = GetDlgItem(g_MemoryDialog.GetHWND(), IDC_RA_MEMTEXTVIEWER);
	if (GetFocus() != hOurDlg)
	{
		destroyEditCaret();
		return;
	}

	g_MemoryDialog.SetWatchingAddress(m_nEditAddress);

	//int nTopLeft = m_nAddressOffset - 0x40;

	int subAddress = (m_nEditAddress - m_nAddressOffset);

	int linePosition = (subAddress & ~(0xF)) / (0x10) + 4;

	if (linePosition < 0 || linePosition >(int)m_nDisplayedLines - 1)

	{
		destroyEditCaret();
		return;
	}

	const int nYSpacing = linePosition;

	int x = 3 + (10 * m_szFontSize.cx) + (m_nEditNibble*m_szFontSize.cx);
	int y = 3 + (nYSpacing*m_szFontSize.cy);

	y += m_szFontSize.cy;	//	Account for header

	switch (m_nDataSize)
	{
	case 0:
		x += 3 * m_szFontSize.cx*(subAddress & 15);
		break;
	case 1:
		x += 5 * m_szFontSize.cx*((subAddress >> 1) & 7);
		break;
	case 2:
		x += 9 * m_szFontSize.cx*((subAddress >> 2) & 3);
		break;
	}

	RECT r;
	GetClientRect(hOurDlg, &r);
	r.right -= 3;
	if (x >= r.right) {
		destroyEditCaret();
		return;
	}
	int w = m_szFontSize.cx;
	if ((x + m_szFontSize.cx) >= r.right)
	{
		w = r.right - x;
	}
	createEditCaret(w, m_szFontSize.cy);
	::SetCaretPos(x, y);
	ShowCaret(hOurDlg);
}

void MemoryViewerControl::OnClick(POINT point)
{
	if (g_MemManager.NumMemoryBanks() == 0)
		return;

	HWND hOurDlg = GetDlgItem(g_MemoryDialog.GetHWND(), IDC_RA_MEMTEXTVIEWER);

	int x = point.x;
	int y = point.y - m_szFontSize.cy;	//	Adjust for header
	int line = ((y - 3) / m_szFontSize.cy);

	if (line == -1 || line >= (int)m_nDisplayedLines)
		return;	//	clicked on header

	int rowLengthPx = m_nDataStartXOffset;
	int inc = 1;
	int sub = 3 * m_szFontSize.cx;

	switch (m_nDataSize)
	{
	case 0:
		rowLengthPx += 47 * m_szFontSize.cx;
		inc = 1;					//	increment mem offset by 1 each subset
		sub = 3 * m_szFontSize.cx;	//	2 char set plus one char space
		break;
	case 1:
		rowLengthPx += 39 * m_szFontSize.cx;
		inc = 2;					//	increment mem offset by 2 each subset
		sub = 5 * m_szFontSize.cx;	//	4 char set plus one char space
		break;
	case 2:
		rowLengthPx += 35 * m_szFontSize.cx;
		inc = 4;					//	increment mem offset by 4 each subset
		sub = 9 * m_szFontSize.cx;	//	8 char set plus one char space
		break;
	}

	int nTopLeft = m_nAddressOffset - 0x40;

	int nAddressRowClicked = (nTopLeft + (line << 4));

	//	Clamp:
	if (nAddressRowClicked < 0 || nAddressRowClicked >(int)g_MemManager.TotalBankSize())
	{
		//	ignore; clicked above limit
		return;
	}

	m_nEditAddress = static_cast<unsigned int>(nAddressRowClicked);


	if (x >= (int)m_nDataStartXOffset && x < rowLengthPx)
	{
		x -= m_nDataStartXOffset;
		m_nEditNibble = 0;

		while (x > 0)
		{
			//	Adjust x by one subset til we find out the correct offset:
			x -= sub;
			if (x >= 0)
				m_nEditAddress += inc;
			else
				m_nEditNibble = (x + sub) / m_szFontSize.cx;
		}

	}
	else
	{
		return;
	}

	unsigned int maxNibble = 0;
	if (m_nDataSize == 0)
		maxNibble = 1;
	else if (m_nDataSize == 1)
		maxNibble = 3;
	else
		maxNibble = 7;

	if (m_nEditNibble > maxNibble)
		m_nEditNibble = maxNibble;
	SetFocus(hOurDlg);
	SetCaretPos();
}

void MemoryViewerControl::RenderMemViewer(HWND hTarget)
{
	PAINTSTRUCT ps;
	HDC dc = BeginPaint(hTarget, &ps);

	HDC hMemDC = CreateCompatibleDC(dc);

	RECT rect;
	GetClientRect(hTarget, &rect);
	int w = rect.right - rect.left;
	int h = rect.bottom - rect.top - 6;

	//	Pick font
	if (m_hViewerFont == NULL)
		m_hViewerFont = (HFONT)GetStockObject(SYSTEM_FIXED_FONT);
	HGDIOBJ hOldFont = SelectObject(hMemDC, m_hViewerFont);

	HBITMAP hBitmap = CreateCompatibleBitmap(dc, w, rect.bottom - rect.top);
	HGDIOBJ hOldBitmap = SelectObject(hMemDC, hBitmap);

	GetTextExtentPoint32(hMemDC, TEXT("0"), 1, &m_szFontSize);

	//	Fill white:
	HBRUSH hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
	FillRect(hMemDC, &rect, hBrush);
	DrawEdge(hMemDC, &rect, EDGE_ETCHED, BF_RECT);

	BOOL bGroup32 = (IsDlgButtonChecked(g_MemoryDialog.GetHWND(), IDC_RA_MEMVIEW32BIT) == BST_CHECKED);
	BOOL bGroup16 = bGroup32 | (IsDlgButtonChecked(g_MemoryDialog.GetHWND(), IDC_RA_MEMVIEW16BIT) == BST_CHECKED);

	m_nDataSize = (bGroup32 ? 2 : (bGroup16 ? 1 : 0));

	const char* sHeader = bGroup32 ? "          0        4        8        c"
		: bGroup16 ? "          0    2    4    6    8    a    c    e"
		: "          0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f";

	int lines = h / m_szFontSize.cy;
	lines -= 1;	//	Watch out for header
	m_nDisplayedLines = lines;

	//	Infer the offset from the watcher window
	TCHAR bufferNative[64];
	ComboBox_GetText(GetDlgItem(g_MemoryDialog.GetHWND(), IDC_RA_WATCHING), bufferNative, 64);

	//char buffer[64];
	//memcpy_s(buffer, 64, Narrow(bufferNative).c_str(), 64);

	unsigned long nWatchedAddress = std::strtoul(Narrow(bufferNative).c_str(), nullptr, 16);
	//m_nAddressOffset = ( nWatchedAddress - ( nWatchedAddress & 0xf ) );

	int addr = m_nAddressOffset;
	addr -= (0x40);	//	Offset will be this quantity (push up four lines)...

	SetTextColor(hMemDC, RGB(0, 0, 0));

	unsigned char data[16];
	unsigned int notes;
	unsigned int bookmarks;
	unsigned int freeze;

	RECT r;
	r.top = 3;
	r.left = 3;
	r.bottom = r.top + m_szFontSize.cy;
	r.right = rect.right - 3;

	//	Draw header:
	DrawText(hMemDC, NativeStr(sHeader).c_str(), strlen(sHeader), &r, DT_TOP | DT_LEFT | DT_NOPREFIX);

	//	Adjust for header:
	r.top += m_szFontSize.cy;
	r.bottom += m_szFontSize.cy;

	if (g_MemManager.NumMemoryBanks() > 0)
	{
		m_nDataStartXOffset = r.left + 10 * m_szFontSize.cx;

		for (int i = 0; i < lines && addr < (int)g_MemManager.TotalBankSize(); ++i, addr += 16)
		{
			if (addr >= 0)
			{
				notes = 0;
				bookmarks = 0;
				freeze = 0;
				for (int j = 0; j < 16; ++j)
				{
					notes |= (g_MemoryDialog.Notes().FindCodeNote(addr + j) != NULL) ? (1 << j) : 0;
					const MemBookmark* bm = g_MemBookmarkDialog.FindBookmark(addr + j);
					bookmarks |= (bm != NULL) ? (1 << j) : 0;
					freeze |= (bm != NULL && bm->Frozen()) ? (1 << j) : 0;
				}

				g_MemManager.ActiveBankRAMRead(data, addr, 16);

				TCHAR* ptr = bufferNative + wsprintf(bufferNative, TEXT("0x%06x  "), addr);
				switch (m_nDataSize)
				{
				case 0:
					for (int j = 0; j < 16; ++j)
						ptr += wsprintf(ptr, TEXT("%02x "), data[j]);
					break;
				case 1:
					for (int j = 0; j < 16; j += 2)
						ptr += wsprintf(ptr, TEXT("%02x%02x "), data[j + 1], data[j]);
					break;
				case 2:
					for (int j = 0; j < 16; j += 4)
						ptr += wsprintf(ptr, TEXT("%02x%02x%02x%02x "), data[j + 3], data[j + 2], data[j + 1], data[j]);
					break;
				}

				DrawText(hMemDC, NativeStr(bufferNative).c_str(), ptr - bufferNative, &r, DT_TOP | DT_LEFT | DT_NOPREFIX);

				if ((nWatchedAddress & ~0x0F) == addr)
				{
					SetTextColor(hMemDC, RGB(255, 0, 0));

					size_t stride;
					switch (m_nDataSize)
					{
					case 0:
						ptr = bufferNative + 10 + 3 * (nWatchedAddress & 0x0F);
						stride = 2;
						break;
					case 1:
						ptr = bufferNative + 10 + 5 * ((nWatchedAddress & 0x0F) / 2);
						stride = 4;
						break;
					case 2:
						ptr = bufferNative + 10 + 9 * ((nWatchedAddress & 0x0F) / 4);
						stride = 8;
						break;
					}

					r.left = 3 + (ptr - bufferNative) * m_szFontSize.cx;
					DrawText(hMemDC, ptr, stride, &r, DT_TOP | DT_LEFT | DT_NOPREFIX);

					SetTextColor(hMemDC, RGB(0, 0, 0));
					r.left = 3;

					// make sure we don't overwrite the current address with an indicator
					notes		&= ~(1 << (nWatchedAddress & 0x0F));
					bookmarks	&= ~(1 << (nWatchedAddress & 0x0F));
				}

				if (notes || bookmarks)
				{
					for (int j = 0; j < 16; ++j)
					{
						bool bDraw = FALSE;

						if (bookmarks & 0x01)
						{
							if (freeze & 0x01)
								SetTextColor(hMemDC, RGB(255, 200, 0));
							else
								SetTextColor(hMemDC, RGB(0, 160, 0));

							bDraw = TRUE;
						}
						else if (notes & 0x01)
						{
							SetTextColor(hMemDC, RGB(0, 0, 255));
							bDraw = TRUE;
						}

						if (bDraw)
						{
							size_t stride;
							switch (m_nDataSize)
							{
							case 0:
								ptr = bufferNative + 10 + 3 * j;
								stride = 2;
								break;
							case 1:
								ptr = bufferNative + 10 + 5 * (j / 2);
								stride = 4;
								break;
							case 2:
								ptr = bufferNative + 10 + 9 * (j / 4);
								stride = 8;
								break;
							}

							r.left = 3 + (ptr - bufferNative) * m_szFontSize.cx;
							DrawText(hMemDC, NativeStr( ptr ).c_str(), stride, &r, DT_TOP | DT_LEFT | DT_NOPREFIX);
						}

						notes >>= 1;
						bookmarks >>= 1;
						freeze >>= 1;
					}

					r.left = 3;
				}

				SetTextColor(hMemDC, RGB(0, 0, 0));
			}

			r.top += m_szFontSize.cy;
			r.bottom += m_szFontSize.cy;
		}
	}

	{
		HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
		HGDIOBJ hOldPen = SelectObject(hMemDC, hPen);

		MoveToEx(hMemDC, 3 + m_szFontSize.cx * 9, 3 + m_szFontSize.cy, NULL);
		LineTo(hMemDC, 3 + m_szFontSize.cx * 9, 3 + ((m_nDisplayedLines + 1) * m_szFontSize.cy));

		SelectObject(hMemDC, hOldPen);
		DeleteObject(hPen);
	}

	SelectObject(hMemDC, hOldFont);

	BitBlt(dc, 0, 0, w, rect.bottom - rect.top, hMemDC, 0, 0, SRCCOPY);

	SelectObject(hMemDC, hOldBitmap);
	DeleteDC(hMemDC);
	DeleteObject(hBitmap);

	EndPaint(hTarget, &ps);
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
	wc.lpszClassName = TEXT("MemoryViewerControl");

	RegisterClass(&wc);
}

//static
INT_PTR CALLBACK Dlg_Memory::s_MemoryProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return g_MemoryDialog.MemoryProc(hDlg, uMsg, wParam, lParam);
}

void Dlg_Memory::ClearLogOutput()
{
	ListView_SetItemCount( GetDlgItem( m_hWnd, IDC_RA_MEM_LIST ), 1 );
	EnableWindow( GetDlgItem( m_hWnd, IDC_RA_RESULTS_BACK ), FALSE );
	EnableWindow( GetDlgItem( m_hWnd, IDC_RA_RESULTS_FORWARD ), FALSE );
}


INT_PTR Dlg_Memory::MemoryProc( HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam )
{
	switch ( nMsg )
	{
		case WM_TIMER:
		{
			if ( ( g_MemManager.NumMemoryBanks() == 0 ) || ( g_MemManager.TotalBankSize() == 0 ) )
			{
				SetDlgItemText( hDlg, IDC_RA_MEMBITS, TEXT( "" ) );
				return FALSE;
			}

			// Update Search Results
			InvalidateRect( GetDlgItem( hDlg, IDC_RA_MEM_LIST ), NULL, TRUE );

			// Display Bits
			bool bView8Bit = ( SendDlgItemMessage( hDlg, IDC_RA_MEMVIEW8BIT, BM_GETCHECK, 0, 0 ) == BST_CHECKED );
			if ( !bView8Bit )
			{
				SetDlgItemText( hDlg, IDC_RA_MEMBITS, TEXT( "" ) );
				return FALSE;
			}

			TCHAR nativeBuffer[ 1024 ];
			GetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_WATCHING, nativeBuffer, 1024 );
			const LPTSTR buffer = nativeBuffer;
			if ( ( _tcslen(buffer) >= 3 ) && buffer[ 0 ] == '0' && buffer[ 1 ] == 'x' )
			{
				ByteAddress nAddr = static_cast<ByteAddress>( strtol( buffer + 2, nullptr, 16 ) );
				unsigned char nVal = g_MemManager.ActiveBankRAMByteRead( nAddr );

				TCHAR sDesc[ 64 ];
				_stprintf_s( sDesc, 64, _T( "      %d %d %d %d %d %d %d %d" ),
					static_cast<int>( ( nVal & ( 1 << 7 ) ) != 0 ),
					static_cast<int>( ( nVal & ( 1 << 6 ) ) != 0 ),
					static_cast<int>( ( nVal & ( 1 << 5 ) ) != 0 ),
					static_cast<int>( ( nVal & ( 1 << 4 ) ) != 0 ),
					static_cast<int>( ( nVal & ( 1 << 3 ) ) != 0 ),
					static_cast<int>( ( nVal & ( 1 << 2 ) ) != 0 ),
					static_cast<int>( ( nVal & ( 1 << 1 ) ) != 0 ),
					static_cast<int>( ( nVal & ( 1 << 0 ) ) != 0 ) );

				GetDlgItemText( hDlg, IDC_RA_MEMBITS, nativeBuffer, 1024 );
				if ( _tcscmp( sDesc, nativeBuffer ) != 0 )
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
			GenerateResizes( hDlg );

			CheckDlgButton( hDlg, IDC_RA_CBO_SEARCHALL, BST_CHECKED );
			CheckDlgButton( hDlg, IDC_RA_CBO_SEARCHCUSTOM, BST_UNCHECKED );
			EnableWindow( GetDlgItem( hDlg, IDC_RA_SEARCHRANGE ), FALSE );
			CheckDlgButton( hDlg, IDC_RA_CBO_SEARCHSYSTEMRAM, BST_UNCHECKED );
			CheckDlgButton( hDlg, IDC_RA_CBO_SEARCHGAMERAM, BST_UNCHECKED );
			CheckDlgButton( hDlg, IDC_RA_CBO_GIVENVAL, BST_UNCHECKED );
			CheckDlgButton( hDlg, IDC_RA_CBO_LASTKNOWNVAL, BST_CHECKED );
			EnableWindow( GetDlgItem( hDlg, IDC_RA_TESTVAL ), FALSE );

			for ( size_t i = 0; i < NumComparisonTypes; ++i )
				ComboBox_AddString( GetDlgItem( hDlg, IDC_RA_CBO_CMPTYPE ), NativeStr( COMPARISONTYPE_STR[ i ] ).c_str() );

			ComboBox_SetCurSel( GetDlgItem( hDlg, IDC_RA_CBO_CMPTYPE ), 0 );

			//	Update timer proc
			SetTimer( hDlg, 1, 1, (TIMERPROC)s_MemoryProc );

			EnableWindow( GetDlgItem( hDlg, IDC_RA_DOTEST ), g_MemManager.NumCandidates() > 0 );

			SetDlgItemText( hDlg, IDC_RA_WATCHING, TEXT( "0x0000" ) );

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

			// Add a single column for list view
			LVCOLUMN Col;
			Col.mask = LVCF_FMT | LVCF_ORDER | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
			Col.fmt = LVCFMT_CENTER;
			GetWindowRect( GetDlgItem( hDlg, IDC_RA_MEM_LIST ), &rc );
			for ( int i = 0; i < 1; i++ )
			{
				Col.iOrder = i;
				Col.iSubItem = i;
				Col.pszText = _T( "Search Result" );
				Col.cx = rc.right - rc.left - 24;
				ListView_InsertColumn( GetDlgItem( hDlg, IDC_RA_MEM_LIST ), i, &Col );
			}
			ListView_SetExtendedListViewStyle( GetDlgItem( hDlg, IDC_RA_MEM_LIST ), LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER );

			CheckDlgButton ( hDlg, IDC_RA_RESULTS_HIGHLIGHT, BST_CHECKED );

			//	Fetch banks
			ClearBanks();
			std::vector<size_t> bankIDs = g_MemManager.GetBankIDs();
			for ( size_t i = 0; i < bankIDs.size(); ++i )
				AddBank( bankIDs[ i ] );

			return TRUE;
		}

		case WM_MEASUREITEM:
			PMEASUREITEMSTRUCT pmis;
			pmis = (PMEASUREITEMSTRUCT)lParam;
			pmis->itemHeight = 16;
			return TRUE;

		case WM_DRAWITEM:
			LPDRAWITEMSTRUCT pDIS;
			HWND hListbox;

			pDIS = (LPDRAWITEMSTRUCT)lParam;
			hListbox = GetDlgItem( hDlg, IDC_RA_MEM_LIST );
			if ( pDIS->hwndItem == hListbox )
			{
				if ( pDIS->itemID == -1 )
					break;

				if ( m_SearchResults.size() > 0 )
				{
					TCHAR buffer[ 1024 ];

					if ( pDIS->itemID < 2 )
					{
						if ( pDIS->itemID == 0 )
							_stprintf_s ( buffer, sizeof( buffer ), _T( "%s" ), m_SearchResults[ m_nPage ].m_sFirstLine.c_str() );
						else
						{
							SetTextColor( pDIS->hDC, RGB( 0, 100, 150 ) );
							_stprintf_s ( buffer, sizeof( buffer ), _T( "%s" ), m_SearchResults[ m_nPage ].m_sSecondLine.c_str() );
						}

						DrawText( pDIS->hDC, buffer, _tcslen( buffer ), &pDIS->rcItem, DT_SINGLELINE | DT_LEFT | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER | DT_END_ELLIPSIS );
						SetTextColor( pDIS->hDC, GetSysColor( COLOR_WINDOWTEXT ) );
					}
					else
					{
						unsigned int nVal = 0;
						MemCandidate& currentResult = m_SearchResults[ m_nPage ].m_ResultCandidate[ pDIS->itemID - 2 ];
						UpdateSearchResult( pDIS->itemID, nVal, buffer );

						const CodeNotes::CodeNoteObj* pSavedNote = m_CodeNotes.FindCodeNote( currentResult.m_nAddr );
						if ( ( pSavedNote != NULL ) && ( pSavedNote->Note().length() > 0 ) )
							_tcscat( buffer, tstring( "   (" + pSavedNote->Note() + ")" ).c_str() );

						COLORREF color;

						if ( pDIS->itemState & ODS_SELECTED )
						{
							SetTextColor( pDIS->hDC, GetSysColor( COLOR_HIGHLIGHTTEXT ) );
							color = GetSysColor( COLOR_HIGHLIGHT );
						}
						else if ( SendMessage ( GetDlgItem( hDlg, IDC_RA_RESULTS_HIGHLIGHT ), BM_GETCHECK, 0, 0 ) )
						{
							SetTextColor( pDIS->hDC, GetSysColor( COLOR_WINDOWTEXT ) );
							if ( !CompareSearchResult( nVal, currentResult.m_nLastKnownValue ) )
							{
								color = RGB( 255, 215, 215 ); // Red if search result doesn't match comparison.
								currentResult.m_bHasChanged = true;
							}
							else if ( g_MemBookmarkDialog.FindBookmark( currentResult.m_nAddr ) != NULL )
								color = RGB( 220, 255, 220 ); // Green if Bookmark is found.
							else if ( g_MemoryDialog.Notes().FindCodeNote( currentResult.m_nAddr ) != NULL )
								color = RGB( 220, 240, 255 ); // Blue if Code Note is found.
							else if ( currentResult.m_bHasChanged )
								color = RGB( 240, 240, 240 ); // Grey if still valid, but has changed
							else
								color = GetSysColor( COLOR_WINDOW );
						}
						else
							color = GetSysColor( COLOR_WINDOW );

						HBRUSH hBrush = CreateSolidBrush( color );
						FillRect( pDIS->hDC, &pDIS->rcItem, hBrush );
						DeleteObject( hBrush );

						DrawText( pDIS->hDC, buffer, _tcslen( buffer ), &pDIS->rcItem, DT_SINGLELINE | DT_LEFT | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER | DT_END_ELLIPSIS );
					}
				}
			}
			return TRUE;

		case WM_NOTIFY:
		{
			switch ( LOWORD( wParam ) )
			{
				case IDC_RA_MEM_LIST:
				{
					if ( ( (LPNMHDR)lParam )->code == NM_CLICK )
					{
						int nSelect = ListView_GetNextItem( GetDlgItem( hDlg, IDC_RA_MEM_LIST ), -1, LVNI_FOCUSED );

						if ( nSelect == -1 )
							break;
						else if ( nSelect >= 2 )
						{
							TCHAR nString[ 1024 ];
							ByteAddress nAddr = m_SearchResults[ m_nPage ].m_ResultCandidate[ nSelect - 2 ].m_nAddr;
							_stprintf_s( nString, 1024, "0x%06x", nAddr );
							ComboBox_SetText( GetDlgItem( hDlg, IDC_RA_WATCHING ), nString );

							const CodeNotes::CodeNoteObj* pSavedNote = m_CodeNotes.FindCodeNote( nAddr );
							if ( ( pSavedNote != nullptr ) && ( pSavedNote->Note().length() > 0 ) )
								SetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, pSavedNote->Note().c_str() );
							else
								SetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, "" );

							MemoryViewerControl::setAddress( ( nAddr & ~( 0xf ) ) - ( (int)( MemoryViewerControl::m_nDisplayedLines / 2 ) << 4 ) + ( 0x50 ) );

							Invalidate();
						}
						else
							ListView_SetItemState( GetDlgItem( hDlg, IDC_RA_MEM_LIST ), -1, LVIF_STATE, LVIS_SELECTED );
					}
				}
			}
		}
		break;

		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
			lpmmi->ptMaxTrackSize.x = nDlgMemoryMinX;
			lpmmi->ptMinTrackSize.x = nDlgMemoryMinX;
			lpmmi->ptMinTrackSize.y = nDlgMemoryMinY;
		}
		return TRUE;

		case WM_SIZE:
		{
			RECT itemRect, winRect;
			GetWindowRect( hDlg, &winRect );

			HWND hItem = GetDlgItem( hDlg, IDC_RA_MEMTEXTVIEWER );
			GetWindowRect( hItem, &itemRect );
			SetWindowPos( hItem, NULL, 0, 0,
				itemRect.right - itemRect.left, ( winRect.bottom - itemRect.top ) + nDlgMemViewerGapY, SWP_NOMOVE | SWP_NOZORDER );
		}
		return TRUE;

		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case IDC_RA_DOTEST:
				{
					if ( g_MemManager.NumMemoryBanks() == 0 )
						return TRUE;	//	Ignored

					if ( g_MemManager.TotalBankSize() == 0 )
						return TRUE;	//	Handled

					ComparisonType nCmpType = static_cast<ComparisonType>( ComboBox_GetCurSel( GetDlgItem( hDlg, IDC_RA_CBO_CMPTYPE ) ) );

					while ( m_SearchResults.size() > m_nPage + 1 )
						m_SearchResults.pop_back();

					ClearLogOutput();
					SearchResult sr;
					m_SearchResults.push_back( sr );
					m_nPage++;

					if ( m_SearchResults.size() > MIN_SEARCH_PAGE_SIZE )
					{
						m_SearchResults.erase ( m_SearchResults.begin() );
						m_nPage--;
					}

					EnableWindow( GetDlgItem( hDlg, IDC_RA_RESULTS_BACK ), TRUE );
					EnableWindow( GetDlgItem( hDlg, IDC_RA_RESULTS_FORWARD ), FALSE );

					unsigned int nValueQuery = 0;

					{
						TCHAR nativeBuffer[ 1024 ];
						if ( GetDlgItemText( hDlg, IDC_RA_TESTVAL, nativeBuffer, 1024 ) )
						{
							tstring buffer = nativeBuffer;
							//	Read hex or dec
							if ( buffer[ 0 ] == '0' && buffer[ 1 ] == 'x' )
								nValueQuery = static_cast<unsigned int>( std::strtoul( buffer.c_str() + 2, NULL, 16 ) );
							else
								nValueQuery = static_cast<unsigned int>( std::strtoul( buffer.c_str(), NULL, 10 ) );
						}
					}

					tstring str(
						"Filtering for " + tstring( COMP_STR[ nCmpType ] ) +
						( ( g_MemManager.UseLastKnownValue() ) ? " last known value..." : std::to_string( nValueQuery ) ) );

					m_SearchResults[ m_nPage ].m_sFirstLine = str;

					//////////////////////////////////////////////////////////////////////////
					bool bResultsFound = false;
					size_t nResults = g_MemManager.Compare( nCmpType, nValueQuery, bResultsFound );
					//////////////////////////////////////////////////////////////////////////

					std::stringstream sstr;
					if ( !bResultsFound )
						sstr << "Found *ZERO* matches: restoring old results set ( " << nResults << " results )!";
					else
						sstr << "Found " << nResults << " matches!";

					// Store the search results to vector.
					m_SearchResults[ m_nPage ].m_sSecondLine = NativeStr( sstr.str() );
					m_SearchResults[ m_nPage ].m_nCount = nResults;
					m_SearchResults[ m_nPage ].m_nCompareType = nCmpType;
					m_SearchResults[ m_nPage ].m_nLastQueryVal = nValueQuery;
					m_SearchResults[ m_nPage ].m_bUseLastValue = g_MemManager.UseLastKnownValue();

					for ( size_t i = 0; i < nResults; i++ )
					{
						m_SearchResults[ m_nPage ].m_ResultCandidate.push_back ( g_MemManager.GetCandidate( i ) );
						if ( i >= MIN_RESULTS_TO_DUMP - 1 )
						{
							m_SearchResults[ m_nPage ].m_sSecondLine += " (Displaying first " + std::to_string(MIN_RESULTS_TO_DUMP) + " results)";
							break;
						}
					}

					ListView_SetItemCount( GetDlgItem( hDlg, IDC_RA_MEM_LIST ), m_SearchResults[ m_nPage ].m_ResultCandidate.size() + 2 );

					EnableWindow( GetDlgItem( hDlg, IDC_RA_DOTEST ), g_MemManager.NumCandidates() > 0 );
				}
				return TRUE;

				case IDC_RA_MEMVIEW8BIT:
				case IDC_RA_MEMVIEW16BIT:
				case IDC_RA_MEMVIEW32BIT:
				{
					Invalidate();	//	Cause the MemoryViewerControl to refresh

					bool bView8Bit = ( SendDlgItemMessage( hDlg, IDC_RA_MEMVIEW8BIT, BM_GETCHECK, 0, 0 ) == BST_CHECKED );
					SetDlgItemText( hDlg, IDC_RA_MEMBITS_TITLE, bView8Bit ? TEXT( "Bits: 7 6 5 4 3 2 1 0" ) : TEXT( "" ) );

					MemoryViewerControl::destroyEditCaret();
					return FALSE;
				}

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
					if ( b4BitSet )
						nCompSize = Nibble_Lower;
					else if ( b8BitSet )
						nCompSize = EightBit;
					else if ( b16BitSet )
						nCompSize = SixteenBit;
					else //if( b32BitSet )
						nCompSize = ThirtyTwoBit;

					ByteAddress start, end;
					if ( GetSelectedMemoryRange( start, end ) )
					{
						g_MemManager.ResetAll( nCompSize, start, end );

						m_nStart = start;
						m_nEnd = end;
						m_nCompareSize = nCompSize;

						ClearLogOutput();
						m_nPage = 0;
						m_SearchResults.clear();
						SearchResult sr;
						m_SearchResults.push_back( sr );
						m_SearchResults[ m_nPage ].m_nCount = g_MemManager.NumCandidates();
						m_SearchResults[ m_nPage ].m_sFirstLine =
							"Cleared: (" + NativeStr( std::string( COMPARISONVARIABLESIZE_STR[ nCompSize ] ) ) +
							") mode. Aware of " + NativeStr( std::to_string( g_MemManager.NumCandidates() ) ) + " RAM locations.";
						EnableWindow( GetDlgItem( hDlg, IDC_RA_DOTEST ), g_MemManager.NumCandidates() > 0 );
					}
					else
					{
						ClearLogOutput();
						m_nPage = 0;
						m_SearchResults.clear();
						SearchResult sr;
						m_SearchResults.push_back( sr );
						m_SearchResults[ m_nPage ].m_sFirstLine = "Invalid Range";
					}

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

				case IDC_RA_CBO_SEARCHALL:
				case IDC_RA_CBO_SEARCHCUSTOM:
				case IDC_RA_CBO_SEARCHSYSTEMRAM:
				case IDC_RA_CBO_SEARCHGAMERAM:
					EnableWindow( GetDlgItem( hDlg, IDC_RA_SEARCHRANGE ), IsDlgButtonChecked( hDlg, IDC_RA_CBO_SEARCHCUSTOM ) == BST_CHECKED );
					return TRUE;

				case IDC_RA_ADDNOTE:
				{
					HWND hMemWatch = GetDlgItem( hDlg, IDC_RA_WATCHING );
					HWND hMemDescription = GetDlgItem( hDlg, IDC_RA_MEMSAVENOTE );

					TCHAR sAddressWide[ 16 ];
					ComboBox_GetText( hMemWatch, sAddressWide, 16 );
					const std::string sAddress = Narrow( sAddressWide );

					if ( sAddress[ 0 ] != '0' || sAddress[ 1 ] != 'x' )
						return FALSE;

					TCHAR sNewNoteWide[ 512 ];
					GetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, sNewNoteWide, 512 );
					const std::string sNewNote = Narrow( sNewNoteWide );

					const ByteAddress nAddr = static_cast<ByteAddress>( std::strtoul( sAddress.c_str() + 2, nullptr, 16 ) );
					const CodeNotes::CodeNoteObj* pSavedNote = m_CodeNotes.FindCodeNote( nAddr );
					if ( ( pSavedNote != nullptr ) && ( pSavedNote->Note().length() > 0 ) )
					{
						if ( pSavedNote->Note().compare( sNewNote ) != 0 )	//	New note is different
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

							if ( MessageBox( hDlg, NativeStr( sWarning ).c_str(), TEXT( "Warning: overwrite note?" ), MB_YESNO ) == IDYES )
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
						ComboBox_AddString( hMemWatch, NativeStr( sAddress ).c_str() );
					}

					return FALSE;
				}

				case IDC_RA_REMNOTE:
				{
					HWND hMemWatch = GetDlgItem( hDlg, IDC_RA_WATCHING );
					HWND hMemDescription = GetDlgItem( hDlg, IDC_RA_MEMSAVENOTE );

					TCHAR sAddressWide[ 16 ];
					ComboBox_GetText( hMemWatch, sAddressWide, 16 );
					const std::string sAddress = Narrow( sAddressWide );

					TCHAR sDescriptionWide[ 1024 ];
					GetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, sDescriptionWide, 1024 );
					const std::string sDescription = Narrow( sDescriptionWide );

					ByteAddress nAddr = static_cast<ByteAddress>( std::strtoul( sAddress.c_str() + 2, nullptr, 16 ) );

					m_CodeNotes.Remove( nAddr );

					SetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, TEXT( "" ) );

					int nIndex = ComboBox_FindString( hMemWatch, -1, NativeStr( sAddress ).c_str() );
					if ( nIndex != CB_ERR )
						ComboBox_DeleteString( hMemWatch, nIndex );

					ComboBox_SetText( hMemWatch, TEXT( "" ) );

					return FALSE;
				}

				case IDC_RA_OPENPAGE:
				{
					if ( g_pCurrentGameData->GetGameID() != 0 )
					{
						tstring sTarget = "http://" RA_HOST_URL + tstring( "/codenotes.php?g=" ) + std::to_string( g_pCurrentGameData->GetGameID() );
						ShellExecute( NULL,
							_T( "open" ),
							NativeStr( sTarget ).c_str(),
							NULL,
							NULL,
							SW_SHOWNORMAL );
					}
					else
					{
						MessageBox( nullptr, _T( "No ROM loaded!" ), _T( "Error!" ), MB_ICONWARNING );
					}

					return FALSE;
				}

				case IDC_RA_OPENBOOKMARKS:
				{
					if ( g_MemBookmarkDialog.GetHWND() == NULL )
						g_MemBookmarkDialog.InstallHWND( CreateDialog( g_hThisDLLInst, MAKEINTRESOURCE( IDD_RA_MEMBOOKMARK ), hDlg, g_MemBookmarkDialog.s_MemBookmarkDialogProc ) );
					if ( g_MemBookmarkDialog.GetHWND() != NULL )
						ShowWindow( g_MemBookmarkDialog.GetHWND(), SW_SHOW );

					return FALSE;
				}

				case IDC_RA_RESULTS_BACK:
				{
					m_nPage--;

					if ( m_SearchResults[m_nPage].m_sSecondLine.length() > 0)
					{
						g_MemManager.ChangeNumCandidates ( m_SearchResults[ m_nPage ].m_nCount );
						MemCandidate* candidate = g_MemManager.GetCandidatePointer();

						for ( unsigned int i = 0; i < m_SearchResults[ m_nPage ].m_ResultCandidate.size(); i++ )
						{
							candidate[ i ].m_nAddr = m_SearchResults[ m_nPage ].m_ResultCandidate[ i ].m_nAddr;
							candidate[ i ].m_nLastKnownValue = m_SearchResults[ m_nPage ].m_ResultCandidate[ i ].m_nLastKnownValue;
							candidate[ i ].m_bUpperNibble = m_SearchResults[ m_nPage ].m_ResultCandidate[ i ].m_bUpperNibble;
							candidate[ i ].m_bHasChanged = m_SearchResults[ m_nPage ].m_ResultCandidate[ i ].m_bHasChanged;
						}

						if ( m_nPage != 0 )
							EnableWindow( GetDlgItem( hDlg, IDC_RA_RESULTS_FORWARD ), TRUE );
						else
							EnableWindow( GetDlgItem( hDlg, IDC_RA_RESULTS_BACK ), FALSE );

						ListView_SetItemCount( GetDlgItem( hDlg, IDC_RA_MEM_LIST ), m_SearchResults[ m_nPage ].m_nCount + 2 );
					}
					else
					{
						g_MemManager.ResetAll( m_nCompareSize, m_nStart, m_nEnd );

						g_MemManager.ChangeNumCandidates ( m_SearchResults[ m_nPage ].m_nCount );
						EnableWindow( GetDlgItem( hDlg, IDC_RA_RESULTS_BACK ), FALSE );
						ListView_SetItemCount( GetDlgItem( hDlg, IDC_RA_MEM_LIST ), 1 );
					}

					return FALSE;
				}

				case IDC_RA_RESULTS_FORWARD:
				{
					m_nPage++;

					g_MemManager.ChangeNumCandidates ( m_SearchResults[ m_nPage ].m_nCount );
					MemCandidate* candidate = g_MemManager.GetCandidatePointer();

					for ( unsigned int i = 0; i < m_SearchResults[ m_nPage ].m_ResultCandidate.size(); i++ )
					{
						candidate[ i ].m_nAddr = m_SearchResults[ m_nPage ].m_ResultCandidate[ i ].m_nAddr;
						candidate[ i ].m_nLastKnownValue = m_SearchResults[ m_nPage ].m_ResultCandidate[ i ].m_nLastKnownValue;
						candidate[ i ].m_bUpperNibble = m_SearchResults[ m_nPage ].m_ResultCandidate[ i ].m_bUpperNibble;
						candidate[ i ].m_bHasChanged = m_SearchResults[ m_nPage ].m_ResultCandidate[ i ].m_bHasChanged;
					}

					EnableWindow( GetDlgItem( m_hWnd, IDC_RA_RESULTS_BACK ), TRUE );
					if ( m_nPage == m_SearchResults.size() - 1 )
						EnableWindow( GetDlgItem( m_hWnd, IDC_RA_RESULTS_FORWARD ), FALSE );

					ListView_SetItemCount( GetDlgItem( hDlg, IDC_RA_MEM_LIST ), m_SearchResults[ m_nPage ].m_nCount + 2 );
					return FALSE;
				}

				case IDC_RA_RESULTS_REMOVE:
				{
					HWND hList = GetDlgItem( hDlg, IDC_RA_MEM_LIST );
					int nSel = ListView_GetNextItem( hList, -1, LVNI_SELECTED );

					if ( nSel != -1 )
					{
						while ( m_SearchResults.size() > m_nPage + 1 )
							m_SearchResults.pop_back();

						SearchResult sr = m_SearchResults[ m_nPage ];
						m_SearchResults.push_back( sr );
						m_nPage++;

						while ( nSel >= 0 )
						{
							m_SearchResults[ m_nPage ].m_ResultCandidate.erase(
								m_SearchResults[ m_nPage ].m_ResultCandidate.begin() + ( nSel - 2 ) );

							m_SearchResults[ m_nPage ].m_nCount--;
							ListView_DeleteItem( hList, nSel );
							nSel = ListView_GetNextItem( hList, -1, LVNI_SELECTED );
						}

						MemCandidate* candidate = g_MemManager.GetCandidatePointer();

						for ( unsigned int i = 0; i < m_SearchResults[ m_nPage ].m_ResultCandidate.size(); i++ )
						{
							candidate[ i ].m_nAddr = m_SearchResults[ m_nPage ].m_ResultCandidate[ i ].m_nAddr;
							candidate[ i ].m_nLastKnownValue = m_SearchResults[ m_nPage ].m_ResultCandidate[ i ].m_nLastKnownValue;
							candidate[ i ].m_bUpperNibble = m_SearchResults[ m_nPage ].m_ResultCandidate[ i ].m_bUpperNibble;
							candidate[ i ].m_bHasChanged = m_SearchResults[ m_nPage ].m_ResultCandidate[ i ].m_bHasChanged;
						}

						TCHAR buffer[ 1024 ];
						if ( m_SearchResults[ m_nPage ].m_nCount > MIN_RESULTS_TO_DUMP )
							_stprintf_s( buffer, sizeof( buffer ), _T( "Found %d matches! (Displaying first %d results)" ), m_SearchResults[ m_nPage ].m_nCount, MIN_RESULTS_TO_DUMP );
						else
							_stprintf_s( buffer, sizeof( buffer ), _T( "Found %d matches!" ), m_SearchResults[ m_nPage ].m_nCount );

						m_SearchResults[ m_nPage ].m_sSecondLine = buffer;
					}


					return FALSE;
				}

				case IDC_RA_WATCHING:
					switch ( HIWORD( wParam ) )
					{
						case CBN_SELCHANGE:
						{
							HWND hMemWatch = GetDlgItem( hDlg, IDC_RA_WATCHING );
							int nSel = ComboBox_GetCurSel( hMemWatch );
							if ( nSel != CB_ERR )
							{
								TCHAR sAddr[ 64 ];
								if ( ComboBox_GetLBText( hMemWatch, nSel, sAddr ) > 0 )
								{
									ByteAddress nAddr = static_cast<ByteAddress>( std::strtoul( Narrow( sAddr ).c_str(), nullptr, 16 ) );
									const CodeNotes::CodeNoteObj* pSavedNote = m_CodeNotes.FindCodeNote( nAddr );
									if ( pSavedNote != NULL && pSavedNote->Note().length() > 0 )
										SetDlgItemText( hDlg, IDC_RA_MEMSAVENOTE, NativeStr( pSavedNote->Note() ).c_str() );

									MemoryViewerControl::setAddress( ( nAddr & ~( 0xf ) ) - ( (int)( MemoryViewerControl::m_nDisplayedLines / 2 ) << 4 ) + ( 0x50 ) );
								}
							}

							Invalidate();
							return TRUE;
						}
						case CBN_EDITCHANGE:
						{
							OnWatchingMemChange();

							TCHAR sAddrBuffer[ 64 ];
							GetDlgItemText( hDlg, IDC_RA_WATCHING, sAddrBuffer, 64 );
							ByteAddress nAddr = static_cast<ByteAddress>( std::strtoul( Narrow( sAddrBuffer ).c_str(), nullptr, 16 ) );
							MemoryViewerControl::setAddress( ( nAddr & ~( 0xf ) ) - ( (int)( MemoryViewerControl::m_nDisplayedLines / 2 ) << 4 ) + ( 0x50 ) );
							return TRUE;
						}

						default:
							return FALSE;
							//return DefWindowProc( hDlg, nMsg, wParam, lParam );
					}

				case IDC_RA_MEMBANK:
					switch ( HIWORD( wParam ) )
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

	return FALSE;
}

void Dlg_Memory::OnWatchingMemChange()
{
	TCHAR sAddrNative[1024];
	GetDlgItemText(m_hWnd, IDC_RA_WATCHING, sAddrNative, 1024);
	std::string sAddr = Narrow(sAddrNative);
	ByteAddress nAddr = static_cast<ByteAddress>(std::strtoul(sAddr.c_str() + 2, nullptr, 16));

	const CodeNotes::CodeNoteObj* pSavedNote = m_CodeNotes.FindCodeNote(nAddr);
	SetDlgItemText(m_hWnd, IDC_RA_MEMSAVENOTE, NativeStr((pSavedNote != nullptr) ? pSavedNote->Note() : "").c_str());

	MemoryViewerControl::destroyEditCaret();

	Invalidate();
}

void Dlg_Memory::RepopulateMemNotesFromFile()
{
	HWND hMemWatch = GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_WATCHING);
	if (hMemWatch != NULL)
	{
		SetDlgItemText(hMemWatch, IDC_RA_WATCHING, TEXT(""));
		SetDlgItemText(hMemWatch, IDC_RA_MEMSAVENOTE, TEXT(""));

		while (ComboBox_DeleteString(hMemWatch, 0) != CB_ERR) {}

		GameID nGameID = g_pCurrentGameData->GetGameID();
		if (nGameID != 0)
		{
			char sNotesFilename[1024];
			sprintf_s(sNotesFilename, 1024, "%s%d-Notes2.txt", RA_DIR_DATA, nGameID);
			size_t nSize = m_CodeNotes.Load(sNotesFilename);

			//	Issue a fetch instead!
			std::map<ByteAddress, CodeNotes::CodeNoteObj>::const_iterator iter = m_CodeNotes.FirstNote();
			while (iter != m_CodeNotes.EndOfNotes())
			{
				const std::string sAddr = ByteAddressToString(iter->first);
				ComboBox_AddString(hMemWatch, NativeStr(sAddr).c_str());
				iter++;
			}

			if (nSize > 0)
			{
				//	Select the first one.
				ComboBox_SetCurSel(hMemWatch, 0);

				//	Note: as this is sorted, we should grab the desc again
				TCHAR sAddrBuffer[64];
				ComboBox_GetLBText(hMemWatch, 0, sAddrBuffer);
				const std::string sAddr = Narrow(sAddrBuffer);

				ByteAddress nAddr = static_cast<ByteAddress>(std::strtoul(sAddr.c_str() + 2, nullptr, 16));
				const CodeNotes::CodeNoteObj* pSavedNote = m_CodeNotes.FindCodeNote(nAddr);
				if ((pSavedNote != nullptr) && (pSavedNote->Note().length() > 0))
				{
					SetDlgItemText(m_hWnd, IDC_RA_MEMSAVENOTE, NativeStr(pSavedNote->Note()).c_str());
				}
			}
		}
	}
}

void Dlg_Memory::OnLoad_NewRom()
{
	m_CodeNotes.ReloadFromWeb(g_pCurrentGameData->GetGameID());

	SetDlgItemText(g_MemoryDialog.m_hWnd, IDC_RA_MEM_LIST, TEXT(""));
	SetDlgItemText(g_MemoryDialog.m_hWnd, IDC_RA_MEMSAVENOTE, TEXT(""));
	if (g_pCurrentGameData->GetGameID() == 0)
		SetDlgItemText(g_MemoryDialog.m_hWnd, IDC_RA_WATCHING, TEXT(""));
	else
		SetDlgItemText(g_MemoryDialog.m_hWnd, IDC_RA_WATCHING, TEXT("Loading..."));

	if (g_MemManager.TotalBankSize() > 0)
	{
		ByteAddress start, end;
		if (GetSystemMemoryRange(start, end))
		{
			TCHAR label[64];
			if (g_MemManager.TotalBankSize() > 0x10000)
				wsprintf(label, TEXT("System Memory (0x%06X-0x%06X)"), start, end);
			else
				wsprintf(label, TEXT("System Memory (0x%04X-0x%04X)"), start, end);

			SetDlgItemText(g_MemoryDialog.m_hWnd, IDC_RA_CBO_SEARCHSYSTEMRAM, label);
			EnableWindow( GetDlgItem( g_MemoryDialog.m_hWnd, IDC_RA_CBO_SEARCHSYSTEMRAM ), TRUE );
		}
		else
		{
			SetDlgItemText(g_MemoryDialog.m_hWnd, IDC_RA_CBO_SEARCHSYSTEMRAM, TEXT("System Memory (unspecified)"));
			EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_CBO_SEARCHSYSTEMRAM), FALSE);
		}

		if (GetGameMemoryRange(start, end))
		{
			TCHAR label[64];
			if (g_MemManager.TotalBankSize() > 0x10000)
				wsprintf(label, TEXT("Game Memory (0x%06X-0x%06X)"), start, end);
			else
				wsprintf(label, TEXT("Game Memory (0x%04X-0x%04X)"), start, end);

			SetDlgItemText(g_MemoryDialog.m_hWnd, IDC_RA_CBO_SEARCHGAMERAM, label);
			EnableWindow( GetDlgItem( g_MemoryDialog.m_hWnd, IDC_RA_CBO_SEARCHGAMERAM ), TRUE );
		}
		else
		{
			SetDlgItemText(g_MemoryDialog.m_hWnd, IDC_RA_CBO_SEARCHGAMERAM, TEXT("Game Memory (unspecified)"));
			EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_CBO_SEARCHGAMERAM), FALSE);
		}
	}

	RepopulateMemNotesFromFile();

	MemoryViewerControl::destroyEditCaret();
}

void Dlg_Memory::Invalidate()
{
	MemoryViewerControl::Invalidate();

	if ( g_MemBookmarkDialog.GetHWND() != nullptr )
		g_MemBookmarkDialog.UpdateBookmarks( FALSE );
}

void Dlg_Memory::SetWatchingAddress(unsigned int nAddr)
{
	char buffer[32];
	sprintf_s(buffer, 32, "0x%06x", nAddr);
	SetDlgItemText(g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, NativeStr(buffer).c_str());

	OnWatchingMemChange();
}

BOOL Dlg_Memory::IsActive() const
{
	return(g_MemoryDialog.GetHWND() != NULL) && (IsWindowVisible(g_MemoryDialog.GetHWND()));
}

void Dlg_Memory::ClearBanks()
{
	if (m_hWnd == nullptr)
		return;

	HWND hMemBanks = GetDlgItem(m_hWnd, IDC_RA_MEMBANK);
	while (ComboBox_DeleteString(hMemBanks, 0) != CB_ERR) {}
}

void Dlg_Memory::AddBank(size_t nBankID)
{
	if (m_hWnd == nullptr)
		return;

	HWND hMemBanks = GetDlgItem(m_hWnd, IDC_RA_MEMBANK);
	int nIndex = ComboBox_AddString(hMemBanks, NativeStr(std::to_string(nBankID)).c_str());
	if (nIndex != CB_ERR)
	{
		ComboBox_SetItemData(hMemBanks, nIndex, nBankID);
	}

	//	Select first element by default ('0')
	ComboBox_SetCurSel(hMemBanks, 0);
}

bool Dlg_Memory::GetSystemMemoryRange(ByteAddress& start, ByteAddress& end)
{
	switch (g_ConsoleID)
	{
	case ConsoleID::NES:
		// $0000-$07FF are the 2KB internal RAM for the NES. It's mirrored every 2KB until $1FFF.
		start = 0x0000;
		end = 0x07FF;
		return TRUE;

	case ConsoleID::SNES:
		// SNES RAM runs from $7E0000-$7FFFFF. $7E0000-$7E1FFF is LowRAM, $7E2000-$7E7FFF is 
		// considered HighRAM, and $7E8000-$7FFFFF is considered Expanded RAM. The Memory Manager
		// addresses this data from $000000-$1FFFFF. For simplicity, we'll treat LowRAM and HighRAM
		// as system memory and Expanded RAM as game memory.
		start = 0x0000;
		end = 0x7FFF;
		return TRUE;

	case ConsoleID::GB:
	case ConsoleID::GBC:
		// $C000-$CFFF is fixed internal RAM, $D000-$DFFF is banked internal RAM. $C000-$DFFF is
		// mirrored to $E000-$FDFF.
		start = 0xC000;
		end = 0xDFFF;
		return TRUE;

	case ConsoleID::GBA:
		// Gameboy Advance memory has three separate memory blocks. $02000000-$0203FFFF is
		// on-board RAM, $03000000-$03007FF0 is in-chip RAM, and $0E000000-$0E00FFFF is
		// game pak RAM. The Memory Manager sees the in-chip RAM as $00000-$07FF0, and the
		// on-board as $8000-$47FFF
		start = 0x8000;
		end = 0x47FFF;
		return TRUE;

	case ConsoleID::MegaDrive:
		// Genesis RAM runs from $E00000-$FFFFFF. It's really only 64KB, and mirrored for 
		// each $E0-$FF lead byte. Typically, it's only accessed from $FFxxxx. I'm not sure
		// what memory is represented by $10000-$1FFFF in the Memory Manager.
		start = 0x0000;
		end = 0xFFFF;
		return TRUE;

	case ConsoleID::MasterSystem:
		// $C000-$DFFF is system RAM. It's mirrored at $E000-$FFFF.
		start = 0xC000;
		end = 0xDFFF;
		return TRUE;

	case ConsoleID::N64:
		// $00000-$3FFFFF covers the standard 4MB RDRAM.
		// $00000-$1FFFFF is RDRAM range 0, while $200000-$3FFFFF is RDRAM range 1.
		start = 0x000000;
		end = 0x3FFFFF;
		return TRUE;

	default:
		start = 0;
		end = 0;
		return FALSE;
	}
}

bool Dlg_Memory::GetGameMemoryRange(ByteAddress& start, ByteAddress& end)
{
	switch (g_ConsoleID)
	{
	case ConsoleID::NES:
		// $4020-$FFFF is cartridge memory and subranges will vary by mapper. The most common mappers reference
		// battery-backed RAM or additional work RAM in the $6000-$7FFF range. $8000-$FFFF is usually read-only.
		start = 0x4020;
		end = 0xFFFF;
		return TRUE;

	case ConsoleID::SNES:
		// SNES RAM runs from $7E0000-$7FFFFF. $7E0000-$7E1FFF is LowRAM, $7E2000-$7E7FFF is 
		// considered HighRAM, and $7E8000-$7FFFFF is considered Expanded RAM. The Memory Manager
		// addresses this data from $000000-$1FFFFF. For simplicity, we'll treat LowRAM and HighRAM
		// as system memory and Expanded RAM as game memory.
		start = 0x008000;
		end = 0x1FFFFF;
		return TRUE;

	case ConsoleID::GB:
	case ConsoleID::GBC:
		// $A000-$BFFF is 8KB is external RAM (in cartridge)
		start = 0xA000;
		end = 0xBFFF;
		return TRUE;

	case ConsoleID::GBA:
		// Gameboy Advance memory has three separate memory blocks. $02000000-$0203FFFF is
		// on-board RAM, $03000000-$03007FF0 is in-chip RAM, and $0E000000-$0E00FFFF is
		// game pak RAM. The Memory Manager sees the in-chip RAM as $00000-$07FF0, and the
		// on-board as $8000-$47FFF
		start = 0x0000;
		end = 0x7FFF;
		return TRUE;

	case ConsoleID::MasterSystem:
		// $0000-$BFFF is cartridge ROM/RAM
		start = 0x0000;
		end = 0xBFFF;
		return TRUE;

	case ConsoleID::N64:
		// This range contains the extra 4MB provided by the Expansion Pak.
		// Only avaliable when memory size is greater than 4MB.
		if ( g_MemManager.TotalBankSize() > 0x400000 )
		{
			start = 0x400000;
			end = 0x7FFFFF;
			return TRUE;
		}
		else
		{
			return FALSE;
		}
		
	default:
		start = 0;
		end = 0;
		return FALSE;
	}
}

static TCHAR* ParseAddress(TCHAR* ptr, ByteAddress& address)
{
	if (*ptr == '$')
		++ptr;
	else if (ptr[0] == '0' && ptr[1] == 'x')
		ptr += 2;

	address = 0;
	while (*ptr)
	{
		if (*ptr >= '0' && *ptr <= '9')
		{
			address <<= 4;
			address += (*ptr - '0');
		}
		else if (*ptr >= 'a' && *ptr <= 'f')
		{
			address <<= 4;
			address += (*ptr - 'a' + 10);
		}
		else if (*ptr >= 'A' && *ptr <= 'F')
		{
			address <<= 4;
			address += (*ptr - 'A' + 10);
		}
		else
		{
			break;
		}

		++ptr;
	}

	return ptr;
}

bool Dlg_Memory::GetSelectedMemoryRange( ByteAddress& start, ByteAddress& end )
{
	if ( IsDlgButtonChecked( m_hWnd, IDC_RA_CBO_SEARCHALL ) == BST_CHECKED )
	{
		// all items are in "All" range
		start = 0;
		end = 0xFFFFFFFF;
		return TRUE;
	}

	if ( IsDlgButtonChecked( m_hWnd, IDC_RA_CBO_SEARCHSYSTEMRAM ) == BST_CHECKED )
		return GetSystemMemoryRange( start, end );

	if ( IsDlgButtonChecked( m_hWnd, IDC_RA_CBO_SEARCHGAMERAM ) == BST_CHECKED )
		return GetGameMemoryRange( start, end );

	if ( IsDlgButtonChecked( m_hWnd, IDC_RA_CBO_SEARCHCUSTOM ) == BST_CHECKED )
	{
		TCHAR buffer[ 128 ];
		GetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_SEARCHRANGE, buffer, 128 );

		TCHAR* ptr = ParseAddress( buffer, start );
		while ( iswspace( *ptr ) )
			++ptr;

		if ( *ptr != '-' )
			return FALSE;
		++ptr;

		while ( iswspace( *ptr ) )
			++ptr;

		ptr = ParseAddress( ptr, end );
		return ( *ptr == '\0' );
	}

	return FALSE;
}

void Dlg_Memory::UpdateSearchResult( unsigned int index, unsigned int &nMemVal, TCHAR ( &buffer )[ 1024 ] )
{
	unsigned int element = index - 2;

	const DWORD nAddr = m_SearchResults[ m_nPage ].m_ResultCandidate[ element ].m_nAddr;

	if ( g_MemManager.MemoryComparisonSize() == ThirtyTwoBit )
		nMemVal = ( g_MemManager.ActiveBankRAMByteRead( nAddr ) | ( g_MemManager.ActiveBankRAMByteRead( nAddr + 1 ) << 8 ) );
	if ( g_MemManager.MemoryComparisonSize() == SixteenBit )
		nMemVal = ( g_MemManager.ActiveBankRAMByteRead( nAddr ) | ( g_MemManager.ActiveBankRAMByteRead( nAddr + 1 ) << 8 ) );
	else if ( g_MemManager.MemoryComparisonSize() == EightBit )
		nMemVal = ( g_MemManager.ActiveBankRAMByteRead( nAddr ) );
	else if ( g_MemManager.MemoryComparisonSize() == Nibble_Lower || g_MemManager.MemoryComparisonSize() == Nibble_Upper )
	{
		if ( m_SearchResults[ m_nPage ].m_ResultCandidate[ element ].m_bUpperNibble )
			nMemVal = ( ( g_MemManager.ActiveBankRAMByteRead( nAddr ) >> 4 ) & 0xf );
		else
			nMemVal = ( g_MemManager.ActiveBankRAMByteRead( nAddr ) & 0xf );
	}

	if ( g_MemManager.MemoryComparisonSize() == ThirtyTwoBit )
		_stprintf_s ( buffer, sizeof( buffer ), _T( "0x%06x: 0x%08x" ), nAddr, nMemVal );
	if ( g_MemManager.MemoryComparisonSize() == SixteenBit )
		_stprintf_s ( buffer, sizeof( buffer ), _T( "0x%06x: 0x%04x" ), nAddr, nMemVal );
	else if ( g_MemManager.MemoryComparisonSize() == EightBit )
		_stprintf_s ( buffer, sizeof( buffer ), _T( "0x%06x: 0x%02x" ), nAddr, nMemVal );
	else if ( g_MemManager.MemoryComparisonSize() == Nibble_Lower || g_MemManager.MemoryComparisonSize() == Nibble_Upper )
		_stprintf_s ( buffer, sizeof( buffer ), _T( "0x%06x: 0x%01x" ), nAddr, nMemVal );
}

bool Dlg_Memory::CompareSearchResult( unsigned int nCurVal, unsigned int nPrevVal )
{
	unsigned int nVal = ( m_SearchResults[ m_nPage ].m_bUseLastValue ) ?
		nPrevVal : m_SearchResults[ m_nPage ].m_nLastQueryVal;
	bool bResult = false;

	switch ( m_SearchResults[ m_nPage ].m_nCompareType )
	{
		case Equals:				bResult = ( nCurVal == nVal );	break;
		case LessThan:				bResult = ( nCurVal < nVal );	break;
		case LessThanOrEqual:		bResult = ( nCurVal <= nVal );	break;
		case GreaterThan:			bResult = ( nCurVal > nVal );	break;
		case GreaterThanOrEqual:	bResult = ( nCurVal >= nVal );	break;
		case NotEqualTo:			bResult = ( nCurVal != nVal );	break;
		default:
			bResult = false;
			break;
	}
	return bResult;
}

void Dlg_Memory::GenerateResizes(HWND hDlg)
{
	RECT windowRect;
	GetWindowRect(hDlg, &windowRect);
	nDlgMemoryMinX = windowRect.right - windowRect.left;
	nDlgMemoryMinY = windowRect.bottom - windowRect.top;

	RECT itemRect;
	GetWindowRect(GetDlgItem(hDlg, IDC_RA_MEMTEXTVIEWER), &itemRect);
	nDlgMemViewerGapY = itemRect.bottom - windowRect.bottom;
}