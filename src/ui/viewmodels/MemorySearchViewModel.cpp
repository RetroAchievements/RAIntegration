#include "MemorySearchViewModel.hh"

#include "context\IConsoleContext.hh"

#include "data\context\EmulatorContext.hh"

#include "services\IClock.hh"
#include "services\IFileSystem.hh"
#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

#include "ui\IDesktop.hh"
#include "ui\viewmodels\FileDialogViewModel.hh"
#include "ui\viewmodels\MemoryRegionsViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\ProgressViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include "RA_Defs.h"

namespace ra {
namespace ui {
namespace viewmodels {

constexpr size_t SEARCH_ROWS_DISPLAYED = 9; // needs to be one higher than actual number displayed for scrolling
constexpr size_t SEARCH_MAX_HISTORY = 50;

constexpr int MEMORY_RANGE_ALL = 0;
constexpr int MEMORY_RANGE_SYSTEM = -1;
constexpr int MEMORY_RANGE_EXTRA = -2;
constexpr int MEMORY_RANGE_CUSTOM = -3;
constexpr int MEMORY_RANGE_MANAGE = -4;

const IntModelProperty MemorySearchViewModel::PredefinedFilterRangeProperty("MemorySearchViewModel", "PredefinedFilterRange", MEMORY_RANGE_ALL);
const StringModelProperty MemorySearchViewModel::FilterRangeProperty("MemorySearchViewModel", "FilterRange", L"");
const IntModelProperty MemorySearchViewModel::SearchTypeProperty("MemorySearchViewModel", "SearchType", ra::etoi(ra::services::SearchType::EightBit));
const BoolModelProperty MemorySearchViewModel::IsAlignedProperty("MemorySearchViewModel", "IsAligned", true);
const BoolModelProperty MemorySearchViewModel::CanAlignProperty("MemorySearchViewModel", "CanAlign", false);
const IntModelProperty MemorySearchViewModel::ComparisonTypeProperty("MemorySearchViewModel", "ComparisonType", ra::etoi(ComparisonType::Equals));
const IntModelProperty MemorySearchViewModel::ValueTypeProperty("MemorySearchViewModel", "ValueType", ra::etoi(ra::services::SearchFilterType::LastKnownValue));
const StringModelProperty MemorySearchViewModel::FilterValueProperty("MemorySearchViewModel", "FilterValue", L"");
const StringModelProperty MemorySearchViewModel::FilterSummaryProperty("MemorySearchViewModel", "FilterSummary", L"");
const IntModelProperty MemorySearchViewModel::ResultCountProperty("MemorySearchViewModel", "ResultCount", 0);
const StringModelProperty MemorySearchViewModel::ResultCountTextProperty("MemorySearchViewModel", "ResultCountText", L"0");
const IntModelProperty MemorySearchViewModel::ResultMemSizeProperty("MemorySearchViewModel", "ResultMemSize", ra::etoi(ra::data::Memory::Size::EightBit));
const IntModelProperty MemorySearchViewModel::TotalMemorySizeProperty("MemorySearchViewModel", "TotalMemorySize", 0);
const IntModelProperty MemorySearchViewModel::ScrollOffsetProperty("MemorySearchViewModel", "ScrollOffset", 0);
const IntModelProperty MemorySearchViewModel::ScrollMaximumProperty("MemorySearchViewModel", "ScrollMaximum", 0);
const StringModelProperty MemorySearchViewModel::SelectedPageProperty("MemorySearchViewModel", "SelectedPageProperty", L"0/0");
const BoolModelProperty MemorySearchViewModel::CanBeginNewSearchProperty("MemorySearchViewModel", "CanBeginNewSearch", true);
const BoolModelProperty MemorySearchViewModel::CanFilterProperty("MemorySearchViewModel", "CanFilter", true);
const BoolModelProperty MemorySearchViewModel::CanEditFilterValueProperty("MemorySearchViewModel", "CanEditFilterValue", true);
const BoolModelProperty MemorySearchViewModel::CanContinuousFilterProperty("MemorySearchViewModel", "CanContinuousFilter", true);
const StringModelProperty MemorySearchViewModel::ContinuousFilterLabelProperty("MemorySearchViewModel", "ContinuousFilterLabel", L"Continuous Filter");
const BoolModelProperty MemorySearchViewModel::CanGoToPreviousPageProperty("MemorySearchViewModel", "CanGoToPreviousPage", true);
const BoolModelProperty MemorySearchViewModel::CanGoToNextPageProperty("MemorySearchViewModel", "CanGoToNextPage", false);
const BoolModelProperty MemorySearchViewModel::HasSelectionProperty("MemorySearchViewModel", "HasSelection", false);

const IntModelProperty MemorySearchViewModel::PredefinedFilterRangeViewModel::StartAddressProperty("PredefinedFilterRangeViewModel", "StartAddress", 0);
const IntModelProperty MemorySearchViewModel::PredefinedFilterRangeViewModel::EndAddressProperty("PredefinedFilterRangeViewModel", "EndAddress", 0);

const StringModelProperty MemorySearchViewModel::SearchResultViewModel::DescriptionProperty("SearchResultViewModel", "Description", L"");
const StringModelProperty MemorySearchViewModel::SearchResultViewModel::AddressProperty("SearchResultViewModel", "Address", L"");
const StringModelProperty MemorySearchViewModel::SearchResultViewModel::CurrentValueProperty("SearchResultViewModel", "CurrentValue", L"");
const BoolModelProperty MemorySearchViewModel::SearchResultViewModel::IsSelectedProperty("SearchResultViewModel", "IsSelected", false);
const IntModelProperty MemorySearchViewModel::SearchResultViewModel::RowColorProperty("SearchResultViewModel", "RowColor", 0);
const IntModelProperty MemorySearchViewModel::SearchResultViewModel::DescriptionColorProperty("SearchResultViewModel", "DescriptionColor", 0);

void MemorySearchViewModel::SearchResultViewModel::UpdateRowColor()
{
    if (!bMatchesFilter)
    {
        // red if value no longer matches filter
        SetRowColor(ra::ui::Color(0xFFFFE0E0));
    }
    else if (bHasBookmark)
    {
        // green if bookmark found
        SetRowColor(ra::ui::Color(0xFFE0FFE0));
    }
    else if (bHasCodeNote)
    {
        // blue if code note found
        SetRowColor(ra::ui::Color(0xFFE0F0FF));
    }
    else if (bHasBeenModified)
    {
        // grey if the filter currently matches, but did not at some point
        SetRowColor(ra::ui::Color(0xFFF0F0F0));
    }
    else
    {
        // default color otherwise
        SetRowColor(ra::ui::Color(ra::to_unsigned(RowColorProperty.GetDefaultValue())));
    }
}

void MemorySearchViewModel::SearchResultViewModel::UpdateCodeNote(const std::wstring& sNote)
{
    if (!sNote.empty())
    {
        bHasCodeNote = true;
        SetDescription(sNote);
        SetDescriptionColor(ra::ui::Color(ra::to_unsigned(DescriptionColorProperty.GetDefaultValue())));
    }
    else
    {
        bHasCodeNote = false;

        const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
        const auto* pRegion = pConsoleContext.GetMemoryRegion(nAddress);
        if (pRegion)
        {
            SetDescription(pRegion->GetDescription());
            SetDescriptionColor(ra::ui::Color(0xFFA0A0A0));
        }
        else
        {
            SetDescription(L"");
            SetDescriptionColor(ra::ui::Color(ra::to_unsigned(DescriptionColorProperty.GetDefaultValue())));
        }
    }
}


MemorySearchViewModel::MemorySearchViewModel()
{
    m_vPredefinedFilterRanges.Add(MEMORY_RANGE_ALL, L"All");
    m_vPredefinedFilterRanges.Add(MEMORY_RANGE_CUSTOM, L"Custom");

    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::FourBit), L"4-bit");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::EightBit), L"8-bit");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::SixteenBit), L"16-bit");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::ThirtyTwoBit), L"32-bit");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::SixteenBitBigEndian), L"16-bit BE");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::ThirtyTwoBitBigEndian), L"32-bit BE");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::BitCount), L"BitCount");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::Float), L"Float");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::FloatBigEndian), L"Float BE");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::Double32), L"Double32");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::Double32BigEndian), L"Double32 BE");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::MBF32), L"MBF32");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::MBF32LE), L"MBF32 LE");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::AsciiText), L"ASCII Text");

    m_vComparisonTypes.Add(ra::etoi(ComparisonType::Equals), L"=");
    m_vComparisonTypes.Add(ra::etoi(ComparisonType::LessThan), L"<");
    m_vComparisonTypes.Add(ra::etoi(ComparisonType::LessThanOrEqual), L"<=");
    m_vComparisonTypes.Add(ra::etoi(ComparisonType::GreaterThan), L">");
    m_vComparisonTypes.Add(ra::etoi(ComparisonType::GreaterThanOrEqual), L">=");
    m_vComparisonTypes.Add(ra::etoi(ComparisonType::NotEqualTo), L"!=");

    m_vValueTypes.Add(ra::etoi(ra::services::SearchFilterType::Constant), L"Constant");
    m_vValueTypes.Add(ra::etoi(ra::services::SearchFilterType::LastKnownValue), L"Last Value");
    m_vValueTypes.Add(ra::etoi(ra::services::SearchFilterType::LastKnownValuePlus), L"Last Value Plus");
    m_vValueTypes.Add(ra::etoi(ra::services::SearchFilterType::LastKnownValueMinus), L"Last Value Minus");
    m_vValueTypes.Add(ra::etoi(ra::services::SearchFilterType::InitialValue), L"Initial Value");

    m_vResults.AddNotifyTarget(*this);

    SetValue(CanBeginNewSearchProperty, false);
    SetValue(CanGoToPreviousPageProperty, false);

    // explicitly set the Memory::Size to some non-selectable value so the first search will
    // trigger a PropertyChanged event.
    SetValue(ResultMemSizeProperty, ra::etoi(ra::data::Memory::Size::Bit0));
}

