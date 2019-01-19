#include "RA_Dlg_AchEditor.h"

#include "RA_AchievementSet.h"
#include "RA_Core.h"
#include "RA_Dlg_Achievement.h"
#include "RA_Dlg_Memory.h"
#include "RA_ImageFactory.h"
#include "RA_MemManager.h"
#include "RA_Resource.h"
#include "RA_User.h"
#include "RA_httpthread.h"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\drawing\gdi\ImageRepository.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "ra_math.h"

inline constexpr std::array<const char*, 10> COLUMN_TITLE{"ID",  "Flag", "Type", "Size",    "Memory",
                                                          "Cmp", "Type", "Size", "Mem/Val", "Hits"};
inline constexpr std::array<int, 10> COLUMN_WIDTH{30, 75, 42, 50, 72, 35, 42, 50, 72, 72};
static_assert(ra::is_same_size_v<decltype(COLUMN_TITLE), decltype(COLUMN_WIDTH)>, "Must match!");
static_assert(COLUMN_TITLE.size() == COLUMN_WIDTH.size());

enum class CondSubItems : std::size_t
{
    Id,
    Group,
    Type_Src,
    Size_Src,
    Value_Src,
    Comparison,
    Type_Tgt,
    Size_Tgt,
    Value_Tgt,
    Hitcount
};

Dlg_AchievementEditor g_AchievementEditorDialog;

// Dialog Resizing
std::vector<ResizeContent> vDlgAchEditorResize;
POINT pDlgAchEditorMin;

INT_PTR CALLBACK AchProgressProc(HWND hDlg, UINT nMsg, WPARAM wParam, _UNUSED LPARAM) noexcept
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

Dlg_AchievementEditor::Dlg_AchievementEditor() noexcept
{
    m_lbxGroupNames.front() = _T("Core");
    for (auto it = std::next(m_lbxGroupNames.begin()); it != m_lbxGroupNames.end(); ++it)
    {
        const auto i = std::distance(std::next(m_lbxGroupNames.begin()), it);
        *it = ra::StringPrintf(_T("Alt %02t"), i);
    }
}

void Dlg_AchievementEditor::SetupColumns(HWND hList)
{
    // Remove all columns,
    while (ListView_DeleteColumn(hList, 0))
    {
    }

    // Remove all data.
    ListView_DeleteAllItems(hList);

    auto i = 0U;
    for (const auto sTitle : COLUMN_TITLE)
    {
        ra::tstring colTitle{NativeStr(sTitle)}; // Take non-const copy
        LV_COLUMN col{
            col.mask = ra::to_unsigned(LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT),
            col.fmt = LVCFMT_LEFT | LVCFMT_FIXED_WIDTH,
            col.cx = COLUMN_WIDTH.at(i),
            col.pszText = colTitle.data(),
            col.cchTextMax = 255,
            col.iSubItem = ra::to_signed(i),
        };

        if (i == (m_nNumCols - 1))
            col.fmt |= LVCFMT_FILL;

        ListView_InsertColumn(hList, i, &col);
        i++;
    }

    ZeroMemory(&m_lbxData, sizeof(m_lbxData));

    m_nNumOccupiedRows = 0;

    BOOL bSuccess = ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);
    bSuccess = ListView_EnableGroupView(hList, FALSE);

    // HWND hGroupList = GetDlgItem( m_hAchievementEditorDlg, IDC_RA_ACH_GROUP );
    // ListBox_AddString(
    // bSuccess = ListView_SetExtendedListViewStyle( hGroupList, LVS_EX_FULLROWSELECT );
    // bSuccess = ListView_EnableGroupView( hGroupList, FALSE );
}

BOOL Dlg_AchievementEditor::IsActive() const noexcept
{
    return (g_AchievementEditorDialog.GetHWND() != nullptr) && (IsWindowVisible(g_AchievementEditorDialog.GetHWND()));
}

const int Dlg_AchievementEditor::AddCondition(HWND hList, const Condition& Cond, unsigned int nCurrentHits)
{
    LV_ITEM item{};
    item.mask = LVIF_TEXT;
    item.cchTextMax = 256;
    item.iItem = m_nNumOccupiedRows;
    ra::tstring sData{NativeStr(LbxDataAt(m_nNumOccupiedRows, CondSubItems::Id))};
    item.pszText = sData.data();
    item.iItem = ListView_InsertItem(hList, &item);

    UpdateCondition(hList, item, Cond, nCurrentHits);

    Ensures(item.iItem == m_nNumOccupiedRows);

    m_nNumOccupiedRows++;
    return item.iItem;
}

void Dlg_AchievementEditor::UpdateCondition(HWND hList, LV_ITEM& item, const Condition& Cond, unsigned int nCurrentHits)
{
    const int nRow = item.iItem;

    // Update our local array:
    const char* sMemTypStrSrc = "Value";
    std::string sMemSizeStrSrc;
    if (Cond.CompSource().GetType() != CompVariable::Type::ValueComparison)
    {
        sMemTypStrSrc = (Cond.CompSource().GetType() == CompVariable::Type::Address) ? "Mem" : "Delta";
        sMemSizeStrSrc = ra::Narrow(MEMSIZE_STR.at(ra::etoi(Cond.CompSource().GetSize())));
    }

    const char* sMemTypStrDst = "Value";
    std::string sMemSizeStrDst;
    if (Cond.CompTarget().GetType() != CompVariable::Type::ValueComparison)
    {
        sMemTypStrDst = (Cond.CompTarget().GetType() == CompVariable::Type::Address) ? "Mem" : "Delta";
        sMemSizeStrDst = ra::Narrow(MEMSIZE_STR.at(ra::etoi(Cond.CompTarget().GetSize())));
    }

    LbxDataAt(nRow, CondSubItems::Id) = std::to_string(nRow + 1);;
    LbxDataAt(nRow, CondSubItems::Group) = ra::Narrow(Condition::TYPE_STR.at(ra::etoi(Cond.GetConditionType())));
    LbxDataAt(nRow, CondSubItems::Type_Src) = ra::Narrow(sMemTypStrSrc);
    LbxDataAt(nRow, CondSubItems::Size_Src) = ra::Narrow(sMemSizeStrSrc);
    LbxDataAt(nRow, CondSubItems::Value_Src) = ra::StringPrintf("0x%06x", Cond.CompSource().GetValue());
    LbxDataAt(nRow, CondSubItems::Comparison) = ra::Narrow(COMPARISONTYPE_STR.at(ra::etoi(Cond.CompareType())));
    LbxDataAt(nRow, CondSubItems::Type_Tgt) = ra::Narrow(sMemTypStrDst);
    LbxDataAt(nRow, CondSubItems::Size_Tgt) = ra::Narrow(sMemSizeStrDst);
    LbxDataAt(nRow, CondSubItems::Value_Tgt) = ra::StringPrintf("0x%02x", Cond.CompTarget().GetValue());
    LbxDataAt(nRow, CondSubItems::Hitcount) = ra::StringPrintf("%u (%u)", Cond.RequiredHits(), nCurrentHits);

    auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal))
    {
        if (Cond.CompTarget().GetType() == CompVariable::Type::ValueComparison)
            LbxDataAt(nRow, CondSubItems::Value_Tgt) = std::to_string(Cond.CompTarget().GetValue());
    }

    if (Cond.IsAddCondition() || Cond.IsSubCondition())
    {
        LbxDataAt(nRow, CondSubItems::Comparison).clear();
        LbxDataAt(nRow, CondSubItems::Type_Tgt).clear();
        LbxDataAt(nRow, CondSubItems::Size_Tgt).clear();
        LbxDataAt(nRow, CondSubItems::Value_Tgt).clear();
        LbxDataAt(nRow, CondSubItems::Hitcount).clear();
    }

    for (auto it = COLUMN_TITLE.cbegin(); it != COLUMN_TITLE.cend(); ++it)
    {
        const auto i = gsl::narrow<int>(std::distance(COLUMN_TITLE.cbegin(), it));
        item.iSubItem = i;
        ra::tstring sData{NativeStr(LbxDataAt(nRow, ra::itoe<CondSubItems>(i)))};
        item.pszText = sData.data();
        ListView_SetItem(hList, &item);
    }
}

WNDPROC EOldProc;
HWND g_hIPEEdit;
int nSelItem;
int nSelSubItem;

LRESULT CALLBACK EditProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam) noexcept
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
            // inline suppression not working, and by function not working, have to disable it by code

