#ifndef RA_DATA_VALUE_H
#define RA_DATA_VALUE_H
#pragma once

#include <stdint.h>
#include <string>

namespace ra {
namespace data {

class Value {
public:
    enum class Format : uint8_t
    {
        /// <summary>
        /// Value (in sixtieths of a second) converted to minutes/seconds/hundredths - %02d:%02d.%02d
        /// </summary>
        Frames,

        /// <summary>
        /// Value (in seconds) converted to minutes/seconds - %02d:%02d
        /// </summary>
        Seconds,

        /// <summary>
        /// Value (in hundredths of a second) converted to minutes/seconds/hundredths - %02d:%02d.%02d
        /// </summary>
        Centiseconds,

        /// <summary>
        /// Generic value padded with 0s to 6 digits (1234 => 001234)
        /// </summary>
        Score,

        /// <summary>
        /// Generic value with commas (1234 => 1,234)
        /// </summary>
        Value,

        /// <summary>
        /// Value (in minutes) converted to hours/minutes - %dh%02d
        /// </summary>
        Minutes,

        /// <summary>
        /// Value (in seconds) converted to hours/minutes - %dh%02d
        /// </summary>
        SecondsAsMinutes,

        /// <summary>
        /// A floating point value with one digit after the decimal (3.1)
        /// </summary>
        Float1,

        /// <summary>
        /// A floating point value with two digits after the decimal (3.14)
        /// </summary>
        Float2,

        /// <summary>
        /// A floating point value with three digits after the decimal (3.141)
        /// </summary>
        Float3,

        /// <summary>
        /// A floating point value with four digits after the decimal (3.1416)
        /// </summary>
        Float4,

        /// <summary>
        /// A floating point value with five digits after the decimal (3.14159)
        /// </summary>
        Float5,

        /// <summary>
        /// A floating point value with six digits after the decimal (3.141593)
        /// </summary>
        Float6,

        /// <summary>
        /// A fixed-point value with one digit after the decimal (12345 => 1,234.5)
        /// </summary>
        Fixed1,

        /// <summary>
        /// A fixed-point value with two digits after the decimal (12345 => 123.45)
        /// </summary>
        Fixed2,

        /// <summary>
        /// A fixed-point value with three digits after the decimal (12345 => 12.345)
        /// </summary>
        Fixed3,

        /// <summary>
        /// A number padded with a trailing zero (123 => 1,230)
        /// </summary>
        Tens,

        /// <summary>
        /// A number padded with two trailing zeroes (123 => 12,300)
        /// </summary>
        Hundreds,

        /// <summary>
        /// A number padded with three trailing zeroes (123 => 123,000)
        /// </summary>
        Thousands,

        /// <summary>
        /// Generic unsigned value (1234 => 1,234)
        /// </summary>
        UnsignedValue,

        /// <summary>
        /// Generic value without commas (1234 => 1234)
        /// </summary>
        Unformatted,
    };

    /// <summary>
    /// Gets a string representation of the format (as displayed to the user).
    /// </summary>
    static constexpr const wchar_t* FormatString(Format nFormat) noexcept
    {
        switch (nFormat)
        {
            case Format::Centiseconds:
                return L"Time (Centiseconds)";
            case Format::Frames:
                return L"Time (Frames)";
            case Format::Minutes:
                return L"Time (Minutes)";
            case Format::Score:
                return L"Score";
            case Format::Seconds:
                return L"Time (Seconds)";
            case Format::SecondsAsMinutes:
                return L"Time (Seconds as Minutes)";
            case Format::Value:
                return L"Value";
            case Format::Float1:
                return L"Value (Float1)";
            case Format::Float2:
                return L"Value (Float2)";
            case Format::Float3:
                return L"Value (Float3)";
            case Format::Float4:
                return L"Value (Float4)";
            case Format::Float5:
                return L"Value (Float5)";
            case Format::Float6:
                return L"Value (Float6)";
            case Format::Fixed1:
                return L"Value (Fixed1)";
            case Format::Fixed2:
                return L"Value (Fixed2)";
            case Format::Fixed3:
                return L"Value (Fixed3)";
            case Format::Tens:
                return L"Value (Tens)";
            case Format::Hundreds:
                return L"Value (Hundreds)";
            case Format::Thousands:
                return L"Value (Thousands)";
            case Format::UnsignedValue:
                return L"Value (Unsigned)";
            default:
                return L"Unknown";
        }
    }

    /// <summary>
    /// Gets the server enum for a format.
    /// </summary>
    static const char* FormatToServerEnum(Format nFormat) noexcept;

    /// <summary>
    /// Gets the format from a server enum.
    /// </summary>
    /// <remarks>Returns <c>Format::Value</c> for unknown formats</remarks>
    static Format FormatFromServerEnum(const std::string& sFormat) noexcept;

    /// <summary>
    /// Converts an RC_FORMAT_ value to a Value::Type value.
    /// </summary>
    static Format FormatFromRcheevosFormat(uint8_t nFormat) noexcept;

    /// <summary>
    /// Converts a Value::Type value to an RC_FORMAT_ value.
    /// </summary>
    static uint8_t FormatToRcheevosFormat(Format nFormat) noexcept;

    /// <summary>
    /// Converts a value to a string using the specified format.
    /// </summary>
    static std::wstring FormatValue(int32_t nValue, Format nFormat);
};

} // namespace data
} // namespace ra

#endif RA_DATA_MEMORY_H