void MemorySearchViewModel::InitializeNotifyTargets()
{
    auto& pMemoryContext = ra::services::ServiceLocator::GetMutable<ra::context::IEmulatorMemoryContext>();
    pMemoryContext.AddNotifyTarget(*this);
    OnTotalMemorySizeChanged();

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    pGameContext.AddNotifyTarget(*this);
}

void MemorySearchViewModel::OnBeforeActiveGameChanged()
{
    // reset filter list and select All option when starting to change game
    RebuildPredefinedFilterRanges();
    SetPredefinedFilterRange(MEMORY_RANGE_ALL);
}

void MemorySearchViewModel::OnActiveGameChanged()
{
    // rebuild list with custom filters once the game has loaded
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    if (pGameContext.Assets().FindMemoryRegions() != nullptr)
        RebuildPredefinedFilterRanges();
}

void MemorySearchViewModel::OnTotalMemorySizeChanged()
{
    RebuildPredefinedFilterRanges();

    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
    const auto nTotalBankSize = gsl::narrow_cast<ra::data::ByteAddress>(pMemoryContext.TotalMemorySize());
    SetValue(CanBeginNewSearchProperty, (nTotalBankSize > 0U));
    SetValue(TotalMemorySizeProperty, nTotalBankSize);
}

void MemorySearchViewModel::RebuildPredefinedFilterRanges()
{
    ra::data::ByteAddress nExtraRamStart = 0U;
    ra::data::ByteAddress nExtraRamEnd = 0U;
    ra::data::ByteAddress nSystemRamStart = 0U;
    ra::data::ByteAddress nSystemRamEnd = 0U;
    ra::data::ByteAddress nAllRamEnd = 0U;

    std::vector<ra::data::MemoryRegion> vmRanges;
    auto nPreviousAddressType = ra::data::MemoryRegion::Type::Unused;

    for (const auto& pRegion : ra::services::ServiceLocator::Get<ra::context::IConsoleContext>().MemoryRegions())
    {
        nAllRamEnd = pRegion.GetEndAddress();

        switch (pRegion.GetType())
        {
            case ra::data::MemoryRegion::Type::Unused:
            case ra::data::MemoryRegion::Type::VirtualRAM:
            case ra::data::MemoryRegion::Type::ReadOnlyMemory:
            case ra::data::MemoryRegion::Type::HardwareController:
            case ra::data::MemoryRegion::Type::VideoRAM:
                // don't create filter regions for these.
                // reset the previous type to ensure the next block doesn't get merged to the last one.
                nPreviousAddressType = ra::data::MemoryRegion::Type::Unused;
                continue;

            case ra::data::MemoryRegion::Type::SystemRAM:
                if (nSystemRamEnd == 0)
                    nSystemRamStart = pRegion.GetStartAddress();
                nSystemRamEnd = pRegion.GetEndAddress();
                break;

            case ra::data::MemoryRegion::Type::SaveRAM:
                if (nExtraRamEnd == 0)
                    nExtraRamStart = pRegion.GetStartAddress();
                nExtraRamEnd = pRegion.GetEndAddress();
                break;
        }

        if (pRegion.GetType() == nPreviousAddressType && vmRanges.back().GetDescription() == pRegion.GetDescription())
        {
            vmRanges.back().SetEndAddress(pRegion.GetEndAddress());
            continue;
        }

        nPreviousAddressType = pRegion.GetType();
        vmRanges.emplace_back(pRegion.GetStartAddress(), pRegion.GetEndAddress(), pRegion.GetDescription());
    }

    for (const auto& vmRange : vmRanges)
    {
        if (vmRange.GetEndAddress() == nSystemRamEnd && vmRange.GetStartAddress() == nSystemRamStart)
        {
            // system defined range already exists for "All System RAM"
            nSystemRamEnd = 0U;
        }
        else if (vmRange.GetEndAddress() == nExtraRamEnd && vmRange.GetStartAddress() == nExtraRamStart)
        {
            // system defined range already exists for "All Game RAM"
            nExtraRamEnd = 0U;
        }
    }

    m_vPredefinedFilterRanges.BeginUpdate();

    gsl::index nIndex = 0;
    DefinePredefinedFilterRange(nIndex++, MEMORY_RANGE_ALL, L"All", 0U, nAllRamEnd, false);

    if (vmRanges.size() == 1 &&
        vmRanges.front().GetEndAddress() == nAllRamEnd &&
        vmRanges.front().GetStartAddress() == 0)
    {
        // single defined range is "All Memory". don't list it separately.
    }
    else
    {
        if (nSystemRamEnd != 0U && (nSystemRamStart != 0 || nSystemRamEnd != nAllRamEnd))
            DefinePredefinedFilterRange(nIndex++, MEMORY_RANGE_SYSTEM, L"All System Memory", nSystemRamStart, nSystemRamEnd, true);

        if (nExtraRamEnd != 0U && (nExtraRamStart != 0 || nExtraRamEnd != nAllRamEnd))
            DefinePredefinedFilterRange(nIndex++, MEMORY_RANGE_EXTRA, L"All Extra Memory", nExtraRamStart, nExtraRamEnd, true);

        for (const auto& vmRange : vmRanges)
        {
            DefinePredefinedFilterRange(nIndex, gsl::narrow_cast<int>(nIndex),
                vmRange.GetDescription(), vmRange.GetStartAddress(), vmRange.GetEndAddress(), true);
            ++nIndex;
        }
    }

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pMemoryRegions = pGameContext.Assets().FindMemoryRegions();
    if (pMemoryRegions)
    {
        for (const auto& pRegion : pMemoryRegions->CustomRegions())
        {
            DefinePredefinedFilterRange(nIndex, gsl::narrow_cast<int>(nIndex),
                pRegion.GetDescription(), pRegion.GetStartAddress(), pRegion.GetEndAddress(), true);
            ++nIndex;
        }
    }

    DefinePredefinedFilterRange(nIndex++, MEMORY_RANGE_CUSTOM, L"Custom", 0, 0, false);

    if (pGameContext.GameId() != 0)
        DefinePredefinedFilterRange(nIndex++, MEMORY_RANGE_MANAGE, L"Customize...", 0, 0, false);

    while (m_vPredefinedFilterRanges.Count() > gsl::narrow_cast<size_t>(nIndex))
        m_vPredefinedFilterRanges.RemoveAt(m_vPredefinedFilterRanges.Count() - 1);

    m_vPredefinedFilterRanges.EndUpdate();
}

