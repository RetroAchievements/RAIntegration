#include "GDIBitmapSurface.hh"

#include "ui\drawing\gdi\ImageRepository.hh"

namespace ra {
namespace ui {
namespace drawing {
namespace gdi {

void GDIAlphaBitmapSurface::FillRectangle(int nX, int nY, int nWidth, int nHeight, Color nColor) noexcept
{
    assert(nWidth != 0);
    assert(nHeight != 0);

    auto nStride = ra::to_signed(GetWidth());
    auto nFirstScanline = (GetHeight() - nY - nHeight); // bitmap memory starts with the bottom scanline
    auto pBits = m_pBits + nStride * nFirstScanline + nX;
    if (nStride == nWidth)
    {
        // doing full scanlines, just bulk fill
        const UINT32* pEnd = pBits + nWidth * nHeight;
        do
        {
            *pBits++ = nColor.ARGB;
        } while (pBits < pEnd);
    }
    else
    {
        // partial scanlines, have to fill in strips
        while (nHeight--)
        {
            const UINT32* pEnd = pBits + nWidth;
            do
            {
                *pBits++ = nColor.ARGB;
            } while (pBits < pEnd);

            pBits += (nStride - nWidth);
        }
    }
}

static constexpr void BlendPixel(UINT32* nTarget, UINT32 nBlend) noexcept
{
    const auto alpha = nBlend >> 24;

    // fully transparent - do nothing
    if (alpha == 0)
        return;

    // fully opaque, use the blend value
    if (alpha == 255)
    {
        *nTarget = nBlend;
        return;
    }

    // blend each of the RGB values based on the blend pixel's alpha value
    // do not modify the target pixel's alpha value.
    UINT8* pTarget = reinterpret_cast<UINT8*>(nTarget);
    UINT8* pBlend = reinterpret_cast<UINT8*>(&nBlend);

    *pTarget++ = static_cast<UINT8>(
        (static_cast<UINT32>(*pBlend++) * alpha + static_cast<UINT32>(*pTarget) * (256 - alpha)) / 256);
    *pTarget++ = static_cast<UINT8>(
        (static_cast<UINT32>(*pBlend++) * alpha + static_cast<UINT32>(*pTarget) * (256 - alpha)) / 256);
    *pTarget = static_cast<UINT8>(
        (static_cast<UINT32>(*pBlend) * alpha + static_cast<UINT32>(*pTarget) * (256 - alpha)) / 256);
}

void GDIAlphaBitmapSurface::WriteText(int nX, int nY, int nFont, Color nColor, const std::wstring& sText)
{
    if (sText.empty())
        return;

    SwitchFont(nFont);

    // writing directly to the surface results in 0 for all alpha values - i.e. transparent text.
    // instead, draw white text on black background to get grayscale anti-alias values which will
    // be used as the alpha channel for applying the text to the surface.
    SIZE szText;
    GetTextExtentPoint32W(m_hDC, sText.c_str(), sText.length(), &szText);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = szText.cx;
    bmi.bmiHeader.biHeight = szText.cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;

    HDC hMemDC = CreateCompatibleDC(m_hDC);

    UINT32* pTextBits{};
    HBITMAP hBitmap = CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, reinterpret_cast<LPVOID*>(&pTextBits), nullptr, 0);
    assert(hBitmap != nullptr);
    assert(pTextBits != nullptr);
    SelectBitmap(hMemDC, hBitmap);

    SelectFont(hMemDC, m_pResourceRepository.GetHFont(nFont));
    SetTextColor(hMemDC, 0xFFFFFFFF);
    RECT rcRect{0, 0, szText.cx, szText.cy};
    TextOutW(hMemDC, 0, 0, sText.c_str(), sText.length());

    // copy the greyscale text to the forground using the grey value as the alpha for antialiasing
    auto nStride = GetWidth();
    auto nFirstScanline = (GetHeight() - nY - szText.cy); // bitmap memory starts with the bottom scanline
    assert(ra::to_signed(nFirstScanline) >= 0);
    auto pBits = m_pBits + nStride * nFirstScanline + nX;

    auto nLines = szText.cy;
    while (nLines--)
    {
        const UINT32* pEnd = pBits + szText.cx;
        do
        {
            const UINT8 nAlpha = 0xFF - ((*pTextBits++) & 0xFF);
            const UINT32 pColor = (nColor.ARGB & 0x00FFFFFF) | ((nAlpha * nColor.Channel.A / 255) << 24);
            BlendPixel(pBits, pColor);
            ++pBits;
        } while (pBits < pEnd);

        pBits += (nStride - szText.cx);
    }

    DeleteBitmap(hBitmap);
    DeleteDC(hMemDC);
}

void GDIAlphaBitmapSurface::Blend(HDC hTargetDC, int nX, int nY) const noexcept
{
    const int nWidth = static_cast<int>(GetWidth());
    const int nHeight = static_cast<int>(GetHeight());

    // copy a portion of the target surface into a buffer
    HDC hMemDC = CreateCompatibleDC(hTargetDC);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = nWidth;
    bmi.bmiHeader.biHeight = nHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = nWidth * nHeight * 4;

    UINT32* pBits;
    HBITMAP hBitmap = CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, reinterpret_cast<LPVOID*>(&pBits), nullptr, 0);
    Expects(hBitmap != nullptr);
    SelectBitmap(hMemDC, hBitmap);

    BitBlt(hMemDC, 0, 0, nWidth, nHeight, hTargetDC, nX, nY, SRCCOPY);

    // merge the current surface onto the buffer - they'll both be the same size, so bulk process it
    const UINT32* pEnd = pBits + nWidth * nHeight;
    UINT32* pSrcBits = m_pBits;
    do
    {
        BlendPixel(pBits, *pSrcBits++);
    } while (++pBits < pEnd);

    // copy the buffer back onto the target surface
    ::BitBlt(hTargetDC, nX, nY, nWidth, nHeight, hMemDC, 0, 0, SRCCOPY);

    DeleteBitmap(hBitmap);
    DeleteDC(hMemDC);
}

void GDIAlphaBitmapSurface::SetOpacity(double fAlpha) noexcept
{
    assert(fAlpha >= 0.0 && fAlpha <= 1.0);
    const auto nAlpha = static_cast<UINT8>(255 * fAlpha);
    assert(nAlpha > 0.0); // setting opacity to 0 is irreversible - caller should just not draw it

    const UINT8* pEnd = reinterpret_cast<UINT8*>(m_pBits + GetWidth() * GetHeight());
    UINT8* pBits = reinterpret_cast<UINT8*>(m_pBits) + 3;
    do
    {
        // only update the alpha for non-transparent pixels
        if (*pBits)
            *pBits = nAlpha;

        pBits += 4;
    } while (pBits < pEnd);
}

} // namespace gdi
} // namespace drawing
} // namespace ui
} // namespace ra
