#ifndef RA_UI_DRAWING_GDI_SURFACE_H
#define RA_UI_DRAWING_GDI_SURFACE_H
#pragma once

#include "ui\drawing\ISurface.hh"

namespace ra {
namespace ui {
namespace drawing {
namespace gdi {

class GDISurface : public ISurface
{
public:
    explicit GDISurface(HDC hDC, RECT& rcDEST) noexcept;

    ~GDISurface() noexcept;

    size_t GetWidth() const override { return m_nWidth; }
    size_t GetHeight() const override { return m_nHeight; }

    void FillRectangle(int nX, int nY, int nWidth, int nHeight, Color nColor) override;

    int LoadFont(const std::string& sFont, int nFontSize, FontStyles nStyle) override;
    ra::ui::Size MeasureText(int nFont, const std::wstring& sText) const override;
    void WriteText(int nX, int nY, int nFont, Color nColor, const std::wstring& sText) override;

private:
    void SwitchFont(int nFont) const;

    HDC m_hDC;
    int m_nWidth;
    int m_nHeight;

    struct GDIFont
    {
        GDIFont(const std::string& sFontName, int nFontSize, FontStyles nStyle, HFONT hFont)
            : sFontName(sFontName), nFontSize(nFontSize), nStyle(nStyle), hFont(hFont)
        {
        }

        std::string sFontName;
        int nFontSize{};
        FontStyles nStyle{};
        HFONT hFont{};
    };
    std::vector<GDIFont> m_vFonts;

    mutable int m_nCurrentFont{};
    Color m_nCurrentTextColor{ 0U };
};

} // namespace gdi
} // namespace drawing
} // namespace ui
} // namespace ra

#endif RA_UI_DRAWING_GDI_SURFACE_H
