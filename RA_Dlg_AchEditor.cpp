#include "RA_Dlg_AchEditor.h"

#include <windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <stdio.h>
#include <assert.h>

#include "RA_Achievement.h"
#include "RA_Resource.h"
#include "RA_Defs.h"
#include "RA_Core.h"
#include "RA_Dlg_Achievement.h"
#include "RA_Dlg_Memory.h"
#include "RA_httpthread.h"
#include "RA_User.h"
#include "RA_ImageFactory.h"
#include "RA_MemManager.h"

#if defined (RA_GENS)
//#include "g_main.h"
//#include "G_dsound.h"
//#include "Rom.h"
#elif defined (RA_VBA)
#endif


#pragma comment(lib, "comctl32.lib")

//	NOTE: ENSURE that these match up with the definitions in Achievement.h!
//	Add static assert?

const char* g_sColTitles[] = { "ID", "Special?", "Type", "Size", "Memory", "Cmp", "Type", "Size", "Mem/Val", "Hits" };
const int g_nColSizes[] = { 30, 53, 42, 50, 60, 35, 42, 50, 60, 48 };

enum CondSubItems
{
	CSI_ID = 0,
	CSI_GROUP,
	CSI_TYPE_SRC,
	CSI_SIZE_SRC,
	CSI_VALUE_SRC,
	CSI_COMPARISON,
	CSI_TYPE_TGT,
	CSI_SIZE_TGT,
	CSI_VALUE_TGT,
	CSI_HITCOUNT,
	CSI__NUMCOLUMNS
};

BOOL g_bPreferDecimalVal = TRUE;

//static 
HWND BadgeNames::m_hDestComboBox = NULL;

Dlg_AchievementEditor g_AchievementEditorDialog;

INT_PTR CALLBACK AchProgressProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL bHandled = FALSE;

	switch(message)
	{

	case WM_INITDIALOG:
	{
		Achievement* pAch = g_AchievementEditorDialog.ActiveAchievement();
		if( pAch == NULL )
			return FALSE;

		SetWindowText( GetDlgItem( hDlg, IDC_RA_ACHPROGRESS_FORMULA ), pAch->Progress() );
		SetWindowText( GetDlgItem( hDlg, IDC_RA_ACHPROGRESS_MAXIMUM ), pAch->ProgressMax() );
		SetWindowText( GetDlgItem( hDlg, IDC_RA_ACHPROGRESS_FORMATTING ), pAch->ProgressFmt() );

		bHandled = TRUE;
	}
	break;

	case WM_COMMAND:
	{
		switch(LOWORD(wParam))
		{
		case IDC_RA_ACHPROGRESSENABLE:
		{
			BOOL bEnabled = IsDlgButtonChecked(hDlg, IDC_RA_ACHPROGRESSENABLE );
			EnableWindow( GetDlgItem( hDlg, IDC_RA_ACHPROGRESS_FORMULA ), bEnabled ); 
			EnableWindow( GetDlgItem( hDlg, IDC_RA_ACHPROGRESS_MAXIMUM ), bEnabled ); 
			EnableWindow( GetDlgItem( hDlg, IDC_RA_ACHPROGRESS_FORMATTING ), bEnabled ); 
			EnableWindow( GetDlgItem( hDlg, IDC_RA_ACHPROGRESS_EXAMPLE ), bEnabled ); 
			break;
		}

		case IDOK:
		case IDCLOSE:
			EndDialog(hDlg, true);
			bHandled = TRUE;
			break;
		}

		break;
	}

	case WM_DESTROY:
	{
		EndDialog( hDlg, true );
		break;
	}

	}

	return bHandled;
}



