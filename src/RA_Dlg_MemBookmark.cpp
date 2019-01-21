#include "RA_Dlg_MemBookmark.h"

#include "RA_Core.h"
#include "RA_Dlg_Memory.h"
#include "RA_Resource.h"

#include "ra_math.h"

#include "data\GameContext.hh"

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

inline constexpr ColumnTitles COLUMN_TITLE{_T("Description"), _T("Address"), _T("Value"), _T("Prev."), _T("Changes")};
inline constexpr ColumnWidths COLUMN_WIDTH{112, 64, 64, 64, 54};
static_assert(is_same_size_v<ColumnTitles, ColumnWidths>);

} // namespace ra

_CONSTANT_VAR c_rgFileTypes{
    L"Text Document (*.txt)\x0"
    "*.txt\x0"
    "JSON File (*.json)\x0"
    "*.json\x0"
    "All files (*.*)\x0"
    "*.*\x0\x0"};

_NODISCARD INT_PTR CALLBACK Dlg_MemBookmark::s_MemBookmarkDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return g_MemBookmarkDialog.MemBookmarkDialogProc(hDlg, uMsg, wParam, lParam);
}

LRESULT CALLBACK EditProcBM(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    const auto OnDestroy = []([[maybe_unused]] HWND) noexcept { g_hIPEEditBM = nullptr; };

    // hwndNewFocus -> wParam
    const auto OnKillFocus = [](HWND hwnd, [[maybe_unused]] HWND /*hwndNewFocus*/)
    {
        GSL_SUPPRESS_IO5
#pragma warning(suppress: 26454)
        NMHDR hdr{hwnd, ra::to_unsigned(GetDlgCtrlID(hwnd)), LVN_ENDLABELEDIT};

        LVITEM item{};
        item.mask = LVIF_TEXT;
        item.iItem = nSelItemBM;
        item.iSubItem = nSelSubItemBM;

        LV_DISPINFO lvDispinfo{hdr, item};
        {
            std::wstring sEditText(256, wchar_t());
            GetWindowTextW(hwnd, sEditText.data(), 256);
            g_MemBookmarkDialog.Bookmarks().at(nSelItemBM).SetDescription(sEditText);
        }

        HWND hList = GetDlgItem(g_MemBookmarkDialog.GetHWND(), IDC_RA_LBX_ADDRESSES);

        // the LV ID and the LVs Parent window's HWND
        FORWARD_WM_NOTIFY(GetParent(hList), IDC_RA_LBX_ADDRESSES, &lvDispinfo, ::SendMessage);

        DestroyWindow(hwnd);
    };

    // vk -> wParam
    const auto OnKeyDown = [](HWND hwnd, UINT vk, [[maybe_unused]] BOOL, [[maybe_unused]] int,
                              [[maybe_unused]] UINT) noexcept
    {
        if (vk == VK_RETURN || vk == VK_ESCAPE)
            DestroyWindow(hwnd); // Causing a killfocus :)
    };

    switch (nMsg)
    {
        HANDLE_MSG(hwnd, WM_DESTROY, OnDestroy);
        HANDLE_MSG(hwnd, WM_KILLFOCUS, OnKillFocus);
        HANDLE_MSG(hwnd, WM_KEYDOWN, OnKeyDown);
    }

    return CallWindowProc(EOldProcBM, hwnd, nMsg, wParam, lParam);
}

