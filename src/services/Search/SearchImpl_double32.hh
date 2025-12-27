#ifndef SEARCHIMPL_DOUBLE32_H
#define SEARCHIMPL_DOUBLE32_H
#pragma once

#include "SearchImpl_float.hh"

namespace ra {
namespace services {
namespace search {

class Double32SearchImpl : public FloatSearchImpl
{
public:
    ra::data::Memory::Size GetMemSize() const noexcept override { return ra::data::Memory::Size::Double32; }

    // technically, doubles are 8-bytes long, so padding should be 7, but
    // unaligned searches only look at 4-bytes at a time, so the 3 returned
    // by the base class is correct
    //unsigned int GetPadding() const noexcept override { return 7U; }

protected:
    float BuildFloatValue(const uint8_t* ptr) const noexcept override
    {
        return ConvertFloatValue(ptr, RC_MEMSIZE_DOUBLE32);
    }

    uint32_t DeconstructFloatValue(float fValue) const noexcept override
    {
        return ra::data::Memory::FloatToU32(fValue, ra::data::Memory::Size::Double32);
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_DOUBLE32_H
