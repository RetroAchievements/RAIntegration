#include "RA_Dlg_Memory.h"

#include "RA_Achievement.h"
#include "RA_Core.h"
#include "RA_Resource.h"

#include "data\ConsoleContext.hh"
#include "data\EmulatorContext.hh"
#include "data\GameContext.hh"

#include "services\IAudioSystem.hh"
#include "services\IConfiguration.hh"

#include "ui\IDesktop.hh"

#include "ui\viewmodels\MemoryBookmarksViewModel.hh"
#include "ui\viewmodels\MemoryViewerViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include "ui\drawing\gdi\GDISurface.hh"

#ifndef ID_OK
#define ID_OK 1024
#endif
#ifndef ID_CANCEL
#define ID_CANCEL 1025
#endif

_CONSTANT_VAR MIN_RESULTS_TO_DUMP = 500000U;
_CONSTANT_VAR MIN_SEARCH_PAGE_SIZE = 50U;

static std::unique_ptr<ra::ui::viewmodels::MemoryViewerViewModel> g_pMemoryViewer;
constexpr int MEMVIEW_MARGIN = 4;

Dlg_Memory g_MemoryDialog;

// static
HWND Dlg_Memory::m_hWnd = nullptr;

HFONT MemoryViewerControl::m_hViewerFont = nullptr;
SIZE MemoryViewerControl::m_szFontSize;
unsigned int MemoryViewerControl::m_nDataStartXOffset = 0;
unsigned int MemoryViewerControl::m_nAddressOffset = 0;
unsigned int MemoryViewerControl::m_nWatchedAddress = 0;
MemSize MemoryViewerControl::m_nDataSize = MemSize::EightBit;
unsigned int MemoryViewerControl::m_nEditAddress = 0;
unsigned int MemoryViewerControl::m_nEditNibble = 0;
bool MemoryViewerControl::m_bHasCaret = 0;
unsigned int MemoryViewerControl::m_nCaretWidth = 0;
unsigned int MemoryViewerControl::m_nCaretHeight = 0;
unsigned int MemoryViewerControl::m_nDisplayedLines = 8;
unsigned short MemoryViewerControl::m_nActiveMemBank = 0;

unsigned int m_nPage = 0;

// Dialog Resizing
std::vector<ResizeContent> vDlgMemoryResize;
POINT pDlgMemoryMin;
int nDlgMemoryMinX;
int nDlgMemoryMinY;
int nDlgMemViewerGapY;

LRESULT CALLBACK MemoryViewerControl::s_MemoryDrawProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
            if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
                g_pMemoryViewer->SetFirstAddress(g_pMemoryViewer->GetFirstAddress() - 32);
            else if (GET_WHEEL_DELTA_WPARAM(wParam) < 0)
                g_pMemoryViewer->SetFirstAddress(g_pMemoryViewer->GetFirstAddress() + 32);
            return FALSE;

        case WM_LBUTTONUP:
            OnClick({GET_X_LPARAM(lParam) - MEMVIEW_MARGIN, GET_Y_LPARAM(lParam) - MEMVIEW_MARGIN});
            return FALSE;

        case WM_KEYDOWN:
            return (!OnKeyDown(static_cast<UINT>(LOWORD(wParam))));

        case WM_CHAR:
            return (!OnEditInput(static_cast<UINT>(LOWORD(wParam))));

        case WM_GETDLGCODE:
            return DLGC_WANTCHARS | DLGC_WANTARROWS;
    }

    return DefWindowProc(hDlg, uMsg, wParam, lParam);
}

bool MemoryViewerControl::OnKeyDown(UINT nChar)
{
    const bool bShiftHeld = (GetKeyState(VK_SHIFT) < 0);
    const bool bControlHeld = (GetKeyState(VK_CONTROL) < 0);

    switch (nChar)
    {
        case VK_RIGHT:
            if (bShiftHeld || bControlHeld)
                g_pMemoryViewer->AdvanceCursorWord();
            else
                g_pMemoryViewer->AdvanceCursor();
            return true;

        case VK_LEFT:
            if (bShiftHeld || bControlHeld)
                g_pMemoryViewer->RetreatCursorWord();
            else
                g_pMemoryViewer->RetreatCursor();
            return true;

        case VK_DOWN:
            if (bControlHeld)
                g_pMemoryViewer->SetFirstAddress(g_pMemoryViewer->GetFirstAddress() + 0x10);
            else
                g_pMemoryViewer->AdvanceCursorLine();
            return true;

        case VK_UP:
            if (bControlHeld)
                g_pMemoryViewer->SetFirstAddress(g_pMemoryViewer->GetFirstAddress() - 0x10);
            else
                g_pMemoryViewer->RetreatCursorLine();
            return true;

        case VK_PRIOR: // Page up (!)
            g_pMemoryViewer->RetreatCursorPage();
            return true;

        case VK_NEXT: // Page down (!)
            g_pMemoryViewer->AdvanceCursorPage();
            return true;

        case VK_HOME:
            if (bControlHeld)
            {
                g_pMemoryViewer->SetFirstAddress(0);
                g_pMemoryViewer->SetAddress(0);
            }
            else
            {
                g_pMemoryViewer->SetAddress(g_pMemoryViewer->GetAddress() & ~0x0F);
            }
            return true;

        case VK_END:
            if (bControlHeld)
            {
                const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
                const auto nTotalBytes = pEmulatorContext.TotalMemorySize();

                g_pMemoryViewer->SetFirstAddress(nTotalBytes & ~0x0F);
                g_pMemoryViewer->SetAddress(nTotalBytes - 1);
            }
            else
            {
                g_pMemoryViewer->SetAddress(g_pMemoryViewer->GetAddress() | 0x0F);
            }
            return true;
    }

    return false;
}

void MemoryViewerControl::setAddress(unsigned int address)
{
    m_nAddressOffset = address;

    SetCaretPos();
}

bool MemoryViewerControl::OnEditInput(UINT c)
{
    return g_pMemoryViewer->OnChar(gsl::narrow_cast<char>(c));
}

void MemoryViewerControl::createEditCaret(int w, int h) noexcept
{
    if (!m_bHasCaret || ra::to_signed(m_nCaretWidth) != w || ra::to_signed(m_nCaretHeight) != h)
    {
        m_bHasCaret = true;
        m_nCaretWidth = w;
        m_nCaretHeight = h;
        ::CreateCaret(GetDlgItem(g_MemoryDialog.GetHWND(), IDC_RA_MEMTEXTVIEWER), nullptr, w, h);
    }
}

void MemoryViewerControl::destroyEditCaret() noexcept
{
    m_bHasCaret = false;
    DestroyCaret();
}

