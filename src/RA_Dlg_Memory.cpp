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
#include "ui\win32\bindings\MemoryViewerControlBinding.hh"

#include "ui\drawing\gdi\GDISurface.hh"

#ifndef ID_OK
#define ID_OK 1024
#endif
#ifndef ID_CANCEL
#define ID_CANCEL 1025
#endif

_CONSTANT_VAR MIN_RESULTS_TO_DUMP = 500000U;
_CONSTANT_VAR MIN_SEARCH_PAGE_SIZE = 50U;

constexpr int MEMVIEW_MARGIN = 4;

static ra::ui::viewmodels::MemoryViewerViewModel* g_pMemoryViewer;
static std::unique_ptr<ra::ui::win32::bindings::MemoryViewerControlBinding> g_pMemoryViewerBinding;
static std::unique_ptr<ra::ui::viewmodels::MemorySearchViewModel> g_pMemorySearch;

Dlg_Memory g_MemoryDialog;

// static
HWND Dlg_Memory::m_hWnd = nullptr;

unsigned int m_nPage = 0;

// Dialog Resizing
std::vector<ResizeContent> vDlgMemoryResize;
POINT pDlgMemoryMin;

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

void Dlg_Memory::OnEndGameLoad()
{
    ra::ByteAddress nFirstAddress = 0U;
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    pGameContext.EnumerateCodeNotes([&nFirstAddress](ra::ByteAddress nAddress)
    {
        nFirstAddress = nAddress;
        return false;
    });

    GoToAddress(nFirstAddress);
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

class StandaloneMemoryViewerControlBinding : public ra::ui::win32::bindings::MemoryViewerControlBinding
{
public:
    StandaloneMemoryViewerControlBinding(HWND hControl, ra::ui::viewmodels::MemoryViewerViewModel& vmMemoryViewer)
        : ra::ui::win32::bindings::MemoryViewerControlBinding(vmMemoryViewer)
    {
        m_hWnd = hControl;
        GSL_SUPPRESS_TYPE1 SetWindowLongPtr(hControl, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }
};

INT_PTR Dlg_Memory::MemoryProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg)
    {
        case WM_INITDIALOG:
        {
            g_MemoryDialog.m_hWnd = hDlg;

            auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
            g_pMemoryViewer = &pWindowManager.MemoryInspector.Viewer();
            g_pMemoryViewer->AddNotifyTarget(*this);

            g_pMemoryViewerBinding.reset(new StandaloneMemoryViewerControlBinding(GetDlgItem(hDlg, IDC_RA_MEMTEXTVIEWER), *g_pMemoryViewer));

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

            ra::services::ServiceLocator::GetMutable<ra::data::GameContext>().AddNotifyTarget(*this);

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

            SetDlgItemText(hDlg, IDC_RA_WATCHING, TEXT(""));

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

                case IDC_RA_VIEW_CODENOTES:
                {
                    auto& pWindowManager = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>();
                    pWindowManager.CodeNotes.Show();
                    return true;
                }

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
                        case EN_CHANGE:
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

void Dlg_Memory::OnLoad_NewRom()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();

    if (pGameContext.GameId() == 0)
    {
        SetWindowText(g_MemoryDialog.m_hWnd, TEXT("Memory Inspector [no game loaded]"));
        SetDlgItemText(g_MemoryDialog.m_hWnd, IDC_RA_WATCHING, TEXT(""));

        EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_ADDNOTE), FALSE);
        EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_MEMSAVENOTE), FALSE);
        EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_REMNOTE), FALSE);
        EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_OPENPAGE), FALSE);
    }
    else
    {
        if (pGameContext.GetMode() == ra::data::GameContext::Mode::CompatibilityTest)
        {
            SetWindowText(g_MemoryDialog.m_hWnd, TEXT("Memory Inspector [compatibility mode]"));

            EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_ADDNOTE), FALSE);
            EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_MEMSAVENOTE), FALSE);
            EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_REMNOTE), FALSE);
        }
        else
        {
            SetWindowText(g_MemoryDialog.m_hWnd, TEXT("Memory Inspector"));

            EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_ADDNOTE), TRUE);
            EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_MEMSAVENOTE), TRUE);
            EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_REMNOTE), TRUE);
        }

        EnableWindow(GetDlgItem(g_MemoryDialog.m_hWnd, IDC_RA_OPENPAGE), TRUE);
        SetWatchingAddress(0U);
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
        g_pMemoryViewerBinding->Invalidate();
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
