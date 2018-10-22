#include "GDISurface.hh"

#include "ui\drawing\gdi\ImageRepository.hh"

namespace ra {
namespace ui {
namespace drawing {
namespace gdi {

GDISurface::GDISurface(HDC hDC, RECT& rcDEST, ResourceRepository& pResourceRepository) noexcept
    : m_hDC(hDC), m_nWidth(rcDEST.right - rcDEST.left), m_nHeight(rcDEST.bottom - rcDEST.top), m_pResourceRepository(pResourceRepository)
{
    SelectObject(hDC, GetStockObject(DC_PEN));
    SelectObject(hDC, GetStockObject(DC_BRUSH));
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
    return m_pResourceRepository.LoadFont(sFont, nFontSize, nStyle);
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
        auto hFont = m_pResourceRepository.GetHFont(nFont);
        if (hFont)
        {
            m_nCurrentFont = nFont;
            SelectFont(m_hDC, hFont);
        }
    }
}

void GDISurface::DrawImage(int nX, int nY, int nWidth, int nHeight, const ImageReference& pImage)
{
    auto hBitmap = ImageRepository::GetHBitmap(pImage);
    if (!hBitmap)
        return;

    HDC hdcMem = CreateCompatibleDC(m_hDC);
    if (!hdcMem)
        return;

    auto hOldBitmap = SelectBitmap(hdcMem, hBitmap);

    BITMAP bm;
    if (GetObject(hBitmap, sizeof(bm), &bm) == sizeof(bm))
        BitBlt(m_hDC, nX, nY, nWidth, nHeight, hdcMem, 0, 0, SRCCOPY);

    SelectBitmap(hdcMem, hOldBitmap);

    DeleteDC(hdcMem);
}

} // namespace gdi
} // namespace drawing
} // namespace ui
} // namespace ra
