#include "MemoryInspectorDialog.hh"

#include "RA_Resource.h"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include "ui\win32\bindings\GridAddressColumnBinding.hh"
#include "ui\win32\bindings\GridLookupColumnBinding.hh"
#include "ui\win32\bindings\GridNumberColumnBinding.hh"
#include "ui\win32\bindings\GridTextColumnBinding.hh"

using ra::ui::viewmodels::MemoryInspectorViewModel;
using ra::ui::viewmodels::MemorySearchViewModel;
using ra::ui::viewmodels::MemoryViewerViewModel;
using ra::ui::win32::bindings::GridColumnBinding;

namespace ra {
namespace ui {
namespace win32 {

bool MemoryInspectorDialog::Presenter::IsSupported(const ra::ui::WindowViewModelBase& vmViewModel) noexcept
{
    return (dynamic_cast<const MemoryInspectorViewModel*>(&vmViewModel) != nullptr);
}

void MemoryInspectorDialog::Presenter::ShowModal(ra::ui::WindowViewModelBase& vmViewModel, HWND)
{
    ShowWindow(vmViewModel);
}

void MemoryInspectorDialog::Presenter::ShowWindow(ra::ui::WindowViewModelBase& vmViewModel)
{
    auto* vmMemoryBookmarks = dynamic_cast<MemoryInspectorViewModel*>(&vmViewModel);
    Expects(vmMemoryBookmarks != nullptr);

    if (m_pDialog == nullptr)
    {
        m_pDialog.reset(new MemoryInspectorDialog(*vmMemoryBookmarks));
        if (!m_pDialog->CreateDialogWindow(MAKEINTRESOURCE(IDD_RA_MEMORY), this))
            RA_LOG_ERR("Could not create Memory Inspector dialog!");
    }

    m_pDialog->ShowDialogWindow();
}

void MemoryInspectorDialog::Presenter::OnClosed() noexcept { m_pDialog.reset(); }

// ------------------------------------

void MemoryInspectorDialog::SearchResultsGridBinding::Invalidate()
{
    if (GetViewModel<MemorySearchViewModel>().NeedsRedraw())
    {
        GridBinding::Invalidate();

        ControlBinding::ForceRepaint(m_hWnd);
    }
}

void MemoryInspectorDialog::SearchResultsGridBinding::OnLvnItemChanged(const LPNMLISTVIEW pnmListView)
{
    GridBinding::OnLvnItemChanged(pnmListView);

    if (pnmListView->uNewState & LVIS_SELECTED)
    {
        if (ListView_GetItemState(m_hWnd, pnmListView->iItem, LVIS_FOCUSED))
        {
            auto& vmMemory = GetViewModel<MemorySearchViewModel>();
            const auto sValue = vmMemory.Results().GetItemValue(gsl::narrow_cast<gsl::index>(pnmListView->iItem) - m_nScrollOffset,
                MemorySearchViewModel::SearchResultViewModel::AddressProperty);
            const auto nAddress = ra::ByteAddressFromString(ra::Narrow(sValue));

            ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryInspector.SetCurrentAddress(nAddress);
        }
    }
}

void MemoryInspectorDialog::SearchResultsGridBinding::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == MemorySearchViewModel::ResultMemSizeProperty)
    {
        int nWidth;
        const auto& vmMemory = GetViewModel<MemorySearchViewModel>();
        constexpr int nCharWidth = 6;
        constexpr int nPadding = 6;
        switch (vmMemory.ResultMemSize())
        {
            case MemSize::Nibble_Lower: nWidth = nCharWidth * (1 + 2) + nPadding * 2; break;
            case MemSize::EightBit:     nWidth = nCharWidth * (2 + 2) + nPadding * 2; break;
            default:
            case MemSize::SixteenBit:   nWidth = nCharWidth * (4 + 2) + nPadding * 2; break;
            case MemSize::ThirtyTwoBit: nWidth = nCharWidth * (8 + 2) + nPadding * 2; break;
        }

        m_vColumns.at(1)->SetWidth(GridColumnBinding::WidthType::Pixels, nWidth);
        UpdateLayout();
    }

