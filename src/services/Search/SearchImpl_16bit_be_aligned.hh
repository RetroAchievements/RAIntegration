#ifndef SEARCHIMPL_16BIT_BE_ALIGNED_H
#define SEARCHIMPL_16BIT_BE_ALIGNED_H
#pragma once

#include "SearchImpl_16bit_aligned.hh"

namespace ra {
namespace services {
namespace search {

class SixteenBitBigEndianAlignedSearchImpl : public SixteenBitAlignedSearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::SixteenBitBigEndian; }

protected:
    unsigned int BuildValue(const uint8_t* ptr) const noexcept override
    {
        if (!ptr)
            return 0;

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

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_16BIT_BE_ALIGNED_H
