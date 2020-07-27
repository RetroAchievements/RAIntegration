#include "MemorySearchViewModel.hh"

#include "data\ConsoleContext.hh"
#include "data\EmulatorContext.hh"

#include "services\IClock.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include "RA_Defs.h"

namespace ra {
namespace ui {
namespace viewmodels {

constexpr size_t SEARCH_ROWS_DISPLAYED = 9; // needs to be one higher than actual number displayed for scrolling
constexpr size_t SEARCH_MAX_HISTORY = 50;

constexpr int MEMORY_RANGE_ALL = 0;
constexpr int MEMORY_RANGE_SYSTEM = 1;
constexpr int MEMORY_RANGE_GAME = 2;
constexpr int MEMORY_RANGE_CUSTOM = 3;

const IntModelProperty MemorySearchViewModel::PredefinedFilterRangeProperty("MemorySearchViewModel", "PredefinedFilterRange", MEMORY_RANGE_ALL);
const StringModelProperty MemorySearchViewModel::FilterRangeProperty("MemorySearchViewModel", "FilterRange", L"");
const IntModelProperty MemorySearchViewModel::SearchTypeProperty("MemorySearchViewModel", "SearchType", ra::etoi(ra::services::SearchType::EightBit));
const IntModelProperty MemorySearchViewModel::ComparisonTypeProperty("MemorySearchViewModel", "ComparisonType", ra::etoi(ComparisonType::Equals));
const IntModelProperty MemorySearchViewModel::ValueTypeProperty("MemorySearchViewModel", "ValueType", ra::etoi(ra::services::SearchFilterType::LastKnownValue));
const StringModelProperty MemorySearchViewModel::FilterValueProperty("MemorySearchViewModel", "FilterValue", L"");
const StringModelProperty MemorySearchViewModel::FilterSummaryProperty("MemorySearchViewModel", "FilterSummary", L"");
const IntModelProperty MemorySearchViewModel::ResultCountProperty("MemorySearchViewModel", "ResultCount", 0);
const StringModelProperty MemorySearchViewModel::ResultCountTextProperty("MemorySearchViewModel", "ResultCountText", L"0");
const IntModelProperty MemorySearchViewModel::ResultMemSizeProperty("MemorySearchViewModel", "ResultMemSize", ra::etoi(MemSize::EightBit));
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
const StringModelProperty MemorySearchViewModel::SearchResultViewModel::PreviousValueProperty("SearchResultViewModel", "PreviousValue", L"");
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


MemorySearchViewModel::MemorySearchViewModel()
{
    m_vPredefinedFilterRanges.Add(MEMORY_RANGE_ALL, L"All");
    m_vPredefinedFilterRanges.Add(MEMORY_RANGE_CUSTOM, L"Custom");

    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::FourBit), L"4-bit");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::EightBit), L"8-bit");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::SixteenBit), L"16-bit");
    m_vSearchTypes.Add(ra::etoi(ra::services::SearchType::ThirtyTwoBit), L"32-bit");

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

    // explicitly set the MemSize to some non-selectable value so the first search will
    // trigger a PropertyChanged event.
    SetValue(ResultMemSizeProperty, ra::etoi(MemSize::Bit_0));
}

void MemorySearchViewModel::InitializeNotifyTargets()
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::EmulatorContext>();
    pEmulatorContext.AddNotifyTarget(*this);
    OnTotalMemorySizeChanged();

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::GameContext>();
    pGameContext.AddNotifyTarget(*this);
}

