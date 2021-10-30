#include "SearchResults.h"

#include "RA_StringUtils.h"

#include "ra_utility.h"

#include "data\context\EmulatorContext.hh"

#include "services\ServiceLocator.hh"

#include <algorithm>

// define this to use the generic filtering code for all search types
// if defined, specialized templated code will be used for little endian searches
#undef DISABLE_TEMPLATED_SEARCH

namespace ra {
namespace services {

namespace impl {

_NODISCARD static constexpr bool CompareValues(_In_ unsigned int nLeft, _In_ unsigned int nRight, _In_ ComparisonType nCompareType) noexcept
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

    // populates a vector of addresses that match the specified filter when applied to a previous search result
    virtual void ApplyFilter(SearchResults& srNew, const SearchResults& srPrevious) const
    {
        unsigned int nLargestBlock = 0U;
        for (auto& block : srPrevious.m_vBlocks)
        {
            if (block.GetBytesSize() > nLargestBlock)
                nLargestBlock = block.GetBytesSize();
        }

        std::vector<unsigned char> vMemory(nLargestBlock);
        std::vector<ra::ByteAddress> vMatches;
        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();

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
            pEmulatorContext.ReadMemory(ConvertToRealAddress(block.GetFirstAddress()), vMemory.data(), block.GetBytesSize());

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
    bool GetMatchingAddress(const SearchResults& srResults, gsl::index nIndex, _Out_ SearchResults::Result& result) const
    {
        result.nSize = GetMemSize();

        for (const auto& pBlock : srResults.m_vBlocks)
        {
            if (nIndex < pBlock.GetMatchingAddressCount())
            {
                result.nAddress = pBlock.GetMatchingAddress(nIndex);
                return GetValue(pBlock, result);
            }

            nIndex -= pBlock.GetMatchingAddressCount();
        }

        return false;
    }

    // gets the value associated with the address and size in the search result structure
    bool GetValue(const SearchResults& srResults, SearchResults::Result& result) const noexcept
    {
        for (const auto& block : srResults.m_vBlocks)
        {
            if (GetValue(block, result))
                return true;
        }

        result.nValue = 0;
        return false;
    }

    virtual std::wstring GetFormattedValue(const SearchResults&, const SearchResults::Result& pResult) const
    {
        return L"0x" + ra::data::MemSizeFormat(pResult.nValue, pResult.nSize, MemFormat::Hex);
    }

    virtual std::wstring GetFormattedValue(const SearchResults& pResults, ra::ByteAddress nAddress, MemSize nSize) const
    {
        SearchResults::Result pResult{ ConvertFromRealAddress(nAddress), 0, nSize };
        if (GetValue(pResults, pResult))
            return GetFormattedValue(pResults, pResult);

        return L"";
    }

    virtual void UpdateValue(const SearchResults& pResults, SearchResults::Result& pResult,
        _Out_ std::wstring* sFormattedValue, const ra::data::context::EmulatorContext& pEmulatorContext) const
    {
        pResult.nValue = pEmulatorContext.ReadMemory(pResult.nAddress, pResult.nSize);

        if (sFormattedValue)
            *sFormattedValue = GetFormattedValue(pResults, pResult);
    }

protected:
    // generic implementation for less used search types
    virtual void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::ByteAddress>& vMatches) const noexcept
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
        std::vector<ra::ByteAddress>& vMatches) const noexcept
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
        std::vector<ra::ByteAddress>& vMatches) const noexcept
    {
        ra::ByteAddress nAddress = pPreviousBlock.GetFirstAddress();
        constexpr int TBlockStride = TIsConstantFilter ? 0 : TStride;
        const auto* pBlockBytes = pPreviousBlock.GetBytes();

        const auto* pMatchingAddresses = pPreviousBlock.GetMatchingAddressPointer();
        if (!pMatchingAddresses)
        {
            for (const auto* pScan = pBytes; pScan < pBytesStop; pScan += TStride, pBlockBytes += TBlockStride)
            {
                const unsigned int nValue1 = *(TSize*)pScan;
                const unsigned int nValue2 = TIsConstantFilter ? nAdjustment : *(TSize*)pBlockBytes + nAdjustment;
                if (CompareValues(nValue1, nValue2, TComparison))
                    vMatches.push_back(nAddress);

                ++nAddress;
            }
        }
        else
        {
            uint8_t nMask = 0x01;
            for (const auto* pScan = pBytes; pScan < pBytesStop; pScan += TStride, pBlockBytes += TBlockStride)
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
                    const unsigned int nValue1 = *(TSize*)pScan;
                    const unsigned int nValue2 = TIsConstantFilter ? nAdjustment : *(TSize*)pBlockBytes + nAdjustment;
                    if (CompareValues(nValue1, nValue2, TComparison))
                        vMatches.push_back(nAddress);
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
        std::vector<ra::ByteAddress>& vMatches) const noexcept
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
        std::vector<ra::ByteAddress>& vMatches) const noexcept
    {
        if (TIsConstantFilter)
            SearchImpl::ApplyConstantFilter(pBytes, pBytesStop, pPreviousBlock, nComparison, nAdjustment, vMatches);
        else
            SearchImpl::ApplyCompareFilter(pBytes, pBytesStop, pPreviousBlock, nComparison, nAdjustment, vMatches);
    }
#endif

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

