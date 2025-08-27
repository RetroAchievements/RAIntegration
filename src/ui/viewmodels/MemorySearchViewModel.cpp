#include "MemorySearchViewModel.hh"

#include "data\context\ConsoleContext.hh"
#include "data\context\EmulatorContext.hh"

#include "services\IClock.hh"
#include "services\IFileSystem.hh"
#include "services\ServiceLocator.hh"

#include "ui\IDesktop.hh"
#include "ui\viewmodels\FileDialogViewModel.hh"
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
constexpr int MEMORY_RANGE_SYSTEM = 1;
constexpr int MEMORY_RANGE_GAME = 2;
constexpr int MEMORY_RANGE_CUSTOM = 3;

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
const IntModelProperty MemorySearchViewModel::ResultMemSizeProperty("MemorySearchViewModel", "ResultMemSize", ra::etoi(MemSize::EightBit));
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

        const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>();
        const auto* pRegion = pConsoleContext.GetMemoryRegion(nAddress);
        if (pRegion)
        {
            SetDescription(ra::Widen(pRegion->Description));
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

    // explicitly set the MemSize to some non-selectable value so the first search will
    // trigger a PropertyChanged event.
    SetValue(ResultMemSizeProperty, ra::etoi(MemSize::Bit_0));
}

void MemorySearchViewModel::InitializeNotifyTargets()
{
    auto& pEmulatorContext = ra::services::ServiceLocator::GetMutable<ra::data::context::EmulatorContext>();
    pEmulatorContext.AddNotifyTarget(*this);
    OnTotalMemorySizeChanged();

    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    pGameContext.AddNotifyTarget(*this);
}