Dlg_AchievementEditor::Dlg_AchievementEditor()
{
	m_hAchievementEditorDlg = NULL;
	m_hICEControl = NULL;
	m_pSelectedAchievement = NULL;
	m_bPopulatingAchievementEditorData = false;
	m_hAchievementBadge = NULL;

	for( size_t i = 0; i < g_nMaxConditions; ++i )
	{
		if( i == 0 )
			sprintf_s( m_lbxGroupNames[i], g_nMaxMemStringTextItemSize, "Core" );
		else
			sprintf_s( m_lbxGroupNames[i], g_nMaxMemStringTextItemSize, "Alt %02d", i );
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

	char buffer[256];

	LV_COLUMN col;
	ZeroMemory( &col, sizeof( col ) );

	for( size_t i = 0; i < m_nNumCols; ++i )
	{
		col.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM|LVCF_FMT;
		col.cx = g_nColSizes[i];
		col.cchTextMax = 255;
		sprintf_s( buffer, 256, g_sColTitles[i] );
		col.pszText = buffer;
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

	BOOL bDeltaSrc = ( Cond.CompSource().m_nVarType == CMPTYPE_DELTAMEM );
	BOOL bDeltaDst = ( Cond.CompTarget().m_nVarType == CMPTYPE_DELTAMEM );

	unsigned int nValSrc = Cond.CompSource().m_nVal;
	unsigned int nValDst = Cond.CompTarget().m_nVal;

	unsigned int nValTypeSrc = Cond.CompSource().m_nVarSize;
	unsigned int nValTypeDst = Cond.CompTarget().m_nVarSize;

	const char* sCmpStr = g_CmpStrings[ (int)Cond.ComparisonType() ];

	const char* sMemTypStrSrc = "Value";
	const char* sMemTypStrDst = "Value";
	const char* sMemSizeStrSrc = "";
	const char* sMemSizeStrDst = "";

	if( Cond.CompSource().m_nVarType != CMPTYPE_VALUE )
	{
		sMemSizeStrSrc = g_MemSizeStrings[ (int)Cond.CompSource().m_nVarSize ];
		sMemTypStrSrc = Cond.CompSource().m_nVarType==CMPTYPE_ADDRESS ? "Mem" : "Delta";
	}

	if( Cond.CompTarget().m_nVarType != CMPTYPE_VALUE )
	{
		sMemSizeStrDst = g_MemSizeStrings[ (int)Cond.CompTarget().m_nVarSize ];
		sMemTypStrDst = Cond.CompTarget().m_nVarType==CMPTYPE_ADDRESS ? "Mem" : "Delta";
	}
	
	const char* sGroup = Cond.IsResetCondition() ? "ResetIf:" : Cond.IsPauseCondition() ? "PauseIf:" : "";

	sprintf_s( m_lbxData[m_nNumOccupiedRows][CSI_ID], g_nMaxMemStringTextItemSize, "%d", m_nNumOccupiedRows+1 );
	strcpy_s( m_lbxData[m_nNumOccupiedRows][CSI_GROUP], g_nMaxMemStringTextItemSize, sGroup );
	strcpy_s( m_lbxData[m_nNumOccupiedRows][CSI_TYPE_SRC], g_nMaxMemStringTextItemSize, sMemTypStrSrc );
	strcpy_s( m_lbxData[m_nNumOccupiedRows][CSI_SIZE_SRC], g_nMaxMemStringTextItemSize, sMemSizeStrSrc );
	if( g_MemManager.RAMTotalSize() > 65536 )
		sprintf_s( m_lbxData[m_nNumOccupiedRows][CSI_VALUE_SRC], g_nMaxMemStringTextItemSize, "0x%06x", nValSrc );
	else
		sprintf_s( m_lbxData[m_nNumOccupiedRows][CSI_VALUE_SRC], g_nMaxMemStringTextItemSize, "0x%04x", nValSrc );
	strcpy_s( m_lbxData[m_nNumOccupiedRows][CSI_COMPARISON], g_nMaxMemStringTextItemSize, sCmpStr );
	strcpy_s( m_lbxData[m_nNumOccupiedRows][CSI_TYPE_TGT], g_nMaxMemStringTextItemSize, sMemTypStrDst );
	strcpy_s( m_lbxData[m_nNumOccupiedRows][CSI_SIZE_TGT], g_nMaxMemStringTextItemSize, sMemSizeStrDst );
	sprintf_s( m_lbxData[m_nNumOccupiedRows][CSI_VALUE_TGT], g_nMaxMemStringTextItemSize, "0x%02x", nValDst );
	sprintf_s( m_lbxData[m_nNumOccupiedRows][CSI_HITCOUNT], g_nMaxMemStringTextItemSize, "%d (%d)", Cond.RequiredHits(), Cond.CurrentHits() );

	if( g_bPreferDecimalVal )
	{
		if( Cond.CompTarget().m_nVarType == CMPTYPE_VALUE )
		{
			sprintf_s( m_lbxData[m_nNumOccupiedRows][CSI_VALUE_TGT], g_nMaxMemStringTextItemSize, "%d", nValDst );
		}
	}

	//	Copy our local text into the listbox (:S)

	LV_ITEM item;
	ZeroMemory( &item, sizeof( item ) );

	item.mask = LVIF_TEXT;
	item.cchTextMax = 256;
	item.iItem = m_nNumOccupiedRows;

	item.iSubItem = 0;
	item.pszText = m_lbxData[m_nNumOccupiedRows][CSI_ID];
	item.iItem = ListView_InsertItem( hList, &item );

	for( size_t i = 1; i < CSI__NUMCOLUMNS; ++i )
	{
		item.iSubItem = i;
		item.pszText = m_lbxData[m_nNumOccupiedRows][i];
		ListView_SetItem( hList, &item );
	}

	assert( item.iItem == m_nNumOccupiedRows );

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
// 			//MessageBox( NULL, "ICEDialogProc init!", "TEST", MB_OK );
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
// 				MessageBox( NULL, buffer, "NewVal!", MB_OK );
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

long _stdcall EditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_DESTROY:
			g_hIPEEdit = NULL;
			break;
		case WM_KILLFOCUS:
		{
			LV_DISPINFO lvDispinfo;
			ZeroMemory(&lvDispinfo,sizeof(LV_DISPINFO));
			lvDispinfo.hdr.hwndFrom = hwnd;
			lvDispinfo.hdr.idFrom = GetDlgCtrlID(hwnd);
			lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
			lvDispinfo.item.mask = LVIF_TEXT;
			lvDispinfo.item.iItem = nSelItem;
			lvDispinfo.item.iSubItem = nSelSubItem;
			lvDispinfo.item.pszText = NULL;

			char szEditText[12];
			GetWindowText( hwnd, szEditText, 12 );
			lvDispinfo.item.pszText = szEditText;
			lvDispinfo.item.cchTextMax = lstrlen(szEditText);

			HWND hList = GetDlgItem( g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS );

			//	the LV ID and the LVs Parent window's HWND
			SendMessage( GetParent( hList ), WM_NOTIFY, (WPARAM)IDC_RA_LBX_CONDITIONS, (LPARAM)&lvDispinfo );

			//	Extra validation?
			
			if( strcmp( szEditText, "Value" ) )
			{
				//	Remove the associated 'size' entry
				strcpy_s( g_AchievementEditorDialog.LbxDataAt( nSelItem, nSelSubItem+1 ), 32, "" );
			}

			DestroyWindow(hwnd);
			break;
		}
 		case WM_KEYDOWN:
 		{
 			if( wParam == VK_RETURN || wParam == VK_ESCAPE )
 			{
				DestroyWindow(hwnd);	//	Causing a killfocus :)
			}
			else
			{
				//	Ignore keystroke, or pass it into the edit box!
				break;
			}

 			break;
 		}
	}

	return CallWindowProc(EOldProc, hwnd, message, wParam, lParam);
}

long _stdcall DropDownProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if( message == WM_COMMAND )
	{
		//if( LOWORD(wParam) == IDC_RA_LBX_CONDITIONS )
		{
			if( HIWORD(wParam) == CBN_SELCHANGE )
			{
				LV_DISPINFO lvDispinfo;
				ZeroMemory(&lvDispinfo,sizeof(LV_DISPINFO));
				lvDispinfo.hdr.hwndFrom = hwnd;
				lvDispinfo.hdr.idFrom = GetDlgCtrlID(hwnd);
				lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
				lvDispinfo.item.mask = LVIF_TEXT;
				lvDispinfo.item.iItem = nSelItem;
				lvDispinfo.item.iSubItem = nSelSubItem;
				lvDispinfo.item.pszText = NULL;

				char szEditText[12];
				GetWindowText( hwnd, szEditText, 12 );
				lvDispinfo.item.pszText = szEditText;
				lvDispinfo.item.cchTextMax = lstrlen(szEditText);

				HWND hList = GetDlgItem( g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS );

				//	the LV ID and the LVs Parent window's HWND
				SendMessage( GetParent( hList ), WM_NOTIFY, (WPARAM)IDC_RA_LBX_CONDITIONS, (LPARAM)&lvDispinfo );

				//DestroyWindow(hwnd);
				DestroyWindow(hwnd);
			}
		}
	}

	switch(message)
	{
	case WM_DESTROY:
		g_hIPEEdit = NULL;
		break;
	case WM_KILLFOCUS:
		{
			LV_DISPINFO lvDispinfo;
			ZeroMemory(&lvDispinfo,sizeof(LV_DISPINFO));
			lvDispinfo.hdr.hwndFrom = hwnd;
			lvDispinfo.hdr.idFrom = GetDlgCtrlID(hwnd);
			lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
			lvDispinfo.item.mask = LVIF_TEXT;
			lvDispinfo.item.iItem = nSelItem;
			lvDispinfo.item.iSubItem = nSelSubItem;
			lvDispinfo.item.pszText = NULL;

			char szEditText[12];
			GetWindowText( hwnd, szEditText, 12 );
			lvDispinfo.item.pszText = szEditText;
			lvDispinfo.item.cchTextMax = lstrlen(szEditText);

			HWND hList = GetDlgItem( g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS );

			//	the LV ID and the LVs Parent window's HWND
			SendMessage( GetParent( hList ), WM_NOTIFY, (WPARAM)IDC_RA_LBX_CONDITIONS, (LPARAM)&lvDispinfo );

			//DestroyWindow(hwnd);
			DestroyWindow(hwnd);
		}
		break;
	case WM_KEYDOWN:
		{
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

				DestroyWindow(hwnd);
			}
			else if( wParam == VK_ESCAPE )
			{
				//	Undo changes: i.e. simply destroy the window!
				DestroyWindow(hwnd);
			}
			else
			{
				//	Ignore keystroke, or pass it into the edit box!
				break;
			}

			break;
		}
	case WM_COMMAND:
		switch( HIWORD( wParam ) )
		{
		case CBN_SELENDOK:
		case CBN_SELENDCANCEL:
		case CBN_SELCHANGE:
		case CBN_CLOSEUP:
		case CBN_KILLFOCUS:
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
// 
// 				DestroyWindow(hwnd);
			}
			break;
		}
	//case WM_MOUSE:
	//	{