#pragma warning(suppress: 26454)
            GSL_SUPPRESS(io.5) lvDispinfo.hdr.code = LVN_ENDLABELEDIT;

            lvDispinfo.item.mask = LVIF_TEXT;
            lvDispinfo.item.iItem = nSelItem;
            lvDispinfo.item.iSubItem = nSelSubItem;
            lvDispinfo.item.pszText = nullptr;

            std::array<char, 12> sEditText{};
            GetWindowTextA(hwnd, sEditText.data(), 12);
            lvDispinfo.item.pszText = sEditText.data();
            lvDispinfo.item.cchTextMax = gsl::narrow_cast<int>(std::strlen(sEditText.data()));

            HWND hList = GetDlgItem(g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS);

            // the LV ID and the LVs Parent window's HWND
            FORWARD_WM_NOTIFY(GetParent(hList), IDC_RA_LBX_CONDITIONS, &lvDispinfo, SendMessage); // ##reinterpret? ##SD

            if (g_AchievementEditorDialog.LbxDataAt(lvDispinfo.item.iItem, CondSubItems::Type_Tgt).compare("Value") == 0)
            {
                // Remove the associated 'size' entry
                if ((lvDispinfo.item.iItem >= 0) && (lvDispinfo.item.iSubItem >= 1))
                {
                    g_AchievementEditorDialog
                        .LbxDataAt(lvDispinfo.item.iItem, ra::itoe<CondSubItems>(lvDispinfo.item.iSubItem))
                        .clear();
                }
            }

            DestroyWindow(hwnd);
            return 0;
        }

        case WM_KEYDOWN:
        {
            if (wParam == VK_RETURN || wParam == VK_ESCAPE)
            {
                DestroyWindow(hwnd); // Causing a killfocus :)
            }
            else
            {
                // Ignore keystroke, or pass it into the edit box!
                break;
            }

            break;
        }
    }

    return CallWindowProc(EOldProc, hwnd, nMsg, wParam, lParam);
}

LRESULT CALLBACK DropDownProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam) noexcept
{
    if (nMsg == WM_COMMAND)
    {
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
            LV_DISPINFO lvDispinfo;
            ZeroMemory(&lvDispinfo, sizeof(LV_DISPINFO));
            lvDispinfo.hdr.hwndFrom = hwnd;
            lvDispinfo.hdr.idFrom = GetDlgCtrlID(hwnd);
#pragma warning(suppress: 26454)
            GSL_SUPPRESS(io.5) lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
            lvDispinfo.item.mask = LVIF_TEXT;
            lvDispinfo.item.iItem = nSelItem;
            lvDispinfo.item.iSubItem = nSelSubItem;
            lvDispinfo.item.pszText = nullptr;

            TCHAR sEditText[32];
            GetWindowText(hwnd, sEditText, 32);
            lvDispinfo.item.pszText = sEditText;
            lvDispinfo.item.cchTextMax = lstrlen(sEditText);

            // the LV ID and the LVs Parent window's HWND
            HWND hList = GetDlgItem(g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS);
            FORWARD_WM_NOTIFY(GetParent(hList), IDC_RA_LBX_CONDITIONS, &lvDispinfo, SendMessage);
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
#pragma warning(suppress: 26454)
            GSL_SUPPRESS(io.5) lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
            lvDispinfo.item.mask = LVIF_TEXT;
            lvDispinfo.item.iItem = nSelItem;
            lvDispinfo.item.iSubItem = nSelSubItem;
            lvDispinfo.item.pszText = nullptr;

            TCHAR sEditText[32];
            GetWindowText(hwnd, sEditText, 32);
            lvDispinfo.item.pszText = sEditText;
            lvDispinfo.item.cchTextMax = lstrlen(sEditText);

            HWND hList = GetDlgItem(g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS);

            // the LV ID and the LVs Parent window's HWND
            FORWARD_WM_NOTIFY(GetParent(hList), IDC_RA_LBX_CONDITIONS, &lvDispinfo, SendMessage);

            // DestroyWindow(hwnd);
            DestroyWindow(hwnd);
            return 0;
        }
        break;

        case WM_KEYDOWN:
            if (wParam == VK_RETURN)
            {
                DestroyWindow(hwnd);
            }
            else if (wParam == VK_ESCAPE)
            {
                // Undo changes: i.e. simply destroy the window!
                DestroyWindow(hwnd);
            }
            else
            {
                // Ignore keystroke, or pass it into the edit box!
            }

            break;

        case WM_COMMAND:
            switch (HIWORD(wParam))
            {
                case CBN_SELENDOK:
                case CBN_SELENDCANCEL:
                case CBN_SELCHANGE:
                case CBN_CLOSEUP:
                case CBN_KILLFOCUS:
                    break;
            }
    }

    return CallWindowProc(EOldProc, hwnd, nMsg, wParam, lParam);
}

