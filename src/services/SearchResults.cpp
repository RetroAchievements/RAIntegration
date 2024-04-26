#include "SearchResults.h"

#include "RA_Defs.h"
#include "RA_StringUtils.h"

#include "ra_utility.h"

#include "data\context\EmulatorContext.hh"

#include "services\ServiceLocator.hh"

#include <algorithm>

#include <rcheevos\src\rcheevos\rc_internal.h>

// define this to use the generic filtering code for all search types
// if defined, specialized templated code will be used for little endian searches
#undef DISABLE_TEMPLATED_SEARCH

namespace ra {
namespace services {

namespace impl {

template<typename T>
_NODISCARD static constexpr bool CompareValues(_In_ T nLeft, _In_ T nRight, _In_ ComparisonType nCompareType) noexcept
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
    virtual unsigned int GetPadding() const noexcept { return 0U; }

    // Gets the number of bytes per virtual address
    virtual unsigned int GetStride() const noexcept { return 1U; }

    // Gets the size of values handled by this implementation
    virtual MemSize GetMemSize() const noexcept { return MemSize::EightBit; }

    // Gets the number of addresses that are represented by the specified number of bytes
    virtual unsigned int GetAddressCountForBytes(unsigned int nBytes) const noexcept
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
    virtual bool ContainsAddress(const SearchResults& srResults, ra::ByteAddress nAddress) const
    {
        if (nAddress & (GetStride() - 1))
            return false;

        nAddress = ConvertFromRealAddress(nAddress);
        for (const auto& pBlock : srResults.m_vBlocks)
        {
            if (pBlock.ContainsAddress(nAddress))
                return pBlock.ContainsMatchingAddress(nAddress);
        }

        return false;
    }

    // Removes the result associated to the specified real address from the collection of matched addresses.
    virtual bool ExcludeResult(SearchResults& srResults, const SearchResults::Result& pResult) const
    {
        if (pResult.nAddress & (GetStride() - 1))
            return false;

        return ExcludeAddress(srResults, ConvertFromRealAddress(pResult.nAddress));
    }

    virtual bool ValidateFilterValue(SearchResults& srNew) const noexcept(false)
    {
        // convert the filter string into a value
        if (!srNew.GetFilterString().empty())
        {
            const wchar_t* pStart = srNew.GetFilterString().c_str();
            wchar_t* pEnd;

            // try decimal first
            srNew.m_nFilterValue = std::wcstoul(pStart, &pEnd, 10);
            if (pEnd && *pEnd)
            {
                // decimal parse failed, try hex
                srNew.m_nFilterValue = std::wcstoul(pStart, &pEnd, 16);
                if (*pEnd)
                {
                    // hex parse failed
                    return false;
                }
            }
        }
        else if (srNew.GetFilterType() == SearchFilterType::Constant)
        {
            // constant cannot be empty string
            return false;
        }

        return true;
    }

    // populates a vector of addresses that match the specified filter when applied to a previous search result
    virtual void ApplyFilter(SearchResults& srNew, const SearchResults& srPrevious, std::function<void(ra::ByteAddress,uint8_t*,size_t)> pReadMemory) const
    {
        unsigned int nLargestBlock = 0U;
        for (auto& block : srPrevious.m_vBlocks)
        {
            if (block.GetBytesSize() > nLargestBlock)
                nLargestBlock = block.GetBytesSize();
        }

        std::vector<unsigned char> vMemory(nLargestBlock);
        std::vector<ra::ByteAddress> vMatches;

        unsigned int nAdjustment = 0;
        switch (srNew.GetFilterType())
        {
            case SearchFilterType::LastKnownValuePlus:
                nAdjustment = srNew.GetFilterValue();
                break;
            case SearchFilterType::LastKnownValueMinus:
                nAdjustment = ra::to_unsigned(-ra::to_signed(srNew.GetFilterValue()));
                break;
            default:
                break;
        }

        for (auto& block : srPrevious.m_vBlocks)
        {
            const auto nRealAddress = ConvertToRealAddress(block.GetFirstAddress());
            pReadMemory(nRealAddress, vMemory.data(), block.GetBytesSize());

            const auto nStop = block.GetBytesSize() - GetPadding();

            switch (srNew.GetFilterType())
            {
                case SearchFilterType::Constant:
                    ApplyConstantFilter(vMemory.data(), vMemory.data() + nStop, block,
                        srNew.GetFilterComparison(), srNew.GetFilterValue(), vMatches);
                    break;

                default:
                    // if an entire block is unchanged, everything in it is equal to the LastKnownValue
                    // or InitialValue and we don't have to check every address.
                    if (memcmp(vMemory.data(), block.GetBytes(), block.GetBytesSize()) == 0)
                    {
                        switch (srNew.GetFilterComparison())
                        {
                            case ComparisonType::Equals:
                            case ComparisonType::GreaterThanOrEqual:
                            case ComparisonType::LessThanOrEqual:
                                if (nAdjustment == 0)
                                {
                                    // entire block matches, copy the old block
                                    MemBlock& newBlock = AddBlock(srNew, block.GetFirstAddress(), block.GetBytesSize(), block.GetMaxAddresses());
                                    memcpy(newBlock.GetBytes(), block.GetBytes(), block.GetBytesSize());
                                    newBlock.CopyMatchingAddresses(block);
                                    continue;
                                }
                                else if (srNew.GetFilterComparison() == ComparisonType::Equals)
                                {
                                    // entire block matches, so adjustment won't match. discard the block
                                    continue;
                                }
                                break;

                            default:
                                if (nAdjustment == 0)
                                {
                                    // entire block matches, discard it
                                    continue;
                                }
                                break;
                        }

                        // have to check individual addresses to see if adjustment matches
                    }

                    // per-address comparison
                    ApplyCompareFilter(vMemory.data(), vMemory.data() + nStop, block,
                        srNew.GetFilterComparison(), nAdjustment, vMatches);
                    break;
            }

            if (!vMatches.empty())
            {
                AddBlocks(srNew, vMatches, vMemory, block.GetFirstAddress(), GetPadding());
                vMatches.clear();
            }
        }
    }

    // gets the nIndex'th search result
    bool GetMatchingAddress(const SearchResults& srResults, gsl::index nIndex, _Out_ SearchResults::Result& result) const noexcept
    {
        result.nSize = GetMemSize();

        for (const auto& pBlock : srResults.m_vBlocks)
        {
            if (nIndex < gsl::narrow_cast<gsl::index>(pBlock.GetMatchingAddressCount()))
            {
                result.nAddress = pBlock.GetMatchingAddress(nIndex);
                return GetValueFromMemBlock(pBlock, result);
            }

            nIndex -= pBlock.GetMatchingAddressCount();
        }

        return false;
    }

    /// <summary>
    /// Gets a value from the search results using the provided virtual address (in result.nAddress)
    /// </summary>
    /// <param name="srResults">The search results to read from.</param>
    /// <param name="result">Contains the size and virtual address of the memory to read.</param>
    /// <returns><c>true</c> if the memory was available in the block, <c>false</c> if not.</returns>
    /// <remarks>Sets result.nValue to the value from the captured memory, and changes result.nAddress to a real address</remarks>
    bool GetValueAtVirtualAddress(const SearchResults& srResults, SearchResults::Result& result) const noexcept
    {
        for (const auto& block : srResults.m_vBlocks)
        {
            if (GetValueFromMemBlock(block, result))
                return true;
        }

        result.nValue = 0;
        result.nAddress = ConvertToRealAddress(result.nAddress);
        return false;
    }

    virtual std::wstring GetFormattedValue(const SearchResults&, const SearchResults::Result& pResult) const
    {
        return L"0x" + ra::data::MemSizeFormat(pResult.nValue, pResult.nSize, MemFormat::Hex);
    }

    // gets the formatted value for the search result at the specified real address
    virtual std::wstring GetFormattedValue(const SearchResults& pResults, ra::ByteAddress nAddress, MemSize nSize) const
    {
        SearchResults::Result pResult{ ConvertFromRealAddress(nAddress), 0, nSize };
        if (GetValueAtVirtualAddress(pResults, pResult))
            return GetFormattedValue(pResults, pResult);

        return L"";
    }

