#ifndef SEARCHRESULTS_H
#define SEARCHRESULTS_H
#pragma once

#include "data\Types.hh"

#include "data\context\EmulatorContext.hh"

#include "data\search\MemBlock.hh"

namespace ra {
namespace services {

enum class SearchType
{
    FourBit,
    EightBit,
    SixteenBit,
    TwentyFourBit,
    ThirtyTwoBit,
    SixteenBitAligned,
    ThirtyTwoBitAligned,
    SixteenBitBigEndian,
    ThirtyTwoBitBigEndian,
    SixteenBitBigEndianAligned,
    ThirtyTwoBitBigEndianAligned,
    Float,
    FloatBigEndian,
    MBF32,
    MBF32LE,
    Double32,
    Double32BigEndian,
    AsciiText,
    BitCount,
};

enum class SearchFilterType
{
    None,
    Constant,
    LastKnownValue,
    LastKnownValuePlus,
    LastKnownValueMinus,
    InitialValue,
};

namespace search { class SearchImpl; }

struct SearchResult
{
    ra::ByteAddress nAddress{};
    unsigned int nValue{};
    MemSize nSize{};
};

class SearchResults
{
public:
    /// <summary>
    /// Initializes an unfiltered result set.
    /// </summary>
    /// <param name="nAddress">The address to start reading from.</param>
    /// <param name="nBytes">The number of bytes to read.</param>
    /// <param name="nType">Type of search to initialize.</param>
    void Initialize(ra::ByteAddress nAddress, size_t nBytes, SearchType nType);

    /// <summary>
    /// Initializes a result set by comparing current memory against another result set.
    /// </summary>
    /// <param name="srSource">The result set to filter.</param>
    /// <param name="nCompareType">Type of comparison to apply.</param>
    /// <param name="nFilterType">Type of filter to apply.</param>
    /// <param name="sFilterValue">Parameter for filter being applied.</param>
    /// <returns><c>true</c> if initialization was successful, <c>false</c> if the filter value was not supported</returns>
    bool Initialize(_In_ const SearchResults& srSource, _In_ ComparisonType nCompareType,
        _In_ SearchFilterType nFilterType, _In_ const std::wstring& sFilterValue);

    /// <summary>
    /// Initializes a result set by comparing current memory against another result set using the address filter from a third set.
    /// </summary>
    /// <param name="srSource">The result set to filter.</param>
    /// <param name="srAddresses">The result set specifying which addresses to examine.</param>
    /// <param name="nCompareType">Type of comparison to apply.</param>
    /// <param name="nFilterType">Type of filter to apply.</param>
    /// <param name="sFilterValue">Parameter for filter being applied.</param>
    /// <returns><c>true</c> if initialization was successful, <c>false</c> if the filter value was not supported</returns>
    bool Initialize(_In_ const SearchResults& srSource, _In_ const SearchResults& srAddresses,
        _In_ ComparisonType nCompareType, _In_ SearchFilterType nFilterType, _In_ const std::wstring& sFilterValue);

    /// <summary>
    /// Initializes a result set by comparing provided memory against another result set.
    /// </summary>
    /// <param name="srFirst">The result set to filter.</param>
    /// <param name="pReadMemory">A function that provides current values of memory.</param>
    /// <param name="nCompareType">Type of comparison to apply.</param>
    /// <param name="nFilterType">Type of filter to apply.</param>
    /// <param name="sFilterValue">Parameter for filter being applied.</param>
    /// <returns><c>true</c> if initialization was successful, <c>false</c> if the filter value was not supported</returns>
    bool Initialize(_In_ const SearchResults& srFirst, _In_ std::function<void(ra::ByteAddress,uint8_t*,size_t)> pReadMemory,
        _In_ ComparisonType nCompareType, _In_ SearchFilterType nFilterType, _In_ const std::wstring& sFilterValue);

    /// <summary>
    /// Gets the number of matching addresses.
    /// </summary>
    size_t MatchingAddressCount() const noexcept;

    /// <summary>
    /// Initializes a result set from a list of address/value pairs
    /// </summary>
    /// <param name="srResults">The results to populate from.</param>
    /// <param name="nType">Type of search to initialize.</param>
    void Initialize(_In_ const std::vector<SearchResult>& srResults, _In_ SearchType nSearchType);