void MemoryViewerControl::SetCaretPos()
{
    if (ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().TotalMemorySize() == 0U)
        return;

    HWND hOurDlg = GetDlgItem(g_MemoryDialog.GetHWND(), IDC_RA_MEMTEXTVIEWER);
    if (GetFocus() != hOurDlg)
    {
        destroyEditCaret();
        return;
    }

    g_MemoryDialog.SetWatchingAddress(m_nEditAddress);

    const int subAddress = (m_nEditAddress - m_nAddressOffset);

    const int linePosition = (subAddress & ~(0xF)) / (0x10) + 4;

    if (linePosition < 0 || linePosition > ra::to_signed(m_nDisplayedLines) - 1)

    {
        destroyEditCaret();
        return;
    }

    const int nYSpacing = linePosition;

    int x = 3 + (10 * m_szFontSize.cx) + (m_nEditNibble * m_szFontSize.cx);
    int y = 3 + (nYSpacing * m_szFontSize.cy);

    y += m_szFontSize.cy; //	Account for header

    switch (m_nDataSize)
    {
        case MemSize::EightBit:
            x += 3 * m_szFontSize.cx * (subAddress & 15);
            break;
        case MemSize::SixteenBit:
            x += 5 * m_szFontSize.cx * ((subAddress >> 1) & 7);
            break;
        case MemSize::ThirtyTwoBit:
            x += 9 * m_szFontSize.cx * ((subAddress >> 2) & 3);
            break;
    }

    RECT r;
    GetClientRect(hOurDlg, &r);
    r.right -= 3;
    if (x >= r.right)
    {
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
    g_pMemoryViewer->OnClick(point.x, point.y);

    HWND hOurDlg = GetDlgItem(g_MemoryDialog.GetHWND(), IDC_RA_MEMTEXTVIEWER);
    SetFocus(hOurDlg);
}

void MemoryViewerControl::RenderMemViewer(HWND hTarget)
{
    g_pMemoryViewer->UpdateRenderImage();

    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(hTarget, &ps);

    RECT rcClient;
    GetClientRect(hTarget, &rcClient);

    const auto& pRenderImage = g_pMemoryViewer->GetRenderImage();

    HBRUSH hBackground = GetStockBrush(WHITE_BRUSH);
    RECT rcFill{ rcClient.left + 1, rcClient.top + 1, rcClient.left + MEMVIEW_MARGIN, rcClient.bottom - 2 };
    FillRect(hDC, &rcFill, hBackground);

    rcFill.left = rcClient.left + MEMVIEW_MARGIN + pRenderImage.GetWidth();
    rcFill.right = rcClient.right - 2;
    FillRect(hDC, &rcFill, hBackground);

    rcFill.left = rcClient.left + 1;
    rcFill.bottom = rcClient.top + MEMVIEW_MARGIN;
    FillRect(hDC, &rcFill, hBackground);

    rcFill.top = rcClient.top + MEMVIEW_MARGIN + pRenderImage.GetHeight();
    rcFill.bottom = rcClient.bottom - 2;
    FillRect(hDC, &rcFill, hBackground);

    DrawEdge(hDC, &rcClient, EDGE_ETCHED, BF_RECT);

    ra::ui::drawing::gdi::GDISurface pSurface(hDC, rcClient);
    pSurface.DrawSurface(MEMVIEW_MARGIN, MEMVIEW_MARGIN, pRenderImage);

    EndPaint(hTarget, &ps);
}

void Dlg_Memory::Init() noexcept
{
    WNDCLASSEX wc{sizeof(WNDCLASSEX)};
    wc.style = ra::to_unsigned(CS_PARENTDC | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS);
    wc.lpfnWndProc = MemoryViewerControl::s_MemoryDrawProc;
    wc.hInstance = g_hThisDLLInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = GetStockBrush(WHITE_BRUSH);
    wc.lpszClassName = TEXT("MemoryViewerControl");

    [[maybe_unused]] const auto checkAtom = ::RegisterClassEx(&wc);
    ASSERT(checkAtom != 0);
}

void Dlg_Memory::Shutdown() noexcept
{
    ::UnregisterClass(TEXT("MemoryViewerControl"), g_hThisDLLInst);
}

// static
INT_PTR CALLBACK Dlg_Memory::s_MemoryProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return g_MemoryDialog.MemoryProc(hDlg, uMsg, wParam, lParam);
}

void Dlg_Memory::ClearLogOutput() noexcept
{
    ListView_SetItemCount(GetDlgItem(m_hWnd, IDC_RA_MEM_LIST), 1);
    EnableWindow(GetDlgItem(m_hWnd, IDC_RA_RESULTS_BACK), FALSE);
    EnableWindow(GetDlgItem(m_hWnd, IDC_RA_RESULTS_FORWARD), FALSE);
}

void Dlg_Memory::OnViewModelIntValueChanged(const ra::ui::IntModelProperty::ChangeArgs& args)
{
    if (args.Property == ra::ui::viewmodels::MemoryViewerViewModel::AddressProperty)
    {
        SetWatchingAddress(g_pMemoryViewer->GetAddress());
    }
}