    // updates the provided result with the current value at the provided real address
    virtual bool UpdateValue(const SearchResults& pResults, SearchResults::Result& pResult,
        _Out_ std::wstring* sFormattedValue, const ra::data::context::EmulatorContext& pEmulatorContext) const
    {
        const unsigned int nPreviousValue = pResult.nValue;
        pResult.nValue = pEmulatorContext.ReadMemory(pResult.nAddress, pResult.nSize);

        if (sFormattedValue)
            *sFormattedValue = GetFormattedValue(pResults, pResult);

        return (pResult.nValue != nPreviousValue);
    }

    /// <summary>
    /// Determines if the provided search result would appear in pResults if it were freshly populated from pPreviousResults
    /// </summary>
    /// <param name="pResults">The filtered results.</param>
    /// <param name="pPreviousResults">The results that pResults was constructed from.</param>
    /// <param name="pResult">A result to check, contains the current value and real address of the memory to check.</param>
    /// <returns><c>true</c> if the memory result would be included in pResults, <c>false</c> if not.</returns>
    virtual bool MatchesFilter(const SearchResults& pResults, const SearchResults& pPreviousResults,
        SearchResults::Result& pResult) const noexcept(false)
    {
        unsigned int nPreviousValue = 0;
        if (pResults.GetFilterType() == SearchFilterType::Constant)
        {
            nPreviousValue = pResults.GetFilterValue();
        }
        else
        {
            SearchResults::Result pPreviousResult{ pResult };
            pPreviousResult.nAddress = ConvertFromRealAddress(pResult.nAddress);
            GetValueAtVirtualAddress(pPreviousResults, pPreviousResult);

            switch (pResults.GetFilterType())
            {
                case SearchFilterType::LastKnownValue:
                case SearchFilterType::InitialValue:
                    nPreviousValue = pPreviousResult.nValue;
                    break;

                case SearchFilterType::LastKnownValuePlus:
                    nPreviousValue = pPreviousResult.nValue + pResults.GetFilterValue();
                    break;

                case SearchFilterType::LastKnownValueMinus:
                    nPreviousValue = pPreviousResult.nValue - pResults.GetFilterValue();
                    break;

                default:
                    return false;
            }
        }

        return CompareValues(pResult.nValue, nPreviousValue, pResults.GetFilterComparison());
    }

protected:
    // generic implementation for less used search types
    virtual void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::ByteAddress>& vMatches) const
    {
        const auto nBlockAddress = pPreviousBlock.GetFirstAddress();
        const auto nStride = GetStride();
        const auto* pMatchingAddresses = pPreviousBlock.GetMatchingAddressPointer();

        for (const auto* pScan = pBytes; pScan < pBytesStop; pScan += nStride)
        {
            const unsigned int nValue1 = BuildValue(pScan);
            if (CompareValues(nValue1, nConstantValue, nComparison))
            {
                const ra::ByteAddress nAddress = nBlockAddress +
                    ConvertFromRealAddress(gsl::narrow_cast<unsigned int>(pScan - pBytes));
                if (pPreviousBlock.HasMatchingAddress(pMatchingAddresses, nAddress))
                    vMatches.push_back(nAddress);
            }
        }
    }