void MemorySearchViewModel::DefinePredefinedFilterRange(gsl::index nIndex, int nId, const std::wstring& sLabel, ra::data::ByteAddress nStartAddress, ra::data::ByteAddress nEndAddress, bool bIncludeRangeInLabel)
{
    auto* pItem = m_vPredefinedFilterRanges.GetItemAt(nIndex);
    if (pItem == nullptr)
    {
        auto pNewItem = std::make_unique<PredefinedFilterRangeViewModel>(nId, sLabel);
        pItem = &m_vPredefinedFilterRanges.Append(std::move(pNewItem));
    }

    pItem->SetId(nId);
    pItem->SetStartAddress(nStartAddress);
    pItem->SetEndAddress(nEndAddress);

    if (bIncludeRangeInLabel)
    {
        const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
        pItem->SetLabel(ra::util::String::Printf(L"%s (%s-%s)", sLabel,
            pMemoryContext.FormatAddress(nStartAddress), pMemoryContext.FormatAddress(nEndAddress)));
    }
    else
    {
        pItem->SetLabel(sLabel);
    }
}

void MemorySearchViewModel::OnPredefinedFilterRangeChanged(const IntModelProperty::ChangeArgs& args)
{
    const auto nValue = args.tNewValue;
    if (nValue == MEMORY_RANGE_ALL)
    {
        m_bSelectingFilter = true;
        SetFilterRange(L"");
        m_bSelectingFilter = false;
        return;
    }

    if (nValue == MEMORY_RANGE_MANAGE)
    {
        MemoryRegionsViewModel vmRegions;
        vmRegions.InitializeRegions();

        const auto& pWindowManager = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>();
        if (vmRegions.ShowModal(pWindowManager.MemoryInspector) == ra::ui::DialogResult::OK)
            RebuildPredefinedFilterRanges();

        // change the selection back to the old value asynchronously as the synchronous
        // flow will continue processing the current args, and we don't want that to
        // overwrite the change back to the old value.
        ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().RunAsync([this, nOldValue = args.tOldValue]()
        {
            Sleep(10); // ensure the other event has been fully handled

            // if the previous item was deleted, just select the all option
            if (m_vPredefinedFilterRanges.FindItemIndex(ra::ui::viewmodels::LookupItemViewModel::IdProperty, nOldValue) == -1)
                SetPredefinedFilterRange(MEMORY_RANGE_ALL);
            else
                SetPredefinedFilterRange(nOldValue);
        });
        return;
    }

    const auto nIndex = m_vPredefinedFilterRanges.FindItemIndex(ra::ui::viewmodels::LookupItemViewModel::IdProperty, nValue);
    if (nIndex == -1)
        return;

    const auto* pEntry = m_vPredefinedFilterRanges.GetItemAt(nIndex);
    Ensures(pEntry != nullptr);

    m_bSelectingFilter = true;
    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
    SetFilterRange(ra::util::String::Printf(L"%s-%s",
        pMemoryContext.FormatAddress(pEntry->GetStartAddress()), pMemoryContext.FormatAddress(pEntry->GetEndAddress())));
    m_bSelectingFilter = false;
}

void MemorySearchViewModel::OnFilterRangeChanged()
{
    ra::data::ByteAddress nStart, nEnd;
    if (!ra::data::models::MemoryRegionsModel::ParseFilterRange(GetFilterRange(), nStart, nEnd))
        return;

    for (const auto& pEntry : m_vPredefinedFilterRanges)
    {
        if (pEntry.GetStartAddress() == nStart && pEntry.GetEndAddress() == nEnd)
        {
            SetPredefinedFilterRange(pEntry.GetId());
            return;
        }
    }

    const auto nIndex = m_vPredefinedFilterRanges.FindItemIndex(ra::ui::viewmodels::LookupItemViewModel::IdProperty, MEMORY_RANGE_CUSTOM);
    if (nIndex == -1)
        return;

    auto* pEntry = m_vPredefinedFilterRanges.GetItemAt(nIndex);
    Ensures(pEntry != nullptr);

    pEntry->SetStartAddress(nStart);
    pEntry->SetEndAddress(nEnd);

    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
    pEntry->SetLabel(ra::util::String::Printf(L"Custom (%s-%s)", pMemoryContext.FormatAddress(nStart), pMemoryContext.FormatAddress(nEnd)));

    SetPredefinedFilterRange(MEMORY_RANGE_CUSTOM);
}

void MemorySearchViewModel::DoFrame()
{
    if (m_vSearchResults.size() < 2)
        return;

    if (m_bIsContinuousFiltering)
    {
        ApplyContinuousFilter();
        return;
    }

    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();

    {
        std::lock_guard lock(m_oMutex);
        auto& pCurrentResults = *m_vSearchResults.at(m_nSelectedSearchResult).get();

        m_vResults.BeginUpdate();

        // NOTE: only processes the visible items
        ra::services::SearchResult pResult;
        std::wstring sFormattedValue;

        gsl::index nIndex = GetScrollOffset();
        for (auto& pRow : m_vResults)
        {
            pCurrentResults.pResults.GetMatchingAddress(nIndex++, pResult);

            const auto nPreviousValue = pResult.nValue;
            UpdateResult(pRow, pCurrentResults.pResults, pResult, false, pMemoryContext);

            // when updating an existing result, check to see if it's been modified and update the tracker
            if (!pRow.bHasBeenModified && pResult.nValue != nPreviousValue)
            {
                pCurrentResults.vModifiedAddresses.insert(pResult.nAddress);
                pRow.bHasBeenModified = true;
            }

            pRow.UpdateRowColor();
        }
    }

    // EndUpdate has to be outside the lock as it may raise UI events
    m_vResults.EndUpdate();
}

void MemorySearchViewModel::ClearResults()
{
    if (m_bIsContinuousFiltering)
        ToggleContinuousFilter();

    {
        std::lock_guard lock(m_oMutex);

        m_vSelectedAddresses.clear();
        m_vSearchResults.clear();
        m_vResults.Clear();
        m_nSelectedSearchResult = 0;
    }

    SetValue(FilterSummaryProperty, FilterSummaryProperty.GetDefaultValue());
    SetValue(SelectedPageProperty, SelectedPageProperty.GetDefaultValue());
    SetValue(ScrollOffsetProperty, 0);
    SetValue(ScrollMaximumProperty, 0);
    SetValue(ResultCountProperty, 0);
    SetValue(ResultMemSizeProperty, ResultMemSizeProperty.GetDefaultValue());
}

