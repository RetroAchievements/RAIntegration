#ifndef RA_UI_DRAWING_ISURFACE_HH
#define RA_UI_DRAWING_ISURFACE_HH

#include "ui\Types.hh"

namespace ra {
namespace ui {
namespace drawing {

class ISurface
{
public:
    virtual ~ISurface() noexcept = default;

    ISurface(const ISurface&) noexcept = delete;
    ISurface(ISurface&&) noexcept = delete;
        
    /// <summary>
    /// Gets the width of the surface.
    /// </summary>
    virtual size_t GetWidth() const = 0;

    /// <summary>
    /// Gets the height of the surface.
    /// </summary>
    virtual size_t GetHeight() const = 0;
    
    /// <summary>
    /// Draws a solid rectangle.
    /// </summary>
    /// <param name="nX">The x coordinate of the left side of the rectangle.</param>
    /// <param name="nY">The y coordinate of the top of the rectangle.</param>
    /// <param name="nWidth">The width of the rectangle.</param>
    /// <param name="nHeight">The height of the rectangle.</param>
    /// <param name="nColor">The color of the rectangle.</param>
    virtual void FillRectangle(int nX, int nY, int nWidth, int nHeight, Color nColor) = 0;

    /// <summary>
    /// Loads a font resource.
    /// </summary>
    /// <param name="sFont">The name of the font.</param>
    /// <param name="nFontSize">The size (in pixels) to render the font.</param>
    /// <param name="nStyle">The style of the font.</param>
    /// <returns>Unique identifier for the font resource to pass to <see cref="WriteText" />, <c>0</c> if loading the font failed.</returns>
    virtual int LoadFont(const std::string& sFont, int nFontSize, FontStyles nStyle) = 0;
    
    /// <summary>
    /// Determines how much space would be required to display <paramref name="sText" /> using <paramref name="nFont" />.
    /// </summary>
    /// <param name="nFont">The unique identifier of the font to use. see <see cref="LoadFont" />.</param>
    /// <param name="sText">The text to measure.</param>
    virtual ra::ui::Size MeasureText(int nFont, const std::wstring& sText) const = 0;

    /// <summary>
    /// Writes the specified text to the surface.
    /// </summary>
    /// <param name="nX">The x coordinate to write at.</param>
    /// <param name="nY">The y coordinate to write at.</param>
    /// <param name="nFont">The unique identifier of the font to use. see <see cref="LoadFont" />.</param>
    /// <param name="nColor">The color to use.</param>
    /// <param name="sText">The text to write.</param>
    virtual void WriteText(int nX, int nY, int nFont, Color nColor, const std::wstring& sText) = 0;

protected:
    ISurface() noexcept = default;
};

} // namespace drawing
} // namespace ui
} // namespace ra

#endif // !RA_UI_DRAWING_ISURFACE_HH