    // generic implementation for less used search types
    virtual void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const
    {
        const auto* pBlockBytes = pPreviousBlock.GetBytes();
        const auto nBlockAddress = pPreviousBlock.GetFirstAddress();
        const auto nStride = GetStride();
        const auto* pMatchingAddresses = pPreviousBlock.GetMatchingAddressPointer();

        for (const auto* pScan = pBytes; pScan < pBytesStop; pScan += nStride, pBlockBytes += nStride)
        {
            const unsigned int nValue1 = BuildValue(pScan);
            const unsigned int nValue2 = BuildValue(pBlockBytes) + nAdjustment;
            if (CompareValues(nValue1, nValue2, nComparison))
            {
                const ra::ByteAddress nAddress = nBlockAddress +
                    ConvertFromRealAddress(gsl::narrow_cast<unsigned int>(pScan - pBytes));
                if (pPreviousBlock.HasMatchingAddress(pMatchingAddresses, nAddress))
                    vMatches.push_back(nAddress);
            }
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
                    const unsigned int nValue1 = *(reinterpret_cast<const TSize*>(pScan));
                    const unsigned int nValue2 = TIsConstantFilter ? nAdjustment :
                        *(reinterpret_cast<const TSize*>(pBlockBytes)) + nAdjustment;

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
                        const unsigned int nValue1 = *(reinterpret_cast<const TSize*>(pScan));
                        const unsigned int nValue2 = TIsConstantFilter ? nAdjustment :
                            *(reinterpret_cast<const TSize*>(pBlockBytes)) + nAdjustment;

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
    /// <param name="nAdjustment">The adjustment to apply to each value before comparing, or the constant to compare against</param>
    /// <param name="vMatches">[out] The list of matching addresses</param>
    template<typename TSize, bool TIsConstantFilter, int TStride = 1>
    void ApplyCompareFilterLittleEndian(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const
    {
        switch (nComparison)
        {
            case ComparisonType::Equals:
                return ApplyCompareFilterLittleEndian<TSize, TIsConstantFilter, TStride, ComparisonType::Equals>
                    (pBytes, pBytesStop, pPreviousBlock, nAdjustment, vMatches);
            case ComparisonType::LessThan:
                return ApplyCompareFilterLittleEndian<TSize, TIsConstantFilter, TStride, ComparisonType::LessThan>
                    (pBytes, pBytesStop, pPreviousBlock, nAdjustment, vMatches);
            case ComparisonType::LessThanOrEqual:
                return ApplyCompareFilterLittleEndian<TSize, TIsConstantFilter, TStride, ComparisonType::LessThanOrEqual>
                    (pBytes, pBytesStop, pPreviousBlock, nAdjustment, vMatches);
            case ComparisonType::GreaterThan:
                return ApplyCompareFilterLittleEndian<TSize, TIsConstantFilter, TStride, ComparisonType::GreaterThan>
                    (pBytes, pBytesStop, pPreviousBlock, nAdjustment, vMatches);
            case ComparisonType::GreaterThanOrEqual:
                return ApplyCompareFilterLittleEndian<TSize, TIsConstantFilter, TStride, ComparisonType::GreaterThanOrEqual>
                    (pBytes, pBytesStop, pPreviousBlock, nAdjustment, vMatches);
            case ComparisonType::NotEqualTo:
                return ApplyCompareFilterLittleEndian<TSize, TIsConstantFilter, TStride, ComparisonType::NotEqualTo>
                    (pBytes, pBytesStop, pPreviousBlock, nAdjustment, vMatches);
        }
    }
 #pragma warning(pop)
#else // #ifndef DISABLE_TEMPLATED_SEARCH
    template<typename TSize, bool TIsConstantFilter, int TStride = 1>
    void ApplyCompareFilterLittleEndian(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const
    {
        if (TIsConstantFilter)
            SearchImpl::ApplyConstantFilter(pBytes, pBytesStop, pPreviousBlock, nComparison, nAdjustment, vMatches);
        else
            SearchImpl::ApplyCompareFilter(pBytes, pBytesStop, pPreviousBlock, nComparison, nAdjustment, vMatches);
    }
#endif

    static void SetFilterValue(SearchResults& srResults, unsigned int nValue) noexcept
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

    static const std::vector<impl::MemBlock>& GetBlocks(const SearchResults& srResults) noexcept
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
    /// <remarks>Sets result.nValue to the value from the captured memory, and changes result.nAddress to a real address</remarks>
    virtual bool GetValueFromMemBlock(const impl::MemBlock& block, SearchResults::Result& result) const noexcept
    {
        if (result.nAddress < block.GetFirstAddress())
            return false;

        const unsigned int nOffset = result.nAddress - block.GetFirstAddress();
        if (nOffset >= block.GetBytesSize() - GetPadding())
            return false;

        result.nValue = BuildValue(block.GetBytes() + nOffset);
        return true;
    }

    virtual unsigned int BuildValue(const unsigned char* ptr) const noexcept
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        return ptr[0];
    }

    static impl::MemBlock& AddBlock(SearchResults& srResults, ra::ByteAddress nAddress, unsigned int nSize, unsigned int nMaxAddresses)
    {
        return srResults.m_vBlocks.emplace_back(nAddress, nSize, nMaxAddresses);
    }

public:
    void AddBlocks(SearchResults& srNew, std::vector<ra::ByteAddress>& vMatches,
        std::vector<unsigned char>& vMemory, ra::ByteAddress nPreviousBlockFirstAddress, unsigned int nPadding) const
    {
        const gsl::index nStopIndex = gsl::narrow_cast<gsl::index>(vMatches.size()) - 1;
        gsl::index nFirstIndex = 0;
        gsl::index nLastIndex = 0;
        do
        {
            const auto nFirstMatchingAddress = vMatches.at(nFirstIndex);
            while (nLastIndex < nStopIndex && vMatches.at(nLastIndex + 1) - nFirstMatchingAddress < 64)
                nLastIndex++;

            // determine how many bytes we need to capture
            const auto nFirstRealAddress = ConvertToRealAddress(nFirstMatchingAddress);
            const auto nLastRealAddress = ConvertToRealAddress(vMatches.at(nLastIndex));
            const unsigned int nBlockSize = nLastRealAddress - nFirstRealAddress + 1 + nPadding;

            // determine the maximum number of addresses that can be associated to the captured bytes
            const auto nMaxAddresses = GetAddressCountForBytes(nBlockSize);

            // determine the address of the first captured byte
            const auto nFirstAddress = ConvertFromRealAddress(nFirstRealAddress);

            // allocate the new block
            MemBlock& block = AddBlock(srNew, nFirstAddress, nBlockSize, nMaxAddresses);

            // capture the subset of data that corresponds to the subset of matches
            const auto nOffset = nFirstRealAddress - ConvertToRealAddress(nPreviousBlockFirstAddress);
            memcpy(block.GetBytes(), &vMemory.at(nOffset), nBlockSize);

            // capture the matched addresses
            block.SetMatchingAddresses(vMatches, nFirstIndex, nLastIndex);

            nFirstIndex = nLastIndex + 1;
        } while (nFirstIndex < gsl::narrow_cast<gsl::index>(vMatches.size()));
    }
};

class FourBitSearchImpl : public SearchImpl
{
    MemSize GetMemSize() const noexcept override { return MemSize::Nibble_Lower; }

    unsigned int GetAddressCountForBytes(unsigned int nBytes) const noexcept override
    {
        return nBytes * 2;
    }

    ra::ByteAddress ConvertFromRealAddress(ra::ByteAddress nAddress) const noexcept override
    {
        return nAddress * 2;
    }

    ra::ByteAddress ConvertToRealAddress(ra::ByteAddress nAddress) const noexcept override
    {
        return nAddress / 2;
    }

    bool ContainsAddress(const SearchResults& srResults, ra::ByteAddress nAddress) const override
    {
        nAddress <<= 1;

        for (const auto& pBlock : GetBlocks(srResults))
        {
            if (pBlock.ContainsAddress(nAddress))
                return pBlock.ContainsMatchingAddress(nAddress) || pBlock.ContainsMatchingAddress(nAddress | 1);
        }

        return false;
    }

    bool ExcludeResult(SearchResults& srResults, const SearchResults::Result& pResult) const override
    {
        auto nAddress = pResult.nAddress << 1;
        if (pResult.nSize == MemSize::Nibble_Upper)
            nAddress |= 1;

        return ExcludeAddress(srResults, nAddress);
    }

    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        const auto nBlockAddress = pPreviousBlock.GetFirstAddress();
        const auto* pMatchingAddresses = pPreviousBlock.GetMatchingAddressPointer();
        for (const auto* pScan = pBytes; pScan < pBytesStop; ++pScan)
        {
            const unsigned int nValue1 = BuildValue(pScan);
            const unsigned int nValue1a = (nValue1 & 0x0F);
            const unsigned int nValue1b = ((nValue1 >> 4) & 0x0F);

            if (CompareValues(nValue1a, nConstantValue, nComparison))
            {
                const unsigned int nAddress = nBlockAddress + (gsl::narrow_cast<unsigned>(pScan - pBytes) << 1);
                if (pPreviousBlock.HasMatchingAddress(pMatchingAddresses, nAddress))
                    vMatches.push_back(nAddress);
            }

            if (CompareValues(nValue1b, nConstantValue, nComparison))
            {
                const unsigned int nAddress = (nBlockAddress + (gsl::narrow_cast<unsigned>(pScan - pBytes) << 1)) | 1;
                if (pPreviousBlock.HasMatchingAddress(pMatchingAddresses, nAddress))
                    vMatches.push_back(nAddress);
            }
        }
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        const auto* pBlockBytes = pPreviousBlock.GetBytes();
        const auto nBlockAddress = pPreviousBlock.GetFirstAddress();
        const auto* pMatchingAddresses = pPreviousBlock.GetMatchingAddressPointer();
        for (const auto* pScan = pBytes; pScan < pBytesStop; ++pScan)
        {
            const unsigned int nValue1 = BuildValue(pScan);
            const unsigned int nValue1a = (nValue1 & 0x0F);
            const unsigned int nValue1b = ((nValue1 >> 4) & 0x0F);
            const unsigned int nValue2 = BuildValue(pBlockBytes++);
            const unsigned int nValue2a = (nValue2 & 0x0F) + nAdjustment;
            const unsigned int nValue2b = ((nValue2 >> 4) & 0x0F) + nAdjustment;

            if (CompareValues(nValue1a, nValue2a, nComparison))
            {
                const unsigned int nAddress = nBlockAddress + (gsl::narrow_cast<unsigned>(pScan - pBytes) << 1);
                if (pPreviousBlock.HasMatchingAddress(pMatchingAddresses, nAddress))
                    vMatches.push_back(nAddress);
            }

            if (CompareValues(nValue1b, nValue2b, nComparison))
            {
                const unsigned int nAddress = nBlockAddress + (gsl::narrow_cast<unsigned>(pScan - pBytes) << 1) | 1;
                if (pPreviousBlock.HasMatchingAddress(pMatchingAddresses, nAddress))
                    vMatches.push_back(nAddress);
            }
        }
    }

protected:
    bool GetValueFromMemBlock(const impl::MemBlock& block, SearchResults::Result& result) const noexcept override
    {
        if (result.nAddress & 1)
            result.nSize = MemSize::Nibble_Upper;

        result.nAddress >>= 1;

        const unsigned int nOffset = result.nAddress - (block.GetFirstAddress() >> 1);
        if (nOffset >= block.GetBytesSize())
            return false;

        result.nValue = BuildValue(block.GetBytes() + nOffset);

        if (result.nSize == MemSize::Nibble_Lower)
            result.nValue &= 0x0F;
        else
            result.nValue = (result.nValue >> 4) & 0x0F;

        return true;
    }
};

class EightBitSearchImpl : public SearchImpl
{
protected:
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        ApplyCompareFilterLittleEndian<uint8_t, true>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nConstantValue, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        ApplyCompareFilterLittleEndian<uint8_t, false>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }
};

class SixteenBitSearchImpl : public SearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::SixteenBit; }

    unsigned int GetPadding() const noexcept override { return 1U; }

    unsigned int BuildValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        GSL_SUPPRESS_TYPE1 return *reinterpret_cast<const uint16_t*>(ptr);
    }

protected:
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        ApplyCompareFilterLittleEndian<uint16_t, true>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nConstantValue, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        ApplyCompareFilterLittleEndian<uint16_t, false>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }
};

class TwentyFourBitSearchImpl : public SearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::TwentyFourBit; }

    unsigned int GetPadding() const noexcept override { return 2U; }

    unsigned int BuildValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        GSL_SUPPRESS_TYPE1 return *reinterpret_cast<const unsigned int*>(ptr) & 0x00FFFFFF;
    }
};

class ThirtyTwoBitSearchImpl : public SearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::ThirtyTwoBit; }

    unsigned int GetPadding() const noexcept override { return 3U; }

    unsigned int BuildValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        GSL_SUPPRESS_TYPE1 return *reinterpret_cast<const unsigned int*>(ptr);
    }

protected:
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        ApplyCompareFilterLittleEndian<uint32_t, true>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nConstantValue, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        ApplyCompareFilterLittleEndian<uint32_t, false>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }
};

class ThirtyTwoBitAlignedSearchImpl : public ThirtyTwoBitSearchImpl
{
public:
    unsigned int GetStride() const noexcept override { return 4; }

    unsigned int GetAddressCountForBytes(unsigned int nBytes) const noexcept override
    {
        return nBytes / 4;
    }

