#include "RA_Dlg_AchEditor.h"

#include "RA_AchievementSet.h"
#include "RA_Resource.h"
#include "RA_Core.h"
#include "RA_Dlg_Achievement.h"
#include "RA_Dlg_Memory.h"
#include "RA_httpthread.h"
#include "RA_ImageFactory.h"
#include "RA_MemManager.h"
#include "RA_User.h"

#ifndef _INC_COMMDLG
#include <CommDlg.h>
#endif // !_INC_COMMDLG

// TBD: These files should be included already, but some reported they were not
//      on their machines, the above is not
#ifndef _ARRAY_
#include <array>
#endif // !_ARRAY_

#ifndef _MEMORY_
#include <memory>
#endif // !_MEMORY_

namespace ra {

enum class CondSubItems{ Id, Group, TypeSrc, SizeSrc, ValueSrc, Comparison, TypeTgt, SizeTgt, ValueTgt, HitCount };

inline constexpr std::array<CondSubItems, enum_sizes::NUM_COND_SUBITEMS> aCondSubItems{
    CondSubItems::Id, CondSubItems::Group, CondSubItems::TypeSrc, CondSubItems::SizeSrc, CondSubItems::ValueSrc,
    CondSubItems::Comparison, CondSubItems::TypeTgt, CondSubItems::SizeTgt,  CondSubItems::ValueTgt,
    CondSubItems::HitCount
};

inline constexpr std::array<LPCTSTR, enum_sizes::NUM_COND_SUBITEMS> COLUMN_TITLE{
    _T("ID"), _T("Flag"), _T("Type"), _T("Size"), _T("Memory"), _T("Cmp"), _T("Type"), _T("Size"), _T("Mem/Val"),
    _T("Hits")
};

inline constexpr std::array<int, enum_sizes::NUM_COND_SUBITEMS> COLUMN_WIDTH{
    30, 75, 42, 50, 72, 35, 42, 50, 72, 72
};

} // namespace ra


Dlg_AchievementEditor g_AchievementEditorDialog;

// Dialog Resizing
std::vector<ResizeContent> vDlgAchEditorResize;
POINT pDlgAchEditorMin;

INT_PTR CALLBACK AchProgressProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg)
    {
        case WM_INITDIALOG:
            if (g_AchievementEditorDialog.ActiveAchievement() == nullptr)
                return FALSE;

            return TRUE;

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_RA_ACHPROGRESSENABLE:
                    //BOOL bEnabled = IsDlgButtonChecked( hDlg, IDC_RA_ACHPROGRESSENABLE );
                    return FALSE;

                case IDOK:
                case IDCLOSE:
                    EndDialog(hDlg, true);
                    return TRUE;

                default:
                    return FALSE;
            }
        }

        case WM_DESTROY:
            EndDialog(hDlg, true);
            return FALSE;
    }

    return FALSE;
}

Dlg_AchievementEditor::Dlg_AchievementEditor()
    : m_hAchievementEditorDlg(nullptr),
    m_hICEControl(nullptr),
    m_pSelectedAchievement(nullptr),
    m_bPopulatingAchievementEditorData(false)
{
    for (size_t i = 0; i < ra::MAX_CONDITIONS; ++i)
    {
        if (i == 0)
            _stprintf_s(m_lbxGroupNames[i], ra::MEM_STRING_TEXT_LEN, _T("Core"));
        else
            _stprintf_s(m_lbxGroupNames[i], ra::MEM_STRING_TEXT_LEN, _T("Alt %02d"), i);
    }
}

Dlg_AchievementEditor::~Dlg_AchievementEditor()
{
}

void Dlg_AchievementEditor::SetupColumns(HWND hList)
{
    //	Remove all columns,
    while (ListView_DeleteColumn(hList, 0)) {}

    //	Remove all data.
    ListView_DeleteAllItems(hList);

    auto lplvColumn{ std::make_unique<LV_COLUMN>() };

    for (auto& csi : ra::aCondSubItems)
    {
        const auto i{ ra::to_unsigned(ra::etoi(csi)) };
        lplvColumn->mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
        lplvColumn->fmt = LVCFMT_LEFT | LVCFMT_FIXED_WIDTH;
        lplvColumn->cx = ra::COLUMN_WIDTH.at(i);

        ra::tstring colTitle{ ra::COLUMN_TITLE.at(i) };
        lplvColumn->pszText = colTitle.data();
        lplvColumn->cchTextMax = 255;
        lplvColumn->iSubItem = ra::to_signed(i);

        if (i == (ra::to_signed(ra::aCondSubItems.size()) - 1))
            lplvColumn->fmt |= LVCFMT_FILL;

        ListView_InsertColumn(hList, ra::to_signed(i), lplvColumn.get());
    }

    ZeroMemory(&m_lbxData, sizeof(m_lbxData));

    m_nNumOccupiedRows = 0;

    auto bSuccess = ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);
    bSuccess = static_cast<DWORD>(ra::to_unsigned(ListView_EnableGroupView(hList, FALSE)));

    //HWND hGroupList = GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_GROUP );
    //ListBox_AddString( 
    //bSuccess = ListView_SetExtendedListViewStyle( hGroupList, LVS_EX_FULLROWSELECT );
    //bSuccess = ListView_EnableGroupView( hGroupList, FALSE );

}

BOOL Dlg_AchievementEditor::IsActive() const
{
    return(g_AchievementEditorDialog.GetHWND() != nullptr) && (IsWindowVisible(g_AchievementEditorDialog.GetHWND()));
}

const int Dlg_AchievementEditor::AddCondition(HWND hList, const Condition& Cond)
{
    auto lpItem{ std::make_unique<LV_ITEM>() };
    lpItem->mask     = LVIF_TEXT;
    lpItem->iItem    = m_nNumOccupiedRows;
    lpItem->iSubItem = 0;

    ra::tstring sData{ NativeStr(m_lbxData[m_nNumOccupiedRows][ra::etoi(ra::CondSubItems::Id)]) };
    lpItem->pszText    = sData.data();
    lpItem->cchTextMax = 256;
    lpItem->iItem      = ListView_InsertItem(hList, lpItem.get());

    UpdateCondition(hList, *lpItem, Cond);

    ASSERT(lpItem->iItem == m_nNumOccupiedRows);

    m_nNumOccupiedRows++;
    return lpItem->iItem;
}