INT_PTR Dlg_Memory::MemoryProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg)
    {
        case WM_INITDIALOG:
        {
            g_MemoryDialog.m_hWnd = hDlg;

            g_pMemoryViewer.reset(new ra::ui::viewmodels::MemoryViewerViewModel());
            g_pMemoryViewer->AddNotifyTarget(*this);

            GenerateResizes(hDlg);

            CheckDlgButton(hDlg, IDC_RA_CBO_SEARCHALL, BST_CHECKED);
            CheckDlgButton(hDlg, IDC_RA_CBO_SEARCHCUSTOM, BST_UNCHECKED);
            EnableWindow(GetDlgItem(hDlg, IDC_RA_SEARCHRANGE), FALSE);
            CheckDlgButton(hDlg, IDC_RA_CBO_SEARCHSYSTEMRAM, BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_RA_CBO_SEARCHGAMERAM, BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_RA_CBO_GIVENVAL, BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_RA_CBO_LASTKNOWNVAL, BST_CHECKED);
            EnableWindow(GetDlgItem(hDlg, IDC_RA_TESTVAL), FALSE);

            for (const auto str : COMPARISONTYPE_STR)
                ComboBox_AddString(GetDlgItem(hDlg, IDC_RA_CBO_CMPTYPE), str);

            ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_RA_CBO_CMPTYPE), 0);

            EnableWindow(GetDlgItem(hDlg, IDC_RA_DOTEST), FALSE);

            SetDlgItemText(hDlg, IDC_RA_WATCHING, TEXT("0x0000"));

            SetWindowFont(GetDlgItem(hDlg, IDC_RA_MEMBITS), GetStockObject(SYSTEM_FIXED_FONT), TRUE);
            SetWindowFont(GetDlgItem(hDlg, IDC_RA_MEMBITS_TITLE), GetStockObject(SYSTEM_FIXED_FONT), TRUE);

            CheckDlgButton(hDlg, IDC_RA_MEMVIEW8BIT, BST_CHECKED);
            CheckDlgButton(hDlg, IDC_RA_MEMVIEW16BIT, BST_UNCHECKED);
            CheckDlgButton(hDlg, IDC_RA_MEMVIEW32BIT, BST_UNCHECKED);

            g_MemoryDialog.OnLoad_NewRom();

            // Add a single column for list view
            LVCOLUMN Col{};
            Col.mask = LVCF_FMT | LVCF_ORDER | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
            Col.fmt = LVCFMT_CENTER;
            RECT rc{};
            GetWindowRect(GetDlgItem(hDlg, IDC_RA_MEM_LIST), &rc);
            for (int i = 0; i < 1; i++)
            {
                Col.iOrder = i;
                Col.iSubItem = i;
                ra::tstring str{_T("Search Result")};
                Col.pszText = str.data();
                Col.cx = rc.right - rc.left - 24;
                ListView_InsertColumn(GetDlgItem(hDlg, IDC_RA_MEM_LIST), i, &Col);
            }
            ListView_SetExtendedListViewStyle(GetDlgItem(hDlg, IDC_RA_MEM_LIST),
                                              LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

            CheckDlgButton(hDlg, IDC_RA_RESULTS_HIGHLIGHT, BST_CHECKED);

            RestoreWindowPosition(hDlg, "Memory Inspector", true, false);
            return TRUE;
        }

        case WM_MEASUREITEM:
        {
            LPMEASUREITEMSTRUCT pmis{};
            GSL_SUPPRESS_TYPE1 pmis = reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);
            pmis->itemHeight = 16;
            return TRUE;
        }

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT pDIS{};
            GSL_SUPPRESS_TYPE1 pDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
            HWND hListbox{};
            hListbox = GetDlgItem(hDlg, IDC_RA_MEM_LIST);

            if (pDIS->hwndItem == hListbox)
            {
                if (pDIS->itemID == -1)
                    break;

                if (!m_SearchResults.empty())
                {
                    if (pDIS->itemID < 2)
                    {
                        std::wstring sBuffer;
                        if (pDIS->itemID == 0)
                        {
                            try
                            {
                                const std::string& sFirstLine = m_SearchResults.at(m_nPage).m_results.Summary();
                                sBuffer = ra::Widen(sFirstLine);
                            } catch (const std::out_of_range& e)
                            {
                                sBuffer = ra::Widen(e.what());
                            }
                        }
                        else
                        {
                            SetTextColor(pDIS->hDC, RGB(0, 100, 150));
                            const unsigned int nMatches = m_SearchResults.at(m_nPage).m_results.MatchingAddressCount();
                            if (nMatches > MIN_RESULTS_TO_DUMP)
                                sBuffer = ra::StringPrintf(L"Found %u matches! (Displaying first %u results)",
                                                           nMatches, MIN_RESULTS_TO_DUMP);
                            else if (nMatches == 0)
                                sBuffer = L"Found *ZERO* matches!";
                            else
                                sBuffer = ra::StringPrintf(L"Found %u matches!", nMatches);
                        }

                        DrawTextW(pDIS->hDC, sBuffer.c_str(), gsl::narrow_cast<int>(sBuffer.length()), &pDIS->rcItem,
                                  DT_SINGLELINE | DT_LEFT | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER | DT_END_ELLIPSIS);
                    }
                    else
                    {
                        auto& currentSearch = m_SearchResults.at(m_nPage);
                        ra::services::SearchResults::Result result;
                        if (!currentSearch.m_results.GetMatchingAddress(pDIS->itemID - 2, result))
                            break;

                        std::wstring sValue;
                        unsigned int nVal = 0;
                        UpdateSearchResult(result, nVal, sValue);
                        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
                        auto sNote = pGameContext.FindCodeNote(result.nAddress, result.nSize);

                        HBRUSH hBrush{};
                        COLORREF color{};

                        if (pDIS->itemState & ODS_SELECTED)
                        {
                            SetTextColor(pDIS->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
                            hBrush = GetSysColorBrush(COLOR_HIGHLIGHT);
                        }
                        else if (SendMessage(GetDlgItem(hDlg, IDC_RA_RESULTS_HIGHLIGHT), BM_GETCHECK, 0, 0))
                        {
                            SetTextColor(pDIS->hDC, GetSysColor(COLOR_WINDOWTEXT));
                            if (!CompareSearchResult(nVal, result.nValue))
                            {
                                color = RGB(255, 215, 215); // Red if search result doesn't match comparison.
                                if (!currentSearch.WasModified(result.nAddress))
                                    currentSearch.m_modifiedAddresses.push_back(result.nAddress);
                            }
                            else if (ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryBookmarks.HasBookmark(result.nAddress))
                                color = RGB(220, 255, 220); // Green if Bookmark is found.
                            else if (!sNote.empty())
                                color = RGB(220, 240, 255); // Blue if Code Note is found.
                            else if (currentSearch.WasModified(result.nAddress))
                                color = RGB(240, 240, 240); // Grey if still valid, but has changed
                            else
                                hBrush = GetSysColorBrush(COLOR_WINDOW);
                        }
                        else
                        {
                            SetTextColor(pDIS->hDC, GetSysColor(COLOR_WINDOWTEXT));
                            hBrush = GetSysColorBrush(COLOR_WINDOW);
                        }

                        if (hBrush)
                        {
                            FillRect(pDIS->hDC, &pDIS->rcItem, hBrush);
                        }
                        else
                        {
                            hBrush = CreateSolidBrush(color);
                            FillRect(pDIS->hDC, &pDIS->rcItem, hBrush);
                            DeleteObject(hBrush);
                        }

                        RECT rcValue = pDIS->rcItem;
                        std::wstring sAddress;
                        if (ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().TotalMemorySize() > 0x10000)
                        {
                            sAddress = ra::StringPrintf(L"0x%06x", result.nAddress);
                            rcValue.left += 54;
                        }
                        else
                        {
                            sAddress = ra::StringPrintf(L"0x%04x", result.nAddress);
                            rcValue.left += 44;
                        }

                        if (result.nSize == MemSize::Nibble_Lower)
                        {
                            sAddress.push_back('L');
                            rcValue.left += 6;
                        }
                        else if (result.nSize == MemSize::Nibble_Upper)
                        {
                            sAddress.push_back('U');
                            rcValue.left += 6;
                        }

                        DrawTextW(pDIS->hDC, sAddress.c_str(), gsl::narrow_cast<int>(sAddress.length()), &pDIS->rcItem,
                            DT_SINGLELINE | DT_LEFT | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);

                        rcValue.right = rcValue.left + gsl::narrow_cast<int>(sValue.length()) * 6;
                        DrawTextW(pDIS->hDC, sValue.c_str(), gsl::narrow_cast<int>(sValue.length()), &rcValue,
                            DT_SINGLELINE | DT_LEFT | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER);

                        if (sNote.empty())
                        {
                            const auto* pRegion = ra::services::ServiceLocator::Get<ra::data::ConsoleContext>().GetMemoryRegion(result.nAddress);
                            if (pRegion)
                            {
                                sNote = ra::Widen(pRegion->Description);

                                if (!(pDIS->itemState & ODS_SELECTED))
                                    SetTextColor(pDIS->hDC, RGB(160, 160, 160));
                            }
                        }

                        if (!sNote.empty())
                        {
                            RECT rcNote = pDIS->rcItem;
                            rcNote.left = rcValue.right + 6;
                            DrawTextW(pDIS->hDC, sNote.c_str(), gsl::narrow_cast<int>(sNote.length()), &rcNote,
                                DT_SINGLELINE | DT_LEFT | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER | DT_END_ELLIPSIS);
                        }
                    }
                }
            }
            return TRUE;
        }

        case WM_NOTIFY:
        {
            switch (LOWORD(wParam))
            {
                case IDC_RA_MEM_LIST:
                {
                    GSL_SUPPRESS_IO5 GSL_SUPPRESS_TYPE1
#pragma warning(suppress: 26454 26490)
                    if ((reinterpret_cast<LPNMHDR>(lParam))->code == LVN_ITEMCHANGED ||
                        (reinterpret_cast<LPNMHDR>(lParam))->code == NM_CLICK)
                    {
                        const int nSelect = ListView_GetNextItem(GetDlgItem(hDlg, IDC_RA_MEM_LIST), -1, LVNI_FOCUSED);

                        if (nSelect == -1)
                            break;
                        else if (nSelect >= 2)
                        {
                            ra::services::SearchResults::Result result;
                            if (!m_SearchResults.at(m_nPage).m_results.GetMatchingAddress(nSelect - 2, result))
                                break;

                            ComboBox_SetText(GetDlgItem(hDlg, IDC_RA_WATCHING),
                                             NativeStr(ra::ByteAddressToString(result.nAddress)).c_str());

                            const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
                            const auto* pNote = pGameContext.FindCodeNote(result.nAddress);
                            if (pNote && !pNote->empty())
                                SetDlgItemTextW(hDlg, IDC_RA_MEMSAVENOTE, ra::Widen(*pNote).c_str());
                            else
                                SetDlgItemText(hDlg, IDC_RA_MEMSAVENOTE, _T(""));

                            GoToAddress(result.nAddress);
                        }
                        else
                            ListView_SetItemState(GetDlgItem(hDlg, IDC_RA_MEM_LIST), -1, LVIF_STATE, LVIS_SELECTED);
                    }
                }
            }
        }
        break;

        case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO lpmmi{};
            GSL_SUPPRESS_TYPE1 lpmmi = reinterpret_cast<LPMINMAXINFO>(lParam);
            lpmmi->ptMaxTrackSize.x = pDlgMemoryMin.x;
            lpmmi->ptMinTrackSize = pDlgMemoryMin;
            return TRUE;
        }

        case WM_SIZE:
        {
            RARect winRect;
            GetWindowRect(hDlg, &winRect);

            for (ResizeContent& content : vDlgMemoryResize)
                content.Resize(winRect.Width(), winRect.Height());

            RememberWindowSize(hDlg, "Memory Inspector");

            GetWindowRect(GetDlgItem(hDlg, IDC_RA_MEMTEXTVIEWER), &winRect);
            g_pMemoryViewer->OnResized(winRect.right - winRect.left - MEMVIEW_MARGIN * 2,
                winRect.bottom - winRect.top - MEMVIEW_MARGIN * 2);
            return TRUE;
        }

        case WM_MOVE:
            RememberWindowPosition(hDlg, "Memory Inspector");
            break;

        case WM_LBUTTONDBLCLK:
        {
            // ignore if alt/shift/control pressed
            if (wParam != 1)
                break;

            // ignore if memory not avaialble or mode is not 8-bit
             auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>();
            if (pEmulatorContext.TotalMemorySize() == 0 || MemoryViewerControl::GetDataSize() != MemSize::EightBit)
                break;

            POINT ptCursor{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            HWND hChild;
            hChild = ChildWindowFromPoint(m_hWnd, ptCursor);

            const auto hMemBits = GetDlgItem(m_hWnd, IDC_RA_MEMBITS);
            if (hChild == hMemBits)
            {
                // determine the width of one character
                // ASSERT: field is using a fixed-width font, so all characters have the same width
                INT nCharWidth{};
                HDC hDC = GetDC(hMemBits);
                {
                    SelectFont(hDC, GetWindowFont(hMemBits));
                    GetCharWidth32(hDC, static_cast<UINT>(' '), static_cast<UINT>(' '), &nCharWidth);
                }
                ReleaseDC(hMemBits, hDC);

                // get bounding rect for membits (screen coordinates) and translate click to screen coordinates
                RECT rcMemBits;
                GetWindowRect(hMemBits, &rcMemBits);
                ClientToScreen(hDlg, &ptCursor);

                // figure out which bit was clicked. NOTE: text is right aligned, so start there
                const auto nOffset = rcMemBits.right - ptCursor.x;
                const auto nChars = (nOffset / nCharWidth);

                // bits are in even indices - odd indices are spaces, ignore them
                if ((nChars & 1) == 0)
                {
                    const auto nBit = nChars >> 1;
                    if (nBit < 8)
                    {
                        // if we found a bit, toggle it
                        const auto nAddr = MemoryViewerControl::getWatchedAddress();
                        auto nVal = pEmulatorContext.ReadMemoryByte(nAddr);
                        nVal ^= (1 << nBit);
                        pEmulatorContext.WriteMemoryByte(nAddr, nVal);

                        InvalidateMemoryViewer();

                        UpdateBits();
                    }
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_RA_DOTEST:
                {
                    if (ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().TotalMemorySize() == 0)
                        return TRUE; //	Handled

                    const ComparisonType nCmpType =
                        static_cast<ComparisonType>(ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_RA_CBO_CMPTYPE)));

                    while (m_SearchResults.size() > gsl::narrow_cast<size_t>(m_nPage) + 1)
                        m_SearchResults.pop_back();

                    ClearLogOutput();

                    m_SearchResults.emplace_back();
                    m_nPage++;

                    if (m_SearchResults.size() > MIN_SEARCH_PAGE_SIZE)
                    {
                        m_SearchResults.erase(m_SearchResults.begin());
                        m_nPage--;
                    }

                    EnableWindow(GetDlgItem(hDlg, IDC_RA_RESULTS_BACK), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_RA_RESULTS_FORWARD), FALSE);

                    assert(m_SearchResults.size() >= 2); // expect at least initial search and new filter holder
                    SearchResult& srPrevious = *(m_SearchResults.end() - 2);
                    SearchResult& sr = m_SearchResults.back();
                    sr.m_nCompareType = nCmpType;

                    if (IsDlgButtonChecked(hDlg, IDC_RA_CBO_GIVENVAL) == BST_UNCHECKED)
                    {
                        sr.m_results.Initialize(srPrevious.m_results, nCmpType);
                        sr.m_bUseLastValue = true;
                    }
                    else
                    {
                        unsigned int nValueQuery = 0;

                        std::array<TCHAR, 1024> nativeBuffer{};
                        if (GetDlgItemText(hDlg, IDC_RA_TESTVAL, nativeBuffer.data(), 1024))
                        {
                            const auto sAddr = ra::Narrow(&nativeBuffer.at(0));
                            const char* pStart = sAddr.c_str();

                            if (ra::StringStartsWith(sAddr, "-"))
                                ++pStart;

                            // try decimal parse first
                            char* pEnd;
                            nValueQuery = std::strtoul(pStart, &pEnd, 10);
                            assert(pEnd != nullptr);
                            if (*pEnd)
                            {
                                // decimal parse failed, try hex
                                nValueQuery = std::strtoul(pStart, &pEnd, 16);
                                assert(pEnd != nullptr);
                                if (*pEnd)
                                {
                                    // hex parse failed
                                    nValueQuery = 0;
                                }
                            }

                            if (pStart > sAddr.c_str())
                            {
                                // minus prefix - invert value
                                GSL_SUPPRESS_TYPE1 nValueQuery = static_cast<unsigned int>(-static_cast<int>(nValueQuery));

                                switch (MemoryViewerControl::GetDataSize())
                                {
                                    case MemSize::EightBit:
                                        nValueQuery &= 0xFF;
                                        break;

                                    case MemSize::SixteenBit:
                                        nValueQuery &= 0xFFFF;
                                        break;
                                }
                            }
                        }

                        sr.m_results.Initialize(srPrevious.m_results, nCmpType, nValueQuery);
                        sr.m_nLastQueryVal = nValueQuery;
                        sr.m_bUseLastValue = false;
                    }

                    const unsigned int nMatches = sr.m_results.MatchingAddressCount();
                    if (nMatches == srPrevious.m_results.MatchingAddressCount())
                    {
                        // same number of matches, if the same query was used, don't double up on the search results
                        if (sr.m_bUseLastValue == srPrevious.m_bUseLastValue &&
                            sr.m_nCompareType == srPrevious.m_nCompareType &&
                            sr.m_nLastQueryVal == srPrevious.m_nLastQueryVal)
                        {
                            // comparing against last value for non-equals case may result in different match
                            // highlights, keep it.
                            if (!sr.m_bUseLastValue || sr.m_nCompareType == ComparisonType::Equals)
                            {
                                m_SearchResults.erase(m_SearchResults.end() - 1);
                                m_nPage--;
                            }
                        }
                    }

                    if (nMatches > MIN_RESULTS_TO_DUMP)
                        ListView_SetItemCount(GetDlgItem(hDlg, IDC_RA_MEM_LIST), MIN_RESULTS_TO_DUMP + 2);
                    else
                        ListView_SetItemCount(GetDlgItem(hDlg, IDC_RA_MEM_LIST), gsl::narrow_cast<size_t>(nMatches) + 2);

                    EnableWindow(GetDlgItem(hDlg, IDC_RA_DOTEST), nMatches > 0);
                    return TRUE;
                }

                case IDC_RA_MEMVIEW8BIT:
                    MemoryViewerControl::SetDataSize(MemSize::EightBit);
                    MemoryViewerControl::destroyEditCaret();
                    SetDlgItemText(hDlg, IDC_RA_MEMBITS_TITLE, TEXT("Bits: 7 6 5 4 3 2 1 0"));
                    UpdateBits();
                    return FALSE;

                case IDC_RA_MEMVIEW16BIT:
                    MemoryViewerControl::SetDataSize(MemSize::SixteenBit);
                    MemoryViewerControl::destroyEditCaret();
                    SetDlgItemText(hDlg, IDC_RA_MEMBITS_TITLE, TEXT(""));
                    SetDlgItemText(m_hWnd, IDC_RA_MEMBITS, TEXT(""));
                    return FALSE;

                case IDC_RA_MEMVIEW32BIT:
                    MemoryViewerControl::SetDataSize(MemSize::ThirtyTwoBit);
                    MemoryViewerControl::destroyEditCaret();
                    SetDlgItemText(hDlg, IDC_RA_MEMBITS_TITLE, TEXT(""));
                    SetDlgItemText(m_hWnd, IDC_RA_MEMBITS, TEXT(""));
                    return FALSE;

                case IDC_RA_CBO_4BIT:
                case IDC_RA_CBO_8BIT:
                case IDC_RA_CBO_16BIT:
                case IDC_RA_CBO_32BIT:
                {
                    MemSize nCompSize = MemSize::Nibble_Lower; //	or upper, doesn't really matter
                    if (SendDlgItemMessage(hDlg, IDC_RA_CBO_8BIT, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        nCompSize = MemSize::EightBit;
                    else if (SendDlgItemMessage(hDlg, IDC_RA_CBO_16BIT, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        nCompSize = MemSize::SixteenBit;
                    else if (SendDlgItemMessage(hDlg, IDC_RA_CBO_32BIT, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        nCompSize = MemSize::ThirtyTwoBit;
                    else // if (SendDlgItemMessage(hDlg, IDC_RA_CBO_4BIT, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        nCompSize = MemSize::Nibble_Lower;

                    ClearLogOutput();
                    m_nPage = 0;

                    m_SearchResults.clear();
                    m_SearchResults.emplace_back();
                    SearchResult& sr = m_SearchResults.back();

                    ra::ByteAddress start, end;
                    if (GetSelectedMemoryRange(start, end))
                    {
                        m_nCompareSize = nCompSize;
                        m_nStart = start;
                        m_nEnd = end;
                        sr.m_results.Initialize(start, end - start + 1, nCompSize);

                        EnableWindow(GetDlgItem(hDlg, IDC_RA_DOTEST), sr.m_results.MatchingAddressCount() > 0);
                    }

                    return FALSE;
                }

                case ID_OK:
                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDC_RA_CBO_GIVENVAL:
                case IDC_RA_CBO_LASTKNOWNVAL:
                    EnableWindow(GetDlgItem(hDlg, IDC_RA_TESTVAL),
                                 (IsDlgButtonChecked(hDlg, IDC_RA_CBO_GIVENVAL) == BST_CHECKED));
                    return TRUE;

                case IDC_RA_CBO_SEARCHALL:
                case IDC_RA_CBO_SEARCHCUSTOM:
                case IDC_RA_CBO_SEARCHSYSTEMRAM:
                case IDC_RA_CBO_SEARCHGAMERAM:
                    EnableWindow(GetDlgItem(hDlg, IDC_RA_SEARCHRANGE),
                                 IsDlgButtonChecked(hDlg, IDC_RA_CBO_SEARCHCUSTOM) == BST_CHECKED);
                    return TRUE;

                case IDC_RA_ADDNOTE:
                {
                    HWND hNote = GetDlgItem(hDlg, IDC_RA_MEMSAVENOTE);
                    const int nLength = GetWindowTextLengthW(hNote);
                    std::wstring sNewNote;
                    sNewNote.resize(gsl::narrow_cast<size_t>(nLength) + 1);
                    GetWindowTextW(hNote, sNewNote.data(), gsl::narrow_cast<int>(sNewNote.capacity()));
                    sNewNote.resize(nLength);

                    bool bUpdated = false;
                    const ra::ByteAddress nAddr = MemoryViewerControl::getWatchedAddress();
                    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
                    std::string sAuthor;
                    const auto* pNote = pGameContext.FindCodeNote(nAddr, sAuthor);
                    if (pNote && !pNote->empty())
                    {
                        if (*pNote != sNewNote) // New note is different
                        {
                            ra::ui::viewmodels::MessageBoxViewModel vmPrompt;
                            vmPrompt.SetHeader(ra::StringPrintf(L"Overwrite note for address %s?", ra::ByteAddressToString(nAddr)));

                            if (sNewNote.length() > 256 || pNote->length() > 256)
                            {
                                std::wstring sNewNoteShort = sNewNote.length() > 256 ? (sNewNote.substr(0, 253) + L"...") : sNewNote;
                                std::wstring sOldNoteShort = pNote->length() > 256 ? (pNote->substr(0, 253) + L"...") : *pNote;
                                vmPrompt.SetMessage(ra::StringPrintf(L"Are you sure you want to replace %s's note:\n\n%s\n\nWith your note:\n\n%s",
                                    sAuthor, sOldNoteShort, sNewNoteShort));
                            }
                            else
                            {
                                vmPrompt.SetMessage(ra::StringPrintf(L"Are you sure you want to replace %s's note:\n\n%s\n\nWith your note:\n\n%s",
                                    sAuthor, *pNote, sNewNote));
                            }

                            vmPrompt.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
                            vmPrompt.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
                            if (vmPrompt.ShowModal() == ra::ui::DialogResult::Yes)
                            {
                                bUpdated = pGameContext.SetCodeNote(nAddr, sNewNote);
                            }
                        }
                        else
                        {
                            //	Already exists and is added exactly as described. Ignore.
                            return FALSE;
                        }
                    }
                    else
                    {
                        //	Doesn't yet exist: add it newly!
                        bUpdated = pGameContext.SetCodeNote(nAddr, sNewNote);
                        if (bUpdated)
                        {
                            const std::string sAddress = ra::ByteAddressToString(nAddr);
                            HWND hMemWatch = GetDlgItem(hDlg, IDC_RA_WATCHING);
                            ComboBox_AddString(hMemWatch, NativeStr(sAddress).c_str());
                        }
                    }

                    if (bUpdated)
                    {
                        ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().Beep();
                    }
                    else
                    {
                        // update failed, revert to previous text
                        if (pNote)
                            SetDlgItemTextW(hDlg, IDC_RA_MEMSAVENOTE, pNote->c_str());
                        else
                            SetDlgItemTextW(hDlg, IDC_RA_MEMSAVENOTE, L"");
                    }
                    return FALSE;
                }

                case IDC_RA_REMNOTE:
                {
                    const ra::ByteAddress nAddr = MemoryViewerControl::getWatchedAddress();
                    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
                    std::string sAuthor;
                    const auto* pNote = pGameContext.FindCodeNote(nAddr, sAuthor);
                    if (pNote != nullptr)
                    {
                        auto pNoteShort = (pNote->length() < 256) ? *pNote : (pNote->substr(0, 253) + L"...");

                        ra::ui::viewmodels::MessageBoxViewModel vmPrompt;
                        vmPrompt.SetHeader(ra::StringPrintf(L"Delete note for address %s?", ra::ByteAddressToString(nAddr)));
                        vmPrompt.SetMessage(ra::StringPrintf(L"Are you sure you want to delete %s's note:\n\n%s", sAuthor, pNoteShort));
                        vmPrompt.SetButtons(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo);
                        vmPrompt.SetIcon(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning);
                        if (vmPrompt.ShowModal() == ra::ui::DialogResult::Yes)
                        {
                            if (pGameContext.DeleteCodeNote(nAddr))
                            {
                                ra::services::ServiceLocator::Get<ra::services::IAudioSystem>().Beep();

                                SetDlgItemText(hDlg, IDC_RA_MEMSAVENOTE, TEXT(""));

                                HWND hMemWatch = GetDlgItem(hDlg, IDC_RA_WATCHING);

                                TCHAR sAddressWide[16];
                                ComboBox_GetText(hMemWatch, sAddressWide, 16);
                                int nIndex = ComboBox_FindString(hMemWatch, -1, NativeStr(sAddressWide).c_str());
                                if (nIndex != CB_ERR)
                                    ComboBox_DeleteString(hMemWatch, nIndex);

                                ComboBox_SetText(hMemWatch, sAddressWide);
                            }
                        }
                    }

                    return FALSE;
                }

                case IDC_RA_OPENPAGE:
                {
                    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
                    if (pGameContext.GameId() != 0)
                    {
                        const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
                        const auto sUrl = ra::StringPrintf("%s/codenotes.php?g=%u", pConfiguration.GetHostUrl(), pGameContext.GameId());

                        const auto& pDesktop = ra::services::ServiceLocator::Get<ra::ui::IDesktop>();
                        pDesktop.OpenUrl(sUrl);
                    }
                    else
                    {
                        MessageBox(nullptr, _T("No ROM loaded!"), _T("Error!"), MB_ICONWARNING);
                    }

                    return FALSE;
                }

                case IDC_RA_ADDBOOKMARK:
                {
                    auto& pBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
                    if (!pBookmarks.IsVisible())
                        pBookmarks.Show();
                    pBookmarks.AddBookmark(MemoryViewerControl::getWatchedAddress(), MemoryViewerControl::GetDataSize());
                    return FALSE;
                }

                case IDC_RA_RESULTS_BOOKMARK:
                {
                    if (!m_SearchResults.empty())
                    {
                        auto& pBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
                        if (!pBookmarks.IsVisible())
                            pBookmarks.Show();

                        HWND hList = GetDlgItem(hDlg, IDC_RA_MEM_LIST);
                        int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                        auto& currentSearch = m_SearchResults.at(m_nPage);

                        int nCount = 0;
                        pBookmarks.Bookmarks().BeginUpdate();
                        while (nSel >= 0)
                        {
                            ra::services::SearchResults::Result result;
                            if (!currentSearch.m_results.GetMatchingAddress(nSel - 2, result))
                                break;

                            if (result.nSize == MemSize::Nibble_Lower || result.nSize == MemSize::Nibble_Upper)
                            {
                                ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(L"4-bit bookmarks are not supported");
                                break;
                            }

                            pBookmarks.AddBookmark(result.nAddress, result.nSize);

                            nSel = ListView_GetNextItem(hList, nSel, LVNI_SELECTED);
                            if (++nCount == 100)
                                break;
                        }
                        pBookmarks.Bookmarks().EndUpdate();

                        if (nCount == 100)
                            ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(L"Can only create 100 new bookmarks at a time.");
                    }

                    return FALSE;
                }

                case IDC_RA_RESULTS_BACK:
                {
                    m_nPage--;
                    EnableWindow(GetDlgItem(hDlg, IDC_RA_RESULTS_BACK), m_nPage > 0);
                    EnableWindow(GetDlgItem(hDlg, IDC_RA_RESULTS_FORWARD), TRUE);

                    SearchResult& sr = m_SearchResults.at(m_nPage);
                    if (sr.m_results.Summary().empty())
                        ListView_SetItemCount(GetDlgItem(hDlg, IDC_RA_MEM_LIST), 1);
                    else
                        ListView_SetItemCount(GetDlgItem(hDlg, IDC_RA_MEM_LIST),
                            gsl::narrow_cast<size_t>(sr.m_results.MatchingAddressCount()) + 2);

                    EnableWindow(GetDlgItem(hDlg, IDC_RA_DOTEST), sr.m_results.MatchingAddressCount() > 0);
                    return FALSE;
                }

                case IDC_RA_RESULTS_FORWARD:
                {
                    m_nPage++;
                    EnableWindow(GetDlgItem(hDlg, IDC_RA_RESULTS_BACK), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_RA_RESULTS_FORWARD), gsl::narrow_cast<size_t>(m_nPage) + 1 < m_SearchResults.size());

                    SearchResult& sr = m_SearchResults.at(m_nPage);
                    if (sr.m_results.Summary().empty())
                        ListView_SetItemCount(GetDlgItem(hDlg, IDC_RA_MEM_LIST), 1);
                    else
                        ListView_SetItemCount(GetDlgItem(hDlg, IDC_RA_MEM_LIST),
                            gsl::narrow_cast<size_t>(sr.m_results.MatchingAddressCount()) + 2);

                    EnableWindow(GetDlgItem(hDlg, IDC_RA_DOTEST), sr.m_results.MatchingAddressCount() > 0);
                    return FALSE;
                }

                case IDC_RA_RESULTS_REMOVE:
                {
                    HWND hList = GetDlgItem(hDlg, IDC_RA_MEM_LIST);
                    int nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

                    if (nSel != -1)
                    {
                        while (m_SearchResults.size() > gsl::narrow_cast<size_t>(m_nPage) + 1)
                            m_SearchResults.pop_back();

                        // copy the selected page, so we can return to it if we want
                        m_SearchResults.push_back(m_SearchResults.at(m_nPage));
                        SearchResult& sr = m_SearchResults.back();
                        m_nPage++;

                        while (nSel >= 0)
                        {
                            sr.m_results.ExcludeMatchingAddress(nSel - 2);

                            ListView_DeleteItem(hList, nSel);
                            nSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
                        }
                    }

                    return FALSE;
                }

                case IDC_RA_WATCHING:
                    switch (HIWORD(wParam))
                    {
                        case CBN_SELCHANGE:
                        {
                            HWND hMemWatch = GetDlgItem(hDlg, IDC_RA_WATCHING);
                            const int nSel = ComboBox_GetCurSel(hMemWatch);
                            if (nSel != CB_ERR)
                            {
                                const TCHAR sAddr[64]{};
                                if (ComboBox_GetLBText(hMemWatch, nSel, sAddr) > 0)
                                {
                                    auto nAddr = ra::ByteAddressFromString(ra::Narrow(sAddr));
                                    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
                                    const auto* pNote = pGameContext.FindCodeNote(nAddr);
                                    if (pNote && !pNote->empty())
                                        SetDlgItemTextW(hDlg, IDC_RA_MEMSAVENOTE, ra::Widen(*pNote).c_str());

                                    GoToAddress(nAddr);
                                }
                            }

                            Invalidate();
                            return TRUE;
                        }
                        case CBN_EDITCHANGE:
                        {
                            TCHAR sAddrBuffer[64];
                            GetDlgItemText(hDlg, IDC_RA_WATCHING, sAddrBuffer, 64);
                            auto nAddr = ra::ByteAddressFromString(ra::Narrow(sAddrBuffer));

                            // disable callback while updating address to prevent updating the text box as the user types
                            g_pMemoryViewer->RemoveNotifyTarget(*this);
                            g_pMemoryViewer->SetAddress(nAddr);
                            g_pMemoryViewer->AddNotifyTarget(*this);

                            OnWatchingMemChange();
                            return TRUE;
                        }

                        default:
                            return FALSE;
                            // return DefWindowProc( hDlg, nMsg, wParam, lParam );
                    }

                default:
                    return FALSE; //	unhandled
            }
        }

        case WM_CLOSE:
            EndDialog(hDlg, 0);
            return TRUE;

        default:
            return FALSE; //	unhandled
    }

    return FALSE;
}

void Dlg_Memory::OnWatchingMemChange()
{
    TCHAR sAddrNative[1024];
    GetDlgItemText(m_hWnd, IDC_RA_WATCHING, sAddrNative, 1024);
    std::string sAddr = ra::Narrow(sAddrNative);
    const auto nAddr = ra::ByteAddressFromString(sAddr);

    UpdateBits();

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    const auto* pNote = pGameContext.FindCodeNote(nAddr);
    SetDlgItemTextW(m_hWnd, IDC_RA_MEMSAVENOTE, (pNote != nullptr) ? pNote->c_str() : L"");

    MemoryViewerControl::destroyEditCaret();

    Invalidate();
}

void Dlg_Memory::RepopulateCodeNotes()
{
    HWND hMemWatch = GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_WATCHING);
    if (hMemWatch == nullptr)
        return;

    auto nAddr = MemoryViewerControl::getWatchedAddress();
    ra::ByteAddress nFirst = 0xFFFFFFFF;
    gsl::index nIndex = -1;
    size_t nScan = 0;

    // find the index of the previous selection
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    pGameContext.EnumerateCodeNotes([&nScan, &nIndex, &nFirst, nAddr](ra::ByteAddress nAddress)
    {
        if (nScan == 0)
            nFirst = nAddress;

        if (nAddress == nAddr)
        {
            nIndex = nScan;
            return false;
        }

        ++nScan;
        return true;
    });

    // reset the combobox
    SetDlgItemText(m_hWnd, IDC_RA_MEMSAVENOTE, TEXT(""));
    SetDlgItemText(hMemWatch, IDC_RA_WATCHING, TEXT(""));
    while (ComboBox_DeleteString(hMemWatch, 0) != CB_ERR)
    {
    }

    pGameContext.EnumerateCodeNotes([hMemWatch](ra::ByteAddress nAddress)
    {
        const std::string sAddr = ra::ByteAddressToString(nAddress);
        ComboBox_AddString(hMemWatch, NativeStr(sAddr).c_str());
        return true;
    });

    // reselect the previous selection - if nFirst is MAX_INT, no items are available
    if (nFirst != 0xFFFFFFFF)
    {
        // if the previous selection was not found, select the first entry
        if (nIndex == -1)
        {
            nIndex = 0;
            nAddr = nFirst;
        }

        ComboBox_SetCurSel(hMemWatch, nIndex);

        const auto* pNote = pGameContext.FindCodeNote(nAddr);
        if (pNote && !pNote->empty())
        {
            SetDlgItemTextW(m_hWnd, IDC_RA_MEMSAVENOTE, pNote->c_str());

            GoToAddress(nAddr);
        }
    }
    else if (m_nCodeNotesGameId != pGameContext.GameId() && pGameContext.GameId() != 0)
    {
        SetDlgItemText(m_hWnd, IDC_RA_WATCHING, TEXT("Loading..."));
    }

    // prevent "Loading..." from being displayed until the game changes
    m_nCodeNotesGameId = pGameContext.GameId();
}

void Dlg_Memory::OnLoad_NewRom()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

    EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_DOTEST), m_SearchResults.size() >= 2);

    if (pGameContext.GameId() == 0)
    {
        SetDlgItemText(g_MemoryDialog.m_hWnd, IDC_RA_WATCHING, TEXT(""));

        EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_ADDNOTE), FALSE);
        EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_MEMSAVENOTE), FALSE);
        EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_REMNOTE), FALSE);
        EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_OPENPAGE), FALSE);
    }
    else
    {
        RepopulateCodeNotes();

        if (pGameContext.GetMode() == ra::data::GameContext::Mode::CompatibilityTest)
        {
            EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_ADDNOTE), FALSE);
            EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_MEMSAVENOTE), FALSE);
            EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_REMNOTE), FALSE);
        }
        else
        {
            EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_ADDNOTE), TRUE);
            EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_MEMSAVENOTE), TRUE);
            EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_REMNOTE), TRUE);
        }

        EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_OPENPAGE), TRUE);
    }

    UpdateMemoryRegions();

    MemoryViewerControl::destroyEditCaret();
}

