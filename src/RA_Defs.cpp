#include "RA_Defs.h"

#include "RA_StringUtils.h"

#include "data\context\EmulatorContext.hh"

#include <rcheevos/src/rcheevos/rc_internal.h>

namespace ra {

_Use_decl_annotations_
std::string ByteAddressToString(ByteAddress nAddr)
{
#ifndef RA_UTEST
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::context::EmulatorContext>();
    if (pEmulatorContext.TotalMemorySize() > 0x10000)
        return ra::StringPrintf("0x%06x", nAddr);
    else
#endif
        return ra::StringPrintf("0x%04x", nAddr);
}

_Use_decl_annotations_
ByteAddress ByteAddressFromString(const std::string& sByteAddress)
{
    ra::ByteAddress address{};

    if (!ra::StringStartsWith(sByteAddress, "-")) // negative addresses not supported
    {
        char* pEnd;
        address = std::strtoul(sByteAddress.c_str(), &pEnd, 16);
        Ensures(pEnd != nullptr);
        if (*pEnd)
        {
            // hex parse failed
            address = {};
        }
    }

    return address;
}

namespace data {

static std::wstring U32ToFloatString(unsigned nValue, char nFloatType)
{
    rc_typed_value_t value;
    value.type = RC_VALUE_TYPE_UNSIGNED;
    value.value.u32 = nValue;
    rc_transform_memref_value(&value, nFloatType);

    std::wstring sValue = ra::StringPrintf(L"%f", value.value.f32);
    while (sValue.back() == L'0')
        sValue.pop_back();
    if (sValue.back() == L'.')
        sValue.push_back(L'0');

    return sValue;
}

std::wstring MemSizeFormat(unsigned nValue, MemSize nSize, MemFormat nFormat)
{
    switch (nSize)
    {
        case MemSize::Float:
            return U32ToFloatString(nValue, RC_MEMSIZE_FLOAT);

        case MemSize::MBF32:
            return U32ToFloatString(nValue, RC_MEMSIZE_MBF32);

        default:
            if (nFormat == MemFormat::Dec)
                return std::to_wstring(nValue);

            const auto nBits = MemSizeBits(nSize);
            switch (nBits / 8)
            {
                default: return ra::StringPrintf(L"%x", nValue);
                case 1: return ra::StringPrintf(L"%02x", nValue);
                case 2: return ra::StringPrintf(L"%04x", nValue);
                case 3: return ra::StringPrintf(L"%06x", nValue);
                case 4: return ra::StringPrintf(L"%08x", nValue);
            }
    }
}

unsigned FloatToU32(float fValue, MemSize nFloatType) noexcept
{
    // this leverages the fact that Windows uses IEE754 floats
    union u
    {
        float fValue;
        unsigned nValue;
    } uUnion;

    uUnion.fValue = fValue;
    if (nFloatType == MemSize::Float)
        return uUnion.nValue;

    // MBF32 puts the sign after the exponent, uses a 129 base instead of 127, and stores in big endian
    unsigned nValue = ((uUnion.nValue & 0x007FFFFF)) | // mantissa is unmoved
        ((uUnion.nValue & 0x7F800000) << 1) | // exponent is shifted one bit left
        ((uUnion.nValue & 0x80000000) >> 8);  // sign is shifted eight bits right

    nValue += 0x02000000; // adjust to 129 base

    return ((nValue & 0xFF000000) >> 24) | // convert to big endian
        ((nValue & 0x00FF0000) >> 8) |
        ((nValue & 0x0000FF00) << 8) |
        ((nValue & 0x000000FF) << 24);
}

float U32ToFloat(unsigned nValue, MemSize nFloatType) noexcept
{
    // this leverages the fact that Windows uses IEE754 floats
    union u
    {
        float fValue;
        unsigned nValue;
    } uUnion;

    uUnion.nValue = nValue;

    if (nFloatType != MemSize::Float)
    {
        nValue = ((nValue & 0xFF000000) >> 24) | // convert to big endian
            ((nValue & 0x00FF0000) >> 8) |
            ((nValue & 0x0000FF00) << 8) |
            ((nValue & 0x000000FF) << 24);

        // MBF32 puts the sign after the exponent, uses a 129 base instead of 127, and stores in big endian
        nValue -= 0x02000000; // adjust to 129 base
        nValue = ((nValue & 0x007FFFFF)) | // mantissa is unmoved
            ((nValue & 0xFF000000) >> 1) | // exponent is shifted one bit right
            ((nValue & 0x00800000) << 8);  // sign is shifted eight bits left

        uUnion.nValue = nValue;
    }

    return uUnion.fValue;
}

} /* namespace data */
} /* namespace ra */