void Dlg_AchievementEditor::UpdateCondition(HWND hList, LV_ITEM& item, const Condition& Cond)
{
    int nRow = item.iItem;

    //	Update our local array:
    const char* sMemTypStrSrc = "Value";
    const char* sMemSizeStrSrc = "";
    if (Cond.CompSource().Type() != ra::ComparisonVariableType::ValueComparison)
    {
        sMemTypStrSrc = (Cond.CompSource().Type() == ra::ComparisonVariableType::Address) ? "Mem" : "Delta";
        sMemSizeStrSrc = ra::Narrow(ra::COMPARISONVARIABLESIZE_STR.at(ra::etoi(Cond.CompSource().Size()))).c_str();
    }

    const char* sMemTypStrDst = "Value";
    const char* sMemSizeStrDst = "";
    if (Cond.CompTarget().Type() != ra::ComparisonVariableType::ValueComparison)
    {
        sMemTypStrDst = (Cond.CompTarget().Type() == ra::ComparisonVariableType::Address) ? "Mem" : "Delta";
        sMemSizeStrDst = ra::Narrow(ra::COMPARISONVARIABLESIZE_STR.at(ra::etoi(Cond.CompTarget().Size()))).c_str();
    }

    sprintf_s(m_lbxData[nRow][ra::etoi(ra::CondSubItems::Id)], ra::MEM_STRING_TEXT_LEN, "%d", nRow + 1);
    sprintf_s(m_lbxData[nRow][ra::etoi(ra::CondSubItems::Group)], ra::MEM_STRING_TEXT_LEN, "%s",
              ra::Narrow(ra::CONDITIONTYPE_STR.at(ra::etoi(Cond.GetConditionType()))).c_str());
    sprintf_s(m_lbxData[nRow][ra::etoi(ra::CondSubItems::TypeSrc)], ra::MEM_STRING_TEXT_LEN, "%s", sMemTypStrSrc);
    sprintf_s(m_lbxData[nRow][ra::etoi(ra::CondSubItems::SizeSrc)], ra::MEM_STRING_TEXT_LEN, "%s", sMemSizeStrSrc);
    sprintf_s(m_lbxData[nRow][ra::etoi(ra::CondSubItems::ValueSrc)], ra::MEM_STRING_TEXT_LEN, "0x%06x", Cond.CompSource().RawValue());
    sprintf_s(m_lbxData[nRow][ra::etoi(ra::CondSubItems::Comparison)], ra::MEM_STRING_TEXT_LEN, "%s",
              ra::Narrow(ra::COMPARISONTYPE_STR.at(ra::etoi(Cond.CompareType()))).c_str());
    sprintf_s(m_lbxData[nRow][ra::etoi(ra::CondSubItems::TypeTgt)], ra::MEM_STRING_TEXT_LEN, "%s", sMemTypStrDst);
    sprintf_s(m_lbxData[nRow][ra::etoi(ra::CondSubItems::SizeTgt)], ra::MEM_STRING_TEXT_LEN, "%s", sMemSizeStrDst);
    sprintf_s(m_lbxData[nRow][ra::etoi(ra::CondSubItems::ValueTgt)], ra::MEM_STRING_TEXT_LEN, "0x%02x", Cond.CompTarget().RawValue());
    sprintf_s(m_lbxData[nRow][ra::etoi(ra::CondSubItems::HitCount)], ra::MEM_STRING_TEXT_LEN, "%u (%u)", Cond.RequiredHits(), Cond.CurrentHits());

    if (g_bPreferDecimalVal)
    {
        if (Cond.CompTarget().Type() == ra::ComparisonVariableType::ValueComparison)
            sprintf_s(m_lbxData[nRow][ra::etoi(ra::CondSubItems::ValueTgt)], ra::MEM_STRING_TEXT_LEN, "%u", Cond.CompTarget().RawValue());
    }

    if (Cond.IsAddCondition() || Cond.IsSubCondition())
    {
        sprintf_s(m_lbxData[nRow][ra::etoi(ra::CondSubItems::Comparison)], ra::MEM_STRING_TEXT_LEN, "");
        sprintf_s(m_lbxData[nRow][ra::etoi(ra::CondSubItems::TypeTgt)], ra::MEM_STRING_TEXT_LEN, "");
        sprintf_s(m_lbxData[nRow][ra::etoi(ra::CondSubItems::SizeTgt)], ra::MEM_STRING_TEXT_LEN, "");
        sprintf_s(m_lbxData[nRow][ra::etoi(ra::CondSubItems::ValueTgt)], ra::MEM_STRING_TEXT_LEN, "");
        sprintf_s(m_lbxData[nRow][ra::etoi(ra::CondSubItems::HitCount)], ra::MEM_STRING_TEXT_LEN, "");
    }

    for (auto& csi : ra::aCondSubItems)
    {
        const auto i{ ra::etoi(csi) };
        item.iSubItem = i;

        ra::tstring sData{ NativeStr(m_lbxData[nRow][i]) };
        item.pszText = sData.data();
        ListView_SetItem(hList, &item);
    }
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
// 			//AchievementEditorDialog.SetICEControl( nullptr );
// 			//m_hICEControl = nullptr;	//	end of life
// 			bFullyHandled = FALSE;
// 			break;
// 		}
// 	case WM_KILLFOCUS:
// 		{
// 			//EndDialog( hDlg, FALSE );
// 			//g_AchievementEditorDialog.SetICEControl( nullptr );
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
// 			//g_AchievementEditorDialog.SetICEControl( nullptr );
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

long _stdcall EditProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg)
    {
        case WM_DESTROY:
            g_hIPEEdit = nullptr;
            break;

        case WM_KILLFOCUS:
        {
            LV_DISPINFO lvDispinfo;
            ZeroMemory(&lvDispinfo, sizeof(LV_DISPINFO));
            lvDispinfo.hdr.hwndFrom = hwnd;
            lvDispinfo.hdr.idFrom = GetDlgCtrlID(hwnd);
            lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
            lvDispinfo.item.mask = LVIF_TEXT;
            lvDispinfo.item.iItem = nSelItem;
            lvDispinfo.item.iSubItem = nSelSubItem;
            lvDispinfo.item.pszText = nullptr;

            TCHAR sEditText[12];
            GetWindowText(hwnd, sEditText, 12);
            lvDispinfo.item.pszText = sEditText;
            lvDispinfo.item.cchTextMax = lstrlen(sEditText);

            HWND hList = GetDlgItem(g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS);

            //	the LV ID and the LVs Parent window's HWND
            SendMessage(GetParent(hList), WM_NOTIFY, static_cast<WPARAM>(IDC_RA_LBX_CONDITIONS), reinterpret_cast<LPARAM>(&lvDispinfo));	//	##reinterpret? ##SD

            //	Extra validation?

            if (lstrcmp(sEditText, TEXT("Value")))
            {
                //	Remove the associated 'size' entry
                strcpy_s(g_AchievementEditorDialog.LbxDataAt(lvDispinfo.item.iItem, lvDispinfo.item.iSubItem + 1), 32, "");
            }

            DestroyWindow(hwnd);
            return 0;
        }

        case WM_KEYDOWN:
        {
            if (wParam == VK_RETURN || wParam == VK_ESCAPE)
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

    return CallWindowProc(EOldProc, hwnd, nMsg, wParam, lParam);
}

long _stdcall DropDownProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    if (nMsg == WM_COMMAND)
    {
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
            LV_DISPINFO lvDispinfo;
            ZeroMemory(&lvDispinfo, sizeof(LV_DISPINFO));
            lvDispinfo.hdr.hwndFrom = hwnd;
            lvDispinfo.hdr.idFrom = GetDlgCtrlID(hwnd);
            lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
            lvDispinfo.item.mask = LVIF_TEXT;
            lvDispinfo.item.iItem = nSelItem;
            lvDispinfo.item.iSubItem = nSelSubItem;
            lvDispinfo.item.pszText = nullptr;

            TCHAR sEditText[32];
            GetWindowText(hwnd, sEditText, 32);
            lvDispinfo.item.pszText = sEditText;
            lvDispinfo.item.cchTextMax = lstrlen(sEditText);

            //	the LV ID and the LVs Parent window's HWND
            HWND hList = GetDlgItem(g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS);
            SendMessage(GetParent(hList), WM_NOTIFY, static_cast<WPARAM>(IDC_RA_LBX_CONDITIONS), reinterpret_cast<LPARAM>(&lvDispinfo));

            DestroyWindow(hwnd);
        }
    }

    switch (nMsg)
    {
        case WM_DESTROY:
            g_hIPEEdit = nullptr;
            break;

        case WM_KILLFOCUS:
        {
            LV_DISPINFO lvDispinfo;
            ZeroMemory(&lvDispinfo, sizeof(LV_DISPINFO));
            lvDispinfo.hdr.hwndFrom = hwnd;
            lvDispinfo.hdr.idFrom = GetDlgCtrlID(hwnd);
            lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
            lvDispinfo.item.mask = LVIF_TEXT;
            lvDispinfo.item.iItem = nSelItem;
            lvDispinfo.item.iSubItem = nSelSubItem;
            lvDispinfo.item.pszText = nullptr;

            TCHAR sEditText[32];
            GetWindowText(hwnd, sEditText, 32);
            lvDispinfo.item.pszText = sEditText;
            lvDispinfo.item.cchTextMax = lstrlen(sEditText);

            HWND hList = GetDlgItem(g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS);

            //	the LV ID and the LVs Parent window's HWND
            SendMessage(GetParent(hList), WM_NOTIFY, (WPARAM)IDC_RA_LBX_CONDITIONS, (LPARAM)&lvDispinfo);

            //DestroyWindow(hwnd);
            DestroyWindow(hwnd);
            return 0;
        }
        break;

        case WM_KEYDOWN:
            if (wParam == VK_RETURN)
            {
                // 				LV_DISPINFO lvDispinfo;
                // 				ZeroMemory(&lvDispinfo,sizeof(LV_DISPINFO));
                // 				lvDispinfo.hdr.hwndFrom = hwnd;
                // 				lvDispinfo.hdr.idFrom = GetDlgCtrlID(hwnd);
                // 				lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
                // 				lvDispinfo.item.mask = LVIF_TEXT;
                // 				lvDispinfo.item.iItem = nSelItem;
                // 				lvDispinfo.item.iSubItem = nSelSubItem;
                // 				lvDispinfo.item.pszText = nullptr;
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
            else if (wParam == VK_ESCAPE)
            {
                //	Undo changes: i.e. simply destroy the window!
                DestroyWindow(hwnd);
            }
            else
            {
                //	Ignore keystroke, or pass it into the edit box!
            }

            break;

        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                case CBN_SELENDOK:
                case CBN_SELENDCANCEL:
                case CBN_SELCHANGE:
                case CBN_CLOSEUP:
                    _FALLTHROUGH;
                case CBN_KILLFOCUS:
                    break;
            }
    }

    return CallWindowProc(EOldProc, hwnd, nMsg, wParam, lParam);
}

