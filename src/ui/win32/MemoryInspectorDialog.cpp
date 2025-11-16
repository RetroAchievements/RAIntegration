#include "MemoryInspectorDialog.hh"

#include "RA_Resource.h"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include "ui\win32\bindings\GridAddressColumnBinding.hh"
#include "ui\win32\bindings\GridLookupColumnBinding.hh"
#include "ui\win32\bindings\GridNumberColumnBinding.hh"
#include "ui\win32\bindings\GridTextColumnBinding.hh"

#include "util\EnumOps.hh"
#include "util\Log.hh"

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

void MemoryInspectorDialog::SearchResultsGridBinding::OnLvnItemChanged(const LPNMLISTVIEW pnmListView)
{
    GridBinding::OnLvnItemChanged(pnmListView);

    if (pnmListView->uNewState & LVIS_SELECTED)
    {
        if (ListView_GetItemState(m_hWnd, pnmListView->iItem, LVIS_FOCUSED))
        {
            ra::services::SearchResult pResult{};

            auto& vmSearch = GetViewModel<MemorySearchViewModel>();
            if (!vmSearch.GetResult(pnmListView->iItem, pResult))
                return;

            auto& pMemoryInspector = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryInspector;
            pMemoryInspector.SetCurrentAddress(pResult.nAddress);
        }
    }
}

void MemoryInspectorDialog::SearchResultsGridBinding::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == MemorySearchViewModel::ResultMemSizeProperty ||
        args.Property == MemorySearchViewModel::TotalMemorySizeProperty)
    {
        UpdateColumnWidths();
        UpdateLayout();
    }

    GridBinding::OnViewModelIntValueChanged(args);
}

void MemoryInspectorDialog::SearchResultsGridBinding::UpdateColumnWidths()
{
    int nWidth = 0;
    const auto& vmMemory = GetViewModel<MemorySearchViewModel>();
    const auto nSize = vmMemory.ResultMemSize();
    constexpr int nCharWidth = 6;
    constexpr int nPadding = 6;

    // value column
    auto nMaxChars = (ra::data::MemSizeBits(nSize) + 3) / 4; // 4 bits per nibble
    if (nSize == MemSize::BitCount)
        nMaxChars = 9;
    else if (nMaxChars == 0)
        nMaxChars = 16;
    nWidth = nCharWidth * (nMaxChars + 2) + nPadding * 2;
    m_vColumns.at(1)->SetWidth(GridColumnBinding::WidthType::Pixels, nWidth);

    // address column
    nWidth = ra::ui::win32::bindings::GridAddressColumnBinding::CalculateWidth();
    if (nSize == MemSize::Nibble_Lower)
        nWidth += nCharWidth; // 0x1234L
    m_vColumns.at(0)->SetWidth(GridColumnBinding::WidthType::Pixels, nWidth);
}

// ------------------------------------

class SearchResultValueColumnBinding : public ra::ui::win32::bindings::GridTextColumnBinding
{
public:
    SearchResultValueColumnBinding(const StringModelProperty& pBoundProperty) noexcept
        : ra::ui::win32::bindings::GridTextColumnBinding(pBoundProperty)
    {
    }

    std::wstring GetTooltip(const ra::ui::ViewModelCollectionBase& vmItems, gsl::index nIndex) const override
    {
        const auto* pItem = dynamic_cast<const ra::ui::viewmodels::MemorySearchViewModel::SearchResultViewModel*>(vmItems.GetViewModelAt(nIndex));
        Expects(pItem != nullptr);

        const auto& pMemorySearchViewModel = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryInspector.Search();
        return pMemorySearchViewModel.GetTooltip(*pItem);
    }
};

// ------------------------------------

