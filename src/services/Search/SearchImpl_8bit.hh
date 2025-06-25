#ifndef SEARCHIMPL_8BIT_H
#define SEARCHIMPL_8BIT_H
#pragma once

#include "SearchImpl.hh"

namespace ra {
namespace services {
namespace search {

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

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_8BIT_H
