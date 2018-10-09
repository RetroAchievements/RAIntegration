#include "RA_Dlg_MemBookmark.h"

#include <fstream>
#include <memory>
#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_GameData.h"
#include "RA_Dlg_Memory.h"

#include <CommDlg.h>

Dlg_MemBookmark g_MemBookmarkDialog;
std::vector<ResizeContent> vDlgMemBookmarkResize;
POINT pDlgMemBookmarkMin;

WNDPROC EOldProcBM;
HWND g_hIPEEditBM;
int nSelItemBM;
int nSelSubItemBM;

namespace ra {

using ColumnTitles = std::array<LPCTSTR, 5>;
using ColumnWidths = std::array<int, 5>;

inline constexpr ColumnTitles COLUMN_TITLE{ _T("Description"), _T("Address"), _T("Value"), _T("Prev."), _T("Changes") };
inline constexpr ColumnWidths COLUMN_WIDTH{ 112, 64, 64, 64, 54 };
static_assert(is_same_size_v<ColumnTitles, ColumnWidths>);

} // namespace ra

_CONSTANT_VAR c_rgFileTypes{L"Text Document (*.txt)\x0" "*.txt\x0"
                             "JSON File (*.json)\x0" "*.json\x0"
                             "All files (*.*)\x0" "*.*\x0\x0"};


INT_PTR CALLBACK Dlg_MemBookmark::s_MemBookmarkDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return g_MemBookmarkDialog.MemBookmarkDialogProc(hDlg, uMsg, wParam, lParam);
}

