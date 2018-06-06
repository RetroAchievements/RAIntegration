#include "RA_Dlg_MemBookmark.h"

#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_GameData.h"
#include "RA_Dlg_Memory.h"
#include "RA_MemManager.h"

#include <strsafe.h>
#include <atlbase.h> // CComPtr
#include <mutex>

Dlg_MemBookmark g_MemBookmarkDialog;
std::vector<ResizeContent> vDlgMemBookmarkResize;
POINT pDlgMemBookmarkMin;

WNDPROC EOldProcBM;
HWND g_hIPEEditBM;
int nSelItemBM;
int nSelSubItemBM;

namespace {
const char* COLUMN_TITLE[] ={ "Description", "Address", "Value", "Prev.", "Changes" };
const int COLUMN_WIDTH[] ={ 112, 64, 64, 64, 54 };
static_assert(SIZEOF_ARRAY(COLUMN_TITLE) == SIZEOF_ARRAY(COLUMN_WIDTH), "Must match!");
}

inline constexpr std::array<COMDLG_FILTERSPEC, 1> c_rgFileTypes{ {L"Text Document (*.txt)", L"*.txt"} };


enum BookmarkSubItems
{
    CSI_DESC,
    CSI_ADDRESS,
    CSI_VALUE,
    CSI_PREVIOUS,
    CSI_CHANGES,

    NumColumns
};

// This macro helps me identify that params are used as intended during code
// analysis, function behavior is mostly handled in the STL
_Use_decl_annotations_
INT_PTR CALLBACK Dlg_MemBookmark::s_MemBookmarkDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return g_MemBookmarkDialog.MemBookmarkDialogProc(hDlg, uMsg, wParam, lParam);
}