void Dlg_Memory::UpdateMemoryRegions()
{
    m_nGameRamStart = m_nGameRamEnd = m_nSystemRamStart = m_nSystemRamEnd = 0U;

    for (const auto& pRegion : ra::services::ServiceLocator::Get<ra::data::ConsoleContext>().MemoryRegions())
    {
        if (pRegion.Type == ra::data::ConsoleContext::AddressType::SystemRAM)
        {
            if (m_nSystemRamEnd == 0U)
            {
                m_nSystemRamStart = pRegion.StartAddress;
                m_nSystemRamEnd = pRegion.EndAddress;
            }
            else if (pRegion.StartAddress == m_nSystemRamEnd + 1)
            {
                m_nSystemRamEnd = pRegion.EndAddress;
            }
        }
        else if (pRegion.Type == ra::data::ConsoleContext::AddressType::SaveRAM)
        {
            if (m_nGameRamEnd == 0U)
            {
                m_nGameRamStart = pRegion.StartAddress;
                m_nGameRamEnd = pRegion.EndAddress;
            }
            else if (pRegion.StartAddress == m_nGameRamEnd + 1)
            {
                m_nGameRamEnd = pRegion.EndAddress;
            }
        }
    }

    const auto nTotalBankSize = gsl::narrow_cast<ra::ByteAddress>(ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().TotalMemorySize());
    if (m_nSystemRamEnd >= nTotalBankSize)
    {
        if (nTotalBankSize == 0U)
            m_nSystemRamEnd = 0U;
        else
            m_nSystemRamEnd = nTotalBankSize - 1;
    }

    if (m_nSystemRamEnd != 0U)
    {
        const auto sLabel = ra::StringPrintf("System Memory (%s-%s)", ra::ByteAddressToString(m_nSystemRamStart), ra::ByteAddressToString(m_nSystemRamEnd));
        SetDlgItemText(g_MemoryDialog.m_hWnd, IDC_RA_CBO_SEARCHSYSTEMRAM, NativeStr(sLabel).c_str());
        EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_CBO_SEARCHSYSTEMRAM), (m_nSystemRamEnd - m_nSystemRamStart + 1) < nTotalBankSize);
    }
    else
    {
        SetDlgItemText(g_MemoryDialog.m_hWnd, IDC_RA_CBO_SEARCHSYSTEMRAM, TEXT("System Memory (unspecified)"));
        EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_CBO_SEARCHSYSTEMRAM), FALSE);
    }

    if (m_nGameRamEnd >= nTotalBankSize)
    {
        if (nTotalBankSize == 0U)
        {
            m_nGameRamEnd = 0U;
        }
        else
        {
            m_nGameRamEnd = nTotalBankSize - 1;
            if (m_nGameRamEnd < m_nGameRamStart)
                m_nGameRamStart = m_nGameRamEnd = 0;
        }
    }

    if (m_nGameRamEnd != 0U)
    {
        const auto sLabel = ra::StringPrintf("Game Memory (%s-%s)", ra::ByteAddressToString(m_nGameRamStart), ra::ByteAddressToString(m_nGameRamEnd));
        SetDlgItemText(g_MemoryDialog.m_hWnd, IDC_RA_CBO_SEARCHGAMERAM, NativeStr(sLabel).c_str());
        EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_CBO_SEARCHGAMERAM), TRUE);
    }
    else
    {
        SetDlgItemText(g_MemoryDialog.m_hWnd, IDC_RA_CBO_SEARCHGAMERAM, TEXT("Game Memory (unspecified)"));
        EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_CBO_SEARCHGAMERAM), FALSE);
    }
}