void MemorySearchViewModel::OnTotalMemorySizeChanged()
{
    ra::ByteAddress nGameRamStart = 0U;
    ra::ByteAddress nGameRamEnd = 0U;
    ra::ByteAddress nSystemRamStart = 0U;
    ra::ByteAddress nSystemRamEnd = 0U;

    for (const auto& pRegion : ra::services::ServiceLocator::Get<ra::data::ConsoleContext>().MemoryRegions())
    {
        if (pRegion.Type == ra::data::ConsoleContext::AddressType::SystemRAM)
        {
            if (nSystemRamEnd == 0U)
            {
                nSystemRamStart = pRegion.StartAddress;
                nSystemRamEnd = pRegion.EndAddress;
            }
            else if (pRegion.StartAddress == nSystemRamEnd + 1)
            {
                nSystemRamEnd = pRegion.EndAddress;
            }
        }
        else if (pRegion.Type == ra::data::ConsoleContext::AddressType::SaveRAM)
        {
            if (nGameRamEnd == 0U)
            {
                nGameRamStart = pRegion.StartAddress;
                nGameRamEnd = pRegion.EndAddress;
            }
            else if (pRegion.StartAddress == nGameRamEnd + 1)
            {
                nGameRamEnd = pRegion.EndAddress;
            }
        }
    }

    m_vPredefinedFilterRanges.BeginUpdate();

    const auto nTotalBankSize = gsl::narrow_cast<ra::ByteAddress>(ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().TotalMemorySize());
    if (nGameRamEnd >= nTotalBankSize)
    {
        if (nTotalBankSize == 0U)
        {
            nGameRamEnd = 0U;
        }
        else
        {
            nGameRamEnd = nTotalBankSize - 1;
            if (nGameRamEnd < nGameRamStart)
                nGameRamStart = nGameRamEnd = 0;
        }
    }

    if (nSystemRamEnd >= nTotalBankSize)
    {
        if (nTotalBankSize == 0U)
            nSystemRamEnd = 0U;
        else
            nSystemRamEnd = nTotalBankSize - 1;
    }

    auto nIndex = m_vPredefinedFilterRanges.FindItemIndex(ra::ui::viewmodels::LookupItemViewModel::IdProperty, MEMORY_RANGE_SYSTEM);
    if (nSystemRamEnd != 0U)
    {
        const auto sLabel = ra::StringPrintf(L"System Memory (%s-%s)", ra::ByteAddressToString(nSystemRamStart), ra::ByteAddressToString(nSystemRamEnd));

        auto* pEntry = m_vPredefinedFilterRanges.GetItemAt(nIndex);
        if (pEntry == nullptr)
        {
            pEntry = &m_vPredefinedFilterRanges.Add(MEMORY_RANGE_SYSTEM, sLabel);
            Ensures(pEntry != nullptr);
        }
        else
        {
            pEntry->SetLabel(sLabel);
        }

        pEntry->SetStartAddress(nSystemRamStart);
        pEntry->SetEndAddress(nSystemRamEnd);
    }
    else if (nIndex >= 0)
    {
        m_vPredefinedFilterRanges.RemoveAt(nIndex);
    }

    nIndex = m_vPredefinedFilterRanges.FindItemIndex(ra::ui::viewmodels::LookupItemViewModel::IdProperty, MEMORY_RANGE_GAME);
    if (nGameRamEnd != 0U)
    {
        const auto sLabel = ra::StringPrintf(L"Game Memory (%s-%s)", ra::ByteAddressToString(nGameRamStart), ra::ByteAddressToString(nGameRamEnd));

        auto* pEntry = m_vPredefinedFilterRanges.GetItemAt(nIndex);
        if (pEntry == nullptr)
        {
            pEntry = &m_vPredefinedFilterRanges.Add(MEMORY_RANGE_GAME, sLabel);
            Ensures(pEntry != nullptr);
        }
        else
        {
            pEntry->SetLabel(sLabel);
        }

        pEntry->SetStartAddress(nGameRamStart);
        pEntry->SetEndAddress(nGameRamEnd);
    }
    else if (nIndex >= 0)
    {
        m_vPredefinedFilterRanges.RemoveAt(nIndex);
    }

    for (gsl::index nInsert = 0; nInsert < gsl::narrow_cast<gsl::index>(m_vPredefinedFilterRanges.Count()); ++nInsert)
    {
        gsl::index nMinimumIndex = nInsert;
        int nMinimum = m_vPredefinedFilterRanges.GetItemValue(nInsert, ra::ui::viewmodels::LookupItemViewModel::IdProperty);

        for (gsl::index nScan = nInsert + 1; nScan < gsl::narrow_cast<gsl::index>(m_vPredefinedFilterRanges.Count()); ++nScan)
        {
            const int nScanId = m_vPredefinedFilterRanges.GetItemValue(nScan, ra::ui::viewmodels::LookupItemViewModel::IdProperty);
            if (nScanId < nMinimum)
            {
                nMinimumIndex = nScan;
                nMinimum = nScanId;
            }
        }

        if (nMinimumIndex != nInsert)
            m_vPredefinedFilterRanges.MoveItem(nMinimumIndex, nInsert);
    }

    m_vPredefinedFilterRanges.EndUpdate();

    SetValue(CanBeginNewSearchProperty, (nTotalBankSize > 0U));
}

