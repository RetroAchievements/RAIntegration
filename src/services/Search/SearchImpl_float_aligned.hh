#ifndef SEARCHIMPL_FLOAT_ALIGNED_H
#define SEARCHIMPL_FLOAT_ALIGNED_H
#pragma once

#include "SearchImpl_float.hh"

namespace ra {
namespace services {
namespace search {

class FloatAlignedSearchImpl : public FloatSearchImpl
{
public:
    unsigned int GetStride() const noexcept override { return 4; }

    unsigned int GetAddressCountForBytes(unsigned int nBytes) const noexcept override
    {
        return nBytes / 4;
    }

    ra::data::ByteAddress ConvertFromRealAddress(ra::data::ByteAddress nAddress) const noexcept override
    {
        return nAddress / 4;
    }

    ra::data::ByteAddress ConvertToRealAddress(ra::data::ByteAddress nAddress) const noexcept override
    {
        return nAddress * 4;
    }

protected:
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::data::ByteAddress>& vMatches) const override
    {
        if (nComparison == ComparisonType::Equals || nComparison == ComparisonType::NotEqualTo)
        {
            // for direct equality, we can just compare the raw bytes without converting
            ApplyCompareFilterLittleEndian<uint32_t, true, 4>(pBytes, pBytesStop,
                pPreviousBlock, nComparison, nConstantValue, vMatches);
        }
        else
        {
            FloatSearchImpl::ApplyConstantFilter(pBytes, pBytesStop,
                pPreviousBlock, nComparison, nConstantValue, vMatches);
        }
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::data::ByteAddress>& vMatches) const override
    {
        if (nComparison == ComparisonType::Equals || nComparison == ComparisonType::NotEqualTo)
        {
            // for direct equality, we can just compare the raw bytes without converting
            ApplyCompareFilterLittleEndian<uint32_t, false, 4>(pBytes, pBytesStop,
                pPreviousBlock, nComparison, nAdjustment, vMatches);
        }
        else
        {
            FloatSearchImpl::ApplyCompareFilter(pBytes, pBytesStop,
                pPreviousBlock, nComparison, nAdjustment, vMatches);
        }
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

#endif // SEARCHIMPL_FLOAT_ALIGNED_H