_NODISCARD LRESULT CALLBACK EditProcBM(_In_ HWND hwnd, _In_ UINT nMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    switch (nMsg)
    {
        case WM_DESTROY:
            g_hIPEEditBM = nullptr;
            break;

        case WM_KILLFOCUS:
        {
            LV_DISPINFO lvDispinfo;
            ZeroMemory(&lvDispinfo, sizeof(LV_DISPINFO));
            lvDispinfo.hdr.hwndFrom = hwnd;
            lvDispinfo.hdr.idFrom = GetDlgCtrlID(hwnd);
            // warning C26454: Arithmetic overflow: '-' operation produces a
            // negative unsigned result at compile time (io.5).

            // Not really sure if these codes even have any effect
#pragma warning(disable : 26454)
            lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
#pragma warning(default : 26454)

            lvDispinfo.item.mask = LVIF_TEXT;
            lvDispinfo.item.iItem = nSelItemBM;
            lvDispinfo.item.iSubItem = nSelSubItemBM;
            lvDispinfo.item.pszText = nullptr;

            wchar_t sEditText[256];
            GetWindowTextW(hwnd, sEditText, 256);
            g_MemBookmarkDialog.Bookmarks().at(nSelItemBM).SetDescription(sEditText);

            HWND hList = GetDlgItem(g_MemBookmarkDialog.GetHWND(), IDC_RA_LBX_ADDRESSES);

            //	the LV ID and the LVs Parent window's HWND
            SendMessage(GetParent(hList), WM_NOTIFY, static_cast<WPARAM>(IDC_RA_LBX_ADDRESSES), reinterpret_cast<LPARAM>(&lvDispinfo));	//	##reinterpret? ##SD

            DestroyWindow(hwnd);
            break;
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

    return CallWindowProc(EOldProcBM, hwnd, nMsg, wParam, lParam);
}

_Use_decl_annotations_
INT_PTR Dlg_MemBookmark::MemBookmarkDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PMEASUREITEMSTRUCT pmis;
    PDRAWITEMSTRUCT pdis;
    int nSelect;
    HWND hList;
    int offset = 2;

    RECT rcBounds, rcLabel;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            // "in" parameters do not leave functions

            GenerateResizes(hDlg);
            m_hMemBookmarkDialog = hDlg;
            hList = GetDlgItem(m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES);

            SetupColumns(hList);

            // Auto-import bookmark file when opening dialog
            if (g_pCurrentGameData->GetGameID() != 0)
            {
                // Why is SAL complaining about this? 
                // Well RAII does require named return optimization, resource is acquired at initialization

                // It doesn't like the to_string
                std::ostringstream file;
                file << RA_DIR_BOOKMARKS << g_pCurrentGameData->GetGameID() << "-Bookmarks.txt";
                auto myStr{ file.str() };
                ImportFromFile(std::move(myStr));
            }

            RestoreWindowPosition(hDlg, "Memory Bookmarks", true, false);
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
            GetWindowRect(hDlg, &winRect);

            for (ResizeContent content : vDlgMemBookmarkResize)
                content.Resize(winRect.Width(), winRect.Height());

            //InvalidateRect( hDlg, nullptr, TRUE );
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
            if (pdis->itemID == -1)
                break;


            switch (pdis->itemAction)
            {
                case ODA_SELECT:
                    _FALLTHROUGH;
                    // no wonder
                case ODA_DRAWENTIRE:
                {

                    hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);

                    ListView_GetItemRect(hList, pdis->itemID, &rcBounds, LVIR_BOUNDS);
                    ListView_GetItemRect(hList, pdis->itemID, &rcLabel, LVIR_LABEL);
                    RECT rcCol(rcBounds);
                    rcCol.right = rcCol.left + ListView_GetColumnWidth(hList, 0);

                    // Draw Item Label - Column 0
                    // It's not understanding wchar_t[]
                    std::wstring buffer;
                    buffer.reserve(512);

                    // I'm almost certain it's because it's a pointer
                    if (!m_vBookmarks.at(pdis->itemID).Description().empty())
                    {
                        if (m_vBookmarks.at(pdis->itemID).Decimal())
                        {
                            auto temp = m_vBookmarks.at(pdis->itemID).Description();
                            swprintf_s(buffer.data(), 512, L"(D)%s",
                                m_vBookmarks.at(pdis->itemID).Description().c_str());
                        }
                        else
                        {
                            // I can actually see what it is now! - Samer
                            swprintf_s(buffer.data(), 512, L"%s", m_vBookmarks.at(pdis->itemID).Description().c_str());
                        }
                    }

                    if (pdis->itemState & ODS_SELECTED)
                    {
                        SetTextColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                        SetBkColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
                        FillRect(pdis->hDC, &rcBounds, GetSysColorBrush(COLOR_HIGHLIGHT));
                    }
                    else
                    {
                        SetTextColor(pdis->hDC, GetSysColor(COLOR_WINDOWTEXT));

                        COLORREF color;

                        if (m_vBookmarks.at(pdis->itemID).Frozen())
                            color = RGB(255, 255, 160);
                        else
                            color = GetSysColor(COLOR_WINDOW);

                        HBRUSH hBrush = CreateSolidBrush(color);
                        SetBkColor(pdis->hDC, color);
                        FillRect(pdis->hDC, &rcBounds, hBrush);
                        DeleteObject(hBrush);
                    }

                    if (wcslen(buffer.data()) > 0)
                    {
                        rcLabel.left += (offset / 2);
                        rcLabel.right -= offset;

                        DrawTextW(pdis->hDC, buffer.data(), wcslen(buffer.data()), &rcLabel, DT_SINGLELINE | DT_LEFT | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER | DT_END_ELLIPSIS);
                    }

                    // Draw Item Label for remaining columns
                    LV_COLUMN lvc;
                    lvc.mask = LVCF_FMT | LVCF_WIDTH;

                    for (size_t i = 1; ListView_GetColumn(hList, i, &lvc); ++i)
                    {
                        rcCol.left = rcCol.right;
                        rcCol.right += lvc.cx;

                        switch (i)
                        {
                            case CSI_ADDRESS:
                                swprintf_s(buffer.data(), 512, L"%06x", m_vBookmarks.at(pdis->itemID).Address());
                                break;
                            case CSI_VALUE:
                                if (m_vBookmarks.at(pdis->itemID).Decimal())
                                    swprintf_s(buffer.data(), 512, L"%u", m_vBookmarks.at(pdis->itemID).Value());
                                else
                                {
                                    switch (m_vBookmarks.at(pdis->itemID).Type())
                                    {
                                        case 1: swprintf_s(buffer.data(), 512, L"%02x", m_vBookmarks.at(pdis->itemID).Value()); break;
                                        case 2: swprintf_s(buffer.data(), 512, L"%04x", m_vBookmarks.at(pdis->itemID).Value()); break;
                                        case 3: swprintf_s(buffer.data(), 512, L"%08x", m_vBookmarks.at(pdis->itemID).Value()); break;
                                    }
                                }
                                break;
                            case CSI_PREVIOUS:
                                if (m_vBookmarks.at(pdis->itemID).Decimal())
                                    swprintf_s(buffer.data(), 512, L"%u", m_vBookmarks.at(pdis->itemID).Previous());
                                else
                                {
                                    switch (m_vBookmarks.at(pdis->itemID).Type())
                                    {
                                        case 1: swprintf_s(buffer.data(), 512, L"%02x", m_vBookmarks.at(pdis->itemID).Previous()); break;
                                        case 2: swprintf_s(buffer.data(), 512, L"%04x", m_vBookmarks.at(pdis->itemID).Previous()); break;
                                        case 3: swprintf_s(buffer.data(), 512, L"%08x", m_vBookmarks.at(pdis->itemID).Previous()); break;
                                    }
                                }
                                break;
                            case CSI_CHANGES:
                                swprintf_s(buffer.data(), 512, L"%d", m_vBookmarks.at(pdis->itemID).Count());
                                break;
                            default:
                                swprintf_s(buffer.data(), 512, L"");
                                break;
                        }

                        if (wcslen(buffer.data()) == 0)
                            continue;

                        UINT nJustify = DT_LEFT;
                        switch (lvc.fmt & LVCFMT_JUSTIFYMASK)
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

                        DrawTextW(pdis->hDC, buffer.data(), wcslen(buffer.data()), &rcLabel, nJustify | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_END_ELLIPSIS);
                    }

                    //if (pdis->itemState & ODS_SELECTED) //&& (GetFocus() == this)
                    //	DrawFocusRect(pdis->hDC, &rcBounds);
                }

                break;

                case ODA_FOCUS:
                    break;
            }
            return TRUE;
        }

        case WM_NOTIFY:
        {
            switch (LOWORD(wParam))
            {
                case IDC_RA_LBX_ADDRESSES:
#pragma warning(disable : 26454)
                    if (((LPNMHDR)lParam)->code == NM_CLICK)
#pragma warning(default : 26454)
                    {
                        hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);

                        nSelect = ListView_GetNextItem(hList, -1, LVNI_FOCUSED);

                        if (nSelect == -1)
                            break;
                    }
#pragma warning(disable : 26454)
                    if (((LPNMHDR)lParam)->code == NM_DBLCLK)
#pragma warning(default : 26454)
                    {
                        hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);

                        LPNMITEMACTIVATE pOnClick = (LPNMITEMACTIVATE)lParam;

                        if (pOnClick->iItem != -1 && pOnClick->iSubItem == CSI_DESC)
                        {
                            nSelItemBM = pOnClick->iItem;
                            nSelSubItemBM = pOnClick->iSubItem;

                            const auto myResult = EditLabel(pOnClick->iItem, pOnClick->iSubItem);
                        }
                        else if (pOnClick->iItem != -1 && pOnClick->iSubItem == CSI_ADDRESS)
                        {
                            g_MemoryDialog.SetWatchingAddress(m_vBookmarks.at(pOnClick->iItem).Address());
                            MemoryViewerControl::setAddress((m_vBookmarks.at(pOnClick->iItem).Address() &
                                ~(0xf)) - ((int)(MemoryViewerControl::m_nDisplayedLines / 2) << 4) + (0x50));
                        }
                    }
            }
            return TRUE;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                case IDCLOSE:
                case IDCANCEL:
                    EndDialog(hDlg, true);
                    return TRUE;

                case IDC_RA_ADD_BOOKMARK:
                {
                    if (g_MemoryDialog.GetHWND() != nullptr)
                        AddAddress();

                    return TRUE;
                }
                case IDC_RA_DEL_BOOKMARK:
                {
                    HWND hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);
                    int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

                    if (nSel != -1)
                    {
                        while (nSel >= 0)
                        {
                            const auto& bookmark{ m_vBookmarks.at(nSel) };

                            // Remove from vector
                            const auto _ = m_vBookmarks.erase(std::next(m_vBookmarks.begin(), nSel));

                            // Remove from map

                            auto& pVector = m_BookmarkMap.find(bookmark.Address())->second;
                            const auto __ = pVector.erase(std::find(pVector.begin(), pVector.end(), bookmark));
                            if (pVector.size() == 0)
                                m_BookmarkMap.erase(bookmark.Address());

                            ListView_DeleteItem(hList, nSel);

                            nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                        }

                        InvalidateRect(hList, nullptr, FALSE);
                    }

                    return TRUE;
                }
                case IDC_RA_FREEZE:
                {
                    if (m_vBookmarks.size() > 0)
                    {
                        hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);
                        unsigned int uSelectedCount = ListView_GetSelectedCount(hList);

                        if (uSelectedCount > 0)
                        {
                            for (int i = ListView_GetNextItem(hList, -1, LVNI_SELECTED); i >= 0; i = ListView_GetNextItem(hList, i, LVNI_SELECTED))
                                m_vBookmarks.at(i).SetFrozen(!m_vBookmarks.at(i).Frozen());
                        }
                        ListView_SetItemState(hList, -1, LVIF_STATE, LVIS_SELECTED);
                    }
                    return TRUE;
                }
                case IDC_RA_CLEAR_CHANGE:
                {
                    if (m_vBookmarks.size() > 0)
                    {
                        hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);
                        int idx = -1;
                        for (auto& bookmark : m_vBookmarks)
                        {
                            idx++;

                            bookmark.ResetCount();
                        }

                        InvalidateRect(hList, nullptr, FALSE);
                    }

                    return TRUE;
                }
                case IDC_RA_DECIMALBOOKMARK:
                {
                    if (m_vBookmarks.size() > 0)
                    {
                        hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);
                        unsigned int uSelectedCount = ListView_GetSelectedCount(hList);

                        if (uSelectedCount > 0)
                        {
                            for (int i = ListView_GetNextItem(hList, -1, LVNI_SELECTED); i >= 0; i = ListView_GetNextItem(hList, i, LVNI_SELECTED))
                                m_vBookmarks.at(i).SetDecimal(!m_vBookmarks.at(i).Decimal());
                        }
                        ListView_SetItemState(hList, -1, LVIF_STATE, LVIS_SELECTED);
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
                    if (auto file = ImportDialog(); !file.empty())
                        ImportFromFile(std::move(file));
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
    return(g_MemBookmarkDialog.GetHWND() != nullptr) && (IsWindowVisible(g_MemBookmarkDialog.GetHWND()));
}

