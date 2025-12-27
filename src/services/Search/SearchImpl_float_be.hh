#ifndef SEARCHIMPL_FLOAT_BE_H
#define SEARCHIMPL_FLOAT_BE_H
#pragma once

#include "SearchImpl_float.hh"

namespace ra {
namespace services {
namespace search {

class FloatBESearchImpl : public FloatSearchImpl
{
public:
    ra::data::Memory::Size GetMemSize() const noexcept override { return ra::data::Memory::Size::FloatBigEndian; }

protected:
    float BuildFloatValue(const unsigned char* ptr) const noexcept override
    {
        return ConvertFloatValue(ptr, RC_MEMSIZE_FLOAT_BE);
    }

    uint32_t DeconstructFloatValue(float fValue) const noexcept override
    {
        return ra::data::Memory::FloatToU32(fValue, ra::data::Memory::Size::FloatBigEndian);
    }
};

} // namespace search
} // namespace services
} // namespace ra

#endif // SEARCHIMPL_FLOAT_BE_H
