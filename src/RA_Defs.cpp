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
    return pEmulatorContext.FormatAddress(nAddr);
#else
    return ra::StringPrintf("0x%04x", nAddr);
#endif
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

    if (value.value.f32 < 0.000001)
    {
        if (value.value.f32 > 0.0)
            return ra::StringPrintf(L"%e", value.value.f32);

        if (value.value.f32 < 0.0 && value.value.f32 > -0.000001)
            return ra::StringPrintf(L"%e", value.value.f32);
    }

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

        case MemSize::FloatBigEndian:
            return U32ToFloatString(nValue, RC_MEMSIZE_FLOAT_BE);

        case MemSize::Double32:
            return U32ToFloatString(nValue, RC_MEMSIZE_DOUBLE32);

        case MemSize::Double32BigEndian:
            return U32ToFloatString(nValue, RC_MEMSIZE_DOUBLE32_BE);

        case MemSize::MBF32:
            return U32ToFloatString(nValue, RC_MEMSIZE_MBF32);

        case MemSize::MBF32LE:
            return U32ToFloatString(nValue, RC_MEMSIZE_MBF32_LE);

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

const char* ValueFormatToString(ValueFormat nFormat) noexcept
{
    switch (nFormat)
    {
        case ValueFormat::Centiseconds: return "MILLISECS";
        case ValueFormat::Frames: return "TIME";
        case ValueFormat::Minutes: return "MINUTES";
        case ValueFormat::Score: return "SCORE";
        case ValueFormat::Seconds: return "TIMESECS";
        case ValueFormat::SecondsAsMinutes: return "SECS_AS_MINS";
        case ValueFormat::Value: return "VALUE";
        case ValueFormat::Float1: return "FLOAT1";
        case ValueFormat::Float2: return "FLOAT2";
        case ValueFormat::Float3: return "FLOAT3";
        case ValueFormat::Float4: return "FLOAT4";
        case ValueFormat::Float5: return "FLOAT5";
        case ValueFormat::Float6: return "FLOAT6";
        case ValueFormat::Fixed1: return "FIXED1";
        case ValueFormat::Fixed2: return "FIXED2";
        case ValueFormat::Fixed3: return "FIXED3";
        case ValueFormat::Tens: return "TENS";
        case ValueFormat::Hundreds: return "HUNDREDS";
        case ValueFormat::Thousands: return "THOUSANDS";
        case ValueFormat::UnsignedValue: return "UNSIGNED";
        default: return "UNKNOWN";
    }
}

ValueFormat ValueFormatFromString(const std::string& sFormat) noexcept
{
    return ra::itoe<ValueFormat>(rc_parse_format(sFormat.c_str()));
}

static unsigned ReverseBytes(unsigned nValue) noexcept
{
    return ((nValue & 0xFF000000) >> 24) |
           ((nValue & 0x00FF0000) >> 8) |
           ((nValue & 0x0000FF00) << 8) |
           ((nValue & 0x000000FF) << 24);
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

    switch (nFloatType)
    {
        case MemSize::Float:
            return uUnion.nValue;

        case MemSize::FloatBigEndian:
            return ReverseBytes(uUnion.nValue);

        case MemSize::Double32:
        case MemSize::Double32BigEndian:
        {
            // double has 3 extra bits for the exponent
            const int32_t exponent = ra::to_signed((uUnion.nValue >> 23) & 0xFF) - 127 + 1023; // change exponent base from 127 to 1023
            const unsigned nValue = ((uUnion.nValue & 0x007FFFFF) >> 3)  |        // mantissa is shifted three bits right
                                    ((ra::to_unsigned(exponent) & 0x7FF) << 20) | // adjusted exponent
                                    ((uUnion.nValue & 0x80000000));               // sign is unmoved

            return (nFloatType == MemSize::Double32) ? nValue : ReverseBytes(nValue);
        }

        case MemSize::MBF32:
        case MemSize::MBF32LE:
        {
            // MBF32 puts the sign after the exponent, uses a 129 base instead of 127, and stores in big endian
            unsigned nValue = ((uUnion.nValue & 0x007FFFFF)) |      // mantissa is unmoved
                              ((uUnion.nValue & 0x7F800000) << 1) | // exponent is shifted one bit left
                              ((uUnion.nValue & 0x80000000) >> 8);  // sign is shifted eight bits right

            nValue += 0x02000000; // adjust to 129 base
            return (nFloatType == MemSize::MBF32LE) ? nValue : ReverseBytes(nValue);            
        }

        default:
            return 0;
    }
}

float U32ToFloat(unsigned nValue, MemSize nFloatType) noexcept
{
    // this leverages the fact that Windows uses IEE754 floats
    union u
    {
        float fValue;
        unsigned nValue;
    } uUnion{};

    switch (nFloatType)
    {
        case MemSize::FloatBigEndian:
            nValue = ReverseBytes(nValue);
            __fallthrough; // to MemSize::Float

        case MemSize::Float:
            uUnion.nValue = nValue;
            break;

        case MemSize::Double32BigEndian:
            nValue = ReverseBytes(nValue);
            __fallthrough; // to MemSize::Double32

        case MemSize::Double32:
        {
            // double has 3 extra bits for the exponent, and uses a 1023 base instead of a 127 base
            const int32_t exponent = ra::to_signed((uUnion.nValue >> 20) & 0x7FF) - 1023 + 127; // change exponent base from 1023 to 127
            nValue = ((uUnion.nValue & 0x000FFFFF) << 3) |        // mantissa is shifted three bits left
                     ((ra::to_unsigned(exponent) & 0xFF) << 23) | // adjusted exponent
                     ((uUnion.nValue & 0x80000000));              // sign is unmoved

            uUnion.nValue = nValue;
            break;
        }

        case MemSize::MBF32:
            nValue = ReverseBytes(nValue);
            __fallthrough; // to MemSize::MBF32LE

        case MemSize::MBF32LE:
            // MBF32 puts the sign after the exponent, uses a 129 base instead of 127, and stores in big endian
            nValue -= 0x02000000;                   // adjust to 129 base
            nValue = ((nValue & 0x007FFFFF)) |      // mantissa is unmoved
                     ((nValue & 0xFF000000) >> 1) | // exponent is shifted one bit right
                     ((nValue & 0x00800000) << 8);  // sign is shifted eight bits left

            uUnion.nValue = nValue;
            break;

        default:
            return 0.0f;
    }

    return uUnion.fValue;
}

} /* namespace data */
} /* namespace ra */
