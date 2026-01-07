#ifndef SEARCHIMPL_4BIT_H
#define SEARCHIMPL_4BIT_H
#pragma once

#include "SearchImpl.hh"

namespace ra {
namespace services {
namespace search {

class FourBitSearchImpl : public SearchImpl
{
    ra::data::Memory::Size GetMemSize() const noexcept override { return ra::data::Memory::Size::NibbleLower; }

    unsigned int GetAddressCountForBytes(unsigned int nBytes) const noexcept override
    {
        return nBytes * 2;
    }

    ra::data::ByteAddress ConvertFromRealAddress(ra::data::ByteAddress nAddress) const noexcept override
    {
        return nAddress * 2;
    }

    ra::data::ByteAddress ConvertToRealAddress(ra::data::ByteAddress nAddress) const noexcept override
    {
        return nAddress / 2;
    }

    bool ContainsAddress(const SearchResults& srResults, ra::data::ByteAddress nAddress) const override
    {
        nAddress <<= 1;

        for (const auto& pBlock : GetBlocks(srResults))
        {
            if (pBlock.ContainsAddress(nAddress))
                return pBlock.ContainsMatchingAddress(nAddress) || pBlock.ContainsMatchingAddress(nAddress | 1);
        }

        return false;
    }

    bool ExcludeResult(SearchResults& srResults, const SearchResult& pResult) const override
    {
        auto nAddress = pResult.nAddress << 1;
        if (pResult.nSize == ra::data::Memory::Size::NibbleUpper)
            nAddress |= 1;

        return ExcludeAddress(srResults, nAddress);
    }

    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const CapturedMemoryBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::data::ByteAddress>& vMatches) const override
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
        const CapturedMemoryBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::data::ByteAddress>& vMatches) const override
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
    bool GetValueFromCapturedMemoryBlock(const CapturedMemoryBlock& block, SearchResult& result) const noexcept override
    {
        if (result.nAddress & 1)
            result.nSize = ra::data::Memory::Size::NibbleUpper;

        result.nAddress >>= 1;

        const unsigned int nOffset = result.nAddress - (block.GetFirstAddress() >> 1);
        if (nOffset >= block.GetBytesSize())
            return false;

        result.nValue = BuildValue(block.GetBytes() + nOffset);

        if (result.nSize == ra::data::Memory::Size::NibbleLower)
            result.nValue &= 0x0F;
        else
            result.nValue = (result.nValue >> 4) & 0x0F;

        return true;
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_4BIT_H