long _stdcall EditProcBM(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
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
            lvDispinfo.hdr.code = LVN_ENDLABELEDIT;
            lvDispinfo.item.mask = LVIF_TEXT;
            lvDispinfo.item.iItem = nSelItemBM;
            lvDispinfo.item.iSubItem = nSelSubItemBM;
            lvDispinfo.item.pszText = nullptr;

            wchar_t sEditText[256];
            GetWindowTextW(hwnd, sEditText, 256);
            g_MemBookmarkDialog.Bookmarks()[nSelItemBM]->SetDescription(sEditText);

            HWND hList = GetDlgItem(g_MemBookmarkDialog.GetHWND(), IDC_RA_LBX_ADDRESSES);

            //	the LV ID and the LVs Parent window's HWND
            SendMessage(GetParent(hList), WM_NOTIFY, static_cast<WPARAM>(IDC_RA_LBX_ADDRESSES), reinterpret_cast<LPARAM>(&lvDispinfo));	//	##reinterpret? ##SD

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

    return CallWindowProc(EOldProcBM, hwnd, nMsg, wParam, lParam);
}

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
            GenerateResizes(hDlg);

            m_hMemBookmarkDialog = hDlg;
            hList = GetDlgItem(m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES);

            SetupColumns(hList);

            // Auto-import bookmark file when opening dialog
            if (g_pCurrentGameData->GetGameID() != 0)
            {
                std::wstring file;
                {
                    std::wostringstream oss;
                    oss << g_sHomeDir << RA_DIR_BOOKMARKS << g_pCurrentGameData->GetGameID() << "-Bookmarks.txt";
                    file = oss.str();
                }
                ImportFromFile(file);
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
                case ODA_DRAWENTIRE:

                    hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);

                    ListView_GetItemRect(hList, pdis->itemID, &rcBounds, LVIR_BOUNDS);
                    ListView_GetItemRect(hList, pdis->itemID, &rcLabel, LVIR_LABEL);
                    RECT rcCol(rcBounds);
                    rcCol.right = rcCol.left + ListView_GetColumnWidth(hList, 0);

                    // Draw Item Label - Column 0
                    wchar_t buffer[512];
                    if (m_vBookmarks[pdis->itemID]->Decimal())
                        swprintf_s(buffer, 512, L"(D)%s", m_vBookmarks[pdis->itemID]->Description().c_str());
                    else
                        swprintf_s(buffer, 512, L"%s", m_vBookmarks[pdis->itemID]->Description().c_str());

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

                        if (m_vBookmarks[pdis->itemID]->Frozen())
                            color = RGB(255, 255, 160);
                        else
                            color = GetSysColor(COLOR_WINDOW);

                        HBRUSH hBrush = CreateSolidBrush(color);
                        SetBkColor(pdis->hDC, color);
                        FillRect(pdis->hDC, &rcBounds, hBrush);
                        DeleteObject(hBrush);
                    }

                    if (wcslen(buffer) > 0)
                    {
                        rcLabel.left += (offset / 2);
                        rcLabel.right -= offset;

                        DrawTextW(pdis->hDC, buffer, wcslen(buffer), &rcLabel, DT_SINGLELINE | DT_LEFT | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER | DT_END_ELLIPSIS);
                    }

                    // Draw Item Label for remaining columns
                    LV_COLUMN lvc;
                    lvc.mask = LVCF_FMT | LVCF_WIDTH;

                    for (size_t i = 1; ListView_GetColumn(hList, i, &lvc); ++i)
                    {
                        rcCol.left = rcCol.right;
                        rcCol.right += lvc.cx;

                        const auto eSubItem{ ra::itoe<SubItems>(i) };
                        switch (eSubItem)
                        {
                            case SubItems::Address:
                                swprintf_s(buffer, 512, L"%06x", m_vBookmarks[pdis->itemID]->Address());
                                break;
                            case SubItems::Value:
                                if (m_vBookmarks[pdis->itemID]->Decimal())
                                    swprintf_s(buffer, 512, L"%u", m_vBookmarks[pdis->itemID]->Value());
                                else
                                {
                                    switch (m_vBookmarks[pdis->itemID]->Type())
                                    {
                                        case 1: swprintf_s(buffer, 512, L"%02x", m_vBookmarks[pdis->itemID]->Value()); break;
                                        case 2: swprintf_s(buffer, 512, L"%04x", m_vBookmarks[pdis->itemID]->Value()); break;
                                        case 3: swprintf_s(buffer, 512, L"%08x", m_vBookmarks[pdis->itemID]->Value()); break;
                                    }
                                }
                                break;
                            case SubItems::Previous:
                                if (m_vBookmarks[pdis->itemID]->Decimal())
                                    swprintf_s(buffer, 512, L"%u", m_vBookmarks[pdis->itemID]->Previous());
                                else
                                {
                                    switch (m_vBookmarks[pdis->itemID]->Type())
                                    {
                                        case 1: swprintf_s(buffer, 512, L"%02x", m_vBookmarks[pdis->itemID]->Previous()); break;
                                        case 2: swprintf_s(buffer, 512, L"%04x", m_vBookmarks[pdis->itemID]->Previous()); break;
                                        case 3: swprintf_s(buffer, 512, L"%08x", m_vBookmarks[pdis->itemID]->Previous()); break;
                                    }
                                }
                                break;
                            case SubItems::Changes:
                                swprintf_s(buffer, 512, L"%u", m_vBookmarks[pdis->itemID]->Count());
                                break;
                            default:
                                swprintf_s(buffer, 512, L"");
                                break;
                        }

                        if (wcslen(buffer) == 0)
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

                        DrawTextW(pdis->hDC, buffer, wcslen(buffer), &rcLabel, nJustify | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_END_ELLIPSIS);
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
            switch (LOWORD(wParam))
            {
                case IDC_RA_LBX_ADDRESSES:
                    if (((LPNMHDR)lParam)->code == NM_CLICK)
                    {
                        hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);

                        nSelect = ListView_GetNextItem(hList, -1, LVNI_FOCUSED);

                        if (nSelect == -1)
                            break;
                    }
                    else if (((LPNMHDR)lParam)->code == NM_DBLCLK)
                    {
                        hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);

                        LPNMITEMACTIVATE pOnClick = (LPNMITEMACTIVATE)lParam;

                        using namespace ra::rel_ops;
                        if ((pOnClick->iItem != -1) && (pOnClick->iSubItem == SubItems::Desc))
                        {
                            nSelItemBM = pOnClick->iItem;
                            nSelSubItemBM = pOnClick->iSubItem;

                            EditLabel(pOnClick->iItem, pOnClick->iSubItem);
                        }
                        else if ((pOnClick->iItem != -1) && (pOnClick->iSubItem == SubItems::Address))
                        {
                            g_MemoryDialog.SetWatchingAddress(m_vBookmarks[pOnClick->iItem]->Address());
                            MemoryViewerControl::setAddress((m_vBookmarks[pOnClick->iItem]->Address() &
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
                    hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);
                    int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

                    if (nSel != -1)
                    {
                        while (nSel >= 0)
                        {
                            MemBookmark* pBookmark = m_vBookmarks[nSel];

                            // Remove from vector
                            m_vBookmarks.erase(m_vBookmarks.begin() + nSel);

                            // Remove from map
                            std::vector<const MemBookmark*> *pVector;
                            pVector = &m_BookmarkMap.find(pBookmark->Address())->second;
                            pVector->erase(std::find(pVector->begin(), pVector->end(), pBookmark));
                            if (pVector->size() == 0)
                                m_BookmarkMap.erase(pBookmark->Address());

                            delete pBookmark;

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
                                m_vBookmarks[i]->SetFrozen(!m_vBookmarks[i]->Frozen());
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
                        for (MemBookmark* bookmark : m_vBookmarks)
                        {
                            idx++;

                            bookmark->ResetCount();
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
                                m_vBookmarks[i]->SetDecimal(!m_vBookmarks[i]->Decimal());
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
                    std::wstring file = ImportDialog();
                    if (file.length() > 0)
                        ImportFromFile(file);
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

void Dlg_MemBookmark::UpdateBookmarks(bool bForceWrite)
{
    if (!IsWindowVisible(m_hMemBookmarkDialog))
        return;

    if (m_vBookmarks.size() == 0)
        return;

    HWND hList = GetDlgItem(m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES);

    int index = 0;
    for (MemBookmark* bookmark : m_vBookmarks)
    {
        if (bookmark->Frozen() && !bForceWrite)
        {
            WriteFrozenValue(*bookmark);
            index++;
            continue;
        }

        unsigned int mem_value = GetMemory(bookmark->Address(), bookmark->Type());

        if (bookmark->Value() != mem_value)
        {
            bookmark->SetPrevious(bookmark->Value());
            bookmark->SetValue(mem_value);
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
    if (m_vBookmarks.size() == 0)
        return;

    HWND hList = GetDlgItem(m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES);
    if (hList == nullptr)
        return;

    ListView_DeleteAllItems(hList);
    m_nNumOccupiedRows = 0;

    for (auto it = m_vBookmarks.cbegin(); it != m_vBookmarks.cend(); ++it)
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

void Dlg_MemBookmark::SetupColumns(HWND hList)
{
    //	Remove all columns,
    while (ListView_DeleteColumn(hList, 0)) {}

    //	Remove all data.
    ListView_DeleteAllItems(hList);
    auto idx{ 0 };
    for (auto& sColTitle : ra::COLUMN_TITLE)
    {
        ra::tstring tszText{ sColTitle };
        LV_COLUMN col
        {
            col.mask       = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT,
            col.fmt        = LVCFMT_CENTER | LVCFMT_FIXED_WIDTH,
            col.cx         = ra::COLUMN_WIDTH.at(idx),
            col.pszText    = tszText.data(),
            col.cchTextMax = 255,
            col.iSubItem   = idx
        };

        if (idx == (ra::to_signed(ra::COLUMN_TITLE.size()) - 1))
            col.fmt |= LVCFMT_FILL;

        ListView_InsertColumn(hList, idx, &col);
        idx++;
    }
    m_nNumOccupiedRows = 0;

#if _WIN32_WINNT >= _WIN32_WINNT_LONGHORN
    if (ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER) != 0UL)
        ::MessageBox(::GetActiveWindow(), _T("The styles specified could not be set."), _T("Error!"), MB_OK | MB_ICONERROR);
#endif // _WIN32_WINNT >= _WIN32_WINNT_LONGHORN
}

void Dlg_MemBookmark::AddAddress()
{
    if (g_pCurrentGameData->GetGameID() == 0)
        return;

    MemBookmark* NewBookmark = new MemBookmark();

    // Fetch Memory Address from Memory Inspector
    TCHAR buffer[256];
    GetDlgItemText(g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, buffer, 256);
    unsigned int nAddr = strtoul(ra::Narrow(buffer).c_str(), nullptr, 16);
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
        NewBookmark->SetDescription(ra::Widen(pSavedNote->Note()).c_str());

    // Add Bookmark to vector and map
    AddBookmark(NewBookmark);
    AddBookmarkMap(NewBookmark);

    PopulateList();
}

void Dlg_MemBookmark::ClearAllBookmarks()
{
    while (m_vBookmarks.size() > 0)
    {
        MemBookmark* pBookmark = m_vBookmarks[0];

        // Remove from vector
        m_vBookmarks.erase(m_vBookmarks.begin());

        // Remove from map
        std::vector<const MemBookmark*> *pVector;
        pVector = &m_BookmarkMap.find(pBookmark->Address())->second;
        pVector->erase(std::find(pVector->begin(), pVector->end(), pBookmark));
        if (pVector->size() == 0)
            m_BookmarkMap.erase(pBookmark->Address());

        delete pBookmark;
    }

    ListView_DeleteAllItems(GetDlgItem(m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES));
}

void Dlg_MemBookmark::WriteFrozenValue(const MemBookmark & Bookmark)
{
    if (!Bookmark.Frozen())
        return;

    unsigned int addr{};
    unsigned int width{};
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

unsigned int Dlg_MemBookmark::GetMemory(unsigned int nAddr, int type)
{
    unsigned int mem_value{};

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
        MessageBox(nullptr, _T("No bookmarks to save: please create a bookmark before attempting to save."),
                   _T("Error!"), MB_OK);
        return;
    }

    constexpr auto BUF_SIZE{ 1024UL };
    OPENFILENAMEW ofn{};
    ofn.lStructSize  = static_cast<DWORD>(sizeof(OPENFILENAMEW));
    ofn.hwndOwner    = m_hMemBookmarkDialog;
    ofn.lpstrFilter  = c_rgFileTypes;
    ofn.nFilterIndex = 1UL;
    ofn.nMaxFile     = BUF_SIZE + 16UL;
    ofn.lpstrTitle   = L"Save Bookmark File...";
    ofn.Flags        = OFN_ENABLESIZING | OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"txt"; 

    auto PathTooLong =[]() noexcept
    {
        MessageBox(nullptr, _T("Path to file is too long, it needs to be less than 1023 characters!"), _T("Error!"),
                    MB_OK | MB_ICONERROR);
    };

    wchar_t buf[BUF_SIZE + 16UL]{};
    {
        std::wstring sDefaultFilename;
        {
            std::wostringstream oss;
            oss << g_pCurrentGameData->GetGameID() << L"-Bookmarks.txt";
            sDefaultFilename = oss.str();
        }
        swprintf_s(buf, L"%s", sDefaultFilename.c_str());

        if (static_cast<DWORD>(std::wcslen(buf)) > ofn.nMaxFile)
        {
            PathTooLong();
            return;
        }
        ofn.lpstrFile = buf;
    }

    std::wstring sFilePath{ g_sHomeDir };
    sFilePath += RA_DIR_BOOKMARKS;
    if (sFilePath.length() > (ofn.nMaxFile - static_cast<DWORD>(std::wcslen(buf))))
    {
        PathTooLong();
        return;
    }
    ofn.lpstrInitialDir = sFilePath.c_str();

    if (::GetSaveFileNameW(&ofn) == 0)
        return;

    rapidjson::Document doc;
    auto& allocator{ doc.GetAllocator() };
    doc.SetObject();

    rapidjson::Value bookmarks{ rapidjson::kArrayType };

    for (auto bookmark : m_vBookmarks)
    {
        rapidjson::Value item{ rapidjson::kObjectType };
        std::string str{ ra::Narrow(bookmark->Description()) };
        rapidjson::Value s{ str.c_str(), allocator };

        item.AddMember("Description", s, allocator);
        item.AddMember("Address", bookmark->Address(), allocator);
        item.AddMember("Type", bookmark->Type(), allocator);
        item.AddMember("Decimal", bookmark->Decimal(), allocator);
        bookmarks.PushBack(item, allocator);
    }
    doc.AddMember("Bookmarks", bookmarks, allocator);

    swprintf_s(buf, L"%s", ofn.lpstrFile);
    _WriteBufferToFile(buf, doc);
}

void Dlg_MemBookmark::ImportFromFile(std::wstring sFilename) 
{
    std::ifstream ifile{ sFilename };
    if (!ifile.is_open())
    {
        char buf[2048U]{};
        strerror_s(buf, errno);
        RA_LOG("%s: %s", ra::Narrow(sFilename).c_str(), buf);
        return;
    }

    rapidjson::Document doc;
    rapidjson::IStreamWrapper isw{ ifile };
    doc.ParseStream(isw);

    if (doc.HasParseError())
    {
        ASSERT(" !Invalid Bookmark File...");
        MessageBox(nullptr, _T("Could not load properly. Invalid Bookmark file."), _T("Error"), MB_OK | MB_ICONERROR);
        return;
    }

    if (doc.HasMember("Bookmarks"))
    {
        ClearAllBookmarks();

        const auto& BookmarksData{ doc["Bookmarks"] };
        for (const auto& bmData : BookmarksData.GetArray())
        {
            auto NewBookmark{ new MemBookmark };
            NewBookmark->SetDescription(ra::Widen(bmData["Description"].GetString()));

            NewBookmark->SetAddress(bmData["Address"].GetUint());
            NewBookmark->SetType(bmData["Type"].GetInt());
            NewBookmark->SetDecimal(bmData["Decimal"].GetBool());

            NewBookmark->SetValue(GetMemory(NewBookmark->Address(), NewBookmark->Type()));
            NewBookmark->SetPrevious(NewBookmark->Value());

            AddBookmark(NewBookmark);
            AddBookmarkMap(NewBookmark);
        }

        if (!m_vBookmarks.empty())
            PopulateList();
    }

}

std::wstring Dlg_MemBookmark::ImportDialog()
{
    if (g_pCurrentGameData->GetGameID() == 0)
    {
        MessageBox(nullptr, _T("ROM not loaded: please load a ROM first!"), _T("Error!"), MB_OK);
        return L"";
    }
    
    constexpr auto BUF_SIZE{ 1024UL };
    
    OPENFILENAMEW ofn{};
    ofn.lStructSize  = static_cast<DWORD>(sizeof(OPENFILENAMEW));
    ofn.hwndOwner    = m_hMemBookmarkDialog;
    ofn.lpstrFilter  = c_rgFileTypes;
    ofn.nFilterIndex = 1UL;
    ofn.lpstrTitle   = L"Import Bookmark File...";
    ofn.nMaxFile     = BUF_SIZE + 16UL;
    ofn.Flags        = OFN_ENABLESIZING | OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt  = L"txt";

    auto PathTooLong =[]() noexcept
    {
        MessageBox(nullptr, _T("Path to file is too long, it needs to be less than 1023 characters!"), _T("Error!"),
                   MB_OK | MB_ICONERROR);
    };

    wchar_t buf[BUF_SIZE + 16UL]{};
    {
        std::wstring sDefaultFilename;
        {
            std::wostringstream oss;
            oss << g_pCurrentGameData->GetGameID() << L"-Bookmarks.txt";
            sDefaultFilename = oss.str();
        }   
        
        if (swprintf_s(buf, L"%s", sDefaultFilename.c_str()) == -1)
          return L"";
        if (static_cast<DWORD>(std::wcslen(buf)) > ofn.nMaxFile)
        {
            PathTooLong();
            return L"";
        }
        ofn.lpstrFile = buf;
    }    

    std::wstring sFilePath{ g_sHomeDir };
    sFilePath += RA_DIR_BOOKMARKS;
    if (sFilePath.length() > (ofn.nMaxFile - static_cast<DWORD>(std::wcslen(buf))))
    {
        PathTooLong();
        return L"";
    }
    ofn.lpstrInitialDir = sFilePath.c_str();

    if (::GetOpenFileNameW(&ofn) == 0)
        return L"";
    swprintf_s(buf, L"%s", ofn.lpstrFile);
    return buf;
}

void Dlg_MemBookmark::OnLoad_NewRom()
{
    if (::GetDlgItem(m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES) != nullptr)
    {
        ClearAllBookmarks();

        std::wstring file;
        {
            std::wostringstream oss;
            oss << g_sHomeDir << RA_DIR_BOOKMARKS << g_pCurrentGameData->GetGameID() << L"-Bookmarks.txt";
            file = oss.str();
        }
        ImportFromFile(file);
    }
}

void Dlg_MemBookmark::GenerateResizes(HWND hDlg)
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
    };

    SendMessage(g_hIPEEditBM, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SetWindowText(g_hIPEEditBM, NativeStr(m_vBookmarks[nItem]->Description()).c_str());

    SendMessage(g_hIPEEditBM, EM_SETSEL, 0, -1);
    SetFocus(g_hIPEEditBM);
    EOldProcBM = (WNDPROC)SetWindowLong(g_hIPEEditBM, GWL_WNDPROC, (LONG)EditProcBM);

    return TRUE;
}
