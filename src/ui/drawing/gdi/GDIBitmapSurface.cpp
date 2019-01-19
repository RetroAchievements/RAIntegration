#include "GDIBitmapSurface.hh"

#include "ui\drawing\gdi\ImageRepository.hh"

namespace ra {
namespace ui {
namespace drawing {
namespace gdi {

void GDIAlphaBitmapSurface::FillRectangle(int nX, int nY, int nWidth, int nHeight, Color nColor) noexcept
{
    // clip to surface
    auto nStride = ra::to_signed(GetWidth());
    if (nX >= nStride || nY >= ra::to_signed(GetHeight()))
        return;

    if (nX < 0)
    {
        nWidth += nX;
        nX = 0;
    }
    if (nWidth > nStride - nX)
        nWidth = nStride - nX;
    if (nWidth <= 0)
        return;

    if (nY < 0)
    {
        nHeight += nY;
        nY = 0;
    }
    const auto nMaxHeight = ra::to_signed(GetHeight()) - nY;
    if (nHeight > nMaxHeight)
        nHeight = nMaxHeight;
    if (nHeight <= 0)
        return;

    // fill rectangle
    auto nFirstScanline = (GetHeight() - nY - nHeight); // bitmap memory starts with the bottom scanline
    auto pBits = m_pBits + nStride * nFirstScanline + nX;
    if (nStride == nWidth)
    {
        // doing full scanlines, just bulk fill
        const std::uint32_t* pEnd = pBits + nWidth * nHeight;
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
            const std::uint32_t* pEnd = pBits + nWidth;
            do
            {
                *pBits++ = nColor.ARGB;
            } while (pBits < pEnd);

            pBits += (nStride - nWidth);
        }
    }
}

static constexpr void BlendPixel(gsl::not_null<std::uint32_t* restrict> nTarget, std::uint32_t nBlend) noexcept
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

    std::uint8_t* pTarget{};
    GSL_SUPPRESS_TYPE1 pTarget = reinterpret_cast<std::uint8_t*>(nTarget.get());
    Expects(pTarget != nullptr);

    std::uint8_t* pBlend{};
    GSL_SUPPRESS_TYPE1 pBlend = reinterpret_cast<std::uint8_t*>(&nBlend);
    Expects(pBlend != nullptr);


    *pTarget++ = gsl::narrow_cast<std::uint8_t>(
        (std::uint32_t{*pBlend++} * alpha + std::uint32_t{*pTarget} * (256 - alpha)) / 256);
    *pTarget++ = gsl::narrow_cast<std::uint8_t>(
        (std::uint32_t{*pBlend++} * alpha + std::uint32_t{*pTarget} * (256 - alpha)) / 256);
    *pTarget = gsl::narrow_cast<std::uint8_t>(
        (std::uint32_t{*pBlend} * alpha + std::uint32_t{*pTarget} * (256 - alpha)) / 256);
}

void GDIAlphaBitmapSurface::WriteText(int nX, int nY, int nFont, Color nColor, const std::wstring& sText)
{
    if (sText.empty())
        return;

    // clip to surface
    auto nStride = ra::to_signed(GetWidth());
    if (nX >= nStride || nY >= ra::to_signed(GetHeight()))
        return;
    assert(nX >= 0); // TODO: support negative X starting position

    // measure the text to draw
    SwitchFont(nFont);

    SIZE szText;
    GetTextExtentPoint32W(m_hDC, sText.c_str(), sText.length(), &szText);

    // clip to surface
    if (szText.cx > nStride - nX)
        szText.cx = nStride - nX;
    const auto nMaxHeight = ra::to_signed(GetHeight()) - nY;
    if (szText.cy > nMaxHeight)
        szText.cy = nMaxHeight;

    // writing directly to the surface results in 0 for all alpha values - i.e. transparent text.
    // instead, draw white text on black background to get grayscale anti-alias values which will
    // be used as the alpha channel for applying the text to the surface.
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = szText.cx;
    bmi.bmiHeader.biHeight = szText.cy;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = 0;

    HDC hMemDC = CreateCompatibleDC(m_hDC);

    std::uint32_t* pTextBits{};
    HBITMAP hBitmap{};
    GSL_SUPPRESS_TYPE1 hBitmap =
        CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, reinterpret_cast<LPVOID*>(&pTextBits), nullptr, 0);