    GridBinding::OnViewModelIntValueChanged(args);
}


MemoryInspectorDialog::MemoryInspectorDialog(MemoryInspectorViewModel& vmMemoryInspector)
    : DialogBase(vmMemoryInspector),
      m_bindSearchRanges(vmMemoryInspector.Search()),
      m_bindSearchRange(vmMemoryInspector.Search()),
      m_bindSearchType(vmMemoryInspector.Search()),
      m_bindComparison(vmMemoryInspector.Search()),
      m_bindValueType(vmMemoryInspector.Search()),
      m_bindFilterValue(vmMemoryInspector.Search()),
      m_bindSearchResults(vmMemoryInspector.Search()),
      m_bindAddress(vmMemoryInspector),
      m_bindNoteText(vmMemoryInspector),
      m_bindViewer8Bit(vmMemoryInspector.Viewer()),
      m_bindViewer16Bit(vmMemoryInspector.Viewer()),
      m_bindViewer32Bit(vmMemoryInspector.Viewer()),
      m_bindViewer(vmMemoryInspector.Viewer())
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::Near, "Memory Inspector");
    m_bindWindow.AddChildViewModel(vmMemoryInspector.Search());

    // Search Range
    m_bindSearchRanges.BindItems(vmMemoryInspector.Search().PredefinedFilterRanges(),
        ra::ui::viewmodels::LookupItemViewModel::IdProperty,
        ra::ui::viewmodels::LookupItemViewModel::LabelProperty);
    m_bindSearchRanges.BindSelectedItem(MemorySearchViewModel::PredefinedFilterRangeProperty);

    m_bindSearchRange.BindText(MemorySearchViewModel::FilterRangeProperty);

    // New Search
    m_bindSearchType.BindItems(vmMemoryInspector.Search().SearchTypes());
    m_bindSearchType.BindSelectedItem(MemorySearchViewModel::SearchTypeProperty);

    // Filter
    m_bindComparison.BindItems(vmMemoryInspector.Search().ComparisonTypes());
    m_bindComparison.BindSelectedItem(MemorySearchViewModel::ComparisonTypeProperty);

    m_bindValueType.BindItems(vmMemoryInspector.Search().ValueTypes());
    m_bindValueType.BindSelectedItem(MemorySearchViewModel::ValueTypeProperty);

    m_bindFilterValue.BindText(vmMemoryInspector.Search().FilterValueProperty);
    m_bindWindow.BindEnabled(IDC_RA_RESET_FILTER, MemorySearchViewModel::CanBeginNewSearchProperty);
    m_bindWindow.BindEnabled(IDC_RA_COMPARISON, MemorySearchViewModel::CanFilterProperty);
    m_bindWindow.BindEnabled(IDC_RA_SPECIAL_FILTER, MemorySearchViewModel::CanFilterProperty);
    m_bindWindow.BindEnabled(IDC_RA_FILTER_VALUE, MemorySearchViewModel::CanEditFilterValueProperty);
    m_bindWindow.BindEnabled(IDC_RA_APPLY_FILTER, MemorySearchViewModel::CanFilterProperty);
    m_bindWindow.BindEnabled(IDC_RA_CONTINUOUS_FILTER, MemorySearchViewModel::CanFilterProperty);

    // Results
    m_bindWindow.BindLabel(IDC_RA_RESULT_COUNT, MemorySearchViewModel::ResultCountTextProperty);
    m_bindWindow.BindLabel(IDC_RA_RESULT_FILTER, MemorySearchViewModel::FilterSummaryProperty);
    m_bindWindow.BindLabel(IDC_RA_RESULT_PAGE, MemorySearchViewModel::SelectedPageProperty);
    m_bindWindow.BindEnabled(IDC_RA_RESULTS_BACK, MemorySearchViewModel::CanGoToPreviousPageProperty);
    m_bindWindow.BindEnabled(IDC_RA_RESULTS_FORWARD, MemorySearchViewModel::CanGoToNextPageProperty);
    m_bindWindow.BindEnabled(IDC_RA_RESULTS_REMOVE, MemorySearchViewModel::HasSelectionProperty);
    m_bindWindow.BindEnabled(IDC_RA_RESULTS_BOOKMARK, MemorySearchViewModel::HasSelectionProperty);

    auto pAddressColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        MemorySearchViewModel::SearchResultViewModel::AddressProperty);
    pAddressColumn->SetHeader(L"Address");
    pAddressColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, 60);
    m_bindSearchResults.BindColumn(0, std::move(pAddressColumn));

    auto pValueColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        MemorySearchViewModel::SearchResultViewModel::CurrentValueProperty);
    pValueColumn->SetHeader(L"Value");
    pValueColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Pixels, 50);
    m_bindSearchResults.BindColumn(1, std::move(pValueColumn));

    auto pDescriptionColumn = std::make_unique<ra::ui::win32::bindings::GridTextColumnBinding>(
        MemorySearchViewModel::SearchResultViewModel::DescriptionProperty);
    pDescriptionColumn->SetHeader(L"Description");
    pDescriptionColumn->SetWidth(ra::ui::win32::bindings::GridColumnBinding::WidthType::Fill, 40);
    pDescriptionColumn->SetTextColorProperty(MemorySearchViewModel::SearchResultViewModel::DescriptionColorProperty);
    m_bindSearchResults.BindColumn(2, std::move(pDescriptionColumn));

    m_bindSearchResults.BindItems(vmMemoryInspector.Search().Results());
    m_bindSearchResults.BindRowColor(MemorySearchViewModel::SearchResultViewModel::RowColorProperty);
    m_bindSearchResults.BindIsSelected(MemorySearchViewModel::SearchResultViewModel::IsSelectedProperty);
    m_bindSearchResults.Virtualize(MemorySearchViewModel::ScrollOffsetProperty,
        MemorySearchViewModel::ScrollMaximumProperty, [&vmMemoryInspector](gsl::index nFrom, gsl::index nTo, bool bIsSelected)
    {
        vmMemoryInspector.Search().SelectRange(nFrom, nTo, bIsSelected);
    });

    // Code Notes
    m_bindAddress.BindText(MemoryInspectorViewModel::CurrentAddressTextProperty, ra::ui::win32::bindings::TextBoxBinding::UpdateMode::KeyPress);
    m_bindNoteText.BindText(MemoryInspectorViewModel::CurrentAddressNoteProperty);
    m_bindWindow.BindEnabled(IDC_RA_NOTE_TEXT, MemoryInspectorViewModel::CanModifyNotesProperty);
    m_bindWindow.BindEnabled(IDC_RA_ADD_NOTE, MemoryInspectorViewModel::CanModifyNotesProperty);
    m_bindWindow.BindEnabled(IDC_RA_DELETE_NOTE, MemoryInspectorViewModel::CanModifyNotesProperty);

    // Memory Viewer
    m_bindViewer8Bit.BindCheck(MemoryViewerViewModel::SizeProperty, ra::etoi(MemSize::EightBit));
    m_bindViewer16Bit.BindCheck(MemoryViewerViewModel::SizeProperty, ra::etoi(MemSize::SixteenBit));
    m_bindViewer32Bit.BindCheck(MemoryViewerViewModel::SizeProperty, ra::etoi(MemSize::ThirtyTwoBit));
    m_bindWindow.BindLabel(IDC_RA_MEMBITS, MemoryInspectorViewModel::CurrentAddressBitsProperty);

    // Resize behavior
    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_GBX_SEARCH, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_SEARCHRANGES, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_SEARCHRANGE, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_SEARCHTYPE, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_RESET_FILTER, Anchor::Top | Anchor::Left);

    SetAnchor(IDC_RA_GBX_FILTER, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_COMPARISON, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_SPECIAL_FILTER, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_FILTER_VALUE, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_APPLY_FILTER, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_CONTINUOUS_FILTER, Anchor::Top | Anchor::Right);

    SetAnchor(IDC_RA_GBX_RESULTS, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_RESULT_COUNT, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_RESULT_FILTER, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_RESULTS_BACK, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_RESULT_PAGE, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_RESULTS_FORWARD, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_RESULTS_REMOVE, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_RESULTS_BOOKMARK, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_RESULTS, Anchor::Top | Anchor::Left | Anchor::Right);

    SetAnchor(IDC_RA_GBX_NOTES, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_ADDRESS, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_VIEW_CODENOTES, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_ADDBOOKMARK, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_NOTE_TEXT, Anchor::Top | Anchor::Left | Anchor::Right);
    SetAnchor(IDC_RA_ADD_NOTE, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_DELETE_NOTE, Anchor::Top | Anchor::Right);

    SetAnchor(IDC_RA_MEMVIEW_8BIT, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_MEMVIEW_16BIT, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_MEMVIEW_32BIT, Anchor::Top | Anchor::Left);

    SetAnchor(IDC_RA_MEMBITS_TITLE, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_MEMBITS, Anchor::Top | Anchor::Right);

    SetAnchor(IDC_RA_MEMVIEWER, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);
}