void Dlg_Memory::Invalidate()
{
    if (ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().TotalMemorySize() == 0)
        return;

    if (!IsWindowVisible(m_hWnd))
        return;

    // Update Memory Viewer
    if (g_pMemoryViewer != nullptr)
    {
        g_pMemoryViewer->DoFrame();
        InvalidateMemoryViewer();
    }

    UpdateBits();

    // Update Search Results
    HWND hList = GetDlgItem(m_hWnd, IDC_RA_MEM_LIST);
    if (hList != nullptr)
    {
        InvalidateRect(hList, nullptr, FALSE);

        switch (ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().GetEmulatorId())
        {
            case RA_Libretro:
            case RA_Oricutron:
                UpdateWindow(hList);
                break;
        }
    }
}

void Dlg_Memory::InvalidateMemoryViewer()
{
    if (g_pMemoryViewer->NeedsRedraw())
    {
        HWND hMemViewer = GetDlgItem(g_MemoryDialog.GetHWND(), IDC_RA_MEMTEXTVIEWER);
        InvalidateRect(hMemViewer, nullptr, FALSE);

        // When using SDL, the Windows message queue is never empty (there's a flood of WM_PAINT messages for the
        // SDL window). InvalidateRect only generates a WM_PAINT when the message queue is empty, so we have to
        // explicitly generate (and dispatch) a WM_PAINT message by calling UpdateWindow.
        // Similar code exists in Dlg_Memory::Invalidate for the search results
        switch (ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().GetEmulatorId())
        {
            case RA_Libretro:
            case RA_Oricutron:
                UpdateWindow(hMemViewer);
                break;
        }
    }
}