INT_PTR Dlg_MemBookmark::MemBookmarkDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto nSelect = 0;
    _CONSTANT_LOC offset = 2;

    RECT rcBounds{}, rcLabel{};
    // hwndFocus->wParam
    const auto OnInitDialog = [this](HWND hDlg, [[maybe_unused]] HWND /*hwndFocus*/,
                                     [[maybe_unused]] LPARAM /*lParam*/)

    {
        GenerateResizes(hDlg);

        m_hMemBookmarkDialog = hDlg;
        // const auto -> HWND__* const
        const auto hList = GetDlgItem(m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES);

        SetupColumns(hList);

        // Auto-import bookmark file when opening dialog
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
        if (pGameContext.GameId() != 0)
            ImportFromFile(
                ra::StringPrintf(L"%s%s%u-Bookmarks.txt", g_sHomeDir, RA_DIR_BOOKMARKS, pGameContext.GameId()));

        RestoreWindowPosition(hDlg, "Memory Bookmarks", true, false);
        return TRUE;
    };

    const auto OnGetMinMaxInfo = []([[maybe_unused]] HWND, MINMAXINFO* restrict lpmmi) noexcept
    {
        lpmmi->ptMinTrackSize = pDlgMemBookmarkMin;
    };
    const auto OnSize = [](HWND hDlg, [[maybe_unused]] UINT, int cx, int cy)
    {
        for (auto& content : vDlgMemBookmarkResize)
            content.Resize(cx, cy); // cy, cx are new coordinaes

        // InvalidateRect( hDlg, nullptr, TRUE );
        RememberWindowSize(hDlg, "Memory Bookmarks");
    };

    const auto OnMeasureItem = []([[maybe_unused]] HWND, MEASUREITEMSTRUCT* restrict pmis) noexcept
    {
        pmis->itemHeight = 16;
    };
    const auto OnDrawItem =
        [ this, rcBounds, &rcLabel, offset ](HWND hDlg, const DRAWITEMSTRUCT* restrict pdis) noexcept

    {
        // If there are no list items, skip this message.
        if (pdis->itemID == -1)
            return;

        switch (pdis->itemAction)
        {
            case ODA_SELECT:
            case ODA_DRAWENTIRE:
            {
                auto hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);

                GSL_SUPPRESS_ES47 ListView_GetItemRect(hList, pdis->itemID, &rcBounds, LVIR_BOUNDS);
                GSL_SUPPRESS_ES47 ListView_GetItemRect(hList, pdis->itemID, &rcLabel, LVIR_LABEL);
                RECT rcCol(rcBounds);
                rcCol.right = rcCol.left + ListView_GetColumnWidth(hList, 0);

                // Draw Item Label - Column 0
                std::wstring buffer;
                if (m_vBookmarks.at(pdis->itemID).Decimal())
                    buffer = ra::StringPrintf(L"(D)%s", m_vBookmarks.at(pdis->itemID).Description());
                else
                    buffer = m_vBookmarks.at(pdis->itemID).Description();

                if (pdis->itemState & ODS_SELECTED)
                {
                    SetTextColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                    SetBkColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
                    FillRect(pdis->hDC, &rcBounds, GetSysColorBrush(COLOR_HIGHLIGHT));
                }
                else
                {
                    SetTextColor(pdis->hDC, GetSysColor(COLOR_WINDOWTEXT));

                    COLORREF color{};

                    if (m_vBookmarks.at(pdis->itemID).Frozen())
                        color = RGB(255, 255, 160);
                    else
                        color = GetSysColor(COLOR_WINDOW);

                    HBRUSH hBrush = CreateSolidBrush(color);
                    SetBkColor(pdis->hDC, color);
                    FillRect(pdis->hDC, &rcBounds, hBrush);
                    DeleteObject(hBrush);
                }
                if (!buffer.empty())
                {
                    rcLabel.left += (offset / 2);
                    rcLabel.right -= offset;

                    DrawTextW(pdis->hDC, buffer.c_str(), buffer.length(), &rcLabel,
                              DT_SINGLELINE | DT_LEFT | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER | DT_END_ELLIPSIS);
                }

                // Draw Item Label for remaining columns
                LV_COLUMN lvc{};
                lvc.mask = LVCF_FMT | LVCF_WIDTH;

                for (auto i = 1; ListView_GetColumn(hList, i, &lvc); ++i)
                {
                    rcCol.left = rcCol.right;
                    rcCol.right += lvc.cx;

                    const auto eSubItem{ra::itoe<SubItems>(i)};
                    switch (eSubItem)
                    {
                        case SubItems::Address:
                            buffer = ra::StringPrintf(L"%06x", m_vBookmarks.at(pdis->itemID).Address());
                            break;
                        case SubItems::Value:
                            if (m_vBookmarks.at(pdis->itemID).Decimal())
                                buffer = std::to_wstring(m_vBookmarks.at(pdis->itemID).Value());
                            else
                            {
                                switch (m_vBookmarks.at(pdis->itemID).Type())
                                {
                                    case 1:
                                        buffer = ra::StringPrintf(L"%02x", m_vBookmarks.at(pdis->itemID).Value());
                                        break;
                                    case 2:
                                        buffer = ra::StringPrintf(L"%04x", m_vBookmarks.at(pdis->itemID).Value());
                                        break;
                                    case 3:
                                        buffer = ra::StringPrintf(L"%08x", m_vBookmarks.at(pdis->itemID).Value());
                                        break;
                                }
                            }
                            break;
                        case SubItems::Previous:
                            if (m_vBookmarks.at(pdis->itemID).Decimal())
                                buffer = std::to_wstring(m_vBookmarks.at(pdis->itemID).Previous());
                            else
                            {
                                switch (m_vBookmarks.at(pdis->itemID).Type())
                                {
                                    case 1:
                                        buffer = ra::StringPrintf(L"%02x", m_vBookmarks.at(pdis->itemID).Previous());
                                        break;
                                    case 2:
                                        buffer = ra::StringPrintf(L"%04x", m_vBookmarks.at(pdis->itemID).Previous());
                                        break;
                                    case 3:
                                        buffer = ra::StringPrintf(L"%08x", m_vBookmarks.at(pdis->itemID).Previous());
                                        break;
                                }
                            }
                            break;
                        case SubItems::Changes:
                            buffer = std::to_wstring(m_vBookmarks.at(pdis->itemID).Count());
                            break;
                        default:
                            buffer.clear();
                            break;
                    }

                    if (buffer.empty())
                        continue;

                    UINT nJustify{DT_LEFT};
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

                    DrawTextW(pdis->hDC, buffer.c_str(), buffer.length(), &rcLabel,
                              nJustify | DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_END_ELLIPSIS);
                }

                // if (pdis->itemState & ODS_SELECTED) //&& (GetFocus() == this)
                //  DrawFocusRect(pdis->hDC, &rcBounds);

                break;
            }
            case ODA_FOCUS:
                break;
        }
    };

    // idFrom->wParam; pnmhdr->lParam
    GSL_SUPPRESS_IO5
    const auto OnNotify = [ this, &nSelect ](HWND hDlg, int idFrom, NMHDR* pnmhdr) noexcept
    {
        switch (idFrom)
        {
            case IDC_RA_LBX_ADDRESSES:
            {
                GSL_SUPPRESS_IO5
#pragma warning(suppress: 26454)
                if (pnmhdr->code == NM_CLICK)
                {
                    auto hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);

                    nSelect = ListView_GetNextItem(hList, -1, LVNI_FOCUSED);

                    if (nSelect == -1)
                        break;
                }
#pragma warning(suppress: 26454) // io.5; arithmetic overflow
                else if (pnmhdr->code == NM_DBLCLK)
                {
                    // const NMITEMACTIVATE* const
#pragma warning(push)
#pragma warning(disable: 26490)
                    GSL_SUPPRESS_TYPE1 const auto pOnClick = reinterpret_cast<const NMITEMACTIVATE*>(pnmhdr);
#pragma warning(pop)

                    using namespace ra::rel_ops;
                    if ((pOnClick->iItem != -1) && (pOnClick->iSubItem == SubItems::Desc))
                    {
                        nSelItemBM = pOnClick->iItem;
                        nSelSubItemBM = pOnClick->iSubItem;

                        EditLabel(pOnClick->iItem, pOnClick->iSubItem);
                    }
                    else if ((pOnClick->iItem != -1) && (pOnClick->iSubItem == SubItems::Address))
                    {
                        g_MemoryDialog.SetWatchingAddress(m_vBookmarks.at(pOnClick->iItem).Address());
                        MemoryViewerControl::setAddress(
                            (m_vBookmarks.at(pOnClick->iItem).Address() & ~(0xf)) -
                            (ra::to_signed(MemoryViewerControl::m_nDisplayedLines / 2) << 4) + (0x50));
                    }
                }
            }
        }
        return LRESULT{TRUE};
    };

    // id->LOWORD(wParam), hwndCtl->lParam, codeNotify->HIWORD(wParam)
    const auto OnCommand = [this](HWND hDlg, int id, [[maybe_unused]] HWND /*hwndCtl*/,
                            [[maybe_unused]] UINT /*codeNotify*/) {
        switch (id)
        {
            case IDOK:
            case IDCLOSE:
            case IDCANCEL:
                return EndDialog(hDlg, IDCANCEL);

            case IDC_RA_ADD_BOOKMARK:
            {
                if (g_MemoryDialog.GetHWND() != nullptr)
                    AddAddress();

                return TRUE;
            }
            case IDC_RA_DEL_BOOKMARK:
            {
                auto hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);
                auto nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

                if (nSel != -1)
                {
                    while (nSel >= 0)
                    {
                        // Remove from vector
                        m_vBookmarks.erase(std::next(m_vBookmarks.begin(), nSel));
                        ListView_DeleteItem(hList, nSel);
                        nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                    }

                    InvalidateRect(hList, nullptr, FALSE);
                }

                return TRUE;
            }
            case IDC_RA_FREEZE:
            {
                if (!m_vBookmarks.empty())
                {
                    const auto hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);
                    const auto uSelectedCount = ListView_GetSelectedCount(hList);

                    if (uSelectedCount > 0)
                    {
                        for (auto i = ListView_GetNextItem(hList, -1, LVNI_SELECTED); i >= 0;
                             i = ListView_GetNextItem(hList, i, LVNI_SELECTED))
                            m_vBookmarks.at(i).SetFrozen(!m_vBookmarks.at(i).Frozen());
                    }
                    ListView_SetItemState(hList, -1, LVIF_STATE, LVIS_SELECTED);
                }
                return TRUE;
            }
            case IDC_RA_CLEAR_CHANGE:
            {
                if (!m_vBookmarks.empty())
                {
                    const auto hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);
                    gsl::index idx = -1;
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
                if (!m_vBookmarks.empty())
                {
                    auto hList = GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES);
                    const auto uSelectedCount = ListView_GetSelectedCount(hList);

                    if (uSelectedCount > 0)
                    {
                        for (auto i = ListView_GetNextItem(hList, -1, LVNI_SELECTED); i >= 0;
                             i = ListView_GetNextItem(hList, i, LVNI_SELECTED))
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
                std::wstring file{ImportDialog()};
                if (!file.empty())
                    ImportFromFile(std::move(file));
                return TRUE;
            }
            default:
                return FALSE;
        }
    };

    switch (uMsg)
    {
        HANDLE_MSG(hDlg, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hDlg, WM_GETMINMAXINFO, OnGetMinMaxInfo);
        HANDLE_MSG(hDlg, WM_SIZE, OnSize);
        HANDLE_MSG(hDlg, WM_MEASUREITEM, OnMeasureItem);
        HANDLE_MSG(hDlg, WM_DRAWITEM, OnDrawItem);
        HANDLE_MSG(hDlg, WM_NOTIFY, OnNotify);
        HANDLE_MSG(hDlg, WM_COMMAND, OnCommand);

        case WM_MOVE:
            RememberWindowPosition(hDlg, "Memory Bookmarks");
            break;
        default:
            break;
    }

    return FALSE;
}