BOOL MemoryInspectorDialog::OnInitDialog()
{
    // Search Range
    m_bindSearchRanges.SetControl(*this, IDC_RA_SEARCHRANGES);
    ::SendMessage(GetDlgItem(GetHWND(), IDC_RA_SEARCHRANGES), CB_SETDROPPEDWIDTH, 200, 0);

    m_bindSearchRange.SetControl(*this, IDC_RA_SEARCHRANGE);

    // New Search
    m_bindSearchType.SetControl(*this, IDC_RA_SEARCHTYPE);

    // Filter
    m_bindComparison.SetControl(*this, IDC_RA_COMPARISON);
    m_bindValueType.SetControl(*this, IDC_RA_SPECIAL_FILTER);
    m_bindFilterValue.SetControl(*this, IDC_RA_FILTER_VALUE);

    // Results
    m_bindSearchResults.SetControl(*this, IDC_RA_RESULTS);

    // Code Notes
    m_bindAddress.SetControl(*this, IDC_RA_ADDRESS);
    m_bindNoteText.SetControl(*this, IDC_RA_NOTE_TEXT);

    // Memory Viewer
    m_bindViewer8Bit.SetControl(*this, IDC_RA_MEMVIEW_8BIT);
    m_bindViewer16Bit.SetControl(*this, IDC_RA_MEMVIEW_16BIT);
    m_bindViewer32Bit.SetControl(*this, IDC_RA_MEMVIEW_32BIT);
    m_bindViewer.SetControl(*this, IDC_RA_MEMVIEWER);

    SetWindowFont(GetDlgItem(GetHWND(), IDC_RA_MEMBITS), GetStockObject(SYSTEM_FIXED_FONT), TRUE);
    SetWindowFont(GetDlgItem(GetHWND(), IDC_RA_MEMBITS_TITLE), GetStockObject(SYSTEM_FIXED_FONT), TRUE);

    ShowWindow(GetDlgItem(GetHWND(), IDC_RA_CONTINUOUS_FILTER), SW_HIDE);

    return DialogBase::OnInitDialog();
}