_Use_decl_annotations_
void Dlg_MemBookmark::UpdateBookmarks(bool bForceWrite)
{
    if (!IsWindowVisible(m_hMemBookmarkDialog))
        return;

    if (m_vBookmarks.size() == 0)
        return;

    HWND hList = GetDlgItem(m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES);

    int index = 0;
    for (auto& bookmark : m_vBookmarks)
    {
        if (bookmark.Frozen() && !bForceWrite)
        {
            WriteFrozenValue(bookmark);
            index++;
            continue;
        }

        unsigned int mem_value = GetMemory(bookmark.Address(), bookmark.Type());

        if (bookmark.Value() != mem_value)
        {
            bookmark.SetPrevious(bookmark.Value());
            bookmark.SetValue(mem_value);
            bookmark.IncreaseCount();

            RECT rcBounds;
            ListView_GetItemRect(hList, index, &rcBounds, LVIR_BOUNDS);
            InvalidateRect(hList, &rcBounds, FALSE);
        }

        index++;
    }
}

void Dlg_MemBookmark::PopulateList()
{
    if (m_vBookmarks.size() == 0)
        return;

    HWND hList = GetDlgItem(m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES);
    if (hList == nullptr)
        return;

    int topIndex = ListView_GetTopIndex(hList);
    ListView_DeleteAllItems(hList);
    m_nNumOccupiedRows = 0;

    for (auto& bookmark : m_vBookmarks)
    {
        LV_ITEM item;
        ZeroMemory(&item, sizeof(item));
        item.mask = LVIF_TEXT;
        item.cchTextMax = 256;
        item.iItem = m_nNumOccupiedRows;
        item.iSubItem = 0;
        item.iItem = ListView_InsertItem(hList, &item);

        ASSERT(item.iItem == m_nNumOccupiedRows);

        m_nNumOccupiedRows++;
    }

    ListView_EnsureVisible(hList, m_vBookmarks.size() - 1, FALSE); // Scroll to bottom.
    //ListView_EnsureVisible( hList, topIndex, TRUE ); // return to last position.
}