void Dlg_Memory::UpdateBits() const
{
    TCHAR sNewValue[64] = _T("");

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    if (pEmulatorContext.TotalMemorySize() != 0 && MemoryViewerControl::GetDataSize() == MemSize::EightBit)
    {
        const ra::ByteAddress nAddr = MemoryViewerControl::getWatchedAddress();
        const auto nVal = pEmulatorContext.ReadMemoryByte(nAddr);

        _stprintf_s(sNewValue, 64, _T("      %d %d %d %d %d %d %d %d"), static_cast<int>((nVal & (1 << 7)) != 0),
                    static_cast<int>((nVal & (1 << 6)) != 0), static_cast<int>((nVal & (1 << 5)) != 0),
                    static_cast<int>((nVal & (1 << 4)) != 0), static_cast<int>((nVal & (1 << 3)) != 0),
                    static_cast<int>((nVal & (1 << 2)) != 0), static_cast<int>((nVal & (1 << 1)) != 0),
                    static_cast<int>((nVal & (1 << 0)) != 0));
    }

    TCHAR sOldValue[64];
    GetDlgItemText(m_hWnd, IDC_RA_MEMBITS, sOldValue, 64);
    if (_tcscmp(sNewValue, sOldValue) != 0)
        SetDlgItemText(m_hWnd, IDC_RA_MEMBITS, sNewValue);
}

