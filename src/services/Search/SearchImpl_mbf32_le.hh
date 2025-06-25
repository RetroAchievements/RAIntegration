#ifndef SEARCHIMPL_MBF32_LE_H
#define SEARCHIMPL_MBF32_LE_H
#pragma once

#include "SearchImpl_float.hh"

namespace ra {
namespace services {
namespace search {

class MBF32LESearchImpl : public FloatSearchImpl
{
public:
    MemSize GetMemSize() const noexcept override { return MemSize::MBF32LE; }

protected:
    float BuildFloatValue(const uint8_t* ptr) const noexcept override
    {
        return ConvertFloatValue(ptr, RC_MEMSIZE_MBF32_LE);
    }

    uint32_t DeconstructFloatValue(float fValue) const noexcept override
    {
        return ra::data::FloatToU32(fValue, MemSize::MBF32LE);
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_MBF32_LE_H