    assert(hBitmap != nullptr);
    assert(pTextBits != nullptr);
    SelectBitmap(hMemDC, hBitmap);

    SelectFont(hMemDC, m_pResourceRepository.GetHFont(nFont));
    SetTextColor(hMemDC, 0xFFFFFFFF);
    RECT rcRect{0, 0, szText.cx, szText.cy};
    TextOutW(hMemDC, 0, 0, sText.c_str(), sText.length());

    // copy the greyscale text to the forground using the grey value as the alpha for antialiasing
    auto nFirstScanline = (GetHeight() - nY - szText.cy); // bitmap memory starts with the bottom scanline
    assert(ra::to_signed(nFirstScanline) >= 0);
    auto pBits = m_pBits + nStride * nFirstScanline + nX;

    // clip to surface
    auto nLines = szText.cy;
    if (nY < 0)
        nLines += nY;

    // copy bits
    while (nLines--)
    {
        const std::uint32_t* pEnd = pBits + szText.cx;
        do
        {
            const std::uint8_t nAlpha = 0xFF - ((*pTextBits++) & 0xFF);
            const std::uint32_t pColor = (nColor.ARGB & 0x00FFFFFF) | ((nAlpha * nColor.Channel.A / 255) << 24);
            BlendPixel(gsl::make_not_null(pBits), pColor);
            ++pBits;
        } while (pBits < pEnd);

        pBits += (nStride - szText.cx);
    }

    DeleteBitmap(hBitmap);
    DeleteDC(hMemDC);
}

void GDIAlphaBitmapSurface::Blend(HDC hTargetDC, int nX, int nY) const
{
    const auto nWidth = to_signed(GetWidth());
    const auto nHeight = to_signed(GetHeight());

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

    std::uint32_t* pBits{};

    HBITMAP hBitmap{};
    GSL_SUPPRESS_TYPE1 hBitmap =
        CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, reinterpret_cast<LPVOID*>(&pBits), nullptr, 0);
    Expects(hBitmap != nullptr);
    SelectBitmap(hMemDC, hBitmap);

    BitBlt(hMemDC, 0, 0, nWidth, nHeight, hTargetDC, nX, nY, SRCCOPY);

    // merge the current surface onto the buffer - they'll both be the same size, so bulk process it
    const std::uint32_t* pEnd = pBits + nWidth * nHeight;
    std::uint32_t* pSrcBits = m_pBits;
    Expects(pSrcBits != nullptr);
    do
    {
        BlendPixel(gsl::make_not_null(pBits), *pSrcBits++);
    } while (++pBits < pEnd);
    Ensures(pSrcBits != nullptr);

    // copy the buffer back onto the target surface
    ::BitBlt(hTargetDC, nX, nY, nWidth, nHeight, hMemDC, 0, 0, SRCCOPY);

    DeleteBitmap(hBitmap);
    DeleteDC(hMemDC);
}

void GDIAlphaBitmapSurface::SetOpacity(double fAlpha)
{
    assert(fAlpha >= 0.0 && fAlpha <= 1.0);
    const auto nAlpha = static_cast<std::uint8_t>(255 * fAlpha);
    Expects(nAlpha > 0.0); // setting opacity to 0 is irreversible - caller should just not draw it

    const std::uint8_t* pEnd{};
    GSL_SUPPRESS_TYPE1 pEnd = reinterpret_cast<const std::uint8_t*>(m_pBits + GetWidth() * GetHeight());

    std::uint8_t* pBits{};
    GSL_SUPPRESS_TYPE1 pBits = reinterpret_cast<std::uint8_t*>(m_pBits) + 3;

    Expects(pBits != nullptr);
    do
    {
        // only update the alpha for non-transparent pixels
        if (*pBits)
            *pBits = nAlpha;
        pBits += 4;
        Ensures(pBits != nullptr);
    } while (pBits < pEnd);
}

} // namespace gdi
} // namespace drawing
} // namespace ui
} // namespace ra