//
//
	//		break;
	//	}

	}
	return CallWindowProc(EOldProc, hwnd, message, wParam, lParam);
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
		assert(0);	//	nothing we do if we click the first element!
		break;
	case CSI_GROUP:
		{
			//	Group: BOOL (dropdown?)

			if( g_AchievementEditorDialog.ActiveAchievement() )
			{
				const size_t nGrp = g_AchievementEditorDialog.GetSelectedConditionGroup();
				Condition& rCond = g_AchievementEditorDialog.ActiveAchievement()->GetCondition( nGrp, nItem );
				//char* sNewText = rCond.IsResetCondition() ? "" : "ResetIf:";
				char* sNewText = rCond.IsResetCondition() ? "PauseIf:" : rCond.IsPauseCondition() ? "" : "ResetIf:";

				HWND hList = GetDlgItem( g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS );

				LVITEM lvItem;
				ZeroMemory( &lvItem, sizeof(lvItem) );
				lvItem.iItem = nItem;
				lvItem.iSubItem = nSubItem;
				lvItem.pszText = sNewText;
				lvItem.cchTextMax = 256;

				//	Inject the new text into the lbx
				SendDlgItemMessage( g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS, LVM_SETITEMTEXT, (WPARAM)lvItem.iItem, (LPARAM)&lvItem ); // put new text

				//	Update the cached data:
				char* sData = g_AchievementEditorDialog.LbxDataAt( nItem, nSubItem );
				strcpy_s( sData, g_nMaxMemStringTextItemSize, sNewText );

				//	Update the achievement data
				if( strcmp( sNewText, "ResetIf:" ) == 0 )
				{
					rCond.SetIsResetCondition();
				}
				else if( strcmp( sNewText, "PauseIf:" ) == 0 )
				{
					rCond.SetIsPauseCondition();
				}
				else
				{
					rCond.SetIsBasicCondition();
				}
			}

			break;
		}
		break;
	case CSI_TYPE_SRC:
	case CSI_TYPE_TGT:
		{
			//	Type: dropdown

			assert( g_hIPEEdit == NULL );
			if( g_hIPEEdit )
				break;

			const int nNumItems = 3;	//	"Mem", "Delta" or "Value"

			g_hIPEEdit = CreateWindowEx( 
				WS_EX_CLIENTEDGE, 
				"ComboBox", 
				"", 
				WS_CHILD|WS_VISIBLE|WS_POPUPWINDOW|WS_BORDER|CBS_DROPDOWNLIST,
				rcSubItem.left, rcSubItem.top, nWidth, (int)(1.6f*nHeight*nNumItems),
				g_AchievementEditorDialog.GetHWND(), 
				0, 
				GetModuleHandle(NULL), 
				NULL );

			if( g_hIPEEdit == NULL )
			{
				assert(0);
				MessageBox( NULL, "Could not create edit box.", "Error", MB_OK | MB_ICONERROR );
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
			ComboBox_AddString( g_hIPEEdit, "Mem" );
			ComboBox_AddString( g_hIPEEdit, "Delta" );
			ComboBox_AddString( g_hIPEEdit, "Value" );
			
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
			assert( g_hIPEEdit == NULL );
			if( g_hIPEEdit )
				break;

			//	Note: this relies on column order :S
			if( strcmp( g_AchievementEditorDialog.LbxDataAt( nItem, nSubItem-1 ), "Value" ) == 0 )
			{
				//	Values have no size.
				break;
			}

			const int nNumItems = g_NumMemSizeStrings;	//	"4-bit upper, lower, 8-bit, 16-bit"

			g_hIPEEdit = CreateWindowEx( 
				WS_EX_CLIENTEDGE, 
				"ComboBox", 
				"", 
				WS_CHILD|WS_VISIBLE|WS_POPUPWINDOW|WS_BORDER|CBS_DROPDOWNLIST,
				rcSubItem.left, rcSubItem.top, nWidth, (int)(1.6f*nHeight*nNumItems),
				g_AchievementEditorDialog.GetHWND(), 
				0, 
				GetModuleHandle(NULL), 
				NULL );

			if( g_hIPEEdit == NULL )
			{
				assert(0);
				MessageBox( NULL, "Could not create combo box.", "Error", MB_OK | MB_ICONERROR );
				break;
			};

			for( int i = 0; i < g_NumMemSizeStrings; ++i )
			{
				ComboBox_AddString( g_hIPEEdit, g_MemSizeStrings[i] );

				if( strcmp( g_AchievementEditorDialog.LbxDataAt( nItem, nSubItem ), g_MemSizeStrings[i] ) == 0 )
					ComboBox_SetCurSel( g_hIPEEdit, i );
			}

			SendMessage( g_hIPEEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),  TRUE);
			ComboBox_ShowDropdown( g_hIPEEdit, TRUE );

			EOldProc = (WNDPROC)SetWindowLong( g_hIPEEdit, GWL_WNDPROC, (LONG)DropDownProc );
		}
		break;
	case CSI_COMPARISON:
		{
			//	Compare: dropdown
			assert( g_hIPEEdit == NULL );
			if( g_hIPEEdit )
				break;

			const int nNumItems = g_NumCompTypes;

			g_hIPEEdit = CreateWindowEx( 
				WS_EX_CLIENTEDGE, 
				"ComboBox", 
				"", 
				WS_CHILD|WS_VISIBLE|WS_POPUPWINDOW|WS_BORDER|CBS_DROPDOWNLIST,
				rcSubItem.left, rcSubItem.top, nWidth, (int)(1.6f*nHeight*nNumItems),
				g_AchievementEditorDialog.GetHWND(), 
				0, 
				GetModuleHandle(NULL), 
				NULL );

			if( g_hIPEEdit == NULL )
			{
				assert(0);
				MessageBox( NULL, "Could not create combo box.", "Error", MB_OK | MB_ICONERROR );
				break;
			};

			for( int i = 0; i < g_NumCompTypes; ++i )
			{
				ComboBox_AddString( g_hIPEEdit, g_CmpStrings[i] );

				if( strcmp( g_AchievementEditorDialog.LbxDataAt( nItem, nSubItem ), g_CmpStrings[i] ) == 0 )
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
			assert( g_hIPEEdit == NULL );
			if( g_hIPEEdit )
				break;

			g_hIPEEdit = CreateWindowEx( 
				WS_EX_CLIENTEDGE, 
				"EDIT", 
				"", 
				WS_CHILD|WS_VISIBLE|WS_POPUPWINDOW|WS_BORDER|ES_WANTRETURN,
				rcSubItem.left, rcSubItem.top, nWidth, (int)(1.5f*nHeight), 
				g_AchievementEditorDialog.GetHWND(), 
				0, 
				GetModuleHandle(NULL), 
				NULL );

			if( g_hIPEEdit == NULL )
			{
				assert(0);
				MessageBox( NULL, "Could not create edit box.", "Error", MB_OK | MB_ICONERROR );
				break;
			};

			SendMessage( g_hIPEEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT),  TRUE);

			char* pData = g_AchievementEditorDialog.LbxDataAt( nItem, nSubItem );
			SetWindowText( g_hIPEEdit, pData );
			
			//	Special case, hitcounts
			if( nSubItem == CSI_HITCOUNT )
			{
				if( g_AchievementEditorDialog.ActiveAchievement() != NULL )
				{
					const size_t nGrp = g_AchievementEditorDialog.GetSelectedConditionGroup();
					const Condition& Cond = g_AchievementEditorDialog.ActiveAchievement()->GetCondition( nGrp, nItem );

					char buffer[256];
					sprintf_s( buffer, 256, "%d", Cond.RequiredHits() );
					SetWindowText( g_hIPEEdit, buffer );
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
			RECT r;
			GetWindowRect( g_RAMainWnd, &r );
			SetWindowPos( hDlg, NULL, r.right, r.bottom, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW );

			m_hAchievementEditorDlg = hDlg;

			HWND hList = GetDlgItem( m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS );
			SetupColumns( hList );

			CheckDlgButton( hDlg, IDC_RA_CHK_SHOW_DECIMALS, g_bPreferDecimalVal );

			//	For scanning changes to achievement conditions (hit counts)
			SetTimer( m_hAchievementEditorDlg, 1, 200, (TIMERPROC)s_AchievementEditorProc );

			m_BadgeNames.InstallAchEditorCombo( GetDlgItem( m_hAchievementEditorDlg, IDC_RA_BADGENAME ) );
			m_BadgeNames.FetchNewBadgeNamesThreaded();
		}

		bHandled = TRUE;
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
// 		case IDAPPLY:
// 			//TBD: deal with whatever 'OK' is supposed to do(?)
// 			bHandled = TRUE;
// 			break;
		case IDCLOSE:
			EndDialog(hDlg, true);
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

				char buffer[16] = { '\0' };
				HWND hBadgeNameCtrl = GetDlgItem( hDlg, IDC_RA_BADGENAME );
				switch( HIWORD( wParam ) )
				{
					case CBN_SELENDOK:
					case CBN_SELCHANGE:
						{
							GetWindowText( hBadgeNameCtrl, buffer, 16 );
							UpdateBadge( buffer );
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
						g_AchievementsDialog.OnEditAchievement( pActiveAch );

						HWND hList = GetDlgItem( g_AchievementsDialog.GetHWND(), IDC_RA_LISTACHIEVEMENTS );
						int nSelectedIndex = ListView_GetNextItem( hList, -1, LVNI_SELECTED );
						if( nSelectedIndex == -1 )
							return FALSE;

						//	Implicit updating:
						char* psTitle = g_AchievementsDialog.LbxDataAt( nSelectedIndex, (int)Dlg_Achievements::Title );
						if( GetDlgItemText( hDlg, IDC_RA_ACH_TITLE, psTitle, 80 ) )
						{
							pActiveAch->SetTitle( psTitle );

							//	Reverse find where I am in the list:
							unsigned int nOffset = 0;
							for( ; nOffset < g_pActiveAchievements->Count(); ++nOffset )
							{
								if( pActiveAch == &g_pActiveAchievements->GetAchievement( nOffset ) )
									break;
							}

							assert( nOffset < g_pActiveAchievements->Count() );
							if( nOffset < g_pActiveAchievements->Count() )
							{
								g_AchievementsDialog.OnEditData( nOffset, Dlg_Achievements::Title, pActiveAch->Title() );
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
					g_AchievementsDialog.OnEditAchievement( pActiveAch );

					HWND hList = GetDlgItem( g_AchievementsDialog.GetHWND(), IDC_RA_LISTACHIEVEMENTS );
					int nSelectedIndex = ListView_GetNextItem( hList, -1, LVNI_SELECTED );
					if( nSelectedIndex == -1 )
						return FALSE;

					//	Implicit updating:
					//char* psDesc = g_AchievementsDialog.LbxDataAt( nSelectedIndex, (int)Dlg_Achievements:: );
					char buffer[80];
					if( GetDlgItemText( hDlg, IDC_RA_ACH_DESC, buffer, 80 ) )
						pActiveAch->SetDescription( buffer );
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
					if( pActiveAch == NULL )
						return FALSE;

					char buffer[16];
					if( GetDlgItemText( hDlg, IDC_RA_ACH_POINTS, buffer, 16 ) )
					{
						int nVal = strtol( buffer, NULL, 10 );
						if( nVal < 0 || nVal > 100 )
						{
							return FALSE;
						}

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
				if( pActiveAch == NULL )
					return FALSE;

				Condition NewCondition;
				NewCondition.SetIsBasicCondition();

				NewCondition.CompSource().m_nVarSize = CMP_SZ_16BIT;
				NewCondition.CompSource().m_nVarType = CMPTYPE_ADDRESS;
				NewCondition.CompSource().m_nVal = 0x8000;

				NewCondition.ComparisonType() = CMP_EQ;

				NewCondition.CompTarget().m_nVarSize = CMP_SZ_16BIT;
				NewCondition.CompTarget().m_nVarType = CMPTYPE_VALUE;
				NewCondition.CompTarget().m_nVal = 10;

				//	Helper: guess that the currently watched memory location
				//	 is probably what they are about to want to add a cond for.
				if( g_MemoryDialog.m_hWnd != NULL )
				{
					char Str_Tmp[256];
					GetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_WATCHING, Str_Tmp, 14 );
					NewCondition.CompSource().m_nVal = strtol( Str_Tmp, NULL, 16 );
				}

				const size_t nNewID = pActiveAch->AddCondition( GetSelectedConditionGroup(), NewCondition )-1;

				//	Disable all achievement tracking:
				//g_pActiveAchievements->SetPaused( true );
				//CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );

				//	Set this achievement as 'modified'
				pActiveAch->SetModified( TRUE );
				g_AchievementsDialog.OnEditAchievement( pActiveAch );

				LoadAchievement( pActiveAch, FALSE );
				pActiveAch->ClearDirtyFlag();

				//	Select last item
				HWND hList = GetDlgItem( hDlg, IDC_RA_LBX_CONDITIONS );
				ListView_SetItemState( hList, nNewID, LVIS_FOCUSED|LVIS_SELECTED, -1 );
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
					if( nSel == -1 || nSel > (int)pActiveAch->NumConditions( GetSelectedConditionGroup() ) )
						return FALSE;


					const size_t nGrp = GetSelectedConditionGroup();
					Condition& CondToClone = pActiveAch->GetCondition( nGrp, nSel );

					Condition NewCondition;
					NewCondition.Set( CondToClone );

					const size_t nNewID = pActiveAch->AddCondition( GetSelectedConditionGroup(), NewCondition )-1;

					//	Disable all achievement tracking:
					//g_pActiveAchievements->SetPaused( true );
					//CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );

					//	Update this achievement entry as 'modified':
					pActiveAch->SetModified( TRUE );
					g_AchievementsDialog.OnEditAchievement( pActiveAch );

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
						char buffer[256];
						sprintf_s( buffer, 256, "Are you sure you wish to delete requirement ID %d?", nSel+1 );
						if( MessageBox( hDlg, buffer, "Warning", MB_YESNO ) == IDYES )
						{
							//	Delete condition nSel
							pActiveAch->RemoveCondition( GetSelectedConditionGroup(), nSel );

							//	Disable all achievement tracking:
							//g_pActiveAchievements->SetPaused( true );
							//CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );
							
							//	Set this achievement as 'modified'
							pActiveAch->SetModified( TRUE );
							g_AchievementsDialog.OnEditAchievement( pActiveAch );

							//	Refresh:
							LoadAchievement( pActiveAch, TRUE );
						}
					}
				}

			}
			break;

		case IDC_RA_UPLOAD_BADGE:
			{
				//	Pressed Upload Badge
// 				//	Special case for image:
// 				HWND hPic = GetDlgItem( hDlg, IDC_RA_CHEEVOPIC );
// 				assert( hPic );
// 				RECT rc;
// 				if( GetClientRect( hPic, &rc ) )
 				{
// 					int xPos = GET_X_LPARAM(lParam); 
// 					int yPos = GET_Y_LPARAM(lParam);

// 					//	Remap points to where image would be
// 					POINT pts[1];
// 					pts[0].x = xPos;
// 					pts[0].y = yPos;
// 					MapWindowPoints( hDlg, hPic, pts, 1 );
// 					xPos = pts[0].x;
// 					yPos = pts[0].y;
// 
// 					if( xPos > rc.left && xPos < rc.right &&
// 						yPos > rc.top && yPos < rc.bottom )
 					{
						char Name[1024];
						OPENFILENAME ofn;
						//int sys;

						//	Assume click in icon window
						//SetCurrentDirectory(Gens_Path);
						SetCurrentDirectory("\\");

						memset(Name, 0, 1024);
						memset(&ofn, 0, sizeof(OPENFILENAME));
// 
// 						Achievement* pActiveAch = ActiveAchievement();
// 						if( pActiveAch == NULL )
 							sprintf_s( Name, 1024, "" );
// 						else
// 							sprintf_s( Name, 1024, "%s.png", pActiveAch->BadgeImageFilename() );

						ofn.lStructSize = sizeof(OPENFILENAME);
						ofn.hwndOwner = hDlg;

						//ofn.hInstance = ghInstance;
						ofn.hInstance = (HINSTANCE)(GetModuleHandle(NULL)); //TBD
						
						ofn.lpstrFile = Name;
						ofn.nMaxFile = 1023;

						ofn.lpstrFilter =
							"PNG Files\0" "*.png\0"
							"GIF Files\0" "*.gif\0"
							"JPG Files\0" "*.jpg\0"
							"JPEG Files\0" "*.jpeg\0"
							"All Files\0" "*.*\0";
						//ofn.lpstrFilter = "PNG Files,*.png,JPEG Files,*.jpg,*.jpeg,GIF Files,*.gif";

						//ofn.nFilterIndex = File_Type_Index;
						ofn.nFilterIndex = 1;
						//ofn.lpstrInitialDir = "." RA_DIR_BADGE;	//	Don't suggest using this dir! Misleading!
						ofn.lpstrDefExt = "png";
						ofn.Flags = OFN_FILEMUSTEXIST;

						if (GetOpenFileName(&ofn) == NULL) return 0;

						if( ofn.lpstrFile != NULL )
						{
							DWORD nCharsRead = 0;
							char buffer[1024];
							char* pBuffer = &buffer[0];
							//DoBlockingHttpPost( "requestuploadbadge.php", "Test=true", pBuffer, 1024, nCharsRead );
							BOOL bOK = DoBlockingImageUpload( "requestuploadbadge.php", ofn.lpstrFile, pBuffer, 1024, &nCharsRead );
							if( bOK )
							{
								if( strncmp( pBuffer, "OK:", 3 ) == 0 )
								{
									//TBD: ensure that:
									//	The image is copied to the cache/badge dir
									//	The image doesn't already exist in teh cache/badge dir (ask overwrite)
									//	The image is the correct dimensions or can be scaled
									//	The image can be uploaded OK
									//	The image is not copyright

									char sNewIcon[16];
									strncpy_s( sNewIcon, pBuffer+3, 5 );
									sNewIcon[5] = '\0';
									
									//pBuffer will contain "OK:" and the number of the uploaded file.
									//	Add the value to the available values in the cbo, and it *should* self-populate.
									MessageBox( NULL, "Successful!", "Upload OK", MB_OK );

									//	Probably the only time to call this independently:
									//m_BadgeNames.FetchNewBadgeNamesThreaded();	//	NO!
									m_BadgeNames.AddNewBadgeName( sNewIcon, TRUE );

									UpdateBadge( sNewIcon );
								}
								else
								{
									char sTemp[2048];
									sprintf_s( sTemp, 2048, 
										"Error reported!\n"
										"\nServer response: %s", pBuffer );
									MessageBox( NULL, sTemp, "Error", MB_OK );
								}
							}
							else
							{
								MessageBox( NULL, "Could not contact server!", "Error", MB_OK );
							}
						}
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
				MessageBox( m_hAchievementEditorDlg, "Cannot remove Core Condition Group!", "Warning", MB_OK );
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

					char szEditText[12];
					GetWindowText( g_hIPEEdit, szEditText, 12 );
					lvDispinfo.item.pszText = szEditText;
					lvDispinfo.item.cchTextMax = lstrlen(szEditText);

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
					assert( hList != NULL );

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
						if( (unsigned int)pOnClick->iItem > ActiveAchievement()->NumConditions( GetSelectedConditionGroup() ) )
							return 0;

						Condition& rCond = ActiveAchievement()->GetCondition( GetSelectedConditionGroup(), pOnClick->iItem );

						//HWND hMem = GetDlgItem( HWndMemoryDlg, IDC_RA_WATCHING );
						char buffer[256];
						if( pOnClick->iSubItem == CSI_VALUE_SRC )
						{
							if( rCond.CompSource().m_nVarType != CMPTYPE_VALUE )
							{
								//	Wake up the mem dlg via the main app
								SendMessage( g_RAMainWnd, WM_COMMAND, IDM_RA_FILES_MEMORYFINDER, 0 );

								//	Update the text to match
								if( g_MemManager.RAMTotalSize() > 65536 )
									sprintf_s( buffer, 16, "0x%06x", rCond.CompSource().m_nVal );
								else
									sprintf_s( buffer, 16, "0x%04x", rCond.CompSource().m_nVal );

								SetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_WATCHING, buffer );
								//	Nudge the ComboBox to update the mem note
								SendMessage( g_MemoryDialog.m_hWnd, WM_COMMAND, MAKELONG( IDC_RA_WATCHING, CBN_EDITCHANGE ), 0 );
							}
						}
						else if( pOnClick->iSubItem == CSI_VALUE_TGT )
						{
							if( rCond.CompTarget().m_nVarType != CMPTYPE_VALUE )
							{
								//	Wake up the mem dlg via the main app
								SendMessage( g_RAMainWnd, WM_COMMAND, IDM_RA_FILES_MEMORYFINDER, 0 );

								//	Update the text to match
								if( g_MemManager.RAMTotalSize() > 65536 )
									sprintf_s( buffer, 16, "0x%06x", rCond.CompTarget().m_nVal );
								else
									sprintf_s( buffer, 16, "0x%04x", rCond.CompTarget().m_nVal );

								SetDlgItemText( g_MemoryDialog.m_hWnd, IDC_RA_WATCHING, buffer );
								//	Nudge the ComboBox to update the mem note
								SendMessage( g_MemoryDialog.m_hWnd, WM_COMMAND, MAKELONG( IDC_RA_WATCHING, CBN_EDITCHANGE ), 0 );
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
					if( strcmp( lvItem.pszText, sData ) != 0 )
					{
						//	Disable all achievement tracking:
						//g_pActiveAchievements->SetPaused( true );
						//CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );

						//	Update this achievement as 'modified'
						pActiveAch->SetModified( TRUE );
						g_AchievementsDialog.OnEditAchievement( pActiveAch );
					}

					//	Inject the new text into the lbx
					SendDlgItemMessage( hDlg, IDC_RA_LBX_CONDITIONS, LVM_SETITEMTEXT, (WPARAM)lvItem.iItem, (LPARAM)&lvItem ); // put new text
					
					//	Update the cached data:
					strcpy_s( sData, g_nMaxMemStringTextItemSize, pDispInfo->item.pszText );

					//	Update the achievement data:
					Condition& rCond = pActiveAch->GetCondition( GetSelectedConditionGroup(), pDispInfo->item.iItem );
					switch( pDispInfo->item.iSubItem )
					{
						case CSI_TYPE_SRC:
							{
								if( strcmp( sData, "Mem" ) == 0 )
									rCond.CompSource().m_nVarType = CMPTYPE_ADDRESS;
								else if( strcmp( sData, "Delta" ) == 0 )
									rCond.CompSource().m_nVarType = CMPTYPE_DELTAMEM;
								else
									rCond.CompSource().m_nVarType = CMPTYPE_VALUE;

								break;
							}
						case CSI_TYPE_TGT:
							{
								if( strcmp( sData, "Mem" ) == 0 )
									rCond.CompTarget().m_nVarType = CMPTYPE_ADDRESS;
								else if( strcmp( sData, "Delta" ) == 0 )
									rCond.CompTarget().m_nVarType = CMPTYPE_DELTAMEM;
								else
									rCond.CompTarget().m_nVarType = CMPTYPE_VALUE;

								break;
							}

						case CSI_SIZE_SRC:
							{
								for( int i = 0; i < g_NumMemSizeStrings; ++i )
								{
									if( strcmp( sData, g_MemSizeStrings[i] ) == 0 )
										rCond.CompSource().m_nVarSize = (CompVariableSize)i;
								}
								//	TBD: Limit validation
								break;
							}
						case CSI_SIZE_TGT:
							{
								for( int i = 0; i < g_NumMemSizeStrings; ++i )
								{
									if( strcmp( sData, g_MemSizeStrings[i] ) == 0 )
										rCond.CompTarget().m_nVarSize = (CompVariableSize)i;
								}
								//	TBD: Limit validation
								break;
							}
						case CSI_COMPARISON:
							{
								for( int i = 0; i < g_NumCompTypes; ++i )
								{
									if( strcmp( sData, g_CmpStrings[i] ) == 0 )
										rCond.ComparisonType() = (CompType)i;
								}
								break;
							}

						case CSI_VALUE_SRC:
							{
								int nBase = 16;
								if( rCond.CompSource().m_nVarType == CMPTYPE_VALUE && g_bPreferDecimalVal )
									nBase = 10;

								rCond.CompSource().m_nVal = strtol( sData, NULL, nBase );
								break;
							}
						case CSI_VALUE_TGT:
							{
								int nBase = 16;
								if( rCond.CompTarget().m_nVarType == CMPTYPE_VALUE && g_bPreferDecimalVal )
									nBase = 10;

								rCond.CompTarget().m_nVal = strtol( sData, NULL, nBase );
								break;
							}
						case CSI_HITCOUNT:
							{
								//	Always decimal
								rCond.SetRequiredHits( strtol( sData, NULL, 10 ) );
								break;
							}
						default:
							assert(0); //unhandled!
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

void Dlg_AchievementEditor::UpdateSelectedBadgeImage( const char* sBackupBadgeToUse )
{
	char sCheevoBitmapPath[256];// = DIR_BADGE "00000.png";
	sprintf_s( sCheevoBitmapPath, 256, "%s%s", RA_DIR_BADGE, LockedBadgeFile );

	if( m_pSelectedAchievement != NULL )
		sprintf_s( sCheevoBitmapPath, 256, RA_DIR_BADGE "%s.png", m_pSelectedAchievement->BadgeImageFilename() );
	//else if( sBackupBadgeToUse != NULL )
	//	sprintf_s( sCheevoBitmapPath, 256, RA_DIR_BADGE "%s.png", sBackupBadgeToUse );

	if( m_hAchievementBadge != NULL )
		DeleteObject( m_hAchievementBadge );
	m_hAchievementBadge = NULL;

	HBITMAP hBitmap = LoadLocalPNG( sCheevoBitmapPath, 64, 64 );
	if( hBitmap != NULL )
	{
		m_hAchievementBadge = hBitmap;
	}
	else
	{
		//InvalidateRect( m_hAchievementEditorDlg, NULL, TRUE );
		return;//meh
	}

	HWND hCheevo = GetDlgItem( m_hAchievementEditorDlg, IDC_RA_CHEEVOPIC );

	if( hCheevo != NULL )
	{
		SendMessage( hCheevo, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)m_hAchievementBadge );
		// 				RECT rc;
		// 				GetClientRect( hCheevo, &rc );
		// 				InvalidateRect( m_hAchievementEditorDlg, &rc, TRUE );
		InvalidateRect( m_hAchievementEditorDlg, NULL, TRUE );
	}

	char buffer[16];//= "00000";
	strcpy_s( buffer, 16, LockedBadge );

	if( m_pSelectedAchievement != NULL )
	{
		strcpy_s( buffer, 16, m_pSelectedAchievement->BadgeImageFilename() );
	}
	else if( sBackupBadgeToUse != NULL )
	{
		strcpy_s( buffer, 16, sBackupBadgeToUse );
	}

	//	Trim the '.png"
	buffer[5] = '\0';

	//	Find buffer in the dropdown list
	HWND hCtrl = GetDlgItem( m_hAchievementEditorDlg, IDC_RA_BADGENAME );
	int nSel = ComboBox_FindStringExact( hCtrl, 0, buffer );

	if( nSel != -1 )
	{
		ComboBox_SetCurSel( hCtrl, nSel );
	}
}

void Dlg_AchievementEditor::UpdateBadge( const char* sNewName )
{
	//	If a change is detected: change it!
	if( m_pSelectedAchievement != NULL )
	{
		if( strcmp( m_pSelectedAchievement->BadgeImageFilename(), sNewName ) != 0 )
		{
			//	The badge we are about to show is different from the one stored for this achievement.
			//	This implies that we are changing the badge: this achievement is modified!
			m_pSelectedAchievement->SetBadgeImage( sNewName );
			m_pSelectedAchievement->SetModified( TRUE );

			if( g_nActiveAchievementSet == AT_CORE )
			{
				int nOffs = g_AchievementsDialog.GetSelectedAchievementIndex();
				g_AchievementsDialog.OnEditData( nOffs, Dlg_Achievements::Modified, "Yes" );
			}
		}
	}

	//	Always attempt update.
	UpdateSelectedBadgeImage( sNewName );
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

		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_ID, "" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_POINTS, "" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_TITLE, "<Inactive!>" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_DESC, "<Select or Create an Achievement first!>" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR, "<Inactive!>" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_BADGENAME, "" );

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
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_ID, buffer );

		sprintf_s( buffer, 1024, "%d", m_pSelectedAchievement->Points() );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_POINTS, buffer );

		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_TITLE, m_pSelectedAchievement->Title() );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_DESC, m_pSelectedAchievement->Description() );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR, m_pSelectedAchievement->Author() );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_BADGENAME, m_pSelectedAchievement->BadgeImageFilename() );

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

		UpdateBadge( m_pSelectedAchievement->BadgeImageFilename() );

		m_bPopulatingAchievementEditorData = FALSE;
	}
	else
	{
		//	Same achievement still selected: what's changed?
		
 		HWND hTitle = GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_TITLE );
 		HWND hDesc = GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_DESC );
 		HWND hPoints = GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_POINTS );
 		HWND hFocus = GetFocus();

		BOOL bTitleSelected = (hFocus==hTitle);
		BOOL bDescSelected = (hFocus==hDesc);
		BOOL bPointsSelected = (hFocus==hPoints);

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

