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

constexpr size_t SEARCH_ROWS_DISPLAYED = 8;
constexpr size_t SEARCH_MAX_HISTORY = 50;

const StringModelProperty MemorySearchViewModel::FilterRangeProperty("MemorySearchViewModel", "FilterRange", L"");
const IntModelProperty MemorySearchViewModel::SearchTypeProperty("MemorySearchViewModel", "SearchType", ra::etoi(MemorySearchViewModel::SearchType::EightBit));
const IntModelProperty MemorySearchViewModel::ComparisonTypeProperty("MemorySearchViewModel", "ComparisonType", ra::etoi(ComparisonType::Equals));
const IntModelProperty MemorySearchViewModel::ValueTypeProperty("MemorySearchViewModel", "ValueType", ra::etoi(MemorySearchViewModel::ValueType::LastKnownValue));
const StringModelProperty MemorySearchViewModel::FilterValueProperty("MemorySearchViewModel", "FilterValue", L"");
const IntModelProperty MemorySearchViewModel::ResultCountProperty("MemorySearchViewModel", "ResultCount", 0);
const IntModelProperty MemorySearchViewModel::ScrollOffsetProperty("MemorySearchViewModel", "ScrollOffset", 0);
const IntModelProperty MemorySearchViewModel::ScrollMaximumProperty("MemorySearchViewModel", "ScrollMaximum", 0);
const StringModelProperty MemorySearchViewModel::SelectedPageProperty("MemorySearchViewModel", "SelectedPageProperty", L"0/0");

const StringModelProperty MemorySearchViewModel::SearchResultViewModel::DescriptionProperty("SearchResultViewModel", "Description", L"");
const StringModelProperty MemorySearchViewModel::SearchResultViewModel::AddressProperty("SearchResultViewModel", "Address", L"");
const StringModelProperty MemorySearchViewModel::SearchResultViewModel::CurrentValueProperty("SearchResultViewModel", "CurrentValue", L"");
const StringModelProperty MemorySearchViewModel::SearchResultViewModel::PreviousValueProperty("SearchResultViewModel", "PreviousValue", L"");
const BoolModelProperty MemorySearchViewModel::SearchResultViewModel::IsSelectedProperty("SearchResultViewModel", "IsSelected", false);
const IntModelProperty MemorySearchViewModel::SearchResultViewModel::RowColorProperty("SearchResultViewModel", "RowColor", 0);
const IntModelProperty MemorySearchViewModel::SearchResultViewModel::DescriptionColorProperty("SearchResultViewModel", "DescriptionColor", 0);

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
}

void MemorySearchViewModel::DoFrame()
{

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

    MemSize nMemSize = MemSize::EightBit;
    switch (GetSearchType())
    {
        case SearchType::FourBit:
            nMemSize = MemSize::Nibble_Lower;
            break;
        case SearchType::EightBit:
            nMemSize = MemSize::EightBit;
            break;
        case SearchType::SixteenBit:
            nMemSize = MemSize::SixteenBit;
            break;
    }

    m_vModifiedAddresses.clear();
    m_vSelectedAddresses.clear();

    m_vSearchResults.clear();
    SearchResult& pResult = m_vSearchResults.emplace_back();
    m_nSelectedSearchResult = 0;

    pResult.nCompareType = GetComparisonType();
    pResult.nValueType = GetValueType();
    pResult.nValue = 0;
    pResult.pResults.Initialize(nStart, nEnd - nStart + 1, nMemSize);

    m_vResults.BeginUpdate();
    while (m_vResults.Count() > 0)
        m_vResults.RemoveAt(m_vResults.Count() - 1);
    m_vResults.EndUpdate();

    SetValue(SelectedPageProperty, L"0/0");
    SetValue(ScrollOffsetProperty, 0);
    SetValue(ScrollMaximumProperty, 0);
    SetValue(ResultCountProperty, nEnd - nStart + 1);
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
        if (*pEnd)
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

    SearchResult& pPreviousResult = *(m_vSearchResults.end() - 2);
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
        // same number of matches. if the same filter was applied, don't double up on the search history
        if (pResult.nCompareType == pPreviousResult.nCompareType &&
            pResult.nValueType == pPreviousResult.nValueType &&
            pResult.nValue == pPreviousResult.nValue)
        {
            // comparing against last known value with not equals may result in different highlights, keep it.
            if (pResult.nValueType == ValueType::LastKnownValue || pResult.nCompareType == ComparisonType::Equals)
            {
                m_vSearchResults.pop_back();
                --m_nSelectedSearchResult;

                // in case we removed some
                SetValue(SelectedPageProperty, ra::StringPrintf(L"%u/%u", m_nSelectedSearchResult + 1, m_vSearchResults.size()));
                return;
            }
        }
    }

    ChangePage(m_nSelectedSearchResult);
}

