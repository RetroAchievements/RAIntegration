#include "RA_Dlg_MemBookmark.h"

#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_GameData.h"
#include "RA_Dlg_Memory.h"
#include "RA_MemManager.h"

#include <strsafe.h>

Dlg_MemBookmark g_MemBookmarkDialog;
std::vector<ResizeContent> vDlgMemBookmarkResize;
POINT pDlgMemBookmarkMin;

WNDPROC EOldProcBM;
HWND g_hIPEEditBM;
int nSelItemBM;
int nSelSubItemBM;

namespace
{
	const char* COLUMN_TITLE[] = { "Description", "Address", "Value", "Prev.", "Changes" };
	const int COLUMN_WIDTH[] = { 112, 64, 64, 64, 54 };
	static_assert( SIZEOF_ARRAY( COLUMN_TITLE ) == SIZEOF_ARRAY( COLUMN_WIDTH ), "Must match!" );
}

const COMDLG_FILTERSPEC c_rgFileTypes[] =
{
	{ L"Text Document (*.txt)",       L"*.txt" }
};

enum BookmarkSubItems
{
	CSI_DESC,
	CSI_ADDRESS,
	CSI_VALUE,
	CSI_PREVIOUS,
	CSI_CHANGES,

	NumColumns
};

INT_PTR CALLBACK Dlg_MemBookmark::s_MemBookmarkDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return g_MemBookmarkDialog.MemBookmarkDialogProc( hDlg, uMsg, wParam, lParam );
}

long _stdcall EditProcBM( HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam )
{
	switch ( nMsg )
	{
		case WM_DESTROY:
			g_hIPEEditBM = nullptr;
			break;

		case WM_KILLFOCUS:
		{
			LV_DISPINFO lvDispinfo;
			ZeroMemory( &lvDispinfo, sizeof( LV_DISPINFO ) );
			lvDispinfo.hdr.hwndFrom = hwnd;
			lvDispinfo.hdr.idFrom = GetDlgCtrlID( hwnd );
			lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
			lvDispinfo.item.mask = LVIF_TEXT;
			lvDispinfo.item.iItem = nSelItemBM;
			lvDispinfo.item.iSubItem = nSelSubItemBM;
			lvDispinfo.item.pszText = nullptr;

			wchar_t sEditText[ 256 ];
			GetWindowTextW( hwnd, sEditText, 256 );
			g_MemBookmarkDialog.Bookmarks()[ nSelItemBM ]->SetDescription( sEditText );

			HWND hList = GetDlgItem( g_MemBookmarkDialog.GetHWND(), IDC_RA_LBX_ADDRESSES );

			//	the LV ID and the LVs Parent window's HWND
			SendMessage( GetParent( hList ), WM_NOTIFY, static_cast<WPARAM>( IDC_RA_LBX_ADDRESSES ), reinterpret_cast<LPARAM>( &lvDispinfo ) );	//	##reinterpret? ##SD

			DestroyWindow( hwnd );
			break;
		}

		case WM_KEYDOWN:
		{
			if ( wParam == VK_RETURN || wParam == VK_ESCAPE )
			{
				DestroyWindow( hwnd );	//	Causing a killfocus :)
			}
			else
			{
				//	Ignore keystroke, or pass it into the edit box!
				break;
			}

			break;
		}
	}

	return CallWindowProc( EOldProcBM, hwnd, nMsg, wParam, lParam );
}

