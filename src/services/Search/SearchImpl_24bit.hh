#ifndef SEARCHIMPL_24BIT_H
#define SEARCHIMPL_24BIT_H
#pragma once

#include "SearchImpl.hh"

namespace ra {
namespace services {
namespace search {

class TwentyFourBitSearchImpl : public SearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::TwentyFourBit; }

    unsigned int GetPadding() const noexcept override { return 2U; }

    unsigned int BuildValue(const uint8_t* ptr) const noexcept override
    {
        if (!ptr)
            return 0;

        GSL_SUPPRESS_TYPE1 return *reinterpret_cast<const unsigned int*>(ptr) & 0x00FFFFFF;
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_24BIT_H
