#ifndef RA_DATA_MEMORY_H
#define RA_DATA_MEMORY_H
#pragma once

#include <stdint.h>
#include <string>

namespace ra {
namespace data {

using ByteAddress = uint32_t;

class Memory {
public:
    enum class Size : uint8_t
    {
        Bit_0,
        Bit_1,
        Bit_2,
        Bit_3,
        Bit_4,
        Bit_5,
        Bit_6,
        Bit_7,
        Nibble_Lower,
        Nibble_Upper,
        EightBit,
        SixteenBit,
        TwentyFourBit,
        ThirtyTwoBit,
        BitCount,
        SixteenBitBigEndian,
        TwentyFourBitBigEndian,
        ThirtyTwoBitBigEndian,
        Float,
        MBF32,
        MBF32LE,
        FloatBigEndian,
        Double32,
        Double32BigEndian,

        // extended sizes not supported by rcheevos
        Unknown,
        Text,
        Array
    };

    static constexpr const wchar_t* SizeString(Size nSize)
    {
        switch (nSize)
        {
            case Size::Bit_0:
                return L"Bit0";
            case Size::Bit_1:
                return L"Bit1";
            case Size::Bit_2:
                return L"Bit2";
            case Size::Bit_3:
                return L"Bit3";
            case Size::Bit_4:
                return L"Bit4";
            case Size::Bit_5:
                return L"Bit5";
            case Size::Bit_6:
                return L"Bit6";
            case Size::Bit_7:
                return L"Bit7";
            case Size::Nibble_Lower:
                return L"Lower4";
            case Size::Nibble_Upper:
                return L"Upper4";
            case Size::EightBit:
                return L"8-bit";
            case Size::SixteenBit:
                return L"16-bit";
            case Size::TwentyFourBit:
                return L"24-bit";
            case Size::ThirtyTwoBit:
                return L"32-bit";
            case Size::BitCount:
                return L"BitCount";
            case Size::SixteenBitBigEndian:
                return L"16-bit BE";
            case Size::TwentyFourBitBigEndian:
                return L"24-bit BE";
            case Size::ThirtyTwoBitBigEndian:
                return L"32-bit BE";
            case Size::Float:
                return L"Float";
            case Size::MBF32:
                return L"MBF32";
            case Size::MBF32LE:
                return L"MBF32 LE";
            case Size::FloatBigEndian:
                return L"Float BE";
            case Size::Double32:
                return L"Double32";
            case Size::Double32BigEndian:
                return L"Double32 BE";
            case Size::Text:
                return L"ASCII";
            case Size::Array:
                return L"Array";
            default:
            case Size::Unknown:
                return L"Unknown";
        }
    }

    static constexpr unsigned int SizeBits(Size nSize)
    {
        switch (nSize)
        {
            case Size::ThirtyTwoBit:
            case Size::ThirtyTwoBitBigEndian:
            case Size::Float:
            case Size::FloatBigEndian:
            case Size::MBF32:
            case Size::MBF32LE:
            case Size::Double32:
            case Size::Double32BigEndian:
                return 32;

            case Size::EightBit:
                return 8;

            case Size::SixteenBit:
            case Size::SixteenBitBigEndian:
                return 16;

            case Size::TwentyFourBit:
            case Size::TwentyFourBitBigEndian:
                return 24;

            case Size::BitCount: // value will be 0-8
            case Size::Nibble_Lower:
            case Size::Nibble_Upper:
                return 4;

            case Size::Bit_0:
            case Size::Bit_1:
            case Size::Bit_2:
            case Size::Bit_3:
            case Size::Bit_4:
            case Size::Bit_5:
            case Size::Bit_6:
            case Size::Bit_7:
                return 1;

            default:
                return 0;
        }
    }

    static constexpr unsigned int SizeBytes(Size nSize)
    {
        switch (nSize)
        {
            case Size::ThirtyTwoBit:
            case Size::ThirtyTwoBitBigEndian:
            case Size::Float:
            case Size::FloatBigEndian:
            case Size::MBF32:
            case Size::MBF32LE:
                return 4;

            case Size::SixteenBit:
            case Size::SixteenBitBigEndian:
                return 2;

            case Size::TwentyFourBit:
            case Size::TwentyFourBitBigEndian:
                return 3;

            case Size::Text:
                return 0;

            default:
                return 1;
        }
    }

    static constexpr unsigned int SizeMax(Size nSize)
    {
        const auto nBits = SizeBits(nSize);
        if (nBits >= 32)
            return 0xFFFFFFFF;

        return (1 << nBits) - 1;
    }

    static constexpr bool SizeIsFloat(Size nSize)
    {
        switch (nSize)
        {
            case Size::Float:
            case Size::FloatBigEndian:
            case Size::MBF32:
            case Size::MBF32LE:
            case Size::Double32:
            case Size::Double32BigEndian:
                return true;

            default:
                return false;
        }
    }

    static constexpr bool SizeIsBigEndian(Size nSize)
    {
        switch (nSize)
        {
            case Size::SixteenBitBigEndian:
            case Size::TwentyFourBitBigEndian:
            case Size::ThirtyTwoBitBigEndian:
            case Size::FloatBigEndian:
            case Size::Double32BigEndian:
            case Size::MBF32:
                return true;

            default:
                return false;
        }
    }

    static Size SizeFromRcheevosSize(uint8_t nSize) noexcept;

    enum class Format : uint8_t
    {
        Hex,
        Dec,
        Unknown,
    };

    static std::wstring FormatValue(unsigned nValue, Size nSize, Format nFormat);

    static unsigned FloatToU32(float fValue, Size nFloatType) noexcept;
    static float U32ToFloat(unsigned nValue, Size nFloatType) noexcept;

    static constexpr uint32_t ReverseBytes(uint32_t nValue) noexcept
    {
        return ((nValue & 0xFF000000) >> 24) |
            ((nValue & 0x00FF0000) >> 8) |
            ((nValue & 0x0000FF00) << 8) |
            ((nValue & 0x000000FF) << 24);
    }
};

} // namespace data
} // namespace ra

#endif RA_DATA_MEMORY_H
