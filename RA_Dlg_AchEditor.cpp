#include "RA_Dlg_AchEditor.h"

#include "RA_Achievement.h"
#include "RA_AchievementSet.h"
#include "RA_Resource.h"
#include "RA_Defs.h"
#include "RA_Core.h"
#include "RA_Dlg_Achievement.h"
#include "RA_Dlg_Memory.h"
#include "RA_httpthread.h"
#include "RA_ImageFactory.h"
#include "RA_MemManager.h"
#include "RA_User.h"

#pragma comment(lib, "comctl32.lib")

namespace
{
	const char* COLUMN_TITLE[] = { "ID", "Special?", "Type", "Size", "Memory", "Cmp", "Type", "Size", "Mem/Val", "Hits" };
	const int COLUMN_WIDTH[] = { 30, 53, 42, 50, 60, 35, 42, 50, 60, 48 };
	static_assert( SIZEOF_ARRAY( COLUMN_TITLE ) == SIZEOF_ARRAY( COLUMN_WIDTH ), "Must match!" );
}

enum CondSubItems
{
	CSI_ID,
	CSI_GROUP,
	CSI_TYPE_SRC,
	CSI_SIZE_SRC,
	CSI_VALUE_SRC,
	CSI_COMPARISON,
	CSI_TYPE_TGT,
	CSI_SIZE_TGT,
	CSI_VALUE_TGT,
	CSI_HITCOUNT,

	NumColumns
};

BOOL g_bPreferDecimalVal = TRUE;
Dlg_AchievementEditor g_AchievementEditorDialog;

INT_PTR CALLBACK AchProgressProc( HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam )
{
	switch( nMsg )
	{
	case WM_INITDIALOG:
		if( g_AchievementEditorDialog.ActiveAchievement() == nullptr )
			return FALSE;

		return TRUE;

	case WM_COMMAND:
	{
		switch( LOWORD( wParam ) )
		{
		case IDC_RA_ACHPROGRESSENABLE:
			//BOOL bEnabled = IsDlgButtonChecked( hDlg, IDC_RA_ACHPROGRESSENABLE );
			return FALSE;

		case IDOK:
		case IDCLOSE:
			EndDialog( hDlg, true );
			return TRUE;

		default:
			return FALSE;
			}
	}

	case WM_DESTROY:
		EndDialog( hDlg, true );
		return FALSE;
	}

	return FALSE;
}



Dlg_AchievementEditor::Dlg_AchievementEditor()
 :	m_hAchievementEditorDlg( nullptr ),
	m_hICEControl( nullptr ),
	m_pSelectedAchievement( nullptr ),
	m_hAchievementBadge( nullptr ),
	m_bPopulatingAchievementEditorData( false )
{
	for( size_t i = 0; i < MAX_CONDITIONS; ++i )
	{
		if( i == 0 )
			swprintf_s( m_lbxGroupNames[ i ], MEM_STRING_TEXT_LEN, L"Core" );
		else
			swprintf_s( m_lbxGroupNames[ i ], MEM_STRING_TEXT_LEN, L"Alt %02d", i );
	}
}

Dlg_AchievementEditor::~Dlg_AchievementEditor()
{
	if( m_hAchievementBadge != NULL )
	{
		DeleteObject( m_hAchievementBadge );
		m_hAchievementBadge = NULL;
	}
}

void Dlg_AchievementEditor::SetupColumns( HWND hList )
{
	//	Remove all columns,
	while( ListView_DeleteColumn( hList, 0 ) ) {}

	//	Remove all data.
	ListView_DeleteAllItems( hList );

	LV_COLUMN col;
	ZeroMemory( &col, sizeof( col ) );

	for( size_t i = 0; i < m_nNumCols; ++i )
	{
		col.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM|LVCF_FMT;
		col.cx = COLUMN_WIDTH[ i ];
		std::wstring colTitle = Widen( COLUMN_TITLE[ i ] );
		col.pszText = const_cast<LPWSTR>( colTitle.c_str() );
		col.cchTextMax = 255;
		col.iSubItem = i;

		col.fmt = LVCFMT_LEFT|LVCFMT_FIXED_WIDTH;
		if( i == m_nNumCols-1 )
			col.fmt |= LVCFMT_FILL; 

		ListView_InsertColumn( hList, i, (LPARAM)&col );
	}

	ZeroMemory( &m_lbxData, sizeof(m_lbxData) );

	m_nNumOccupiedRows = 0;

	BOOL bSuccess = ListView_SetExtendedListViewStyle( hList, LVS_EX_FULLROWSELECT );
	bSuccess = ListView_EnableGroupView( hList, FALSE );

	//HWND hGroupList = GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_GROUP );
	//ListBox_AddString( 
	//bSuccess = ListView_SetExtendedListViewStyle( hGroupList, LVS_EX_FULLROWSELECT );
	//bSuccess = ListView_EnableGroupView( hGroupList, FALSE );

}

const int Dlg_AchievementEditor::AddCondition( HWND hList, const Condition& Cond )
{
	//	Add to our local array:
	const char* sMemTypStrSrc = "Value";
	const char* sMemSizeStrSrc = "";
	if( Cond.CompSource().Type() != ValueComparison )
	{
		sMemTypStrSrc = ( Cond.CompSource().Type() == Address ) ? "Mem" : "Delta";
		sMemSizeStrSrc = COMPARISONVARIABLESIZE_STR[ Cond.CompSource().Size() ];
	}
	const char* sMemTypStrDst = "Value";
	const char* sMemSizeStrDst = "";
	if( Cond.CompTarget().Type() != ValueComparison )
	{
		sMemTypStrDst = ( Cond.CompTarget().Type() == Address ) ? "Mem" : "Delta";
		sMemSizeStrDst = COMPARISONVARIABLESIZE_STR[ Cond.CompTarget().Size() ];
	}
	
	sprintf_s( m_lbxData[ m_nNumOccupiedRows ][ CSI_ID ],			MEM_STRING_TEXT_LEN, "%d", m_nNumOccupiedRows + 1 );
	sprintf_s( m_lbxData[ m_nNumOccupiedRows ][ CSI_GROUP ],		MEM_STRING_TEXT_LEN, "%s", CONDITIONTYPE_STR[ Cond.GetConditionType() ] );
	sprintf_s( m_lbxData[ m_nNumOccupiedRows ][ CSI_TYPE_SRC ],		MEM_STRING_TEXT_LEN, "%s", sMemTypStrSrc );
	sprintf_s( m_lbxData[ m_nNumOccupiedRows ][ CSI_SIZE_SRC ],		MEM_STRING_TEXT_LEN, "%s", sMemSizeStrSrc );
	sprintf_s( m_lbxData[ m_nNumOccupiedRows ][ CSI_VALUE_SRC ],	MEM_STRING_TEXT_LEN, "0x%06x", Cond.CompSource().RawValue() );
	sprintf_s( m_lbxData[ m_nNumOccupiedRows ][ CSI_COMPARISON ],	MEM_STRING_TEXT_LEN, "%s", COMPARISONTYPE_STR[ Cond.CompareType() ] );
	sprintf_s( m_lbxData[ m_nNumOccupiedRows ][ CSI_TYPE_TGT ],		MEM_STRING_TEXT_LEN, "%s", sMemTypStrDst );
	sprintf_s( m_lbxData[ m_nNumOccupiedRows ][ CSI_SIZE_TGT ],		MEM_STRING_TEXT_LEN, "%s", sMemSizeStrDst );
	sprintf_s( m_lbxData[ m_nNumOccupiedRows ][ CSI_VALUE_TGT ],	MEM_STRING_TEXT_LEN, "0x%02x", Cond.CompTarget().RawValue() );
	sprintf_s( m_lbxData[ m_nNumOccupiedRows ][ CSI_HITCOUNT ],		MEM_STRING_TEXT_LEN, "%d (%d)", Cond.RequiredHits(), Cond.CurrentHits() );

	if( g_bPreferDecimalVal )
	{
		if( Cond.CompTarget().Type() == ValueComparison )
			sprintf_s( m_lbxData[ m_nNumOccupiedRows ][ CSI_VALUE_TGT ], MEM_STRING_TEXT_LEN, "%d", Cond.CompTarget().RawValue() );
	}

	//	Copy our local text into the listbox (:S)

	LV_ITEM item;
	ZeroMemory( &item, sizeof( item ) );
	item.mask = LVIF_TEXT;
	item.cchTextMax = 256;
	item.iItem = m_nNumOccupiedRows;
	item.iSubItem = 0;
	std::wstring sData = Widen( m_lbxData[ m_nNumOccupiedRows ][ CSI_ID ] );
	item.pszText = const_cast<LPWSTR>( sData.c_str() );
	item.iItem = ListView_InsertItem( hList, &item );

	for( size_t i = 1; i < NumColumns; ++i )
	{
		item.iSubItem = i;
		std::wstring sData = Widen( m_lbxData[ m_nNumOccupiedRows ][ i ] );
		item.pszText = const_cast<LPWSTR>( sData.c_str() );
		ListView_SetItem( hList, &item );
	}

	ASSERT( item.iItem == m_nNumOccupiedRows );

	m_nNumOccupiedRows++;
	return item.iItem;
}