BOOL Dlg_MemBookmark::IsActive() const noexcept
{
    return (g_MemBookmarkDialog.GetHWND() != nullptr) && (IsWindowVisible(g_MemBookmarkDialog.GetHWND()));
}

void Dlg_MemBookmark::UpdateBookmarks(bool bForceWrite)
{
    if (!IsWindowVisible(m_hMemBookmarkDialog))
        return;

    if (m_vBookmarks.size() == 0)
        return;

    auto hList = GetDlgItem(m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES);

    gsl::index index = 0;
    for (auto& bookmark : m_vBookmarks)
    {
        if (bookmark.Frozen() && !bForceWrite)
        {
            WriteFrozenValue(bookmark);
            index++;
            continue;
        }

        const auto mem_value = GetMemory(bookmark.Address(), bookmark.Type());

        if (bookmark.Value() != mem_value)
        {
            bookmark.SetPrevious(bookmark.Value());
            bookmark.SetValue(mem_value);
            bookmark.IncreaseCount();

            RECT rcBounds{};
            GSL_SUPPRESS_ES47 ListView_GetItemRect(hList, index, &rcBounds, LVIR_BOUNDS);
            InvalidateRect(hList, &rcBounds, FALSE);
        }

        index++;
    }
}

void Dlg_MemBookmark::PopulateList()
{
    if (m_vBookmarks.empty())
        return;

    auto hList = GetDlgItem(m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES);
    if (hList == nullptr)
        return;

    ListView_DeleteAllItems(hList);
    m_nNumOccupiedRows = 0;

    for (auto it = m_vBookmarks.cbegin(); it != m_vBookmarks.cend(); ++it)
    {
        LV_ITEM item{};
        item.mask = LVIF_TEXT;
        item.cchTextMax = 256;
        item.iItem = m_nNumOccupiedRows;
        item.iItem = ListView_InsertItem(hList, &item);

        Ensures(item.iItem == m_nNumOccupiedRows);
        m_nNumOccupiedRows++;
    }

    ListView_EnsureVisible(hList, m_vBookmarks.size() - 1, FALSE); // Scroll to bottom.
    // Don't sort here
}