void MemorySearchViewModel::OnTotalMemorySizeChanged()
{
    ra::ByteAddress nGameRamStart = 0U;
    ra::ByteAddress nGameRamEnd = 0U;
    ra::ByteAddress nSystemRamStart = 0U;
    ra::ByteAddress nSystemRamEnd = 0U;

    for (const auto& pRegion : ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>().MemoryRegions())
    {
        if (pRegion.Type == ra::data::context::ConsoleContext::AddressType::SystemRAM)
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
        else if (pRegion.Type == ra::data::context::ConsoleContext::AddressType::SaveRAM)
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

    const auto nTotalBankSize = gsl::narrow_cast<ra::ByteAddress>(ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>().TotalMemorySize());
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
    SetValue(TotalMemorySizeProperty, nTotalBankSize);
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

void MemorySearchViewModel::DoFrame()
{
    if (m_vSearchResults.size() < 2)
        return;

    if (m_bIsContinuousFiltering)
    {
        ApplyContinuousFilter();
        return;
    }

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();

    {
        std::lock_guard lock(m_oMutex);
        auto& pCurrentResults = *m_vSearchResults.at(m_nSelectedSearchResult).get();

        m_vResults.BeginUpdate();

        // NOTE: only processes the visible items
        ra::services::SearchResult pResult;
        std::wstring sFormattedValue;

        gsl::index nIndex = GetScrollOffset();
        for (gsl::index i = 0; i < gsl::narrow_cast<gsl::index>(m_vResults.Count()); ++i)
        {
            auto* pRow = m_vResults.GetItemAt(i);
            Expects(pRow != nullptr);

            pCurrentResults.pResults.GetMatchingAddress(nIndex++, pResult);

            const auto nPreviousValue = pResult.nValue;
            UpdateResult(*pRow, pCurrentResults.pResults, pResult, false, pEmulatorContext);

            // when updating an existing result, check to see if it's been modified and update the tracker
            if (!pRow->bHasBeenModified && pResult.nValue != nPreviousValue)
            {
                pCurrentResults.vModifiedAddresses.insert(pResult.nAddress);
                pRow->bHasBeenModified = true;
            }

            pRow->UpdateRowColor();
        }
    }

    // EndUpdate has to be outside the lock as it may raise UI events
    m_vResults.EndUpdate();
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
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
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
    ra::ByteAddress nStart, nEnd;
    if (!ParseFilterRange(nStart, nEnd))
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(L"Invalid address range");
        return;
    }

    if (m_bIsContinuousFiltering)
        ToggleContinuousFilter();

    DispatchMemoryRead([this, nStart, nEnd]() { BeginNewSearch(nStart, nEnd); });
}

void MemorySearchViewModel::BeginNewSearch(ra::ByteAddress nStart, ra::ByteAddress nEnd)
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
    pResult->sSummary = ra::StringPrintf(L"New %s%s Search",
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
    SetValue(SelectedPageProperty, ra::StringPrintf(L"%u/%u", m_nSelectedSearchResult, m_vSearchResults.size() - 1));

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
        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
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

            auto sAddress = ra::ByteAddressToString(pResult.nAddress);
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

            UpdateResult(*pRow, pCurrentResults.pResults, pResult, true, pEmulatorContext);

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
    bool bForceFilterCheck, const ra::data::context::EmulatorContext& pEmulatorContext)
{
    std::wstring sFormattedValue;
    pResult.nValue = vmResult.nCurrentValue;

    if (pResults.UpdateValue(pResult, &sFormattedValue, pEmulatorContext) || bForceFilterCheck)
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
    std::wstring sTooltip = ra::StringPrintf(L"%s\n%s | Current\n",
        vmResult.GetAddress(), vmResult.GetCurrentValue());

    const auto& pCompareResults = m_vSearchResults.at(m_nSelectedSearchResult)->pResults;
    ra::ByteAddress nAddress = vmResult.nAddress;
    MemSize nSize = pCompareResults.GetSize();

    if (nSize == MemSize::Nibble_Lower)
    {
        if (nAddress & 1)
            nSize = MemSize::Nibble_Upper;
        nAddress >>= 1;
    }

    sTooltip.append(pCompareResults.GetFormattedValue(nAddress, nSize));
    sTooltip.append(L" | Last Filter\n");

    const auto& pInitialResults = m_vSearchResults.front()->pResults;
    sTooltip.append(pInitialResults.GetFormattedValue(nAddress, nSize));
    sTooltip.append(L" | Initial");

    return sTooltip;
}

void MemorySearchViewModel::OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNote)
{
    for (gsl::index i = 0; i < ra::to_signed(m_vResults.Count()); ++i)
    {
        auto* pRow = m_vResults.GetItemAt(i);
        if (pRow != nullptr && pRow->nAddress == nAddress)
        {
            pRow->UpdateCodeNote(sNote);
            break;
        }
    }
}