void MemorySearchViewModel::OnPredefinedFilterRangeChanged()
{
    const auto nValue = GetPredefinedFilterRange();
    if (nValue == MEMORY_RANGE_ALL)
    {
        m_bSelectingFilter = true;
        SetFilterRange(L"");
        m_bSelectingFilter = false;
        return;
    }

    const auto nIndex = m_vPredefinedFilterRanges.FindItemIndex(ra::ui::viewmodels::LookupItemViewModel::IdProperty, nValue);
    if (nIndex == -1)
        return;

    const auto* pEntry = m_vPredefinedFilterRanges.GetItemAt(nIndex);
    Ensures(pEntry != nullptr);

    m_bSelectingFilter = true;
    SetFilterRange(ra::StringPrintf(L"%s-%s",
        ra::ByteAddressToString(pEntry->GetStartAddress()), ra::ByteAddressToString(pEntry->GetEndAddress())));
    m_bSelectingFilter = false;
}

void MemorySearchViewModel::OnFilterRangeChanged()
{
    ra::ByteAddress nStart, nEnd;
    if (!ParseFilterRange(nStart, nEnd))
        return;

    for (gsl::index nIndex = 1; nIndex < ra::to_signed(m_vPredefinedFilterRanges.Count()); ++nIndex)
    {
        const auto* pEntry = m_vPredefinedFilterRanges.GetItemAt(nIndex);
        Ensures(pEntry != nullptr);

        if (pEntry->GetStartAddress() == nStart && pEntry->GetEndAddress() == nEnd)
        {
            SetPredefinedFilterRange(pEntry->GetId());
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
    pEntry->SetLabel(ra::StringPrintf(L"Custom (%s-%s)", ra::ByteAddressToString(nStart), ra::ByteAddressToString(nEnd)));

    SetPredefinedFilterRange(MEMORY_RANGE_CUSTOM);
}

static constexpr const wchar_t* MemSizeFormat(MemSize nSize) 
{
    switch (nSize)
    {
        case MemSize::Nibble_Lower:
        case MemSize::Nibble_Upper:
            return L"0x%01x";
        default:
        case MemSize::EightBit:
            return L"0x%02x";
        case MemSize::SixteenBit:
            return L"0x%04x";
        case MemSize::ThirtyTwoBit:
            return L"0x%08x";
    }
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

    m_bNeedsRedraw = false;
    m_vResults.BeginUpdate();

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    auto& pCurrentResults = m_vSearchResults.at(m_nSelectedSearchResult);
    const wchar_t* sValueFormat = MemSizeFormat(pCurrentResults.pResults.GetSize());

    // only process the visible items
    ra::services::SearchResults::Result pResult;
    unsigned int nPreviousValue = 0U;

    gsl::index nIndex = GetScrollOffset();
    for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(m_vResults.Count()); ++i)
    {
        auto* pRow = m_vResults.GetItemAt(i);
        Expects(pRow != nullptr);

        pCurrentResults.pResults.GetMatchingAddress(nIndex++, pResult);
        nPreviousValue = pResult.nValue;
        pResult.nValue = pEmulatorContext.ReadMemory(pResult.nAddress, pResult.nSize);
        pRow->bMatchesFilter = TestFilter(pResult, pCurrentResults, nPreviousValue);

        pRow->SetCurrentValue(ra::StringPrintf(sValueFormat, pResult.nValue));

        if (pResult.nValue != nPreviousValue)
        {
            pCurrentResults.vModifiedAddresses.insert(pResult.nAddress);
            pRow->bHasBeenModified = true;
        }

        pRow->UpdateRowColor();
    }

    m_vResults.EndUpdate();
}

bool MemorySearchViewModel::NeedsRedraw() noexcept
{
    const bool bNeedsRedraw = m_bNeedsRedraw;
    m_bNeedsRedraw = false;
    return bNeedsRedraw;
}

inline static constexpr auto ParseAddress(const wchar_t* ptr, ra::ByteAddress& address) noexcept
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

bool MemorySearchViewModel::ParseFilterRange(_Out_ ra::ByteAddress& nStart, _Out_ ra::ByteAddress& nEnd)
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    const auto nMax = gsl::narrow_cast<ra::ByteAddress>(pEmulatorContext.TotalMemorySize()) - 1;

    const std::wstring& sRange = GetFilterRange();
    if (sRange.empty())
    {
        // no range specified, search all
        nStart = 0;
        nEnd = nMax;
        return true;
    }

    auto ptr = ParseAddress(sRange.data(), nStart);
    Expects(ptr != nullptr);

    nEnd = nStart;
    if (nStart > nMax)
        return false;

    while (iswspace(*ptr))
        ++ptr;

    if (*ptr == '-')
    {
        ++ptr;
        while (iswspace(*ptr))
            ++ptr;

        ptr = ParseAddress(ptr, nEnd);
        Expects(ptr != nullptr);

        if (nEnd > nMax)
            nEnd = nMax;
        else if (nEnd < nStart)
            std::swap(nStart, nEnd);
    }

    return (*ptr == '\0');
}