void MemorySearchViewModel::ChangePage(size_t nNewPage)
{
    m_nSelectedSearchResult = nNewPage;
    SetValue(SelectedPageProperty, ra::StringPrintf(L"%u/%u", m_nSelectedSearchResult + 1, m_vSearchResults.size()));
    SetValue(ScrollOffsetProperty, 0);

    const auto nMatches = m_vSearchResults.at(nNewPage).pResults.MatchingAddressCount();
    if (nMatches > SEARCH_ROWS_DISPLAYED)
        SetValue(ScrollMaximumProperty, gsl::narrow_cast<int>(nMatches - SEARCH_ROWS_DISPLAYED));
    else
        SetValue(ScrollMaximumProperty, 0);

    SetValue(ResultCountProperty, gsl::narrow_cast<int>(nMatches));

    m_vModifiedAddresses.clear();
    m_vSelectedAddresses.clear();

    UpdateResults();
}

void MemorySearchViewModel::UpdateResults()
{
    const auto& pPreviousResults = m_vSearchResults.at(m_nSelectedSearchResult - 1);
    const auto& pCurrentResults = m_vSearchResults.at(m_nSelectedSearchResult);
    ra::services::SearchResults::Result pResult;
    auto nIndex = GetScrollOffset();

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

        auto sAddress = ra::ByteAddressToString(pResult.nAddress);
        const wchar_t* sValueFormat = L"0x%02x";
        switch (pResult.nSize)
        {
            case MemSize::Nibble_Lower:
                sValueFormat = L"0x%01x";
                sAddress.push_back('L');
                break;

            case MemSize::Nibble_Upper:
                sValueFormat = L"0x%01x";
                sAddress.push_back('U');
                break;

            case MemSize::SixteenBit:
                sValueFormat = L"0x%04x";
                break;
        }

        pRow->SetAddress(ra::Widen(sAddress));
        pRow->SetCurrentValue(ra::StringPrintf(sValueFormat, pResult.nValue));

        const auto nPreviousValue = pPreviousResults.pResults.GetValue(pResult.nAddress, pResult.nSize);
        pRow->SetPreviousValue(ra::StringPrintf(sValueFormat, nPreviousValue));

        const auto* pCodeNote = pGameContext.FindCodeNote(pResult.nAddress);
        if (pCodeNote != nullptr)
        {
            pRow->SetDescription(*pCodeNote);
            pRow->SetDescriptionColor(ra::ui::Color(ra::to_unsigned(SearchResultViewModel::DescriptionColorProperty.GetDefaultValue())));
        }
        else
        {
            const auto* pRegion = pConsoleContext.GetMemoryRegion(pResult.nAddress);
            if (pRegion)
            {
                pRow->SetDescription(ra::Widen(pRegion->Description));
                pRow->SetDescriptionColor(ra::ui::Color(0xFFA0A0A0));
            }
        }

        bool bFilterApplies = false;
        switch (m_vSearchResults.back().nValueType)
        {
            case ValueType::Constant:
                bFilterApplies = pResult.Compare(m_vSearchResults.back().nValue, m_vSearchResults.back().nCompareType);
                break;

            case ValueType::LastKnownValue:
                bFilterApplies = pResult.Compare(nPreviousValue, m_vSearchResults.back().nCompareType);
                break;
        }

        if (!bFilterApplies)
        {
            // red if value no longer matches filter
            pRow->SetRowColor(ra::ui::Color(0xFFFFD0D0)); // was FFD7D7
        }
        else if (vmBookmarks.HasBookmark(pResult.nAddress))
        {
            // green if bookmark found
            pRow->SetRowColor(ra::ui::Color(0xFFD0FFD0));
        }
        else if (pCodeNote != nullptr)
        {
            // blue if code note found
            pRow->SetRowColor(ra::ui::Color(0xFFD0F0FF));
        }
        else if (m_vModifiedAddresses.find(pResult.nAddress) != m_vModifiedAddresses.end())
        {
            // grey if the filter currently matches, but did not at some point
            pRow->SetRowColor(ra::ui::Color(0xFFE0E0E0));
        }
        else
        {
            // default color otherwise
            pRow->SetRowColor(ra::ui::Color(ra::to_unsigned(SearchResultViewModel::RowColorProperty.GetDefaultValue())));
        }

        ++nRow;
    }

    while (m_vResults.Count() > nRow)
        m_vResults.RemoveAt(m_vResults.Count() - 1);

    m_vResults.EndUpdate();
}

void MemorySearchViewModel::OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == ScrollOffsetProperty)
        UpdateResults();
}

void MemorySearchViewModel::NextPage()
{
    if (m_nSelectedSearchResult < m_vSearchResults.size() - 1)
        ChangePage(m_nSelectedSearchResult + 1);
}

void MemorySearchViewModel::PreviousPage()
{
    if (m_nSelectedSearchResult > 0)
        ChangePage(m_nSelectedSearchResult - 1);
}

void MemorySearchViewModel::ExcludeSelected()
{
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
    MemSize nSize = MemSize::EightBit;
    switch (GetSearchType())
    {
        case SearchType::FourBit:
            ra::ui::viewmodels::MessageBoxViewModel::ShowInfoMessage(L"4-bit bookmarks are not supported");
            return;

        case SearchType::EightBit:
            nSize = MemSize::EightBit;
            break;

        case SearchType::SixteenBit:
            nSize = MemSize::SixteenBit;
            break;
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