BOOL CreateIPE(int nItem, CondSubItems nSubItem)
{
    BOOL bSuccess = FALSE;

    HWND hList = GetDlgItem(g_AchievementEditorDialog.GetHWND(), IDC_RA_LBX_CONDITIONS);

    RECT rcSubItem{};
    GSL_SUPPRESS_ES47 ListView_GetSubItemRect(hList, nItem, ra::etoi(nSubItem), LVIR_BOUNDS, &rcSubItem);

    RECT rcOffset{};
    GetWindowRect(hList, &rcOffset);

    rcSubItem.left += rcOffset.left;
    rcSubItem.right += rcOffset.left;
    rcSubItem.top += rcOffset.top;
    rcSubItem.bottom += rcOffset.top;

    const int nHeight = rcSubItem.bottom - rcSubItem.top;
    int nWidth = rcSubItem.right - rcSubItem.left;
    if (nSubItem == CondSubItems())
        nWidth = nWidth / 4; /*NOTE: the ListView has 4 columns;
                            when iSubItem == 0 (an item is clicked),
                            the width (largura) is divided by 4,
                            because for items (not subitems) the
                            width returned is that of the whole row.*/

    switch (nSubItem)
    {
        case CondSubItems::Id:
            ASSERT(!"First element does nothing!"); // nothing we do if we click the first element!
            break;

        case CondSubItems::Group:
        {
            // Condition: dropdown
            ASSERT(g_hIPEEdit == nullptr);
            if (g_hIPEEdit)
                break;

            g_hIPEEdit =
                CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("ComboBox"), TEXT(""),
                               WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | CBS_DROPDOWNLIST, rcSubItem.left,
                               rcSubItem.top, nWidth, ra::ftol(1.6F * nHeight * Condition::TYPE_STR.size()),
                               g_AchievementEditorDialog.GetHWND(), nullptr, GetModuleHandle(nullptr), nullptr);

            if (g_hIPEEdit == nullptr)
            {
                ASSERT(!"Could not create combo box!");
                MessageBox(nullptr, TEXT("Could not create combo box."), TEXT("Error"), MB_OK | MB_ICONERROR);
                break;
            };

            for (const auto str : Condition::TYPE_STR)
            {
                const auto i = ComboBox_AddString(g_hIPEEdit, str);
                if (g_AchievementEditorDialog.LbxDataAt(nItem, nSubItem) == ra::Narrow(str))
                    ComboBox_SetCurSel(g_hIPEEdit, i);
            }

            SetWindowFont(g_hIPEEdit, GetStockObject(DEFAULT_GUI_FONT), TRUE);
            ComboBox_ShowDropdown(g_hIPEEdit, TRUE);

            EOldProc = SubclassWindow(g_hIPEEdit, DropDownProc);
        }
        break;

        case CondSubItems::Type_Src:
        case CondSubItems::Type_Tgt:
        {
            // GetType: dropdown
            ASSERT(g_hIPEEdit == nullptr);
            if (g_hIPEEdit)
                break;

            if (nSubItem == CondSubItems::Type_Tgt)
            {
                const size_t nGrp = g_AchievementEditorDialog.GetSelectedConditionGroup();
                const Condition& Cond = g_AchievementEditorDialog.ActiveAchievement()->GetCondition(nGrp, nItem);

                if (Cond.IsAddCondition() || Cond.IsSubCondition())
                    break;
            }

            const int nNumItems = 3; // "Mem", "Delta" or "Value"

            g_hIPEEdit =
                CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("ComboBox"), TEXT(""),
                               WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | CBS_DROPDOWNLIST, rcSubItem.left,
                               rcSubItem.top, nWidth, static_cast<int>(1.6f * nHeight * nNumItems),
                               g_AchievementEditorDialog.GetHWND(), nullptr, GetModuleHandle(nullptr), nullptr);

            if (g_hIPEEdit == nullptr)
            {
                ASSERT(!"Could not create edit box!");
                MessageBox(nullptr, TEXT("Could not create edit box."), TEXT("Error"), MB_OK | MB_ICONERROR);
                break;
            };

            /*CB_ERRSPACE*/
            ComboBox_AddString(g_hIPEEdit, NativeStr("Mem").c_str());
            ComboBox_AddString(g_hIPEEdit, NativeStr("Delta").c_str());
            ComboBox_AddString(g_hIPEEdit, NativeStr("Value").c_str());

            int nSel{};
            if (g_AchievementEditorDialog.LbxDataAt(nItem, nSubItem) == "Mem")
                nSel = 0;
            else if (g_AchievementEditorDialog.LbxDataAt(nItem, nSubItem) == "Delta")
                nSel = 1;
            else
                nSel = 2;

            ComboBox_SetCurSel(g_hIPEEdit, nSel);

            SetWindowFont(g_hIPEEdit, GetStockObject(DEFAULT_GUI_FONT), TRUE);
            ComboBox_ShowDropdown(g_hIPEEdit, TRUE);

            EOldProc = SubclassWindow(g_hIPEEdit, DropDownProc);
        }
        break;
        case CondSubItems::Size_Src:
        case CondSubItems::Size_Tgt:
        {
            // Size: dropdown
            ASSERT(g_hIPEEdit == nullptr);
            if (g_hIPEEdit)
                break;

            // Note: this relies on column order :S
            using namespace ra::arith_ops;
            if (g_AchievementEditorDialog.LbxDataAt(nItem, nSubItem - 1) == "Value")
            {
                // Values have no size.
                break;
            }

            if (nSubItem == CondSubItems::Size_Tgt)
            {
                const size_t nGrp = g_AchievementEditorDialog.GetSelectedConditionGroup();
                const Condition& Cond = g_AchievementEditorDialog.ActiveAchievement()->GetCondition(nGrp, nItem);

                if (Cond.IsAddCondition() || Cond.IsSubCondition())
                    break;
            }

            g_hIPEEdit =
                CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("ComboBox"), TEXT(""),
                               WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | CBS_DROPDOWNLIST, rcSubItem.left,
                               rcSubItem.top, nWidth, ra::ftoi(1.6f * nHeight * MEMSIZE_STR.size()),
                               g_AchievementEditorDialog.GetHWND(), nullptr, GetModuleHandle(nullptr), nullptr);

            if (g_hIPEEdit == nullptr)
            {
                ASSERT(!"Could not create combo box!");
                MessageBox(nullptr, TEXT("Could not create combo box."), TEXT("Error"), MB_OK | MB_ICONERROR);
                break;
            };

            for (auto& str : MEMSIZE_STR)
            {
                const auto i{ComboBox_AddString(g_hIPEEdit, str)};
                if (g_AchievementEditorDialog.LbxDataAt(nItem, nSubItem) == ra::Narrow(str))
                    ComboBox_SetCurSel(g_hIPEEdit, i);
            }

            SetWindowFont(g_hIPEEdit, GetStockObject(DEFAULT_GUI_FONT), TRUE);
            ComboBox_ShowDropdown(g_hIPEEdit, TRUE);

            EOldProc = SubclassWindow(g_hIPEEdit, DropDownProc);
        }
        break;
        case CondSubItems::Comparison:
        {
            // Compare: dropdown
            ASSERT(g_hIPEEdit == nullptr);
            if (g_hIPEEdit)
                break;

            const size_t nGrp = g_AchievementEditorDialog.GetSelectedConditionGroup();
            const Condition& Cond = g_AchievementEditorDialog.ActiveAchievement()->GetCondition(nGrp, nItem);
            if (Cond.IsAddCondition() || Cond.IsSubCondition())
                break;

            g_hIPEEdit =
                CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("ComboBox"), TEXT(""),
                               WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | CBS_DROPDOWNLIST, rcSubItem.left,
                               rcSubItem.top, nWidth, static_cast<int>(1.6F * nHeight * COMPARISONTYPE_STR.size()),
                               g_AchievementEditorDialog.GetHWND(), nullptr, GetModuleHandle(nullptr), nullptr);

            if (g_hIPEEdit == nullptr)
            {
                ASSERT(!"Could not create combo box...");
                MessageBox(nullptr, TEXT("Could not create combo box."), TEXT("Error"), MB_OK | MB_ICONERROR);
                break;
            };

            for (const auto str : COMPARISONTYPE_STR)
            {
                const auto idx = ComboBox_AddString(g_hIPEEdit, str);
                if (g_AchievementEditorDialog.LbxDataAt(nItem, nSubItem) == ra::Narrow(str))
                    ComboBox_SetCurSel(g_hIPEEdit, idx);
            }

            SetWindowFont(g_hIPEEdit, GetStockObject(DEFAULT_GUI_FONT), TRUE);
            ComboBox_ShowDropdown(g_hIPEEdit, TRUE);

            EOldProc = SubclassWindow(g_hIPEEdit, DropDownProc);
        }
        break;
        case CondSubItems::Hitcount:
        case CondSubItems::Value_Src: // Mem/Val: edit
        case CondSubItems::Value_Tgt: // Mem/Val: edit
        {
            ASSERT(g_hIPEEdit == nullptr);
            if (g_hIPEEdit)
                break;

            if (nSubItem != CondSubItems::Value_Src)
            {
                const size_t nGrp = g_AchievementEditorDialog.GetSelectedConditionGroup();
                const Condition& Cond = g_AchievementEditorDialog.ActiveAchievement()->GetCondition(nGrp, nItem);

                if (Cond.IsAddCondition() || Cond.IsSubCondition())
                    break;
            }

            g_hIPEEdit = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT(""),
                                        WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | ES_WANTRETURN,
                                        rcSubItem.left, rcSubItem.top, nWidth, ra::ftoi(1.5f * nHeight),
                                        g_AchievementEditorDialog.GetHWND(), nullptr, GetModuleHandle(nullptr), nullptr);

            if (g_hIPEEdit == nullptr)
            {
                ASSERT(!"Could not create edit box!");
                MessageBox(nullptr, TEXT("Could not create edit box."), TEXT("Error"), MB_OK | MB_ICONERROR);
                break;
            };

            SetWindowFont(g_hIPEEdit, GetStockObject(DEFAULT_GUI_FONT), TRUE);

            const auto& pData = g_AchievementEditorDialog.LbxDataAt(nItem, nSubItem);
            SetWindowText(g_hIPEEdit, NativeStr(pData).c_str());

            // Special case, hitcounts
            if (nSubItem == CondSubItems::Hitcount)
            {
                if (g_AchievementEditorDialog.ActiveAchievement() != nullptr)
                {
                    const size_t nGrp = g_AchievementEditorDialog.GetSelectedConditionGroup();
                    const Condition& Cond = g_AchievementEditorDialog.ActiveAchievement()->GetCondition(nGrp, nItem);

                    char buffer[256];
                    sprintf_s(buffer, 256, "%u", Cond.RequiredHits());
                    SetWindowText(g_hIPEEdit, NativeStr(buffer).c_str());
                }
            }

            SendMessage(g_hIPEEdit, EM_SETSEL, 0, -1);
            SetFocus(g_hIPEEdit);
            EOldProc = SubclassWindow(g_hIPEEdit, EditProc);

            bSuccess = TRUE;
        }

        break;
    }

    return bSuccess;
}

// static
LRESULT CALLBACK Dlg_AchievementEditor::ListViewWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept
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
                    // setting lpszText to empty string, it won't try to display again unless we tell it to, so do so
                    // now
                    SendMessage(g_AchievementEditorDialog.m_hTooltip, TTM_POPUP, 0, 0);
                    // but we don't want the tooltip to immediately display when entering a new cell, so immediately
                    // hide it. the normal hover behavior will make it display as expected after a normal hover wait
                    // time.
                    SendMessage(g_AchievementEditorDialog.m_hTooltip, TTM_POP, 0, 0);
                }
            }
            break;
    }

    return CallWindowProc(g_AchievementEditorDialog.m_pListViewWndProc, hWnd, uMsg, wParam, lParam);
}

// static
INT_PTR CALLBACK Dlg_AchievementEditor::s_AchievementEditorProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Intercept some msgs?
    return g_AchievementEditorDialog.AchievementEditorProc(hDlg, uMsg, wParam, lParam);
}