_Success_(return != 0)
_NODISCARD BOOL CALLBACK CreateIPE(_In_ int nItem, _In_ ra::CondSubItems eSubItem) noexcept
{
    static_assert(std::is_same_v<std::underlying_type_t<ra::CondSubItems>, int>);
    auto bSuccess{ FALSE };
    const auto nSubItem{ ra::etoi(eSubItem) };
    const auto& hList{ ::GetDlgItem(g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS) };

    auto lprcSubItem{ std::make_unique<RECT>() };
    ListView_GetSubItemRect(hList, nItem, nSubItem, LVIR_BOUNDS, lprcSubItem.get());

    auto lprcOffset{ std::make_unique<RECT>() };

    ::GetWindowRect(hList, lprcOffset.get());

    lprcSubItem->left   += lprcOffset->left;
    lprcSubItem->right  += lprcOffset->left;
    lprcSubItem->top    += lprcOffset->top;
    lprcSubItem->bottom += lprcOffset->top;

    auto nHeight{ lprcSubItem->bottom - lprcSubItem->top };
    auto nWidth{lprcSubItem->right - lprcSubItem->left };
    if (nSubItem == 0)
    {
        nWidth /= 4; /*NOTE: the ListView has 4 columns;
                             when iSubItem == 0 (an item is clicked),
                             the width (largura) is divided by 4,
                             because for items (not subitems) the
                             width returned is that of the whole row.*/
    }


    switch (eSubItem)
    {
        case ra::CondSubItems::Id:
            ASSERT(!"First element does nothing!");	//	nothing we do if we click the first element!
            break;

        case ra::CondSubItems::Group:
        {
            //	Condition: dropdown
            ASSERT(g_hIPEEdit == nullptr);
            if (g_hIPEEdit)
                break;

            g_hIPEEdit = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                TEXT("ComboBox"),
                TEXT(""),
                WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | CBS_DROPDOWNLIST,
                lprcSubItem->left, lprcSubItem->top, nWidth, ra::ftoi(1.6f * nHeight * ra::enum_sizes::NUM_CONDITION_TYPES),
                g_AchievementEditorDialog.GetHWND(),
                nullptr,
                nullptr,
                nullptr);

            if (g_hIPEEdit == nullptr)
            {
                ASSERT(!"Could not create combo box!");
                MessageBox(nullptr, TEXT("Could not create combo box."), TEXT("Error"), MB_OK | MB_ICONERROR);
                break;
            };

            auto count{ 0 };
            for (auto& cond_name : ra::CONDITIONTYPE_STR)
            {
                ComboBox_AddString(g_hIPEEdit, cond_name);

                if (g_AchievementEditorDialog.LbxDataAt(nItem, nSubItem) == ra::Narrow(cond_name))
                    ComboBox_SetCurSel(g_hIPEEdit, count);
                count++;
            }

            SetWindowFont(g_hIPEEdit, GetStockFont(DEFAULT_GUI_FONT), TRUE);
            ComboBox_ShowDropdown(g_hIPEEdit, TRUE);
            EOldProc = SubclassWindow(g_hIPEEdit, DropDownProc);
            bSuccess = TRUE;
        }
        break;

        case ra::CondSubItems::TypeSrc:
            /*_FALLTHROUGH;*/
        case ra::CondSubItems::TypeTgt:
        {
            //	Type: dropdown
            ASSERT(g_hIPEEdit == nullptr);
            if (g_hIPEEdit)
                break;

            if (eSubItem == ra::CondSubItems::TypeTgt)
            {
                const auto nGrp{ g_AchievementEditorDialog.GetSelectedConditionGroup() };
                const auto& Cond{ g_AchievementEditorDialog.ActiveAchievement()->GetCondition(nGrp, nItem) };

                if (Cond.IsAddCondition() || Cond.IsSubCondition())
                    break;
            }

            _CONSTANT_LOC nNumItems{ 3 };	//	"Mem", "Delta" or "Value"

            g_hIPEEdit = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                TEXT("ComboBox"),
                TEXT(""),
                WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | CBS_DROPDOWNLIST,
                lprcSubItem->left, lprcSubItem->top, nWidth, ra::ftoi(1.6f * nHeight * nNumItems),
                g_AchievementEditorDialog.GetHWND(),
                nullptr,
                nullptr,
                nullptr);

            if (g_hIPEEdit == nullptr)
            {
                ASSERT(!"Could not create edit box!");
                MessageBox(nullptr, TEXT("Could not create edit box."), TEXT("Error"), MB_OK | MB_ICONERROR);
                break;
            };

            /* CB_ERRSPACE */
            ComboBox_AddString(g_hIPEEdit, NativeStr("Mem").c_str());
            ComboBox_AddString(g_hIPEEdit, NativeStr("Delta").c_str());
            ComboBox_AddString(g_hIPEEdit, NativeStr("Value").c_str());


            auto nSel{ 0 };
            if (g_AchievementEditorDialog.LbxDataAt(nItem, nSubItem) == "Mem")
                nSel = 0;
            else if (g_AchievementEditorDialog.LbxDataAt(nItem, nSubItem) == "Delta")
                nSel = 1;
            else
                nSel = 2;

            ComboBox_SetCurSel(g_hIPEEdit, nSel);

            SetWindowFont(g_hIPEEdit, GetStockFont(DEFAULT_GUI_FONT), TRUE);
            ComboBox_ShowDropdown(g_hIPEEdit, TRUE);

            EOldProc = SubclassWindow(g_hIPEEdit, DropDownProc);
            bSuccess = TRUE;
        }
        break;
        case ra::CondSubItems::SizeSrc:
            _FALLTHROUGH;
        case ra::CondSubItems::SizeTgt:
        {
            //	Size: dropdown
            ASSERT(g_hIPEEdit == nullptr);
            if (g_hIPEEdit)
                break;

            //	Note: this relies on column order :S
            if (strcmp(g_AchievementEditorDialog.LbxDataAt(nItem, nSubItem - 1), "Value") == 0)
            {
                //	Values have no size.
                break;
            }

            if (eSubItem == ra::CondSubItems::SizeTgt)
            {
                const auto nGrp{ g_AchievementEditorDialog.GetSelectedConditionGroup() };
                const auto& Cond{ g_AchievementEditorDialog.ActiveAchievement()->GetCondition(nGrp, nItem) };

                if (Cond.IsAddCondition() or Cond.IsSubCondition())
                    break;
            }

            g_hIPEEdit = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                TEXT("ComboBox"),
                TEXT(""),
                WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | CBS_DROPDOWNLIST,
                lprcSubItem->left, lprcSubItem->top, nWidth, ra::ftoi(1.6f * nHeight * ra::enum_sizes::NUM_COMPARISON_VARIABLE_SIZETYPES),
                g_AchievementEditorDialog.GetHWND(),
                nullptr,
                nullptr,
                nullptr);

            if (g_hIPEEdit == nullptr)
            {
                ASSERT(!"Could not create combo box!");
                MessageBox(nullptr, TEXT("Could not create combo box."), TEXT("Error"), MB_OK | MB_ICONERROR);
                break;
            };

            auto count{ 0 };
            for (auto& cvs_name : ra::COMPARISONVARIABLESIZE_STR)
            {
                ComboBox_AddString(g_hIPEEdit, cvs_name);

                if (g_AchievementEditorDialog.LbxDataAt(nItem, nSubItem) == ra::Narrow(cvs_name))
                    ComboBox_SetCurSel(g_hIPEEdit, count);
                count++;
            }

            SetWindowFont(g_hIPEEdit, GetStockFont(DEFAULT_GUI_FONT), TRUE);
            ComboBox_ShowDropdown(g_hIPEEdit, TRUE);
            EOldProc = SubclassWindow(g_hIPEEdit, DropDownProc);
            bSuccess = TRUE;
        }
        break;
        case ra::CondSubItems::Comparison:
        {
            //	Compare: dropdown
            ASSERT(g_hIPEEdit == nullptr);
            if (g_hIPEEdit)
                break;

            {
                const auto nGrp{ g_AchievementEditorDialog.GetSelectedConditionGroup() };
                const auto& Cond{ g_AchievementEditorDialog.ActiveAchievement()->GetCondition(nGrp, nItem) };
                if (Cond.IsAddCondition() or Cond.IsSubCondition())
                    break;
            }

            g_hIPEEdit = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                TEXT("ComboBox"),
                TEXT(""),
                WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | CBS_DROPDOWNLIST,
                lprcSubItem->left, lprcSubItem->top, nWidth, ra::ftoi(1.6f * nHeight * ra::enum_sizes::NUM_COMPARISON_TYPES),
                g_AchievementEditorDialog.GetHWND(),
                nullptr,
                nullptr,
                nullptr);

            if (g_hIPEEdit == nullptr)
            {
                ASSERT(!"Could not create combo box...");
                MessageBox(nullptr, TEXT("Could not create combo box."), TEXT("Error"), MB_OK | MB_ICONERROR);
                break;
            };

            auto count{ 0 };
            for (auto& comptype_name : ra::COMPARISONTYPE_STR)
            {
                ComboBox_AddString(g_hIPEEdit, comptype_name);

                if (g_AchievementEditorDialog.LbxDataAt(nItem, nSubItem) == ra::Narrow(comptype_name))
                    ComboBox_SetCurSel(g_hIPEEdit, count);
                count++;
            }

            SetWindowFont(g_hIPEEdit, GetStockFont(DEFAULT_GUI_FONT), TRUE);
            ComboBox_ShowDropdown(g_hIPEEdit, TRUE);
            EOldProc = SubclassWindow(g_hIPEEdit, DropDownProc);
            bSuccess = TRUE;
        }
        break;
        case ra::CondSubItems::HitCount:
        case ra::CondSubItems::ValueSrc: // Mem/Val: edit
            _FALLTHROUGH;
        case ra::CondSubItems::ValueTgt: // Mem/Val: edit
        {
            ASSERT(g_hIPEEdit == nullptr);
            if (g_hIPEEdit)
                break;

            if (eSubItem != ra::CondSubItems::ValueSrc)
            {
                const auto nGrp{ g_AchievementEditorDialog.GetSelectedConditionGroup() };
                const auto& Cond{ g_AchievementEditorDialog.ActiveAchievement()->GetCondition(nGrp, nItem) };

                if (Cond.IsAddCondition() || Cond.IsSubCondition())
                    break;
            }

            g_hIPEEdit = CreateWindowEx(
                WS_EX_CLIENTEDGE,
                TEXT("EDIT"),
                TEXT(""),
                WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | ES_WANTRETURN,
                lprcSubItem->left, lprcSubItem->top, nWidth, ra::ftoi(1.5f*nHeight),
                g_AchievementEditorDialog.GetHWND(),
                nullptr,
                nullptr,
                nullptr);

            if (g_hIPEEdit == nullptr)
            {
                ASSERT(!"Could not create edit box!");
                MessageBox(nullptr, TEXT("Could not create edit box."), TEXT("Error"), MB_OK | MB_ICONERROR);
                break;
            };

            SetWindowFont(g_hIPEEdit, GetStockFont(DEFAULT_GUI_FONT), TRUE);

            {
                const std::string& sData{ g_AchievementEditorDialog.LbxDataAt(nItem, nSubItem) };
                SetWindowText(g_hIPEEdit, NativeStr(sData).c_str());
            }

            //	Special case, hitcounts
            if (eSubItem == ra::CondSubItems::HitCount)
            {
                if (g_AchievementEditorDialog.ActiveAchievement() != nullptr)
                {
                    const auto nGrp{ g_AchievementEditorDialog.GetSelectedConditionGroup() };
                    const auto& Cond{ g_AchievementEditorDialog.ActiveAchievement()->GetCondition(nGrp, nItem) };

                    auto buffer{ std::make_unique<TCHAR[]>(256) };
                    _stprintf_s(buffer.get(), 256, _T("%u"), Cond.RequiredHits());
                    SetWindowText(g_hIPEEdit, buffer.get());
                }
            }

            Edit_SetSel(g_hIPEEdit, 0, -1);
            ::SetFocus(g_hIPEEdit);
            EOldProc = SubclassWindow(g_hIPEEdit, EditProc);

            bSuccess = TRUE;
        }

        break;
    }

    return bSuccess;
}

// static
LRESULT CALLBACK Dlg_AchievementEditor::ListViewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_MOUSEMOVE:
            LVHITTESTINFO lvHitTestInfo;
            GetCursorPos(&lvHitTestInfo.pt);
            ScreenToClient(hWnd, &lvHitTestInfo.pt);

            int nTooltipLocation = -1;
            if (ListView_SubItemHitTest(hWnd, &lvHitTestInfo) != -1)
                nTooltipLocation = lvHitTestInfo.iItem * 256 + lvHitTestInfo.iSubItem;

            // if the mouse has moved to a new grid cell, hide the toolip
            if (nTooltipLocation != g_AchievementEditorDialog.m_nTooltipLocation)
            {
                g_AchievementEditorDialog.m_nTooltipLocation = nTooltipLocation;

                if (IsWindowVisible(g_AchievementEditorDialog.m_hTooltip))
                {
                    // tooltip is visible, just hide it
                    SendMessage(g_AchievementEditorDialog.m_hTooltip, TTM_POP, 0, 0);
                }
                else
                {
                    // tooltip is not visible. if we told the tooltip to not display in the TTM_GETDISPINFO handler by
                    // setting lpszText to empty string, it won't try to display again unless we tell it to, so do so now
                    SendMessage(g_AchievementEditorDialog.m_hTooltip, TTM_POPUP, 0, 0);
                    // but we don't want the tooltip to immediately display when entering a new cell, so immediately
                    // hide it. the normal hover behavior will make it display as expected after a normal hover wait time.
                    SendMessage(g_AchievementEditorDialog.m_hTooltip, TTM_POP, 0, 0);
                }
            }
            break;
    }

    return CallWindowProc(g_AchievementEditorDialog.m_pListViewWndProc, hWnd, uMsg, wParam, lParam);
}

//static
INT_PTR CALLBACK Dlg_AchievementEditor::s_AchievementEditorProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //	Intercept some msgs?
    return g_AchievementEditorDialog.AchievementEditorProc(hDlg, uMsg, wParam, lParam);
}

