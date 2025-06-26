#ifndef SEARCHIMPL_H
#define SEARCHIMPL_H
#pragma once

#include "data\Types.hh"

#include "services\SearchResults.h"

// define this to use the generic filtering code for all search types
// if defined, specialized templated code will be used for little endian searches
#undef DISABLE_TEMPLATED_SEARCH

using ra::data::search::MemBlock;

namespace ra {
namespace services {
namespace search {

class SearchImpl
{
public:
    SearchImpl() noexcept = default;
    virtual ~SearchImpl() noexcept = default;
    SearchImpl(const SearchImpl&) noexcept = delete;
    SearchImpl& operator=(const SearchImpl&) noexcept = delete;
    SearchImpl(SearchImpl&&) noexcept = delete;
    SearchImpl& operator=(SearchImpl&&) noexcept = delete;

    // Gets the number of bytes after an address that are required to hold the data at the address
    virtual uint32_t GetPadding() const noexcept { return 0U; }

    // Gets the number of bytes per virtual address
    virtual uint32_t GetStride() const noexcept { return 1U; }

    // Gets the size of values handled by this implementation
    virtual MemSize GetMemSize() const noexcept { return MemSize::EightBit; }

    // Gets the number of addresses that are represented by the specified number of bytes
    virtual uint32_t GetAddressCountForBytes(uint32_t nBytes) const noexcept
    {
        return nBytes - GetPadding();
    }

    // Gets the virtual address for a real address
    virtual ra::ByteAddress ConvertFromRealAddress(ra::ByteAddress nAddress) const noexcept
    {
        return nAddress;
    }

    // Gets the real address for a virtual address
    virtual ra::ByteAddress ConvertToRealAddress(ra::ByteAddress nAddress) const noexcept
    {
        return nAddress;
    }

    // Determines if the specified real address exists in the collection of matched addresses.
    virtual bool ContainsAddress(const SearchResults& srResults, ra::ByteAddress nAddress) const;

    void AddBlocks(SearchResults& srNew, std::vector<ra::ByteAddress>& vMatches, std::vector<uint8_t>& vMemory,
                   ra::ByteAddress nPreviousBlockFirstAddress, uint32_t nPadding) const;

    // Removes the result associated to the specified real address from the collection of matched addresses.
    virtual bool ExcludeResult(SearchResults& srResults, const SearchResult& pResult) const;

    virtual bool ValidateFilterValue(SearchResults& srNew) const noexcept(false);

    // populates a vector of addresses that match the specified filter when applied to a previous search result
    virtual void ApplyFilter(SearchResults& srNew, const SearchResults& srPrevious,
                             std::function<void(ra::ByteAddress, uint8_t*, size_t)> pReadMemory) const;

    // gets the nIndex'th search result
    bool GetMatchingAddress(const SearchResults& srResults, gsl::index nIndex,
                            _Out_ SearchResult& result) const noexcept;

    /// <summary>
    /// Gets a value from the search results using the provided virtual address (in result.nAddress)
    /// </summary>
    /// <param name="srResults">The search results to read from.</param>
    /// <param name="result">Contains the size and virtual address of the memory to read.</param>
    /// <returns><c>true</c> if the memory was available in the block, <c>false</c> if not.</returns>
    /// <remarks>Sets result.nValue to the value from the captured memory, and changes result.nAddress to a real
    /// address</remarks>
    bool GetValueAtVirtualAddress(const SearchResults& srResults, SearchResult& result) const noexcept;

    virtual std::wstring GetFormattedValue(const SearchResults&, const SearchResult& pResult) const;

    // gets the formatted value for the search result at the specified real address
    virtual std::wstring GetFormattedValue(const SearchResults& pResults, ra::ByteAddress nAddress,
                                           MemSize nSize) const;

    // updates the provided result with the current value at the provided real address
    virtual bool UpdateValue(const SearchResults& pResults, SearchResult& pResult, _Out_ std::wstring* sFormattedValue,
                             const ra::data::context::EmulatorContext& pEmulatorContext) const;

    /// <summary>
    /// Determines if the provided search result would appear in pResults if it were freshly populated from
    /// pPreviousResults
    /// </summary>
    /// <param name="pResults">The filtered results.</param>
    /// <param name="pPreviousResults">The results that pResults was constructed from.</param>
    /// <param name="pResult">A result to check, contains the current value and real address of the memory to
    /// check.</param> <returns><c>true</c> if the memory result would be included in pResults, <c>false</c> if
    /// not.</returns>
    virtual bool MatchesFilter(const SearchResults& pResults, const SearchResults& pPreviousResults,
                               SearchResult& pResult) const noexcept(false);

protected:
    // generic implementation for less used search types
    virtual void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop, const MemBlock& pPreviousBlock,
                                     ComparisonType nComparison, unsigned nConstantValue,
                                     std::vector<ra::ByteAddress>& vMatches) const;

