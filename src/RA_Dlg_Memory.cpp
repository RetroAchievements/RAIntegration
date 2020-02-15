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
#include "ui\viewmodels\MemorySearchViewModel.hh"
#include "ui\viewmodels\MemoryViewerViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include "ui\win32\bindings\GridTextColumnBinding.hh"

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
static std::unique_ptr<ra::ui::viewmodels::MemorySearchViewModel> g_pMemorySearch;
constexpr int MEMVIEW_MARGIN = 4;
static bool g_bSuppressMemoryViewerInvalidate = false;

Dlg_Memory g_MemoryDialog;

// static
HWND Dlg_Memory::m_hWnd = nullptr;

unsigned int m_nPage = 0;

// Dialog Resizing
std::vector<ResizeContent> vDlgMemoryResize;
POINT pDlgMemoryMin;


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
            {
                g_pMemoryViewer->SetFirstAddress(g_pMemoryViewer->GetFirstAddress() - 32);
                Invalidate();
            }
            else if (GET_WHEEL_DELTA_WPARAM(wParam) < 0)
            {
                g_pMemoryViewer->SetFirstAddress(g_pMemoryViewer->GetFirstAddress() + 32);
                Invalidate();
            }
            return FALSE;

        case WM_LBUTTONUP:
            OnClick({GET_X_LPARAM(lParam) - MEMVIEW_MARGIN, GET_Y_LPARAM(lParam) - MEMVIEW_MARGIN});
            return FALSE;

        case WM_KEYDOWN:
            return (!OnKeyDown(static_cast<UINT>(LOWORD(wParam))));

        case WM_CHAR:
            return (!OnEditInput(static_cast<UINT>(LOWORD(wParam))));

        case WM_SETFOCUS:
            g_pMemoryViewer->OnGotFocus();
            return FALSE;

        case WM_KILLFOCUS:
            g_pMemoryViewer->OnLostFocus();
            return FALSE;

        case WM_GETDLGCODE:
            return DLGC_WANTCHARS | DLGC_WANTARROWS;
    }

    return DefWindowProc(hDlg, uMsg, wParam, lParam);
}

bool MemoryViewerControl::OnKeyDown(UINT nChar)
{
    const bool bShiftHeld = (GetKeyState(VK_SHIFT) < 0);
    const bool bControlHeld = (GetKeyState(VK_CONTROL) < 0);
    bool bHandled = false;

    // multiple properties may change while navigating, we'll do a single Invalidate after we're done
    g_bSuppressMemoryViewerInvalidate = true;

    switch (nChar)
    {
        case VK_RIGHT:
            if (bShiftHeld || bControlHeld)
                g_pMemoryViewer->AdvanceCursorWord();
            else
                g_pMemoryViewer->AdvanceCursor();
            bHandled = true;
            break;

        case VK_LEFT:
            if (bShiftHeld || bControlHeld)
                g_pMemoryViewer->RetreatCursorWord();
            else
                g_pMemoryViewer->RetreatCursor();
            bHandled = true;
            break;

        case VK_DOWN:
            if (bControlHeld)
                g_pMemoryViewer->SetFirstAddress(g_pMemoryViewer->GetFirstAddress() + 0x10);
            else
                g_pMemoryViewer->AdvanceCursorLine();
            bHandled = true;
            break;

        case VK_UP:
            if (bControlHeld)
                g_pMemoryViewer->SetFirstAddress(g_pMemoryViewer->GetFirstAddress() - 0x10);
            else
                g_pMemoryViewer->RetreatCursorLine();
            bHandled = true;
            break;

        case VK_PRIOR: // Page up (!)
            g_pMemoryViewer->RetreatCursorPage();
            bHandled = true;
            break;

        case VK_NEXT: // Page down (!)
            g_pMemoryViewer->AdvanceCursorPage();
            bHandled = true;
            break;

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
            bHandled = true;
            break;

        case VK_END:
            if (bControlHeld)
            {
                const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
                const auto nTotalBytes = gsl::narrow<ra::ByteAddress>(pEmulatorContext.TotalMemorySize());

                g_pMemoryViewer->SetFirstAddress(nTotalBytes & ~0x0F);
                g_pMemoryViewer->SetAddress(nTotalBytes - 1);
            }
            else
            {
                switch (g_pMemoryViewer->GetSize())
                {
                    case MemSize::ThirtyTwoBit:
                        g_pMemoryViewer->SetAddress((g_pMemoryViewer->GetAddress() & ~0x0F) | 0x0C);
                        break;

                    case MemSize::SixteenBit:
                        g_pMemoryViewer->SetAddress((g_pMemoryViewer->GetAddress() & ~0x0F) | 0x0E);
                        break;

                    default:
                        g_pMemoryViewer->SetAddress(g_pMemoryViewer->GetAddress() | 0x0F);
                        break;
                }
            }
            bHandled = true;
            break;
    }

    g_bSuppressMemoryViewerInvalidate = false;

    if (bHandled)
        Invalidate();

    return bHandled;
}

