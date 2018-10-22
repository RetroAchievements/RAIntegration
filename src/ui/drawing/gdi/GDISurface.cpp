#include "GDISurface.hh"

#include "ra_utility.h"

namespace ra {
namespace ui {
namespace drawing {
namespace gdi {

GDISurface::GDISurface(HDC hDC, RECT& rcDEST) noexcept
    : m_hDC(hDC), m_nWidth(rcDEST.right - rcDEST.left), m_nHeight(rcDEST.bottom - rcDEST.top)
{
    SelectObject(hDC, GetStockObject(DC_PEN));
    SelectObject(hDC, GetStockObject(DC_BRUSH));
}

GDISurface::~GDISurface() noexcept
{
    for (auto& pFont : m_vFonts)
        DeleteFont(pFont.hFont);

    m_vFonts.clear();
}

void GDISurface::FillRectangle(int nX, int nY, int nWidth, int nHeight, Color nColor)
{
    if (nColor.Channel.A != 0xFF)
    {

    }
    else
    {
        SetDCBrushColor(m_hDC, RGB(nColor.Channel.R, nColor.Channel.G, nColor.Channel.B));
        SetDCPenColor(m_hDC, RGB(nColor.Channel.R, nColor.Channel.G, nColor.Channel.B));
        Rectangle(m_hDC, nX, nY, nX + nWidth, nY + nHeight);
    }
}

int GDISurface::LoadFont(const std::string& sFont, int nFontSize, FontStyles nStyle)
{
    int i = 1;
    for (const auto& pFont : m_vFonts)
    {
        if (pFont.nFontSize == nFontSize && pFont.nStyle == nStyle && pFont.sFontName == sFont)
            return i;
        ++i;
    }

    using namespace ra::bitwise_ops;
    auto nWeight = ((nStyle & FontStyles::Bold) == FontStyles::Bold) ? FW_BOLD : FW_NORMAL;
    auto nItalic = ((nStyle & FontStyles::Italic) == FontStyles::Italic) ? TRUE : FALSE;
    auto nUnderline = ((nStyle & FontStyles::Underline) == FontStyles::Underline) ? TRUE : FALSE;
    auto nStrikeOut = ((nStyle & FontStyles::Strikethrough) == FontStyles::Strikethrough) ? TRUE : FALSE;

    HFONT hFont = CreateFontA(nFontSize, 0, 0, 0, nWeight, nItalic, nUnderline, nStrikeOut,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, sFont.c_str());

    if (hFont)
    {
        m_vFonts.emplace_back(sFont, nFontSize, nStyle, hFont);
        return m_vFonts.size();
    }

    return 0;
}

ra::ui::Size GDISurface::MeasureText(int nFont, const std::wstring& sText) const
{
    SwitchFont(nFont);

    SIZE szText;
    GetTextExtentPoint32W(m_hDC, sText.c_str(), sText.length(), &szText);

    return ra::ui::Size{ szText.cx, szText.cy };
}

void GDISurface::WriteText(int nX, int nY, int nFont, Color nColor, const std::wstring& sText)
{
    SwitchFont(nFont);

    if (nColor != m_nCurrentTextColor)
    {
        m_nCurrentTextColor = nColor;
        SetTextColor(m_hDC, nColor.ARGB);
    }

    SetBkMode(m_hDC, TRANSPARENT);
    TextOutW(m_hDC, nX, nY, sText.c_str(), sText.length());
}

void GDISurface::SwitchFont(int nFont) const
{
    if (nFont != m_nCurrentFont)
    {
        if (nFont > 0 && ra::to_unsigned(nFont) <= m_vFonts.size())
        {
            m_nCurrentFont = nFont;
            SelectFont(m_hDC, m_vFonts.at(nFont - 1).hFont);
        }
    }
}

} // namespace gdi
} // namespace drawing
} // namespace ui
} // namespace ra
