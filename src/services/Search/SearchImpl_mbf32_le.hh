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
    ra::data::Memory::Size GetMemSize() const noexcept override { return ra::data::Memory::Size::MBF32LE; }

protected:
    float BuildFloatValue(const uint8_t* ptr) const noexcept override
    {
        return ConvertFloatValue(ptr, RC_MEMSIZE_MBF32_LE);
    }

    uint32_t DeconstructFloatValue(float fValue) const noexcept override
    {
        return ra::data::Memory::FloatToU32(fValue, ra::data::Memory::Size::MBF32LE);
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_MBF32_LE_H