_Use_decl_annotations_
void Dlg_MemBookmark::SetupColumns(HWND hList)
{
    // Some of these functions return values so we have to make sure they don't leave the function

    //	Remove all columns,
    while (ListView_DeleteColumn(hList, 0)) {}

    //	Remove all data.
    const auto a = ListView_DeleteAllItems(hList);

    LV_COLUMN col;
    auto b = ZeroMemory(&col, sizeof(col));
    b = nullptr;

    for (size_t i = 0; i < NumColumns; ++i)
    {
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT;
        col.cx = COLUMN_WIDTH[i];
        tstring colTitle = NativeStr(COLUMN_TITLE[i]).c_str();
        col.pszText = const_cast<LPTSTR>(colTitle.c_str());
        col.cchTextMax = 255;
        col.iSubItem = i;

        col.fmt = LVCFMT_CENTER | LVCFMT_FIXED_WIDTH;
        if (i == NumColumns - 1)
            col.fmt |= LVCFMT_FILL;

        const auto c = ListView_InsertColumn(hList, i, (LPARAM)&col);
    }

    m_nNumOccupiedRows = 0;

    BOOL bSuccess = ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    bSuccess = ListView_EnableGroupView(hList, FALSE);

}

void Dlg_MemBookmark::AddAddress()
{
    if (g_pCurrentGameData->GetGameID() == 0)
        return;

    auto NewBookmark = std::make_unique<MemBookmark>();

    // Fetch Memory Address from Memory Inspector
    TCHAR buffer[256];
    GetDlgItemText(g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, buffer, 256);
    unsigned int nAddr = strtol(Narrow(buffer).c_str(), nullptr, 16);
    NewBookmark->SetAddress(nAddr);

    // Check Data Type
    if (SendDlgItemMessage(g_MemoryDialog.GetHWND(), IDC_RA_MEMVIEW8BIT, BM_GETCHECK, 0, 0) == BST_CHECKED)
        NewBookmark->SetType(1);
    else if (SendDlgItemMessage(g_MemoryDialog.GetHWND(), IDC_RA_MEMVIEW16BIT, BM_GETCHECK, 0, 0) == BST_CHECKED)
        NewBookmark->SetType(2);
    else
        NewBookmark->SetType(3);

    // Get Memory Value
    NewBookmark->SetValue(GetMemory(nAddr, NewBookmark->Type()));
    NewBookmark->SetPrevious(NewBookmark->Value());

    // Get Code Note and add as description
    const CodeNotes::CodeNoteObj* pSavedNote = g_MemoryDialog.Notes().FindCodeNote(nAddr);
    if ((pSavedNote != nullptr) && (pSavedNote->Note().length() > 0))
    {
        const auto wstr = Widen(pSavedNote->Note());
        NewBookmark->SetDescription(wstr);
    }

    // Add Bookmark to vector and map
    MemBookmark myClone;
    NewBookmark->Clone(myClone);

    AddBookmark(std::move(*NewBookmark.release()));
    AddBookmarkMap(std::move(myClone));

    PopulateList();
}