    ra::ByteAddress ConvertFromRealAddress(ra::ByteAddress nAddress) const noexcept override
    {
        return nAddress / 4;
    }

    ra::ByteAddress ConvertToRealAddress(ra::ByteAddress nAddress) const noexcept override
    {
        return nAddress * 4;
    }

protected:
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        ApplyCompareFilterLittleEndian<uint32_t, true, 4>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nConstantValue, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        ApplyCompareFilterLittleEndian<uint32_t, false, 4>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }


    bool GetValueFromMemBlock(const impl::MemBlock& block, SearchResults::Result& result) const noexcept override
    {
        const unsigned int nOffset = (result.nAddress - block.GetFirstAddress()) * 4;
        if (nOffset + 3 >= block.GetBytesSize())
            return false;

        result.nValue = BuildValue(block.GetBytes() + nOffset);
        result.nAddress *= 4;

        return true;
    }
};

class SixteenBitAlignedSearchImpl : public SixteenBitSearchImpl
{
public:
    unsigned int GetStride() const noexcept override { return 2; }

    unsigned int GetAddressCountForBytes(unsigned int nBytes) const noexcept override
    {
        return nBytes / 2;
    }

    ra::ByteAddress ConvertFromRealAddress(ra::ByteAddress nAddress) const noexcept override
    {
        return nAddress / 2;
    }

    ra::ByteAddress ConvertToRealAddress(ra::ByteAddress nAddress) const noexcept override
    {
        return nAddress * 2;
    }

protected:
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        ApplyCompareFilterLittleEndian<uint16_t, true, 2>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nConstantValue, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        ApplyCompareFilterLittleEndian<uint16_t, false, 2>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }


    bool GetValueFromMemBlock(const impl::MemBlock& block, SearchResults::Result& result) const noexcept override
    {
        const unsigned int nOffset = (result.nAddress - block.GetFirstAddress()) * 2;
        if (nOffset + 1 >= block.GetBytesSize())
            return false;

        result.nValue = BuildValue(block.GetBytes() + nOffset);
        result.nAddress *= 2;

        return true;
    }
};

class SixteenBitBigEndianSearchImpl : public SixteenBitSearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::SixteenBitBigEndian; }

protected:
    unsigned int BuildValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        return (ptr[0] << 8) | ptr[1];
    }

protected:
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        SearchImpl::ApplyConstantFilter(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nConstantValue, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        SearchImpl::ApplyCompareFilter(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }
};

class SixteenBitBigEndianAlignedSearchImpl : public SixteenBitAlignedSearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::SixteenBitBigEndian; }

protected:
    unsigned int BuildValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        return (ptr[0] << 8) | ptr[1];
    }

protected:
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        SearchImpl::ApplyConstantFilter(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nConstantValue, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        SearchImpl::ApplyCompareFilter(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }
};

class ThirtyTwoBitBigEndianSearchImpl : public ThirtyTwoBitSearchImpl
{
    MemSize GetMemSize() const noexcept override { return MemSize::ThirtyTwoBitBigEndian; }

    unsigned int BuildValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        return (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
    }

protected:
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        SearchImpl::ApplyConstantFilter(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nConstantValue, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        SearchImpl::ApplyCompareFilter(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }
};

class ThirtyTwoBitBigEndianAlignedSearchImpl : public ThirtyTwoBitAlignedSearchImpl
{
    MemSize GetMemSize() const noexcept override { return MemSize::ThirtyTwoBitBigEndian; }

    unsigned int BuildValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        return (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
    }

protected:
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        SearchImpl::ApplyConstantFilter(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nConstantValue, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        SearchImpl::ApplyCompareFilter(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }
};

class BitCountSearchImpl : public SearchImpl
{
protected:
    // captured value (result.nValue) is actually the raw byte
    MemSize GetMemSize() const noexcept override { return MemSize::BitCount; }

    unsigned int BuildValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        return GetBitCount(ptr[0]);
    }

    bool UpdateValue(const SearchResults& pResults, SearchResults::Result& pResult,
        _Out_ std::wstring* sFormattedValue, const ra::data::context::EmulatorContext& pEmulatorContext) const override
    {
        const unsigned int nPreviousValue = pResult.nValue;
        pResult.nValue = pEmulatorContext.ReadMemory(pResult.nAddress, MemSize::EightBit);

        if (sFormattedValue)
            *sFormattedValue = GetFormattedValue(pResults, pResult);

        return (pResult.nValue != nPreviousValue);
    }

    std::wstring GetFormattedValue(const SearchResults&, const SearchResults::Result& pResult) const override
    {
        return ra::StringPrintf(L"%u (%c%c%c%c%c%c%c%c)", GetBitCount(pResult.nValue),
            (pResult.nValue & 0x80) ? '1' : '0',
            (pResult.nValue & 0x40) ? '1' : '0',
            (pResult.nValue & 0x20) ? '1' : '0',
            (pResult.nValue & 0x10) ? '1' : '0',
            (pResult.nValue & 0x08) ? '1' : '0',
            (pResult.nValue & 0x04) ? '1' : '0',
            (pResult.nValue & 0x02) ? '1' : '0',
            (pResult.nValue & 0x01) ? '1' : '0');
    }

private:
    bool GetValueFromMemBlock(const impl::MemBlock& block, SearchResults::Result& result) const noexcept override
    {
        if (result.nAddress < block.GetFirstAddress())
            return false;

        const unsigned int nOffset = result.nAddress - block.GetFirstAddress();
        if (nOffset >= block.GetBytesSize() - GetPadding())
            return false;

        result.nValue = *(block.GetBytes() + nOffset);
        return true;
    }

    static unsigned int GetBitCount(unsigned nValue) noexcept
    {
        static const std::array<unsigned,16> vBits = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 };
        return vBits.at(nValue & 0x0F) + vBits.at((nValue >> 4) & 0x0F);
    }
};

class AsciiTextSearchImpl : public SearchImpl
{
public:
    // indicate that search results can be very wide
    MemSize GetMemSize() const noexcept override { return MemSize::Text; }

    bool ValidateFilterValue(SearchResults& srNew) const noexcept override
    {
        if (srNew.GetFilterString().empty())
        {
            // can't match 0 characters!
            if (srNew.GetFilterType() == SearchFilterType::Constant)
                return false;
        }

        return true;
    }

    // populates a vector of addresses that match the specified filter when applied to a previous search result
    void ApplyFilter(SearchResults& srNew, const SearchResults& srPrevious,  std::function<void(ra::ByteAddress,uint8_t*,size_t)> pReadMemory) const override
    {
        size_t nCompareLength = 16; // maximum length for generic search
        std::vector<unsigned char> vSearchText;
        GetSearchText(vSearchText, srNew.GetFilterString());

        if (!vSearchText.empty())
            nCompareLength = vSearchText.size();

        unsigned int nLargestBlock = 0U;
        for (auto& block : GetBlocks(srPrevious))
        {
            if (block.GetBytesSize() > nLargestBlock)
                nLargestBlock = block.GetBytesSize();
        }

        std::vector<unsigned char> vMemory(nLargestBlock);
        std::vector<ra::ByteAddress> vMatches;

        for (auto& block : GetBlocks(srPrevious))
        {
            pReadMemory(block.GetFirstAddress(), vMemory.data(), block.GetBytesSize());

            const auto nStop = block.GetBytesSize() - 1;
            const auto nFilterComparison = srNew.GetFilterComparison();
            const auto* pMatchingAddresses = block.GetMatchingAddressPointer();

            switch (srNew.GetFilterType())
            {
                default:
                case SearchFilterType::Constant:
                    for (unsigned int i = 0; i < nStop; ++i)
                    {
                        if (CompareMemory(&vMemory.at(i), &vSearchText.at(0), nCompareLength, nFilterComparison))
                        {
                            const unsigned int nAddress = block.GetFirstAddress() + i;
                            if (block.HasMatchingAddress(pMatchingAddresses, nAddress))
                                vMatches.push_back(nAddress);
                        }
                    }
                    break;

                case SearchFilterType::InitialValue:
                case SearchFilterType::LastKnownValue:
                    for (unsigned int i = 0; i < nStop; ++i)
                    {
                        if (CompareMemory(&vMemory.at(i), block.GetBytes() + i, nCompareLength, nFilterComparison))
                        {
                            const unsigned int nAddress = block.GetFirstAddress() + i;
                            if (block.HasMatchingAddress(pMatchingAddresses, nAddress))
                                vMatches.push_back(nAddress);
                        }
                    }
                    break;
            }

            if (!vMatches.empty())
            {
                // adjust the block size to account for the length of the string to ensure
                // the block contains the whole string
                AddBlocks(srNew, vMatches, vMemory, block.GetFirstAddress(), gsl::narrow_cast<unsigned int>(nCompareLength - 1));
                vMatches.clear();
            }
        }
    }

