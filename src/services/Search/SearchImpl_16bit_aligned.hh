#ifndef SEARCHIMPL_16BIT_ALIGNED_H
#define SEARCHIMPL_16BIT_ALIGNED_H
#pragma once

#include "SearchImpl_16bit.hh"

namespace ra {
namespace services {
namespace search {

class SixteenBitAlignedSearchImpl : public SixteenBitSearchImpl
{
public:
    unsigned int GetStride() const noexcept override { return 2; }

    unsigned int GetAddressCountForBytes(unsigned int nBytes) const noexcept override
    {
        return nBytes / 2;
    }

    ra::data::ByteAddress ConvertFromRealAddress(ra::data::ByteAddress nAddress) const noexcept override
    {
        return nAddress / 2;
    }

    ra::data::ByteAddress ConvertToRealAddress(ra::data::ByteAddress nAddress) const noexcept override
    {
        return nAddress * 2;
    }

protected:
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const CapturedMemoryBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::data::ByteAddress>& vMatches) const override
    {
        ApplyCompareFilterLittleEndian<uint16_t, true, 2>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nConstantValue, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const CapturedMemoryBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::data::ByteAddress>& vMatches) const override
    {
        ApplyCompareFilterLittleEndian<uint16_t, false, 2>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }


    bool GetValueFromCapturedMemoryBlock(const CapturedMemoryBlock& block, SearchResult& result) const noexcept override
    {
        const unsigned int nOffset = (result.nAddress - block.GetFirstAddress()) * 2;
        if (nOffset + 1 >= block.GetBytesSize())
            return false;

        result.nValue = BuildValue(block.GetBytes() + nOffset);
        result.nAddress *= 2;

        return true;
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_16BIT_ALIGNED_H