void MemorySearchViewModel::BeginNewSearch()
{
    ra::data::ByteAddress nStart, nEnd;
    if (!ra::data::models::MemoryRegionsModel::ParseFilterRange(GetFilterRange(), nStart, nEnd))
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Invalid address range");
        return;
    }

    if (m_bIsContinuousFiltering)
        ToggleContinuousFilter();

    DispatchMemoryRead([this, nStart, nEnd]() { BeginNewSearch(nStart, nEnd); });
}

void MemorySearchViewModel::BeginNewSearch(ra::data::ByteAddress nStart, ra::data::ByteAddress nEnd)
{
    auto nSearchType = GetSearchType();
    bool bIsAligned = false;
    if (IsAligned())
    {
        const auto nAlignedSearchType = ra::services::GetAlignedSearchType(nSearchType);
        if (nAlignedSearchType != ra::services::SearchType::None)
        {
            nSearchType = nAlignedSearchType;
            bIsAligned = true;
        }
    }

    std::unique_ptr<SearchResult> pResult;
    pResult.reset(new SearchResult());
    pResult->pResults.Initialize(nStart, gsl::narrow<size_t>(nEnd) - nStart + 1, nSearchType);
    pResult->sSummary = ra::util::String::Printf(L"New %s%s Search",
        SearchTypes().GetLabelForId(ra::etoi(GetSearchType())),
        bIsAligned ? L" (aligned)" : L"");

    SetValue(FilterSummaryProperty, pResult->sSummary);
    SetValue(SelectedPageProperty, L"1/1");
    SetValue(ScrollOffsetProperty, 0);
    SetValue(ScrollMaximumProperty, 0);
    SetValue(ResultCountProperty, gsl::narrow_cast<int>(pResult->pResults.MatchingAddressCount()));
    SetValue(ResultMemSizeProperty, ra::etoi(pResult->pResults.GetSize()));

    {
        std::lock_guard lock(m_oMutex);

        m_vSearchResults.clear();
        m_vSearchResults.push_back(std::move(pResult));
        m_nSelectedSearchResult = 0;
        m_vSelectedAddresses.clear();

        m_vResults.BeginUpdate();
        while (m_vResults.Count() > 0)
            m_vResults.RemoveAt(m_vResults.Count() - 1);
        m_vResults.EndUpdate();
    }
}

void MemorySearchViewModel::AddNewPage(std::unique_ptr<SearchResult>&& pNewPage)
{
    // remove any search history after the current node
    while (m_vSearchResults.size() - 1 > m_nSelectedSearchResult)
        m_vSearchResults.pop_back();

    // add a new search history entry
    m_vSearchResults.push_back(std::move(pNewPage));
    if (m_vSearchResults.size() > SEARCH_MAX_HISTORY + 1)
    {
        // always discard the second search result in case the user wants to do an initial search later
        m_vSearchResults.erase(m_vSearchResults.begin() + 1);
    }
    else
    {
        ++m_nSelectedSearchResult;
    }
}

void MemorySearchViewModel::ApplyFilter()
{
    if (m_vSearchResults.empty())
        return;

    DispatchMemoryRead([this]() { DoApplyFilter(); });
}

void MemorySearchViewModel::DoApplyFilter()
{
    const std::wstring sEmptyString;
    const auto* sValue = GetValue(CanEditFilterValueProperty) ? &GetFilterValue() : &sEmptyString;

    SearchResult& pPreviousResult = *m_vSearchResults.at(m_nSelectedSearchResult).get();
    std::unique_ptr<SearchResult> pResult;
    pResult.reset(new SearchResult());

    if (!ApplyFilter(*pResult, pPreviousResult, GetComparisonType(), GetValueType(), *sValue))
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Invalid filter value");
        return;
    }

    // if this isn't the first filter being applied, and the result count hasn't changed
    const auto nMatches = pResult->pResults.MatchingAddressCount();
    if (nMatches == pPreviousResult.pResults.MatchingAddressCount() && m_vSearchResults.size() > 2)
    {
        // check to see if the same filter was applied.
        if (pResult->pResults.GetFilterComparison() == pPreviousResult.pResults.GetFilterComparison() &&
            pResult->pResults.GetFilterType() == pPreviousResult.pResults.GetFilterType() &&
            pResult->pResults.GetFilterValue() == pPreviousResult.pResults.GetFilterValue())
        {
            // same filter applied, result set didn't change, if applying an equality filter we know the memory
            // didn't change, so don't generate a new result set. do clear the modified addresses list.
            if (pResult->pResults.GetFilterComparison() == ComparisonType::Equals)
            {
                pPreviousResult.vModifiedAddresses.clear();
                return;
            }
        }
    }

    ra::util::StringBuilder builder;
    builder.Append(ComparisonTypes().GetLabelForId(ra::etoi(GetComparisonType())));
    builder.Append(L" ");
    switch (GetValueType())
    {
        case ra::services::SearchFilterType::Constant:
            builder.Append(GetFilterValue());
            break;

        case ra::services::SearchFilterType::LastKnownValue:
            builder.Append(L"Last Known");
            break;

        case ra::services::SearchFilterType::LastKnownValuePlus:
            builder.Append(L"Last +");
            builder.Append(GetFilterValue());
            break;

        case ra::services::SearchFilterType::LastKnownValueMinus:
            builder.Append(L"Last -");
            builder.Append(GetFilterValue());
            break;

        case ra::services::SearchFilterType::InitialValue:
            builder.Append(L"Initial");
            break;
    }
    pResult->sSummary = builder.ToWString();
    SetValue(FilterSummaryProperty, pResult->sSummary);

    AddNewPage(std::move(pResult));
    DispatchMemoryRead([this]() { ChangePage(m_nSelectedSearchResult); });
}

bool MemorySearchViewModel::ApplyFilter(SearchResult& pResult, const SearchResult& pPreviousResult,
    ComparisonType nComparisonType, ra::services::SearchFilterType nValueType, const std::wstring& sValue)
{
    if (nValueType == ra::services::SearchFilterType::InitialValue)
    {
        SearchResult const& pInitialResult = *m_vSearchResults.front().get();
        return pResult.pResults.Initialize(pInitialResult.pResults, pPreviousResult.pResults,
            nComparisonType, nValueType, sValue);
    }
    else
    {
        return pResult.pResults.Initialize(pPreviousResult.pResults, nComparisonType, nValueType, sValue);
    }
}

void MemorySearchViewModel::ToggleContinuousFilter()
{
    if (m_bIsContinuousFiltering)
    {
        m_bIsContinuousFiltering = false;
        SetValue(CanFilterProperty, GetResultCount() > 0);
        SetValue(ContinuousFilterLabelProperty, ContinuousFilterLabelProperty.GetDefaultValue());
    }
    else
    {
        DispatchMemoryRead([this]() {
            // apply the filter before disabling CanFilter or the filter value will be ignored
            ApplyFilter();

            m_bIsContinuousFiltering = true;
            m_tLastContinuousFilter = ra::services::ServiceLocator::Get<ra::services::IClock>().UpTime();

            SetValue(CanFilterProperty, false);
            SetValue(ContinuousFilterLabelProperty, L"Stop Filtering");
        });
    }
}