    /// <summary>
    /// Gets the nIndex'th matching address.
    /// </summary>
    /// <param name="result">The result.</param>
    /// <returns><c>true</c> if result was populated, <c>false</c> if the index was invalid.</returns>
    bool GetMatchingAddress(gsl::index nIndex, _Out_ SearchResult& result) const noexcept;

    /// <summary>
    /// Gets and item from the results matching a result from another set of results.
    /// </summary>
    /// <param name="pSrcResult">The result to find a match for.</param>
    /// <param name="result">The result.</param>
    /// <returns><c>true</c> if result was populated, <c>false</c> if the index was invalid.</returns>
    bool GetMatchingAddress(const SearchResult& pSrcResult, _Out_ SearchResult& result) const noexcept;

    /// <summary>
    /// Gets the raw bytes at the specified address.
    /// </summary>
    bool GetBytes(ra::ByteAddress nAddress, unsigned char* pBuffer, size_t nCount) const noexcept;

    /// <summary>
    /// Gets a formatted value for the specified address.
    /// </summary>
    /// <param name="nAddress">Address of the start of the value.</param>
    /// <param name="nSize">Size of the value.</param>
    /// <returns>Formatted value.</returns>
    std::wstring GetFormattedValue(ra::ByteAddress nAddress, MemSize nSize) const;

    /// <summary>
    /// Updates the current value of the provided result.
    /// </summary>
    /// <param name="pResult">The result to update.</param>
    /// <param name="pPreviousResult">The previous value of the result.</param>
    /// <param name="sFormattedValue">Pointer to a string to populate with a textual representation of the value. [optional]</param>
    /// <returns><c>true</c> if the value changed, <c>false</c> if not.</returns>
    bool UpdateValue(SearchResult& pResult, _Out_ std::wstring* sFormattedValue,
        const ra::data::context::EmulatorContext& pEmulatorContext) const;

    /// <summary>
    /// Determines if a value compared to its previous value matches the filter.
    /// </summary>
    /// <param name="pPreviousResults">The previous results to compare against.</param>
    /// <param name="pResult">The result to test.</param>
    /// <returns><c>true</c> if the value still matches the filter, <c>false</c> if not.</returns>
    bool MatchesFilter(const SearchResults& pPreviousResults, SearchResult& pResult) const;

    /// <summary>
    /// Gets the type of search performed.
    /// </summary>
    SearchType GetSearchType() const noexcept { return m_nType; }

    /// <summary>
    /// Gets the size of the matching items.
    /// </summary>
    MemSize GetSize() const noexcept;

    /// <summary>
    /// Gets the type of filter that was applied to generate this search result.
    /// </summary>
    SearchFilterType GetFilterType() const noexcept { return m_nFilterType; }

    /// <summary>
    /// Gets the filter parameter that was applied to generate this search result.
    /// </summary>
    unsigned int GetFilterValue() const noexcept { return m_nFilterValue; }

    /// <summary>
    /// Gets the filter parameter that was applied to generate this search result.
    /// </summary>
    const std::wstring& GetFilterString() const noexcept { return m_sFilterValue; }

    /// <summary>
    /// Gets the filter comparison that was applied to generate this search result.
    /// </summary>
    ComparisonType GetFilterComparison() const noexcept { return m_nCompareType; }

    /// <summary>
    /// Determines whether the specified address appears in the matching address list.
    /// </summary>
    /// <param name="nAddress">The n address.</param>
    /// <returns><c>true</c> if the specified address is in the matching address list; otherwise
    /// <c>false</c>.</returns>
    bool ContainsAddress(ra::ByteAddress nAddress) const;

    /// <summary>
    /// Removes an entry from the matching address list.
    /// </summary>
    /// <param name="pResult">The result to remove.</param>
    /// <returns><c>true</c> if the specified address was removed from the matching address list;
    /// otherwise <c>false</c>.</returns>
    bool ExcludeResult(const SearchResult& pResult);

private:
    void MergeSearchResults(const SearchResults& srMemory, const SearchResults& srAddresses);

    std::vector<data::search::MemBlock> m_vBlocks;
    SearchType m_nType = SearchType::EightBit;

    friend class search::SearchImpl;
    search::SearchImpl* m_pImpl = nullptr;

    ComparisonType m_nCompareType = ComparisonType::Equals;
    SearchFilterType m_nFilterType = SearchFilterType::None;
    unsigned int m_nFilterValue = 0U;
    std::wstring m_sFilterValue;
};

} // namespace services
} // namespace ra

#endif /* !SEARCHRESULTS_H */
