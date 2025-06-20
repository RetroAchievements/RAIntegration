#ifndef SEARCHIMPL_32BIT_ALIGNED_H
#define SEARCHIMPL_32BIT_ALIGNED_H
#pragma once

#include "SearchImpl_32bit.hh"

namespace ra {
namespace services {
namespace search {

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


    bool GetValueFromMemBlock(const MemBlock& block, SearchResult& result) const noexcept override
    {
        const unsigned int nOffset = (result.nAddress - block.GetFirstAddress()) * 4;
        if (nOffset + 3 >= block.GetBytesSize())
            return false;

        result.nValue = BuildValue(block.GetBytes() + nOffset);
        result.nAddress *= 4;

        return true;
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_32BIT_ALIGNED_H