void MemorySearchViewModel::BeginNewSearch()
{
    ra::ByteAddress nStart, nEnd;
    if (!ParseFilterRange(nStart, nEnd))
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Invalid address range");
        return;
    }

    if (m_bIsContinuousFiltering)
        ToggleContinuousFilter();

    m_vSelectedAddresses.clear();

    m_vSearchResults.clear();
    SearchResult& pResult = m_vSearchResults.emplace_back();
    m_nSelectedSearchResult = 0;

    pResult.pResults.Initialize(nStart, gsl::narrow<size_t>(nEnd) - nStart + 1, GetSearchType());
    pResult.sSummary = ra::StringPrintf(L"New %s Search", SearchTypes().GetLabelForId(ra::etoi(GetSearchType())));

    m_vResults.BeginUpdate();
    while (m_vResults.Count() > 0)
        m_vResults.RemoveAt(m_vResults.Count() - 1);
    m_vResults.EndUpdate();

    SetValue(FilterSummaryProperty, pResult.sSummary);
    SetValue(SelectedPageProperty, L"1/1");
    SetValue(ScrollOffsetProperty, 0);
    SetValue(ScrollMaximumProperty, 0);
    SetValue(ResultCountProperty, gsl::narrow_cast<int>(pResult.pResults.MatchingAddressCount()));
    SetValue(ResultMemSizeProperty, ra::etoi(pResult.pResults.GetSize()));
}

void MemorySearchViewModel::AddNewPage()
{
    // remove any search history after the current node
    while (m_vSearchResults.size() - 1 > m_nSelectedSearchResult)
        m_vSearchResults.pop_back();

    // add a new search history entry
    m_vSearchResults.emplace_back();
    if (m_vSearchResults.size() > SEARCH_MAX_HISTORY)
        m_vSearchResults.erase(m_vSearchResults.begin());
    else
        ++m_nSelectedSearchResult;
}

void MemorySearchViewModel::ApplyFilter()
{
    if (m_vSearchResults.empty())
        return;

    unsigned int nValue = 0U;
    if (GetValue(CanEditFilterValueProperty))
    {
        const auto& sValue = GetFilterValue();
        if (sValue.empty())
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Invalid filter value");
            return;
        }

        const wchar_t* pStart = sValue.c_str();

        // try decimal first
        wchar_t* pEnd;
        nValue = std::wcstoul(pStart, &pEnd, 10);
        if (pEnd && *pEnd)
        {
            // decimal parse failed, try hex
            nValue = std::wcstoul(pStart, &pEnd, 16);
            if (*pEnd)
            {
                // hex parse failed
                ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Invalid filter value");
                return;
            }
        }
    }

    AddNewPage();

    SearchResult const& pPreviousResult = *(m_vSearchResults.end() - 2);
    SearchResult& pResult = m_vSearchResults.back();

    if (GetValueType() == ra::services::SearchFilterType::InitialValue)
    {
        SearchResult const& pInitialResult = m_vSearchResults.front();
        pResult.pResults.Initialize(pInitialResult.pResults, pPreviousResult.pResults,
            GetComparisonType(), GetValueType(), nValue);
    }
    else
    {
        pResult.pResults.Initialize(pPreviousResult.pResults, GetComparisonType(), GetValueType(), nValue);
    }

    // if this isn't the first filter being applied, and the result count hasn't changed
    const auto nMatches = pResult.pResults.MatchingAddressCount();
    if (nMatches == pPreviousResult.pResults.MatchingAddressCount() && m_vSearchResults.size() > 2)
    {
        // check to see if the same filter was applied.
        if (pResult.pResults.GetFilterComparison() == pPreviousResult.pResults.GetFilterComparison() &&
            pResult.pResults.GetFilterType() == pPreviousResult.pResults.GetFilterType() &&
            pResult.pResults.GetFilterValue() == pPreviousResult.pResults.GetFilterValue())
        {
            // same filter applied, result set didn't change, if applying an equality filter we know the memory
            // didn't change, so don't generate a new result set. do clear the modified addresses list.
            if (pResult.pResults.GetFilterComparison() == ComparisonType::Equals)
            {
                m_vSearchResults.pop_back();
                m_vSearchResults.back().vModifiedAddresses.clear();
                --m_nSelectedSearchResult;
                return;
            }
        }
    }

    ra::StringBuilder builder;
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
    pResult.sSummary = builder.ToWString();
    SetValue(FilterSummaryProperty, pResult.sSummary);

    ChangePage(m_nSelectedSearchResult);
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
        // apply the filter before disabling CanFilter or the filter value will be ignored
        ApplyFilter();

        m_bIsContinuousFiltering = true;
        m_tLastContinuousFilter = ra::services::ServiceLocator::Get<ra::services::IClock>().UpTime();

        SetValue(CanFilterProperty, false);
        SetValue(ContinuousFilterLabelProperty, L"Stop Filtering");
    }
}