void MemorySearchViewModel::ApplyContinuousFilter()
{
    const SearchResult& pResult = *m_vSearchResults.back().get();

    // if there are more than 1000 results, don't filter every frame.
    const auto nResults = pResult.pResults.MatchingAddressCount();
    if (nResults > 1000)
    {
        const auto tNow = ra::services::ServiceLocator::Get<ra::services::IClock>().UpTime();

        auto nElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(tNow - m_tLastContinuousFilterCheck);
        m_tLastContinuousFilterCheck = tNow;

        auto nElapsedMilliseconds = gsl::narrow_cast<size_t>(nElapsed.count());
        if (nElapsedMilliseconds >= 200)
        {
            // if at least 200ms have passed since the last check, assume the user is
            // frame-stepping and just allow the continuous filter to be applied.
        }
        else
        {
            // formula is "number of results / 100" ms between filterings. i.e.:
            // * for 10000 results, only filter every 100ms
            // * for 50000 results, only filter every 500ms
            // * for 100000 results, only filter every second
            // up to a max of one filter every ten seconds at 1000000 or more results
            nElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(tNow - m_tLastContinuousFilter);
            nElapsedMilliseconds = gsl::narrow_cast<size_t>(nElapsed.count());
            if (nElapsedMilliseconds < 10000 && nElapsedMilliseconds < nResults / 100)
                return;
        }

        m_tLastContinuousFilter = tNow;
    }

    // apply the current filter
    std::unique_ptr<SearchResult> pNewResult;
    pNewResult.reset(new SearchResult());
    ApplyFilter(*pNewResult, pResult, pResult.pResults.GetFilterComparison(),
        pResult.pResults.GetFilterType(), pResult.pResults.GetFilterString());
    pNewResult->sSummary = pResult.sSummary;
    const auto nNewResults = pNewResult->pResults.MatchingAddressCount();

    // replace the last item with the new results
    {
        std::lock_guard lock(m_oMutex);
        m_vSearchResults.back().swap(pNewResult);
    }

    ChangePage(m_nSelectedSearchResult);

    if (nNewResults == 0)
    {
        // if no results are remaining, stop filtering
        ToggleContinuousFilter();
    }
}

void MemorySearchViewModel::ChangePage(size_t nNewPage)
{
    // NOTE: UpdateResults reads memory from the emulator. As such, this function should
    //       only be called from the thread that calls DoFrame(). That can be managed
    //       by using DispatchMemoryRead().
    {
        std::lock_guard lock(m_oMutex);
        m_nSelectedSearchResult = nNewPage;
    }
    SetValue(SelectedPageProperty, ra::util::String::Printf(L"%u/%u", m_nSelectedSearchResult, m_vSearchResults.size() - 1));

    const auto& pResult = *m_vSearchResults.at(nNewPage).get();
    const auto nMatches = pResult.pResults.MatchingAddressCount();
    SetValue(ResultCountProperty, gsl::narrow_cast<int>(nMatches));
    SetValue(FilterSummaryProperty, pResult.sSummary);

    // attempt to keep the current results in the results window
    const auto nNewScrollOffset = (nMatches <= SEARCH_ROWS_DISPLAYED) ? 0
        : std::min(CalculatePreserveResultsScrollOffset(pResult),
                   gsl::narrow_cast<unsigned>(nMatches - SEARCH_ROWS_DISPLAYED));

    // prevent scrolling from triggering a call to UpdateResults - we'll do that in a few lines.
    m_bScrolling = true;
    // note: update maximum first - it can affect offset. then update offset to the value we want.
    SetValue(ScrollMaximumProperty, gsl::narrow_cast<int>(nMatches));
    SetValue(ScrollOffsetProperty, nNewScrollOffset);
    m_bScrolling = false;

    m_vSelectedAddresses.clear();

    UpdateResults();

    SetValue(CanGoToPreviousPageProperty, (nNewPage > 1));
    SetValue(CanGoToNextPageProperty, (nNewPage < m_vSearchResults.size() - 1));
}

unsigned MemorySearchViewModel::CalculatePreserveResultsScrollOffset(const SearchResult& pResult) const
{
    const auto nScrollOffset = GetScrollOffset();
    if (nScrollOffset == 0)
        return 0;

    // Results() is virtualizing. The first item in Results() is the item at the scroll offset
    const auto* nItemAtScrollOffset = m_vResults.GetItemAt(0);
    if (nItemAtScrollOffset == nullptr)
        return 0;

    const auto nAddressAtScrollOffset = nItemAtScrollOffset->nAddress;

    ra::services::SearchResult pSearchResult;
    if (pResult.pResults.GetMatchingAddress(nScrollOffset, pSearchResult) &&
        pSearchResult.nAddress == nAddressAtScrollOffset)
    {
        // item offset didn't change
        return nScrollOffset;
    }

    // find new offset of item (or nearest item)
    // assume a non-match means the item was filtered out and will be at an index
    // lower than nScrollOffset
    gsl::index nLow = 0;
    gsl::index nHigh = nScrollOffset;

    // if the found item is lower than the search item, search forward instead of back
    if (pSearchResult.nAddress < nAddressAtScrollOffset)
    {
        nLow = gsl::narrow_cast<gsl::index>(nScrollOffset + 1);
        nHigh = gsl::narrow_cast<gsl::index>(pResult.pResults.MatchingAddressCount());
    }

    do
    {
        const gsl::index nMid = (nLow + nHigh) / 2;
        if (!pResult.pResults.GetMatchingAddress(nMid, pSearchResult))
        {
            // nMid outside results range, set new upper limit
            nHigh = gsl::narrow_cast<gsl::index>(pResult.pResults.MatchingAddressCount());
        }
        else if (pSearchResult.nAddress == nAddressAtScrollOffset)
        {
            // found a match
            return gsl::narrow_cast<unsigned int>(nMid);
        }
        else
        {
            // apply bsearch algorithm
            if (pSearchResult.nAddress > nAddressAtScrollOffset)
                nHigh = nMid;
            else
                nLow = nMid + 1;
        }
    } while (nLow < nHigh);

    // no match found, return the closest item before the search item (if one exists)
    if (nHigh < 1)
        return 0;

    return gsl::narrow_cast<unsigned int>(nHigh - 1);
}

void MemorySearchViewModel::UpdateResults()
{
    // NOTE: UpdateResult reads memory from the emulator. As such, this function should
    //       only be called from the thread that calls DoFrame(). That can be managed
    //       by using DispatchMemoryRead().

    // intentionally release the lock before calling EndUpdate to ensure callbacks aren't within the lock scope
    {
        std::lock_guard lock(m_oMutex);

        if (m_vSearchResults.size() < 2)
            return;

        const auto& pCurrentResults = *m_vSearchResults.at(m_nSelectedSearchResult).get();
        ra::services::SearchResult pResult;
        const auto nIndex = gsl::narrow_cast<gsl::index>(GetScrollOffset());

        const auto& vmBookmarks = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
        const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
        const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();

        m_vResults.RemoveNotifyTarget(*this);
        m_vResults.BeginUpdate();

        unsigned int nRow = 0;
        while (nRow < SEARCH_ROWS_DISPLAYED)
        {
            if (!pCurrentResults.pResults.GetMatchingAddress(nIndex + nRow, pResult))
                break;

            auto* pRow = m_vResults.GetItemAt(nRow);
            if (pRow == nullptr)
            {
                pRow = &m_vResults.Add();
                Ensures(pRow != nullptr);
            }

            pRow->nAddress = pResult.nAddress;

            auto sAddress = pMemoryContext.FormatAddress(pResult.nAddress);
            switch (pResult.nSize)
            {
                case ra::data::Memory::Size::NibbleLower:
                    sAddress.push_back('L');
                    pRow->nAddress <<= 1;
                    break;

                case ra::data::Memory::Size::NibbleUpper:
                    sAddress.push_back('U');
                    pRow->nAddress = (pRow->nAddress << 1) | 1;
                    break;
            }

            pRow->SetAddress(ra::util::String::Widen(sAddress));

            UpdateResult(*pRow, pCurrentResults.pResults, pResult, true, pMemoryContext);

            const auto pCodeNote = (pCodeNotes != nullptr) ?
                pCodeNotes->FindCodeNote(pResult.nAddress, pResult.nSize) : std::wstring(L"");
            pRow->UpdateCodeNote(pCodeNote);

            pRow->SetSelected(m_vSelectedAddresses.find(pRow->nAddress) != m_vSelectedAddresses.end());

            pRow->bHasBookmark = vmBookmarks.HasBookmark(pResult.nAddress);
            pRow->bHasBeenModified = (pCurrentResults.vModifiedAddresses.find(pRow->nAddress) != pCurrentResults.vModifiedAddresses.end());
            pRow->UpdateRowColor();
            ++nRow;
        }

        while (m_vResults.Count() > nRow)
            m_vResults.RemoveAt(m_vResults.Count() - 1);
    }

    m_vResults.EndUpdate();
    m_vResults.AddNotifyTarget(*this);
}

