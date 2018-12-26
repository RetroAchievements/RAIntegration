#ifndef RA_UI_TYPES_H
#define RA_UI_TYPES_H
#pragma once

namespace ra {
namespace ui {

struct Position
{
    int X{};
    int Y{};
};

struct Size
{
    int Width{};
    int Height{};
};

enum class FontStyles
{
    Normal        = 0x00,
    Bold          = 0x01,
    Italic        = 0x02,
    Underline     = 0x04,
    Strikethrough = 0x08,
};

union Color {
    explicit Color(unsigned char R, unsigned char G, unsigned char B) noexcept : Color(0xFF, R, G, B) {}

    explicit Color(unsigned char A, unsigned char R, unsigned char G, unsigned char B) noexcept :
        Color((A << 24) | (R << 16) | (G << 8) | B)
    {}

    explicit Color(unsigned int ARGB) noexcept { this->ARGB = ARGB; }

    Color(const Color&) noexcept = default;
    Color& operator=(const Color&) noexcept = default;
    Color(Color&&) noexcept                 = default;
    Color& operator=(Color&&) noexcept = default;

    bool operator!=(const Color& that) const noexcept { return (this->ARGB != that.ARGB); }

    /// <summary>
    /// The 32-bit ARGB value.
    /// </summary>
    unsigned int ARGB{};

    struct Channel
    {
        /// <summary>
        /// The 8-bit blue value.
        /// </summary>
        unsigned char B{};

        /// <summary>
        /// The 8-bit green value.
        /// </summary>
        unsigned char G{};

        /// <summary>
        /// The 8-bit red value.
        /// </summary>
        unsigned char R{};

        /// <summary>
        /// The 8-bit alpha value.
        /// </summary>
        unsigned char A{};
    } Channel;
};
static_assert(sizeof(Color) == sizeof(unsigned int));

enum class RelativePosition
{
    None = 0,
    Before,    // To the left of, or above the target
    Near,      // Aligned with the target's left or top side
    Center,    // Centered relative to the target
    Far,       // Aligned with the target's right or bottom side
    After,     // To the right of, or below the target
};

} // namespace ui
} // namespace ra

#endif RA_UI_TYPES_H