void Dlg_Memory::GoToAddress(unsigned int nAddr)
{
    g_pMemoryViewer->SetAddress(nAddr);
    SetWatchingAddress(nAddr);
}

void Dlg_Memory::SetWatchingAddress(unsigned int nAddr)
{
    const auto sAddr = ra::ByteAddressToString(nAddr);
    SetDlgItemText(g_MemoryDialog.GetHWND(), IDC_RA_WATCHING, NativeStr(sAddr).c_str());

    OnWatchingMemChange();
}

BOOL Dlg_Memory::IsActive() const noexcept
{
    return (g_MemoryDialog.GetHWND() != nullptr) && (IsWindowVisible(g_MemoryDialog.GetHWND()));
}

inline static constexpr auto ParseAddress(TCHAR* ptr, ra::ByteAddress& address) noexcept
{
    if (ptr == nullptr)
        return ptr;

    if (*ptr == '$')
    {
        ++ptr;
    }
    else if (ptr[0] == '0' && ptr[1] == 'x')
    {
        ptr += 2;
    }

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
            break;

        ++ptr;
    }

    return ptr;
}

bool Dlg_Memory::GetSelectedMemoryRange(ra::ByteAddress& start, ra::ByteAddress& end)
{
    if (IsDlgButtonChecked(m_hWnd, IDC_RA_CBO_SEARCHALL) == BST_CHECKED)
    {
        // all items are in "All" range
        start = 0;
        end = gsl::narrow_cast<ra::ByteAddress>(ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().TotalMemorySize()) - 1;
        return true;
    }

    if (IsDlgButtonChecked(m_hWnd, IDC_RA_CBO_SEARCHSYSTEMRAM) == BST_CHECKED)
    {
        start = m_nSystemRamStart;
        end = m_nSystemRamEnd;
        return (end > start);
    }

    if (IsDlgButtonChecked(m_hWnd, IDC_RA_CBO_SEARCHGAMERAM) == BST_CHECKED)
    {
        start = m_nGameRamStart;
        end = m_nGameRamEnd;
        return (end > start);
    }

    if (IsDlgButtonChecked(m_hWnd, IDC_RA_CBO_SEARCHCUSTOM) == BST_CHECKED)
    {
        std::array<TCHAR, 128> buffer{};
        GetDlgItemText(g_MemoryDialog.m_hWnd, IDC_RA_SEARCHRANGE, buffer.data(), 128);

        auto ptr = ParseAddress(buffer.data(), start);
        Expects(ptr != nullptr);
        while (iswspace(*ptr))
            ++ptr;
        if (*ptr != '-')
            return false;
        ++ptr;

        while (iswspace(*ptr))
            ++ptr;

        ptr = ParseAddress(ptr, end);
        Ensures(ptr != nullptr);
        return (*ptr == '\0');
    }
    return false;
}

