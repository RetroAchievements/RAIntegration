#ifndef SEARCHIMPL_DOUBLE32_ALIGNED_H
#define SEARCHIMPL_DOUBLE32_ALIGNED_H
#pragma once

#include "SearchImpl_double32.hh"

namespace ra {
namespace services {
namespace search {

class Double32AlignedSearchImpl : public Double32SearchImpl
{
public:
    unsigned int GetStride() const noexcept override { return 8U; }

    unsigned int GetPadding() const noexcept override { return 7U; }

    bool ContainsAddress(const SearchResults& srResults, ra::ByteAddress nAddress) const override
    {
        if ((nAddress & 7) != 4)
            return false;

        nAddress = ConvertFromRealAddress(nAddress);
        for (const auto& pBlock : GetBlocks(srResults))
        {
            if (pBlock.ContainsAddress(nAddress))
                return pBlock.ContainsMatchingAddress(nAddress);
        }

        return false;
    }

    unsigned int GetAddressCountForBytes(unsigned int nBytes) const noexcept override
    {
        return nBytes / 8;
    }

    ra::ByteAddress ConvertFromRealAddress(ra::ByteAddress nAddress) const noexcept override
    {
        return nAddress / 8;
    }

    ra::ByteAddress ConvertToRealAddress(ra::ByteAddress nAddress) const noexcept override
    {
        return nAddress * 8;
    }

protected:
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        pBytes += 4; // we're only looking at the four most significant bytes

        if (nComparison == ComparisonType::Equals || nComparison == ComparisonType::NotEqualTo)
        {
            // for direct equality, we can just compare the raw bytes without converting
            ApplyCompareFilterLittleEndian<uint32_t, true, 8>(pBytes, pBytesStop,
                pPreviousBlock, nComparison, nConstantValue, vMatches);
        }
        else
        {
            // can still use base implementation because we don't have to offset the constant value
            Double32SearchImpl::ApplyConstantFilter(pBytes, pBytesStop,
                pPreviousBlock, nComparison, nConstantValue, vMatches);
        }
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        // cannot use base implementation because we need to offset pBytes and pBlockBytes
        const auto* pBlockBytes = pPreviousBlock.GetBytes() + 4;
        const auto nBlockAddress = pPreviousBlock.GetFirstAddress();
        const auto nStride = 8;
        const auto* pMatchingAddresses = pPreviousBlock.GetMatchingAddressPointer();

        const auto* pAdjustment = reinterpret_cast<const uint8_t*>(&nAdjustment);
        const float fAdjustment = BuildFloatValue(pAdjustment);

        for (const auto* pScan = pBytes + 4; pScan < pBytesStop; pScan += nStride, pBlockBytes += nStride)
        {
            const float fValue1 = BuildFloatValue(pScan);
            const float fValue2 = BuildFloatValue(pBlockBytes) + fAdjustment;
            if (CompareValues(fValue1, fValue2, nComparison))
            {
                const ra::ByteAddress nAddress = nBlockAddress +
                    ConvertFromRealAddress(gsl::narrow_cast<uint32_t>(pScan - pBytes - 4));
                if (pPreviousBlock.HasMatchingAddress(pMatchingAddresses, nAddress))
                    vMatches.push_back(nAddress);
            }
        }
    }

    bool GetValueFromMemBlock(const MemBlock& block, SearchResult& result) const noexcept override
    {
        const unsigned int nOffset = (result.nAddress - block.GetFirstAddress()) * 8;
        if (nOffset + 7 >= block.GetBytesSize())
            return false;

        // the actual value being returned is 4-bytes into the 8-byte double
        result.nValue = BuildValue(block.GetBytes() + nOffset + 4);
        result.nAddress = result.nAddress * 8 + 4;

        return true;
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_DOUBLE32_ALIGNED_H
