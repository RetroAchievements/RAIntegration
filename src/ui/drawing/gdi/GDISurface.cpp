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

void GDISurface::FillRectangle(int nX, int nY, int nWidth, int nHeight, Color nColor) noexcept
{
    assert(nColor.Channel.A == 0xFF || nColor.Channel.A == 0x00);

    const COLORREF pColorRef = RGB(nColor.Channel.R, nColor.Channel.G, nColor.Channel.B);
    SetDCBrushColor(m_hDC, pColorRef);
    SetDCPenColor(m_hDC, pColorRef);
    Rectangle(m_hDC, nX, nY, nX + nWidth, nY + nHeight);
}

int GDISurface::LoadFont(const std::string& sFont, int nFontSize, FontStyles nStyle)
{
    return m_pResourceRepository.LoadFont(sFont, nFontSize, nStyle);
}

ra::ui::Size GDISurface::MeasureText(int nFont, const std::wstring& sText) const
{
    SwitchFont(nFont);

    SIZE szText;
    GetTextExtentPoint32W(m_hDC, sText.c_str(), gsl::narrow_cast<int>(sText.length()), &szText);

    return ra::ui::Size{ szText.cx, szText.cy };
}

void GDISurface::WriteText(int nX, int nY, int nFont, Color nColor, const std::wstring& sText)
{
    SwitchFont(nFont);

    if (nColor != m_nCurrentTextColor)
    {
        m_nCurrentTextColor = nColor;
        SetTextColor(m_hDC, RGB(nColor.Channel.R, nColor.Channel.G, nColor.Channel.B));
    }

    SetBkMode(m_hDC, TRANSPARENT);
    TextOutW(m_hDC, nX, nY, sText.c_str(), gsl::narrow_cast<int>(sText.length()));
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

void GDISurface::DrawImageStretched(int nX, int nY, int nWidth, int nHeight, const ImageReference& pImage)
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
    {
        SetStretchBltMode(m_hDC, HALFTONE);
        StretchBlt(m_hDC, nX, nY, nWidth, nHeight, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
    }

    SelectBitmap(hdcMem, hOldBitmap);

    DeleteDC(hdcMem);
}

void GDISurface::DrawSurface(int nX, int nY, const ISurface& pSurface)
{
    auto* pAlphaSurface = dynamic_cast<const GDIAlphaBitmapSurface*>(&pSurface);
    if (pAlphaSurface != nullptr)
    {
        pAlphaSurface->Blend(m_hDC, nX, nY);
        return;
    }

    auto* pGDISurface = dynamic_cast<const GDISurface*>(&pSurface);
    assert(pGDISurface != nullptr);

    if (pGDISurface != nullptr)
    {
        ::BitBlt(m_hDC, nX, nY,
            gsl::narrow_cast<int>(pSurface.GetWidth()), gsl::narrow_cast<int>(pSurface.GetHeight()),
            pGDISurface->m_hDC, 0, 0, SRCCOPY);
    }
}

void GDISurface::DrawSurface(int nX, int nY, const ISurface& pSurface, int nSurfaceX, int nSurfaceY, int nWidth, int nHeight) noexcept
{
    assert(dynamic_cast<const GDIAlphaBitmapSurface*>(&pSurface) == nullptr); // clipped alpha blend not currently supported

    auto* pGDISurface = dynamic_cast<const GDISurface*>(&pSurface);
    assert(pGDISurface != nullptr);
    if (pGDISurface != nullptr)
        ::BitBlt(m_hDC, nX, nY, nWidth, nHeight, pGDISurface->m_hDC, nSurfaceX, nSurfaceY, SRCCOPY);
}

} // namespace gdi
} // namespace drawing
} // namespace ui
} // namespace ra