INT_PTR Dlg_AchievementEditor::AchievementEditorProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bHandled = TRUE;

    switch (uMsg)
    {
        case WM_TIMER:
        {
            //	Ignore if we are currently editing a box!
            if (g_hIPEEdit != nullptr)
                break;

            Achievement* pActiveAch = ActiveAchievement();
            if (pActiveAch == nullptr)
                break;

            if (pActiveAch->IsDirty())
            {
                LoadAchievement(pActiveAch, TRUE);
                pActiveAch->ClearDirtyFlag();
            }
        }
        break;

        case WM_INITDIALOG:
        {
            m_hAchievementEditorDlg = hDlg;
            GenerateResizes(hDlg);

            HWND hList = GetDlgItem(m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS);
            SetupColumns(hList);
            CheckDlgButton(hDlg, IDC_RA_CHK_SHOW_DECIMALS, g_bPreferDecimalVal);

            //	For scanning changes to achievement conditions (hit counts)
            SetTimer(m_hAchievementEditorDlg, 1, 200, (TIMERPROC)s_AchievementEditorProc);

            m_BadgeNames.InstallAchEditorCombo(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_BADGENAME));
            m_BadgeNames.FetchNewBadgeNamesThreaded();

            // achievement loaded before UI was created won't have populated the UI. reload it.
            if (m_pSelectedAchievement != nullptr)
            {
                auto* pAchievement = m_pSelectedAchievement;
                m_pSelectedAchievement = nullptr;
                LoadAchievement(pAchievement, false);
            }

            if (!CanCausePause())
            {
                ShowWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_CHK_ACH_PAUSE_ON_TRIGGER), SW_HIDE);
                ShowWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_CHK_ACH_PAUSE_ON_RESET), SW_HIDE);
            }

            // set up the list view tooltip
            m_hTooltip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr, WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
                hDlg, nullptr, GetInstanceModule(g_hThisDLLInst), nullptr);
            if (m_hTooltip)
            {
                TOOLINFO toolInfo;
                memset(&toolInfo, 0, sizeof(toolInfo));
                toolInfo.cbSize = TTTOOLINFO_V1_SIZE;
                toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
                toolInfo.hwnd = hDlg;
                toolInfo.uId = reinterpret_cast<UINT_PTR>(hList);
                toolInfo.lpszText = LPSTR_TEXTCALLBACK;
                SendMessage(m_hTooltip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
                SendMessage(m_hTooltip, TTM_ACTIVATE, TRUE, 0);
                SendMessage(m_hTooltip, TTM_SETMAXTIPWIDTH, 0, 320);
                SendMessage(m_hTooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000); // show for 30 seconds

                // install a mouse hook so we can show different tooltips per cell within the list view
                m_pListViewWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hList, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(&ListViewWndProc)));
            }

            RestoreWindowPosition(hDlg, "Achievement Editor", true, true);
        }
        return TRUE;

        case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
            lpmmi->ptMinTrackSize = pDlgAchEditorMin;
        }
        break;

        case WM_SIZE:
        {
            RARect winRect;
            GetWindowRect(hDlg, &winRect);

            for (ResizeContent content : vDlgAchEditorResize)
                content.Resize(winRect.Width(), winRect.Height());

            InvalidateRect(hDlg, nullptr, TRUE);

            RememberWindowSize(hDlg, "Achievement Editor");
        }
        break;

        case WM_MOVE:
            RememberWindowPosition(hDlg, "Achievement Editor");
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDCLOSE:
                    EndDialog(hDlg, true);
                    bHandled = TRUE;
                    break;

                case IDC_RA_CHK_SHOW_DECIMALS:
                {
                    g_bPreferDecimalVal = !g_bPreferDecimalVal;
                    if (ActiveAchievement() != nullptr)
                    {
                        ActiveAchievement()->SetDirtyFlag(ra::Achievement_DirtyFlags::All);
                        LoadAchievement(ActiveAchievement(), TRUE);
                        ActiveAchievement()->ClearDirtyFlag();
                    }
                    bHandled = TRUE;
                    break;
                }

                case IDC_RA_CHK_ACH_ACTIVE:
                    if (ActiveAchievement() != nullptr)
                    {
                        // TODO: Use the literals once PR 23 is accepted
                        SendMessage(g_AchievementsDialog.GetHWND(), WM_COMMAND, IDC_RA_RESET_ACH, LPARAM{});
                        CheckDlgButton(hDlg, IDC_RA_CHK_ACH_ACTIVE, ActiveAchievement()->Active());
                    }
                    bHandled = TRUE;
                    break;

                case IDC_RA_CHK_ACH_PAUSE_ON_TRIGGER:
                {
                    if (ActiveAchievement() != nullptr)
                        ActiveAchievement()->SetPauseOnTrigger(!ActiveAchievement()->GetPauseOnTrigger());

                    bHandled = TRUE;
                }
                break;

                case IDC_RA_CHK_ACH_PAUSE_ON_RESET:
                {
                    if (ActiveAchievement() != nullptr)
                        ActiveAchievement()->SetPauseOnReset(!ActiveAchievement()->GetPauseOnReset());

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
                    // 				if( m_pSelectedAchievement == nullptr )
                    // 				{
                    // 					bHandled = TRUE;
                    // 					break;
                    // 				}

                    HWND hBadgeNameCtrl = GetDlgItem(hDlg, IDC_RA_BADGENAME);
                    switch (HIWORD(wParam))
                    {
                        case CBN_SELENDOK:
                        case CBN_SELCHANGE:
                        {
                            TCHAR buffer[16];
                            GetWindowText(hBadgeNameCtrl, buffer, 16);
                            UpdateBadge(ra::Narrow(buffer));
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
                    DialogBox((HINSTANCE)(GetModuleHandle(nullptr)), MAKEINTRESOURCE(IDD_RA_ACHIEVEMENTPROGRESS), hDlg, AchProgressProc);
                    //DialogBox( ghInstance, MAKEINTRESOURCE(IDD_RA_ACHIEVEMENTPROGRESS), hDlg, AchProgressProc );
                    bHandled = TRUE;
                }
                break;

                case IDC_RA_ACH_TITLE:
                {
                    switch (HIWORD(wParam))
                    {
                        case EN_CHANGE:
                        {
                            if (IsPopulatingAchievementEditorData())
                                return TRUE;

                            Achievement* pActiveAch = ActiveAchievement();
                            if (pActiveAch == nullptr)
                                return FALSE;

                            //	Disable all achievement tracking
                            //g_pActiveAchievements->SetPaused( true );
                            //CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );

                            //	Set this achievement as modified
                            pActiveAch->SetModified(TRUE);
                            g_AchievementsDialog.OnEditAchievement(*pActiveAch);

                            HWND hList = GetDlgItem(g_AchievementsDialog.GetHWND(), IDC_RA_LISTACHIEVEMENTS);
                            int nSelectedIndex = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                            if (nSelectedIndex != -1)
                            {
                                //	Implicit updating:
                                TCHAR buffer[1024];
                                if (GetDlgItemText(hDlg, IDC_RA_ACH_TITLE, buffer, 1024))
                                {
                                    pActiveAch->SetTitle(ra::Narrow(buffer));

                                    //	Persist/Update/Inject local LBX data back into LBX (?)
                                    g_AchievementsDialog.OnEditData(g_pActiveAchievements->GetAchievementIndex(*pActiveAch), ra::DlgAchievementColumn::Title, pActiveAch->Title());
                                }
                            }
                        }
                        break;
                    }
                }
                break;
                case IDC_RA_ACH_DESC:
                    switch (HIWORD(wParam))
                    {
                        case EN_CHANGE:
                        {
                            if (IsPopulatingAchievementEditorData())
                                return TRUE;

                            Achievement* pActiveAch = ActiveAchievement();
                            if (pActiveAch == nullptr)
                                return FALSE;

                            //	Disable all achievement tracking:
                            //g_pActiveAchievements->SetPaused( true );
                            //CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );

                            //	Set this achievement as modified:
                            pActiveAch->SetModified(TRUE);
                            g_AchievementsDialog.OnEditAchievement(*pActiveAch);

                            HWND hList = GetDlgItem(g_AchievementsDialog.GetHWND(), IDC_RA_LISTACHIEVEMENTS);
                            int nSelectedIndex = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                            if (nSelectedIndex == -1)
                                return FALSE;

                            //	Implicit updating:
                            //char* psDesc = g_AchievementsDialog.LbxDataAt( nSelectedIndex, (int)Dlg_Achievements:: );
                            TCHAR buffer[128];
                            if (GetDlgItemText(hDlg, IDC_RA_ACH_DESC, buffer, 128))
                                pActiveAch->SetDescription(ra::Narrow(buffer));
                        }
                        break;
                    }
                    break;

                case IDC_RA_ACH_POINTS:
                    switch (HIWORD(wParam))
                    {
                        case EN_CHANGE:
                        {
                            if (IsPopulatingAchievementEditorData())
                                return TRUE;

                            Achievement* pActiveAch = ActiveAchievement();
                            if (pActiveAch == nullptr)
                                return FALSE;

                            TCHAR buffer[16];
                            if (GetDlgItemText(hDlg, IDC_RA_ACH_POINTS, buffer, 16))
                            {
                                
                                int nVal = std::stoi(NativeStr(buffer));
                                if (nVal < 0 || nVal > 100)
                                    return FALSE;

                                pActiveAch->SetPoints(nVal);
                                // achievement is now dirty
                                pActiveAch->SetModified(TRUE);
                                g_AchievementsDialog.OnEditAchievement(*pActiveAch);
                                return TRUE;
                            }
                        }
                        break;
                    }
                    break;

                case IDC_RA_ADDCOND:
                {
                    Achievement* pActiveAch = ActiveAchievement();
                    if (pActiveAch == nullptr)
                        return FALSE;

                    Condition NewCondition;
                    NewCondition.CompSource().Set(ra::ComparisonVariableSize::EightBit, ra::ComparisonVariableType::Address, 0x0000);
                    NewCondition.CompTarget().Set(ra::ComparisonVariableSize::EightBit, ra::ComparisonVariableType::ValueComparison, 0);	//	Compare defaults!

                    //	Helper: guess that the currently watched memory location
                    //	 is probably what they are about to want to add a cond for.
                    if (g_MemoryDialog.GetHWND() != nullptr)
                    {
                        TCHAR buffer[256];
                        GetDlgItemText(g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, buffer, 256);
                        unsigned int nVal = strtoul(ra::Narrow(buffer).c_str(), nullptr, 16);
                        NewCondition.CompSource().SetValues(nVal, nVal);
                    }

                    const size_t nNewID = pActiveAch->AddCondition(GetSelectedConditionGroup(), NewCondition) - 1;

                    //	Disable all achievement tracking:
                    //g_pActiveAchievements->SetPaused( true );
                    //CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );

                    //	Set this achievement as 'modified'
                    pActiveAch->SetModified(TRUE);
                    g_AchievementsDialog.OnEditAchievement(*pActiveAch);

                    LoadAchievement(pActiveAch, FALSE);
                    pActiveAch->ClearDirtyFlag();

                    //	Select last item
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LBX_CONDITIONS);
                    ListView_SetItemState(hList, nNewID, LVIS_FOCUSED | LVIS_SELECTED, -1);
                    ListView_EnsureVisible(hList, nNewID, FALSE);
                }
                break;

                case IDC_RA_COPYCOND:
                {
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LBX_CONDITIONS);
                    Achievement* pActiveAch = ActiveAchievement();
                    if (pActiveAch != nullptr)
                    {
                        unsigned int uSelectedCount = ListView_GetSelectedCount(hList);

                        if (uSelectedCount > 0)
                        {
                            m_ConditionClipboard.Clear();

                            for (int i = ListView_GetNextItem(hList, -1, LVNI_SELECTED); i >= 0; i = ListView_GetNextItem(hList, i, LVNI_SELECTED))
                            {
                                const Condition& CondToCopy = pActiveAch->GetCondition(GetSelectedConditionGroup(), static_cast<size_t>(i));

                                Condition NewCondition(CondToCopy);

                                m_ConditionClipboard.Add(NewCondition);
                            }

                            m_ConditionClipboard.Count();
                        }
                        else
                            return FALSE;
                    }
                }
                break;

                case IDC_RA_PASTECOND:
                {
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LBX_CONDITIONS);
                    Achievement* pActiveAch = ActiveAchievement();

                    if (pActiveAch != nullptr)
                    {
                        if (m_ConditionClipboard.Count() > 0)
                        {
                            for (unsigned int i = 0; i < m_ConditionClipboard.Count(); i++)
                            {
                                const size_t nNewID = pActiveAch->AddCondition(GetSelectedConditionGroup(), m_ConditionClipboard.GetAt(i)) - 1;
                                ListView_SetItemState(hList, nNewID, LVIS_FOCUSED | LVIS_SELECTED, -1);
                                ListView_EnsureVisible(hList, nNewID, FALSE);
                            }

                            //	Update this achievement entry as 'modified':
                            pActiveAch->SetModified(TRUE);
                            g_AchievementsDialog.OnEditAchievement(*pActiveAch);

                            LoadAchievement(pActiveAch, FALSE);
                            pActiveAch->ClearDirtyFlag();
                        }
                        else
                            return FALSE;
                    }
                }
                break;

                case IDC_RA_DELETECOND:
                {
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LBX_CONDITIONS);
                    int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

                    if (nSel != -1)
                    {
                        Achievement* pActiveAch = ActiveAchievement();
                        if (pActiveAch != nullptr)
                        {
                            unsigned int uSelectedCount = ListView_GetSelectedCount(hList);

                            char buffer[256];
                            sprintf_s(buffer, 256, "Are you sure you wish to delete %u condition(s)?", uSelectedCount);
                            if (MessageBox(hDlg, NativeStr(buffer).c_str(), TEXT("Warning"), MB_YESNO) == IDYES)
                            {
                                nSel = -1;
                                std::vector<int> items;

                                while ((nSel = ListView_GetNextItem(hList, nSel, LVNI_SELECTED)) != -1)
                                    items.push_back(nSel);

                                while (!items.empty())
                                {
                                    nSel = items.back();
                                    pActiveAch->RemoveCondition(GetSelectedConditionGroup(), nSel);

                                    items.pop_back();
                                }

                                //	Set this achievement as 'modified'
                                pActiveAch->SetModified(TRUE);
                                g_AchievementsDialog.OnEditAchievement(*pActiveAch);

                                //	Refresh:
                                LoadAchievement(pActiveAch, TRUE);
                            }
                        }
                    }

                }
                break;

                case IDC_RA_MOVECONDUP:
                {
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LBX_CONDITIONS);
                    Achievement* pActiveAch = ActiveAchievement();
                    if (pActiveAch != nullptr)
                    {
                        int nSelectedIndex = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                        if (nSelectedIndex >= 0)
                        {
                            //  Get conditions to move
                            std::vector<Condition> conditionsToMove;
                            size_t nSelectedConditionGroup = GetSelectedConditionGroup();

                            for (int i = nSelectedIndex; i >= 0; i = ListView_GetNextItem(hList, i, LVNI_SELECTED))
                            {
                                // as we remove items, the index within the achievement changes, but not in the UI until we refresh
                                size_t nUpdatedIndex = static_cast<size_t>(i) - conditionsToMove.size();

                                const Condition& CondToMove = pActiveAch->GetCondition(nSelectedConditionGroup, static_cast<size_t>(nUpdatedIndex));
                                conditionsToMove.push_back(std::move(CondToMove));
                                pActiveAch->RemoveCondition(nSelectedConditionGroup, static_cast<size_t>(nUpdatedIndex));
                            }

                            //  Insert at new location
                            int nInsertIndex = (nSelectedIndex > 0) ? nSelectedIndex - 1 : 0;
                            size_t nInsertCount = conditionsToMove.size();

                            for (size_t i = 0; i < nInsertCount; ++i)
                            {
                                const Condition& CondToMove = conditionsToMove[i];
                                pActiveAch->InsertCondition(nSelectedConditionGroup, nInsertIndex + i, CondToMove);
                            }

                            //	Set this achievement as 'modified'
                            pActiveAch->SetModified(TRUE);
                            g_AchievementsDialog.OnEditAchievement(*pActiveAch);

                            //	Refresh:
                            LoadAchievement(pActiveAch, TRUE);

                            //  Update selections
                            ListView_SetItemState(hList, -1, LVIF_STATE, LVIS_SELECTED);
                            for (size_t i = 0; i < nInsertCount; ++i)
                                ListView_SetItemState(hList, nInsertIndex + i, LVIS_FOCUSED | LVIS_SELECTED, -1);

                            ListView_EnsureVisible(hList, nInsertIndex, FALSE);
                        }

                        SetFocus(hList);
                        bHandled = TRUE;
                    }
                }
                break;

                case IDC_RA_MOVECONDDOWN:
                {
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LBX_CONDITIONS);
                    Achievement* pActiveAch = ActiveAchievement();
                    if (pActiveAch != nullptr)
                    {
                        int nSelectedIndex = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                        if (nSelectedIndex >= 0)
                        {
                            //  Get conditions to move
                            std::vector<Condition> conditionsToMove;
                            size_t nSelectedConditionGroup = GetSelectedConditionGroup();

                            for (int i = nSelectedIndex; i >= 0; i = ListView_GetNextItem(hList, i, LVNI_SELECTED))
                            {
                                // as we remove items, the index within the achievement changes, but not in the UI until we refresh
                                size_t nUpdatedIndex = static_cast<size_t>(i) - conditionsToMove.size();

                                const Condition& CondToMove = pActiveAch->GetCondition(nSelectedConditionGroup, static_cast<size_t>(nUpdatedIndex));
                                conditionsToMove.push_back(std::move(CondToMove));
                                pActiveAch->RemoveCondition(nSelectedConditionGroup, static_cast<size_t>(nUpdatedIndex));

                                // want to insert after last selected item, update nSelectedIndex
                                nSelectedIndex = nUpdatedIndex;
                            }

                            //  Insert at new location
                            int nConditionCount = pActiveAch->NumConditions(nSelectedConditionGroup);
                            int nInsertIndex = (nSelectedIndex < nConditionCount) ? nSelectedIndex + 1 : nConditionCount;
                            size_t nInsertCount = conditionsToMove.size();

                            for (size_t i = 0; i < nInsertCount; ++i)
                            {
                                const Condition& CondToMove = conditionsToMove[i];
                                pActiveAch->InsertCondition(nSelectedConditionGroup, nInsertIndex + i, CondToMove);
                            }

                            //	Set this achievement as 'modified'
                            pActiveAch->SetModified(TRUE);
                            g_AchievementsDialog.OnEditAchievement(*pActiveAch);

                            //	Refresh:
                            LoadAchievement(pActiveAch, TRUE);

                            //  Update selections
                            ListView_SetItemState(hList, -1, LVIF_STATE, LVIS_SELECTED);
                            for (size_t i = 0; i < nInsertCount; ++i)
                                ListView_SetItemState(hList, nInsertIndex + i, LVIS_FOCUSED | LVIS_SELECTED, -1);

                            ListView_EnsureVisible(hList, nInsertIndex, FALSE);
                        }

                        SetFocus(hList);
                        bHandled = TRUE;
                    }
                }
                break;

                case IDC_RA_UPLOAD_BADGE:
                {
                    const int BUF_SIZE = 1024;
                    TCHAR buffer[BUF_SIZE];
                    ZeroMemory(buffer, BUF_SIZE);

                    OPENFILENAME ofn;
                    ZeroMemory(&ofn, sizeof(OPENFILENAME));
                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.hwndOwner = hDlg;
                    ofn.hInstance = static_cast<HINSTANCE>(GetModuleHandle(nullptr));
                    ofn.lpstrFile = buffer;
                    ofn.nMaxFile = BUF_SIZE - 1;

                    ofn.lpstrFilter =
                        TEXT("Image Files\0" "*.png;*.gif;*.jpg;*.jpeg\0")
                        TEXT("All Files\0" "*.*\0\0");	//	Last element MUST be doubly-terminated: https://msdn.microsoft.com/en-us/library/windows/desktop/ms646839(v=vs.85).aspx

                    ofn.nFilterIndex = 1;
                    ofn.lpstrDefExt = TEXT("png");
                    ofn.Flags = OFN_FILEMUSTEXIST;

                    if (GetOpenFileName(&ofn) == 0)	//	0 == cancelled
                        return FALSE;

                    if (ofn.lpstrFile != nullptr)
                    {
                        Document Response;
                        if (RAWeb::DoBlockingImageUpload(ra::UploadType::RequestUploadBadgeImage, ra::Narrow( ofn.lpstrFile), Response))
                        {
                            //TBD: ensure that:
                            //	The image is copied to the cache/badge dir
                            //	The image doesn't already exist in teh cache/badge dir (ask overwrite)
                            //	The image is the correct dimensions or can be scaled
                            //	The image can be uploaded OK
                            //	The image is not copyright

                            const Value& ResponseData = Response["Response"];
                            if (ResponseData.HasMember("BadgeIter"))
                            {
                                const char* sNewBadgeIter = ResponseData["BadgeIter"].GetString();

                                //pBuffer will contain "OK:" and the number of the uploaded file.
                                //	Add the value to the available values in the cbo, and it *should* self-populate.
                                MessageBox(nullptr, TEXT("Successful!"), TEXT("Upload OK"), MB_OK);

                                m_BadgeNames.AddNewBadgeName(sNewBadgeIter, true);
                                UpdateBadge(sNewBadgeIter);
                            }
                            else if (ResponseData.HasMember("Error"))
                            {
                                MessageBox(nullptr, NativeStr(ResponseData["Error"].GetString()).c_str(), TEXT("Error"), MB_OK);
                            }
                            else
                            {
                                MessageBox(nullptr, TEXT("Error in upload! Try another image format, or maybe smaller image?"), TEXT("Error"), MB_OK);
                            }
                        }
                        else
                        {
                            MessageBox(nullptr, TEXT("Could not contact server!"), TEXT("Error"), MB_OK);
                        }
                    }
                }
                break;

                case IDC_RA_ACH_ADDGROUP:
                {
                    if (ActiveAchievement() == nullptr)
                        break;

                    if (ActiveAchievement()->NumConditionGroups() == 1)
                    {
                        ActiveAchievement()->AddConditionGroup();
                        ActiveAchievement()->AddConditionGroup();
                    }
                    else if (ActiveAchievement()->NumConditionGroups() > 1)
                    {
                        ActiveAchievement()->AddConditionGroup();
                    }
                    else
                    {
                        //?
                    }

                    RepopulateGroupList(ActiveAchievement());
                }
                break;
                case IDC_RA_ACH_DELGROUP:
                {
                    if (ActiveAchievement() == nullptr)
                        break;

                    if (ActiveAchievement()->NumConditionGroups() == 3)
                    {
                        ActiveAchievement()->RemoveConditionGroup();
                        ActiveAchievement()->RemoveConditionGroup();	//	##SD this intentional?
                    }
                    else if (ActiveAchievement()->NumConditionGroups() > 3)
                    {
                        ActiveAchievement()->RemoveConditionGroup();
                    }
                    else
                    {
                        MessageBox(m_hAchievementEditorDlg, TEXT("Cannot remove Core Condition Group!"), TEXT("Warning"), MB_OK);
                    }

                    RepopulateGroupList(ActiveAchievement());
                }
                break;
                case IDC_RA_ACH_GROUP:
                    switch (HIWORD(wParam))
                    {
                        case LBN_SELCHANGE:
                            PopulateConditions(ActiveAchievement());
                            break;
                    }
                    break;
            }

            //	Switch also on the highword:

            switch (HIWORD(wParam))
            {
                case CBN_SELENDOK:
                case CBN_SELENDCANCEL:
                case CBN_SELCHANGE:
                case CBN_CLOSEUP:
                    _FALLTHROUGH;
                case CBN_KILLFOCUS:
                {
                    //	Only post this if the edit control is not empty:
                    //	 if it's empty, we're not dealing with THIS edit control!
                    if (g_hIPEEdit != nullptr)
                    {
                        LV_DISPINFO lvDispinfo;
                        ZeroMemory(&lvDispinfo, sizeof(LV_DISPINFO));
                        lvDispinfo.hdr.hwndFrom = g_hIPEEdit;
                        lvDispinfo.hdr.idFrom = GetDlgCtrlID(g_hIPEEdit);
                        lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
                        lvDispinfo.item.mask = LVIF_TEXT;
                        lvDispinfo.item.iItem = nSelItem;
                        lvDispinfo.item.iSubItem = nSelSubItem;
                        lvDispinfo.item.pszText = nullptr;

                        TCHAR sEditText[32];
                        GetWindowText(g_hIPEEdit, sEditText, 32);
                        lvDispinfo.item.pszText = sEditText;
                        lvDispinfo.item.cchTextMax = lstrlen(sEditText);

                        HWND hList = GetDlgItem(GetHWND(), IDC_RA_LBX_CONDITIONS);

                        //	the LV ID and the LVs Parent window's HWND
                        SendMessage(GetParent(hList), WM_NOTIFY, (WPARAM)IDC_RA_LBX_CONDITIONS, (LPARAM)&lvDispinfo);

                        DestroyWindow(g_hIPEEdit);
                    }
                }
                break;
            }
            break;

        case WM_NOTIFY:
        {
            switch ((((LPNMHDR)lParam)->code))
            {
                case NM_CLICK:
                {
                    LPNMITEMACTIVATE pOnClick = (LPNMITEMACTIVATE)lParam;

                    //	http://cboard.cprogramming.com/windows-programming/122733-%5Bc%5D-editing-subitems-listview-win32-api.html

                    HWND hList = GetDlgItem(m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS);
                    ASSERT(hList != nullptr);

                    //	Note: first item should be an ID!
                    if (pOnClick->iItem != -1 && pOnClick->iSubItem != 0)
                    {
                        nSelItem = pOnClick->iItem;
                        nSelSubItem = pOnClick->iSubItem;
                        
                        if (auto csi{ ra::aCondSubItems.at(pOnClick->iSubItem) }; CreateIPE(pOnClick->iItem, csi) == 0)
                            return 0;
                    }
                    else
                    {
                        nSelItem = -1;
                        nSelSubItem = -1;
                        /*If SubItemHitTest does return error (lResult=-1),
                        it kills focus of hEdit in order to destroy it.*/
                        FORWARD_WM_KILLFOCUS(g_hIPEEdit, nullptr, SendMessage);
                    }
                    return 0;

                }
                break;
                case NM_RCLICK:
                {
                    LPNMITEMACTIVATE pOnClick = (LPNMITEMACTIVATE)lParam;
                    if (pOnClick->iItem != -1 &&
                        pOnClick->iSubItem != -1)
                    {
                        if (static_cast<size_t>(pOnClick->iItem) > ActiveAchievement()->NumConditions(GetSelectedConditionGroup()))
                            return 0;

                        Condition& rCond = ActiveAchievement()->GetCondition(GetSelectedConditionGroup(), pOnClick->iItem);

                        using namespace ra::rel_ops;
                        //HWND hMem = GetDlgItem( HWndMemoryDlg, IDC_RA_WATCHING );
                        if (pOnClick->iSubItem == ra::CondSubItems::ValueSrc)
                        {
                            if (rCond.CompSource().Type() != ra::ComparisonVariableType::ValueComparison)
                            {
                                //	Wake up the mem dlg via the main app
                                FORWARD_WM_COMMAND(g_RAMainWnd, IDM_RA_FILES_MEMORYFINDER, nullptr, 0U, SendMessage);

                                //	Update the text to match
                                auto buffer{ std::make_unique<TCHAR[]>(16) };
                                ::_stprintf_s(buffer.get(), 16, _T("0x%06x"), rCond.CompSource().RawValue());
                                SetDlgItemText(g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, buffer.get());

                                //	Nudge the ComboBox to update the mem note
                                FORWARD_WM_COMMAND(g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, nullptr,
                                                   CBN_EDITCHANGE, SendMessage);
                            }
                        }
                        else if (pOnClick->iSubItem == ra::CondSubItems::ValueTgt)
                        {
                            if (rCond.CompTarget().Type() != ra::ComparisonVariableType::ValueComparison)
                            {
                                //	Wake up the mem dlg via the main app
                                FORWARD_WM_COMMAND(g_RAMainWnd, IDM_RA_FILES_MEMORYFINDER, nullptr, 0U, SendMessage);

                                //	Update the text to match
                                auto buffer{ std::make_unique<TCHAR[]>(16) };
                                ::_stprintf_s(buffer.get(), 16, _T("0x%06x"), rCond.CompTarget().RawValue());
                                SetDlgItemText(g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, buffer.get());

                                //	Nudge the ComboBox to update the mem note
                                FORWARD_WM_COMMAND(g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, nullptr,
                                                   CBN_EDITCHANGE, SendMessage);
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
                    if (pActiveAch == nullptr)
                        return FALSE;

                    if (pDispInfo->item.iItem < 0 || pDispInfo->item.iItem >= (int)pActiveAch->NumConditions(GetSelectedConditionGroup()))
                        return FALSE;

                    LVITEM lvItem;
                    lvItem.iItem = pDispInfo->item.iItem;
                    lvItem.iSubItem = pDispInfo->item.iSubItem;
                    lvItem.pszText = pDispInfo->item.pszText;
                    if (pDispInfo->item.iItem == -1 ||
                        pDispInfo->item.iSubItem == -1)
                    {
                        nSelItem = -1;
                        nSelSubItem = -1;
                        break;
                    }

                    //	Get cached data:
                    char* sData = LbxDataAt(pDispInfo->item.iItem, pDispInfo->item.iSubItem);

                    //	Update modified flag:
                    if (strcmp(ra::Narrow(lvItem.pszText).c_str(), sData) != 0)
                    {
                        //	Disable all achievement tracking:
                        //g_pActiveAchievements->SetPaused( true );
                        //CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );

                        //	Update this achievement as 'modified'
                        pActiveAch->SetModified(TRUE);
                        g_AchievementsDialog.OnEditAchievement(*pActiveAch);
                    }

                    //	Inject the new text into the lbx
                    SendDlgItemMessage(hDlg, IDC_RA_LBX_CONDITIONS, LVM_SETITEMTEXT, (WPARAM)lvItem.iItem, (LPARAM)&lvItem); // put new text

                    //	Update the cached data:
                    strcpy_s(sData, ra::MEM_STRING_TEXT_LEN, ra::Narrow( pDispInfo->item.pszText).c_str());

                    //	Update the achievement data:
                    Condition& rCond = pActiveAch->GetCondition(GetSelectedConditionGroup(), pDispInfo->item.iItem);
                    
                    switch (const auto csi{ ra::aCondSubItems.at(pDispInfo->item.iSubItem) }; csi)
                    {
                        case ra::CondSubItems::Group:
                        {
                            auto count{ 0 };
                            for (auto& condtype_name : ra::CONDITIONTYPE_STR)
                            {
                                if (sData == ra::Narrow(condtype_name))
                                    rCond.SetConditionType(ra::itoe<ra::ConditionType>(count));
                                count++;
                            }

                            UpdateCondition(GetDlgItem(hDlg, IDC_RA_LBX_CONDITIONS), pDispInfo->item, rCond);
                            break;
                        }
                        case ra::CondSubItems::TypeSrc:
                        {
                            if (strcmp(sData, "Mem") == 0)
                                rCond.CompSource().SetType(ra::ComparisonVariableType::Address);
                            else if (strcmp(sData, "Delta") == 0)
                                rCond.CompSource().SetType(ra::ComparisonVariableType::DeltaMem);
                            else
                                rCond.CompSource().SetType(ra::ComparisonVariableType::ValueComparison);

                            break;
                        }
                        case ra::CondSubItems::TypeTgt:
                        {
                            if (strcmp(sData, "Mem") == 0)
                                rCond.CompTarget().SetType(ra::ComparisonVariableType::Address);
                            else if (strcmp(sData, "Delta") == 0)
                                rCond.CompTarget().SetType(ra::ComparisonVariableType::DeltaMem);
                            else
                                rCond.CompTarget().SetType(ra::ComparisonVariableType::ValueComparison);

                            break;
                        }

                        case ra::CondSubItems::SizeSrc:
                        {
                            auto i{ 0 }; // signed since the underlying type is int
                            for (auto& cvs_name : ra::COMPARISONVARIABLESIZE_STR)
                            {
                                if (sData == ra::Narrow(cvs_name))
                                    rCond.CompSource().SetSize(ra::itoe<ra::ComparisonVariableSize>(i));
                                i++;
                            }

                            //	TBD: Limit validation
                            break;
                        }
                        case ra::CondSubItems::SizeTgt:
                        {
                            auto i{ 0 }; // signed since the underlying type is int
                            for (auto& cvs_name : ra::COMPARISONVARIABLESIZE_STR)
                            {
                                if (sData == ra::Narrow(cvs_name))
                                    rCond.CompTarget().SetSize(ra::itoe<ra::ComparisonVariableSize>(i));
                                i++;
                            }

                            //	TBD: Limit validation
                            break;
                        }
                        case ra::CondSubItems::Comparison:
                        {
                            auto count{ 0 };
                            for (auto& comptype_name : ra::COMPARISONTYPE_STR)
                            {
                                if(sData == ra::Narrow(comptype_name))
                                    rCond.SetCompareType(ra::itoe<ra::ComparisonType>(count));
                                count++;
                            }
                            break;
                        }

                        case ra::CondSubItems::ValueSrc:
                        {
                            int nBase = 16;
                            if (rCond.CompSource().Type() == ra::ComparisonVariableType::ValueComparison && g_bPreferDecimalVal)
                                nBase = 10;

                            unsigned int nVal = strtoul(sData, nullptr, nBase);
                            rCond.CompSource().SetValues(nVal, nVal);
                            break;
                        }
                        case ra::CondSubItems::ValueTgt:
                        {
                            int nBase = 16;
                            if (rCond.CompTarget().Type() == ra::ComparisonVariableType::ValueComparison && g_bPreferDecimalVal)
                                nBase = 10;

                            unsigned int nVal = strtoul(sData, nullptr, nBase);
                            rCond.CompTarget().SetValues(nVal, nVal);
                            break;
                        }
                        case ra::CondSubItems::HitCount:
                        {
                            //	Always decimal
                            rCond.SetRequiredHits(strtoul(sData, nullptr, 10));
                            break;
                        }
                        default:
                            ASSERT(!"Unhandled!");
                            break;
                    }

                    nSelItem = -1;
                    nSelSubItem = -1;

                }
                break;

                case TTN_GETDISPINFO:
                {
                    LPNMTTDISPINFO lpDispInfo = reinterpret_cast<LPNMTTDISPINFO>(lParam);
                    GetListViewTooltip();
                    if (!m_sTooltip.empty())
                    {
                        lpDispInfo->lpszText = m_sTooltip.data();
                        return TRUE;
                    }

                    lpDispInfo->szText[0] = '\0';
                    lpDispInfo->lpszText = lpDispInfo->szText;
                    return FALSE;
                }
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

void Dlg_AchievementEditor::GetListViewTooltip()
{
    m_sTooltip.clear();

    Achievement* pActiveAch = ActiveAchievement();
    if (pActiveAch == nullptr)
        return;

    HWND hList = GetDlgItem(GetHWND(), IDC_RA_LBX_CONDITIONS);

    LVHITTESTINFO lvHitTestInfo;
    GetCursorPos(&lvHitTestInfo.pt);
    ScreenToClient(hList, &lvHitTestInfo.pt);

    if (ListView_SubItemHitTest(hList, &lvHitTestInfo) == -1)
        return;

    m_nTooltipLocation = lvHitTestInfo.iItem * 256 + lvHitTestInfo.iSubItem;

    Condition& rCond = pActiveAch->GetCondition(GetSelectedConditionGroup(), lvHitTestInfo.iItem);
    unsigned int nAddr = 0;
    switch (auto csi{ra::aCondSubItems.at(lvHitTestInfo.iSubItem)}; csi)
    {
        using namespace ra::rel_ops;
        case ra::CondSubItems::ValueSrc:
            if (rCond.CompSource().Type() != ra::ComparisonVariableType::Address && rCond.CompSource().Type() != ra::ComparisonVariableType::DeltaMem)
                return;

            nAddr = rCond.CompSource().RawValue();
            break;

        case ra::CondSubItems::ValueTgt:
            if (rCond.CompTarget().Type() != ra::ComparisonVariableType::Address && rCond.CompTarget().Type() != ra::ComparisonVariableType::DeltaMem)
                return;

            nAddr = rCond.CompTarget().RawValue();
            break;

        default:
            return;
    }

    std::ostringstream oss;
    oss << ra::ByteAddressToString(nAddr) << "\r\n";

    const CodeNotes::CodeNoteObj* pNote = g_MemoryDialog.Notes().FindCodeNote(nAddr);
    if (pNote != nullptr)
        oss << pNote->Note();
    else
        oss << "[no notes]";

    m_sTooltip = NativeStr(oss.str());
}

void Dlg_AchievementEditor::UpdateSelectedBadgeImage(const std::string& sBackupBadgeToUse)
{
    std::string sAchievementBadgeURI;

    if (m_pSelectedAchievement != nullptr)
        sAchievementBadgeURI = m_pSelectedAchievement->BadgeImageURI();
    else if (sBackupBadgeToUse.length() > 2)
        sAchievementBadgeURI = sBackupBadgeToUse;

    m_hAchievementBadge.ChangeReference(ra::services::ImageType::Badge, sAchievementBadgeURI);
    HBITMAP hBitmap = m_hAchievementBadge.GetHBitmap();
    if (hBitmap != nullptr)
    {
        HWND hCheevoPic = GetDlgItem(m_hAchievementEditorDlg, IDC_RA_CHEEVOPIC);
        SendMessage(hCheevoPic, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBitmap);
        InvalidateRect(hCheevoPic, nullptr, TRUE);
    }

    //	Find buffer in the dropdown list
    int nSel = ComboBox_FindStringExact(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_BADGENAME), 0, sAchievementBadgeURI.c_str());
    if (nSel != -1)
        ComboBox_SetCurSel(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_BADGENAME), nSel);	//	Force select
}

void Dlg_AchievementEditor::UpdateBadge(const std::string& sNewName)
{
    //	If a change is detected: change it!
    if (m_pSelectedAchievement != nullptr)
    {
        if (m_pSelectedAchievement->BadgeImageURI().compare(sNewName) != 0)
        {
            //	The badge we are about to show is different from the one stored for this achievement.
            //	This implies that we are changing the badge: this achievement is modified!
            m_pSelectedAchievement->SetBadgeImage(sNewName);
            m_pSelectedAchievement->SetModified(TRUE);

            if (g_nActiveAchievementSet == ra::AchievementSetType::Core)
            {
                int nOffs = g_AchievementsDialog.GetSelectedAchievementIndex();
                g_AchievementsDialog.OnEditData(nOffs, ra::DlgAchievementColumn::Modified, "Yes");
            }
        }
    }

    //	Always attempt update.
    UpdateSelectedBadgeImage(sNewName.c_str());
}

void Dlg_AchievementEditor::RepopulateGroupList(Achievement* pCheevo)
{
    HWND hGroupList = GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_GROUP);
    if (hGroupList == nullptr)
        return;

    int nSel = ListBox_GetCurSel(hGroupList);

    while (ListBox_DeleteString(hGroupList, 0) >= 0) {}

    if (pCheevo != nullptr)
    {
        for (size_t i = 0; i < pCheevo->NumConditionGroups(); ++i)
        {
            ListBox_AddString(hGroupList, m_lbxGroupNames[i]);
        }
    }

    //	Try and restore selection
    if (nSel < 0 || nSel >= (int)pCheevo->NumConditionGroups())
        nSel = 0;	//	Reset to core if unsure

    ListBox_SetCurSel(hGroupList, nSel);
}

void Dlg_AchievementEditor::PopulateConditions(Achievement* pCheevo)
{
    HWND hCondList = GetDlgItem(m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS);
    if (hCondList == nullptr)
        return;

    ListView_DeleteAllItems(hCondList);
    m_nNumOccupiedRows = 0;

    if (pCheevo != nullptr)
    {
        unsigned int nGrp = GetSelectedConditionGroup();
        for (size_t i = 0; i < m_pSelectedAchievement->NumConditions(nGrp); ++i)
            AddCondition(hCondList, m_pSelectedAchievement->GetCondition(nGrp, i));
    }
}

void Dlg_AchievementEditor::LoadAchievement(Achievement* pCheevo, BOOL bAttemptKeepSelected)
{
    char buffer[1024];

    if (pCheevo == nullptr)
    {
        m_pSelectedAchievement = pCheevo;

        //	Just clear data

        m_bPopulatingAchievementEditorData = TRUE;

        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_ID, TEXT(""));
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_POINTS, TEXT(""));
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_TITLE, TEXT("<Inactive!>"));
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_DESC, TEXT("<Select or Create an Achievement first!>"));
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR, TEXT("<Inactive!>"));
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_BADGENAME, TEXT(""));

        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_ID), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_TITLE), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_DESC), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_POINTS), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ADDCOND), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_COPYCOND), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_DELETECOND), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_GROUP), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_PROGRESSINDICATORS), FALSE);

        UpdateBadge("00000");
        PopulateConditions(nullptr);

        m_bPopulatingAchievementEditorData = FALSE;
    }
    else if (m_pSelectedAchievement != pCheevo)
    {
        m_pSelectedAchievement = pCheevo;

        //	New achievement selected.
        //	Naturally: update everything!

        m_bPopulatingAchievementEditorData = TRUE;

        //	Repopulate the group box: 
        RepopulateGroupList(m_pSelectedAchievement);
        SetSelectedConditionGroup(0);	//	Select 0 by default
        PopulateConditions(m_pSelectedAchievement);

        CheckDlgButton(m_hAchievementEditorDlg, IDC_RA_CHK_ACH_ACTIVE, ActiveAchievement()->Active());
        CheckDlgButton(m_hAchievementEditorDlg, IDC_RA_CHK_ACH_PAUSE_ON_TRIGGER, ActiveAchievement()->GetPauseOnTrigger());
        CheckDlgButton(m_hAchievementEditorDlg, IDC_RA_CHK_ACH_PAUSE_ON_RESET, ActiveAchievement()->GetPauseOnReset());

        sprintf_s(buffer, 1024, "%u", m_pSelectedAchievement->ID());
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_ID, NativeStr(buffer).c_str());

        sprintf_s(buffer, 1024, "%u", m_pSelectedAchievement->Points());
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_POINTS, NativeStr(buffer).c_str());

        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_TITLE, NativeStr(m_pSelectedAchievement->Title()).c_str());
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_DESC, NativeStr(m_pSelectedAchievement->Description()).c_str());
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR, NativeStr(m_pSelectedAchievement->Author()).c_str());
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_BADGENAME, NativeStr(m_pSelectedAchievement->BadgeImageURI()).c_str());

        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_ID), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_TITLE), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_DESC), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_POINTS), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ADDCOND), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_COPYCOND), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_DELETECOND), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_GROUP), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_PROGRESSINDICATORS), FALSE);

        UpdateBadge(m_pSelectedAchievement->BadgeImageURI());

        m_bPopulatingAchievementEditorData = FALSE;
    }
    else
    {
        //	Same achievement still selected: what's changed?

        BOOL bTitleSelected = (GetFocus() == GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_TITLE));
        BOOL bDescSelected = (GetFocus() == GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_DESC));
        BOOL bPointsSelected = (GetFocus() == GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_POINTS));
        HWND hCtrl = GetDlgItem(m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS);

        if (!m_pSelectedAchievement->IsDirty())
            return;

        CheckDlgButton(m_hAchievementEditorDlg, IDC_RA_CHK_ACH_ACTIVE, ActiveAchievement()->Active());

        m_bPopulatingAchievementEditorData = TRUE;
        using namespace ra::bitwise_ops;

        if (ra::etoi(pCheevo->GetDirtyFlags() & ra::Achievement_DirtyFlags::ID))
            SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_ID, NativeStr(std::to_string(m_pSelectedAchievement->ID())).c_str());
        if (ra::etoi(pCheevo->GetDirtyFlags() & ra::Achievement_DirtyFlags::Points) && !bPointsSelected)
            SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_POINTS, NativeStr(std::to_string(m_pSelectedAchievement->Points())).c_str());
        if (ra::etoi(pCheevo->GetDirtyFlags() & ra::Achievement_DirtyFlags::Title) && !bTitleSelected)
            SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_TITLE, NativeStr(m_pSelectedAchievement->Title()).c_str());
        if (ra::etoi(pCheevo->GetDirtyFlags() & ra::Achievement_DirtyFlags::Description) && !bDescSelected)
            SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_DESC, NativeStr(m_pSelectedAchievement->Description()).c_str());
        if (ra::etoi(pCheevo->GetDirtyFlags() & ra::Achievement_DirtyFlags::Author))
            SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR, NativeStr(m_pSelectedAchievement->Author()).c_str());
        if (ra::etoi(pCheevo->GetDirtyFlags() & ra::Achievement_DirtyFlags::Badge))
        {
            SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_BADGENAME, NativeStr(m_pSelectedAchievement->BadgeImageURI()).c_str());
            UpdateBadge(m_pSelectedAchievement->BadgeImageURI());
        }

        if (ra::etoi(pCheevo->GetDirtyFlags() & ra::Achievement_DirtyFlags::Conditions))
        {
            HWND hCondList = GetDlgItem(m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS);
            if (hCondList != nullptr)
            {
                unsigned int nGrp = GetSelectedConditionGroup();

                if (ListView_GetItemCount(hCondList) != m_pSelectedAchievement->NumConditions(nGrp))
                {
                    PopulateConditions(pCheevo);
                }
                else
                {
                    LV_ITEM item;
                    ZeroMemory(&item, sizeof(item));
                    item.mask = LVIF_TEXT;
                    item.cchTextMax = 256;

                    for (size_t i = 0; i < m_pSelectedAchievement->NumConditions(nGrp); ++i)
                    {
                        const Condition& Cond = m_pSelectedAchievement->GetCondition(nGrp, i);
                        item.iItem = i;
                        UpdateCondition(hCondList, item, Cond);
                    }
                }
            }
        }

        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_ID), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_TITLE), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_DESC), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_POINTS), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ADDCOND), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_COPYCOND), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_DELETECOND), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_GROUP), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_PROGRESSINDICATORS), FALSE);

        m_bPopulatingAchievementEditorData = FALSE;
    }

    //InvalidateRect( hList, nullptr, TRUE );
    //UpdateWindow( hList );
    //RedrawWindow( hList, nullptr, nullptr, RDW_INVALIDATE );
}