void MemorySearchViewModel::UpdateResult(SearchResultViewModel& vmResult,
    const ra::services::SearchResults& pResults, ra::services::SearchResult& pResult,
    bool bForceFilterCheck, const ra::context::IEmulatorMemoryContext& pMemoryContext)
{
    std::wstring sFormattedValue;
    pResult.nValue = vmResult.nCurrentValue;

    if (pResults.UpdateValue(pResult, &sFormattedValue, pMemoryContext) || bForceFilterCheck)
    {
        if (pResults.GetFilterType() == ra::services::SearchFilterType::InitialValue)
            vmResult.bMatchesFilter = pResults.MatchesFilter(m_vSearchResults.front()->pResults, pResult);
        else
            vmResult.bMatchesFilter = pResults.MatchesFilter(m_vSearchResults.at(m_nSelectedSearchResult - 1)->pResults, pResult);

        vmResult.SetCurrentValue(sFormattedValue);
        vmResult.nCurrentValue = pResult.nValue;
    }
}

bool MemorySearchViewModel::GetResult(gsl::index nIndex, ra::services::SearchResult& pResult) const
{
    memset(&pResult, 0, sizeof(pResult));

    std::lock_guard lock(m_oMutex);

    if (m_vSearchResults.size() < 2)
        return false;

    const auto& pCurrentResults = *m_vSearchResults.at(m_nSelectedSearchResult).get();
    return pCurrentResults.pResults.GetMatchingAddress(nIndex, pResult);
}


std::wstring MemorySearchViewModel::GetTooltip(const SearchResultViewModel& vmResult) const
{
    std::wstring sTooltip = ra::util::String::Printf(L"%s\n%s | Current\n",
        vmResult.GetAddress(), vmResult.GetCurrentValue());

    const auto& pCompareResults = m_vSearchResults.at(m_nSelectedSearchResult)->pResults;
    ra::data::ByteAddress nAddress = vmResult.nAddress;
    auto nSize = pCompareResults.GetSize();

    if (nSize == ra::data::Memory::Size::NibbleLower)
    {
        if (nAddress & 1)
            nSize = ra::data::Memory::Size::NibbleUpper;
        nAddress >>= 1;
    }

    sTooltip.append(pCompareResults.GetFormattedValue(nAddress, nSize));
    sTooltip.append(L" | Last Filter\n");

    const auto& pInitialResults = m_vSearchResults.front()->pResults;
    sTooltip.append(pInitialResults.GetFormattedValue(nAddress, nSize));
    sTooltip.append(L" | Initial");

    return sTooltip;
}

void MemorySearchViewModel::OnCodeNoteChanged(ra::data::ByteAddress nAddress, const std::wstring&)
{
    if (m_vResults.Count() == 0 || m_vSearchResults.empty())
        return;

    const auto nSize = m_vSearchResults.front()->pResults.GetSize();

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
    if (!pCodeNotes)
        return;

    const auto* pNote = pCodeNotes->FindCodeNoteModel(nAddress);
    if (pNote != nullptr)
    {
        // if updated note is before first visible result, ignore
        if (pNote->GetAddress() + pNote->GetBytes() <= nAddress)
            return;

        // if updated note is after last visible result, ignore
        const auto nBytesForSize = ra::data::Memory::SizeBytes(nSize);
        if (pNote->GetAddress() > m_vResults.GetItemAt(m_vResults.Count() - 1)->nAddress + nBytesForSize)
            return;
    }

    for (auto& pRow : m_vResults)
    {
        const auto sProcessedNote = pCodeNotes->FindCodeNote(pRow.nAddress, nSize);
        pRow.UpdateCodeNote(sProcessedNote);
    }
}

void MemorySearchViewModel::OnCodeNoteMoved(ra::data::ByteAddress nOldAddress, ra::data::ByteAddress nNewAddress, const std::wstring& sNote)
{
    for (auto& pRow : m_vResults)
    {
        if (pRow.nAddress == nOldAddress)
            pRow.UpdateCodeNote(L"");
        else if (pRow.nAddress == nNewAddress)
            pRow.UpdateCodeNote(sNote);
    }
}

static constexpr bool CanEditFilterValue(ra::services::SearchFilterType nFilterType)
{
    switch (nFilterType)
    {
        case ra::services::SearchFilterType::Constant:
        case ra::services::SearchFilterType::LastKnownValuePlus:
        case ra::services::SearchFilterType::LastKnownValueMinus:
            return true;

        default:
            return false;
    }
}

void MemorySearchViewModel::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == CanFilterProperty)
    {
        if (CanEditFilterValue(GetValueType()))
            SetValue(CanEditFilterValueProperty, args.tNewValue);
        else
            SetValue(CanEditFilterValueProperty, false);
    }
    else if (args.Property == CanBeginNewSearchProperty)
    {
        if (args.tNewValue)
        {
            SetValue(CanFilterProperty, GetResultCount() > 0);
            SetValue(CanContinuousFilterProperty, GetResultCount() > 0);
        }
        else
        {
            SetValue(CanFilterProperty, false);
            SetValue(CanContinuousFilterProperty, false);
        }
    }

    ViewModelBase::OnValueChanged(args);
}

void MemorySearchViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == ScrollOffsetProperty)
    {
        if (!m_bScrolling)
            DispatchMemoryRead([this]() { UpdateResults(); });
    }
    else if (args.Property == ResultCountProperty)
    {
        SetValue(ResultCountTextProperty, std::to_wstring(args.tNewValue));
        SetValue(CanFilterProperty, !m_bIsContinuousFiltering && (args.tNewValue > 0));
        SetValue(CanContinuousFilterProperty, args.tNewValue > 0);
    }
    else if (args.Property == ValueTypeProperty)
    {
        SetValue(CanEditFilterValueProperty, CanEditFilterValue(GetValueType()) && GetValue(CanFilterProperty));
    }
    else if (args.Property == PredefinedFilterRangeProperty)
    {
        OnPredefinedFilterRangeChanged(args);
    }
    else if (args.Property == SearchTypeProperty)
    {
        const auto nNewValue = ra::itoe<ra::services::SearchType>(args.tNewValue);
        if (nNewValue == ra::services::SearchType::AsciiText)
            SetValueType(ra::services::SearchFilterType::Constant);

        SetValue(CanAlignProperty, ra::services::GetAlignedSearchType(nNewValue) != ra::services::SearchType::None);
    }

    ViewModelBase::OnValueChanged(args);
}

void MemorySearchViewModel::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    if (args.Property == FilterRangeProperty && !m_bSelectingFilter)
        OnFilterRangeChanged();

    ViewModelBase::OnValueChanged(args);
}

void MemorySearchViewModel::OnViewModelBoolValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == SearchResultViewModel::IsSelectedProperty)
    {
        const auto* pRow = m_vResults.GetItemAt(nIndex);
        if (args.tNewValue)
            m_vSelectedAddresses.insert(pRow->nAddress);
        else
            m_vSelectedAddresses.erase(pRow->nAddress);

        SetValue(HasSelectionProperty, (m_vSelectedAddresses.size() > 0));
    }
}