void Dlg_Memory::UpdateSearchResult(const ra::services::SearchResults::Result& result, _Out_ unsigned int& nMemVal,
                                    std::wstring& sBuffer)
{
    nMemVal = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().ReadMemory(result.nAddress, result.nSize);

    switch (result.nSize)
    {
        case MemSize::ThirtyTwoBit:
            sBuffer = ra::StringPrintf(L"0x%08x", nMemVal);
            break;
        case MemSize::SixteenBit:
            sBuffer = ra::StringPrintf(L"0x%04x", nMemVal);
            break;
        default:
        case MemSize::EightBit:
            sBuffer = ra::StringPrintf(L"0x%02x", nMemVal);
            break;
        case MemSize::Nibble_Lower:
        case MemSize::Nibble_Upper:
            sBuffer = ra::StringPrintf(L"0x%01x", nMemVal);
            break;
    }
}

bool Dlg_Memory::CompareSearchResult(unsigned int nCurVal, unsigned int nPrevVal)
{
    const unsigned int nVal =
        (m_SearchResults.at(m_nPage).m_bUseLastValue) ? nPrevVal : m_SearchResults.at(m_nPage).m_nLastQueryVal;
    bool bResult = false;

    switch (m_SearchResults.at(m_nPage).m_nCompareType)
    {
        case ComparisonType::Equals:
            bResult = (nCurVal == nVal);
            break;
        case ComparisonType::LessThan:
            bResult = (nCurVal < nVal);
            break;
        case ComparisonType::LessThanOrEqual:
            bResult = (nCurVal <= nVal);
            break;
        case ComparisonType::GreaterThan:
            bResult = (nCurVal > nVal);
            break;
        case ComparisonType::GreaterThanOrEqual:
            bResult = (nCurVal >= nVal);
            break;
        case ComparisonType::NotEqualTo:
            bResult = (nCurVal != nVal);
            break;
        default:
            bResult = false;
            break;
    }
    return bResult;
}

void Dlg_Memory::GenerateResizes(HWND hDlg)
{
    RARect windowRect;
    GetWindowRect(hDlg, &windowRect);
    pDlgMemoryMin.x = windowRect.Width();
    pDlgMemoryMin.y = windowRect.Height();

    using AlignType = ResizeContent::AlignType;
    vDlgMemoryResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_MEMTEXTVIEWER), AlignType::Bottom, true);
}