void Dlg_MemBookmark::ClearAllBookmarks() noexcept
{
    // no pointers no more, RAII style, still fast
    // If the bookmark map isn't empty then surely the vectors placed in them can't be empty, otherwise it's undefined
    if (!m_BookmarkMap.empty())
    {
        // Let vector destroy them, they aren't pointers no more
        m_vBookmarks.clear();

        // Clear the map
        m_BookmarkMap.clear();

    }

    ListView_DeleteAllItems(GetDlgItem(m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES));
}

_Use_decl_annotations_
void Dlg_MemBookmark::WriteFrozenValue(const MemBookmark& Bookmark)
{
    if (!Bookmark.Frozen())
        return;

    unsigned int addr{ 0U };
    unsigned int width{ 0U };
    int n;
    char c;

    switch (Bookmark.Type())
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

    char buffer[32];
    sprintf_s(buffer, sizeof(buffer), "%0*x", width, Bookmark.Value());

    for (unsigned int i = 0; i < strlen(buffer); i++)
    {
        c = buffer[i];
        n = (c >= 'a') ? (c - 'a' + 10) : (c - '0');
        MemoryViewerControl::editData(addr, (i % 2 != 0), n);

        if (i % 2 != 0)
            addr--;
    }
}

_Use_decl_annotations_
unsigned int Dlg_MemBookmark::GetMemory(unsigned int nAddr, int type)
{
    unsigned int mem_value{ 0U };

    switch (type)
    {
        case 1:
            mem_value = g_MemManager.ActiveBankRAMRead(nAddr, EightBit);
            break;
        case 2:
            mem_value = g_MemManager.ActiveBankRAMRead(nAddr, SixteenBit);
            break;
        case 3:
            mem_value = g_MemManager.ActiveBankRAMRead(nAddr, ThirtyTwoBit);
            break;
    }

    return mem_value;
}