BOOL MemoryInspectorDialog::OnCommand(WORD nCommand)
{
    switch (nCommand)
    {
        case IDC_RA_RESET_FILTER:
        {
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->Search().BeginNewSearch();

            return TRUE;
        }

        case IDC_RA_APPLY_FILTER:
        {
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->Search().ApplyFilter();

            return TRUE;
        }

        case IDC_RA_RESULTS_BACK:
        {
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->Search().PreviousPage();

            return TRUE;
        }

        case IDC_RA_RESULTS_FORWARD:
        {
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->Search().NextPage();

            return TRUE;
        }

        case IDC_RA_RESULTS_REMOVE:
        {
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->Search().ExcludeSelected();

            return TRUE;
        }

        case IDC_RA_RESULTS_BOOKMARK:
        {
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->Search().BookmarkSelected();

            return TRUE;
        }

        case IDC_RA_VIEW_CODENOTES:
        {
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->OpenNotesList();

            return TRUE;
        }

        case IDC_RA_ADDBOOKMARK:
        {
            const auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->BookmarkCurrentAddress();

            return TRUE;
        }

        case IDC_RA_ADD_NOTE:
        {
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->SaveCurrentAddressNote();

            return TRUE;
        }

        case IDC_RA_DELETE_NOTE:
        {
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->DeleteCurrentAddressNote();

            return TRUE;
        }
    }

    return DialogBase::OnCommand(nCommand);
}

INT_PTR CALLBACK MemoryInspectorDialog::DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_LBUTTONDBLCLK:
        {
            // ignore if alt/shift/control pressed
            if (wParam != 1)
                break;

            // ignore if memory not avaialble
            const auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>();
            if (pEmulatorContext.TotalMemorySize() == 0)
                break;

            // ignore if not 8-bit mode
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector->Viewer().GetSize() != MemSize::EightBit)
                break;

            POINT ptCursor{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            HWND hChild;
            hChild = ChildWindowFromPoint(GetHWND(), ptCursor);

            const auto hMemBits = GetDlgItem(GetHWND(), IDC_RA_MEMBITS);
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
                        vmMemoryInspector->ToggleBit(nBit);
                }
            }
            break;
        }

        default:
            break;
    }

    return DialogBase::DialogProc(hDlg, uMsg, wParam, lParam);
}


} // namespace win32
} // namespace ui
} // namespace ra