    virtual void GetSearchText(std::vector<unsigned char>& vText, const std::wstring& sText) const
    {
        for (const auto c : sText)
            vText.push_back(gsl::narrow_cast<unsigned char>(c));

        // NOTE: excludes null terminator
    }

    virtual bool CompareMemory(const unsigned char* pLeft,
        const unsigned char* pRight, size_t nCount, ComparisonType nCompareType) const
    {
        Expects(pLeft != nullptr);
        Expects(pRight != nullptr);

        do
        {
            // not exact match, compare the current character normally
            if (*pLeft != *pRight)
                return CompareValues(*pLeft, *pRight, nCompareType);

            // both strings terminated; exact match
            if (*pLeft == '\0')
                break;

            // proceed to next character
            ++pLeft;
            ++pRight;
        } while (--nCount);

        // exact match
        return (nCompareType == ComparisonType::Equals ||
            nCompareType == ComparisonType::GreaterThanOrEqual ||
            nCompareType == ComparisonType::LessThanOrEqual);
    }

    static void GetASCIIText(std::wstring& sText, const unsigned char* pBytes, size_t nBytes)
    {
        sText.clear();
        for (size_t i = 0; i < nBytes; ++i)
        {
            wchar_t c = gsl::narrow_cast<wchar_t>(*pBytes++);
            if (c == 0)
                break;

            if (c < 0x20 || c > 0x7E)
                c = L'\xFFFD';

            sText.push_back(c);
        }
    }

    std::wstring GetFormattedValue(const SearchResults& pResults, const SearchResults::Result& pResult) const override
    {
        return GetFormattedValue(pResults, pResult.nAddress, GetMemSize());
    }

    std::wstring GetFormattedValue(const SearchResults& pResults, ra::ByteAddress nAddress, MemSize) const override
    {
        std::array<unsigned char, 16> pBuffer = {0};
        pResults.GetBytes(nAddress, &pBuffer.at(0), pBuffer.size());

        std::wstring sText;
        GetASCIIText(sText, pBuffer.data(), pBuffer.size());

        return sText;
    }

    bool UpdateValue(const SearchResults&, SearchResults::Result& pResult,
        _Out_ std::wstring* sFormattedValue, const ra::data::context::EmulatorContext& pEmulatorContext) const override
    {
        std::array<unsigned char, 16> pBuffer;
        pEmulatorContext.ReadMemory(pResult.nAddress, &pBuffer.at(0), pBuffer.size());

        std::wstring sText;
        GetASCIIText(sText, pBuffer.data(), pBuffer.size());

        const unsigned int nPreviousValue = pResult.nValue;
        pResult.nValue = ra::StringHash(sText);

        if (sFormattedValue)
            sFormattedValue->swap(sText);

        return (pResult.nValue != nPreviousValue);
    }

    bool MatchesFilter(const SearchResults& pResults, const SearchResults& pPreviousResults,
        SearchResults::Result& pResult) const override
    {
        std::wstring sPreviousValue;
        if (pResults.GetFilterType() == SearchFilterType::Constant)
            sPreviousValue = pResults.GetFilterString();
        else
            sPreviousValue = GetFormattedValue(pPreviousResults, pResult);

        const std::wstring sValue = GetFormattedValue(pResults, pResult);
        const int nResult = wcscmp(sValue.c_str(), sPreviousValue.c_str());

        switch (pResults.GetFilterComparison())
        {
            case ComparisonType::Equals:
                return (nResult == 0);

            case ComparisonType::NotEqualTo:
                return (nResult != 0);

            case ComparisonType::GreaterThan:
                return (nResult > 0);

            case ComparisonType::LessThan:
                return (nResult < 0);

            case ComparisonType::GreaterThanOrEqual:
                return (nResult >= 0);

            case ComparisonType::LessThanOrEqual:
                return (nResult <= 0);

            default:
                return false;
        }
    }
};

class FloatSearchImpl : public ThirtyTwoBitSearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::Float; }

    std::wstring GetFormattedValue(const SearchResults&, const SearchResults::Result& pResult) const override
    {
        return ra::data::MemSizeFormat(pResult.nValue, pResult.nSize, MemFormat::Dec);
    }

    bool ValidateFilterValue(SearchResults& srNew) const noexcept override
    {
        // convert the filter string into a value
        if (!srNew.GetFilterString().empty())
        {
            const wchar_t* pStart = srNew.GetFilterString().c_str();
            wchar_t* pEnd;

            const float fFilterValue = std::wcstof(pStart, &pEnd);
            if (pEnd && *pEnd)
            {
                // parse failed
                return false;
            }

            SetFilterValue(srNew, DeconstructFloatValue(fFilterValue));
        }
        else if (srNew.GetFilterType() == SearchFilterType::Constant)
        {
            // constant cannot be empty string
            return false;
        }

        return true;
    }

    GSL_SUPPRESS_TYPE1
    bool MatchesFilter(const SearchResults& pResults, const SearchResults& pPreviousResults,
        SearchResults::Result& pResult) const noexcept override
    {
        float fPreviousValue = 0.0;
        if (pResults.GetFilterType() == SearchFilterType::Constant)
        {
            const auto nFilterValue = pResults.GetFilterValue();
            const auto* pFilterValue = reinterpret_cast<const unsigned char*>(&nFilterValue);
            fPreviousValue = BuildFloatValue(pFilterValue);
        }
        else
        {
            SearchResults::Result pPreviousResult{ pResult };
            GetValueAtVirtualAddress(pPreviousResults, pPreviousResult);
            const auto* pPreviousValue = reinterpret_cast<const unsigned char*>(&pPreviousResult.nValue);
            fPreviousValue = BuildFloatValue(pPreviousValue);
        }

        const auto* pValue = reinterpret_cast<const unsigned char*>(&pResult.nValue);
        const auto fValue = BuildFloatValue(pValue);
        return CompareValues(fValue, fPreviousValue, pResults.GetFilterComparison());
    }

protected:
    virtual float BuildFloatValue(const unsigned char* ptr) const noexcept
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        GSL_SUPPRESS_TYPE1 return *reinterpret_cast<const float*>(ptr);
    }

    virtual unsigned int DeconstructFloatValue(float fValue) const noexcept
    {
        return ra::data::FloatToU32(fValue, MemSize::Float);
    }

    GSL_SUPPRESS_TYPE1
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        if (nComparison == ComparisonType::Equals || nComparison == ComparisonType::NotEqualTo)
        {
            // for direct equality, we can just compare the raw bytes without converting
            ThirtyTwoBitSearchImpl::ApplyConstantFilter(pBytes, pBytesStop,
                pPreviousBlock, nComparison, nConstantValue, vMatches);
            return;
        }

        const auto nBlockAddress = pPreviousBlock.GetFirstAddress();
        const auto nStride = GetStride();
        const auto* pMatchingAddresses = pPreviousBlock.GetMatchingAddressPointer();

        const auto* pConstantValue = reinterpret_cast<const unsigned char*>(&nConstantValue);
        const float fConstantValue = BuildFloatValue(pConstantValue);

        for (const auto* pScan = pBytes; pScan < pBytesStop; pScan += nStride)
        {
            const float fValue1 = BuildFloatValue(pScan);
            if (CompareValues(fValue1, fConstantValue, nComparison))
            {
                const ra::ByteAddress nAddress = nBlockAddress +
                    ConvertFromRealAddress(gsl::narrow_cast<unsigned int>(pScan - pBytes));
                if (pPreviousBlock.HasMatchingAddress(pMatchingAddresses, nAddress))
                    vMatches.push_back(nAddress);
            }
        }
    }

    GSL_SUPPRESS_TYPE1
    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        if (nComparison == ComparisonType::Equals || nComparison == ComparisonType::NotEqualTo)
        {
            // for direct equality, we can just compare the raw bytes without converting
            ThirtyTwoBitSearchImpl::ApplyCompareFilter(pBytes, pBytesStop,
                pPreviousBlock, nComparison, nAdjustment, vMatches);
            return;
        }

        const auto* pBlockBytes = pPreviousBlock.GetBytes();
        const auto nBlockAddress = pPreviousBlock.GetFirstAddress();
        const auto nStride = GetStride();
        const auto* pMatchingAddresses = pPreviousBlock.GetMatchingAddressPointer();

        const auto* pAdjustment = reinterpret_cast<const unsigned char*>(&nAdjustment);
        const float fAdjustment = BuildFloatValue(pAdjustment);

        for (const auto* pScan = pBytes; pScan < pBytesStop; pScan += nStride, pBlockBytes += nStride)
        {
            const float fValue1 = BuildFloatValue(pScan);
            const float fValue2 = BuildFloatValue(pBlockBytes) + fAdjustment;
            if (CompareValues(fValue1, fValue2, nComparison))
            {
                const ra::ByteAddress nAddress = nBlockAddress +
                    ConvertFromRealAddress(gsl::narrow_cast<unsigned int>(pScan - pBytes));
                if (pPreviousBlock.HasMatchingAddress(pMatchingAddresses, nAddress))
                    vMatches.push_back(nAddress);
            }
        }
    }
};