// LRESULT CALLBACK ICEDialogProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
// {
// 	BOOL bFullyHandled = FALSE;
// 
// 	switch(uMsg)
// 	{
// 	case WM_INITDIALOG:
// 		{
// 			//MessageBox( nullptr, "ICEDialogProc init!", "TEST", MB_OK );
// 
// // 			{
// // 				//	Disable the underlying dialog!
// // 				HWND hDlg2 = g_AchievementEditorDialog.GetHWND();
// // 				HWND hLbx = GetDlgItem( hDlg2, IDC_RA_LBX_CONDITIONS );
// // 				DWORD nStyle = GetWindowStyle( hLbx );
// // 				nStyle &= ~WS_VISIBLE;
// // 				SetWindowLongPtr( hLbx, GWL_EXSTYLE, nStyle );
// // 				UpdateWindow( hLbx );
// // 			}
// 
// 			//ComboBox_ShowDropdown( hDlg, TRUE );
// 			int i = ComboBox_GetCurSel( g_AchievementEditorDialog.GetICEControl() );
// 
// 			bFullyHandled = TRUE;
// 			break;
// 		}
// 	case WM_SETFOCUS:
// 		{
// 			//BringWindowToTop( hDlg );
// 
// 			//EndDialog( hDlg, FALSE );
// 			//AchievementEditorDialog.SetICEControl( NULL );
// 			//m_hICEControl = NULL;	//	end of life
// 			bFullyHandled = FALSE;
// 			break;
// 		}
// 	case WM_KILLFOCUS:
// 		{
// 			//EndDialog( hDlg, FALSE );
// 			//g_AchievementEditorDialog.SetICEControl( NULL );
// // 
// // 			{
// // 				HWND hDlg = g_AchievementEditorDialog.GetHWND();
// // 				HWND hLbx = GetDlgItem( hDlg, IDC_RA_LBX_CONDITIONS );
// // 				DWORD nStyle = GetWindowStyle( hLbx );
// // 				nStyle |= WS_VISIBLE;
// // 				SetWindowLongPtr( hLbx, GWL_EXSTYLE, nStyle );
// // 			}
// 
// 			bFullyHandled = TRUE;
// 			break;
// 		}
// 	case WM_COMMAND:
// 		switch( LOWORD(wParam) )
// 		{
// 			case IDC_CBODROPDOWN:
// 			case CBN_SELENDOK:
// 			{
// 				HWND hCbo = GetDlgItem( hDlg, IDC_CBODROPDOWN );
// 
// 				char buffer[255];
// 				sprintf_s( buffer, "NewVal: %d", ComboBox_GetCurSel( hCbo ) );
// 				MessageBox( nullptr, buffer, "NewVal!", MB_OK );
// 				bFullyHandled = TRUE;
// 			}
// 			break;
// 		}
// 			//EndDialog( hDlg, FALSE );
// 			//g_AchievementEditorDialog.SetICEControl( NULL );
// 			// 
// 			// 			{
// 			// 				HWND hDlg = g_AchievementEditorDialog.GetHWND();
// 			// 				HWND hLbx = GetDlgItem( hDlg, IDC_RA_LBX_CONDITIONS );
// 			// 				DWORD nStyle = GetWindowStyle( hLbx );
// 			// 				nStyle |= WS_VISIBLE;
// 			// 				SetWindowLongPtr( hLbx, GWL_EXSTYLE, nStyle );
// // 			// 			}
// // 
// // 		bFullyHandled = TRUE;
// // 		break;
// 	}
// 	return !bFullyHandled;
// }
// 

WNDPROC EOldProc;
HWND g_hIPEEdit;
int nSelItem;
int nSelSubItem;