    // Gets a value from a memory block using a virtual address (in result.nAddress)
    virtual bool GetValue(const impl::MemBlock& block, SearchResults::Result& result) const noexcept
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

    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::ByteAddress>& vMatches) const noexcept override
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
        std::vector<ra::ByteAddress>& vMatches) const noexcept override
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
    bool GetValue(const impl::MemBlock& block, SearchResults::Result& result) const noexcept override
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
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const noexcept override
    {
        ApplyCompareFilterLittleEndian<uint8_t, true>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const noexcept override
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
        return (ptr[1] << 8) | ptr[0];
    }

protected:
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const noexcept override
    {
        ApplyCompareFilterLittleEndian<uint16_t, true>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const noexcept override
    {
        ApplyCompareFilterLittleEndian<uint16_t, false>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
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
        return (ptr[3] << 24) | (ptr[2] << 16) | (ptr[1] << 8) | ptr[0];
    }

protected:
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const noexcept override
    {
        ApplyCompareFilterLittleEndian<uint32_t, true>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const noexcept override
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
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const noexcept override
    {
        ApplyCompareFilterLittleEndian<uint32_t, true, 4>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const noexcept override
    {
        ApplyCompareFilterLittleEndian<uint32_t, false, 4>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }


    bool GetValue(const impl::MemBlock& block, SearchResults::Result& result) const noexcept override
    {
        result.nAddress *= 4;

        const unsigned int nOffset = result.nAddress - (block.GetFirstAddress() * 4);
        result.nValue = BuildValue(block.GetBytes() + nOffset);

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
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const noexcept override
    {
        ApplyCompareFilterLittleEndian<uint16_t, true, 2>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const noexcept override
    {
        ApplyCompareFilterLittleEndian<uint16_t, false, 2>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }


    bool GetValue(const impl::MemBlock& block, SearchResults::Result& result) const noexcept override
    {
        result.nAddress *= 2;

        const unsigned int nOffset = result.nAddress - (block.GetFirstAddress() * 2);
        result.nValue = BuildValue(block.GetBytes() + nOffset);

        return true;
    }
};

class SixteenBitBigEndianSearchImpl : public SixteenBitSearchImpl
{
    MemSize GetMemSize() const noexcept override { return MemSize::SixteenBitBigEndian; }

    unsigned int BuildValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        return (ptr[0] << 8) | ptr[1];
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
};

class AsciiTextSearchImpl : public SearchImpl
{
public:
    // indicate that search results can be very wide
    MemSize GetMemSize() const noexcept override { return MemSize::Text; }

    // populates a vector of addresses that match the specified filter when applied to a previous search result
    void ApplyFilter(SearchResults& srNew, const SearchResults& srPrevious) const override
    {
        size_t nCompareLength = 16; // maximum length for generic search
        std::vector<unsigned char> vSearchText;
        GetSearchText(vSearchText, srNew.GetFilterString());

        if (vSearchText.empty())
        {
            // can't match 0 characters!
            if (srNew.GetFilterType() == SearchFilterType::Constant)
                return;
        }
        else
        {
            nCompareLength = vSearchText.size();
        }

        unsigned int nLargestBlock = 0U;
        for (auto& block : GetBlocks(srPrevious))
        {
            if (block.GetBytesSize() > nLargestBlock)
                nLargestBlock = block.GetBytesSize();
        }

        std::vector<unsigned char> vMemory(nLargestBlock);
        std::vector<ra::ByteAddress> vMatches;
        const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();

        for (auto& block : GetBlocks(srPrevious))
        {
            pEmulatorContext.ReadMemory(block.GetFirstAddress(), vMemory.data(), block.GetBytesSize());

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
        std::array<unsigned char, 16> pBuffer;
        pResults.GetBytes(nAddress, &pBuffer.at(0), pBuffer.size());

        std::wstring sText;
        GetASCIIText(sText, pBuffer.data(), pBuffer.size());

        return sText;
    }

    void UpdateValue(const SearchResults&, SearchResults::Result& pResult,
        _Out_ std::wstring* sFormattedValue, const ra::data::context::EmulatorContext& pEmulatorContext) const override
    {
        std::array<unsigned char, 16> pBuffer;
        pEmulatorContext.ReadMemory(pResult.nAddress, &pBuffer.at(0), pBuffer.size());

        std::wstring sText;
        GetASCIIText(sText, pBuffer.data(), pBuffer.size());

        pResult.nValue = ra::StringHash(sText);

        if (sFormattedValue)
            sFormattedValue->swap(sText);
    }
};

static FourBitSearchImpl s_pFourBitSearchImpl;
static EightBitSearchImpl s_pEightBitSearchImpl;
static SixteenBitSearchImpl s_pSixteenBitSearchImpl;
static ThirtyTwoBitSearchImpl s_pThirtyTwoBitSearchImpl;
static SixteenBitAlignedSearchImpl s_pSixteenBitAlignedSearchImpl;
static ThirtyTwoBitAlignedSearchImpl s_pThirtyTwoBitAlignedSearchImpl;
static SixteenBitBigEndianSearchImpl s_pSixteenBitBigEndianSearchImpl;
static ThirtyTwoBitBigEndianSearchImpl s_pThirtyTwoBitBigEndianSearchImpl;
static AsciiTextSearchImpl s_pAsciiTextSearchImpl;

uint8_t* MemBlock::AllocateMatchingAddresses()
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
    unsigned char* pAddresses = NULL;
    const auto nIndex = nAddress - m_nFirstAddress;
    const auto nBit = 1 << (nIndex & 7);

    if (AreAllAddressesMatching())
    {
        pAddresses = AllocateMatchingAddresses();
        memset(pAddresses, 0xFF, nAddressesSize);
    }
    else
    {
        pAddresses = (nAddressesSize > sizeof(m_vAddresses)) ? m_pAddresses : &m_vAddresses[0];
        if (!(pAddresses[nIndex >> 3] & nBit))
            return;
    }

    pAddresses[nIndex >> 3] &= ~nBit;
    --m_nMatchingAddresses;
}

bool MemBlock::ContainsAddress(ra::ByteAddress nAddress) const
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
    const auto nBit = 1 << (nIndex & 7);
    return (pAddresses[nIndex >> 3] & nBit);
}

ra::ByteAddress MemBlock::GetMatchingAddress(gsl::index nIndex) const
{
    if (AreAllAddressesMatching())
        return m_nFirstAddress + gsl::narrow_cast<ra::ByteAddress>(nIndex);

    const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
    const uint8_t* pAddresses = (nAddressesSize > sizeof(m_vAddresses)) ? m_pAddresses : &m_vAddresses[0];
    ra::ByteAddress nAddress = m_nFirstAddress;
    ra::ByteAddress nStop = m_nFirstAddress + m_nMaxAddresses;
    uint8_t nMask = 0x01;

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
        }
        else
        {
            nMask <<= 1;
        }

        nAddress++;
    } while (nAddress < nStop);

    return 0;
}


} // namespace impl