void MemorySearchViewModel::ApplyContinuousFilter()
{
    const SearchResult& pResult = m_vSearchResults.back();

    // if there are more than 1000 results, only apply the filter periodically.
    // formula is "number of results / 100" ms between filterings
    // for 10000 results, only filter every 100ms
    // for 50000 results, only filter every 500ms
    // for 100000 results, only filter every second
    const auto nResults = pResult.pResults.MatchingAddressCount();
    if (nResults > 1000)
    {
        const auto tNow = ra::services::ServiceLocator::Get<ra::services::IClock>().UpTime();
        const auto nElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(tNow - m_tLastContinuousFilter);

        if (gsl::narrow_cast<size_t>(nElapsed.count()) < nResults / 100)
            return;

        m_tLastContinuousFilter = tNow;
    }

    // apply the current filter
    SearchResult pNewResult;
    pNewResult.pResults.Initialize(pResult.pResults, pResult.pResults.GetFilterComparison(),
        pResult.pResults.GetFilterType(), pResult.pResults.GetFilterValue());
    pNewResult.sSummary = pResult.sSummary;
    const auto nNewResults = pNewResult.pResults.MatchingAddressCount();

    // replace the last item with the new results
    m_vSearchResults.erase(m_vSearchResults.end() - 1);
    m_vSearchResults.push_back(pNewResult);

    ChangePage(m_nSelectedSearchResult);

    if (nNewResults == 0)
    {
        // if no results are remaining, stop filtering
        ToggleContinuousFilter();
    }
}

void MemorySearchViewModel::ChangePage(size_t nNewPage)
{
    m_nSelectedSearchResult = nNewPage;
    SetValue(SelectedPageProperty, ra::StringPrintf(L"%u/%u", m_nSelectedSearchResult, m_vSearchResults.size() - 1));

    const auto nMatches = m_vSearchResults.at(nNewPage).pResults.MatchingAddressCount();
    SetValue(ResultCountProperty, gsl::narrow_cast<int>(nMatches));
    SetValue(FilterSummaryProperty, m_vSearchResults.at(nNewPage).sSummary);

    // prevent scrolling from triggering a call to UpdateResults - we'll do that in a few lines.
    m_bScrolling = true;
    // note: update maximum first - it can affect offset. then update offset to the value we want.
    SetValue(ScrollMaximumProperty, gsl::narrow_cast<int>(nMatches));
    SetValue(ScrollOffsetProperty, 0);
    m_bScrolling = false;

    m_vSelectedAddresses.clear();

    UpdateResults();

    SetValue(CanGoToPreviousPageProperty, (nNewPage > 1));
    SetValue(CanGoToNextPageProperty, (nNewPage < m_vSearchResults.size() - 1));
}