long _stdcall EditProc( HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam )
{
	switch( nMsg )
	{
		case WM_DESTROY:
			g_hIPEEdit = nullptr;
			break;

		case WM_KILLFOCUS:
		{
			LV_DISPINFO lvDispinfo;
			ZeroMemory( &lvDispinfo, sizeof( LV_DISPINFO ) );
			lvDispinfo.hdr.hwndFrom = hwnd;
			lvDispinfo.hdr.idFrom = GetDlgCtrlID( hwnd );
			lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
			lvDispinfo.item.mask = LVIF_TEXT;
			lvDispinfo.item.iItem = nSelItem;
			lvDispinfo.item.iSubItem = nSelSubItem;
			lvDispinfo.item.pszText = nullptr;

			wchar_t sEditText[ 12 ];
			GetWindowText( hwnd, sEditText, 12 );
			lvDispinfo.item.pszText = sEditText;
			lvDispinfo.item.cchTextMax = lstrlen( sEditText );

			HWND hList = GetDlgItem( g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS );

			//	the LV ID and the LVs Parent window's HWND
			SendMessage( GetParent( hList ), WM_NOTIFY, static_cast<WPARAM>( IDC_RA_LBX_CONDITIONS ), reinterpret_cast<LPARAM>( &lvDispinfo ) );	//	##reinterpret? ##SD

			//	Extra validation?
			
			if( lstrcmp( sEditText, L"Value" ) )
			{
				//	Remove the associated 'size' entry
				strcpy_s( g_AchievementEditorDialog.LbxDataAt( nSelItem, nSelSubItem + 1 ), 32, "" );
			}

			DestroyWindow( hwnd );
			break;
		}

 		case WM_KEYDOWN:
 		{
 			if( wParam == VK_RETURN || wParam == VK_ESCAPE )
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

	return CallWindowProc( EOldProc, hwnd, nMsg, wParam, lParam );
}

long _stdcall DropDownProc( HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam )
{
	if( nMsg == WM_COMMAND )
	{
		if( HIWORD( wParam ) == CBN_SELCHANGE )
		{
			LV_DISPINFO lvDispinfo;
			ZeroMemory( &lvDispinfo, sizeof( LV_DISPINFO ) );
			lvDispinfo.hdr.hwndFrom = hwnd;
			lvDispinfo.hdr.idFrom = GetDlgCtrlID( hwnd );
			lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
			lvDispinfo.item.mask = LVIF_TEXT;
			lvDispinfo.item.iItem = nSelItem;
			lvDispinfo.item.iSubItem = nSelSubItem;
			lvDispinfo.item.pszText = NULL;

			wchar_t sEditText[ 12 ];
			GetWindowText( hwnd, sEditText, 12 );
			lvDispinfo.item.pszText = sEditText;
			lvDispinfo.item.cchTextMax = lstrlen( sEditText );

			//	the LV ID and the LVs Parent window's HWND
			HWND hList = GetDlgItem( g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS );
			SendMessage( GetParent( hList ), WM_NOTIFY, static_cast<WPARAM>( IDC_RA_LBX_CONDITIONS ), reinterpret_cast<LPARAM>( &lvDispinfo ) );

			DestroyWindow( hwnd );
		}
	}

	switch( nMsg )
	{
	case WM_DESTROY:
		g_hIPEEdit = nullptr;
		break;

	case WM_KILLFOCUS:
		{
			LV_DISPINFO lvDispinfo;
			ZeroMemory( &lvDispinfo, sizeof( LV_DISPINFO ) );
			lvDispinfo.hdr.hwndFrom = hwnd;
			lvDispinfo.hdr.idFrom = GetDlgCtrlID( hwnd );
			lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
			lvDispinfo.item.mask = LVIF_TEXT;
			lvDispinfo.item.iItem = nSelItem;
			lvDispinfo.item.iSubItem = nSelSubItem;
			lvDispinfo.item.pszText = nullptr;

			wchar_t sEditText[ 12 ];
			GetWindowText( hwnd, sEditText, 12 );
			lvDispinfo.item.pszText = sEditText;
			lvDispinfo.item.cchTextMax = lstrlen( sEditText );

			HWND hList = GetDlgItem( g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS );

			//	the LV ID and the LVs Parent window's HWND
			SendMessage( GetParent( hList ), WM_NOTIFY, (WPARAM)IDC_RA_LBX_CONDITIONS, (LPARAM)&lvDispinfo );

			//DestroyWindow(hwnd);
			DestroyWindow( hwnd );
		}
		break;

	case WM_KEYDOWN:
		if( wParam == VK_RETURN )
		{
// 				LV_DISPINFO lvDispinfo;
// 				ZeroMemory(&lvDispinfo,sizeof(LV_DISPINFO));
// 				lvDispinfo.hdr.hwndFrom = hwnd;
// 				lvDispinfo.hdr.idFrom = GetDlgCtrlID(hwnd);
// 				lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
// 				lvDispinfo.item.mask = LVIF_TEXT;
// 				lvDispinfo.item.iItem = nSelItem;
// 				lvDispinfo.item.iSubItem = nSelSubItem;
// 				lvDispinfo.item.pszText = NULL;
// 
// 				char szEditText[12];
// 				GetWindowText( hwnd, szEditText, 12 );
// 				lvDispinfo.item.pszText = szEditText;
// 				lvDispinfo.item.cchTextMax = lstrlen(szEditText);
// 
// 				HWND hList = GetDlgItem( g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS );
// 
// 				//	the LV ID and the LVs Parent window's HWND
// 				SendMessage( GetParent( hList ), WM_NOTIFY, (WPARAM)IDC_RA_LBX_CONDITIONS, (LPARAM)&lvDispinfo );

			DestroyWindow( hwnd );
		}
		else if( wParam == VK_ESCAPE )
		{
			//	Undo changes: i.e. simply destroy the window!
			DestroyWindow( hwnd );
		}
		else
		{
			//	Ignore keystroke, or pass it into the edit box!
		}
			
		break;

	case WM_COMMAND:
		switch( HIWORD( wParam ) )
		{
		case CBN_SELENDOK:
		case CBN_SELENDCANCEL:
		case CBN_SELCHANGE:
		case CBN_CLOSEUP:
		case CBN_KILLFOCUS:
			break;
		}
	}

	return CallWindowProc( EOldProc, hwnd, nMsg, wParam, lParam );
}

BOOL CreateIPE( int nItem, int nSubItem )
{
	BOOL bSuccess = FALSE;

	HWND hList = GetDlgItem( g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS );
	
	RECT rcSubItem;
	ListView_GetSubItemRect( hList, nItem, nSubItem, LVIR_BOUNDS, &rcSubItem );

	RECT rcOffset;
	GetWindowRect( hList, &rcOffset );

	rcSubItem.left += rcOffset.left;
	rcSubItem.right += rcOffset.left;
	rcSubItem.top += rcOffset.top;
	rcSubItem.bottom += rcOffset.top;

	int nHeight = rcSubItem.bottom - rcSubItem.top;
	int nWidth = rcSubItem.right - rcSubItem.left;
	if( nSubItem == 0 )
		nWidth = nWidth / 4; /*NOTE: the ListView has 4 columns;
							when iSubItem == 0 (an item is clicked),
							the width (largura) is divided by 4,
							because for items (not subitems) the
							width returned is that of the whole row.*/


	switch( nSubItem )
	{
	case CSI_ID:
		ASSERT( !"First element does nothing!" );	//	nothing we do if we click the first element!
		break;

	case CSI_GROUP:
		{
			//	Group: BOOL (dropdown?)
			if( g_AchievementEditorDialog.ActiveAchievement() != nullptr )
			{
				const size_t nGrp = g_AchievementEditorDialog.GetSelectedConditionGroup();
				Condition& rCond = g_AchievementEditorDialog.ActiveAchievement()->GetCondition( nGrp, nItem );
				//wchar_t* sNewText = rCond.IsResetCondition() ? L"PauseIf:" : rCond.IsPauseCondition() ? L"" : L"ResetIf:";
				const char* sNewText = CONDITIONTYPE_STR[ ( rCond.GetConditionType() + 1 ) % Condition::NumConditionTypes ];

				HWND hList = GetDlgItem( g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS );

				LVITEM lvItem;
				ZeroMemory( &lvItem, sizeof( lvItem ) );
				lvItem.iItem = nItem;
				lvItem.iSubItem = nSubItem;
				std::wstring sStrWide = Widen( sNewText );	//	Scoped cache!
				lvItem.pszText = const_cast<LPWSTR>( sStrWide.c_str() );
				lvItem.cchTextMax = 256;

				//	Inject the new text into the lbx
				SendDlgItemMessage( g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS, LVM_SETITEMTEXT, (WPARAM)lvItem.iItem, (LPARAM)&lvItem ); // put new text

				//	Update the cached data:
				char* sData = g_AchievementEditorDialog.LbxDataAt( nItem, nSubItem );
				strcpy_s( sData, MEM_STRING_TEXT_LEN, sNewText );
				
				//	Update the achievement data
				rCond.SetConditionType( static_cast<Condition::ConditionType>( ( rCond.GetConditionType() + 1 ) % Condition::NumConditionTypes ) );

        // achievement is now dirty
        Achievement* pActiveAch = g_AchievementEditorDialog.ActiveAchievement();
        pActiveAch->SetModified(TRUE);
        g_AchievementsDialog.OnEditAchievement(*pActiveAch);
			}
		}
		break;

	case CSI_TYPE_SRC:
	case CSI_TYPE_TGT:
		{
			//	Type: dropdown
			ASSERT( g_hIPEEdit == nullptr );
			if( g_hIPEEdit )
				break;

			const int nNumItems = 3;	//	"Mem", "Delta" or "Value"

			g_hIPEEdit = CreateWindowEx(
				WS_EX_CLIENTEDGE,
				L"ComboBox",
				L"",
				WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | CBS_DROPDOWNLIST,
				rcSubItem.left, rcSubItem.top, nWidth, static_cast<int>( 1.6f * nHeight * nNumItems ),
				g_AchievementEditorDialog.GetHWND(),
				0,
				GetModuleHandle( NULL ),
				NULL );

			if( g_hIPEEdit == NULL )
			{
				ASSERT( !"Could not create edit box!" );
				MessageBox( nullptr, L"Could not create edit box.", L"Error", MB_OK | MB_ICONERROR );
				break;
			};

// 			for( size_t i = 0; i < g_NumMemSizeStrings; ++i )
// 			{
// 				ComboBox_AddString( g_hIPEEdit, g_MemSizeStrings[i] );
// 
// 				if( strcmp( g_AchievementEditorDialog.LbxDataAt( nItem, nSubItem ), g_MemSizeStrings[i] ) == 0 )
// 					ComboBox_SetCurSel( g_hIPEEdit, i );
// 			}

			/*CB_ERRSPACE*/
			ComboBox_AddString( g_hIPEEdit, Widen("Mem").c_str() );
			ComboBox_AddString( g_hIPEEdit, Widen("Delta").c_str() );
			ComboBox_AddString( g_hIPEEdit, Widen("Value").c_str() );
			
			int nSel;
			if( strcmp( g_AchievementEditorDialog.LbxDataAt( nItem, nSubItem ), "Mem" ) == 0 )
				nSel = 0;
			else if( strcmp( g_AchievementEditorDialog.LbxDataAt( nItem, nSubItem ), "Delta" ) == 0 )
				nSel = 1;
			else
				nSel = 2;

			ComboBox_SetCurSel( g_hIPEEdit, nSel );

			SendMessage( g_hIPEEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),  TRUE);
			ComboBox_ShowDropdown( g_hIPEEdit, TRUE );

			EOldProc = (WNDPROC)SetWindowLong( g_hIPEEdit, GWL_WNDPROC, (LONG)DropDownProc );
		}
		break;
	case CSI_SIZE_SRC:
	case CSI_SIZE_TGT:
		{
			//	Size: dropdown
			ASSERT( g_hIPEEdit == NULL );
			if( g_hIPEEdit )
				break;

			//	Note: this relies on column order :S
			if( strcmp( g_AchievementEditorDialog.LbxDataAt( nItem, nSubItem-1 ), "Value" ) == 0 )
			{
				//	Values have no size.
				break;
			}

			g_hIPEEdit = CreateWindowEx( 
				WS_EX_CLIENTEDGE, 
				L"ComboBox", 
				L"", 
				WS_CHILD|WS_VISIBLE|WS_POPUPWINDOW|WS_BORDER|CBS_DROPDOWNLIST,
				rcSubItem.left, rcSubItem.top, nWidth, (int)( 1.6f * nHeight * NumComparisonVariableSizeTypes ),
				g_AchievementEditorDialog.GetHWND(), 
				0, 
				GetModuleHandle(NULL), 
				NULL );

			if( g_hIPEEdit == NULL )
			{
				ASSERT( !"Could not create combo box!" );
				MessageBox( nullptr, L"Could not create combo box.", L"Error", MB_OK | MB_ICONERROR );
				break;
			};

			for( size_t i = 0; i < NumComparisonVariableSizeTypes; ++i )
			{
				ComboBox_AddString( g_hIPEEdit, Widen( COMPARISONVARIABLESIZE_STR[ i ] ).c_str() );

				if( strcmp( g_AchievementEditorDialog.LbxDataAt( nItem, nSubItem ), COMPARISONVARIABLESIZE_STR[i] ) == 0 )
					ComboBox_SetCurSel( g_hIPEEdit, i );
			}

			SendMessage( g_hIPEEdit, WM_SETFONT, (WPARAM)GetStockObject( DEFAULT_GUI_FONT ),  TRUE);
			ComboBox_ShowDropdown( g_hIPEEdit, TRUE );

			EOldProc = (WNDPROC)SetWindowLong( g_hIPEEdit, GWL_WNDPROC, (LONG)DropDownProc );
		}
		break;
	case CSI_COMPARISON:
		{
			//	Compare: dropdown
			ASSERT( g_hIPEEdit == NULL );
			if( g_hIPEEdit )
				break;

			g_hIPEEdit = CreateWindowEx( 
				WS_EX_CLIENTEDGE, 
				L"ComboBox", 
				L"", 
				WS_CHILD|WS_VISIBLE|WS_POPUPWINDOW|WS_BORDER|CBS_DROPDOWNLIST,
				rcSubItem.left, rcSubItem.top, nWidth, (int)( 1.6f * nHeight * NumComparisonTypes ),
				g_AchievementEditorDialog.GetHWND(), 
				0, 
				GetModuleHandle( NULL ),
				NULL );

			if( g_hIPEEdit == NULL )
			{
				ASSERT( !"Could not create combo box..." );
				MessageBox( nullptr, L"Could not create combo box.", L"Error", MB_OK | MB_ICONERROR );
				break;
			};

			for( size_t i = 0; i < NumComparisonTypes; ++i )
			{
				ComboBox_AddString( g_hIPEEdit, Widen(COMPARISONTYPE_STR[ i ] ).c_str() );

				if( strcmp( g_AchievementEditorDialog.LbxDataAt( nItem, nSubItem ), COMPARISONTYPE_STR[ i ] ) == 0 )
					ComboBox_SetCurSel( g_hIPEEdit, i );
			}

			SendMessage( g_hIPEEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),  TRUE);
			ComboBox_ShowDropdown( g_hIPEEdit, TRUE );

			EOldProc = (WNDPROC)SetWindowLong( g_hIPEEdit, GWL_WNDPROC, (LONG)DropDownProc );
		}
		break;
	case CSI_HITCOUNT:
	case CSI_VALUE_SRC:	//	Mem/Val: edit
	case CSI_VALUE_TGT: //	Mem/Val: edit
		{
			ASSERT( g_hIPEEdit == NULL );
			if( g_hIPEEdit )
				break;

			g_hIPEEdit = CreateWindowEx( 
				WS_EX_CLIENTEDGE, 
				L"EDIT", 
				L"", 
				WS_CHILD|WS_VISIBLE|WS_POPUPWINDOW|WS_BORDER|ES_WANTRETURN,
				rcSubItem.left, rcSubItem.top, nWidth, (int)(1.5f*nHeight), 
				g_AchievementEditorDialog.GetHWND(), 
				0, 
				GetModuleHandle(NULL), 
				NULL );

			if( g_hIPEEdit == NULL )
			{
				ASSERT( !"Could not create edit box!" );
				MessageBox( nullptr, L"Could not create edit box.", L"Error", MB_OK | MB_ICONERROR );
				break;
			};

			SendMessage( g_hIPEEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),  TRUE);

			char* pData = g_AchievementEditorDialog.LbxDataAt( nItem, nSubItem );
			SetWindowText( g_hIPEEdit, Widen( pData ).c_str() );
			
			//	Special case, hitcounts
			if( nSubItem == CSI_HITCOUNT )
			{
				if( g_AchievementEditorDialog.ActiveAchievement() != NULL )
				{
					const size_t nGrp = g_AchievementEditorDialog.GetSelectedConditionGroup();
					const Condition& Cond = g_AchievementEditorDialog.ActiveAchievement()->GetCondition( nGrp, nItem );

					char buffer[ 256 ];
					sprintf_s( buffer, 256, "%d", Cond.RequiredHits() );
					SetWindowText( g_hIPEEdit, Widen( buffer ).c_str() );
				}
			}

			SendMessage( g_hIPEEdit, EM_SETSEL, 0, -1 );
			SetFocus( g_hIPEEdit );
			EOldProc = (WNDPROC)SetWindowLong( g_hIPEEdit, GWL_WNDPROC, (LONG)EditProc );

			bSuccess = TRUE;
		}

		break;
	}

	return bSuccess;
}