bool MemoryViewerControl::OnEditInput(UINT c)
{
    // multiple properties may change while typing, we'll do a single Invalidate after we're done
    g_bSuppressMemoryViewerInvalidate = true;
    const bool bResult = g_pMemoryViewer->OnChar(gsl::narrow_cast<char>(c));
    g_bSuppressMemoryViewerInvalidate = false;

    if (bResult)
        Invalidate();

    return bResult;
}

void MemoryViewerControl::OnClick(POINT point)
{
    // multiple properties may change while typing, we'll do a single Invalidate after we're done
    g_bSuppressMemoryViewerInvalidate = true;
    g_pMemoryViewer->OnClick(point.x, point.y);
    g_bSuppressMemoryViewerInvalidate = false;

    HWND hOurDlg = GetDlgItem(g_MemoryDialog.GetHWND(), IDC_RA_MEMTEXTVIEWER);
    SetFocus(hOurDlg);

    Invalidate();
}

MemSize MemoryViewerControl::GetDataSize()
{
    return g_pMemoryViewer->GetSize();
}

void MemoryViewerControl::Invalidate()
{
    if (g_pMemoryViewer->NeedsRedraw() && !g_bSuppressMemoryViewerInvalidate)
    {
        HWND hOurDlg = GetDlgItem(g_MemoryDialog.GetHWND(), IDC_RA_MEMTEXTVIEWER);

        InvalidateRect(hOurDlg, nullptr, FALSE);

        // When using SDL, the Windows message queue is never empty (there's a flood of WM_PAINT messages for the
        // SDL window). InvalidateRect only generates a WM_PAINT when the message queue is empty, so we have to
        // explicitly generate (and dispatch) a WM_PAINT message by calling UpdateWindow.
        // Similar code exists in Dlg_Memory::Invalidate for the search results
        switch (ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().GetEmulatorId())
        {
            case RA_Libretro:
            case RA_Oricutron:
                UpdateWindow(hOurDlg);
                break;
        }
    }
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

void Dlg_Memory::OnViewModelIntValueChanged(const ra::ui::IntModelProperty::ChangeArgs& args)
{
    if (args.Property == ra::ui::viewmodels::MemoryViewerViewModel::AddressProperty)
    {
        SetWatchingAddress(g_pMemoryViewer->GetAddress());
    }
}

void Dlg_Memory::OnViewModelBoolValueChanged(gsl::index nIndex, const ra::ui::BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == ra::ui::viewmodels::MemorySearchViewModel::SearchResultViewModel::IsSelectedProperty)
    {
        if (args.tNewValue)
        {
            auto nAddress = g_pMemorySearch->Results().GetItemAt(nIndex)->nAddress;
            if (g_pMemorySearch->ResultMemSize() == MemSize::Nibble_Lower)
                nAddress >>= 1;

            GoToAddress(nAddress);
        }
    }
}

void Dlg_Memory::SetAddressRange()
{
    if (IsDlgButtonChecked(m_hWnd, IDC_RA_CBO_SEARCHCUSTOM) == BST_CHECKED)
    {
        std::array<TCHAR, 1024> nativeBuffer{};
        if (GetDlgItemText(m_hWnd, IDC_RA_SEARCHRANGE, nativeBuffer.data(), gsl::narrow_cast<int>(nativeBuffer.size())))
            g_pMemorySearch->SetFilterRange(ra::Widen(&nativeBuffer.at(0)));
    }
    else if (IsDlgButtonChecked(m_hWnd, IDC_RA_CBO_SEARCHALL) == BST_CHECKED)
    {
        g_pMemorySearch->SetFilterRange(L"");
    }
    else if (IsDlgButtonChecked(m_hWnd, IDC_RA_CBO_SEARCHSYSTEMRAM) == BST_CHECKED)
    {
        g_pMemorySearch->SetFilterRange(ra::StringPrintf(L"%08X-%08X", m_nSystemRamStart, m_nSystemRamEnd));
    }
    else if (IsDlgButtonChecked(m_hWnd, IDC_RA_CBO_SEARCHGAMERAM) == BST_CHECKED)
    {
        g_pMemorySearch->SetFilterRange(ra::StringPrintf(L"%08X-%08X", m_nGameRamStart, m_nGameRamEnd));
    }
}

