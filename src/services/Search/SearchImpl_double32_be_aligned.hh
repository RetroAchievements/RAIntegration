#ifndef SEARCHIMPL_DOUBLE32_BE_ALIGNED_H
#define SEARCHIMPL_DOUBLE32_BE_ALIGNED_H
#pragma once

#include "SearchImpl_double32_be.hh"

namespace ra {
namespace services {
namespace search {

class Double32BEAlignedSearchImpl : public Double32BESearchImpl
{
public:
    unsigned int GetStride() const noexcept override { return 8U; }

    unsigned int GetPadding() const noexcept override { return 7U; }

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
        if (nComparison == ComparisonType::Equals || nComparison == ComparisonType::NotEqualTo)
        {
            // for direct equality, we can just compare the raw bytes without converting
            ApplyCompareFilterLittleEndian<uint32_t, true, 8>(pBytes, pBytesStop,
                pPreviousBlock, nComparison, nConstantValue, vMatches);
        }
        else
        {
            Double32BESearchImpl::ApplyConstantFilter(pBytes, pBytesStop,
                pPreviousBlock, nComparison, nConstantValue, vMatches);
        }
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::ByteAddress>& vMatches) const override
    {
        if (nComparison == ComparisonType::Equals || nComparison == ComparisonType::NotEqualTo)
        {
            // for direct equality, we can just compare the raw bytes without converting
            ApplyCompareFilterLittleEndian<uint32_t, false, 8>(pBytes, pBytesStop,
                pPreviousBlock, nComparison, nAdjustment, vMatches);
        }
        else
        {
            Double32BESearchImpl::ApplyCompareFilter(pBytes, pBytesStop,
                pPreviousBlock, nComparison, nAdjustment, vMatches);
        }
    }

    bool GetValueFromMemBlock(const MemBlock& block, SearchResult& result) const noexcept override
    {
        const unsigned int nOffset = (result.nAddress - block.GetFirstAddress()) * 8;
        if (nOffset + 3 >= block.GetBytesSize())
            return false;

        result.nValue = BuildValue(block.GetBytes() + nOffset);
        result.nAddress *= 8;

        return true;
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_DOUBLE32_BE_ALIGNED_H
