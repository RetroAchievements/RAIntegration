#ifndef SEARCHIMPL_DOUBLE32_BE_H
#define SEARCHIMPL_DOUBLE32_BE_H
#pragma once

#include "SearchImpl_float.hh"

namespace ra {
namespace services {
namespace search {

class Double32BESearchImpl : public FloatSearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::Double32BigEndian; }

    // technically, doubles are 8-bytes long, so padding should be 7, but
    // unaligned searches only look at 4-bytes at a time, so the 3 returned
    // by the base class is correct
    //unsigned int GetPadding() const noexcept override { return 7U; }

protected:
    float BuildFloatValue(const unsigned char* ptr) const noexcept override
    {
        return ConvertFloatValue(ptr, RC_MEMSIZE_DOUBLE32_BE);
    }

    uint32_t DeconstructFloatValue(float fValue) const noexcept override
    {
        return ra::data::FloatToU32(fValue, MemSize::Double32BigEndian);
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_DOUBLE32_BE_H