void Dlg_MemBookmark::SetupColumns(HWND hList)
{
    // Remove all columns,
    while (ListView_DeleteColumn(hList, 0))
    {
    }

    //	Remove all data.
    ListView_DeleteAllItems(hList);
    gsl::index idx{0};
    for (auto& sColTitle : ra::COLUMN_TITLE)
    {
        ra::tstring tszText{sColTitle};
        LV_COLUMN col{col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_FMT,
                      col.fmt = LVCFMT_CENTER | LVCFMT_FIXED_WIDTH,
                      col.cx = ra::COLUMN_WIDTH.at(idx),
                      col.pszText = tszText.data(),
                      col.cchTextMax = 255,
                      col.iSubItem = idx};

        if (idx == (ra::to_signed(ra::COLUMN_TITLE.size()) - 1))
            col.fmt |= LVCFMT_FILL;

        ListView_InsertColumn(hList, idx, &col);
        idx++;
    }
    m_nNumOccupiedRows = 0;

#if _WIN32_WINNT >= _WIN32_WINNT_VISTA
    if (ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER) != 0UL)
        ::MessageBox(::GetActiveWindow(), _T("The styles specified could not be set."), _T("Error!"),
                     MB_OK | MB_ICONERROR);
#endif // _WIN32_WINNT >= _WIN32_WINNT_VISTA
}