//static
INT_PTR CALLBACK Dlg_AchievementEditor::s_AchievementEditorProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	//	Intercept some msgs?
	return g_AchievementEditorDialog.AchievementEditorProc( hDlg, uMsg, wParam, lParam );
}

INT_PTR Dlg_AchievementEditor::AchievementEditorProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	BOOL bHandled = TRUE;

	switch(uMsg)
	{
	case WM_TIMER:
		{
			//	Ignore if we are currently editing a box!
			if( g_hIPEEdit != NULL )
				break;

			Achievement* pActiveAch = ActiveAchievement();
			if( pActiveAch == NULL )
				break;

			if( pActiveAch->IsDirty() )
			{
				LoadAchievement( pActiveAch, TRUE );
				pActiveAch->ClearDirtyFlag();
			}
		}
		break;

	case WM_INITDIALOG:
		{
			RECT rc;
			GetWindowRect( g_RAMainWnd, &rc );
			SetWindowPos( hDlg, NULL, rc.right, rc.bottom, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW );

			m_hAchievementEditorDlg = hDlg;

			HWND hList = GetDlgItem( m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS );
			SetupColumns( hList );

			CheckDlgButton( hDlg, IDC_RA_CHK_SHOW_DECIMALS, g_bPreferDecimalVal );

			//	For scanning changes to achievement conditions (hit counts)
			SetTimer( m_hAchievementEditorDlg, 1, 200, (TIMERPROC)s_AchievementEditorProc );

			m_BadgeNames.InstallAchEditorCombo( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_BADGENAME ) );
			m_BadgeNames.FetchNewBadgeNamesThreaded();
		}
		return TRUE;

	case WM_COMMAND:
		switch( LOWORD( wParam ) )
		{
		case IDCLOSE:
			EndDialog( hDlg, true );
			bHandled = TRUE;
			break;

		case IDC_RA_CHK_SHOW_DECIMALS:
			{
				g_bPreferDecimalVal = !g_bPreferDecimalVal;
				if( ActiveAchievement() != NULL )
				{
					ActiveAchievement()->SetDirtyFlag( Dirty__All );
					LoadAchievement( ActiveAchievement(), TRUE );
					ActiveAchievement()->ClearDirtyFlag();
				}
				bHandled = TRUE;
			}
			break;
// 		case ID_SELECT_ALL:
// 			{
// 				HWND hList = GetDlgItem( hDlg, IDC_RA_LBX_CONDITIONS );
// 				for( size_t i = 0; i < m_nNumOccupiedRows; ++i )
// 					ListView_SetCheckState( hList, i, TRUE );
// 				bHandled = TRUE;
// 			}
// 			break;

		case IDC_RA_BADGENAME:
			{
// 				if( m_pSelectedAchievement == NULL )
// 				{
// 					bHandled = TRUE;
// 					break;
// 				}

				HWND hBadgeNameCtrl = GetDlgItem( hDlg, IDC_RA_BADGENAME );
				switch( HIWORD( wParam ) )
				{
					case CBN_SELENDOK:
					case CBN_SELCHANGE:
						{
							wchar_t buffer[ 16 ];
							GetWindowText( hBadgeNameCtrl, buffer, 16 );
							UpdateBadge( Narrow( buffer ) );
						}
						break;
					case CBN_SELENDCANCEL:
						//	Restore?
						break;
				}

				bHandled = TRUE;
			}
			break;

		case IDC_RA_PROGRESSINDICATORS:
			{
				DialogBox( (HINSTANCE)(GetModuleHandle(NULL)), MAKEINTRESOURCE(IDD_RA_ACHIEVEMENTPROGRESS), hDlg, AchProgressProc );
				//DialogBox( ghInstance, MAKEINTRESOURCE(IDD_RA_ACHIEVEMENTPROGRESS), hDlg, AchProgressProc );
				bHandled = TRUE;
			}
			break;

		case IDC_RA_ACH_TITLE:
			{
				switch( HIWORD(wParam) )
				{
				case EN_CHANGE:
					{
						if( IsPopulatingAchievementEditorData() )
							return TRUE;

						Achievement* pActiveAch = ActiveAchievement();
						if( pActiveAch == NULL )
							return FALSE;

						//	Disable all achievement tracking
						//g_pActiveAchievements->SetPaused( true );
						//CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );

						//	Set this achievement as modified
						pActiveAch->SetModified( TRUE );
						g_AchievementsDialog.OnEditAchievement( *pActiveAch );

						HWND hList = GetDlgItem( g_AchievementsDialog.GetHWND(), IDC_RA_LISTACHIEVEMENTS );
						int nSelectedIndex = ListView_GetNextItem( hList, -1, LVNI_SELECTED );
						if( nSelectedIndex != -1 )
						{
							//	Implicit updating:
							wchar_t buffer[ 1024 ];
							if( GetDlgItemText( hDlg, IDC_RA_ACH_TITLE, buffer, 1024 ) )
							{
								pActiveAch->SetTitle( Narrow( buffer ) );

								//	Persist/Update/Inject local LBX data back into LBX (?)
								g_AchievementsDialog.OnEditData( g_pActiveAchievements->GetAchievementIndex( *pActiveAch ), Dlg_Achievements::Title, pActiveAch->Title() );
							}
						}
					}
					break;
				}
			}
			break;
		case IDC_RA_ACH_DESC:
			switch( HIWORD(wParam) )
			{
			case EN_CHANGE:
				{
					if( IsPopulatingAchievementEditorData() )
						return TRUE;

					Achievement* pActiveAch = ActiveAchievement();
					if( pActiveAch == NULL )
						return FALSE;

					//	Disable all achievement tracking:
					//g_pActiveAchievements->SetPaused( true );
					//CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );
					
					//	Set this achievement as modified:
					pActiveAch->SetModified( TRUE );
					g_AchievementsDialog.OnEditAchievement( *pActiveAch );

					HWND hList = GetDlgItem( g_AchievementsDialog.GetHWND(), IDC_RA_LISTACHIEVEMENTS );
					int nSelectedIndex = ListView_GetNextItem( hList, -1, LVNI_SELECTED );
					if( nSelectedIndex == -1 )
						return FALSE;

					//	Implicit updating:
					//char* psDesc = g_AchievementsDialog.LbxDataAt( nSelectedIndex, (int)Dlg_Achievements:: );
					wchar_t buffer[ 80 ];
					if( GetDlgItemText( hDlg, IDC_RA_ACH_DESC, buffer, 80 ) )
						pActiveAch->SetDescription( Narrow( buffer ) );
				}
				break;
			}
			break;

		case IDC_RA_ACH_POINTS:
			switch( HIWORD(wParam) )
			{
			case EN_CHANGE:
				{
					if( IsPopulatingAchievementEditorData() )
						return TRUE;

					Achievement* pActiveAch = ActiveAchievement();
					if( pActiveAch == nullptr )
						return FALSE;

					wchar_t buffer[ 16 ];
					if( GetDlgItemText( hDlg, IDC_RA_ACH_POINTS, buffer, 16 ) )
					{
						int nVal = strtol( Narrow( buffer ).c_str(), nullptr, 10 );
						if( nVal < 0 || nVal > 100 )
							return FALSE;

						pActiveAch->SetPoints( nVal );
						return TRUE;
					}
				}
				break;
			}
			break;

		case IDC_RA_ADDCOND:
			{
				Achievement* pActiveAch = ActiveAchievement();
				if( pActiveAch == nullptr )
					return FALSE;

				Condition NewCondition;
				NewCondition.SetIsBasicCondition();
				NewCondition.CompSource().Set( EightBit, Address, 0x0000, 0 );
				NewCondition.CompTarget().Set( EightBit, ValueComparison, 0, 0 );	//	Compare defaults!

				//	Helper: guess that the currently watched memory location
				//	 is probably what they are about to want to add a cond for.
				if( g_MemoryDialog.GetHWND() != nullptr )
				{
					wchar_t buffer[ 256 ];
					GetDlgItemText( g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, buffer, 256 );
					unsigned int nVal = strtol( Narrow( buffer ).c_str(), nullptr, 16 );
					NewCondition.CompSource().SetValues( nVal, nVal );
				}

				const size_t nNewID = pActiveAch->AddCondition( GetSelectedConditionGroup(), NewCondition )-1;

				//	Disable all achievement tracking:
				//g_pActiveAchievements->SetPaused( true );
				//CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );

				//	Set this achievement as 'modified'
				pActiveAch->SetModified( TRUE );
				g_AchievementsDialog.OnEditAchievement( *pActiveAch );

				LoadAchievement( pActiveAch, FALSE );
				pActiveAch->ClearDirtyFlag();

				//	Select last item
				HWND hList = GetDlgItem( hDlg, IDC_RA_LBX_CONDITIONS );
				ListView_SetItemState( hList, nNewID, LVIS_FOCUSED | LVIS_SELECTED, -1 );
				ListView_EnsureVisible( hList, nNewID, FALSE );
			}
			break;
		case IDC_RA_CLONECOND:
			{
				HWND hList = GetDlgItem( hDlg, IDC_RA_LBX_CONDITIONS );
				int nSel = ListView_GetNextItem( hList, -1, LVNI_SELECTED );
				Achievement* pActiveAch = ActiveAchievement();
				if( pActiveAch != NULL )
				{
					if( nSel == -1 || nSel > static_cast<int>( pActiveAch->NumConditions( GetSelectedConditionGroup() ) ) )
						return FALSE;

					Condition& CondToClone = pActiveAch->GetCondition( GetSelectedConditionGroup(), static_cast<size_t>( nSel ) );

					Condition NewCondition;
					NewCondition.Set( CondToClone );

					const size_t nNewID = pActiveAch->AddCondition( GetSelectedConditionGroup(), NewCondition ) - 1;

					//	Disable all achievement tracking:
					//g_pActiveAchievements->SetPaused( true );
					//CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );

					//	Update this achievement entry as 'modified':
					pActiveAch->SetModified( TRUE );
					g_AchievementsDialog.OnEditAchievement( *pActiveAch );

					LoadAchievement( pActiveAch, FALSE );
					pActiveAch->ClearDirtyFlag();

					//	Select last item
					ListView_SetItemState( hList, nNewID, LVIS_FOCUSED|LVIS_SELECTED, -1 );
					ListView_EnsureVisible( hList, nNewID, FALSE );
				}
			}
			break;
		case IDC_RA_DELETECOND:
			{
				HWND hList = GetDlgItem( hDlg, IDC_RA_LBX_CONDITIONS );
				int nSel = ListView_GetNextItem( hList, -1, LVNI_SELECTED );

				if( nSel != -1 )
				{
					Achievement* pActiveAch = ActiveAchievement();
					if( pActiveAch != NULL )
					{
						char buffer[ 256 ];
						sprintf_s( buffer, 256, "Are you sure you wish to delete requirement ID %d?", nSel + 1 );
						if( MessageBox( hDlg, Widen( buffer ).c_str(), L"Warning", MB_YESNO ) == IDYES )
						{
							//	Delete condition nSel
							pActiveAch->RemoveCondition( GetSelectedConditionGroup(), nSel );

							//	Disable all achievement tracking:
							//g_pActiveAchievements->SetPaused( true );
							//CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );
							
							//	Set this achievement as 'modified'
							pActiveAch->SetModified( TRUE );
							g_AchievementsDialog.OnEditAchievement( *pActiveAch );

							//	Refresh:
							LoadAchievement( pActiveAch, TRUE );
						}
					}
				}

			}
			break;

		case IDC_RA_UPLOAD_BADGE:
			{
				SetCurrentDirectory( L"\\" );
				
				const int BUF_SIZE = 1024;
				wchar_t buffer[ BUF_SIZE ];
				ZeroMemory( buffer, BUF_SIZE );

				OPENFILENAME ofn;
				ZeroMemory( &ofn, sizeof( OPENFILENAME ) );
				ofn.lStructSize = sizeof( OPENFILENAME );
				ofn.hwndOwner = hDlg;
				ofn.hInstance = static_cast<HINSTANCE>( GetModuleHandle( nullptr ) );
				ofn.lpstrFile = buffer;
				ofn.nMaxFile = BUF_SIZE-1;

				ofn.lpstrFilter = L"PNG Files\0" L"*.png\0"
								  L"GIF Files\0" L"*.gif\0"
								  L"JPG Files\0" L"*.jpg\0"
								  L"JPEG Files\0" L"*.jpeg\0"
								  L"All Files\0" L"*.*\0";

				ofn.nFilterIndex = 1;
				ofn.lpstrDefExt = L"png";
				ofn.Flags = OFN_FILEMUSTEXIST;

				if( GetOpenFileName( &ofn ) == 0 )	//	0 == cancelled
					return FALSE;

				if( ofn.lpstrFile != NULL )
				{
					Document Response;
					if( RAWeb::DoBlockingImageUpload( RequestUploadBadgeImage, Narrow( ofn.lpstrFile ), Response ) )
					{
						//TBD: ensure that:
						//	The image is copied to the cache/badge dir
						//	The image doesn't already exist in teh cache/badge dir (ask overwrite)
						//	The image is the correct dimensions or can be scaled
						//	The image can be uploaded OK
						//	The image is not copyright

						const Value& ResponseData = Response[ "Response" ];
						const char* sNewBadgeIter = ResponseData[ "BadgeIter" ].GetString();

						//pBuffer will contain "OK:" and the number of the uploaded file.
						//	Add the value to the available values in the cbo, and it *should* self-populate.
						MessageBox( nullptr, L"Successful!", L"Upload OK", MB_OK );

						m_BadgeNames.AddNewBadgeName( sNewBadgeIter, true );
						UpdateBadge( sNewBadgeIter );
					}
					else
					{
						MessageBox( nullptr, L"Could not contact server!", L"Error", MB_OK );
					}
				}
			}
		break;

		case IDC_RA_ACH_ADDGROUP:
		{
			if( ActiveAchievement() == NULL )
				break;

			if( ActiveAchievement()->NumConditionGroups() == 1 )
			{
				ActiveAchievement()->AddConditionGroup();
				ActiveAchievement()->AddConditionGroup();
			}
			else if( ActiveAchievement()->NumConditionGroups() > 1 )
			{
				ActiveAchievement()->AddConditionGroup();
			}
			else
			{
				//?
			}

			RepopulateGroupList( ActiveAchievement() );
		}
		break;
		case IDC_RA_ACH_DELGROUP:
		{
			if( ActiveAchievement() == NULL )
				break;

			if( ActiveAchievement()->NumConditionGroups() == 3 )
			{
				ActiveAchievement()->RemoveConditionGroup();
				ActiveAchievement()->RemoveConditionGroup();
			}
			else if( ActiveAchievement()->NumConditionGroups() > 3 )
			{
				ActiveAchievement()->RemoveConditionGroup();
			}
			else
			{
				MessageBox( m_hAchievementEditorDlg, L"Cannot remove Core Condition Group!", L"Warning", MB_OK );
			}

			RepopulateGroupList( ActiveAchievement() );
		}
		break;
		case IDC_RA_ACH_GROUP:	
			switch(HIWORD(wParam))
			{
			case LBN_SELCHANGE:
				PopulateConditions( ActiveAchievement() );
				break;
			}
		break;
		}

		//	Switch also on the highword:

		switch( HIWORD( wParam ) )
		{
		case CBN_SELENDOK:
		case CBN_SELENDCANCEL:
		case CBN_SELCHANGE:
		case CBN_CLOSEUP:
		case CBN_KILLFOCUS:
			{
				//	Only post this if the edit control is not empty:
				//	 if it's empty, we're not dealing with THIS edit control!
				if( g_hIPEEdit != NULL )
				{
					LV_DISPINFO lvDispinfo;
					ZeroMemory(&lvDispinfo,sizeof(LV_DISPINFO));
					lvDispinfo.hdr.hwndFrom = g_hIPEEdit;
					lvDispinfo.hdr.idFrom = GetDlgCtrlID(g_hIPEEdit);
					lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
					lvDispinfo.item.mask = LVIF_TEXT;
					lvDispinfo.item.iItem = nSelItem;
					lvDispinfo.item.iSubItem = nSelSubItem;
					lvDispinfo.item.pszText = NULL;

					wchar_t sEditText[ 12 ];
					GetWindowText( g_hIPEEdit, sEditText, 12 );
					lvDispinfo.item.pszText = sEditText;
					lvDispinfo.item.cchTextMax = lstrlen( sEditText );

					HWND hList = GetDlgItem( GetHWND(), IDC_RA_LBX_CONDITIONS );

					//	the LV ID and the LVs Parent window's HWND
					SendMessage( GetParent( hList ), WM_NOTIFY, (WPARAM)IDC_RA_LBX_CONDITIONS, (LPARAM)&lvDispinfo );

					DestroyWindow( g_hIPEEdit );
				}
			}
			break;
		}
		break;

	case WM_NOTIFY:
		{
			switch( (((LPNMHDR)lParam)->code) )
			{
			case NM_CLICK:
				{
					LPNMITEMACTIVATE pOnClick = (LPNMITEMACTIVATE)lParam;
					
					//	http://cboard.cprogramming.com/windows-programming/122733-%5Bc%5D-editing-subitems-listview-win32-api.html

					HWND hList = GetDlgItem( m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS );
					ASSERT( hList != NULL );

					//	Note: first item should be an ID!
					if( pOnClick->iItem != -1 && pOnClick->iSubItem != 0 )
					{
						nSelItem = pOnClick->iItem;
						nSelSubItem = pOnClick->iSubItem;

						CreateIPE( pOnClick->iItem, pOnClick->iSubItem );
					} 
					else 
					{
						nSelItem = -1;
						nSelSubItem = -1;
						/*If SubItemHitTest does return error (lResult=-1),
						it kills focus of hEdit in order to destroy it.*/
						SendMessage( g_hIPEEdit, WM_KILLFOCUS, 0, 0 );
					}
					return 0;

				}
				break;
			case NM_RCLICK:
				{
					LPNMITEMACTIVATE pOnClick = (LPNMITEMACTIVATE)lParam;
					if( pOnClick->iItem != -1 &&
						pOnClick->iSubItem != -1 )
					{
						if( static_cast<size_t>( pOnClick->iItem ) > ActiveAchievement()->NumConditions( GetSelectedConditionGroup() ) )
							return 0;

						Condition& rCond = ActiveAchievement()->GetCondition( GetSelectedConditionGroup(), pOnClick->iItem );

						//HWND hMem = GetDlgItem( HWndMemoryDlg, IDC_RA_WATCHING );
						if( pOnClick->iSubItem == CSI_VALUE_SRC )
						{
							if( rCond.CompSource().Type() != ValueComparison )
							{
								//	Wake up the mem dlg via the main app
								SendMessage( g_RAMainWnd, WM_COMMAND, IDM_RA_FILES_MEMORYFINDER, 0 );

								//	Update the text to match
								char buffer[ 16 ];
								sprintf_s( buffer, 16, "0x%06x", rCond.CompSource().RawValue() );
								SetDlgItemText( g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, Widen( buffer ).c_str() );

								//	Nudge the ComboBox to update the mem note
								SendMessage( g_MemoryDialog.GetHWND(), WM_COMMAND, MAKELONG( IDC_RA_WATCHING, CBN_EDITCHANGE ), 0 );
							}
						}
						else if( pOnClick->iSubItem == CSI_VALUE_TGT )
						{
							if( rCond.CompTarget().Type() != ValueComparison )
							{
								//	Wake up the mem dlg via the main app
								SendMessage( g_RAMainWnd, WM_COMMAND, IDM_RA_FILES_MEMORYFINDER, 0 );

								//	Update the text to match
								char buffer[ 16 ];
								sprintf_s( buffer, 16, "0x%06x", rCond.CompTarget().RawValue() );
								SetDlgItemText( g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, Widen( buffer ).c_str() );

								//	Nudge the ComboBox to update the mem note
								SendMessage( g_MemoryDialog.GetHWND(), WM_COMMAND, MAKELONG( IDC_RA_WATCHING, CBN_EDITCHANGE ), 0 );
							}
						}
					}
					bHandled = TRUE;
				}
				break;

			case LVN_ENDLABELEDIT:
				{
					NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)(lParam);

					Achievement* pActiveAch = ActiveAchievement();
					if( pActiveAch == NULL )
						return FALSE;

					if( pDispInfo->item.iItem < 0 || pDispInfo->item.iItem >= (int)pActiveAch->NumConditions( GetSelectedConditionGroup() ) )
						return FALSE;

					LVITEM lvItem;
					lvItem.iItem = pDispInfo->item.iItem;
					lvItem.iSubItem = pDispInfo->item.iSubItem;
					lvItem.pszText = pDispInfo->item.pszText;
					if( pDispInfo->item.iItem == -1 ||
						pDispInfo->item.iSubItem == -1 )
					{
						nSelItem = -1;
						nSelSubItem = -1;
						break;
					}

					//	Get cached data:
					char* sData = LbxDataAt( pDispInfo->item.iItem, pDispInfo->item.iSubItem );

					//	Update modified flag:
					if( strcmp( Narrow( lvItem.pszText ).c_str(), sData ) != 0 )
					{
						//	Disable all achievement tracking:
						//g_pActiveAchievements->SetPaused( true );
						//CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );

						//	Update this achievement as 'modified'
						pActiveAch->SetModified( TRUE );
						g_AchievementsDialog.OnEditAchievement( *pActiveAch );
					}

					//	Inject the new text into the lbx
					SendDlgItemMessage( hDlg, IDC_RA_LBX_CONDITIONS, LVM_SETITEMTEXT, (WPARAM)lvItem.iItem, (LPARAM)&lvItem ); // put new text
					
					//	Update the cached data:
					strcpy_s( sData, MEM_STRING_TEXT_LEN, Narrow( pDispInfo->item.pszText ).c_str() );

					//	Update the achievement data:
					Condition& rCond = pActiveAch->GetCondition( GetSelectedConditionGroup(), pDispInfo->item.iItem );
					switch( pDispInfo->item.iSubItem )
					{
						case CSI_TYPE_SRC:
							{
								if( strcmp( sData, "Mem" ) == 0 )
									rCond.CompSource().SetType( Address );
								else if( strcmp( sData, "Delta" ) == 0 )
									rCond.CompSource().SetType( DeltaMem );
								else
									rCond.CompSource().SetType( ValueComparison );

								break;
							}
						case CSI_TYPE_TGT:
							{
								if( strcmp( sData, "Mem" ) == 0 )
									rCond.CompTarget().SetType( Address );
								else if( strcmp( sData, "Delta" ) == 0 )
									rCond.CompTarget().SetType( DeltaMem );
								else
									rCond.CompTarget().SetType( ValueComparison );

								break;
							}

						case CSI_SIZE_SRC:
							{
								for( int i = 0; i < NumComparisonVariableSizeTypes; ++i )
								{
									if( strcmp( sData, COMPARISONVARIABLESIZE_STR[ i ] ) == 0 )
										rCond.CompSource().SetSize( static_cast<ComparisonVariableSize>( i ) );
								}
								//	TBD: Limit validation
								break;
							}
						case CSI_SIZE_TGT:
							{
								for( int i = 0; i < NumComparisonVariableSizeTypes; ++i )
								{
									if( strcmp( sData, COMPARISONVARIABLESIZE_STR[ i ] ) == 0 )
										rCond.CompTarget().SetSize( static_cast<ComparisonVariableSize>( i ) );
								}
								//	TBD: Limit validation
								break;
							}
						case CSI_COMPARISON:
							{
								for( int i = 0; i < NumComparisonTypes; ++i )
								{
									if( strcmp( sData, COMPARISONTYPE_STR[ i ] ) == 0 )
										rCond.SetCompareType( static_cast<ComparisonType>( i ) );
								}
								break;
							}

						case CSI_VALUE_SRC:
							{
								int nBase = 16;
								if( rCond.CompSource().Type() == ComparisonVariableType::ValueComparison && g_bPreferDecimalVal )
									nBase = 10;

								unsigned int nVal = strtol( sData, NULL, nBase );
								rCond.CompSource().SetValues( nVal, nVal );
								break;
							}
						case CSI_VALUE_TGT:
							{
								int nBase = 16;
								if( rCond.CompTarget().Type() == ComparisonVariableType::ValueComparison && g_bPreferDecimalVal )
									nBase = 10;
								
								unsigned int nVal = strtol( sData, NULL, nBase );
								rCond.CompTarget().SetValues( nVal, nVal );
								break;
							}
						case CSI_HITCOUNT:
							{
								//	Always decimal
								rCond.SetRequiredHits( strtol( sData, NULL, 10 ) );
								break;
							}
						default:
							ASSERT( !"Unhandled!" );
							break;
					}

					nSelItem = -1;
					nSelSubItem = -1;

				}
				break;
			}
		}
		break;
	case WM_CLOSE:
		EndDialog(hDlg, true);
		bHandled = TRUE;
		break;