void Dlg_AchievementEditor::OnLoad_NewRom()
{
    HWND hList = GetDlgItem(m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS);
    if (hList != nullptr)
    {
        SetIgnoreEdits(TRUE);
        SetupColumns(hList);

        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_ID, TEXT(""));
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_POINTS, TEXT(""));
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_TITLE, TEXT(""));
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_DESC, TEXT(""));
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR, TEXT(""));

        m_pSelectedAchievement = nullptr;
        SetIgnoreEdits(FALSE);
        LoadAchievement(nullptr, FALSE);
    }
}


size_t Dlg_AchievementEditor::GetSelectedConditionGroup() const
{
    HWND hList = GetDlgItem(g_AchievementEditorDialog.GetHWND(), IDC_RA_ACH_GROUP);
    int nSel = ListBox_GetCurSel(hList);
    if (nSel == LB_ERR)
    {
        OutputDebugString(TEXT("ListBox_GetCurSel returning LB_ERR\n"));
        return 0;
    }

    return static_cast<size_t>(nSel);
}

void Dlg_AchievementEditor::SetSelectedConditionGroup(size_t nGrp) const
{
    HWND hList = GetDlgItem(g_AchievementEditorDialog.GetHWND(), IDC_RA_ACH_GROUP);
    ListBox_SetCurSel(hList, static_cast<int>(nGrp));
}