void Dlg_MemBookmark::AddAddress()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    if (pGameContext.GameId() == 0)
        return;

    MemBookmark NewBookmark;

    // Fetch Memory Address from Memory Inspector
    ra::tstring buffer(256, ra::tstring::value_type());
    GetDlgItemText(g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, buffer.data(), 256);

    auto nAddr = strtoul(ra::Narrow(buffer).c_str(), nullptr, 16);
    NewBookmark.SetAddress(nAddr);

    // Check Data GetType
    if (Button_GetCheck(::GetDlgItem(g_MemoryDialog.GetHWND(), IDC_RA_MEMVIEW8BIT)))
        NewBookmark.SetType(1);
    else if (Button_GetCheck(::GetDlgItem(g_MemoryDialog.GetHWND(), IDC_RA_MEMVIEW16BIT)))
        NewBookmark.SetType(2);
    else
        NewBookmark.SetType(3);

    // Get Memory Value
    NewBookmark.SetValue(GetMemory(nAddr, NewBookmark.Type()));
    NewBookmark.SetPrevious(NewBookmark.Value());

    // Get Code Note and add as description
    const CodeNotes::CodeNoteObj* pSavedNote = g_MemoryDialog.Notes().FindCodeNote(nAddr);
    if ((pSavedNote != nullptr) && (pSavedNote->Note().length() > 0))
        NewBookmark.SetDescription(ra::Widen(pSavedNote->Note()));

    // Add Bookmark to vector
    AddBookmark(std::move(NewBookmark));

    PopulateList();
}