// 	case WM_LBUTTONDOWN:
// 		{
// 			break;
// 		}
	}

	return !bHandled;
	//return DefWindowProc( hDlg, uMsg, wParam, lParam );
}

void Dlg_AchievementEditor::UpdateSelectedBadgeImage( const std::string& sBackupBadgeToUse )
{
	std::string sAchievementBadgeURI = RA_UNKNOWN_BADGE_IMAGE_URI;

	if( m_pSelectedAchievement != nullptr )
		sAchievementBadgeURI = m_pSelectedAchievement->BadgeImageURI();
	else if( sBackupBadgeToUse.length() > 2 )
		sAchievementBadgeURI = sBackupBadgeToUse;

	if( m_hAchievementBadge != nullptr )
		DeleteObject( m_hAchievementBadge );
	m_hAchievementBadge = nullptr;

	HBITMAP hBitmap = LoadOrFetchBadge( sAchievementBadgeURI, RA_BADGE_PX );
	if( hBitmap == nullptr )
		return;

	m_hAchievementBadge = hBitmap;

	SendMessage( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_CHEEVOPIC ), STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)m_hAchievementBadge );
	InvalidateRect( m_hAchievementEditorDlg, NULL, TRUE );

	//	Find buffer in the dropdown list
	int nSel = ComboBox_FindStringExact( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_BADGENAME ), 0, sAchievementBadgeURI.c_str() );
	if( nSel != -1 )
		ComboBox_SetCurSel( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_BADGENAME ), nSel );	//	Force select
}