void BadgeNames::FetchNewBadgeNamesThreaded()
{
    RAWeb::CreateThreadedHTTPRequest(ra::RequestType::BadgeIter);
}

void BadgeNames::OnNewBadgeNames(const Document& data)
{
    unsigned int nLowerLimit = data["FirstBadge"].GetUint();
    unsigned int nUpperLimit = data["NextBadge"].GetUint();

    SendMessage(m_hDestComboBox, WM_SETREDRAW, FALSE, LPARAM{});

    //	Clean out cbo
    while (ComboBox_DeleteString(m_hDestComboBox, 0) > 0)
    {
    }

    for (unsigned int i = nLowerLimit; i < nUpperLimit; ++i)
    {
        TCHAR buffer[256];
        _stprintf_s(buffer, _T("%05d"), i);
        ComboBox_AddString(m_hDestComboBox, buffer);
    }

    SendMessage(m_hDestComboBox, WM_SETREDRAW, TRUE, LPARAM{});
    RedrawWindow(m_hDestComboBox, nullptr, nullptr, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);

    //	Find buffer in the dropdown list
    if (g_AchievementEditorDialog.ActiveAchievement() != nullptr)
    {
        int nSel = ComboBox_FindStringExact(m_hDestComboBox, 0, g_AchievementEditorDialog.ActiveAchievement()->BadgeImageURI().c_str());
        if (nSel != -1)
            ComboBox_SetCurSel(m_hDestComboBox, nSel);	//	Force select
    }
}