class StandaloneGridBinding : public ra::ui::win32::bindings::GridBinding
{
public:
    StandaloneGridBinding(HWND hWnd, ra::ui::viewmodels::MemorySearchViewModel& vmViewModel) noexcept
        : ra::ui::win32::bindings::GridBinding(vmViewModel)
    {
        m_hWnd = hWnd;
    }
};

INT_PTR Dlg_Memory::MemoryProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg)
    {
        case WM_INITDIALOG:
        {
            g_MemoryDialog.m_hWnd = hDlg;

            g_pMemoryViewer.reset(new ra::ui::viewmodels::MemoryViewerViewModel());
            g_pMemoryViewer->AddNotifyTarget(*this);

            g_pMemorySearch.reset(new ra::ui::viewmodels::MemorySearchViewModel());
            g_pMemorySearch->Results().AddNotifyTarget(*this);

            // these _will_ go out of scope, but as long as the binding doesn't disable itself, it doesn't matter
            HWND hListbox = GetDlgItem(hDlg, IDC_RA_MEM_LIST);
            m_pSearchGridBinding.reset(new StandaloneGridBinding(hListbox, *g_pMemorySearch));

            auto pAddressColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
                ra::ui::viewmodels::MemorySearchViewModel::SearchResultViewModel::AddressProperty);
            pAddressColumn->SetHeader(L"Address");
            pAddressColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, 60);
            m_pSearchGridBinding->BindColumn(0, std::move(pAddressColumn));

            auto pValueColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
                ra::ui::viewmodels::MemorySearchViewModel::SearchResultViewModel::CurrentValueProperty);
            pValueColumn->SetHeader(L"Value");
            pValueColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, 50);
            m_pSearchGridBinding->BindColumn(1, std::move(pValueColumn));

            auto pDescriptionColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
                ra::ui::viewmodels::MemorySearchViewModel::SearchResultViewModel::DescriptionProperty);
            pDescriptionColumn->SetHeader(L"Description");
            pDescriptionColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Fill, 40);
            pDescriptionColumn->SetTextColorProperty(ra::ui::viewmodels::MemorySearchViewModel::SearchResultViewModel::DescriptionColorProperty);
            m_pSearchGridBinding->BindColumn(2, std::move(pDescriptionColumn));

            m_pSearchGridBinding->BindItems(g_pMemorySearch->Results());
            m_pSearchGridBinding->BindRowColor(ra::ui::viewmodels::MemorySearchViewModel::SearchResultViewModel::RowColorProperty);
            m_pSearchGridBinding->BindIsSelected(ra::ui::viewmodels::MemorySearchViewModel::SearchResultViewModel::IsSelectedProperty);
            m_pSearchGridBinding->Virtualize(ra::ui::viewmodels::MemorySearchViewModel::ScrollOffsetProperty,
                ra::ui::viewmodels::MemorySearchViewModel::ResultCountProperty, [](gsl::index nFrom, gsl::index nTo, bool bIsSelected)
            {
                g_pMemorySearch->SelectRange(nFrom, nTo, bIsSelected);
            });

            ListView_SetExtendedListViewStyle(hListbox, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

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

        case WM_NOTIFY:
        {
            switch (LOWORD(wParam))
            {
                case IDC_RA_MEM_LIST:
                {
                    LPNMHDR pnmHdr;
                    GSL_SUPPRESS_TYPE1{ pnmHdr = reinterpret_cast<LPNMHDR>(lParam); }
                    switch (pnmHdr->code)
                    {
                        case LVN_ITEMCHANGING:
                        {
                            LPNMLISTVIEW pnmListView;
                            GSL_SUPPRESS_TYPE1{ pnmListView = reinterpret_cast<LPNMLISTVIEW>(pnmHdr); }
                            SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, m_pSearchGridBinding->OnLvnItemChanging(pnmListView));
                            return TRUE;
                        }

                        case LVN_ITEMCHANGED:
                        {
                            LPNMLISTVIEW pnmListView;
                            GSL_SUPPRESS_TYPE1{ pnmListView = reinterpret_cast<LPNMLISTVIEW>(pnmHdr); }
                            m_pSearchGridBinding->OnLvnItemChanged(pnmListView);
                            return 0;
                        }

                        case LVN_ODSTATECHANGED:
                        {
                            LPNMLVODSTATECHANGE pnmStateChanged;
                            GSL_SUPPRESS_TYPE1{ pnmStateChanged = reinterpret_cast<LPNMLVODSTATECHANGE>(pnmHdr); }
                            m_pSearchGridBinding->OnLvnOwnerDrawStateChanged(pnmStateChanged);
                            return 0;
                        }

                        case NM_CLICK:
                        {
                            NMITEMACTIVATE* pnmItemActivate;
                            GSL_SUPPRESS_TYPE1{ pnmItemActivate = reinterpret_cast<NMITEMACTIVATE*>(lParam); }
                            m_pSearchGridBinding->OnNmClick(pnmItemActivate);
                            return 0;
                        }

                        case NM_CUSTOMDRAW:
                        {
                            NMLVCUSTOMDRAW* pnmCustomDraw;
                            GSL_SUPPRESS_TYPE1{ pnmCustomDraw = reinterpret_cast<NMLVCUSTOMDRAW*>(lParam); }
                            SetWindowLongPtr(m_hWnd, DWLP_MSGRESULT, m_pSearchGridBinding->OnCustomDraw(pnmCustomDraw));
                            return 1;
                        }

                        case LVN_GETDISPINFO:
                        {
                            NMLVDISPINFO* plvdi;
                            GSL_SUPPRESS_TYPE1{ plvdi = reinterpret_cast<NMLVDISPINFO*>(lParam); }
                            m_pSearchGridBinding->OnLvnGetDispInfo(*plvdi);
                            return 1;
                        }
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
            if (pEmulatorContext.TotalMemorySize() == 0 || g_pMemoryViewer->GetSize() != MemSize::EightBit)
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
                        const auto nAddr = g_pMemoryViewer->GetAddress();
                        auto nVal = pEmulatorContext.ReadMemoryByte(nAddr);
                        nVal ^= (1 << nBit);
                        pEmulatorContext.WriteMemoryByte(nAddr, nVal);

                        MemoryViewerControl::Invalidate();

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
                    const ComparisonType nCmpType =
                        static_cast<ComparisonType>(ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_RA_CBO_CMPTYPE)));
                    g_pMemorySearch->SetComparisonType(nCmpType);

                    if (IsDlgButtonChecked(hDlg, IDC_RA_CBO_GIVENVAL) == BST_CHECKED)
                        g_pMemorySearch->SetValueType(ra::ui::viewmodels::MemorySearchViewModel::ValueType::Constant);
                    else
                        g_pMemorySearch->SetValueType(ra::ui::viewmodels::MemorySearchViewModel::ValueType::LastKnownValue);

                    std::array<TCHAR, 1024> nativeBuffer{};
                    if (GetDlgItemText(hDlg, IDC_RA_TESTVAL, nativeBuffer.data(), gsl::narrow_cast<int>(nativeBuffer.size())))
                        g_pMemorySearch->SetFilterValue(ra::Widen(&nativeBuffer.at(0)));

                    g_pMemorySearch->ApplyFilter();

                    EnableWindow(GetDlgItem(hDlg, IDC_RA_RESULTS_BACK), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_RA_RESULTS_FORWARD), FALSE);

                    EnableWindow(GetDlgItem(hDlg, IDC_RA_DOTEST), g_pMemorySearch->GetResultCount() > 0);
                    return TRUE;
                }

                case IDC_RA_MEMVIEW8BIT:
                    g_pMemoryViewer->SetSize(MemSize::EightBit);
                    SetDlgItemText(hDlg, IDC_RA_MEMBITS_TITLE, TEXT("Bits: 7 6 5 4 3 2 1 0"));
                    UpdateBits();
                    return FALSE;

                case IDC_RA_MEMVIEW16BIT:
                    g_pMemoryViewer->SetSize(MemSize::SixteenBit);
                    SetDlgItemText(hDlg, IDC_RA_MEMBITS_TITLE, TEXT(""));
                    SetDlgItemText(m_hWnd, IDC_RA_MEMBITS, TEXT(""));
                    return FALSE;

                case IDC_RA_MEMVIEW32BIT:
                    g_pMemoryViewer->SetSize(MemSize::ThirtyTwoBit);
                    SetDlgItemText(hDlg, IDC_RA_MEMBITS_TITLE, TEXT(""));
                    SetDlgItemText(m_hWnd, IDC_RA_MEMBITS, TEXT(""));
                    return FALSE;

                case IDC_RA_CBO_4BIT:
                    g_pMemorySearch->SetSearchType(ra::ui::viewmodels::MemorySearchViewModel::SearchType::FourBit);
                    SetAddressRange();
                    g_pMemorySearch->BeginNewSearch();
                    if (g_pMemorySearch->GetResultCount() > 0)
                        EnableWindow(GetDlgItem(hDlg, IDC_RA_DOTEST), TRUE);
                    return FALSE;

                case IDC_RA_CBO_8BIT:
                    g_pMemorySearch->SetSearchType(ra::ui::viewmodels::MemorySearchViewModel::SearchType::EightBit);
                    SetAddressRange();
                    g_pMemorySearch->BeginNewSearch();
                    if (g_pMemorySearch->GetResultCount() > 0)
                        EnableWindow(GetDlgItem(hDlg, IDC_RA_DOTEST), TRUE);
                    return FALSE;

                case IDC_RA_CBO_16BIT:
                    g_pMemorySearch->SetSearchType(ra::ui::viewmodels::MemorySearchViewModel::SearchType::SixteenBit);
                    SetAddressRange();
                    g_pMemorySearch->BeginNewSearch();
                    if (g_pMemorySearch->GetResultCount() > 0)
                        EnableWindow(GetDlgItem(hDlg, IDC_RA_DOTEST), TRUE);
                    return FALSE;

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
                    const ra::ByteAddress nAddr = g_pMemoryViewer->GetAddress();
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
                    const ra::ByteAddress nAddr = g_pMemoryViewer->GetAddress();
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
                    pBookmarks.AddBookmark(g_pMemoryViewer->GetAddress(), g_pMemoryViewer->GetSize());
                    return FALSE;
                }

                case IDC_RA_RESULTS_BOOKMARK:
                    g_pMemorySearch->BookmarkSelected();
                    return FALSE;

                case IDC_RA_RESULTS_BACK:
                    g_pMemorySearch->PreviousPage();
                    EnableWindow(GetDlgItem(hDlg, IDC_RA_RESULTS_BACK), m_nPage > 0);
                    EnableWindow(GetDlgItem(hDlg, IDC_RA_RESULTS_FORWARD), TRUE);
                    return FALSE;

                case IDC_RA_RESULTS_FORWARD:
                    g_pMemorySearch->PreviousPage();
                    EnableWindow(GetDlgItem(hDlg, IDC_RA_RESULTS_BACK), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_RA_RESULTS_FORWARD), TRUE);
                    return FALSE;

                case IDC_RA_RESULTS_REMOVE:
                    g_pMemorySearch->ExcludeSelected();
                    return FALSE;

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

    Invalidate();
}