void Dlg_AchievementEditor::UpdateBadge( const std::string& sNewName )
{
	//	If a change is detected: change it!
	if( m_pSelectedAchievement != NULL )
	{
		if( m_pSelectedAchievement->BadgeImageURI().compare( sNewName ) != 0 )
		{
			//	The badge we are about to show is different from the one stored for this achievement.
			//	This implies that we are changing the badge: this achievement is modified!
			m_pSelectedAchievement->SetBadgeImage( sNewName );
			m_pSelectedAchievement->SetModified( TRUE );

			if( g_nActiveAchievementSet == Core )
			{
				int nOffs = g_AchievementsDialog.GetSelectedAchievementIndex();
				g_AchievementsDialog.OnEditData( nOffs, Dlg_Achievements::Modified, "Yes" );
			}
		}
	}

	//	Always attempt update.
	UpdateSelectedBadgeImage( sNewName.c_str() );
}

void Dlg_AchievementEditor::RepopulateGroupList( Achievement* pCheevo )
{
	HWND hGroupList = GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_GROUP );
	if( hGroupList == NULL )
		return;

	int nSel = ListBox_GetCurSel( hGroupList );

	while( ListBox_DeleteString( hGroupList, 0 ) >= 0 ) {}

	if( pCheevo != NULL )
	{
		for( size_t i = 0; i < pCheevo->NumConditionGroups(); ++i )
		{
			ListBox_AddString( hGroupList, m_lbxGroupNames[i] );
		}
	}

	//	Try and restore selection
	if( nSel < 0 || nSel >= (int)pCheevo->NumConditionGroups() )
		nSel = 0;	//	Reset to core if unsure
	
	ListBox_SetCurSel( hGroupList, nSel );
}

