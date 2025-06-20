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
    MemSize GetMemSize() const noexcept override { return MemSize::Double32; }

protected:
    float BuildFloatValue(const uint8_t* ptr) const noexcept override
    {
        return ConvertFloatValue(ptr, RC_MEMSIZE_DOUBLE32);
    }

    uint32_t DeconstructFloatValue(float fValue) const noexcept override
    {
        return ra::data::FloatToU32(fValue, MemSize::Double32);
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_DOUBLE32_H
