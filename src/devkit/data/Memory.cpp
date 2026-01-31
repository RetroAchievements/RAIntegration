#include "Memory.hh"

#include "util/Strings.hh"

#include <rc_runtime_types.h>
#include <rcheevos/src/rcheevos/rc_internal.h>

namespace ra {
namespace data {

Memory::Size Memory::SizeFromRcheevosSize(uint8_t nSize) noexcept
{
    switch (nSize)
    {
        case RC_MEMSIZE_BIT_0: return Size::Bit0;
        case RC_MEMSIZE_BIT_1: return Size::Bit1;
        case RC_MEMSIZE_BIT_2: return Size::Bit2;
        case RC_MEMSIZE_BIT_3: return Size::Bit3;
        case RC_MEMSIZE_BIT_4: return Size::Bit4;
        case RC_MEMSIZE_BIT_5: return Size::Bit5;
        case RC_MEMSIZE_BIT_6: return Size::Bit6;
        case RC_MEMSIZE_BIT_7: return Size::Bit7;
        case RC_MEMSIZE_LOW: return Size::NibbleLower;
        case RC_MEMSIZE_HIGH: return Size::NibbleUpper;
        case RC_MEMSIZE_8_BITS: return Size::EightBit;
        case RC_MEMSIZE_16_BITS: return Size::SixteenBit;
        case RC_MEMSIZE_24_BITS: return Size::TwentyFourBit;
        case RC_MEMSIZE_32_BITS: return Size::ThirtyTwoBit;
        case RC_MEMSIZE_BITCOUNT: return Size::BitCount;
        case RC_MEMSIZE_16_BITS_BE: return Size::SixteenBitBigEndian;
        case RC_MEMSIZE_24_BITS_BE: return Size::TwentyFourBitBigEndian;
        case RC_MEMSIZE_32_BITS_BE: return Size::ThirtyTwoBitBigEndian;
        case RC_MEMSIZE_FLOAT: return Size::Float;
        case RC_MEMSIZE_FLOAT_BE: return Size::FloatBigEndian;
        case RC_MEMSIZE_MBF32: return Size::MBF32;
        case RC_MEMSIZE_MBF32_LE: return Size::MBF32LE;
        case RC_MEMSIZE_DOUBLE32: return Size::Double32;
        case RC_MEMSIZE_DOUBLE32_BE: return Size::Double32BigEndian;
        default:
            assert(!"Unsupported operand size");
            return Size::EightBit;
    }
}

static std::wstring U32ToFloatString(uint32_t nValue, uint8_t nFloatType)
{
    rc_typed_value_t value{};
    value.type = RC_VALUE_TYPE_UNSIGNED;
    value.value.u32 = nValue;
    rc_transform_memref_value(&value, nFloatType);

    if (value.value.f32 < 0.000001)
    {
        if (value.value.f32 > 0.0)
            return ra::util::String::Printf(L"%e", value.value.f32);

        if (value.value.f32 < 0.0 && value.value.f32 > -0.000001)
            return ra::util::String::Printf(L"%e", value.value.f32);
    }

    std::wstring sValue = ra::util::String::Printf(L"%f", value.value.f32);
    while (sValue.back() == L'0')
        sValue.pop_back();
    if (sValue.back() == L'.')
        sValue.push_back(L'0');

    return sValue;
}

std::wstring Memory::FormatValue(uint32_t nValue, Memory::Size nSize, Memory::Format nFormat)
{
    switch (nSize)
    {
        case Size::Float:
            return U32ToFloatString(nValue, RC_MEMSIZE_FLOAT);

        case Size::FloatBigEndian:
            return U32ToFloatString(nValue, RC_MEMSIZE_FLOAT_BE);

        case Size::Double32:
            return U32ToFloatString(nValue, RC_MEMSIZE_DOUBLE32);

        case Size::Double32BigEndian:
            return U32ToFloatString(nValue, RC_MEMSIZE_DOUBLE32_BE);

        case Size::MBF32:
            return U32ToFloatString(nValue, RC_MEMSIZE_MBF32);

        case Size::MBF32LE:
            return U32ToFloatString(nValue, RC_MEMSIZE_MBF32_LE);

        default:
            if (nFormat == Format::Dec)
                return std::to_wstring(nValue);

            const auto nBits = SizeBits(nSize);
            switch (nBits / 8)
            {
                default: return ra::util::String::Printf(L"%x", nValue);
                case 1: return ra::util::String::Printf(L"%02x", nValue);
                case 2: return ra::util::String::Printf(L"%04x", nValue);
                case 3: return ra::util::String::Printf(L"%06x", nValue);
                case 4: return ra::util::String::Printf(L"%08x", nValue);
            }
    }
}

uint32_t Memory::FloatToU32(float fValue, Memory::Size nFloatType) noexcept
{
    // this leverages the fact that a "float" is encoded as little-endian IEE754
    union u
    {
        float fValue;
        uint32_t nValue;
    } uUnion{};

    uUnion.fValue = fValue;

    switch (nFloatType)
    {
        case Size::Float:
            return uUnion.nValue;

        case Size::FloatBigEndian:
            return ReverseBytes(uUnion.nValue);

        case Size::Double32:
        case Size::Double32BigEndian:
        {
            // double has 3 extra bits for the exponent
            const int32_t exponent = ra::to_signed((uUnion.nValue >> 23) & 0xFF) - 127 + 1023; // change exponent base from 127 to 1023
            const unsigned nValue = ((uUnion.nValue & 0x007FFFFF) >> 3) |        // mantissa is shifted three bits right
                ((ra::to_unsigned(exponent) & 0x7FF) << 20) | // adjusted exponent
                ((uUnion.nValue & 0x80000000));               // sign is unmoved

            return (nFloatType == Size::Double32) ? nValue : ReverseBytes(nValue);
        }

        case Size::MBF32:
        case Size::MBF32LE:
        {
            // MBF32 puts the sign after the exponent, uses a 129 base instead of 127, and stores in big endian
            unsigned nValue = ((uUnion.nValue & 0x007FFFFF)) |      // mantissa is unmoved
                ((uUnion.nValue & 0x7F800000) << 1) | // exponent is shifted one bit left
                ((uUnion.nValue & 0x80000000) >> 8);  // sign is shifted eight bits right

            nValue += 0x02000000; // adjust to 129 base
            return (nFloatType == Size::MBF32LE) ? nValue : ReverseBytes(nValue);
        }

        default:
            return 0;
    }
}

float Memory::U32ToFloat(uint32_t nValue, Memory::Size nFloatType) noexcept
{
    // this leverages the fact that a "float" is encoded as little-endian IEE754
    union u
    {
        float fValue;
        unsigned nValue;
    } uUnion{};

    switch (nFloatType)
    {
        case Size::FloatBigEndian:
            nValue = ReverseBytes(nValue);
            __fallthrough; // to Memory::Size::Float

        case Size::Float:
            uUnion.nValue = nValue;
            break;

        case Size::Double32BigEndian:
            nValue = ReverseBytes(nValue);
            __fallthrough; // to Memory::Size::Double32

        case Size::Double32:
        {
            // double has 3 extra bits for the exponent, and uses a 1023 base instead of a 127 base
            const int32_t exponent = ra::to_signed((uUnion.nValue >> 20) & 0x7FF) - 1023 + 127; // change exponent base from 1023 to 127
            nValue = ((uUnion.nValue & 0x000FFFFF) << 3) |        // mantissa is shifted three bits left
                ((ra::to_unsigned(exponent) & 0xFF) << 23) | // adjusted exponent
                ((uUnion.nValue & 0x80000000));              // sign is unmoved

            uUnion.nValue = nValue;
            break;
        }

        case Size::MBF32:
            nValue = ReverseBytes(nValue);
            __fallthrough; // to Memory::Size::MBF32LE

        case Size::MBF32LE:
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

template<typename CharT, typename = std::enable_if_t<is_char_v<CharT>>, typename... Ts>
static uint32_t TryParseAddressInternal(const CharT* sAddress, ra::data::ByteAddress& nAddress) noexcept
{
    const CharT* ptr = sAddress;
    if (ptr == nullptr)
        return 0;

    if (*ptr == '$')
        ++ptr;
    else if (ptr[0] == '0' && ptr[1] == 'x')
        ptr += 2;

    const CharT* start = ptr;
    nAddress = 0;
    while (CharT c = *ptr)
    {
        if (c >= '0' && c <= '9')
        {
            nAddress <<= 4;
            nAddress += (c - '0');
        }
        else if (c >= 'a' && c <= 'f')
        {
            nAddress <<= 4;
            nAddress += (c - 'a' + 10);
        }
        else if (c >= 'A' && c <= 'F')
        {
            nAddress <<= 4;
            nAddress += (c - 'A' + 10);
        }
        else
        {
            break;
        }

        ++ptr;
    }

    if (ptr == start)
        return 0;

    return gsl::narrow_cast<uint32_t>(ptr - sAddress);
}

bool Memory::TryParseAddress(const std::string_view sAddress, ra::data::ByteAddress& nAddress) noexcept
{
    const uint32_t nUsed = TryParseAddressInternal(sAddress.data(), nAddress);
    return (nUsed == sAddress.length());
}

bool Memory::TryParseAddress(const std::wstring_view sAddress, ra::data::ByteAddress& nAddress) noexcept
{
    const uint32_t nUsed = TryParseAddressInternal(sAddress.data(), nAddress);
    return (nUsed == sAddress.length());
}

ra::data::ByteAddress Memory::ParseAddress(const std::string_view sAddress) noexcept
{
    ra::data::ByteAddress nAddress;
    return TryParseAddress(sAddress, nAddress) ? nAddress : 0;
}

ra::data::ByteAddress Memory::ParseAddress(const std::wstring_view sAddress) noexcept
{
    ra::data::ByteAddress nAddress;
    return TryParseAddress(sAddress, nAddress) ? nAddress : 0;
}

bool Memory::TryParseAddressRange(const std::wstring& sRange, ra::data::ByteAddress& nStart, ra::data::ByteAddress& nEnd) noexcept
{
    const wchar_t* ptr = sRange.data();
    if (!ptr || *ptr == '\0')
    {
        // no range specified, search all
        nStart = 0;
        nEnd = 0xFFFFFFFF;
        return true;
    }

    auto nAddressLength = TryParseAddressInternal(ptr, nStart);
    if (nAddressLength == 0)
    {
        nStart = 0;
        nEnd = 0;
        return false;
    }

    ptr += nAddressLength;
    while (iswspace(*ptr))
        ++ptr;

    if (*ptr == '\0')
    {
        // range is a single address
        nEnd = nStart;
        return true;
    }

    if (*ptr != '-')
    {
        nEnd = 0;
        return false;
    }

    ++ptr;
    while (iswspace(*ptr))
        ++ptr;

    nAddressLength = TryParseAddressInternal(ptr, nEnd);
    if (nAddressLength == 0)
    {
        nEnd = 0;
        return false;
    }

    ptr += nAddressLength;
    if (*ptr != '\0')
        return false;

    if (nEnd < nStart)
        std::swap(nStart, nEnd);

    return true;
}

} // namespace data
} // namespace ra
