#include "GDISurface.hh"
#include "GDIBitmapSurface.hh"

#include "ui\drawing\gdi\ImageRepository.hh"

namespace ra {
namespace ui {
namespace drawing {
namespace gdi {

GDISurface::GDISurface(HDC hDC, const RECT& rcDEST, ResourceRepository& pResourceRepository) noexcept
    : m_hDC(hDC), m_nWidth(rcDEST.right - rcDEST.left), m_nHeight(rcDEST.bottom - rcDEST.top), m_pResourceRepository(pResourceRepository)
{
    SelectObject(hDC, GetStockObject(DC_PEN));
    SelectObject(hDC, GetStockObject(DC_BRUSH));
}

void GDISurface::FillRectangle(const Point& nXY, const Size& nWH, Color nColor) noexcept
{
    assert(nColor.Channel.A == 0xFF);
    SetDCBrushColor(m_hDC, RGB(nColor.Channel.R, nColor.Channel.G, nColor.Channel.B));
    SetDCPenColor(m_hDC, RGB(nColor.Channel.R, nColor.Channel.G, nColor.Channel.B));
    Rectangle(m_hDC, nXY.X, nXY.Y, nXY.X + nWH.Width, nXY.Y + nWH.Height);
}

gsl::index GDISurface::LoadFont(const std::string& sFont, int nFontSize, FontStyles nStyle)
{
    return m_pResourceRepository.LoadFont(sFont, nFontSize, nStyle);
}

ra::ui::Size GDISurface::MeasureText(gsl::index nFont, const std::wstring& sText) const
{
    SwitchFont(nFont);

    SIZE szText{};
    GetTextExtentPoint32W(m_hDC, sText.c_str(), gsl::narrow_cast<int>(sText.length()), &szText);

    return {szText.cx, szText.cy};
}

void GDISurface::WriteText(const Point& nXY, gsl::index nFont, Color nColor, const std::wstring& sText)
{
    SwitchFont(nFont);

    if (nColor != m_nCurrentTextColor)
    {
        m_nCurrentTextColor = nColor;
        SetTextColor(m_hDC, nColor.ARGB);
    }

    SetBkMode(m_hDC, TRANSPARENT);
    TextOutW(m_hDC, nXY.X, nXY.Y, sText.c_str(), gsl::narrow_cast<int>(sText.length()));
}

void GDISurface::SwitchFont(gsl::index nFont) const
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

void GDISurface::DrawImage(const Point& nXY, const Size& nWH, const ImageReference& pImage)
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
        BitBlt(m_hDC, nXY.X, nXY.Y, nWH.Width, nWH.Height, hdcMem, 0, 0, SRCCOPY);

    SelectBitmap(hdcMem, hOldBitmap);

    DeleteDC(hdcMem);
}

void GDISurface::DrawSurface(const Point& nXY, const ISurface& pSurface)
{
    auto* pAlphaSurface = dynamic_cast<const GDIAlphaBitmapSurface*>(&pSurface);

    if (pAlphaSurface != nullptr)
    {
        pAlphaSurface->Blend(m_hDC, nXY);
        return;
    }

    auto* pGDISurface = dynamic_cast<const GDISurface*>(&pSurface);
    assert(pGDISurface != nullptr);

    ::BitBlt(m_hDC, nXY.X, nXY.Y, pSurface.GetWidth(), pSurface.GetHeight(), pGDISurface->m_hDC, 0, 0, SRCCOPY);
}

} // namespace gdi
} // namespace drawing
} // namespace ui
} // namespace ra