INT_PTR Dlg_AchievementEditor::AchievementEditorProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bHandled = TRUE;

    switch (uMsg)
    {
        case WM_TIMER:
        {
            // Ignore if we are currently editing a box!
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
            auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
            CheckDlgButton(hDlg, IDC_RA_CHK_SHOW_DECIMALS,
                           pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal));

            // For scanning changes to achievement conditions (hit counts)
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
                                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hDlg, nullptr,
                                        GetInstanceModule(g_hThisDLLInst), nullptr);
            if (m_hTooltip)
            {
                TOOLINFO toolInfo;
                memset(&toolInfo, 0, sizeof(toolInfo));
                GSL_SUPPRESS_ES47 toolInfo.cbSize = TTTOOLINFO_V1_SIZE;
                toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
                toolInfo.hwnd = hDlg;
                GSL_SUPPRESS_TYPE1 toolInfo.uId = reinterpret_cast<UINT_PTR>(hList);
                toolInfo.lpszText = LPSTR_TEXTCALLBACK;
                GSL_SUPPRESS_TYPE1 SendMessage(m_hTooltip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo));
                SendMessage(m_hTooltip, TTM_ACTIVATE, TRUE, 0);
                SendMessage(m_hTooltip, TTM_SETMAXTIPWIDTH, 0, 320);
                SendMessage(m_hTooltip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000); // show for 30 seconds

                // install a mouse hook so we can show different tooltips per cell within the list view
                m_pListViewWndProc = SubclassWindow(hList, ListViewWndProc);
            }

            RestoreWindowPosition(hDlg, "Achievement Editor", true, true);
        }
            return TRUE;

        case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO lpmmi{};
            GSL_SUPPRESS_TYPE1 lpmmi = reinterpret_cast<LPMINMAXINFO>(lParam);
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
                    auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
                    pConfiguration.SetFeatureEnabled(
                        ra::services::Feature::PreferDecimal,
                        !pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal));
                    if (ActiveAchievement() != nullptr)
                    {
                        ActiveAchievement()->SetDirtyFlag(Achievement::DirtyFlags::All);
                        LoadAchievement(ActiveAchievement(), TRUE);
                        ActiveAchievement()->ClearDirtyFlag();
                    }
                    bHandled = TRUE;
                    break;
                }

                case IDC_RA_CHK_ACH_ACTIVE:
                    if (ActiveAchievement() != nullptr)
                    {
                        SendMessage(g_AchievementsDialog.GetHWND(), WM_COMMAND, IDC_RA_RESET_ACH, 0L);
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

                case IDC_RA_BADGENAME:
                {
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
                            // Restore?
                            break;
                    }

                    bHandled = TRUE;
                }
                break;

                case IDC_RA_PROGRESSINDICATORS:
                {
                    DialogBox((HINSTANCE)(GetModuleHandle(nullptr)), MAKEINTRESOURCE(IDD_RA_ACHIEVEMENTPROGRESS), hDlg,
                              AchProgressProc);
                    // DialogBox( ghInstance, MAKEINTRESOURCE(IDD_RA_ACHIEVEMENTPROGRESS), hDlg, AchProgressProc );
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

                            // Disable all achievement tracking
                            // g_pActiveAchievements->SetPaused( true );
                            // CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );

                            // Set this achievement as modified
                            pActiveAch->SetModified(TRUE);
                            g_AchievementsDialog.OnEditAchievement(*pActiveAch);

                            HWND hList = GetDlgItem(g_AchievementsDialog.GetHWND(), IDC_RA_LISTACHIEVEMENTS);
                            const int nSelectedIndex = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                            if (nSelectedIndex != -1)
                            {
                                // Implicit updating:
                                TCHAR buffer[1024];
                                if (GetDlgItemText(hDlg, IDC_RA_ACH_TITLE, buffer, 1024))
                                {
                                    pActiveAch->SetTitle(ra::Narrow(buffer));

                                    // Persist/Update/Inject local LBX data back into LBX (?)
                                    g_AchievementsDialog.OnEditData(
                                        g_pActiveAchievements->GetAchievementIndex(*pActiveAch),
                                        Dlg_Achievements::Column::Title, pActiveAch->Title());
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

                            // Disable all achievement tracking:
                            // g_pActiveAchievements->SetPaused( true );
                            // CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );

                            // Set this achievement as modified:
                            pActiveAch->SetModified(TRUE);
                            g_AchievementsDialog.OnEditAchievement(*pActiveAch);

                            HWND hList = GetDlgItem(g_AchievementsDialog.GetHWND(), IDC_RA_LISTACHIEVEMENTS);
                            const int nSelectedIndex = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                            if (nSelectedIndex == -1)
                                return FALSE;

                            // Implicit updating:
                            // char* psDesc = g_AchievementsDialog.LbxDataAt( nSelectedIndex, (int)Dlg_Achievements:: );
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

                            std::string buffer(16, '\0');
                            if (GetDlgItemTextA(hDlg, IDC_RA_ACH_POINTS, buffer.data(), 16))
                            {
                                int nVal = 0;
                                try
                                {
                                    nVal = std::stoi(NativeStr(buffer));
                                } catch (const std::invalid_argument& e)
                                {
                                    RA_LOG_ERR("Invalid Argument: %s", e.what());
                                    ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(
                                        L"The points field may only contain digits."); // for users

                                    // Set it back to 0 immediately, it won't change back by itself
                                    SetWindowText(::GetDlgItem(hDlg, IDC_RA_ACH_POINTS), _T("0"));
                                    return FALSE;
                                }

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
                    NewCondition.CompSource().Set(MemSize::EightBit, CompVariable::Type::Address, 0x0000);
                    NewCondition.CompTarget().Set(MemSize::EightBit, CompVariable::Type::ValueComparison,
                                                  0); // Compare defaults!

                    // Helper: guess that the currently watched memory location
                    //  is probably what they are about to want to add a cond for.
                    if (g_MemoryDialog.GetHWND() != nullptr)
                    {
                        TCHAR buffer[256];
                        GetDlgItemText(g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, buffer, 256);
                        unsigned int nVal = strtoul(ra::Narrow(buffer).c_str(), nullptr, 16);
                        NewCondition.CompSource().SetValue(nVal);
                    }

                    const size_t nNewID = pActiveAch->AddCondition(GetSelectedConditionGroup(), NewCondition) - 1;

                    // Disable all achievement tracking:
                    // g_pActiveAchievements->SetPaused( true );
                    // CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );

                    // Set this achievement as 'modified'
                    pActiveAch->SetModified(TRUE);
                    g_AchievementsDialog.OnEditAchievement(*pActiveAch);

                    LoadAchievement(pActiveAch, FALSE);
                    pActiveAch->ClearDirtyFlag();
                    pActiveAch->RebuildTrigger();

                    // Select last item
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LBX_CONDITIONS);
                    ListView_SetItemState(hList, nNewID, LVIS_FOCUSED | LVIS_SELECTED, ra::to_unsigned(-1));
                    ListView_EnsureVisible(hList, nNewID, FALSE);
                }
                break;

                case IDC_RA_COPYCOND:
                {
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LBX_CONDITIONS);
                    Achievement* pActiveAch = ActiveAchievement();
                    if (pActiveAch != nullptr)
                    {
                        const unsigned int uSelectedCount = ListView_GetSelectedCount(hList);

                        if (uSelectedCount > 0)
                        {
                            m_ConditionClipboard.Clear();

                            for (int i = ListView_GetNextItem(hList, -1, LVNI_SELECTED); i >= 0;
                                 i = ListView_GetNextItem(hList, i, LVNI_SELECTED))
                            {
                                const Condition& CondToCopy =
                                    pActiveAch->GetCondition(GetSelectedConditionGroup(), ra::to_unsigned(i));

                                const Condition NewCondition(CondToCopy);

                                m_ConditionClipboard.Add(NewCondition);
                            }
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
                                const size_t nNewID = pActiveAch->AddCondition(GetSelectedConditionGroup(),
                                                                               m_ConditionClipboard.GetAt(i)) -
                                                      1;
                                ListView_SetItemState(hList, nNewID, LVIS_FOCUSED | LVIS_SELECTED, ra::to_unsigned(-1));
                                ListView_EnsureVisible(hList, nNewID, FALSE);
                            }

                            // Update this achievement entry as 'modified':
                            pActiveAch->SetModified(TRUE);
                            g_AchievementsDialog.OnEditAchievement(*pActiveAch);

                            LoadAchievement(pActiveAch, FALSE);
                            pActiveAch->ClearDirtyFlag();
                            pActiveAch->RebuildTrigger();
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
                            const unsigned int uSelectedCount = ListView_GetSelectedCount(hList);

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

                                // Set this achievement as 'modified'
                                pActiveAch->SetModified(TRUE);
                                g_AchievementsDialog.OnEditAchievement(*pActiveAch);

                                // Refresh:
                                LoadAchievement(pActiveAch, TRUE);
                                pActiveAch->ClearDirtyFlag();
                                pActiveAch->RebuildTrigger();
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
                        const int nSelectedIndex = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                        if (nSelectedIndex >= 0)
                        {
                            //  Get conditions to move
                            std::vector<Condition> conditionsToMove;
                            const size_t nSelectedConditionGroup = GetSelectedConditionGroup();

                            for (int i = nSelectedIndex; i >= 0; i = ListView_GetNextItem(hList, i, LVNI_SELECTED))
                            {
                                // as we remove items, the index within the achievement changes, but not in the UI until
                                // we refresh
                                const auto nUpdatedIndex = ra::to_unsigned(i) - conditionsToMove.size();

                                const Condition& CondToMove =
                                    pActiveAch->GetCondition(nSelectedConditionGroup, nUpdatedIndex);
                                conditionsToMove.push_back(std::move(CondToMove));
                                pActiveAch->RemoveCondition(nSelectedConditionGroup, nUpdatedIndex);
                            }

                            //  Insert at new location
                            const int nInsertIndex = (nSelectedIndex > 0) ? nSelectedIndex - 1 : 0;
                            const size_t nInsertCount = conditionsToMove.size();

                            for (size_t i = 0; i < nInsertCount; ++i)
                            {
                                const Condition& CondToMove = conditionsToMove.at(i);
                                pActiveAch->InsertCondition(nSelectedConditionGroup, nInsertIndex + i, CondToMove);
                            }

                            // Set this achievement as 'modified'
                            pActiveAch->SetModified(TRUE);
                            pActiveAch->RebuildTrigger();
                            g_AchievementsDialog.OnEditAchievement(*pActiveAch);

                            // Refresh:
                            LoadAchievement(pActiveAch, TRUE);

                            //  Update selections
                            ListView_SetItemState(hList, -1, LVIF_STATE, LVIS_SELECTED);
                            for (size_t i = 0; i < nInsertCount; ++i)
                                ListView_SetItemState(hList, nInsertIndex + i, LVIS_FOCUSED | LVIS_SELECTED,
                                                      ra::to_unsigned(-1));

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
                            const size_t nSelectedConditionGroup = GetSelectedConditionGroup();

                            for (int i = nSelectedIndex; i >= 0; i = ListView_GetNextItem(hList, i, LVNI_SELECTED))
                            {
                                // as we remove items, the index within the achievement changes, but not in the UI until
                                // we refresh
                                const auto nUpdatedIndex = ra::to_unsigned(i) - conditionsToMove.size();

                                const Condition& CondToMove =
                                    pActiveAch->GetCondition(nSelectedConditionGroup, nUpdatedIndex);
                                conditionsToMove.push_back(std::move(CondToMove));
                                pActiveAch->RemoveCondition(nSelectedConditionGroup, nUpdatedIndex);

                                // want to insert after last selected item, update nSelectedIndex
                                nSelectedIndex = nUpdatedIndex;
                            }

                            //  Insert at new location
                            const int nConditionCount = pActiveAch->NumConditions(nSelectedConditionGroup);
                            const int nInsertIndex =
                                (nSelectedIndex < nConditionCount) ? nSelectedIndex + 1 : nConditionCount;
                            const size_t nInsertCount = conditionsToMove.size();

                            for (size_t i = 0; i < nInsertCount; ++i)
                            {
                                const Condition& CondToMove = conditionsToMove.at(i);
                                pActiveAch->InsertCondition(nSelectedConditionGroup, nInsertIndex + i, CondToMove);
                            }

                            // Set this achievement as 'modified'
                            pActiveAch->SetModified(TRUE);
                            pActiveAch->RebuildTrigger();
                            g_AchievementsDialog.OnEditAchievement(*pActiveAch);

                            // Refresh:
                            LoadAchievement(pActiveAch, TRUE);

                            //  Update selections
                            ListView_SetItemState(hList, -1, LVIF_STATE, LVIS_SELECTED);
                            for (size_t i = 0; i < nInsertCount; ++i)
                                ListView_SetItemState(hList, nInsertIndex + i, LVIS_FOCUSED | LVIS_SELECTED,
                                                      ra::to_unsigned(-1));

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

                    OPENFILENAME ofn{};
                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.hwndOwner = hDlg;
                    ofn.hInstance = GetModuleHandle(nullptr);
                    ofn.lpstrFile = buffer;
                    ofn.nMaxFile = BUF_SIZE - 1;

                    ofn.lpstrFilter = TEXT(
                        "Image Files\0"
                        "*.png;*.gif;*.jpg;*.jpeg\0")
                        TEXT(
                            "All Files\0"
                            "*.*\0\0"); // Last element MUST be doubly-terminated:
                                        // https://msdn.microsoft.com/en-us/library/windows/desktop/ms646839(v=vs.85).aspx

                    ofn.nFilterIndex = 1;
                    ofn.lpstrDefExt = TEXT("png");
                    ofn.Flags = OFN_FILEMUSTEXIST;

                    if (GetOpenFileName(&ofn) == 0) // 0 == cancelled
                        return FALSE;

                    if (ofn.lpstrFile != nullptr)
                    {
                        rapidjson::Document Response;
                        if (RAWeb::DoBlockingImageUpload(RequestUploadBadgeImage, ra::Narrow(ofn.lpstrFile), Response))
                        {
                            // TBD: ensure that:
                            // The image is copied to the cache/badge dir
                            // The image doesn't already exist in teh cache/badge dir (ask overwrite)
                            // The image is the correct dimensions or can be scaled
                            // The image can be uploaded OK
                            // The image is not copyright

                            const rapidjson::Value& ResponseData = Response["Response"];
                            if (ResponseData.HasMember("BadgeIter"))
                            {
                                const char* sNewBadgeIter = ResponseData["BadgeIter"].GetString();

                                // pBuffer will contain "OK:" and the number of the uploaded file.
                                // Add the value to the available values in the cbo, and it *should* self-populate.
                                MessageBox(nullptr, TEXT("Successful!"), TEXT("Upload OK"), MB_OK);

                                m_BadgeNames.AddNewBadgeName(sNewBadgeIter, true);
                                UpdateBadge(sNewBadgeIter);
                            }
                            else if (ResponseData.HasMember("Error"))
                            {
                                MessageBox(nullptr, NativeStr(ResponseData["Error"].GetString()).c_str(), TEXT("Error"),
                                           MB_OK);
                            }
                            else
                            {
                                MessageBox(nullptr,
                                           TEXT("Error in upload! Try another image format, or maybe smaller image?"),
                                           TEXT("Error"), MB_OK);
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
                    ActiveAchievement()->RebuildTrigger();
                }
                break;
                case IDC_RA_ACH_DELGROUP:
                {
                    if (ActiveAchievement() == nullptr)
                        break;

                    if (ActiveAchievement()->NumConditionGroups() == 3)
                    {
                        ActiveAchievement()->RemoveConditionGroup();
                        ActiveAchievement()->RemoveConditionGroup(); // ##SD this intentional?
                    }
                    else if (ActiveAchievement()->NumConditionGroups() > 3)
                    {
                        ActiveAchievement()->RemoveConditionGroup();
                    }
                    else
                    {
                        MessageBox(m_hAchievementEditorDlg, TEXT("Cannot remove Core Condition Group!"),
                                   TEXT("Warning"), MB_OK);
                    }

                    RepopulateGroupList(ActiveAchievement());
                    ActiveAchievement()->RebuildTrigger();
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

            // Switch also on the highword:

            switch (HIWORD(wParam))
            {
                case CBN_SELENDOK:
                case CBN_SELENDCANCEL:
                case CBN_SELCHANGE:
                case CBN_CLOSEUP:
                case CBN_KILLFOCUS:
                {
                    // Only post this if the edit control is not empty:
                    //  if it's empty, we're not dealing with THIS edit control!
                    if (g_hIPEEdit != nullptr)
                    {
                        LV_DISPINFO lvDispinfo;
                        ZeroMemory(&lvDispinfo, sizeof(LV_DISPINFO));
                        lvDispinfo.hdr.hwndFrom = g_hIPEEdit;
                        lvDispinfo.hdr.idFrom = GetDlgCtrlID(g_hIPEEdit);
#pragma warning(suppress: 26454)
                        GSL_SUPPRESS(io.5) lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
                        lvDispinfo.item.mask = LVIF_TEXT;
                        lvDispinfo.item.iItem = nSelItem;
                        lvDispinfo.item.iSubItem = nSelSubItem;
                        lvDispinfo.item.pszText = nullptr;

                        TCHAR sEditText[32];
                        GetWindowText(g_hIPEEdit, sEditText, 32);
                        lvDispinfo.item.pszText = sEditText;
                        lvDispinfo.item.cchTextMax = lstrlen(sEditText);

                        HWND hList = GetDlgItem(GetHWND(), IDC_RA_LBX_CONDITIONS);

                        // the LV ID and the LVs Parent window's HWND
                        FORWARD_WM_NOTIFY(GetParent(hList), IDC_RA_LBX_CONDITIONS, &lvDispinfo, SendMessage);

                        DestroyWindow(g_hIPEEdit);
                    }
                }
                break;
            }
            break;

        case WM_NOTIFY:
        {
#pragma warning(push)
#pragma warning(disable: 26490)
            GSL_SUPPRESS_TYPE1
            switch (((reinterpret_cast<LPNMHDR>(lParam))->code))
#pragma warning(pop)
            {
                case NM_CLICK:
                {
                    const NMITEMACTIVATE* pOnClick{};
                    GSL_SUPPRESS_TYPE1 pOnClick = reinterpret_cast<const NMITEMACTIVATE*>(lParam);
                    // http://cboard.cprogramming.com/windows-programming/122733-%5Bc%5D-editing-subitems-listview-win32-api.html

                    ASSERT(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS) != nullptr);

                    // Note: first item should be an ID!
                    if (pOnClick->iItem != -1 && pOnClick->iSubItem != 0)
                    {
                        nSelItem = pOnClick->iItem;
                        nSelSubItem = pOnClick->iSubItem;

                        CreateIPE(pOnClick->iItem, ra::itoe<CondSubItems>(pOnClick->iSubItem));
                    }
                    else
                    {
                        nSelItem = -1;
                        nSelSubItem = -1;
                        /*If SubItemHitTest does return error (lResult=-1),
                        it kills focus of hEdit in order to destroy it.*/
                        SendMessage(g_hIPEEdit, WM_KILLFOCUS, 0, 0);
                    }
                    return 0;
                }
                break;
                case NM_RCLICK:
                {
                    const NMITEMACTIVATE* pOnClick{};
                    GSL_SUPPRESS_TYPE1 pOnClick = reinterpret_cast<const NMITEMACTIVATE*>(lParam);

                    if (pOnClick->iItem != -1 && pOnClick->iSubItem != -1)
                    {
                        if (ra::to_unsigned(pOnClick->iItem) >
                            ActiveAchievement()->NumConditions(GetSelectedConditionGroup()))
                            return 0;

                        Condition& rCond =
                            ActiveAchievement()->GetCondition(GetSelectedConditionGroup(), pOnClick->iItem);

                        // HWND hMem = GetDlgItem( HWndMemoryDlg, IDC_RA_WATCHING );
                        using namespace ra::rel_ops;
                        if (pOnClick->iSubItem == CondSubItems::Value_Src)
                        {
                            if (rCond.CompSource().GetType() != CompVariable::Type::ValueComparison)
                            {
                                // Wake up the mem dlg via the main app
                                SendMessage(g_RAMainWnd, WM_COMMAND, IDM_RA_FILES_MEMORYFINDER, 0);

                                // Update the text to match
                                char buffer[16];
                                sprintf_s(buffer, 16, "0x%06x", rCond.CompSource().GetValue());
                                SetDlgItemText(g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, NativeStr(buffer).c_str());

                                // Nudge the ComboBox to update the mem note
                                SendMessage(g_MemoryDialog.GetHWND(), WM_COMMAND,
                                            MAKELONG(IDC_RA_WATCHING, CBN_EDITCHANGE), 0);
                            }
                        }
                        else if (pOnClick->iSubItem == CondSubItems::Value_Tgt)
                        {
                            if (rCond.CompTarget().GetType() != CompVariable::Type::ValueComparison)
                            {
                                // Wake up the mem dlg via the main app
                                SendMessage(g_RAMainWnd, WM_COMMAND, IDM_RA_FILES_MEMORYFINDER, 0);

                                // Update the text to match
                                char buffer[16];
                                sprintf_s(buffer, 16, "0x%06x", rCond.CompTarget().GetValue());
                                SetDlgItemText(g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, NativeStr(buffer).c_str());

                                // Nudge the ComboBox to update the mem note
                                SendMessage(g_MemoryDialog.GetHWND(), WM_COMMAND,
                                            MAKELONG(IDC_RA_WATCHING, CBN_EDITCHANGE), 0);
                            }
                        }
                    }
                    bHandled = TRUE;
                }
                break;

                case LVN_ENDLABELEDIT:
                {
                    NMLVDISPINFO* pDispInfo{};
                    GSL_SUPPRESS_TYPE1 pDispInfo = reinterpret_cast<NMLVDISPINFO*>(lParam);

                    Achievement* pActiveAch = ActiveAchievement();
                    if (pActiveAch == nullptr)
                        return FALSE;

                    if (pDispInfo->item.iItem < 0 ||
                        pDispInfo->item.iItem >= ra::to_signed(pActiveAch->NumConditions(GetSelectedConditionGroup())))
                        return FALSE;

                    LVITEM lvItem;
                    lvItem.iItem = pDispInfo->item.iItem;
                    lvItem.iSubItem = pDispInfo->item.iSubItem;
                    lvItem.pszText = pDispInfo->item.pszText;
                    if (pDispInfo->item.iItem == -1 || pDispInfo->item.iSubItem == -1)
                    {
                        nSelItem = -1;
                        nSelSubItem = -1;
                        break;
                    }

                    // Get cached data:
                    auto& sData = LbxDataAt(pDispInfo->item.iItem, ra::itoe<CondSubItems>(pDispInfo->item.iSubItem));

                    // Update modified flag:
                    if (ra::Narrow(lvItem.pszText) != sData)
                    {
                        // Disable all achievement tracking:
                        // g_pActiveAchievements->SetPaused( true );
                        // CheckDlgButton( HWndAchievementsDlg, IDC_RA_CHKACHPROCESSINGACTIVE, FALSE );

                        // Update this achievement as 'modified'
                        pActiveAch->SetModified(TRUE);
                        g_AchievementsDialog.OnEditAchievement(*pActiveAch);
                    }

                    // Inject the new text into the lbx 
                    ListView_SetItemText(::GetDlgItem(hDlg, IDC_RA_LBX_CONDITIONS), lvItem.iItem, lvItem.iSubItem,
                                         lvItem.pszText); // put new text

                    // Update the cached data:
                    sData = ra::Narrow(pDispInfo->item.pszText);

                    // Update the achievement data:
                    Condition& rCond = pActiveAch->GetCondition(GetSelectedConditionGroup(), pDispInfo->item.iItem);
                    switch (ra::itoe<CondSubItems>(pDispInfo->item.iSubItem))
                    {
                        case CondSubItems::Group:
                        {
                            auto i = 0U;
                            for (const auto str : Condition::TYPE_STR)
                            {
                                if (sData == ra::Narrow(str))
                                    rCond.SetConditionType(ra::itoe<Condition::Type>(i));
                                i++;
                            }
                            UpdateCondition(
                                GetDlgItem(hDlg, IDC_RA_LBX_CONDITIONS), pDispInfo->item, rCond,
                                pActiveAch->GetConditionHitCount(GetSelectedConditionGroup(), pDispInfo->item.iItem));
                            break;
                        }
                        case CondSubItems::Type_Src:
                        {
                            if (sData == "Mem")
                                rCond.CompSource().SetType(CompVariable::Type::Address);
                            else if (sData == "Delta")
                                rCond.CompSource().SetType(CompVariable::Type::DeltaMem);
                            else
                                rCond.CompSource().SetType(CompVariable::Type::ValueComparison);

                            break;
                        }
                        case CondSubItems::Type_Tgt:
                        {
                            if (sData == "Mem")
                                rCond.CompTarget().SetType(CompVariable::Type::Address);
                            else if (sData == "Delta")
                                rCond.CompTarget().SetType(CompVariable::Type::DeltaMem);
                            else
                                rCond.CompTarget().SetType(CompVariable::Type::ValueComparison);

                            break;
                        }

                        case CondSubItems::Size_Src:
                        {
                            auto i{0};
                            for (auto& str : MEMSIZE_STR)
                            {
                                if (sData == ra::Narrow(str))
                                    rCond.CompSource().SetSize(ra::itoe<MemSize>(i));
                                i++;
                            }
                            // TBD: Limit validation
                        }
                        break;
                        case CondSubItems::Size_Tgt:
                        {
                            auto i{0};
                            for (auto& str : MEMSIZE_STR)
                            {
                                if (sData == ra::Narrow(str))
                                    rCond.CompTarget().SetSize(ra::itoe<MemSize>(i));
                                i++;
                            }
                            // TBD: Limit validation
                        }
                        break;
                        case CondSubItems::Comparison:
                        {
                            auto i = 0U;
                            for (const auto str : COMPARISONTYPE_STR)
                            {
                                if (sData == ra::Narrow(str))
                                    rCond.SetCompareType(ra::itoe<ComparisonType>(i));
                                i++;
                            }
                        }
                        break;

                        case CondSubItems::Value_Src:
                        {
                            int nBase = 16;
                            if (rCond.CompSource().GetType() == CompVariable::Type::ValueComparison)
                            {
                                auto& pConfiguration =
                                    ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
                                if (pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal))
                                    nBase = 10;
                            }

                            const auto nVal = std::stoul(sData, nullptr, nBase);
                            rCond.CompSource().SetValue(nVal);
                            break;
                        }
                        case CondSubItems::Value_Tgt:
                        {
                            int nBase = 16;
                            if (rCond.CompTarget().GetType() == CompVariable::Type::ValueComparison)
                            {
                                auto& pConfiguration =
                                    ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
                                if (pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal))
                                    nBase = 10;
                            }
                            
                            auto lVal = 0UL;
                            auto lineErrNumber = 0;
                            try
                            {
                                lineErrNumber = __LINE__ + 1;
                                lVal = std::stoul(sData.c_str(), nullptr, nBase);
                            } catch (const std::invalid_argument& e)
                            {
                                RA_LOG_ERR("\nFunction: %s\nLine: %d\nMessage: %s\n\n", __FUNCTION__, lineErrNumber,
                                           e.what());
                                const auto bIsDecimal = Button_GetCheck(::GetDlgItem(hDlg, IDC_RA_CHK_SHOW_DECIMALS));
                                if (bIsDecimal)
                                {
                                    ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Invalid Input",
                                        L"Only values that can be represented as decimal are allowed while the 'show "
                                        L"decimal values' checkbox is "
                                        L"checked.\n\nPlease try again!");
                                }
                                else
                                {
                                    ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Invalid Input",
                                        L"Only values that can be represented as hexadecimal are allowed while the "
                                        L"'show decimal values' checkbox is unchecked.\n\nPlease try again!");
                                }
                            } catch (const std::out_of_range& e)
                            {
                                RA_LOG_ERR("Function: %s\nLine: %d\nMessage: %s", __FUNCTION__, lineErrNumber,
                                           e.what());
                                constexpr auto maxUlong = std::numeric_limits<unsigned long>::max();
                                ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(
                                    L"Too Large",
                                    ra::StringPrintf(L"Value supplied is negative or too large!\nIt must be "
                                                     L"non-negative and less than %lu (decimal) or 0x%X (hexadecimal)",
                                                     maxUlong, maxUlong));
                            }
                            rCond.CompTarget().SetValue(lVal);
                            break;
                        }
                        case CondSubItems::Hitcount:
                        {
                            // Always decimal
                            try
                            {
                                rCond.SetRequiredHits(std::stoul(sData));
                            } catch (const std::invalid_argument& e)
                            {
                                RA_LOG_ERR("invalid_argument: %s", e.what());
                                ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(
                                    L"Only digits are allowed in this field!");
                                rCond.SetRequiredHits(0);
                            }
                            break;
                        }
                        default:
                            ASSERT(!"Unhandled!");
                            break;
                    }

                    nSelItem = -1;
                    nSelSubItem = -1;

                    pActiveAch->RebuildTrigger();
                }
                break;

                case TTN_GETDISPINFO:
                {
                    LPNMTTDISPINFO lpDispInfo{};
                    GSL_SUPPRESS_TYPE1 lpDispInfo = reinterpret_cast<LPNMTTDISPINFO>(lParam);

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
    }

    return !bHandled;
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
    switch (ra::itoe<CondSubItems>(lvHitTestInfo.iSubItem))
    {
        case CondSubItems::Value_Src:
            if (rCond.CompSource().GetType() != CompVariable::Type::Address &&
                rCond.CompSource().GetType() != CompVariable::Type::DeltaMem)
                return;

            nAddr = rCond.CompSource().GetValue();
            break;

        case CondSubItems::Value_Tgt:
            if (rCond.CompTarget().GetType() != CompVariable::Type::Address &&
                rCond.CompTarget().GetType() != CompVariable::Type::DeltaMem)
                return;

            nAddr = rCond.CompTarget().GetValue();
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

GSL_SUPPRESS_CON4
void Dlg_AchievementEditor::UpdateSelectedBadgeImage(const std::string& sBackupBadgeToUse)
{
    std::string sAchievementBadgeURI;

    if (m_pSelectedAchievement != nullptr)
        sAchievementBadgeURI = m_pSelectedAchievement->BadgeImageURI();
    else if (sBackupBadgeToUse.length() > 2)
        sAchievementBadgeURI = sBackupBadgeToUse;

    m_hAchievementBadge.ChangeReference(ra::ui::ImageType::Badge, sAchievementBadgeURI);
    GSL_SUPPRESS_CON4 // inline suppression for this currently not working
    const auto hBitmap = ra::ui::drawing::gdi::ImageRepository::GetHBitmap(m_hAchievementBadge);

    if (hBitmap != nullptr)
    {
        HWND hCheevoPic = GetDlgItem(m_hAchievementEditorDlg, IDC_RA_CHEEVOPIC);
        GSL_SUPPRESS_TYPE1
        SendMessage(hCheevoPic, STM_SETIMAGE, ra::to_unsigned(IMAGE_BITMAP), reinterpret_cast<LPARAM>(hBitmap));
        InvalidateRect(hCheevoPic, nullptr, TRUE);
    }

    // Find buffer in the dropdown list
    const int nSel = ComboBox_FindStringExact(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_BADGENAME), 0,
                                              sAchievementBadgeURI.c_str());
    if (nSel != -1)
        ComboBox_SetCurSel(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_BADGENAME), nSel); // Force select
}

void Dlg_AchievementEditor::UpdateBadge(const std::string& sNewName)
{
    // If a change is detected: change it!
    if (m_pSelectedAchievement != nullptr)
    {
        if (m_pSelectedAchievement->BadgeImageURI().compare(sNewName) != 0)
        {
            // The badge we are about to show is different from the one stored for this achievement.
            // This implies that we are changing the badge: this achievement is modified!
            m_pSelectedAchievement->SetBadgeImage(sNewName);
            m_pSelectedAchievement->SetModified(TRUE);

            if (g_nActiveAchievementSet == AchievementSet::Type::Core)
            {
                const int nOffs = g_AchievementsDialog.GetSelectedAchievementIndex();
                g_AchievementsDialog.OnEditData(nOffs, Dlg_Achievements::Column::Modified, "Yes");
            }
        }
    }

    // Always attempt update.
    UpdateSelectedBadgeImage(sNewName.c_str());
}

_Use_decl_annotations_ void
    Dlg_AchievementEditor::RepopulateGroupList(const Achievement* const restrict pCheevo) noexcept
{
    HWND hGroupList = GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_GROUP);
    if (hGroupList == nullptr)
        return;

    int nSel = ListBox_GetCurSel(hGroupList);

    while (ListBox_DeleteString(hGroupList, 0) >= 0)
    {
    }

    if (pCheevo != nullptr)
    {
        for (size_t i = 0; i < pCheevo->NumConditionGroups(); ++i)
            ListBox_AddString(hGroupList, m_lbxGroupNames.at(i).c_str());

        // Try and restore selection
        if (nSel < 0 || nSel >= gsl::narrow_cast<int>(pCheevo->NumConditionGroups()))
            nSel = 0; // Reset to core if unsure
        ListBox_SetCurSel(hGroupList, nSel);
    }
}

_Use_decl_annotations_ void Dlg_AchievementEditor::PopulateConditions(const Achievement* const restrict pCheevo)
{
    HWND hCondList = GetDlgItem(m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS);
    if (hCondList == nullptr)
        return;

    ListView_DeleteAllItems(hCondList);
    m_nNumOccupiedRows = 0;

    if (pCheevo != nullptr)
    {
        const unsigned int nGrp = GetSelectedConditionGroup();
        for (size_t i = 0; i < m_pSelectedAchievement->NumConditions(nGrp); ++i)
            AddCondition(hCondList, m_pSelectedAchievement->GetCondition(nGrp, i),
                         m_pSelectedAchievement->GetConditionHitCount(nGrp, i));

        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ADDCOND),
                     m_pSelectedAchievement->NumConditions(nGrp) < MAX_CONDITIONS);
    }
}