void MemorySearchViewModel::OnCodeNoteMoved(ra::ByteAddress nOldAddress, ra::ByteAddress nNewAddress, const std::wstring& sNote)
{
    for (gsl::index i = 0; i < ra::to_signed(m_vResults.Count()); ++i)
    {
        auto* pRow = m_vResults.GetItemAt(i);
        if (pRow != nullptr)
        {
            if (pRow->nAddress == nOldAddress)
            {
                pRow->UpdateCodeNote(L"");
                break;
            }
            else if (pRow->nAddress == nNewAddress)
            {
                pRow->UpdateCodeNote(sNote);
                break;
            }
        }
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
        OnPredefinedFilterRangeChanged();
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

    const auto& pCurrentResults = *m_vSearchResults.at(m_nSelectedSearchResult).get();
    std::unique_ptr<SearchResult> pResult;
    pResult.reset(new SearchResult(pCurrentResults)); // clone current item
    ra::services::SearchResult pItem {};

    const auto nSize = pCurrentResults.pResults.GetSize();
    if (nSize == MemSize::Nibble_Lower)
    {
        for (const auto nAddress : m_vSelectedAddresses)
        {
            pItem.nAddress = nAddress >> 1;
            pItem.nSize = (nAddress & 1) ? MemSize::Nibble_Upper : MemSize::Nibble_Lower;
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

    const MemSize nSize = m_vSearchResults.back()->pResults.GetSize();
    auto& vmBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
    if (!vmBookmarks.IsVisible())
        vmBookmarks.Show();

    vmBookmarks.Bookmarks().Items().BeginUpdate();

    int nCount = 0;
    for (const auto nAddress : m_vSelectedAddresses)
    {
        switch (nSize)
        {
            case MemSize::Nibble_Lower:
            case MemSize::Nibble_Upper:
                vmBookmarks.AddBookmark(nAddress >> 1, (nAddress & 1) ? MemSize::Nibble_Upper : MemSize::Nibble_Lower);
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
    vmFileDialog.SetFileName(ra::StringPrintf(L"%u-SearchResults.csv", pGameContext.GameId()));

    if (vmFileDialog.ShowSaveFileDialog() == ra::ui::DialogResult::OK)
    {
        const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
        auto pTextWriter = pFileSystem.CreateTextFile(vmFileDialog.GetFileName());
        if (pTextWriter == nullptr)
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
                ra::StringPrintf(L"Could not create %s", vmFileDialog.GetFileName()));
        }
        else
        {
            const auto& pResults = m_vSearchResults.at(m_nSelectedSearchResult)->pResults;
            const auto nResults = pResults.MatchingAddressCount();
            if (pResults.MatchingAddressCount() > 500)
            {
                ra::ui::viewmodels::ProgressViewModel vmProgress;
                vmProgress.SetMessage(ra::StringPrintf(L"Exporting %d search results", nResults));
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

    if (pCompareResults.GetSize() == MemSize::Nibble_Lower)
    {
        for (gsl::index nIndex = 0; ra::to_unsigned(nIndex) < pResults.MatchingAddressCount(); ++nIndex)
        {
            if (!pResults.GetMatchingAddress(nIndex, pResult))
                continue;

            const auto nSize = (pResult.nAddress & 1) ? MemSize::Nibble_Upper : MemSize::Nibble_Lower;
            const auto nAddress = pResult.nAddress >> 1;

            sFile.WriteLine(ra::StringPrintf(L"%s%s,%s,%s,%s",
                ra::ByteAddressToString(nAddress), (pResult.nAddress & 1) ? "U" : "L",
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
            sFile.WriteLine(ra::StringPrintf(L"%s,%s,%s,%s",
                ra::ByteAddressToString(nAddress),
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
                ra::StringPrintf(L"Cannot import %s search results",
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
    vmFileDialog.SetFileName(ra::StringPrintf(L"%u-SearchResults.csv", pGameContext.GameId()));

    if (vmFileDialog.ShowOpenFileDialog() == ra::ui::DialogResult::OK)
    {
        const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
        auto pTextReader = pFileSystem.OpenTextFile(vmFileDialog.GetFileName());
        if (pTextReader == nullptr)
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowErrorMessage(
                ra::StringPrintf(L"Could not open %s", vmFileDialog.GetFileName()));
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
                    ra::StringPrintf(L"Imported Search", SearchTypes().GetLabelForId(ra::etoi(GetSearchType())));

                LoadResults(*pTextReader, *pResult);

                std::unique_ptr<SearchResult> pResult2;
                pResult2.reset(new SearchResult());
                pResult2->sSummary = pResult->sSummary;

                pResult2->pResults.Initialize(pResult->pResults,
                    [pResults = &pResult->pResults](ra::ByteAddress nAddress, uint8_t* pBuffer, size_t nCount) noexcept {
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

    const bool bIsFloat = ra::data::MemSizeIsFloat(pResult.nSize);

    std::string sLine;
    while (pTextReader.GetLine(sLine))
    {
        const auto index = sLine.find(',');
        if (index == std::string::npos)
            continue;
        const auto index2 = sLine.find(',', index + 1);
        if (index2 == std::string::npos)
            continue;

        pResult.nAddress = ra::ByteAddressFromString(sLine.substr(0, index));

        const auto sValue = sLine.substr(index + 1, index2 - index - 1);
        if (bIsFloat)
        {
            char* pEnd = nullptr;
            const auto fValue = std::strtof(sValue.c_str(), &pEnd);
            if (pEnd && *pEnd != '\0')
                continue;
            pResult.nValue = ra::data::FloatToU32(fValue, pResult.nSize);
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