MemoryInspectorDialog::MemoryInspectorDialog(MemoryInspectorViewModel& vmMemoryInspector)
    : DialogBase(vmMemoryInspector),
      m_bindSearchRanges(vmMemoryInspector.Search()),
      m_bindSearchRange(vmMemoryInspector.Search()),
      m_bindSearchType(vmMemoryInspector.Search()),
      m_bindSearchAligned(vmMemoryInspector.Search()),
      m_bindComparison(vmMemoryInspector.Search()),
      m_bindValueType(vmMemoryInspector.Search()),
      m_bindFilterValue(vmMemoryInspector.Search()),
      m_bindSearchResults(vmMemoryInspector.Search()),
      m_bindAddress(vmMemoryInspector),
      m_bindNoteText(vmMemoryInspector),
      m_bindViewer8Bit(vmMemoryInspector.Viewer()),
      m_bindViewer16Bit(vmMemoryInspector.Viewer()),
      m_bindViewer32Bit(vmMemoryInspector.Viewer()),
      m_bindViewer32BitBE(vmMemoryInspector.Viewer()),
      m_bindViewer(vmMemoryInspector.Viewer())
{
    m_bindWindow.SetInitialPosition(RelativePosition::After, RelativePosition::Near, "Memory Inspector");
    m_bindWindow.AddChildViewModel(vmMemoryInspector.Search());

    // Search Range
    m_bindSearchRanges.BindItems(vmMemoryInspector.Search().PredefinedFilterRanges(),
        ra::ui::viewmodels::LookupItemViewModel::IdProperty,
        ra::ui::viewmodels::LookupItemViewModel::LabelProperty);
    m_bindSearchRanges.BindSelectedItem(MemorySearchViewModel::PredefinedFilterRangeProperty);
    m_bindSearchRanges.SetDropDownWidth(250);

    m_bindSearchRange.BindText(MemorySearchViewModel::FilterRangeProperty);

    // New Search
    m_bindSearchType.BindItems(vmMemoryInspector.Search().SearchTypes());
    m_bindSearchType.BindSelectedItem(MemorySearchViewModel::SearchTypeProperty);
    m_bindSearchAligned.BindCheck(MemorySearchViewModel::IsAlignedProperty);
    m_bindWindow.BindEnabled(IDC_RA_CHK_ALIGNED, MemorySearchViewModel::CanAlignProperty);

    // Filter
    m_bindComparison.BindItems(vmMemoryInspector.Search().ComparisonTypes());
    m_bindComparison.BindSelectedItem(MemorySearchViewModel::ComparisonTypeProperty);

    m_bindValueType.BindItems(vmMemoryInspector.Search().ValueTypes());
    m_bindValueType.BindSelectedItem(MemorySearchViewModel::ValueTypeProperty);

    m_bindFilterValue.BindText(vmMemoryInspector.Search().FilterValueProperty);
    m_bindWindow.BindEnabled(IDC_RA_SEARCHRANGES, MemorySearchViewModel::CanBeginNewSearchProperty);
    m_bindWindow.BindEnabled(IDC_RA_SEARCHRANGE, MemorySearchViewModel::CanBeginNewSearchProperty);
    m_bindWindow.BindEnabled(IDC_RA_SEARCHTYPE, MemorySearchViewModel::CanBeginNewSearchProperty);
    m_bindWindow.BindEnabled(IDC_RA_RESET_FILTER, MemorySearchViewModel::CanBeginNewSearchProperty);
    m_bindWindow.BindEnabled(IDC_RA_COMPARISON, MemorySearchViewModel::CanFilterProperty);
    m_bindWindow.BindEnabled(IDC_RA_SPECIAL_FILTER, MemorySearchViewModel::CanFilterProperty);
    m_bindWindow.BindEnabled(IDC_RA_FILTER_VALUE, MemorySearchViewModel::CanEditFilterValueProperty);
    m_bindWindow.BindEnabled(IDC_RA_APPLY_FILTER, MemorySearchViewModel::CanFilterProperty);
    m_bindWindow.BindEnabled(IDC_RA_RESULTS_EXPORT, MemorySearchViewModel::CanFilterProperty);
    m_bindWindow.BindEnabled(IDC_RA_RESULTS_IMPORT, MemorySearchViewModel::CanBeginNewSearchProperty);
    m_bindWindow.BindEnabled(IDC_RA_CONTINUOUS_FILTER, MemorySearchViewModel::CanContinuousFilterProperty);
    m_bindWindow.BindLabel(IDC_RA_CONTINUOUS_FILTER, MemorySearchViewModel::ContinuousFilterLabelProperty);

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

    auto pValueColumn = std::make_unique<SearchResultValueColumnBinding>(
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

    m_bindSearchResults.UpdateColumnWidths();

    m_bindSearchResults.BindItems(vmMemoryInspector.Search().Results());
    m_bindSearchResults.BindRowColor(MemorySearchViewModel::SearchResultViewModel::RowColorProperty);
    m_bindSearchResults.BindIsSelected(MemorySearchViewModel::SearchResultViewModel::IsSelectedProperty);
    m_bindSearchResults.Virtualize(MemorySearchViewModel::ScrollOffsetProperty,
        MemorySearchViewModel::ScrollMaximumProperty,
        [&vmMemoryInspector](gsl::index nFrom, gsl::index nTo, bool bIsSelected)
        {
            vmMemoryInspector.Search().SelectRange(nFrom, nTo, bIsSelected);
        },
        nullptr // Search results are readonly, no need to multi-set values
    );

    // Code Notes
    m_bindAddress.BindText(MemoryInspectorViewModel::CurrentAddressTextProperty, ra::ui::win32::bindings::TextBoxBinding::UpdateMode::KeyPress);
    m_bindAddress.BindKey(VK_UP, [this]()
    {
        auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
        vmMemoryInspector->PreviousNote();
        return true;
    });
    m_bindAddress.BindKey(VK_DOWN, [this]()
    {
        auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
        vmMemoryInspector->NextNote();
        return true;
    });
    m_bindNoteText.BindText(MemoryInspectorViewModel::CurrentAddressNoteProperty, ra::ui::win32::bindings::TextBoxBinding::UpdateMode::Typing);
    m_bindNoteText.BindReadOnly(MemoryInspectorViewModel::IsCurrentAddressNoteReadOnlyProperty);
    m_bindWindow.BindEnabled(IDC_RA_NOTE_TEXT, MemoryInspectorViewModel::CanEditCurrentAddressNoteProperty);
    m_bindWindow.BindEnabled(IDC_RA_PUBLISH_NOTE, MemoryInspectorViewModel::CanPublishCurrentAddressNoteProperty);
    m_bindWindow.BindEnabled(IDC_RA_REVERT_NOTE, MemoryInspectorViewModel::CanRevertCurrentAddressNoteProperty);

    // Memory Viewer
    m_bindViewer8Bit.BindCheck(MemoryViewerViewModel::SizeProperty, ra::etoi(MemSize::EightBit));
    m_bindViewer16Bit.BindCheck(MemoryViewerViewModel::SizeProperty, ra::etoi(MemSize::SixteenBit));
    m_bindViewer32Bit.BindCheck(MemoryViewerViewModel::SizeProperty, ra::etoi(MemSize::ThirtyTwoBit));
    m_bindViewer32BitBE.BindCheck(MemoryViewerViewModel::SizeProperty, ra::etoi(MemSize::ThirtyTwoBitBigEndian));
    m_bindWindow.BindLabel(IDC_RA_MEMBITS, MemoryInspectorViewModel::CurrentAddressBitsProperty);
    m_bindWindow.BindVisible(IDC_RA_MEMBITS_TITLE, MemoryInspectorViewModel::CurrentBitsVisibleProperty);
    m_bindWindow.BindVisible(IDC_RA_MEMBITS, MemoryInspectorViewModel::CurrentBitsVisibleProperty);

    // Resize behavior
    using namespace ra::bitwise_ops;
    SetAnchor(IDC_RA_GBX_SEARCH, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_SEARCHRANGES, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_SEARCHRANGE, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_SEARCHTYPE, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_CHK_ALIGNED, Anchor::Top | Anchor::Left);
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
    SetAnchor(IDC_RA_PUBLISH_NOTE, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_REVERT_NOTE, Anchor::Top | Anchor::Right);

    SetAnchor(IDC_RA_MEMVIEW_8BIT, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_MEMVIEW_16BIT, Anchor::Top | Anchor::Left);
    SetAnchor(IDC_RA_MEMVIEW_32BIT, Anchor::Top | Anchor::Left);

    SetAnchor(IDC_RA_MEMBITS_TITLE, Anchor::Top | Anchor::Right);
    SetAnchor(IDC_RA_MEMBITS, Anchor::Top | Anchor::Right);

    SetAnchor(IDC_RA_MEMVIEWER, Anchor::Top | Anchor::Left | Anchor::Bottom | Anchor::Right);

    SetMinimumSize(496, 458);
}

BOOL MemoryInspectorDialog::OnInitDialog()
{
    // Search Range
    m_bindSearchRanges.SetControl(*this, IDC_RA_SEARCHRANGES);
    m_bindSearchRange.SetControl(*this, IDC_RA_SEARCHRANGE);

    // New Search
    m_bindSearchType.SetControl(*this, IDC_RA_SEARCHTYPE);
    m_bindSearchAligned.SetControl(*this, IDC_RA_CHK_ALIGNED);

    // Filter
    m_bindComparison.SetControl(*this, IDC_RA_COMPARISON);
    m_bindValueType.SetControl(*this, IDC_RA_SPECIAL_FILTER);
    m_bindFilterValue.SetControl(*this, IDC_RA_FILTER_VALUE);

    // Results
    m_bindSearchResults.SetControl(*this, IDC_RA_RESULTS);
    m_bindSearchResults.InitializeTooltips(std::chrono::seconds(3));

    // Code Notes
    m_bindAddress.SetControl(*this, IDC_RA_ADDRESS);
    m_bindNoteText.SetControl(*this, IDC_RA_NOTE_TEXT);

    // Memory Viewer
    m_bindViewer8Bit.SetControl(*this, IDC_RA_MEMVIEW_8BIT);
    m_bindViewer16Bit.SetControl(*this, IDC_RA_MEMVIEW_16BIT);
    m_bindViewer32Bit.SetControl(*this, IDC_RA_MEMVIEW_32BIT);
    m_bindViewer32BitBE.SetControl(*this, IDC_RA_MEMVIEW_32BITBE);
    m_bindViewer.SetControl(*this, IDC_RA_MEMVIEWER);

    SetWindowFont(GetDlgItem(GetHWND(), IDC_RA_MEMBITS), GetStockObject(SYSTEM_FIXED_FONT), TRUE);
    SetWindowFont(GetDlgItem(GetHWND(), IDC_RA_MEMBITS_TITLE), GetStockObject(SYSTEM_FIXED_FONT), TRUE);

    // NOTE: This is the number of Unicode characters allowed. The database stores data
    //       in UTF-8, so this is the upper bound and less will actually be allowed if
    //       non-ASCII characters are used.
    SendMessage(GetDlgItem(GetHWND(), IDC_RA_NOTE_TEXT), EM_SETLIMITTEXT, 65530, 0);

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

        case IDC_RA_CONTINUOUS_FILTER:
        {
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->Search().ToggleContinuousFilter();

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
            {
                vmMemoryInspector->Search().ExcludeSelected();
                m_bindSearchResults.DeselectAll();
            }

            return TRUE;
        }

        case IDC_RA_RESULTS_BOOKMARK:
        {
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->Search().BookmarkSelected();

            return TRUE;
        }

        case IDC_RA_RESULTS_EXPORT:
        {
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->Search().ExportResults();

            return TRUE;
        }

        case IDC_RA_RESULTS_IMPORT: {
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->Search().ImportResults();

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

        case IDC_RA_PUBLISH_NOTE:
        {
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->PublishCurrentAddressNote();

            return TRUE;
        }

        case IDC_RA_REVERT_NOTE:
        {
            auto* vmMemoryInspector = dynamic_cast<MemoryInspectorViewModel*>(&m_vmWindow);
            if (vmMemoryInspector)
                vmMemoryInspector->RevertCurrentAddressNote();

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
            const auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
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
                    {
                        vmMemoryInspector->ToggleBit(nBit);
                        m_bindViewer.Invalidate();
                    }
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