void Dlg_MemBookmark::ClearAllBookmarks() noexcept
{
    m_vBookmarks.clear();
    ListView_DeleteAllItems(GetDlgItem(m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES));
}

void Dlg_MemBookmark::WriteFrozenValue(const MemBookmark& Bookmark)
{
    if (!Bookmark.Frozen())
        return;

    unsigned int addr{};
    unsigned int width{};

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

    const auto buffer{ra::StringPrintf("%0*x", width, Bookmark.Value())};
    gsl::index i = 0;
    for (const auto c : buffer)
    {
        const auto n = (c >= 'a') ? (c - 'a' + 10) : (c - '0');
        MemoryViewerControl::editData(addr, (i % 2 != 0), n);

        if (i % 2 != 0)
            addr--;
        i++;
    }
}

unsigned int Dlg_MemBookmark::GetMemory(unsigned int nAddr, int type)
{
    unsigned int mem_value{};

    switch (type)
    {
        case 1:
            mem_value = g_MemManager.ActiveBankRAMRead(nAddr, MemSize::EightBit);
            break;
        case 2:
            mem_value = g_MemManager.ActiveBankRAMRead(nAddr, MemSize::SixteenBit);
            break;
        case 3:
            mem_value = g_MemManager.ActiveBankRAMRead(nAddr, MemSize::ThirtyTwoBit);
            break;
    }

    return mem_value;
}

void Dlg_MemBookmark::ExportJSON()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    if (pGameContext.GameId() == 0)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"ROM not loaded: please load a ROM first!");
        return;
    }

    if (m_vBookmarks.size() == 0)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
            L"No bookmarks to save: please create a bookmark before attempting to save.");
        return;
    }

    constexpr auto BUF_SIZE{1024UL};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = m_hMemBookmarkDialog;
    ofn.lpstrFilter = c_rgFileTypes;
    ofn.nFilterIndex = 1UL;
    ofn.nMaxFile = BUF_SIZE + 16UL;
    ofn.lpstrTitle = L"Save Bookmark File...";
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"txt";

    const auto PathTooLong = []() noexcept
    {
        GSL_SUPPRESS_F6 ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(
            L"Path to file is too long, it needs to be less than 1023 characters!");
    };

    // TODO: With all known options, only a raw array works. We'll have suppress bounds.1 when running that ruleset
    wchar_t buf[BUF_SIZE + 16UL]{}; // Extra capacity to prevent buffer overrun
    {
        const auto sDefaultFilename{ra::StringPrintf(L"%u-Bookmarks.txt", pGameContext.GameId())};
        Expects(swprintf_s(buf, BUF_SIZE, L"%s", sDefaultFilename.c_str()) >= 0);

        if (std::wcslen(buf) > ofn.nMaxFile)
        {
            PathTooLong();
            return;
        }
        ofn.lpstrFile = buf;
    }

    const auto sFilePath{ra::StringPrintf(L"%s%s", g_sHomeDir, RA_DIR_BOOKMARKS)};
    if (sFilePath.length() > (ofn.nMaxFile - std::wcslen(buf)))
    {
        PathTooLong();
        return;
    }
    ofn.lpstrInitialDir = sFilePath.c_str();

    if (::GetSaveFileNameW(&ofn) == 0)
        return;

    rapidjson::Document doc;
    auto& allocator{doc.GetAllocator()};
    doc.SetObject();

    rapidjson::Value bookmarks{rapidjson::kArrayType};

    for (const auto& bookmark : m_vBookmarks)
    {
        rapidjson::Value item{rapidjson::kObjectType};
        std::string str{ra::Narrow(bookmark.Description())};
        rapidjson::Value s{str.c_str(), allocator};

        item.AddMember("Description", s, allocator);
        item.AddMember("Address", bookmark.Address(), allocator);
        item.AddMember("Type", bookmark.Type(), allocator);
        item.AddMember("Decimal", bookmark.Decimal(), allocator);
        bookmarks.PushBack(item, allocator);
    }
    doc.AddMember("Bookmarks", bookmarks, allocator);

    Ensures(swprintf_s(buf, BUF_SIZE, L"%s", ofn.lpstrFile) >= 0);
    _WriteBufferToFile(buf, doc);
}