void Dlg_Memory::RepopulateCodeNotes()
{
    HWND hMemWatch = GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_WATCHING);
    if (hMemWatch == nullptr)
        return;

    auto nAddr = g_pMemoryViewer->GetAddress();
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
        MemoryViewerControl::Invalidate();
    }

    UpdateBits();

    // Update Search Results
    if (g_pMemorySearch != nullptr)
    {
        g_pMemorySearch->DoFrame();

        if (g_pMemorySearch->NeedsRedraw())
        {
            HWND hListbox = GetDlgItem(m_hWnd, IDC_RA_MEM_LIST);

            InvalidateRect(hListbox, nullptr, FALSE);

            // When using SDL, the Windows message queue is never empty (there's a flood of WM_PAINT messages for the
            // SDL window). InvalidateRect only generates a WM_PAINT when the message queue is empty, so we have to
            // explicitly generate (and dispatch) a WM_PAINT message by calling UpdateWindow.
            // Similar code exists in Dlg_Memory::Invalidate for the search results
            switch (ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().GetEmulatorId())
            {
                case RA_Libretro:
                case RA_Oricutron:
                    UpdateWindow(hListbox);
                    break;
            }
        }
    }
}

void Dlg_Memory::UpdateBits() const
{
    TCHAR sNewValue[64] = _T("");

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    if (pEmulatorContext.TotalMemorySize() != 0 && g_pMemoryViewer->GetSize() == MemSize::EightBit)
    {
        const ra::ByteAddress nAddr = g_pMemoryViewer->GetAddress();
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

void Dlg_Memory::GenerateResizes(HWND hDlg)
{
    RARect windowRect;
    GetWindowRect(hDlg, &windowRect);
    pDlgMemoryMin.x = windowRect.Width();
    pDlgMemoryMin.y = windowRect.Height();

    using AlignType = ResizeContent::AlignType;
    vDlgMemoryResize.emplace_back(::GetDlgItem(hDlg, IDC_RA_MEMTEXTVIEWER), AlignType::Bottom, true);
}