INT_PTR Dlg_MemBookmark::MemBookmarkDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	PMEASUREITEMSTRUCT pmis;
	PDRAWITEMSTRUCT pdis;
	int nSelect;
	HWND hList;
	int offset = 2;

	RECT rcBounds, rcLabel;

	switch ( uMsg )
	{
		case WM_INITDIALOG:
		{
			GenerateResizes( hDlg );

			m_hMemBookmarkDialog = hDlg;
			hList = GetDlgItem( m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES );

			SetupColumns( hList );

			// Auto-import bookmark file when opening dialog
			if ( g_pCurrentGameData->GetGameID() != 0 )
			{
				std::string file = RA_DIR_BOOKMARKS + std::to_string( g_pCurrentGameData->GetGameID() ) + "-Bookmarks.txt";
				ImportFromFile( file );
			}

			RestoreWindowPosition( hDlg, "Memory Bookmarks", true, false );
			return TRUE;
		}

		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
			lpmmi->ptMinTrackSize = pDlgMemBookmarkMin;
		}
		break;

		case WM_SIZE:
		{
			RARect winRect;
			GetWindowRect( hDlg, &winRect );

			for ( ResizeContent content : vDlgMemBookmarkResize )
				content.Resize( winRect.Width(), winRect.Height() );

			//InvalidateRect( hDlg, NULL, TRUE );
			RememberWindowSize(hDlg, "Memory Bookmarks");
		}
		break;

		case WM_MOVE:
			RememberWindowPosition(hDlg, "Memory Bookmarks");
			break;

		case WM_MEASUREITEM:
			pmis = (PMEASUREITEMSTRUCT)lParam;
			pmis->itemHeight = 16;
			return TRUE;

		case WM_DRAWITEM:
		{
			pdis = (PDRAWITEMSTRUCT)lParam;

			// If there are no list items, skip this message.
			if ( pdis->itemID == -1 )
				break;

			switch ( pdis->itemAction )
			{
				case ODA_SELECT:
				case ODA_DRAWENTIRE:

					hList = GetDlgItem( hDlg, IDC_RA_LBX_ADDRESSES );

					ListView_GetItemRect( hList, pdis->itemID, &rcBounds, LVIR_BOUNDS );
					ListView_GetItemRect( hList, pdis->itemID, &rcLabel, LVIR_LABEL );
					RECT rcCol ( rcBounds );
					rcCol.right = rcCol.left + ListView_GetColumnWidth( hList, 0 );

					// Draw Item Label - Column 0
					wchar_t buffer[ 512 ];
					if ( m_vBookmarks[ pdis->itemID ]->Decimal() )
						swprintf_s ( buffer, 512, L"(D)%s", m_vBookmarks[ pdis->itemID ]->Description().c_str() );
					else
						swprintf_s ( buffer, 512, L"%s", m_vBookmarks[ pdis->itemID ]->Description().c_str() );

					if ( pdis->itemState & ODS_SELECTED )
					{
						SetTextColor( pdis->hDC, GetSysColor( COLOR_HIGHLIGHTTEXT ) );
						SetBkColor( pdis->hDC, GetSysColor( COLOR_HIGHLIGHT ) );
						FillRect( pdis->hDC, &rcBounds, GetSysColorBrush( COLOR_HIGHLIGHT ) );
					}
					else
					{
						SetTextColor( pdis->hDC, GetSysColor( COLOR_WINDOWTEXT ) );

						COLORREF color;

						if ( m_vBookmarks[ pdis->itemID ]->Frozen() )
							color = RGB( 255, 255, 160 );
						else
							color = GetSysColor( COLOR_WINDOW );

						HBRUSH hBrush = CreateSolidBrush( color );
						SetBkColor( pdis->hDC, color );
						FillRect( pdis->hDC, &rcBounds, hBrush );
						DeleteObject( hBrush );
					}

					if ( wcslen( buffer ) > 0 )
					{
						rcLabel.left += ( offset / 2 );
						rcLabel.right -= offset;

						DrawTextW( pdis->hDC, buffer, wcslen( buffer ), &rcLabel, DT_SINGLELINE | DT_LEFT | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER | DT_END_ELLIPSIS );
					}

					// Draw Item Label for remaining columns
					LV_COLUMN lvc;
					lvc.mask = LVCF_FMT | LVCF_WIDTH;

					for ( size_t i = 1; ListView_GetColumn( hList, i, &lvc ); ++i )
					{
						rcCol.left = rcCol.right;
						rcCol.right += lvc.cx;

						switch ( i )
						{
							case CSI_ADDRESS:
								swprintf_s ( buffer, 512, L"%06x", m_vBookmarks[ pdis->itemID ]->Address() );
								break;
							case CSI_VALUE:
								if ( m_vBookmarks[ pdis->itemID ]->Decimal() )
									swprintf_s ( buffer, 512, L"%u", m_vBookmarks[ pdis->itemID ]->Value() );
								else
								{
									switch ( m_vBookmarks[ pdis->itemID ]->Type() )
									{
										case 1: swprintf_s ( buffer, 512, L"%02x", m_vBookmarks[ pdis->itemID ]->Value() ); break;
										case 2: swprintf_s ( buffer, 512, L"%04x", m_vBookmarks[ pdis->itemID ]->Value() ); break;
										case 3: swprintf_s ( buffer, 512, L"%08x", m_vBookmarks[ pdis->itemID ]->Value() ); break;
									}
								}
								break;
							case CSI_PREVIOUS:
								if ( m_vBookmarks[ pdis->itemID ]->Decimal() )
									swprintf_s ( buffer, 512, L"%u", m_vBookmarks[ pdis->itemID ]->Previous() );
								else
								{
									switch ( m_vBookmarks[ pdis->itemID ]->Type() )
									{
										case 1: swprintf_s ( buffer, 512, L"%02x", m_vBookmarks[ pdis->itemID ]->Previous() ); break;
										case 2: swprintf_s ( buffer, 512, L"%04x", m_vBookmarks[ pdis->itemID ]->Previous() ); break;
										case 3: swprintf_s ( buffer, 512, L"%08x", m_vBookmarks[ pdis->itemID ]->Previous() ); break;
									}
								}
								break;
							case CSI_CHANGES:
								swprintf_s ( buffer, 512, L"%d", m_vBookmarks[ pdis->itemID ]->Count() );
								break;
							default:
								swprintf_s ( buffer, 512, L"" );
								break;
						}

						if ( wcslen( buffer ) == 0 )
							continue;

						UINT nJustify = DT_LEFT;
						switch ( lvc.fmt & LVCFMT_JUSTIFYMASK )
						{
							case LVCFMT_RIGHT:
								nJustify = DT_RIGHT;
								break;
							case LVCFMT_CENTER:
								nJustify = DT_CENTER;
								break;
							default:
								break;
						}

						rcLabel = rcCol;
						rcLabel.left += offset;
						rcLabel.right -= offset;

						DrawTextW( pdis->hDC, buffer, wcslen( buffer ), &rcLabel, nJustify | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_END_ELLIPSIS );
					}

					//if (pdis->itemState & ODS_SELECTED) //&& (GetFocus() == this)
					//	DrawFocusRect(pdis->hDC, &rcBounds);

					break;

				case ODA_FOCUS:
					break;
			}
			return TRUE;
		}

		case WM_NOTIFY:
		{
			switch ( LOWORD( wParam ) )
			{
				case IDC_RA_LBX_ADDRESSES:
					if ( ( (LPNMHDR)lParam )->code == NM_CLICK )
					{
						hList = GetDlgItem( hDlg, IDC_RA_LBX_ADDRESSES );

						nSelect = ListView_GetNextItem( hList, -1, LVNI_FOCUSED );

						if ( nSelect == -1 )
							break;
					}
					else if ( ( (LPNMHDR)lParam )->code == NM_DBLCLK )
					{
						hList = GetDlgItem( hDlg, IDC_RA_LBX_ADDRESSES );

						LPNMITEMACTIVATE pOnClick = (LPNMITEMACTIVATE)lParam;

						if ( pOnClick->iItem != -1 && pOnClick->iSubItem == CSI_DESC )
						{
							nSelItemBM = pOnClick->iItem;
							nSelSubItemBM = pOnClick->iSubItem;

							EditLabel ( pOnClick->iItem, pOnClick->iSubItem );
						}
						else if ( pOnClick->iItem != -1 && pOnClick->iSubItem == CSI_ADDRESS )
						{
							g_MemoryDialog.SetWatchingAddress( m_vBookmarks[ pOnClick->iItem ]->Address() );
							MemoryViewerControl::setAddress( ( m_vBookmarks[ pOnClick->iItem ]->Address() & 
								~( 0xf ) ) - ( (int)( MemoryViewerControl::m_nDisplayedLines / 2 ) << 4 ) + ( 0x50 ) );
						}
					}
			}
			return TRUE;
		}
		case WM_COMMAND:
		{
			switch ( LOWORD( wParam ) )
			{
				case IDOK:
				case IDCLOSE:
				case IDCANCEL:
					EndDialog( hDlg, true );
					return TRUE;

				case IDC_RA_ADD_BOOKMARK:
				{
					if ( g_MemoryDialog.GetHWND() != nullptr )
						AddAddress();

					return TRUE;
				}
				case IDC_RA_DEL_BOOKMARK:
				{
					HWND hList = GetDlgItem( hDlg, IDC_RA_LBX_ADDRESSES );
					int nSel = ListView_GetNextItem( hList, -1, LVNI_SELECTED );

					if ( nSel != -1 )
					{
						while ( nSel >= 0 )
						{
							MemBookmark* pBookmark = m_vBookmarks[ nSel ];

							// Remove from vector
							m_vBookmarks.erase( m_vBookmarks.begin() + nSel );

							// Remove from map
							std::vector<const MemBookmark*> *pVector;
							pVector = &m_BookmarkMap.find( pBookmark->Address() )->second;
							pVector->erase( std::find( pVector->begin(), pVector->end(), pBookmark ) );
							if ( pVector->size() == 0 )
								m_BookmarkMap.erase( pBookmark->Address() );

							delete pBookmark;

							ListView_DeleteItem( hList, nSel );

							nSel = ListView_GetNextItem( hList, -1, LVNI_SELECTED );
						}

						InvalidateRect( hList, NULL, FALSE );
					}

					return TRUE;
				}
				case IDC_RA_FREEZE:
				{
					if ( m_vBookmarks.size() > 0 )
					{
						hList = GetDlgItem( hDlg, IDC_RA_LBX_ADDRESSES );
						unsigned int uSelectedCount = ListView_GetSelectedCount( hList );

						if ( uSelectedCount > 0 )
						{
							for ( int i = ListView_GetNextItem( hList, -1, LVNI_SELECTED ); i >= 0; i = ListView_GetNextItem( hList, i, LVNI_SELECTED ) )
								m_vBookmarks[ i ]->SetFrozen( !m_vBookmarks[ i ]->Frozen() );
						}
						ListView_SetItemState( hList, -1, LVIF_STATE, LVIS_SELECTED );
					}
					return TRUE;
				}
				case IDC_RA_CLEAR_CHANGE:
				{
					if ( m_vBookmarks.size() > 0 )
					{
						hList = GetDlgItem( hDlg, IDC_RA_LBX_ADDRESSES );
						int idx = -1;
						for ( MemBookmark* bookmark : m_vBookmarks )
						{
							idx++;

							bookmark->ResetCount();
						}
						
						InvalidateRect( hList, NULL, FALSE );
					}

					return TRUE;
				}
				case IDC_RA_DECIMALBOOKMARK:
				{
					if ( m_vBookmarks.size() > 0 )
					{
						hList = GetDlgItem( hDlg, IDC_RA_LBX_ADDRESSES );
						unsigned int uSelectedCount = ListView_GetSelectedCount( hList );

						if ( uSelectedCount > 0 )
						{
							for ( int i = ListView_GetNextItem( hList, -1, LVNI_SELECTED ); i >= 0; i = ListView_GetNextItem( hList, i, LVNI_SELECTED ) )
								m_vBookmarks[ i ]->SetDecimal( !m_vBookmarks[ i ]->Decimal() );
						}
						ListView_SetItemState( hList, -1, LVIF_STATE, LVIS_SELECTED );
					}
					return TRUE;
				}
				case IDC_RA_SAVEBOOKMARK:
				{
					ExportJSON();
					return TRUE;
				}
				case IDC_RA_LOADBOOKMARK:
				{
					std::string file = ImportDialog();
					if (file.length() > 0 )
						ImportFromFile( file );
					return TRUE;
				}
				default:
					return FALSE;
			}
		}
		default:
			break;
	}

	return FALSE;
}

