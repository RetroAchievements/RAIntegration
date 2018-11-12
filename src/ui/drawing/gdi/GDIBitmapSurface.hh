#ifndef RA_UI_DRAWING_GDI_BITMAP_SURFACE_H
#define RA_UI_DRAWING_GDI_BITMAP_SURFACE_H
#pragma once

#include "RA_Core.h"

#include "ui\drawing\gdi\GDISurface.hh"
#include "ui\drawing\gdi\ResourceRepository.hh"

namespace ra {
namespace ui {
namespace drawing {
namespace gdi {

class GDIBitmapSurface : public GDISurface
{
public:
    explicit GDIBitmapSurface(const int nWidth, const int nHeight, ResourceRepository& pResourceRepository) noexcept
        : GDISurface(CreateBitmapHDC(nWidth, nHeight), RECT{ 0, 0, nWidth, nHeight }, pResourceRepository)
    {
    }

    explicit GDIBitmapSurface(const int nWidth, const int nHeight) noexcept
        : GDISurface(CreateBitmapHDC(nWidth, nHeight), RECT { 0, 0, nWidth, nHeight })
    {
    }

    GDIBitmapSurface(const GDIBitmapSurface&) noexcept = delete;
    GDIBitmapSurface& operator=(const GDIBitmapSurface&) noexcept = delete;
    GDIBitmapSurface(GDIBitmapSurface&&) noexcept = delete;
    GDIBitmapSurface& operator=(GDIBitmapSurface&&) noexcept = delete;

    ~GDIBitmapSurface()
    {
        if (m_hBitmap)
            DeleteBitmap(m_hBitmap);
        if (m_hMemDC)
            DeleteDC(m_hMemDC);
    }

protected:
    UINT32* m_pBits;

private:
    HDC CreateBitmapHDC(const int nWidth, const int nHeight)
    {
        BITMAPINFO bmi{};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = nWidth;
        bmi.bmiHeader.biHeight = nHeight;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        bmi.bmiHeader.biSizeImage = nWidth * nHeight * 4;

        HDC hWndDC = GetDC(g_RAMainWnd);
        m_hMemDC = CreateCompatibleDC(hWndDC);
        ReleaseDC(g_RAMainWnd, hWndDC);

        LPVOID pBits{};
        m_hBitmap = CreateDIBSection(m_hMemDC, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);
        m_pBits = reinterpret_cast<UINT32*>(pBits);

        SelectObject(m_hMemDC, m_hBitmap);
        return m_hMemDC;
    }

    HBITMAP m_hBitmap{};
    HDC m_hMemDC{};
};

class GDIAlphaBitmapSurface : public GDIBitmapSurface
{
public:
    explicit GDIAlphaBitmapSurface(const int nWidth, const int nHeight, ResourceRepository& pResourceRepository) noexcept
        : GDIBitmapSurface(nWidth, nHeight, pResourceRepository)
    {
    }

    explicit GDIAlphaBitmapSurface(const int nWidth, const int nHeight) noexcept
        : GDIBitmapSurface(nWidth, nHeight)
    {
    }

    GDIAlphaBitmapSurface(const GDIAlphaBitmapSurface&) noexcept = delete;
    GDIAlphaBitmapSurface& operator=(const GDIAlphaBitmapSurface&) noexcept = delete;
    GDIAlphaBitmapSurface(GDIAlphaBitmapSurface&&) noexcept = delete;
    GDIAlphaBitmapSurface& operator=(GDIAlphaBitmapSurface&&) noexcept = delete;

    void FillRectangle(int nX, int nY, int nWidth, int nHeight, Color nColor) override;
    void WriteText(int nX, int nY, int nFont, Color nColor, const std::wstring& sText) override;

    void Blend(HDC hTargetDC, int nX, int nY) const;
    
    /// <summary>
    /// Sets the alpha value of anything that's not currently transparent to the specified value.
    /// </summary>
    /// <param name="nAlpha">The new opacity (1-255).</param>
    void SetOpacity(UINT8 nAlpha);
};

} // namespace gdi
} // namespace drawing
} // namespace ui
} // namespace ra

#endif RA_UI_DRAWING_GDI_SURFACE_H