void Dlg_AchievementEditor::LoadAchievement(Achievement* pCheevo, _UNUSED BOOL)
{
    if (pCheevo == nullptr)
    {
        m_pSelectedAchievement = pCheevo;

        // Just clear data

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

        // New achievement selected.
        // Naturally: update everything!

        m_bPopulatingAchievementEditorData = TRUE;

        // Repopulate the group box:
        RepopulateGroupList(m_pSelectedAchievement);
        SetSelectedConditionGroup(0); // Select 0 by default
        PopulateConditions(m_pSelectedAchievement);

        CheckDlgButton(m_hAchievementEditorDlg, IDC_RA_CHK_ACH_ACTIVE, ActiveAchievement()->Active());
        CheckDlgButton(m_hAchievementEditorDlg, IDC_RA_CHK_ACH_PAUSE_ON_TRIGGER,
                       ActiveAchievement()->GetPauseOnTrigger());
        CheckDlgButton(m_hAchievementEditorDlg, IDC_RA_CHK_ACH_PAUSE_ON_RESET, ActiveAchievement()->GetPauseOnReset());

        if (m_pSelectedAchievement->Category() == ra::etoi(AchievementSet::Type::Local))
            SetDlgItemTextA(m_hAchievementEditorDlg, IDC_RA_ACH_ID, "0");
        else
            SetDlgItemTextA(m_hAchievementEditorDlg, IDC_RA_ACH_ID, std::to_string(m_pSelectedAchievement->ID()).c_str());

        const auto buffer = std::to_string(m_pSelectedAchievement->Points());
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_POINTS, NativeStr(buffer).c_str());

        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_TITLE, NativeStr(m_pSelectedAchievement->Title()).c_str());
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_DESC,
                       NativeStr(m_pSelectedAchievement->Description()).c_str());
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR, NativeStr(m_pSelectedAchievement->Author()).c_str());
        SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_BADGENAME,
                       NativeStr(m_pSelectedAchievement->BadgeImageURI()).c_str());

        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_ID), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_TITLE), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_DESC), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_POINTS), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ADDCOND),
                     m_pSelectedAchievement->NumConditions(0) < MAX_CONDITIONS);
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
        // Same achievement still selected: what's changed?

        const BOOL bTitleSelected = (GetFocus() == GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_TITLE));
        const BOOL bDescSelected = (GetFocus() == GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_DESC));
        const BOOL bPointsSelected = (GetFocus() == GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_POINTS));

        if (!m_pSelectedAchievement->IsDirty())
            return;

        CheckDlgButton(m_hAchievementEditorDlg, IDC_RA_CHK_ACH_ACTIVE, ActiveAchievement()->Active());

        m_bPopulatingAchievementEditorData = TRUE;

        using namespace ra::bitwise_ops;
        if (ra::etoi(pCheevo->GetDirtyFlags() & Achievement::DirtyFlags::ID))
        {
            if (m_pSelectedAchievement->Category() == ra::etoi(AchievementSet::Type::Local))
                SetDlgItemTextA(m_hAchievementEditorDlg, IDC_RA_ACH_ID, "0");
            else
                SetDlgItemTextA(m_hAchievementEditorDlg, IDC_RA_ACH_ID, std::to_string(m_pSelectedAchievement->ID()).c_str());
        }
        if (ra::etoi(pCheevo->GetDirtyFlags() & Achievement::DirtyFlags::Points) && !bPointsSelected)
            SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_POINTS,
                           NativeStr(std::to_string(m_pSelectedAchievement->Points())).c_str());
        if (ra::etoi(pCheevo->GetDirtyFlags() & Achievement::DirtyFlags::Title) && !bTitleSelected)
            SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_TITLE,
                           NativeStr(m_pSelectedAchievement->Title()).c_str());
        if (ra::etoi(pCheevo->GetDirtyFlags() & Achievement::DirtyFlags::Description) && !bDescSelected)
            SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_DESC,
                           NativeStr(m_pSelectedAchievement->Description()).c_str());
        if (ra::etoi(pCheevo->GetDirtyFlags() & Achievement::DirtyFlags::Author))
            SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR,
                           NativeStr(m_pSelectedAchievement->Author()).c_str());
        if (ra::etoi(pCheevo->GetDirtyFlags() & Achievement::DirtyFlags::Badge))
        {
            SetDlgItemText(m_hAchievementEditorDlg, IDC_RA_BADGENAME,
                           NativeStr(m_pSelectedAchievement->BadgeImageURI()).c_str());
            UpdateBadge(m_pSelectedAchievement->BadgeImageURI());
        }

        const unsigned int nGrp = GetSelectedConditionGroup();

        if (ra::etoi(pCheevo->GetDirtyFlags() & Achievement::DirtyFlags::Conditions))
        {
            HWND hCondList = GetDlgItem(m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS);
            if (hCondList != nullptr)
            {
                if (ListView_GetItemCount(hCondList) != ra::to_signed(m_pSelectedAchievement->NumConditions(nGrp)))
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
                        UpdateCondition(hCondList, item, Cond, m_pSelectedAchievement->GetConditionHitCount(nGrp, i));
                    }
                }
            }
        }

        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_AUTHOR), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_ID), FALSE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_TITLE), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_DESC), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_POINTS), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ADDCOND),
                     m_pSelectedAchievement->NumConditions(nGrp) < MAX_CONDITIONS);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_COPYCOND), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_DELETECOND), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_LBX_CONDITIONS), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_ACH_GROUP), TRUE);
        EnableWindow(GetDlgItem(m_hAchievementEditorDlg, IDC_RA_PROGRESSINDICATORS), FALSE);

        m_bPopulatingAchievementEditorData = FALSE;
    }

    // InvalidateRect( hList, nullptr, TRUE );
    // UpdateWindow( hList );
    // RedrawWindow( hList, nullptr, nullptr, RDW_INVALIDATE );
}