_CONSTANT_VAR MAX_BLOCK_SIZE = 256U * 1024; // 256K

void SearchResults::Initialize(ra::ByteAddress nAddress, size_t nBytes, SearchType nType)
{
    m_nType = nType;

    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    if (nBytes + nAddress > pEmulatorContext.TotalMemorySize())
        nBytes = pEmulatorContext.TotalMemorySize() - nAddress;

    switch (nType)
    {
        case SearchType::FourBit:
            m_pImpl = &ra::services::impl::s_pFourBitSearchImpl;
            break;
        default:
        case SearchType::EightBit:
            m_pImpl = &ra::services::impl::s_pEightBitSearchImpl;
            break;
        case SearchType::SixteenBit:
            m_pImpl = &ra::services::impl::s_pSixteenBitSearchImpl;
            break;
        case SearchType::ThirtyTwoBit:
            m_pImpl = &ra::services::impl::s_pThirtyTwoBitSearchImpl;
            break;
        case SearchType::SixteenBitAligned:
            m_pImpl = &ra::services::impl::s_pSixteenBitAlignedSearchImpl;
            break;
        case SearchType::ThirtyTwoBitAligned:
            m_pImpl = &ra::services::impl::s_pThirtyTwoBitAlignedSearchImpl;
            break;
        case SearchType::SixteenBitBigEndian:
            m_pImpl = &ra::services::impl::s_pSixteenBitBigEndianSearchImpl;
            break;
        case SearchType::ThirtyTwoBitBigEndian:
            m_pImpl = &ra::services::impl::s_pThirtyTwoBitBigEndianSearchImpl;
            break;
        case SearchType::AsciiText:
            m_pImpl = &ra::services::impl::s_pAsciiTextSearchImpl;
            break;
    }

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

bool SearchResults::Result::Compare(unsigned int nPreviousValue, ComparisonType nCompareType) const noexcept
{
    return impl::CompareValues(nValue, nPreviousValue, nCompareType);
}

bool SearchResults::ContainsAddress(ra::ByteAddress nAddress) const
{
    if (m_pImpl)
        return m_pImpl->ContainsAddress(*this, nAddress);

    return false;
}

_Use_decl_annotations_
void SearchResults::Initialize(const SearchResults& srSource, ComparisonType nCompareType,
    SearchFilterType nFilterType, unsigned int nFilterValue)
{
    m_nType = srSource.m_nType;
    m_pImpl = srSource.m_pImpl;
    m_nCompareType = nCompareType;
    m_nFilterType = nFilterType;
    m_nFilterValue = nFilterValue;

    m_pImpl->ApplyFilter(*this, srSource);
}

_Use_decl_annotations_
void SearchResults::Initialize(const SearchResults& srMemory, const SearchResults& srAddresses,
    ComparisonType nCompareType, SearchFilterType nFilterType, unsigned int nFilterValue)
{
    if (&srMemory == &srAddresses)
    {
        Initialize(srMemory, nCompareType, nFilterType, nFilterValue);
        return;
    }

    // create a new merged SearchResults object that has the matching addresses from srAddresses,
    // and the memory blocks from srMemory.
    SearchResults srMerge;
    srMerge.MergeSearchResults(srMemory, srAddresses);

    // then do a standard comparison against the merged SearchResults
    Initialize(srMerge, nCompareType, nFilterType, nFilterValue);
}

void SearchResults::MergeSearchResults(const SearchResults& srMemory, const SearchResults& srAddresses)
{
    m_vBlocks.reserve(srAddresses.m_vBlocks.size());
    m_nType = srAddresses.m_nType;
    m_pImpl = srAddresses.m_pImpl;
    m_nCompareType = srAddresses.m_nCompareType;
    m_nFilterType = srAddresses.m_nFilterType;
    m_nFilterValue = srAddresses.m_nFilterValue;

    for (const auto pSrcBlock : srAddresses.m_vBlocks)
    {
        unsigned int nSize = pSrcBlock.GetBytesSize();
        ra::ByteAddress nAddress = pSrcBlock.GetFirstAddress();
        const auto nMaxAddresses = pSrcBlock.GetMaxAddresses();
        auto& pNewBlock = m_vBlocks.emplace_back(nAddress, nSize, nMaxAddresses);
        unsigned char* pWrite = pNewBlock.GetBytes();

        for (const auto pMemBlock : srMemory.m_vBlocks)
        {
            if (nAddress >= pMemBlock.GetFirstAddress() && nAddress < pMemBlock.GetFirstAddress() + pMemBlock.GetBytesSize())
            {
                const auto nOffset = nAddress - pMemBlock.GetFirstAddress();
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
void SearchResults::Initialize(const SearchResults& srSource, ComparisonType nCompareType,
    SearchFilterType nFilterType, const std::wstring& sFilterValue)
{
    m_nType = srSource.m_nType;
    m_pImpl = srSource.m_pImpl;
    m_nCompareType = nCompareType;
    m_nFilterType = nFilterType;
    m_sFilterValue = sFilterValue;

    m_pImpl->ApplyFilter(*this, srSource);
}

_Use_decl_annotations_
void SearchResults::Initialize(const SearchResults& srMemory, const SearchResults& srAddresses,
    ComparisonType nCompareType, SearchFilterType nFilterType, const std::wstring& sFilterValue)
{
    if (&srMemory == &srAddresses)
    {
        Initialize(srMemory, nCompareType, nFilterType, sFilterValue);
        return;
    }

    // create a new merged SearchResults object that has the matching addresses from srAddresses,
    // and the memory blocks from srMemory.
    SearchResults srMerge;
    srMerge.MergeSearchResults(srMemory, srAddresses);

    // then do a standard comparison against the merged SearchResults
    Initialize(srMerge, nCompareType, nFilterType, sFilterValue);
}

size_t SearchResults::MatchingAddressCount() const noexcept
{
    size_t nCount = 0;
    for (const auto& pBlock : m_vBlocks)
        nCount += pBlock.GetMatchingAddressCount();

    return nCount;
}

void SearchResults::ExcludeAddress(ra::ByteAddress nAddress) noexcept
{
    if (m_nFilterType == SearchFilterType::None)
        return;

    for (auto& pBlock : m_vBlocks)
    {
        if (pBlock.ContainsAddress(nAddress))
        {
            pBlock.ExcludeMatchingAddress(nAddress);
            break;
        }
    }
}

bool SearchResults::GetMatchingAddress(gsl::index nIndex, _Out_ SearchResults::Result& result) const
{
    if (m_pImpl == nullptr)
    {
        memset(&result, 0, sizeof(SearchResults::Result));
        return false;
    }

    return m_pImpl->GetMatchingAddress(*this, nIndex, result);
}

bool SearchResults::GetBytes(ra::ByteAddress nAddress, unsigned char* pBuffer, size_t nCount) const noexcept
{
    if (m_pImpl != nullptr)
    {
        const unsigned int nPadding = m_pImpl->GetPadding();
        for (auto& block : m_vBlocks)
        {
            if (block.GetFirstAddress() > nAddress)
                break;

            const int nRemaining = ra::to_signed(block.GetFirstAddress() + block.GetBytesSize() - nAddress);
            if (nRemaining < 0)
                continue;

            if (ra::to_unsigned(nRemaining) >= nCount)
            {
                memcpy(pBuffer, block.GetBytes() + (nAddress - block.GetFirstAddress()), nCount);
                return true;
            }

            memcpy(pBuffer, block.GetBytes() + (nAddress - block.GetFirstAddress()), nRemaining);
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

void SearchResults::UpdateValue(SearchResults::Result& pResult, _Out_ std::wstring* sFormattedValue,
    const ra::data::context::EmulatorContext& pEmulatorContext) const
{
    if (m_pImpl)
        return m_pImpl->UpdateValue(*this, pResult, sFormattedValue, pEmulatorContext);

    if (sFormattedValue)
        sFormattedValue->clear();
}

MemSize SearchResults::GetSize() const noexcept
{
    return m_pImpl ? m_pImpl->GetMemSize() : MemSize::EightBit;
}

} // namespace services
} // namespace ra