void Dlg_MemBookmark::ImportFromFile(std::wstring&& sFilename)
{
    const auto file_name{std::move(sFilename)};
    std::ifstream ifile{file_name};
    if (!ifile.is_open())
    {
        std::array<char, 2048> buf{};
        strerror_s(buf.data(), sizeof(buf), errno);
        RA_LOG("%s: %s", ra::Narrow(sFilename).c_str(), buf.data());
        return;
    }

    rapidjson::Document doc;
    rapidjson::IStreamWrapper isw{ifile};
    doc.ParseStream(isw);

    if (doc.HasParseError())
    {
        ASSERT(" !Invalid Bookmark File...");
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Could not load properly. Invalid Bookmark file!");
        return;
    }

    if (doc.HasMember("Bookmarks"))
    {
        ClearAllBookmarks();

        const auto& BookmarksData{doc["Bookmarks"]};

        for (const auto& bmData : BookmarksData.GetArray())
        {
            MemBookmark NewBookmark;
            NewBookmark.SetDescription(ra::Widen(bmData["Description"].GetString()).c_str());

            NewBookmark.SetAddress(bmData["Address"].GetUint());
            NewBookmark.SetType(bmData["Type"].GetInt());
            NewBookmark.SetDecimal(bmData["Decimal"].GetBool());

            NewBookmark.SetValue(GetMemory(NewBookmark.Address(), NewBookmark.Type()));
            NewBookmark.SetPrevious(NewBookmark.Value());

            AddBookmark(std::move(NewBookmark));
        }
        if (!m_vBookmarks.empty())
            PopulateList();
    }
}

std::wstring Dlg_MemBookmark::ImportDialog()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    if (pGameContext.GameId() == 0)
    {
        MessageBox(nullptr, _T("ROM not loaded: please load a ROM first!"), _T("Error!"), MB_OK);
        return std::wstring();
    }

    constexpr auto BUF_SIZE{1024UL};

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = m_hMemBookmarkDialog;
    ofn.lpstrFilter = c_rgFileTypes;
    ofn.nFilterIndex = 1UL;
    ofn.lpstrTitle = L"Import Bookmark File...";
    ofn.nMaxFile = BUF_SIZE + 16UL;
    ofn.Flags = OFN_ENABLESIZING | OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    ofn.lpstrDefExt = L"txt";

    const auto PathTooLong = []() noexcept
    {
        GSL_SUPPRESS_F6 ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(
            L"Path to file is too long, it needs to be less than 1023 characters!");
        return std::wstring();
    };

    wchar_t buf[BUF_SIZE + 16UL]{};
    {
        const auto sDefaultFilename{ra::StringPrintf(L"%u-Bookmarks.txt", pGameContext.GameId())};
        Expects(swprintf_s(buf, BUF_SIZE, L"%s", sDefaultFilename.c_str()) >= 0);
        if (std::wcslen(buf) > ofn.nMaxFile)
            return PathTooLong();
        ofn.lpstrFile = buf;
    }

    const auto sFilePath{ra::StringPrintf(L"%s%s", g_sHomeDir, RA_DIR_BOOKMARKS)};
    if (sFilePath.length() > (ofn.nMaxFile - gsl::narrow<DWORD>(std::wcslen(buf))))
        return PathTooLong();
    ofn.lpstrInitialDir = sFilePath.c_str();

    if (::GetOpenFileNameW(&ofn) == 0)
        return std::wstring();
    Ensures(swprintf_s(buf, BUF_SIZE, L"%s", ofn.lpstrFile) >= 0);
    return buf;
}