inline std::string& Dlg_AchievementEditor::LbxDataAt(unsigned int nRow, CondSubItems nCol) noexcept
{
    return m_lbxData.at(nRow).at(ra::etoi(nCol));
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

size_t Dlg_AchievementEditor::GetSelectedConditionGroup() const noexcept
{
    HWND hList = GetDlgItem(g_AchievementEditorDialog.GetHWND(), IDC_RA_ACH_GROUP);
    const int nSel = ListBox_GetCurSel(hList);
    if (nSel == LB_ERR)
    {
        OutputDebugString(TEXT("ListBox_GetCurSel returning LB_ERR\n"));
        return 0;
    }

    return ra::to_unsigned(nSel);
}

void Dlg_AchievementEditor::SetSelectedConditionGroup(size_t nGrp) const noexcept
{
    HWND hList = GetDlgItem(g_AchievementEditorDialog.GetHWND(), IDC_RA_ACH_GROUP);
    ListBox_SetCurSel(hList, ra::to_signed(nGrp));
}

void BadgeNames::FetchNewBadgeNamesThreaded() { RAWeb::CreateThreadedHTTPRequest(RequestBadgeIter); }

void BadgeNames::OnNewBadgeNames(const rapidjson::Document& data)
{
    const unsigned int nLowerLimit = data["FirstBadge"].GetUint();
    const unsigned int nUpperLimit = data["NextBadge"].GetUint();

    SetWindowRedraw(m_hDestComboBox, FALSE);

    // Clean out cbo
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

    // Find buffer in the dropdown list
    if (g_AchievementEditorDialog.ActiveAchievement() != nullptr)
    {
        const int nSel = ComboBox_FindStringExact(
            m_hDestComboBox, 0, g_AchievementEditorDialog.ActiveAchievement()->BadgeImageURI().c_str());
        if (nSel != -1)
            ComboBox_SetCurSel(m_hDestComboBox, nSel); // Force select
    }
}

void BadgeNames::AddNewBadgeName(const char* pStr, bool bAndSelect)
{
    {
        const auto nSel{ComboBox_AddString(m_hDestComboBox, NativeStr(pStr).c_str())};
        if ((nSel == CB_ERR) || (nSel == CB_ERRSPACE))
        {
            ::MessageBox(::GetActiveWindow(),
                         _T("An error has occurred or there is insufficient space "
                            "to store the new string"),
                         _T("Error!"), MB_OK | MB_ICONERROR);
            return;
        }
    }

    if (bAndSelect)
        ComboBox_SelectString(m_hDestComboBox, 0, pStr);
}

void GenerateResizes(HWND hDlg)
{
    RARect windowRect;
    GetWindowRect(hDlg, &windowRect);
    pDlgAchEditorMin.x = windowRect.Width();
    pDlgAchEditorMin.y = windowRect.Height();

    using AlignType = ResizeContent::AlignType;
    vDlgAchEditorResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_LBX_CONDITIONS), AlignType::BottomRight, true);
    vDlgAchEditorResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_ACH_GROUP), AlignType::Bottom, true);

    vDlgAchEditorResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_CHK_ACH_PAUSE_ON_RESET), AlignType::Right, false);
    vDlgAchEditorResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_CHK_ACH_PAUSE_ON_TRIGGER), AlignType::Right, false);
    vDlgAchEditorResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_CHK_ACH_ACTIVE), AlignType::Right, false);
    vDlgAchEditorResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_CHK_SHOW_DECIMALS), AlignType::BottomRight, false);

    vDlgAchEditorResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_ACH_ADDGROUP), AlignType::Bottom, false);
    vDlgAchEditorResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_ACH_DELGROUP), AlignType::Bottom, false);
    vDlgAchEditorResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_ADDCOND), AlignType::Bottom, false);
    vDlgAchEditorResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_DELETECOND), AlignType::Bottom, false);
    vDlgAchEditorResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_COPYCOND), AlignType::Bottom, false);
    vDlgAchEditorResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_PASTECOND), AlignType::Bottom, false);
    vDlgAchEditorResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_MOVECONDUP), AlignType::Bottom, false);
    vDlgAchEditorResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_MOVECONDDOWN), AlignType::Bottom, false);

    vDlgAchEditorResize.emplace_back(::GetDlgItem(hDlg, IDCLOSE), AlignType::BottomRight, false);
}