class FloatBESearchImpl : public FloatSearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::FloatBigEndian; }

protected:
    GSL_SUPPRESS_TYPE1
    float BuildFloatValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        rc_typed_value_t value;
        value.type = RC_VALUE_TYPE_UNSIGNED;
        value.value.u32 = *reinterpret_cast<const unsigned int*>(ptr);
        rc_transform_memref_value(&value, RC_MEMSIZE_FLOAT_BE);

        return value.value.f32;
    }

    unsigned int DeconstructFloatValue(float fValue) const noexcept override
    {
        return ra::data::FloatToU32(fValue, MemSize::FloatBigEndian);
    }
};

class Double32SearchImpl : public FloatSearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::Double32; }

protected:
    GSL_SUPPRESS_TYPE1
    float BuildFloatValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        rc_typed_value_t value;
        value.type = RC_VALUE_TYPE_UNSIGNED;
        value.value.u32 = *reinterpret_cast<const unsigned int*>(ptr);
        rc_transform_memref_value(&value, RC_MEMSIZE_DOUBLE32);

        return value.value.f32;
    }

    unsigned int DeconstructFloatValue(float fValue) const noexcept override
    {
        return ra::data::FloatToU32(fValue, MemSize::Double32);
    }
};

class Double32BESearchImpl : public FloatSearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::Double32BigEndian; }

protected:
    GSL_SUPPRESS_TYPE1
    float BuildFloatValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        rc_typed_value_t value;
        value.type = RC_VALUE_TYPE_UNSIGNED;
        value.value.u32 = *reinterpret_cast<const unsigned int*>(ptr);
        rc_transform_memref_value(&value, RC_MEMSIZE_DOUBLE32_BE);

        return value.value.f32;
    }

    unsigned int DeconstructFloatValue(float fValue) const noexcept override
    {
        return ra::data::FloatToU32(fValue, MemSize::Double32BigEndian);
    }
};

class MBF32SearchImpl : public FloatSearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::MBF32; }

protected:
    GSL_SUPPRESS_TYPE1
    float BuildFloatValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        rc_typed_value_t value;
        value.type = RC_VALUE_TYPE_UNSIGNED;
        value.value.u32 = *reinterpret_cast<const unsigned int*>(ptr);
        rc_transform_memref_value(&value, RC_MEMSIZE_MBF32);

        return value.value.f32;
    }

    unsigned int DeconstructFloatValue(float fValue) const noexcept override
    {
        return ra::data::FloatToU32(fValue, MemSize::MBF32);
    }
};

class MBF32LESearchImpl : public FloatSearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::MBF32LE; }

protected:
    GSL_SUPPRESS_TYPE1
    float BuildFloatValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        rc_typed_value_t value;
        value.type = RC_VALUE_TYPE_UNSIGNED;
        value.value.u32 = *reinterpret_cast<const unsigned int*>(ptr);
        rc_transform_memref_value(&value, RC_MEMSIZE_MBF32_LE);

        return value.value.f32;
    }

    unsigned int DeconstructFloatValue(float fValue) const noexcept override
    {
        return ra::data::FloatToU32(fValue, MemSize::MBF32LE);
    }
};


static FourBitSearchImpl s_pFourBitSearchImpl;
static EightBitSearchImpl s_pEightBitSearchImpl;
static SixteenBitSearchImpl s_pSixteenBitSearchImpl;
static TwentyFourBitSearchImpl s_pTwentyFourBitSearchImpl;
static ThirtyTwoBitSearchImpl s_pThirtyTwoBitSearchImpl;
static SixteenBitAlignedSearchImpl s_pSixteenBitAlignedSearchImpl;
static ThirtyTwoBitAlignedSearchImpl s_pThirtyTwoBitAlignedSearchImpl;
static SixteenBitBigEndianSearchImpl s_pSixteenBitBigEndianSearchImpl;
static ThirtyTwoBitBigEndianSearchImpl s_pThirtyTwoBitBigEndianSearchImpl;
static SixteenBitBigEndianAlignedSearchImpl s_pSixteenBitBigEndianAlignedSearchImpl;
static ThirtyTwoBitBigEndianAlignedSearchImpl s_pThirtyTwoBitBigEndianAlignedSearchImpl;
static BitCountSearchImpl s_pBitCountSearchImpl;
static AsciiTextSearchImpl s_pAsciiTextSearchImpl;
static FloatSearchImpl s_pFloatSearchImpl;
static FloatBESearchImpl s_pFloatBESearchImpl;
static Double32SearchImpl s_pDouble32SearchImpl;
static Double32BESearchImpl s_pDouble32BESearchImpl;
static MBF32SearchImpl s_pMBF32SearchImpl;
static MBF32LESearchImpl s_pMBF32LESearchImpl;

uint8_t* MemBlock::AllocateMatchingAddresses() noexcept
{
    const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
    if (nAddressesSize > sizeof(m_vAddresses))
    {
        if (m_pAddresses == nullptr)
            m_pAddresses = new (std::nothrow) uint8_t[nAddressesSize];

        return m_pAddresses;
    }

    return &m_vAddresses[0];
}

void MemBlock::SetMatchingAddresses(std::vector<ra::ByteAddress>& vAddresses, gsl::index nFirstIndex, gsl::index nLastIndex)
{
    m_nMatchingAddresses = gsl::narrow_cast<unsigned int>(nLastIndex - nFirstIndex) + 1;

    if (m_nMatchingAddresses != m_nMaxAddresses)
    {
        const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
        unsigned char* pAddresses = AllocateMatchingAddresses();
        Expects(pAddresses != nullptr);

        memset(pAddresses, 0, nAddressesSize);
        for (gsl::index nIndex = nFirstIndex; nIndex <= nLastIndex; ++nIndex)
        {
            const auto nOffset = vAddresses.at(nIndex) - m_nFirstAddress;
            const auto nBit = 1 << (nOffset & 7);
            pAddresses[nOffset >> 3] |= nBit;
        }
    }
}

void MemBlock::CopyMatchingAddresses(const MemBlock& pSource)
{
    Expects(pSource.m_nMaxAddresses == m_nMaxAddresses);
    if (pSource.AreAllAddressesMatching())
    {
        m_nMatchingAddresses = m_nMaxAddresses;
    }
    else
    {
        const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
        unsigned char* pAddresses = AllocateMatchingAddresses();
        memcpy(pAddresses, pSource.GetMatchingAddressPointer(), nAddressesSize);
        m_nMatchingAddresses = pSource.m_nMatchingAddresses;
    }
}

void MemBlock::ExcludeMatchingAddress(ra::ByteAddress nAddress)
{
    const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
    unsigned char* pAddresses = nullptr;
    const auto nIndex = nAddress - m_nFirstAddress;
    const auto nBit = 1 << (nIndex & 7);

    if (AreAllAddressesMatching())
    {
        pAddresses = AllocateMatchingAddresses();
        Expects(pAddresses != nullptr);
        memset(pAddresses, 0xFF, nAddressesSize);
    }
    else
    {
        pAddresses = (nAddressesSize > sizeof(m_vAddresses)) ? m_pAddresses : &m_vAddresses[0];
        Expects(pAddresses != nullptr);
        if (!(pAddresses[nIndex >> 3] & nBit))
            return;
    }

    pAddresses[nIndex >> 3] &= ~nBit;
    --m_nMatchingAddresses;
}

