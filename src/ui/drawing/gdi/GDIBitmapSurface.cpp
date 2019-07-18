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
    assert(m_pBits != nullptr);
    auto pBits = m_pBits + nStride * nFirstScanline + nX;
    assert(pBits != nullptr);

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

static constexpr uint8_t BlendPixel(std::uint8_t nTarget, std::uint8_t nBlend, std::uint8_t nAlpha)
{
    return gsl::narrow_cast<std::uint8_t>(((nBlend * nAlpha) + (nTarget * (256 - nAlpha))) / 256);
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
    SetTextColor(hMemDC, RGB(255, 255, 255));
    SetBkMode(hMemDC, TRANSPARENT);
    RECT rcRect{0, 0, szText.cx, szText.cy};
    TextOutW(hMemDC, 0, 0, sText.c_str(), sText.length());

    // copy the greyscale text to the forground using the grey value as the alpha for antialiasing
    auto nFirstScanline = (GetHeight() - nY - szText.cy); // bitmap memory starts with the bottom scanline
    assert(ra::to_signed(nFirstScanline) >= 0);
    assert(m_pBits != nullptr);
    auto pBits = m_pBits + nStride * nFirstScanline + nX;
    assert(pBits != nullptr);

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
            const ra::ui::Color nTextColor(*pTextBits++);
            if (nTextColor.Channel.R == 255)
            {
                *pBits = nColor.ARGB;
            }
            else if (nTextColor.Channel.R != 0)
            {
                const uint8_t nAlpha = nTextColor.Channel.R;
                ra::ui::Color nImageColor(*pBits);
                nImageColor.Channel.R = BlendPixel(nImageColor.Channel.R, nColor.Channel.R, nAlpha);
                nImageColor.Channel.G = BlendPixel(nImageColor.Channel.G, nColor.Channel.G, nAlpha);
                nImageColor.Channel.B = BlendPixel(nImageColor.Channel.B, nColor.Channel.B, nAlpha);
                *pBits = nImageColor.ARGB;
            }

            ++pBits;
        } while (pBits < pEnd);

        pBits += (nStride - szText.cx);
    }

    DeleteBitmap(hBitmap);
    DeleteDC(hMemDC);
}

#pragma warning(push)
#pragma warning(disable : 5045)

void GDIAlphaBitmapSurface::Blend(HDC hTargetDC, int nX, int nY) const noexcept
{
    const auto nWidth = to_signed(GetWidth());
    const auto nHeight = to_signed(GetHeight());

    // copy a portion of the target surface into a buffer
    ::BitBlt(m_pBlendBuffer.m_hMemDC, 0, 0, nWidth, nHeight, hTargetDC, nX, nY, SRCCOPY);

    // merge the current surface onto the buffer - they'll both be the same size, so bulk process it
    std::uint8_t* pBits;
    GSL_SUPPRESS_TYPE1 pBits = reinterpret_cast<std::uint8_t*>(m_pBlendBuffer.m_pBits);
    assert(pBits != nullptr);
    std::uint8_t* pSrcBits;
    GSL_SUPPRESS_TYPE1 pSrcBits = reinterpret_cast<std::uint8_t*>(m_pBits);
    assert(pSrcBits != nullptr);

    const std::uint8_t* pEnd = pSrcBits + (nWidth * nHeight * 4);
    while (pSrcBits < pEnd)
    {
        const auto nAlpha = pSrcBits[3];
        if (nAlpha == 0)
        {
            // ignore fully transparent pixels
        }
        else if (nAlpha == 0xFF)
        {
            // copy fully opaque pixels
            pBits[0] = pSrcBits[0];
            pBits[1] = pSrcBits[1];
            pBits[2] = pSrcBits[2];
        }
        else
        {
            // merge partially transparent pixels
            pBits[0] = BlendPixel(pBits[0], pSrcBits[0], nAlpha);
            pBits[1] = BlendPixel(pBits[1], pSrcBits[1], nAlpha);
            pBits[2] = BlendPixel(pBits[2], pSrcBits[2], nAlpha);
        }

        pBits += 4;
        pSrcBits += 4;
    }

    // copy the buffer back onto the target surface
    ::BitBlt(hTargetDC, nX, nY, nWidth, nHeight, m_pBlendBuffer.m_hMemDC, 0, 0, SRCCOPY);
}

#pragma warning(pop)

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
