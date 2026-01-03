#ifndef SEARCHIMPL_16BIT_H
#define SEARCHIMPL_16BIT_H
#pragma once

#include "SearchImpl.hh"

namespace ra {
namespace services {
namespace search {

class SixteenBitSearchImpl : public SearchImpl
{
public:
    ra::data::Memory::Size GetMemSize() const noexcept override { return ra::data::Memory::Size::SixteenBit; }

    unsigned int GetPadding() const noexcept override { return 1U; }

    unsigned int BuildValue(const uint8_t* ptr) const noexcept override
    {
        if (!ptr)
            return 0;

        GSL_SUPPRESS_TYPE1 return *reinterpret_cast<const uint16_t*>(ptr);
    }

protected:
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::data::ByteAddress>& vMatches) const override
    {
        ApplyCompareFilterLittleEndian<uint16_t, true>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nConstantValue, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::data::ByteAddress>& vMatches) const override
    {
        ApplyCompareFilterLittleEndian<uint16_t, false>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_16BIT_H