void Dlg_AchievementEditor::PopulateConditions( Achievement* pCheevo )
{
	HWND hCondList = GetDlgItem( m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS );
	if( hCondList == NULL )
		return;

	ListView_DeleteAllItems( hCondList );

	unsigned int nGrp = GetSelectedConditionGroup();

	m_nNumOccupiedRows = 0;

	if( pCheevo != NULL )
	{
		//for( size_t nGrp = 0; nGrp < pCheevo->NumConditionGroups(); ++nGrp )
		{
			for( size_t i = 0; i < m_pSelectedAchievement->NumConditions( nGrp ); ++i )
				AddCondition( hCondList, m_pSelectedAchievement->GetCondition( nGrp, i ) );
		}
	}
}

void Dlg_AchievementEditor::LoadAchievement( Achievement* pCheevo, BOOL bAttemptKeepSelected )
{
	char buffer[1024];

	if( pCheevo == NULL )
	{
		m_pSelectedAchievement = pCheevo;

		//	Just clear data

		m_bPopulatingAchievementEditorData = TRUE;

		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_ID, L"" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_POINTS, L"" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_TITLE, L"<Inactive!>" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_DESC, L"<Select or Create an Achievement first!>" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR, L"<Inactive!>" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_BADGENAME, L"" );

		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_ID ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_TITLE ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_DESC ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_POINTS ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ADDCOND ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_CLONECOND ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_DELETECOND ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_GROUP ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_PROGRESSINDICATORS ), FALSE );

		UpdateBadge( "00000" );

		m_bPopulatingAchievementEditorData = FALSE;
	}
	else if( m_pSelectedAchievement != pCheevo )
	{
		m_pSelectedAchievement = pCheevo;

		//	New achievement selected.
		//	Naturally: update everything!

		m_bPopulatingAchievementEditorData = TRUE;

		//	Repopulate the group box: 
		RepopulateGroupList( m_pSelectedAchievement );
		SetSelectedConditionGroup( 0 );	//	Select 0 by default
		PopulateConditions( m_pSelectedAchievement );

		sprintf_s( buffer, 1024, "%d", m_pSelectedAchievement->ID() );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_ID, Widen( buffer ).c_str() );

		sprintf_s( buffer, 1024, "%d", m_pSelectedAchievement->Points() );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_POINTS, Widen( buffer ).c_str() );

		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_TITLE, Widen( m_pSelectedAchievement->Title() ).c_str() );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_DESC, Widen( m_pSelectedAchievement->Description() ).c_str() );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR, Widen( m_pSelectedAchievement->Author() ).c_str() );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_BADGENAME, Widen( m_pSelectedAchievement->BadgeImageURI() ).c_str() );

		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_ID ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_TITLE ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_DESC ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_POINTS ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ADDCOND ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_CLONECOND ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_DELETECOND ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_GROUP ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_PROGRESSINDICATORS ), FALSE );

		UpdateBadge( m_pSelectedAchievement->BadgeImageURI() );

		m_bPopulatingAchievementEditorData = FALSE;
	}
	else
	{
		//	Same achievement still selected: what's changed?
		
		BOOL bTitleSelected  = ( GetFocus() == GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_TITLE ) );
		BOOL bDescSelected	 = ( GetFocus() == GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_DESC ) );
		BOOL bPointsSelected = ( GetFocus() == GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_POINTS ) );
		HWND hCtrl = GetDlgItem( m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS );

		int nScrollTo = -1;
		int nSelected = -1;
		if( bAttemptKeepSelected )
		{
			SCROLLINFO scroll;
			scroll.cbSize = sizeof( SCROLLINFO );
			scroll.fMask = SIF_POS;
			GetScrollInfo( hCtrl, SB_VERT, &scroll );
			nScrollTo = scroll.nPos;

			//	Get selected item:
			nSelected = ListView_GetNextItem( hCtrl, -1, LVNI_SELECTED );
		}

		if( !m_pSelectedAchievement->IsDirty() )
			return;

		m_bPopulatingAchievementEditorData = TRUE;

		if( pCheevo->GetDirtyFlags() & Dirty_ID )
			SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_ID, std::to_wstring( m_pSelectedAchievement->ID() ).c_str() );
		if( ( pCheevo->GetDirtyFlags() & Dirty_Points ) && !bPointsSelected )
			SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_POINTS, std::to_wstring( m_pSelectedAchievement->Points() ).c_str() );
		if( ( pCheevo->GetDirtyFlags() & Dirty_Title ) && !bTitleSelected )
			SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_TITLE, Widen( m_pSelectedAchievement->Title() ).c_str() );
		if( ( pCheevo->GetDirtyFlags() & Dirty_Description ) && !bDescSelected )
			SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_DESC, Widen( m_pSelectedAchievement->Description() ).c_str() );
		if( pCheevo->GetDirtyFlags() & Dirty_Author )
			SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR, Widen( m_pSelectedAchievement->Author() ).c_str() );
		if( pCheevo->GetDirtyFlags() & Dirty_Badge )
		{
			SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_BADGENAME, Widen( m_pSelectedAchievement->BadgeImageURI() ).c_str() );
			UpdateBadge( m_pSelectedAchievement->BadgeImageURI() );
		}
		if( pCheevo->GetDirtyFlags() & Dirty_Conditions )
			PopulateConditions( m_pSelectedAchievement );
		
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_ID ), FALSE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_TITLE ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_DESC ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_POINTS ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ADDCOND ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_CLONECOND ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_DELETECOND ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_GROUP ), TRUE );
		EnableWindow( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_PROGRESSINDICATORS ), FALSE );
		
		m_bPopulatingAchievementEditorData = FALSE;

		if( bAttemptKeepSelected && ( nScrollTo != 0 || nSelected != -1 ) )
		{
			//	Note: bit of a hack, but this actually selects a condition. We don't want to also be doing this 
			//	 if the scroller is right at the top.
			if( nScrollTo > 0 )
			{
				ListView_SetItemState( hCtrl, nScrollTo, LVIS_FOCUSED|LVIS_SELECTED, -1 );
				ListView_EnsureVisible( hCtrl, nScrollTo, FALSE );
			}
			else if( nSelected >= 0 )
			{
				ListView_SetItemState( hCtrl, nSelected, LVIS_FOCUSED|LVIS_SELECTED, -1 )
				ListView_EnsureVisible( hCtrl, nSelected, FALSE );
			}
		}
	}

	//InvalidateRect( hList, NULL, TRUE );
	//UpdateWindow( hList );
	//RedrawWindow( hList, NULL, NULL, RDW_INVALIDATE );
}