//  		if( hFocus == hDesc ||
//  			hFocus == hTitle ||
//  			hFocus == hPoints )
//  		{
//  			//	Don't repopulate when highlighting these fields...
// 			//	Awkwardly sends the cursor back to home if we repopulate unnecessarily!
//  			return;
//  		}
		
		//	Work out what the difference is and only update that!
		
		if( !m_pSelectedAchievement->IsDirty() )
			return;

		m_bPopulatingAchievementEditorData = TRUE;

		if( pCheevo->GetDirtyFlags() & Dirty_ID )
		{
			sprintf_s( buffer, 1024, "%d", m_pSelectedAchievement->ID() );
			SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_ID, buffer );
		}
		if( (pCheevo->GetDirtyFlags() & Dirty_Points) && !bPointsSelected )
		{
			sprintf_s( buffer, 1024, "%d", m_pSelectedAchievement->Points() );
			SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_POINTS, buffer );
		}
		if( (pCheevo->GetDirtyFlags() & Dirty_Title) && !bTitleSelected )
		{
			SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_TITLE, m_pSelectedAchievement->Title() );
		}
		if( (pCheevo->GetDirtyFlags() & Dirty_Description) && !bDescSelected )
		{
			SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_DESC, m_pSelectedAchievement->Description() );
		}
		if( pCheevo->GetDirtyFlags() & Dirty_Author )
		{
			SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR, m_pSelectedAchievement->Author() );
		}
		if( pCheevo->GetDirtyFlags() & Dirty_Badge )
		{
			SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_BADGENAME, m_pSelectedAchievement->BadgeImageFilename() );
			UpdateBadge( m_pSelectedAchievement->BadgeImageFilename() );
		}
		if( pCheevo->GetDirtyFlags() & Dirty_Conditions )
		{
			//	Getting rid of this HORRIBLE cludge of a hack
			//HWND hList = GetDlgItem( m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS );
			//unsigned int nSel = ListView_GetSelectedCount( hList );//ListView_GetSelectionMark( hList );//ListView_GetCurSel( hList );

			//if( nSel == 0 || pCheevo->GetDirtyFlags() == Dirty__All )

			PopulateConditions( m_pSelectedAchievement );
		}
		
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
	if( hList != NULL )
	{
		SetIgnoreEdits( TRUE );

		SetupColumns( hList );
		
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_ID, "" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_POINTS, "" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_TITLE, "" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_DESC, "" );
		SetDlgItemText( m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR, "" );

		m_pSelectedAchievement = NULL;

		SetIgnoreEdits( FALSE );

		LoadAchievement( NULL, FALSE );
	}
}