void MemorySearchViewModel::NextPage()
{
    if (m_nSelectedSearchResult < m_vSearchResults.size() - 1)
        DispatchMemoryRead([this]() { ChangePage(m_nSelectedSearchResult + 1); });
}

void MemorySearchViewModel::PreviousPage()
{
    // at least two pages (initialization and first filter) must be available to go back to
    if (m_nSelectedSearchResult > 1)
    {
        if (m_bIsContinuousFiltering)
            ToggleContinuousFilter();

        DispatchMemoryRead([this]() { ChangePage(m_nSelectedSearchResult - 1); });
    }
}

void MemorySearchViewModel::SelectRange(gsl::index nFrom, gsl::index nTo, bool bValue)
{
    if (m_vSearchResults.empty())
        return;

    const auto& pCurrentResults = m_vSearchResults.back()->pResults;
    if (!bValue && nFrom == 0 && nTo >= gsl::narrow_cast<gsl::index>(pCurrentResults.MatchingAddressCount()) - 1)
    {
        m_vSelectedAddresses.clear();
        return;
    }

    ra::services::SearchResult pResult;

    // ignore IsSelectedProperty events - we'll update the lists directly
    m_vResults.RemoveNotifyTarget(*this);

    if (pCurrentResults.GetSize() == ra::data::Memory::Size::NibbleLower)
    {
        for (auto nIndex = nFrom; nIndex <= nTo; ++nIndex)
        {
            if (pCurrentResults.GetMatchingAddress(nIndex, pResult))
            {
                auto nAddress = pResult.nAddress << 1;
                if (pResult.nSize == ra::data::Memory::Size::NibbleUpper)
                    nAddress |= 1;

                if (bValue)
                    m_vSelectedAddresses.insert(nAddress);
                else
                    m_vSelectedAddresses.erase(nAddress);
            }
        }
    }
    else if (bValue)
    {
        for (auto nIndex = nFrom; nIndex <= nTo; ++nIndex)
        {
            if (pCurrentResults.GetMatchingAddress(nIndex, pResult))
                m_vSelectedAddresses.insert(pResult.nAddress);
        }
    }
    else
    {
        for (auto nIndex = nFrom; nIndex <= nTo; ++nIndex)
        {
            if (pCurrentResults.GetMatchingAddress(nIndex, pResult))
                m_vSelectedAddresses.erase(pResult.nAddress);
        }
    }

    m_vResults.AddNotifyTarget(*this);

    SetValue(HasSelectionProperty, (m_vSelectedAddresses.size() > 0));
}

void MemorySearchViewModel::ExcludeSelected()
{
    if (m_vSelectedAddresses.empty())
        return;

    const auto& pCurrentResults = *m_vSearchResults.at(m_nSelectedSearchResult).get();
    std::unique_ptr<SearchResult> pResult;
    pResult.reset(new SearchResult(pCurrentResults)); // clone current item
    ra::services::SearchResult pItem {};

    const auto nSize = pCurrentResults.pResults.GetSize();
    if (nSize == ra::data::Memory::Size::NibbleLower)
    {
        for (const auto nAddress : m_vSelectedAddresses)
        {
            pItem.nAddress = nAddress >> 1;
            pItem.nSize = (nAddress & 1) ? ra::data::Memory::Size::NibbleUpper : ra::data::Memory::Size::NibbleLower;
            pResult->pResults.ExcludeResult(pItem);
        }
    }
    else
    {
        pItem.nSize = nSize;
        for (const auto nAddress : m_vSelectedAddresses)
        {
            pItem.nAddress = nAddress;
            pResult->pResults.ExcludeResult(pItem);
        }
    }

    // attempt to keep scroll offset after filtering.
    // adjust for any items removed above the first visible address.
    auto nScrollOffset = GetScrollOffset();
    const auto nFirstVisibleAddress = m_vResults.GetItemAt(0)->nAddress;
    for (const auto nAddress : m_vSelectedAddresses)
    {
        if (nAddress < nFirstVisibleAddress)
            --nScrollOffset;
        else
            break;
    }

    m_vSelectedAddresses.clear();
    SetValue(HasSelectionProperty, false);

    const size_t nMatchingAddressCount = pResult->pResults.MatchingAddressCount();
    if (nMatchingAddressCount < gsl::narrow_cast<size_t>(nScrollOffset) + SEARCH_ROWS_DISPLAYED)
        nScrollOffset = 0;

    AddNewPage(std::move(pResult));
    DispatchMemoryRead([this, nScrollOffset]() {
        ChangePage(m_nSelectedSearchResult);

        SetValue(ScrollOffsetProperty, nScrollOffset);
    });
}

void MemorySearchViewModel::BookmarkSelected()
{
    if (m_vSelectedAddresses.empty())
        return;

    const auto nSize = m_vSearchResults.back()->pResults.GetSize();
    auto& vmBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    if (!vmBookmarks.IsVisible())
        vmBookmarks.Show();

    vmBookmarks.Bookmarks().Items().BeginUpdate();

    int nCount = 0;
    for (const auto nAddress : m_vSelectedAddresses)
    {
        switch (nSize)
        {
            case ra::data::Memory::Size::NibbleLower:
            case ra::data::Memory::Size::NibbleUpper:
                vmBookmarks.AddBookmark(nAddress >> 1, (nAddress & 1) ? ra::data::Memory::Size::NibbleUpper : ra::data::Memory::Size::NibbleLower);
                break;

            default:
                vmBookmarks.AddBookmark(nAddress, nSize);
                break;
        }


        if (++nCount == 100)
            break;
    }

    vmBookmarks.Bookmarks().Items().EndUpdate();

    if (nCount == 100)
        ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(L"Can only create 100 new bookmarks at a time.");
}

void MemorySearchViewModel::ExportResults() const
{
    if (m_vResults.Count() == 0 || m_nSelectedSearchResult == 0)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Nothing to export");
        return;
    }

    ra::ui::viewmodels::FileDialogViewModel vmFileDialog;
    vmFileDialog.SetWindowTitle(L"Export Search Results");
    vmFileDialog.AddFileType(L"CSV File", L"*.csv");
    vmFileDialog.SetDefaultExtension(L"csv");

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    vmFileDialog.SetFileName(ra::util::String::Printf(L"%u-SearchResults.csv", pGameContext.GameId()));

    if (vmFileDialog.ShowSaveFileDialog() == ra::ui::DialogResult::OK)
    {
        const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
        auto pTextWriter = pFileSystem.CreateTextFile(vmFileDialog.GetFileName());
        if (pTextWriter == nullptr)
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
                ra::util::String::Printf(L"Could not create %s", vmFileDialog.GetFileName()));
        }
        else
        {
            const auto& pResults = m_vSearchResults.at(m_nSelectedSearchResult)->pResults;
            const auto nResults = pResults.MatchingAddressCount();
            if (pResults.MatchingAddressCount() > 500)
            {
                ra::ui::viewmodels::ProgressViewModel vmProgress;
                vmProgress.SetMessage(ra::util::String::Printf(L"Exporting %d search results", nResults));
                vmProgress.QueueTask([this, &pTextWriter, &vmProgress]() {
                    SaveResults(*pTextWriter, [&vmProgress](int nProgress) {
                        vmProgress.SetProgress(nProgress);
                        return (vmProgress.GetDialogResult() == ra::ui::DialogResult::None);
                    });
                });

                vmProgress.ShowModal(ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryInspector);
            }
            else
            {
                SaveResults(*pTextWriter, nullptr);
            }
        }
    }
}