    // generic implementation for less used search types
    virtual void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop, const MemBlock& pPreviousBlock,
                                    ComparisonType nComparison, unsigned nAdjustment,
                                    std::vector<ra::ByteAddress>& vMatches) const;

    template<typename T>
    _NODISCARD static constexpr bool CompareValues(_In_ T nLeft, _In_ T nRight,
                                                   _In_ ComparisonType nCompareType) noexcept
    {
        switch (nCompareType)
        {
            case ComparisonType::Equals:
                return nLeft == nRight;
            case ComparisonType::LessThan:
                return nLeft < nRight;
            case ComparisonType::LessThanOrEqual:
                return nLeft <= nRight;
            case ComparisonType::GreaterThan:
                return nLeft > nRight;
            case ComparisonType::GreaterThanOrEqual:
                return nLeft >= nRight;
            case ComparisonType::NotEqualTo:
                return nLeft != nRight;
            default:
                return false;
        }
    }

    // templated implementation for commonly used search types for best performance
#ifndef DISABLE_TEMPLATED_SEARCH
#pragma warning(push)
#pragma warning(disable : 5045)
    template<typename TSize, bool TIsConstantFilter, int TStride, ComparisonType TComparison>
    void ApplyCompareFilterLittleEndian(const uint8_t* pBytes, const uint8_t* pBytesStop,
                                        const MemBlock& pPreviousBlock, unsigned nAdjustment,
                                        std::vector<ra::ByteAddress>& vMatches) const
    {
        const auto* pScan = pBytes;
        if (pScan == nullptr)
            return;

        ra::ByteAddress nAddress = pPreviousBlock.GetFirstAddress();
        constexpr int TBlockStride = TIsConstantFilter ? 0 : TStride;
        const auto* pBlockBytes = TIsConstantFilter ? pScan : pPreviousBlock.GetBytes();
        Expects(pBlockBytes != nullptr);

        const auto* pMatchingAddresses = pPreviousBlock.GetMatchingAddressPointer();
        if (!pMatchingAddresses)
        {
            // all addresses in previous block match
            for (; pScan < pBytesStop; pScan += TStride, pBlockBytes += TBlockStride)
            {
                GSL_SUPPRESS_TYPE1
                {
                    const uint32_t nValue1 = *(reinterpret_cast<const TSize*>(pScan));
                    const uint32_t nValue2 =
                        TIsConstantFilter ? nAdjustment : *(reinterpret_cast<const TSize*>(pBlockBytes)) + nAdjustment;

                    if (CompareValues(nValue1, nValue2, TComparison))
                        vMatches.push_back(nAddress);
                }

                ++nAddress;
            }
        }
        else
        {
            // only a subset of addresses in the previous block match
            uint8_t nMask = 0x01;
            for (; pScan < pBytesStop; pScan += TStride, pBlockBytes += TBlockStride)
            {
                const bool bPreviousMatch = *pMatchingAddresses & nMask;
                if (nMask == 0x80)
                {
                    nMask = 0x01;
                    ++pMatchingAddresses;
                }
                else
                {
                    nMask <<= 1;
                }

                if (bPreviousMatch)
                {
                    GSL_SUPPRESS_TYPE1
                    {
                        const uint32_t nValue1 = *(reinterpret_cast<const TSize*>(pScan));
                        const uint32_t nValue2 = TIsConstantFilter
                                                         ? nAdjustment
                                                         : *(reinterpret_cast<const TSize*>(pBlockBytes)) + nAdjustment;

                        if (CompareValues(nValue1, nValue2, TComparison))
                            vMatches.push_back(nAddress);
                    }
                }

                ++nAddress;
            }
        }
    }