BOOL Dlg_MemBookmark::IsActive() const
{
	return(g_MemBookmarkDialog.GetHWND() != NULL) && (IsWindowVisible(g_MemBookmarkDialog.GetHWND()));
}

void Dlg_MemBookmark::UpdateBookmarks( bool bForceWrite )
{
	if ( !IsWindowVisible( m_hMemBookmarkDialog ) )
		return;

	if ( m_vBookmarks.size() == 0 )
		return;

	HWND hList = GetDlgItem( m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES );

	int index = 0;
	for ( MemBookmark* bookmark : m_vBookmarks )
	{
		if ( bookmark->Frozen() && !bForceWrite )
		{
			WriteFrozenValue( *bookmark );
			index++;
			continue;
		}

		unsigned int mem_value = GetMemory( bookmark->Address(), bookmark->Type() );

		if ( bookmark->Value() != mem_value )
		{
			bookmark->SetPrevious( bookmark->Value() );
			bookmark->SetValue( mem_value );
			bookmark->IncreaseCount();

			RECT rcBounds;
			ListView_GetItemRect(hList, index, &rcBounds, LVIR_BOUNDS);
			InvalidateRect(hList, &rcBounds, FALSE);
		}

		index++;
	}
}

void Dlg_MemBookmark::PopulateList()
{
	if ( m_vBookmarks.size() == 0 )
		return;

	HWND hList = GetDlgItem( m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES );
	if ( hList == NULL )
		return;

	int topIndex = ListView_GetTopIndex( hList );
	ListView_DeleteAllItems( hList );
	m_nNumOccupiedRows = 0;

	for ( MemBookmark* bookmark : m_vBookmarks )
	{
		LV_ITEM item;
		ZeroMemory( &item, sizeof( item ) );
		item.mask = LVIF_TEXT;
		item.cchTextMax = 256;
		item.iItem = m_nNumOccupiedRows;
		item.iSubItem = 0;
		item.iItem = ListView_InsertItem( hList, &item );

		ASSERT( item.iItem == m_nNumOccupiedRows );

		m_nNumOccupiedRows++;
	}

	ListView_EnsureVisible( hList, m_vBookmarks.size() - 1, FALSE ); // Scroll to bottom.
	//ListView_EnsureVisible( hList, topIndex, TRUE ); // return to last position.
}

