#ifndef SEARCHIMPL_32BIT_H
#define SEARCHIMPL_32BIT_H
#pragma once

#include "SearchImpl.hh"

namespace ra {
namespace services {
namespace search {

class ThirtyTwoBitSearchImpl : public SearchImpl
{
public:
    ra::data::Memory::Size GetMemSize() const noexcept override { return ra::data::Memory::Size::ThirtyTwoBit; }

    unsigned int GetPadding() const noexcept override { return 3U; }

    unsigned int BuildValue(const uint8_t* ptr) const noexcept override
    {
        if (!ptr)
            return 0;

        GSL_SUPPRESS_TYPE1 return *reinterpret_cast<const unsigned int*>(ptr);
    }

protected:
    void ApplyConstantFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nConstantValue,
        std::vector<ra::data::ByteAddress>& vMatches) const override
    {
        ApplyCompareFilterLittleEndian<uint32_t, true>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nConstantValue, vMatches);
    }

    void ApplyCompareFilter(const uint8_t* pBytes, const uint8_t* pBytesStop,
        const MemBlock& pPreviousBlock, ComparisonType nComparison, unsigned nAdjustment,
        std::vector<ra::data::ByteAddress>& vMatches) const override
    {
        ApplyCompareFilterLittleEndian<uint32_t, false>(pBytes, pBytesStop,
            pPreviousBlock, nComparison, nAdjustment, vMatches);
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_32BIT_H