void Dlg_MemBookmark::ExportJSON()
{
    if (g_pCurrentGameData->GetGameID() == 0)
    {
        MessageBox(nullptr, _T("ROM not loaded: please load a ROM first!"), _T("Error!"), MB_OK);
        return;
    }

    if (m_vBookmarks.size() == 0)
    {
        MessageBox(nullptr, _T("No bookmarks to save: please create a bookmark before attempting to save."), _T("Error!"), MB_OK);
        return;
    }

    std::string defaultDir = RA_DIR_BOOKMARKS;
    defaultDir.erase(0, 2); // Removes the characters (".\\")
    defaultDir = g_sHomeDir + defaultDir;

    CComPtr<IFileSaveDialog> pDlg;

    HRESULT hr;
    if (SUCCEEDED(hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pDlg))))
    {
        if (SUCCEEDED(hr = pDlg->SetFileTypes(c_rgFileTypes.size(), &c_rgFileTypes.front())))
        {
            std::ostringstream oss;
            oss << g_pCurrentGameData->GetGameID() << "-Bookmarks.txt";
            auto defaultFileName{ oss.str() };

            if (SUCCEEDED(hr = pDlg->SetFileName(Widen(defaultFileName).c_str())))
            {
                PIDLIST_ABSOLUTE pidl{ nullptr };
                if (SUCCEEDED(hr = SHParseDisplayName(Widen(defaultDir).c_str(), nullptr, &pidl, SFGAO_FOLDER, nullptr)))
                {
                    CComPtr<IShellItem> pItem;
                    SHCreateShellItem(nullptr, nullptr, pidl, &pItem);

                    if (SUCCEEDED(hr = pDlg->SetDefaultFolder(pItem)))
                    {
                        pDlg->SetDefaultExtension(L"txt");
                        if (SUCCEEDED(hr = pDlg->Show(nullptr)))
                        {
                            if (SUCCEEDED(hr = pDlg->GetResult(&pItem.p)))
                            {
                                LPWSTR pStr = nullptr;
                                if (SUCCEEDED(hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pStr)))
                                {
                                    Document doc;
                                    auto& allocator = doc.GetAllocator();
                                    doc.SetObject();

                                    Value bookmarks(kArrayType);

                                    for (auto& bookmark : m_vBookmarks)
                                    {
                                        Value item(kObjectType);

                                        oss.str("");
                                        oss << Narrow(bookmark.Description());
                                        // I may have done this, this needs to get fixed
                                        auto myStr{ oss.str() };
                                        Value s{ myStr.c_str(), allocator };

                                        item.AddMember("Description", s, allocator);
                                        item.AddMember("Address", bookmark.Address(), allocator);
                                        item.AddMember("Type", bookmark.Type(), allocator);
                                        item.AddMember("Decimal", bookmark.Decimal(), allocator);
                                        bookmarks.PushBack(item, allocator);
                                    }
                                    doc.AddMember("Bookmarks", bookmarks, allocator);

                                    _WriteBufferToFile(Narrow(pStr), doc);
                                    CoTaskMemFree(static_cast<LPVOID>(pStr));
                                    pStr = nullptr;
                                }

                                pItem.Release();
                                ILFree(pidl);
                            }
                        }
                    }
                }
            }
        }
        pDlg.Release();
    }
}