void Dlg_MemBookmark::OnLoad_NewRom()
{
    if (::GetDlgItem(m_hMemBookmarkDialog, IDC_RA_LBX_ADDRESSES) != nullptr)
    {
        ClearAllBookmarks();
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
        ImportFromFile(ra::StringPrintf(L"%s%s%u-Bookmarks.txt", g_sHomeDir, RA_DIR_BOOKMARKS, pGameContext.GameId()));
    }
}

void Dlg_MemBookmark::GenerateResizes(HWND hDlg)
{
    RARect windowRect;
    GetWindowRect(hDlg, &windowRect);
    pDlgMemBookmarkMin.x = windowRect.Width();
    pDlgMemBookmarkMin.y = windowRect.Height();

    using AlignType = ResizeContent::AlignType;
    vDlgMemBookmarkResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_LBX_ADDRESSES), AlignType::BottomRight, true);

    vDlgMemBookmarkResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_ADD_BOOKMARK), AlignType::Right, false);
    vDlgMemBookmarkResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_DEL_BOOKMARK), AlignType::Right, false);
    vDlgMemBookmarkResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_CLEAR_CHANGE), AlignType::Right, false);

    vDlgMemBookmarkResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_FREEZE), AlignType::BottomRight, false);
    vDlgMemBookmarkResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_DECIMALBOOKMARK), AlignType::BottomRight, false);
    vDlgMemBookmarkResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_SAVEBOOKMARK), AlignType::BottomRight, false);
    vDlgMemBookmarkResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_LOADBOOKMARK), AlignType::BottomRight, false);
}

BOOL Dlg_MemBookmark::EditLabel(int nItem, int nSubItem)
{
    auto hList = GetDlgItem(g_MemBookmarkDialog.GetHWND(), IDC_RA_LBX_ADDRESSES);
    RECT rcSubItem{};
    GSL_SUPPRESS_ES47 ListView_GetSubItemRect(hList, nItem, nSubItem, LVIR_BOUNDS, &rcSubItem);

    RECT rcOffset{};
    GetWindowRect(hList, &rcOffset);
    rcSubItem.left += rcOffset.left;
    rcSubItem.right += rcOffset.left;
    rcSubItem.top += rcOffset.top;
    rcSubItem.bottom += rcOffset.top;

    const auto nHeight = rcSubItem.bottom - rcSubItem.top;
    const auto nWidth = rcSubItem.right - rcSubItem.left;

    Expects(g_hIPEEditBM == nullptr);
    if (g_hIPEEditBM)
        return FALSE;

    g_hIPEEditBM = CreateWindowEx(WS_EX_CLIENTEDGE, _T("EDIT"), _T(""),
                                  WS_CHILD | WS_VISIBLE | WS_POPUPWINDOW | WS_BORDER | ES_WANTRETURN, rcSubItem.left,
                                  rcSubItem.top, nWidth, ra::ftol(1.5f * nHeight), g_MemBookmarkDialog.GetHWND(),
                                  nullptr, nullptr, nullptr);

    if (g_hIPEEditBM == nullptr)
    {
        ASSERT(!"Could not create edit box!");
        MessageBox(nullptr, _T("Could not create edit box."), _T("Error"), MB_OK | MB_ICONERROR);
        return FALSE;
    };

    SetWindowFont(g_hIPEEditBM, ::GetStockObject(DEFAULT_GUI_FONT), TRUE);
    SetWindowText(g_hIPEEditBM, NativeStr(m_vBookmarks.at(nItem).Description()).c_str());

    Edit_SetSel(g_hIPEEditBM, 0, -1);
    SetFocus(g_hIPEEditBM);
    EOldProcBM = SubclassWindow(g_hIPEEditBM, EditProcBM);

    return TRUE;
}