void MemorySearchViewModel::UpdateResults()
{
    if (m_vSearchResults.size() < 2)
        return;

    const auto& pPreviousResults = m_vSearchResults.at(m_nSelectedSearchResult - 1);
    const auto& pCurrentResults = m_vSearchResults.at(m_nSelectedSearchResult);
    ra::services::SearchResults::Result pResult;
    const auto nIndex = gsl::narrow_cast<gsl::index>(GetScrollOffset());

    const auto& vmBookmarks = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::ConsoleContext>();
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();

    m_vResults.RemoveNotifyTarget(*this);
    m_vResults.BeginUpdate();

    unsigned int nRow = 0;
    while (nRow < SEARCH_ROWS_DISPLAYED)
    {
        if (!pCurrentResults.pResults.GetMatchingAddress(nIndex + nRow, pResult))
            break;

        if (m_vResults.Count() <= nRow)
            m_vResults.Add();

        auto* pRow = m_vResults.GetItemAt(nRow);
        Expects(pRow != nullptr);
        pRow->nAddress = pResult.nAddress;

        auto sAddress = ra::ByteAddressToString(pResult.nAddress);
        const wchar_t* sValueFormat = MemSizeFormat(pResult.nSize);
        switch (pResult.nSize)
        {
            case MemSize::Nibble_Lower:
                sAddress.push_back('L');
                pRow->nAddress <<= 1;
                break;

            case MemSize::Nibble_Upper:
                sAddress.push_back('U');
                pRow->nAddress = (pRow->nAddress << 1) | 1;
                break;
        }

        pRow->SetAddress(ra::Widen(sAddress));

        pResult.nValue = pEmulatorContext.ReadMemory(pResult.nAddress, pResult.nSize);
        pRow->SetCurrentValue(ra::StringPrintf(sValueFormat, pResult.nValue));

        unsigned int nPreviousValue = 0;
        if (pCurrentResults.pResults.GetFilterType() == ra::services::SearchFilterType::InitialValue)
            m_vSearchResults.front().pResults.GetValue(pResult.nAddress, pResult.nSize, nPreviousValue);
        else
            pPreviousResults.pResults.GetValue(pResult.nAddress, pResult.nSize, nPreviousValue);

        pRow->SetPreviousValue(ra::StringPrintf(sValueFormat, nPreviousValue));

        const auto pCodeNote = pGameContext.FindCodeNote(pResult.nAddress, pResult.nSize);
        if (!pCodeNote.empty())
        {
            pRow->bHasCodeNote = true;
            pRow->SetDescription(pCodeNote);
            pRow->SetDescriptionColor(ra::ui::Color(ra::to_unsigned(SearchResultViewModel::DescriptionColorProperty.GetDefaultValue())));
        }
        else
        {
            pRow->bHasCodeNote = false;

            const auto* pRegion = pConsoleContext.GetMemoryRegion(pResult.nAddress);
            if (pRegion)
            {
                pRow->SetDescription(ra::Widen(pRegion->Description));
                pRow->SetDescriptionColor(ra::ui::Color(0xFFA0A0A0));
            }
            else
            {
                pRow->SetDescription(L"");
                pRow->SetDescriptionColor(ra::ui::Color(ra::to_unsigned(SearchResultViewModel::DescriptionColorProperty.GetDefaultValue())));
            }
        }

        pRow->SetSelected(m_vSelectedAddresses.find(pRow->nAddress) != m_vSelectedAddresses.end());

        // when continuous filtering values should always match - don't bother testing
        pRow->bMatchesFilter = m_bIsContinuousFiltering || TestFilter(pResult, pCurrentResults, nPreviousValue);
        pRow->bHasBookmark = vmBookmarks.HasBookmark(pResult.nAddress);
        pRow->bHasBeenModified = (pCurrentResults.vModifiedAddresses.find(pRow->nAddress) != pCurrentResults.vModifiedAddresses.end());
        pRow->UpdateRowColor();
        ++nRow;
    }

    while (m_vResults.Count() > nRow)
        m_vResults.RemoveAt(m_vResults.Count() - 1);

    m_vResults.EndUpdate();
    m_vResults.AddNotifyTarget(*this);
}

void MemorySearchViewModel::OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring&)
{
    for (gsl::index i = 0; i < ra::to_signed(m_vResults.Count()); ++i)
    {
        auto* pRow = m_vResults.GetItemAt(i);
        if (pRow != nullptr && pRow->nAddress == nAddress)
        {
            UpdateResults();
            break;
        }
    }
}

