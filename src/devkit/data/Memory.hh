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
        Bit0,
        Bit1,
        Bit2,
        Bit3,
        Bit4,
        Bit5,
        Bit6,
        Bit7,
        NibbleLower,
        NibbleUpper,
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

    /// <summary>
    /// Gets a string representation of the size (as displayed to the user).
    /// </summary>
    static constexpr const wchar_t* SizeString(Size nSize)
    {
        switch (nSize)
        {
            case Size::Bit0:
                return L"Bit0";
            case Size::Bit1:
                return L"Bit1";
            case Size::Bit2:
                return L"Bit2";
            case Size::Bit3:
                return L"Bit3";
            case Size::Bit4:
                return L"Bit4";
            case Size::Bit5:
                return L"Bit5";
            case Size::Bit6:
                return L"Bit6";
            case Size::Bit7:
                return L"Bit7";
            case Size::NibbleLower:
                return L"Lower4";
            case Size::NibbleUpper:
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

    /// <summary>
    /// Gets the number of bits needed to represent a value of the specified size.
    /// </summary>
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
            case Size::NibbleLower:
            case Size::NibbleUpper:
                return 4;

            case Size::Bit0:
            case Size::Bit1:
            case Size::Bit2:
            case Size::Bit3:
            case Size::Bit4:
            case Size::Bit5:
            case Size::Bit6:
            case Size::Bit7:
                return 1;

            default:
                return 0;
        }
    }

    /// <summary>
    /// Gets the number of bytes needed to represent a value of the specified size.
    /// </summary>
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

    /// <summary>
    /// Gets the maximum value that can be represented by the specified size.
    /// </summary>
    static constexpr unsigned int SizeMax(Size nSize)
    {
        const auto nBits = SizeBits(nSize);
        if (nBits >= 32)
            return 0xFFFFFFFF;

        return (1 << nBits) - 1;
    }

    /// <summary>
    /// Gets whether or not a size supports floating point values.
    /// </summary>
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

    /// <summary>
    /// Gets whether or not a size is stored in memory in big-endian order.
    /// </summary>
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

    /// <summary>
    /// Converts an RC_MEMSIZE_ value to a Memory::Size value.
    /// </summary>
    static Size SizeFromRcheevosSize(uint8_t nSize) noexcept;

    enum class Format : uint8_t
    {
        Hex,
        Dec,
        Unknown,
    };

    /// <summary>
    /// Converts a raw 32-bits of memory into a user-friendly string value.
    /// </summary>
    static std::wstring FormatValue(uint32_t nValue, Size nSize, Format nFormat);

    /// <summary>
    /// Encodes a float into a raw 32-bit memory value.
    /// </summary>
    static uint32_t FloatToU32(float fValue, Size nFloatType) noexcept;

    /// <summary>
    /// Decodes a float from a raw 32-bit memory valu.
    /// </summary>
    static float U32ToFloat(uint32_t nValue, Size nFloatType) noexcept;

    /// <summary>
    /// Swaps the order of bytes in a 32-bit value (switches little-endian to big-endian).
    /// </summary>
    static constexpr uint32_t ReverseBytes(uint32_t nValue) noexcept
    {
        return ((nValue & 0xFF000000) >> 24) |
            ((nValue & 0x00FF0000) >> 8) |
            ((nValue & 0x0000FF00) << 8) |
            ((nValue & 0x000000FF) << 24);
    }

    /// <summary>
    /// Attempts to parse a string into a <see cref="ByteAddress" />.
    /// </summary>
    /// <param name="sAddress">The string to parse.</param>
    /// <param name="nAddress">[out] The resulting address.</param>
    /// <returns><c>true</c> if the string was parsed successfully, <c>false</c> if not.</returns>
    /// <remarks>Expects a string of hex characters with an optional '$' or '0x' prefix.</remarks>
    static bool TryParseAddress(const std::string_view sAddress, ByteAddress& nAddress) noexcept;

    /// <summary>
    /// Attempts to parse a string into a <see cref="ByteAddress" />.
    /// </summary>
    /// <param name="sAddress">The string to parse.</param>
    /// <param name="nAddress">[out] The resulting address.</param>
    /// <returns><c>true</c> if the string was parsed successfully, <c>false</c> if not.</returns>
    /// <remarks>Expects a string of hex characters with an optional '$' or '0x' prefix.</remarks>
    static bool TryParseAddress(const std::wstring_view sAddress, ByteAddress& nAddress) noexcept;

    /// <summary>
    /// Attempts to parse a string into a <see cref="ByteAddress" />.
    /// </summary>
    /// <param name="sAddress">The string to parse.</param>
    /// <returns>The parsed value, <c>0</c> if parsing failed.</returns>
    /// <remarks>Expects a string of hex characters with an optional '$' or '0x' prefix.</remarks>
    static ByteAddress ParseAddress(const std::string_view sAddress) noexcept;

    /// <summary>
    /// Attempts to parse a string into a <see cref="ByteAddress" />.
    /// </summary>
    /// <param name="sAddress">The string to parse.</param>
    /// <returns>The parsed value, <c>0</c> if parsing failed.</returns>
    /// <remarks>Expects a string of hex characters with an optional '$' or '0x' prefix.</remarks>
    static ByteAddress ParseAddress(const std::wstring_view sAddress) noexcept;

    /// <summary>
    /// Attempts to parse a string into two <see cref="ByteAddress" />es.
    /// </summary>
    /// <param name="sRange">The string to parse.</param>
    /// <param name="nStartAddress">[out] The first address in the range.</param>
    /// <param name="nEndAddress">[out] The last address in the range.</param>
    /// <returns><c>true</c> if the provided string could be parsed, <c>false</c> if not.</returns>
    static bool TryParseAddressRange(const std::wstring& sRange, ByteAddress& nStartAddress, ByteAddress& nEndAddress) noexcept;
};

} // namespace data
} // namespace ra

#endif RA_DATA_MEMORY_H