    /// <summary>
    /// Finds items in a block of memory that match a specified filter.
    /// </summary>
    /// <typeparam name="TSize">The size of each item being compared</typeparam>
    /// <typeparam name="TIsConstantFilter"><c>true</c> if nAdjustment is a constant.</typeparam>
    /// <typeparam name="TStride">The amount to advance the pointer after each comparison</typeparam>
    /// <param name="pBytes">The memory to examine</param>
    /// <param name="pBytesStop">When to stop examining memory</param>
    /// <param name="pPreviousBlock">The block being compared against</param>
    /// <param name="nComparison">The comparison to perform</param>
    /// <param name="nAdjustment">The adjustment to apply to each value before comparing, or the constant to compare
    /// against</param> <param name="vMatches">[out] The list of matching addresses</param>
    template<typename TSize, bool TIsConstantFilter, int TStride = 1>
    void ApplyCompareFilterLittleEndian(const uint8_t* pBytes, const uint8_t* pBytesStop,
                                        const MemBlock& pPreviousBlock, ComparisonType nComparison,
                                        unsigned nAdjustment, std::vector<ra::ByteAddress>& vMatches) const
    {
        switch (nComparison)
        {
            case ComparisonType::Equals:
                return ApplyCompareFilterLittleEndian<TSize, TIsConstantFilter, TStride, ComparisonType::Equals>(
                    pBytes, pBytesStop, pPreviousBlock, nAdjustment, vMatches);
            case ComparisonType::LessThan:
                return ApplyCompareFilterLittleEndian<TSize, TIsConstantFilter, TStride, ComparisonType::LessThan>(
                    pBytes, pBytesStop, pPreviousBlock, nAdjustment, vMatches);
            case ComparisonType::LessThanOrEqual:
                return ApplyCompareFilterLittleEndian<TSize, TIsConstantFilter, TStride,
                                                      ComparisonType::LessThanOrEqual>(
                    pBytes, pBytesStop, pPreviousBlock, nAdjustment, vMatches);
            case ComparisonType::GreaterThan:
                return ApplyCompareFilterLittleEndian<TSize, TIsConstantFilter, TStride, ComparisonType::GreaterThan>(
                    pBytes, pBytesStop, pPreviousBlock, nAdjustment, vMatches);
            case ComparisonType::GreaterThanOrEqual:
                return ApplyCompareFilterLittleEndian<TSize, TIsConstantFilter, TStride,
                                                      ComparisonType::GreaterThanOrEqual>(
                    pBytes, pBytesStop, pPreviousBlock, nAdjustment, vMatches);
            case ComparisonType::NotEqualTo:
                return ApplyCompareFilterLittleEndian<TSize, TIsConstantFilter, TStride, ComparisonType::NotEqualTo>(
                    pBytes, pBytesStop, pPreviousBlock, nAdjustment, vMatches);
        }
    }
#pragma warning(pop)
#else // #ifndef DISABLE_TEMPLATED_SEARCH
    template<typename TSize, bool TIsConstantFilter, int TStride = 1>
    void ApplyCompareFilterLittleEndian(const uint8_t* pBytes, const uint8_t* pBytesStop,
                                        const MemBlock& pPreviousBlock, ComparisonType nComparison,
                                        unsigned nAdjustment, std::vector<ra::ByteAddress>& vMatches) const
    {
        if (TIsConstantFilter)
            SearchImpl::ApplyConstantFilter(pBytes, pBytesStop, pPreviousBlock, nComparison, nAdjustment, vMatches);
        else
            SearchImpl::ApplyCompareFilter(pBytes, pBytesStop, pPreviousBlock, nComparison, nAdjustment, vMatches);
    }
#endif

    static void SetFilterValue(SearchResults& srResults, uint32_t nValue) noexcept
    {
        srResults.m_nFilterValue = nValue;
    }

    static bool HasFirstAddress(const SearchResults& srResults) noexcept
    {
        return !srResults.m_vBlocks.empty();
    }

    static ra::ByteAddress GetFirstAddress(const SearchResults& srResults) noexcept
    {
        return srResults.m_vBlocks.front().GetFirstAddress();
    }

    static const std::vector<MemBlock>& GetBlocks(const SearchResults& srResults) noexcept
    {
        return srResults.m_vBlocks;
    }

    // Removes the result associated to the specified virtual address from the collection of matched addresses.
    static bool ExcludeAddress(SearchResults& srResults, ra::ByteAddress nAddress)
    {
        for (auto& pBlock : srResults.m_vBlocks)
        {
            if (pBlock.ContainsAddress(nAddress))
            {
                pBlock.ExcludeMatchingAddress(nAddress);
                return true;
            }
        }

        return false;
    }

    /// <summary>
    /// Gets a value from a memory block using the provided virtual address (in result.nAddress)
    /// </summary>
    /// <param name="block">The memory block to read from.</param>
    /// <param name="result">Contains the size and virtual address of the memory to read.</param>
    /// <returns><c>true</c> if the memory was available in the block, <c>false</c> if not.</returns>
    /// <remarks>Sets result.nValue to the value from the captured memory, and changes result.nAddress to a real
    /// address</remarks>
    virtual bool GetValueFromMemBlock(const MemBlock& block, SearchResult& result) const noexcept;

    virtual uint32_t BuildValue(const uint8_t* ptr) const noexcept;

    static MemBlock& AddBlock(SearchResults& srResults, ra::ByteAddress nAddress, uint32_t nSize,
                              uint32_t nMaxAddresses)
    {
        return srResults.m_vBlocks.emplace_back(nAddress, nSize, nMaxAddresses);
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif /* !SEARCHIMPL_H */