void Dlg_MemBookmark::SetupColumns( HWND hList )
{
	//	Remove all columns,
	while ( ListView_DeleteColumn( hList, 0 ) ) {}

	//	Remove all data.
	ListView_DeleteAllItems( hList );

	LV_COLUMN col;
	ZeroMemory( &col, sizeof( col ) );

	for ( size_t i = 0; i < NumColumns; ++i )
	{
		col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
		col.cx = COLUMN_WIDTH[ i ];
		tstring colTitle = NativeStr( COLUMN_TITLE[ i ] ).c_str();
		col.pszText = const_cast<LPTSTR>( colTitle.c_str() );
		col.cchTextMax = 255;
		col.iSubItem = i;

		col.fmt = LVCFMT_CENTER | LVCFMT_FIXED_WIDTH;
		if ( i == NumColumns - 1 )
			col.fmt |= LVCFMT_FILL;

		ListView_InsertColumn( hList, i, (LPARAM)&col );
	}

	m_nNumOccupiedRows = 0;

	BOOL bSuccess = ListView_SetExtendedListViewStyle( hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER );
	bSuccess = ListView_EnableGroupView( hList, FALSE );

}

void Dlg_MemBookmark::AddAddress()
{
	if ( g_pCurrentGameData->GetGameID() == 0 )
		return;

	MemBookmark* NewBookmark = new MemBookmark();

	// Fetch Memory Address from Memory Inspector
	TCHAR buffer[ 256 ];
	GetDlgItemText( g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, buffer, 256 );
	unsigned int nAddr = strtol( Narrow( buffer ).c_str(), nullptr, 16 );
	NewBookmark->SetAddress( nAddr );

	// Check Data Type
	if ( SendDlgItemMessage( g_MemoryDialog.GetHWND(), IDC_RA_MEMVIEW8BIT, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
		NewBookmark->SetType( 1 );
	else if ( SendDlgItemMessage( g_MemoryDialog.GetHWND(), IDC_RA_MEMVIEW16BIT, BM_GETCHECK, 0, 0 ) == BST_CHECKED )
		NewBookmark->SetType( 2 );
	else
		NewBookmark->SetType( 3 );

	// Get Memory Value
	NewBookmark->SetValue( GetMemory( nAddr, NewBookmark->Type() ) );
	NewBookmark->SetPrevious( NewBookmark->Value() );

	// Get Code Note and add as description
	const CodeNotes::CodeNoteObj* pSavedNote = g_MemoryDialog.Notes().FindCodeNote( nAddr );
	if ( ( pSavedNote != nullptr ) && ( pSavedNote->Note().length() > 0 ) )
		NewBookmark->SetDescription( Widen( pSavedNote->Note() ).c_str()  );

	// Add Bookmark to vector and map
	AddBookmark( NewBookmark );
	AddBookmarkMap( NewBookmark );

	PopulateList();
}

void Dlg_MemBookmark::ClearAllBookmarks()
{
	while ( m_vBookmarks.size() > 0 )
	{
		MemBookmark* pBookmark = m_vBookmarks[ 0 ];

		// Remove from vector
		m_vBookmarks.erase( m_vBookmarks.begin() );

		// Remove from map
		std::vector<const MemBookmark*> *pVector;
		pVector = &m_BookmarkMap.find( pBookmark->Address() )->second;
		pVector->erase( std::find( pVector->begin(), pVector->end(), pBookmark ) );
		if ( pVector->size() == 0 )
			m_BookmarkMap.erase( pBookmark->Address() );

		delete pBookmark;
	}

	ListView_DeleteAllItems( GetDlgItem( m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES ) );
}

void Dlg_MemBookmark::WriteFrozenValue( const MemBookmark & Bookmark )
{
	if ( !Bookmark.Frozen() )
		return;

	unsigned int addr;
	unsigned int width;
	int n;
	char c;

	switch ( Bookmark.Type() )
	{
		case 1: 
			addr = Bookmark.Address();		
			width = 2;
			break;
		case 2: 
			addr = Bookmark.Address() + 1;	
			width = 4;
			break;
		case 3: 
			addr = Bookmark.Address() + 3;	
			width = 8;
			break;
		default:
			break;
	}

	char buffer[ 32 ];
	sprintf_s ( buffer, sizeof(buffer), "%0*x", width, Bookmark.Value() );

	for ( unsigned int i = 0; i < strlen( buffer ); i++ )
	{
		c = buffer[ i ];
		n = ( c >= 'a' ) ? ( c - 'a' + 10 ) : ( c - '0' );
		MemoryViewerControl::editData( addr, ( i % 2 != 0 ), n );

		if ( i % 2 != 0 )
			addr--;
	}
}

unsigned int Dlg_MemBookmark::GetMemory( unsigned int nAddr, int type )
{
	unsigned int mem_value;

	switch ( type )
	{
		case 1:
			mem_value = g_MemManager.ActiveBankRAMByteRead( nAddr );
			break;
		case 2:
			mem_value = ( g_MemManager.ActiveBankRAMByteRead( nAddr ) | ( g_MemManager.ActiveBankRAMByteRead( nAddr + 1 ) << 8 ) );
			break;
		case 3:
			mem_value = ( g_MemManager.ActiveBankRAMByteRead( nAddr ) | ( g_MemManager.ActiveBankRAMByteRead( nAddr + 1 ) << 8 ) |
				( g_MemManager.ActiveBankRAMByteRead( nAddr + 2 ) << 16 ) | ( g_MemManager.ActiveBankRAMByteRead( nAddr + 3 ) << 24 ) );
			break;
	}

	return mem_value;
}

void Dlg_MemBookmark::ExportJSON()
{
	if ( g_pCurrentGameData->GetGameID() == 0 )
	{
		MessageBox( nullptr, _T("ROM not loaded: please load a ROM first!"), _T("Error!"), MB_OK );
		return;
	}

	if ( m_vBookmarks.size() == 0)
	{
		MessageBox( nullptr, _T("No bookmarks to save: please create a bookmark before attempting to save."), _T("Error!"), MB_OK );
		return;
	}

	std::string defaultDir = RA_DIR_BOOKMARKS;
	defaultDir.erase ( 0, 2 ); // Removes the characters (".\\")
	defaultDir = g_sHomeDir + defaultDir;

	IFileSaveDialog* pDlg = nullptr;
	HRESULT hr = CoCreateInstance( CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>( &pDlg ) );
	if ( hr == S_OK )
	{
		hr = pDlg->SetFileTypes( ARRAYSIZE( c_rgFileTypes ), c_rgFileTypes );
		if ( hr == S_OK )
		{
			char defaultFileName[ 512 ];
			sprintf_s ( defaultFileName, 512, "%s-Bookmarks.txt", std::to_string( g_pCurrentGameData->GetGameID() ).c_str() );
			hr = pDlg->SetFileName( Widen( defaultFileName ).c_str() );
			if ( hr == S_OK )
			{
				PIDLIST_ABSOLUTE pidl;
				hr = SHParseDisplayName( Widen( defaultDir ).c_str(), NULL, &pidl, SFGAO_FOLDER, 0 );
				if ( hr == S_OK )
				{
					IShellItem* pItem = nullptr;
					SHCreateShellItem( NULL, NULL, pidl, &pItem );
					hr = pDlg->SetDefaultFolder( pItem );
					if ( hr == S_OK )
					{
						pDlg->SetDefaultExtension( L"txt" );
						hr = pDlg->Show( nullptr );
						if ( hr == S_OK )
						{

							hr = pDlg->GetResult( &pItem );
							if ( hr == S_OK )
							{
								LPWSTR pStr = nullptr;
								hr = pItem->GetDisplayName( SIGDN_FILESYSPATH, &pStr );
								if ( hr == S_OK )
								{
									Document doc;
									Document::AllocatorType& allocator = doc.GetAllocator();
									doc.SetObject();

									Value bookmarks( kArrayType );
									for ( MemBookmark* bookmark : m_vBookmarks )
									{
										Value item( kObjectType );
										char buffer[ 256 ];
										sprintf_s( buffer, Narrow( bookmark->Description() ).c_str(), sizeof( buffer ) );
										Value s( buffer, allocator );

										item.AddMember( "Description", s, allocator );
										item.AddMember( "Address", bookmark->Address(), allocator );
										item.AddMember( "Type", bookmark->Type(), allocator );
										item.AddMember( "Decimal", bookmark->Decimal(), allocator );
										bookmarks.PushBack( item, allocator );
									}
									doc.AddMember( "Bookmarks", bookmarks, allocator );

									_WriteBufferToFile( Narrow( pStr ), doc );
								}

								pItem->Release();
								ILFree( pidl );
							}
						}
					}
				}
			}
		}
		pDlg->Release();
	}
}

void Dlg_MemBookmark::ImportFromFile( std::string sFilename )
{
	FILE* pFile = nullptr;
	errno_t nErr = fopen_s( &pFile, sFilename.c_str(), "r" );
	if ( pFile != nullptr )
	{
		Document doc;
		doc.ParseStream( FileStream( pFile ) );
		if ( !doc.HasParseError() )
		{
			if ( doc.HasMember( "Bookmarks" ) )
			{
				ClearAllBookmarks();

				const Value& BookmarksData = doc[ "Bookmarks" ];
				for ( SizeType i = 0; i < BookmarksData.Size(); ++i )
				{
					MemBookmark* NewBookmark = new MemBookmark();

					wchar_t buffer[ 256 ];
					swprintf_s ( buffer, 256, L"%s", Widen( BookmarksData[ i ][ "Description" ].GetString() ).c_str() );
					NewBookmark->SetDescription ( buffer );

					NewBookmark->SetAddress( BookmarksData[ i ][ "Address" ].GetUint() );
					NewBookmark->SetType( BookmarksData[ i ][ "Type" ].GetInt() );
					NewBookmark->SetDecimal( BookmarksData[ i ][ "Decimal" ].GetBool() );

					NewBookmark->SetValue( GetMemory( NewBookmark->Address(), NewBookmark->Type() ) );
					NewBookmark->SetPrevious ( NewBookmark->Value() );

					AddBookmark ( NewBookmark );
					AddBookmarkMap( NewBookmark );
				}

				if ( m_vBookmarks.size() > 0 )
					PopulateList();
			}
			else
			{
				ASSERT ( " !Invalid Bookmark File..." );
				MessageBox( nullptr, _T("Could not load properly. Invalid Bookmark file."), _T("Error"), MB_OK | MB_ICONERROR );
				return;
			}
		}

		fclose( pFile );
	}
}

std::string Dlg_MemBookmark::ImportDialog()
{
	std::string str;

	if ( g_pCurrentGameData->GetGameID() == 0 )
	{
		MessageBox( nullptr, _T("ROM not loaded: please load a ROM first!"), _T("Error!"), MB_OK );
		return str;
	}

	IFileOpenDialog* pDlg = nullptr;
	HRESULT hr = CoCreateInstance( CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>( &pDlg ) );
	if ( hr == S_OK )
	{
		hr = pDlg->SetFileTypes( ARRAYSIZE( c_rgFileTypes ), c_rgFileTypes );
		if ( hr == S_OK )
		{
			hr = pDlg->Show( nullptr );
			if ( hr == S_OK )
			{
				IShellItem* pItem = nullptr;
				hr = pDlg->GetResult( &pItem );
				if ( hr == S_OK )
				{
					LPWSTR pStr = nullptr;
					hr = pItem->GetDisplayName( SIGDN_FILESYSPATH, &pStr );
					if ( hr == S_OK )
					{
						str = Narrow( pStr );
					}

					pItem->Release();
				}
			}
		}
		pDlg->Release();
	}

	return str;
}

void Dlg_MemBookmark::OnLoad_NewRom()
{
	HWND hList = GetDlgItem( m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES );
	if ( hList != nullptr )
	{
		ClearAllBookmarks();

		std::string file = RA_DIR_BOOKMARKS + std::to_string( g_pCurrentGameData->GetGameID() ) + "-Bookmarks.txt";
		ImportFromFile( file );
	}
}

void Dlg_MemBookmark::GenerateResizes( HWND hDlg )
{
	RARect windowRect;
	GetWindowRect ( hDlg, &windowRect );
	pDlgMemBookmarkMin.x = windowRect.Width();
	pDlgMemBookmarkMin.y = windowRect.Height();

	vDlgMemBookmarkResize.push_back ( ResizeContent( hDlg,
		GetDlgItem( hDlg, IDC_RA_LBX_ADDRESSES ), ResizeContent::ALIGN_BOTTOM_RIGHT, TRUE ) );

	vDlgMemBookmarkResize.push_back ( ResizeContent( hDlg,
		GetDlgItem( hDlg, IDC_RA_ADD_BOOKMARK ), ResizeContent::ALIGN_RIGHT, FALSE ) );
	vDlgMemBookmarkResize.push_back ( ResizeContent( hDlg,
		GetDlgItem( hDlg, IDC_RA_DEL_BOOKMARK ), ResizeContent::ALIGN_RIGHT, FALSE ) );
	vDlgMemBookmarkResize.push_back ( ResizeContent( hDlg,
		GetDlgItem( hDlg, IDC_RA_CLEAR_CHANGE ), ResizeContent::ALIGN_RIGHT, FALSE ) );

	vDlgMemBookmarkResize.push_back ( ResizeContent( hDlg,
		GetDlgItem( hDlg, IDC_RA_FREEZE ), ResizeContent::ALIGN_BOTTOM_RIGHT, FALSE ) );
	vDlgMemBookmarkResize.push_back ( ResizeContent( hDlg,
		GetDlgItem( hDlg, IDC_RA_DECIMALBOOKMARK ), ResizeContent::ALIGN_BOTTOM_RIGHT, FALSE ) );
	vDlgMemBookmarkResize.push_back ( ResizeContent( hDlg,
		GetDlgItem( hDlg, IDC_RA_SAVEBOOKMARK ), ResizeContent::ALIGN_BOTTOM_RIGHT, FALSE ) );
	vDlgMemBookmarkResize.push_back ( ResizeContent( hDlg,
		GetDlgItem( hDlg, IDC_RA_LOADBOOKMARK ), ResizeContent::ALIGN_BOTTOM_RIGHT, FALSE ) );
}

BOOL Dlg_MemBookmark::EditLabel ( int nItem, int nSubItem )
{
	HWND hList = GetDlgItem( g_MemBookmarkDialog.GetHWND(), IDC_RA_LBX_ADDRESSES );
	RECT rcSubItem;
	ListView_GetSubItemRect( hList, nItem, nSubItem, LVIR_BOUNDS, &rcSubItem );

	RECT rcOffset;
	GetWindowRect ( hList, &rcOffset );
	rcSubItem.left += rcOffset.left;
	rcSubItem.right += rcOffset.left;
	rcSubItem.top += rcOffset.top;
	rcSubItem.bottom += rcOffset.top;

	int nHeight = rcSubItem.bottom - rcSubItem.top;
	int nWidth = rcSubItem.right - rcSubItem.left;

	ASSERT ( g_hIPEEditBM == nullptr );
	if ( g_hIPEEditBM ) return FALSE;

	g_hIPEEditBM = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		_T("EDIT"),
		_T(""),
		WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | ES_WANTRETURN,
		rcSubItem.left, rcSubItem.top, nWidth, (int)( 1.5f*nHeight ),
		g_MemBookmarkDialog.GetHWND(),
		0,
		GetModuleHandle( NULL ),
		NULL );

	if ( g_hIPEEditBM == NULL )
	{
		ASSERT( !"Could not create edit box!" );
		MessageBox( nullptr, _T("Could not create edit box."), _T("Error"), MB_OK | MB_ICONERROR );
		return FALSE;
	};

	SendMessage( g_hIPEEditBM, WM_SETFONT, (WPARAM)GetStockObject( DEFAULT_GUI_FONT ), TRUE );
	SetWindowText( g_hIPEEditBM, NativeStr( m_vBookmarks[ nItem ]->Description() ).c_str() );

	SendMessage( g_hIPEEditBM, EM_SETSEL, 0, -1 );
	SetFocus( g_hIPEEditBM );
	EOldProcBM = (WNDPROC)SetWindowLong( g_hIPEEditBM, GWL_WNDPROC, (LONG)EditProcBM );

	return TRUE;
}