_Use_decl_annotations_
void Dlg_MemBookmark::ImportFromFile(std::string&& sFilename)
{
    // mf'er
    using FileH = std::unique_ptr<FILE, decltype(&std::fclose)>;

    if (FileH pFile{ std::fopen(sFilename.c_str(), "rb"), &std::fclose }; pFile == nullptr)
        return;
    else
    {
        Document doc;
        FileStream myStream{ pFile.get() };
        doc.ParseStream(myStream);

        if (doc.HasParseError())
            return;


        if (!doc.HasMember("Bookmarks"))
        {
            ASSERT(" !Invalid Bookmark File...");
            MessageBox(nullptr, _T("Could not load properly. Invalid Bookmark file."), _T("Error"), MB_OK | MB_ICONERROR);
            return;
        }

        ClearAllBookmarks();

        const Value& BookmarksData = doc["Bookmarks"];
        for (SizeType i = 0; i < BookmarksData.Size(); ++i)
        {
            auto NewBookmark = std::make_unique<MemBookmark>();

            wchar_t buffer[256];
            auto myStr{ Widen(BookmarksData[i]["Description"].GetString()) };
            swprintf_s(buffer, 256, L"%s", myStr.c_str());
            myStr = std::move(buffer); // it will invalidate it but not nullify
            NewBookmark->SetDescription(myStr);

            NewBookmark->SetAddress(BookmarksData[i]["Address"].GetUint());
            NewBookmark->SetType(BookmarksData[i]["Type"].GetInt());
            NewBookmark->SetDecimal(BookmarksData[i]["Decimal"].GetBool());

            NewBookmark->SetValue(GetMemory(NewBookmark->Address(), NewBookmark->Type()));
            NewBookmark->SetPrevious(NewBookmark->Value());

            MemBookmark myClone;
            NewBookmark->Clone(myClone);

            AddBookmark(std::move(myClone));
            // It refuses to come here, makes sense though
            
        }

        if (m_vBookmarks.size() > 0)
            PopulateList();
    }
}



std::string Dlg_MemBookmark::ImportDialog()
{
    std::string str;

    if (g_pCurrentGameData->GetGameID() == 0)
    {
        MessageBox(nullptr, _T("ROM not loaded: please load a ROM first!"), _T("Error!"), MB_OK);
        return str;
    }

    CComPtr<IFileOpenDialog> pDlg;

    HRESULT hr;
    if (SUCCEEDED(hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pDlg))))
    {
        if (SUCCEEDED(hr = pDlg->SetFileTypes(c_rgFileTypes.size(), &c_rgFileTypes.front())))
        {
            if (SUCCEEDED(hr = pDlg->Show(nullptr)))
            {
                CComPtr<IShellItem> pItem;
                if (SUCCEEDED(hr = pDlg->GetResult(&pItem)))
                {
                    LPWSTR pStr = nullptr;
                    if (SUCCEEDED(hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pStr)))
                    {
                        str = Narrow(pStr);
                        CoTaskMemFree(static_cast<LPVOID>(pStr));
                        pStr = nullptr;
                    }
                    pItem.Release();
                }
            }
        }
        pDlg.Release();
    }

    return str;
}

void Dlg_MemBookmark::AddBookmarkMap(MemBookmark&& bookmark) noexcept
{
    if (FindBookmark(bookmark.Address()) == end())
    {
        const auto _ = m_BookmarkMap.try_emplace(bookmark.Address(),
            MyBookmarks{});
    }

    auto& v = m_BookmarkMap.at(bookmark.Address());
    v.push_back(std::move(bookmark));
}

void Dlg_MemBookmark::OnLoad_NewRom()
{
    if (HWND hList = GetDlgItem(m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES); hList != nullptr)
    {
        ClearAllBookmarks();
        std::ostringstream file;
        file << RA_DIR_BOOKMARKS << g_pCurrentGameData->GetGameID() << "-Bookmarks.txt";
        auto myStr{ file.str() };
        ImportFromFile(std::move(myStr));
    }
}