int Dlg_AchievementEditor::GetSelectedConditionGroup() const
{
	HWND hList = GetDlgItem( g_AchievementEditorDialog.GetHWND(), IDC_RA_ACH_GROUP );
	int nSel = ListBox_GetCurSel( hList );
	//return ListView_GetSelectionMark( hList );
	if( nSel == LB_ERR )
	{
		OutputDebugString( "ListBox_GetCurSel returning LB_ERR\n" );
		return 0;
	}

	return nSel;
}

void Dlg_AchievementEditor::SetSelectedConditionGroup( int nGrp ) const
{
	HWND hList = GetDlgItem( g_AchievementEditorDialog.GetHWND(), IDC_RA_ACH_GROUP );
	ListBox_SetCurSel( hList, nGrp );
	//ListView_SetSelectionMark( hList, nGrp );
}


void BadgeNames::FetchNewBadgeNamesThreaded()
{
	CreateHTTPRequestThread( "requestbadgenames.php", "", HTTPRequest_Get, 0, CB_OnNewBadgeNames );
}

//static
void BadgeNames::CB_OnNewBadgeNames(void* pObj)
{
	RequestObject* pRObj = (RequestObject*)(pObj);
	RequestObject& rObj = *pRObj;

	if( strncmp( rObj.m_sResponse, "OK:", 3 ) )
		return;

	//	Clean out cbo
	while( ComboBox_DeleteString( m_hDestComboBox, 0 ) > 0 )
	{
	}

	char* pIter = &rObj.m_sResponse[3];

	const size_t nLowerLimit = 80;
	const size_t nUpperLimit = atoi( pIter );

	char buffer[256];
	for( size_t i = nLowerLimit; i < nUpperLimit; ++i )
	{
		sprintf_s( buffer, 256, "%05d", i );
		ComboBox_AddString( m_hDestComboBox, buffer );
	}

	////	Assume we are in mainthread, as the thread request was made from main thread
	//char* pNextBadgeName = NULL;
	//while( ( pNextBadgeName = strtok_s( pIter, ",", &pIter ) ) != NULL )
	//{
	//	//	Only want properly formatted names
	//	if( strlen( pNextBadgeName ) != 5 )
	//		continue;

	//	for( size_t i = 0; i < 5; ++i )
	//		if( !isdigit( pNextBadgeName[i] ) )
	//			continue;

	//	ComboBox_AddString( m_hDestComboBox, pNextBadgeName );
	//}
}

void BadgeNames::AddNewBadgeName( const char* pStr, BOOL bAndSelect )
{
	int nSel = ComboBox_AddString( m_hDestComboBox, pStr );

	if( bAndSelect )
	{
		ComboBox_SelectString( m_hDestComboBox, 0, pStr );
		//ComboBox_SetCurSel( m_hDestComboBox, nSel );
	}
}
