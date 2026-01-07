#ifndef SEARCHIMPL_BITCOUNT_H
#define SEARCHIMPL_BITCOUNT_H
#pragma once

#include "SearchImpl.hh"

#include "util\Strings.hh"

namespace ra {
namespace services {
namespace search {

class BitCountSearchImpl : public SearchImpl
{
protected:
    // captured value (result.nValue) is actually the raw byte
    ra::data::Memory::Size GetMemSize() const noexcept override { return ra::data::Memory::Size::BitCount; }

    unsigned int BuildValue(const unsigned char* ptr) const noexcept override
    {
        GSL_SUPPRESS_F6 Expects(ptr != nullptr);
        return GetBitCount(ptr[0]);
    }

    bool UpdateValue(const SearchResults& pResults, SearchResult& pResult,
        _Out_ std::wstring* sFormattedValue, const ra::data::context::EmulatorContext& pEmulatorContext) const override
    {
        const unsigned int nPreviousValue = pResult.nValue;
        pResult.nValue = pEmulatorContext.ReadMemory(pResult.nAddress, ra::data::Memory::Size::EightBit);

        if (sFormattedValue)
            *sFormattedValue = GetFormattedValue(pResults, pResult);

        return (pResult.nValue != nPreviousValue);
    }

    std::wstring GetFormattedValue(const SearchResults&, const SearchResult& pResult) const override
    {
        return ra::StringPrintf(L"%u (%c%c%c%c%c%c%c%c)", GetBitCount(pResult.nValue),
            (pResult.nValue & 0x80) ? '1' : '0',
            (pResult.nValue & 0x40) ? '1' : '0',
            (pResult.nValue & 0x20) ? '1' : '0',
            (pResult.nValue & 0x10) ? '1' : '0',
            (pResult.nValue & 0x08) ? '1' : '0',
            (pResult.nValue & 0x04) ? '1' : '0',
            (pResult.nValue & 0x02) ? '1' : '0',
            (pResult.nValue & 0x01) ? '1' : '0');
    }

private:
    bool GetValueFromCapturedMemoryBlock(const CapturedMemoryBlock& block, SearchResult& result) const noexcept override
    {
        if (result.nAddress < block.GetFirstAddress())
            return false;

        const unsigned int nOffset = result.nAddress - block.GetFirstAddress();
        if (nOffset >= block.GetBytesSize() - GetPadding())
            return false;

        result.nValue = *(block.GetBytes() + nOffset);
        return true;
    }

    static unsigned int GetBitCount(unsigned nValue) noexcept
    {
        static const std::array<unsigned,16> vBits = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 };
        return vBits.at(nValue & 0x0F) + vBits.at((nValue >> 4) & 0x0F);
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_BITCOUNT_H