void BadgeNames::AddNewBadgeName(const char* pStr, bool bAndSelect)
{
    int nSel = ComboBox_AddString(m_hDestComboBox, NativeStr(pStr).c_str());

    if (bAndSelect)
    {
        ComboBox_SelectString(m_hDestComboBox, 0, pStr);
        //ComboBox_SetCurSel( m_hDestComboBox, nSel );
    }
}

void GenerateResizes(HWND hDlg)
{
    RARect windowRect;
    GetWindowRect(hDlg, &windowRect);
    pDlgAchEditorMin.x = windowRect.Width();
    pDlgAchEditorMin.y = windowRect.Height();

    vDlgAchEditorResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_LBX_CONDITIONS), ra::AlignType::BOTTOM_RIGHT, TRUE));
    vDlgAchEditorResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_ACH_GROUP), ra::AlignType::BOTTOM, TRUE));

    vDlgAchEditorResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_CHK_ACH_PAUSE_ON_RESET), ra::AlignType::RIGHT, FALSE));
    vDlgAchEditorResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_CHK_ACH_PAUSE_ON_TRIGGER), ra::AlignType::RIGHT, FALSE));
    vDlgAchEditorResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_CHK_ACH_ACTIVE), ra::AlignType::RIGHT, FALSE));
    vDlgAchEditorResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_CHK_SHOW_DECIMALS), ra::AlignType::BOTTOM_RIGHT, FALSE));

    vDlgAchEditorResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_ACH_ADDGROUP), ra::AlignType::BOTTOM, FALSE));
    vDlgAchEditorResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_ACH_DELGROUP), ra::AlignType::BOTTOM, FALSE));
    vDlgAchEditorResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_ADDCOND), ra::AlignType::BOTTOM, FALSE));
    vDlgAchEditorResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_DELETECOND), ra::AlignType::BOTTOM, FALSE));
    vDlgAchEditorResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_COPYCOND), ra::AlignType::BOTTOM, FALSE));
    vDlgAchEditorResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_PASTECOND), ra::AlignType::BOTTOM, FALSE));
    vDlgAchEditorResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_MOVECONDUP), ra::AlignType::BOTTOM, FALSE));
    vDlgAchEditorResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_MOVECONDDOWN), ra::AlignType::BOTTOM, FALSE));

    vDlgAchEditorResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDCLOSE), ra::AlignType::BOTTOM_RIGHT, FALSE));
}