void MemorySearchViewModel::SaveResults(ra::services::TextWriter& sFile, std::function<bool(int)> pProgressCallback) const
{
    const auto& pResults = m_vSearchResults.at(m_nSelectedSearchResult)->pResults;
    const auto& pCompareResults = m_vSearchResults.at(m_nSelectedSearchResult - 1)->pResults;
    const auto& pInitialResults = m_vSearchResults.front()->pResults;
    ra::services::SearchResult pResult;

    const auto nResults = pResults.MatchingAddressCount();

    const int nPerPercentage = (nResults > 100) ? gsl::narrow_cast<int>(nResults / 100) : 1;

    sFile.WriteLine("Address,Value,PreviousValue,InitialValue");

    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
    if (pCompareResults.GetSize() == ra::data::Memory::Size::NibbleLower)
    {
        for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < pResults.MatchingAddressCount(); ++nIndex)
        {
            if (!pResults.GetMatchingAddress(nIndex, pResult))
                continue;

            const auto nSize = (pResult.nAddress & 1) ? ra::data::Memory::Size::NibbleUpper : ra::data::Memory::Size::NibbleLower;
            const auto nAddress = pResult.nAddress >> 1;

            sFile.WriteLine(ra::util::String::Printf(L"%s%s,%s,%s,%s",
                pMemoryContext.FormatAddress(nAddress), (pResult.nAddress & 1) ? "U" : "L",
                pResults.GetFormattedValue(nAddress, nSize),
                pCompareResults.GetFormattedValue(nAddress, nSize),
                pInitialResults.GetFormattedValue(nAddress, nSize)));

            if (pProgressCallback != nullptr && nIndex % nPerPercentage == 0)
            {
                if (!pProgressCallback(gsl::narrow_cast<int>(nIndex / nPerPercentage)))
                    break;
            }
        }
    }
    else
    {
        const auto nSize = pCompareResults.GetSize();

        for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < nResults; ++nIndex)
        {
            if (!pResults.GetMatchingAddress(nIndex, pResult))
                continue;

            const auto nAddress = pResult.nAddress;
            sFile.WriteLine(ra::util::String::Printf(L"%s,%s,%s,%s",
                pMemoryContext.FormatAddress(nAddress),
                pResults.GetFormattedValue(nAddress, nSize),
                pCompareResults.GetFormattedValue(nAddress, nSize),
                pInitialResults.GetFormattedValue(nAddress, nSize)));

            if (pProgressCallback != nullptr && nIndex % nPerPercentage == 0)
            {
                if (!pProgressCallback(gsl::narrow_cast<int>(nIndex / nPerPercentage)))
                    break;
            }
        }
    }
}

void MemorySearchViewModel::ImportResults()
{
    switch (GetSearchType())
    {
        case ra::services::SearchType::AsciiText:
        case ra::services::SearchType::BitCount:
        case ra::services::SearchType::FourBit:
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
                ra::util::String::Printf(L"Cannot import %s search results",
                    m_vSearchTypes.GetLabelForId(ra::etoi(GetSearchType()))));
            return;
    }

    if (m_vResults.Count() != 0 || m_nSelectedSearchResult != 0)
    {
        if (ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(L"Start new search?",
            L"This will initialize a new search with previously exported search results.",
            ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo) == ra::ui::DialogResult::No)
        {
            return;
        }
    }

    ra::ui::viewmodels::FileDialogViewModel vmFileDialog;
    vmFileDialog.SetWindowTitle(L"Import Search Results");
    vmFileDialog.AddFileType(L"CSV File", L"*.csv");
    vmFileDialog.SetDefaultExtension(L"csv");

    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    vmFileDialog.SetFileName(ra::util::String::Printf(L"%u-SearchResults.csv", pGameContext.GameId()));

    if (vmFileDialog.ShowOpenFileDialog() == ra::ui::DialogResult::OK)
    {
        const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
        auto pTextReader = pFileSystem.OpenTextFile(vmFileDialog.GetFileName());
        if (pTextReader == nullptr)
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
                ra::util::String::Printf(L"Could not open %s", vmFileDialog.GetFileName()));
        }
        else
        {
            std::string sLine;
            if (!pTextReader->GetLine(sLine) || sLine != "Address,Value,PreviousValue,InitialValue")
            {
                ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
                    L"File does not appear to be an exported search result");
            }
            else
            {
                if (m_bIsContinuousFiltering)
                    ToggleContinuousFilter();

                std::unique_ptr<SearchResult> pResult;
                pResult.reset(new SearchResult());
                pResult->sSummary =
                    ra::util::String::Printf(L"Imported Search", SearchTypes().GetLabelForId(ra::etoi(GetSearchType())));

                LoadResults(*pTextReader, *pResult);

                std::unique_ptr<SearchResult> pResult2;
                pResult2.reset(new SearchResult());
                pResult2->sSummary = pResult->sSummary;

                pResult2->pResults.Initialize(pResult->pResults,
                    [pResults = &pResult->pResults](ra::data::ByteAddress nAddress, uint8_t* pBuffer, size_t nCount) noexcept {
                        pResults->GetBytes(nAddress, pBuffer, nCount);
                    },
                    ComparisonType::Equals, GetValueType(), GetFilterValue());

                SetValue(FilterSummaryProperty, pResult->sSummary);
                SetValue(SelectedPageProperty, L"1/1");
                SetValue(ScrollOffsetProperty, 0);
                SetValue(ScrollMaximumProperty, 0);
                SetValue(ResultCountProperty, gsl::narrow_cast<int>(pResult->pResults.MatchingAddressCount()));
                SetValue(ResultMemSizeProperty, ra::etoi(pResult->pResults.GetSize()));

                {
                    std::lock_guard lock(m_oMutex);

                    m_vSearchResults.clear();
                    m_vSearchResults.push_back(std::move(pResult));
                    m_vSearchResults.push_back(std::move(pResult2));
                }

                DispatchMemoryRead([this]() { ChangePage(1); });
            }
        }
    }
}

void MemorySearchViewModel::LoadResults(ra::services::TextReader& pTextReader,
                                        MemorySearchViewModel::SearchResult& vmResult) const
{
    std::vector<ra::services::SearchResult> vEntries;
    ra::services::SearchResult pResult{};

    ra::services::SearchResults pTemp;
    pTemp.Initialize(0, 0, GetSearchType());
    pResult.nSize = pTemp.GetSize();

    const bool bIsFloat = ra::data::Memory::SizeIsFloat(pResult.nSize);

    std::string sLine;
    while (pTextReader.GetLine(sLine))
    {
        const auto index = sLine.find(',');
        if (index == std::string::npos)
            continue;
        const auto index2 = sLine.find(',', index + 1);
        if (index2 == std::string::npos)
            continue;

        pResult.nAddress = ra::data::Memory::ParseAddress(sLine.substr(0, index));

        const auto sValue = sLine.substr(index + 1, index2 - index - 1);
        if (bIsFloat)
        {
            char* pEnd = nullptr;
            const auto fValue = std::strtof(sValue.c_str(), &pEnd);
            if (pEnd && *pEnd != '\0')
                continue;
            pResult.nValue = ra::data::Memory::FloatToU32(fValue, pResult.nSize);
        }
        else
        {
            char* pEnd = nullptr;
            const auto nValue = std::strtoul(sValue.c_str(), &pEnd, 16);
            if (pEnd && *pEnd != '\0')
                continue;
            pResult.nValue = gsl::narrow_cast<unsigned int>(nValue);
        }

        vEntries.push_back(pResult);
    }

    if (!vEntries.empty())
        vmResult.pResults.Initialize(vEntries, GetSearchType());
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