bool MemorySearchViewModel::TestFilter(const ra::services::SearchResults::Result& pResult, const SearchResult& pCurrentResults, unsigned int nPreviousValue) noexcept
{
    switch (pCurrentResults.pResults.GetFilterType())
    {
        case ra::services::SearchFilterType::Constant:
            return pResult.Compare(pCurrentResults.pResults.GetFilterValue(), pCurrentResults.pResults.GetFilterComparison());

        case ra::services::SearchFilterType::InitialValue:
        case ra::services::SearchFilterType::LastKnownValue:
            return pResult.Compare(nPreviousValue, pCurrentResults.pResults.GetFilterComparison());

        case ra::services::SearchFilterType::LastKnownValuePlus:
            return pResult.Compare(nPreviousValue + pCurrentResults.pResults.GetFilterValue(), pCurrentResults.pResults.GetFilterComparison());

        case ra::services::SearchFilterType::LastKnownValueMinus:
            return pResult.Compare(nPreviousValue - pCurrentResults.pResults.GetFilterValue(), pCurrentResults.pResults.GetFilterComparison());

        default:
            return false;
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
            UpdateResults();
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
        OnPredefinedFilterRangeChanged();
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

void MemorySearchViewModel::OnViewModelIntValueChanged(gsl::index, const IntModelProperty::ChangeArgs&) noexcept
{
    // assume color
    m_bNeedsRedraw = true;
}

void MemorySearchViewModel::OnViewModelStringValueChanged(gsl::index, const StringModelProperty::ChangeArgs&) noexcept
{
    // assume text
    m_bNeedsRedraw = true;
}

void MemorySearchViewModel::NextPage()
{
    if (m_nSelectedSearchResult < m_vSearchResults.size() - 1)
        ChangePage(m_nSelectedSearchResult + 1);
}

void MemorySearchViewModel::PreviousPage()
{
    // at least two pages (initialization and first filter) must be available to go back to
    if (m_nSelectedSearchResult > 1)
    {
        if (m_bIsContinuousFiltering)
            ToggleContinuousFilter();

        ChangePage(m_nSelectedSearchResult - 1);
    }
}

void MemorySearchViewModel::SelectRange(gsl::index nFrom, gsl::index nTo, bool bValue)
{
    if (m_vSearchResults.empty())
        return;

    const auto& pCurrentResults = m_vSearchResults.back().pResults;
    if (!bValue && nFrom == 0 && nTo >= gsl::narrow_cast<gsl::index>(pCurrentResults.MatchingAddressCount()) - 1)
    {
        m_vSelectedAddresses.clear();
        return;
    }

    ra::services::SearchResults::Result pResult;

    // ignore IsSelectedProperty events - we'll update the lists directly
    m_vResults.RemoveNotifyTarget(*this);

    if (pCurrentResults.GetSize() == MemSize::Nibble_Lower)
    {
        for (auto nIndex = nFrom; nIndex <= nTo; ++nIndex)
        {
            if (pCurrentResults.GetMatchingAddress(nIndex, pResult))
            {
                auto nAddress = pResult.nAddress << 1;
                if (pResult.nSize == MemSize::Nibble_Upper)
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

    AddNewPage();

    SearchResult pPreviousResult = *(m_vSearchResults.end() - 2);

    m_vSearchResults.pop_back(); // remove temporary item
    SearchResult& pResult = m_vSearchResults.emplace_back(pPreviousResult); // clone previous item

    for (const auto nAddress : m_vSelectedAddresses)
        pResult.pResults.ExcludeAddress(nAddress);

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

    ChangePage(m_nSelectedSearchResult);

    if (pResult.pResults.MatchingAddressCount() < gsl::narrow_cast<size_t>(nScrollOffset) + SEARCH_ROWS_DISPLAYED)
        nScrollOffset = 0;

    SetValue(ScrollOffsetProperty, nScrollOffset);
}

void MemorySearchViewModel::BookmarkSelected()
{
    if (m_vSelectedAddresses.empty())
        return;

    const MemSize nSize = m_vSearchResults.back().pResults.GetSize();
    if (nSize == MemSize::Nibble_Lower)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(L"4-bit bookmarks are not supported");
        return;
    }

    auto& vmBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    if (!vmBookmarks.IsVisible())
        vmBookmarks.Show();

    vmBookmarks.Bookmarks().BeginUpdate();

    int nCount = 0;
    for (const auto nAddress : m_vSelectedAddresses)
    {
        vmBookmarks.AddBookmark(nAddress, nSize);

        if (++nCount == 100)
            break;
    }

    vmBookmarks.Bookmarks().EndUpdate();

    if (nCount == 100)
        ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(L"Can only create 100 new bookmarks at a time.");
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
