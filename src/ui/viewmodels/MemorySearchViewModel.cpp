#include "MemorySearchViewModel.hh"

#include "data\ConsoleContext.hh"
#include "data\EmulatorContext.hh"

#include "services\ServiceLocator.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"
#include "ui\viewmodels\WindowManager.hh"

#include "RA_Defs.h"

namespace ra {
namespace ui {
namespace viewmodels {

constexpr size_t SEARCH_ROWS_DISPLAYED = 12;
constexpr size_t SEARCH_MAX_HISTORY = 50;

const StringModelProperty MemorySearchViewModel::FilterRangeProperty("MemorySearchViewModel", "FilterRange", L"");
const IntModelProperty MemorySearchViewModel::SearchTypeProperty("MemorySearchViewModel", "SearchType", ra::etoi(MemorySearchViewModel::SearchType::EightBit));
const IntModelProperty MemorySearchViewModel::ComparisonTypeProperty("MemorySearchViewModel", "ComparisonType", ra::etoi(ComparisonType::Equals));
const IntModelProperty MemorySearchViewModel::ValueTypeProperty("MemorySearchViewModel", "ValueType", ra::etoi(MemorySearchViewModel::ValueType::LastKnownValue));
const StringModelProperty MemorySearchViewModel::FilterValueProperty("MemorySearchViewModel", "FilterValue", L"");
const StringModelProperty MemorySearchViewModel::FilterSummaryProperty("MemorySearchViewModel", "FilterSummary", L"");
const IntModelProperty MemorySearchViewModel::ResultCountProperty("MemorySearchViewModel", "ResultCount", 0);
const IntModelProperty MemorySearchViewModel::ScrollOffsetProperty("MemorySearchViewModel", "ScrollOffset", 0);
const StringModelProperty MemorySearchViewModel::SelectedPageProperty("MemorySearchViewModel", "SelectedPageProperty", L"0/0");

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
    m_vSearchTypes.Add(ra::etoi(SearchType::FourBit), L"4-bit");
    m_vSearchTypes.Add(ra::etoi(SearchType::EightBit), L"8-bit");
    m_vSearchTypes.Add(ra::etoi(SearchType::SixteenBit), L"16-bit");

    m_vComparisonTypes.Add(ra::etoi(ComparisonType::Equals), L"=");
    m_vComparisonTypes.Add(ra::etoi(ComparisonType::LessThan), L"<");
    m_vComparisonTypes.Add(ra::etoi(ComparisonType::LessThanOrEqual), L"<=");
    m_vComparisonTypes.Add(ra::etoi(ComparisonType::GreaterThan), L">");
    m_vComparisonTypes.Add(ra::etoi(ComparisonType::GreaterThanOrEqual), L">=");
    m_vComparisonTypes.Add(ra::etoi(ComparisonType::NotEqualTo), L"!=");

    m_vValueTypes.Add(ra::etoi(ValueType::Constant), L"Constant");
    m_vValueTypes.Add(ra::etoi(ValueType::LastKnownValue), L"Last Known Value");

    AddNotifyTarget(*this);
    m_vResults.AddNotifyTarget(*this);
}

static constexpr MemSize SearchTypeToMemSize(MemorySearchViewModel::SearchType nSearchType) 
{
    switch (nSearchType)
    {
        case MemorySearchViewModel::SearchType::FourBit:
            return MemSize::Nibble_Lower;
        default:
        case MemorySearchViewModel::SearchType::EightBit:
            return MemSize::EightBit;
        case MemorySearchViewModel::SearchType::SixteenBit:
            return MemSize::SixteenBit;
    }
}

MemSize MemorySearchViewModel::ResultMemSize() const
{
    if (m_vSearchResults.empty())
        return SearchTypeToMemSize(GetSearchType());

    return m_vSearchResults.back().pResults.GetSize();
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
    }
}

void MemorySearchViewModel::DoFrame()
{
    if (m_vSearchResults.size() < 2)
        return;

    m_bNeedsRedraw = false;

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    const auto& pCurrentResults = m_vSearchResults.at(m_nSelectedSearchResult);
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
            m_vModifiedAddresses.insert(pResult.nAddress);
            pRow->bHasBeenModified = true;
        }