_Use_decl_annotations_
void Dlg_MemBookmark::GenerateResizes(HWND& hDlg)
{
    RARect windowRect;
    GetWindowRect(hDlg, &windowRect);
    pDlgMemBookmarkMin.x = windowRect.Width();
    pDlgMemBookmarkMin.y = windowRect.Height();

    vDlgMemBookmarkResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES), ResizeContent::ALIGN_BOTTOM_RIGHT, TRUE));

    vDlgMemBookmarkResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_ADD_BOOKMARK), ResizeContent::ALIGN_RIGHT, FALSE));
    vDlgMemBookmarkResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_DEL_BOOKMARK), ResizeContent::ALIGN_RIGHT, FALSE));
    vDlgMemBookmarkResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_CLEAR_CHANGE), ResizeContent::ALIGN_RIGHT, FALSE));

    vDlgMemBookmarkResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_FREEZE), ResizeContent::ALIGN_BOTTOM_RIGHT, FALSE));
    vDlgMemBookmarkResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_DECIMALBOOKMARK), ResizeContent::ALIGN_BOTTOM_RIGHT, FALSE));
    vDlgMemBookmarkResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_SAVEBOOKMARK), ResizeContent::ALIGN_BOTTOM_RIGHT, FALSE));
    vDlgMemBookmarkResize.push_back(ResizeContent(hDlg,
        GetDlgItem(hDlg, IDC_RA_LOADBOOKMARK), ResizeContent::ALIGN_BOTTOM_RIGHT, FALSE));
}

_Use_decl_annotations_
BOOL Dlg_MemBookmark::EditLabel(int nItem, int nSubItem)
{
    HWND hList = GetDlgItem(g_MemBookmarkDialog.GetHWND(), IDC_RA_LBX_ADDRESSES);
    RECT rcSubItem;
    ListView_GetSubItemRect(hList, nItem, nSubItem, LVIR_BOUNDS, &rcSubItem);

    RECT rcOffset;
    GetWindowRect(hList, &rcOffset);
    rcSubItem.left += rcOffset.left;
    rcSubItem.right += rcOffset.left;
    rcSubItem.top += rcOffset.top;
    rcSubItem.bottom += rcOffset.top;

    int nHeight = rcSubItem.bottom - rcSubItem.top;
    int nWidth = rcSubItem.right - rcSubItem.left;

    ASSERT(g_hIPEEditBM == nullptr);
    if (g_hIPEEditBM) return FALSE;

    g_hIPEEditBM = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        _T("EDIT"),
        _T(""),
        WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | ES_WANTRETURN,
        rcSubItem.left, rcSubItem.top, nWidth, (int)(1.5f*nHeight),
        g_MemBookmarkDialog.GetHWND(),
        0,
        GetModuleHandle(nullptr),
        nullptr);

    if (g_hIPEEditBM == nullptr)
    {
        ASSERT(!"Could not create edit box!");
        MessageBox(nullptr, _T("Could not create edit box."), _T("Error"), MB_OK | MB_ICONERROR);
        return FALSE;
    }

    SetWindowFont(g_hIPEEditBM, GetStockFont(DEFAULT_GUI_FONT), TRUE);
    SetWindowText(g_hIPEEditBM, NativeStr(m_vBookmarks.at(nItem).Description()).c_str());


    Edit_SetSel(g_hIPEEditBM, 0, -1);
    SetFocus(g_hIPEEditBM);

    // LRESULT is a typedef of LONG_PTR
    EOldProcBM = reinterpret_cast<WNDPROC>(SetWindowLongPtr(g_hIPEEditBM, GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(EditProcBM)));

    return TRUE;
}

auto swap(MemBookmark& a, MemBookmark& b) noexcept->decltype(std::swap(a.m_nAddress, b.m_nAddress))
{
    std::swap(a.m_sDescription, b.m_sDescription);
    std::swap(a.m_nAddress, b.m_nAddress);
    std::swap(a.m_nType, b.m_nType);
    std::swap(a.m_sValue, b.m_sValue);
    std::swap(a.m_sPrevious, b.m_sPrevious);
    std::swap(a.m_nCount, b.m_nCount);
    std::swap(a.m_bFrozen, b.m_bFrozen);
    std::swap(a.m_bDecimal, b.m_bDecimal);
}