bool MemBlock::ContainsAddress(ra::ByteAddress nAddress) const noexcept
{
    if (nAddress < m_nFirstAddress)
        return false;

    nAddress -= m_nFirstAddress;
    return (nAddress < m_nMaxAddresses);
}

bool MemBlock::ContainsMatchingAddress(ra::ByteAddress nAddress) const
{
    if (nAddress < m_nFirstAddress)
        return false;

    const auto nIndex = nAddress - m_nFirstAddress;
    if (nIndex >= m_nMaxAddresses)
        return false;

    if (AreAllAddressesMatching())
        return true;

    const uint8_t* pAddresses = GetMatchingAddressPointer();
    Expects(pAddresses != nullptr);
    const auto nBit = 1 << (nIndex & 7);
    return (pAddresses[nIndex >> 3] & nBit);
}

ra::ByteAddress MemBlock::GetMatchingAddress(gsl::index nIndex) const noexcept
{
    if (AreAllAddressesMatching())
        return m_nFirstAddress + gsl::narrow_cast<ra::ByteAddress>(nIndex);

    const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
    const uint8_t* pAddresses = (nAddressesSize > sizeof(m_vAddresses)) ? m_pAddresses : &m_vAddresses[0];
    ra::ByteAddress nAddress = m_nFirstAddress;
    const ra::ByteAddress nStop = m_nFirstAddress + m_nMaxAddresses;
    uint8_t nMask = 0x01;

    if (pAddresses != nullptr)
    {
        do
        {
            if (*pAddresses & nMask)
            {
                if (nIndex-- == 0)
                    return nAddress;
            }

            if (nMask == 0x80)
            {
                nMask = 0x01;
                pAddresses++;

                while (!*pAddresses)
                {
                    nAddress += 8;
                    if (nAddress >= nStop)
                        break;

                    pAddresses++;
                }
            }
            else
            {
                nMask <<= 1;
            }

            nAddress++;
        } while (nAddress < nStop);
    }

    return 0;
}


} // namespace impl

_CONSTANT_VAR MAX_BLOCK_SIZE = 256U * 1024; // 256K

static impl::SearchImpl* GetSearchImpl(SearchType nType) noexcept
{
    switch (nType)
    {
        case SearchType::FourBit:
            return &ra::services::impl::s_pFourBitSearchImpl;
        default:
        case SearchType::EightBit:
            return &ra::services::impl::s_pEightBitSearchImpl;
        case SearchType::SixteenBit:
            return &ra::services::impl::s_pSixteenBitSearchImpl;
        case SearchType::TwentyFourBit:
            return &ra::services::impl::s_pTwentyFourBitSearchImpl;
        case SearchType::ThirtyTwoBit:
            return &ra::services::impl::s_pThirtyTwoBitSearchImpl;
        case SearchType::SixteenBitAligned:
            return &ra::services::impl::s_pSixteenBitAlignedSearchImpl;
        case SearchType::ThirtyTwoBitAligned:
            return &ra::services::impl::s_pThirtyTwoBitAlignedSearchImpl;
        case SearchType::SixteenBitBigEndian:
            return &ra::services::impl::s_pSixteenBitBigEndianSearchImpl;
        case SearchType::ThirtyTwoBitBigEndian:
            return &ra::services::impl::s_pThirtyTwoBitBigEndianSearchImpl;
        case SearchType::SixteenBitBigEndianAligned:
            return &ra::services::impl::s_pSixteenBitBigEndianAlignedSearchImpl;
        case SearchType::ThirtyTwoBitBigEndianAligned:
            return &ra::services::impl::s_pThirtyTwoBitBigEndianAlignedSearchImpl;
        case SearchType::BitCount:
            return &ra::services::impl::s_pBitCountSearchImpl;
        case SearchType::AsciiText:
            return &ra::services::impl::s_pAsciiTextSearchImpl;
        case SearchType::Float:
            return &ra::services::impl::s_pFloatSearchImpl;
        case SearchType::FloatBigEndian:
            return &ra::services::impl::s_pFloatBESearchImpl;
        case SearchType::Double32:
            return &ra::services::impl::s_pDouble32SearchImpl;
        case SearchType::Double32BigEndian:
            return &ra::services::impl::s_pDouble32BESearchImpl;
        case SearchType::MBF32:
            return &ra::services::impl::s_pMBF32SearchImpl;
        case SearchType::MBF32LE:
            return &ra::services::impl::s_pMBF32LESearchImpl;
    }
}

void SearchResults::Initialize(ra::ByteAddress nAddress, size_t nBytes, SearchType nType)
{
    m_nType = nType;
    m_pImpl = GetSearchImpl(nType);

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    const auto nTotalMemorySize = pEmulatorContext.TotalMemorySize();
    if (nAddress > nTotalMemorySize)
        nAddress = 0;
    if (nBytes + nAddress > nTotalMemorySize)
        nBytes = nTotalMemorySize - nAddress;

    const unsigned int nPadding = m_pImpl->GetPadding();
    if (nPadding >= nBytes)
        nBytes = 0;
    else if (nBytes > nPadding)
        nBytes -= nPadding;

    while (nBytes > 0)
    {
        const auto nBlockSize = gsl::narrow_cast<unsigned int>((nBytes > MAX_BLOCK_SIZE) ? MAX_BLOCK_SIZE : nBytes);
        const auto nMaxAddresses = m_pImpl->GetAddressCountForBytes(nBlockSize + nPadding);
        const auto nBlockAddress = m_pImpl->ConvertFromRealAddress(nAddress);
        auto& block = m_vBlocks.emplace_back(nBlockAddress, nBlockSize + nPadding, nMaxAddresses);
        pEmulatorContext.ReadMemory(nAddress, block.GetBytes(), gsl::narrow_cast<size_t>(block.GetBytesSize()));

        nAddress += nBlockSize;
        nBytes -= nBlockSize;
    }
}

_Use_decl_annotations_
void SearchResults::Initialize(const std::vector<Result>& vResults, SearchType nType)
{
    m_nType = nType;
    m_pImpl = GetSearchImpl(nType);

    const auto bAligned = m_pImpl->ConvertToRealAddress(m_pImpl->ConvertFromRealAddress(7)) != 7;
    const auto nSize = ra::data::MemSizeBytes(m_pImpl->GetMemSize());
    bool bNeedsReversed = false;
    switch (nType)
    {
        case SearchType::SixteenBitBigEndian:
        case SearchType::SixteenBitBigEndianAligned:
        case SearchType::ThirtyTwoBitBigEndian:
        case SearchType::ThirtyTwoBitBigEndianAligned:
            bNeedsReversed = true;
            break;
    }

    auto nFirstAddress = vResults.front().nAddress;
    auto nLastAddressPlusOne = vResults.back().nAddress + nSize;
    if (bAligned)
    {
        nFirstAddress = m_pImpl->ConvertToRealAddress(m_pImpl->ConvertFromRealAddress(nFirstAddress));
        nLastAddressPlusOne = m_pImpl->ConvertToRealAddress(m_pImpl->ConvertFromRealAddress(nLastAddressPlusOne));
    }

    const auto nMemorySize = gsl::narrow_cast<size_t>(nLastAddressPlusOne) - nFirstAddress;
    std::vector<unsigned char> vMemory;
    vMemory.resize(nMemorySize);

    std::vector<ra::ByteAddress> vAddresses;
    vAddresses.reserve(vResults.size());

    for (const auto& pResult : vResults)
    {
        if (pResult.nAddress < nFirstAddress || pResult.nAddress + nSize > nLastAddressPlusOne)
            continue;

        const auto nVirtualAddress = m_pImpl->ConvertFromRealAddress(pResult.nAddress);

        // ignore unaligned values for aligned types
        if (bAligned && m_pImpl->ConvertToRealAddress(nVirtualAddress) != pResult.nAddress)
            continue;

        vAddresses.push_back(nVirtualAddress);

        auto nOffset = pResult.nAddress - nFirstAddress;        
        auto nValue = pResult.nValue;
        if (bNeedsReversed)
        {
            nValue = (((nValue & 0xFF000000) >> 24) |
                      ((nValue & 0x00FF0000) >> 8) |
                      ((nValue & 0x0000FF00) << 8) |
                      ((nValue & 0x000000FF) << 24));
        }

        switch (nSize)
        {
            case 4:
                vMemory.at(nOffset++) = nValue & 0xFF;
                nValue >>= 8;
                _FALLTHROUGH;
            case 3:
                vMemory.at(nOffset++) = nValue & 0xFF;
                nValue >>= 8;
                _FALLTHROUGH;
            case 2:
                vMemory.at(nOffset++) = nValue & 0xFF;
                nValue >>= 8;
                _FALLTHROUGH;
            default:
                vMemory.at(nOffset) = nValue & 0xFF;
                break;
        }
    }

    m_pImpl->AddBlocks(*this, vAddresses, vMemory, nFirstAddress, m_pImpl->GetPadding());
}