        pRow->UpdateRowColor();
    }
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

    const MemSize nMemSize = SearchTypeToMemSize(GetSearchType());

    m_vModifiedAddresses.clear();
    m_vSelectedAddresses.clear();

    m_vSearchResults.clear();
    SearchResult& pResult = m_vSearchResults.emplace_back();
    m_nSelectedSearchResult = 0;

    pResult.nCompareType = GetComparisonType();
    pResult.nValueType = GetValueType();
    pResult.nValue = 0;
    pResult.pResults.Initialize(nStart, gsl::narrow<size_t>(nEnd) - nStart + 1, nMemSize);
    pResult.sSummary = ra::StringPrintf(L"New %s Search", SearchTypes().GetLabelForId(ra::etoi(GetSearchType())));

    m_vResults.BeginUpdate();
    while (m_vResults.Count() > 0)
        m_vResults.RemoveAt(m_vResults.Count() - 1);
    m_vResults.EndUpdate();

    SetValue(FilterSummaryProperty, pResult.sSummary);
    SetValue(SelectedPageProperty, L"1/1");
    SetValue(ScrollOffsetProperty, 0);
    SetValue(ResultCountProperty, gsl::narrow_cast<int>(pResult.pResults.MatchingAddressCount()));
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
    if (GetValueType() == ValueType::Constant)
    {
        const auto& sValue = GetFilterValue();
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

    pResult.nCompareType = GetComparisonType();
    pResult.nValueType = GetValueType();
    pResult.nValue = nValue;

    if (pResult.nValueType == ValueType::LastKnownValue)
        pResult.pResults.Initialize(pPreviousResult.pResults, pResult.nCompareType);
    else
        pResult.pResults.Initialize(pPreviousResult.pResults, pResult.nCompareType, nValue);

    const auto nMatches = pResult.pResults.MatchingAddressCount();
    if (nMatches == pPreviousResult.pResults.MatchingAddressCount())
    {
        // same number of matches. if the same filter was applied, don't double up on the search history.
        // unless this is the first filter, then display it anyway.
        if (pResult.nCompareType == pPreviousResult.nCompareType &&
            pResult.nValueType == pPreviousResult.nValueType &&
            pResult.nValue == pPreviousResult.nValue &&
            m_vSearchResults.size() > 2)
        {
            // comparing against last known value with not equals may result in different highlights, keep it.
            if (pResult.nValueType == ValueType::LastKnownValue || pResult.nCompareType == ComparisonType::Equals)
            {
                m_vSearchResults.pop_back();
                --m_nSelectedSearchResult;

                // in case we removed some
                SetValue(SelectedPageProperty, ra::StringPrintf(L"%u/%u", m_nSelectedSearchResult, m_vSearchResults.size() - 1));

                return;
            }
        }
    }

    ra::StringBuilder builder;
    builder.Append(L"Filter: ");
    builder.Append(ComparisonTypes().GetLabelForId(ra::etoi(GetComparisonType())));
    builder.Append(L" ");
    switch (GetValueType())
    {
        case ValueType::Constant:
            builder.Append(GetFilterValue());
            break;

        case ValueType::LastKnownValue:
            builder.Append(L"Last Known Value");
            break;
    }
    pResult.sSummary = builder.ToWString();
    SetValue(FilterSummaryProperty, pResult.sSummary);

    ChangePage(m_nSelectedSearchResult);
}

void MemorySearchViewModel::ChangePage(size_t nNewPage)
{
    m_nSelectedSearchResult = nNewPage;
    SetValue(SelectedPageProperty, ra::StringPrintf(L"%u/%u", m_nSelectedSearchResult, m_vSearchResults.size() - 1));
    SetValue(ScrollOffsetProperty, 0);

    const auto nMatches = m_vSearchResults.at(nNewPage).pResults.MatchingAddressCount();
    SetValue(ResultCountProperty, gsl::narrow_cast<int>(nMatches));
    SetValue(FilterSummaryProperty, m_vSearchResults.at(nNewPage).sSummary);

    m_vModifiedAddresses.clear();
    m_vSelectedAddresses.clear();

    UpdateResults();
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
        pRow->SetCurrentValue(ra::StringPrintf(sValueFormat, pResult.nValue));

        unsigned int nPreviousValue;
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

        pRow->bMatchesFilter = TestFilter(pResult, pCurrentResults, nPreviousValue);
        pRow->bHasBookmark = vmBookmarks.HasBookmark(pResult.nAddress);
        pRow->bHasBeenModified = (m_vModifiedAddresses.find(pRow->nAddress) != m_vModifiedAddresses.end());
        pRow->UpdateRowColor();
        ++nRow;
    }

    while (m_vResults.Count() > nRow)
        m_vResults.RemoveAt(m_vResults.Count() - 1);

    m_vResults.EndUpdate();
}

bool MemorySearchViewModel::TestFilter(const ra::services::SearchResults::Result& pResult, const SearchResult& pCurrentResults, unsigned int nPreviousValue) noexcept
{
    switch (pCurrentResults.nValueType)
    {
        case ValueType::Constant:
            return pResult.Compare(pCurrentResults.nValue, pCurrentResults.nCompareType);

        case ValueType::LastKnownValue:
            return pResult.Compare(nPreviousValue, pCurrentResults.nCompareType);

        default:
            return false;
    }
}

void MemorySearchViewModel::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == ScrollOffsetProperty)
        UpdateResults();
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
    // at least two pages (initialization and first filter) must be avaiable to go back to
    if (m_nSelectedSearchResult > 1)
        ChangePage(m_nSelectedSearchResult - 1);
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

    ChangePage(m_nSelectedSearchResult);
}

void MemorySearchViewModel::BookmarkSelected()
{
    if (m_vSelectedAddresses.empty())
        return;

    const MemSize nSize = SearchTypeToMemSize(GetSearchType());
    if (nSize == MemSize::Nibble_Lower)
    {
        ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(L"4-bit bookmarks are not supported");
        return;
    }

    auto& vmBookmarks = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().MemoryBookmarks;
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