void Dlg_AchievementEditor::OnLoad_NewRom()
{
	HWND hList = GetDlgItem( m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS );
	if( hList != nullptr )
	{
		SetIgnoreEdits( TRUE );
		SetupColumns( hList );
		
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_ID, L"" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_POINTS, L"" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_TITLE, L"" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_DESC, L"" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR, L"" );

		m_pSelectedAchievement = nullptr;
		SetIgnoreEdits( FALSE );
		LoadAchievement( nullptr, FALSE );
	}
}


size_t Dlg_AchievementEditor::GetSelectedConditionGroup() const
{
	HWND hList = GetDlgItem( g_AchievementEditorDialog.GetHWND(), IDC_RA_ACH_GROUP );
	int nSel = ListBox_GetCurSel( hList );
	if( nSel == LB_ERR )
	{
		OutputDebugString( L"ListBox_GetCurSel returning LB_ERR\n" );
		return 0;
	}

	return static_cast<size_t>( nSel );
}

void Dlg_AchievementEditor::SetSelectedConditionGroup( size_t nGrp ) const
{
	HWND hList = GetDlgItem( g_AchievementEditorDialog.GetHWND(), IDC_RA_ACH_GROUP );
	ListBox_SetCurSel( hList, static_cast<int>( nGrp ) );
}


void BadgeNames::FetchNewBadgeNamesThreaded()
{
	RAWeb::CreateThreadedHTTPRequest( RequestBadgeIter );
}

void BadgeNames::OnNewBadgeNames( const Document& data )
{
	unsigned int nLowerLimit = data[ "FirstBadge" ].GetUint();
	unsigned int nUpperLimit = data[ "NextBadge" ].GetUint();

	//	Clean out cbo
	while( ComboBox_DeleteString( m_hDestComboBox, 0 ) > 0 )
	{
	}

	for( unsigned int i = nLowerLimit; i < nUpperLimit; ++i )
	{
		wchar_t buffer[ 256 ];
		wsprintf( buffer, L"%05d", i );
		ComboBox_AddString( m_hDestComboBox, buffer );
	}
}

void BadgeNames::AddNewBadgeName( const char* pStr, bool bAndSelect )
{
	int nSel = ComboBox_AddString( m_hDestComboBox, Widen( pStr ).c_str() );

	if( bAndSelect )
	{
		ComboBox_SelectString( m_hDestComboBox, 0, pStr );
		//ComboBox_SetCurSel( m_hDestComboBox, nSel );
	}
}