bool SearchResults::ContainsAddress(ra::ByteAddress nAddress) const
{
    if (m_pImpl)
        return m_pImpl->ContainsAddress(*this, nAddress);

    return false;
}

void SearchResults::MergeSearchResults(const SearchResults& srMemory, const SearchResults& srAddresses)
{
    m_vBlocks.reserve(srAddresses.m_vBlocks.size());
    m_nType = srAddresses.m_nType;
    m_pImpl = srAddresses.m_pImpl;
    m_nCompareType = srAddresses.m_nCompareType;
    m_nFilterType = srAddresses.m_nFilterType;
    m_nFilterValue = srAddresses.m_nFilterValue;

    for (const auto& pSrcBlock : srAddresses.m_vBlocks)
    {
        // copy the block from the srAddresses collection, then update the memory from the srMemory collection.
        // this creates a new block with the memory from the srMemory collection and the addresses from the
        // srAddresses collection.
        auto& pNewBlock = m_vBlocks.emplace_back(pSrcBlock);
        unsigned int nSize = pNewBlock.GetBytesSize();
        ra::ByteAddress nAddress = m_pImpl->ConvertToRealAddress(pNewBlock.GetFirstAddress());
        unsigned char* pWrite = pNewBlock.GetBytes();

        for (const auto& pMemBlock : srMemory.m_vBlocks)
        {
            const auto nBlockFirstAddress = m_pImpl->ConvertToRealAddress(pMemBlock.GetFirstAddress());
            if (nAddress >= nBlockFirstAddress && nAddress < nBlockFirstAddress + pMemBlock.GetBytesSize())
            {
                const auto nOffset = nAddress - nBlockFirstAddress;
                const auto nAvailable = pMemBlock.GetBytesSize() - nOffset;
                if (nAvailable >= nSize)
                {
                    memcpy(pWrite, pMemBlock.GetBytes() + nOffset, nSize);
                    break;
                }
                else
                {
                    memcpy(pWrite, pMemBlock.GetBytes() + nOffset, nAvailable);
                    nSize -= nAvailable;
                    pWrite += nAvailable;
                    nAddress += nAvailable;
                }
            }
        }
    }
}

_Use_decl_annotations_
bool SearchResults::Initialize(const SearchResults& srFirst, std::function<void(ra::ByteAddress,uint8_t*,size_t)> pReadMemory,
    ComparisonType nCompareType, SearchFilterType nFilterType, const std::wstring& sFilterValue)
{
    m_nType = srFirst.m_nType;
    m_pImpl = srFirst.m_pImpl;
    m_nCompareType = nCompareType;
    m_nFilterType = nFilterType;
    m_sFilterValue = sFilterValue;

    if (!m_pImpl->ValidateFilterValue(*this))
        return false;

    m_pImpl->ApplyFilter(*this, srFirst, pReadMemory);
    return true;
}

_Use_decl_annotations_
bool SearchResults::Initialize(const SearchResults& srSource, ComparisonType nCompareType,
    SearchFilterType nFilterType, const std::wstring& sFilterValue)
{
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    auto pReadMemory = [&pEmulatorContext](ra::ByteAddress nAddress, uint8_t* pBuffer, size_t nBufferSize) {
        pEmulatorContext.ReadMemory(nAddress, pBuffer, nBufferSize);
    };

    return Initialize(srSource, pReadMemory, nCompareType, nFilterType, sFilterValue);
}

_Use_decl_annotations_
bool SearchResults::Initialize(const SearchResults& srMemory, const SearchResults& srAddresses,
    ComparisonType nCompareType, SearchFilterType nFilterType, const std::wstring& sFilterValue)
{
    if (&srMemory == &srAddresses)
        return Initialize(srMemory, nCompareType, nFilterType, sFilterValue);

    // create a new merged SearchResults object that has the matching addresses from srAddresses,
    // and the memory blocks from srMemory.
    SearchResults srMerge;
    srMerge.MergeSearchResults(srMemory, srAddresses);

    // then do a standard comparison against the merged SearchResults
    return Initialize(srMerge, nCompareType, nFilterType, sFilterValue);
}

size_t SearchResults::MatchingAddressCount() const noexcept
{
    size_t nCount = 0;
    for (const auto& pBlock : m_vBlocks)
        nCount += pBlock.GetMatchingAddressCount();

    return nCount;
}

bool SearchResults::ExcludeResult(const SearchResults::Result& pResult)
{
    if (m_nFilterType != SearchFilterType::None && m_pImpl != nullptr)
        return m_pImpl->ExcludeResult(*this, pResult);

    return false;
}

bool SearchResults::GetMatchingAddress(gsl::index nIndex, _Out_ SearchResults::Result& result) const noexcept
{
    if (m_pImpl == nullptr)
    {
        memset(&result, 0, sizeof(SearchResults::Result));
        return false;
    }

    return m_pImpl->GetMatchingAddress(*this, nIndex, result);
}

bool SearchResults::GetMatchingAddress(const SearchResults::Result& pSrcResult, _Out_ SearchResults::Result& result) const noexcept
{
    memcpy(&result, &pSrcResult, sizeof(SearchResults::Result));

    if (m_pImpl == nullptr)
        return false;

    return m_pImpl->GetValueAtVirtualAddress(*this, result);
}

bool SearchResults::GetBytes(ra::ByteAddress nAddress, unsigned char* pBuffer, size_t nCount) const noexcept
{
    if (m_pImpl != nullptr)
    {
        for (auto& block : m_vBlocks)
        {
            const auto nFirstAddress = m_pImpl->ConvertToRealAddress(block.GetFirstAddress());
            if (nFirstAddress > nAddress)
                break;

            const int nRemaining = ra::to_signed(nFirstAddress + block.GetBytesSize() - nAddress);
            if (nRemaining < 0)
                continue;

            if (ra::to_unsigned(nRemaining) >= nCount)
            {
                memcpy(pBuffer, block.GetBytes() + (nAddress - nFirstAddress), nCount);
                return true;
            }

            memcpy(pBuffer, block.GetBytes() + (nAddress - nFirstAddress), nRemaining);
            nCount -= nRemaining;
            pBuffer += nRemaining;
            nAddress += nRemaining;
        }
    }

    memset(pBuffer, gsl::narrow_cast<int>(nCount), 0);
    return false;
}

std::wstring SearchResults::GetFormattedValue(ra::ByteAddress nAddress, MemSize nSize) const
{
    return m_pImpl ? m_pImpl->GetFormattedValue(*this, nAddress, nSize) : L"";
}

bool SearchResults::UpdateValue(SearchResults::Result& pResult,
    _Out_ std::wstring* sFormattedValue, const ra::data::context::EmulatorContext& pEmulatorContext) const
{
    if (m_pImpl)
        return m_pImpl->UpdateValue(*this, pResult, sFormattedValue, pEmulatorContext);

    if (sFormattedValue)
        sFormattedValue->clear();

    return false;
}

bool SearchResults::MatchesFilter(const SearchResults& pPreviousResults, SearchResults::Result& pResult) const
{
    if (m_pImpl)
        return m_pImpl->MatchesFilter(*this, pPreviousResults, pResult);

    return true;
}

MemSize SearchResults::GetSize() const noexcept
{
    return m_pImpl ? m_pImpl->GetMemSize() : MemSize::EightBit;
}

} // namespace services
} // namespace ra
