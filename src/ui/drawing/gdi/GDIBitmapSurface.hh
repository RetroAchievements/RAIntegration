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
    GSL_SUPPRESS_TYPE6 // m_hBitmap not initialized, adhered to note below
    explicit GDIBitmapSurface(const int nWidth, const int nHeight, ResourceRepository& pResourceRepository) noexcept :
        GDISurface(CreateBitmapHDC(nWidth, nHeight), RECT{0, 0, nWidth, nHeight}, pResourceRepository)
    {}

    GSL_SUPPRESS_TYPE6 // m_hBitmap not initialized, adhered to note below
    explicit GDIBitmapSurface(const int nWidth, const int nHeight) noexcept :
        GDISurface(CreateBitmapHDC(nWidth, nHeight), RECT{0, 0, nWidth, nHeight})
    {}

    GDIBitmapSurface(const GDIBitmapSurface&) noexcept = delete;
    GDIBitmapSurface& operator=(const GDIBitmapSurface&) noexcept = delete;
    GDIBitmapSurface(GDIBitmapSurface&&) noexcept = delete;
    GDIBitmapSurface& operator=(GDIBitmapSurface&&) noexcept = delete;

    ~GDIBitmapSurface() noexcept
    {
        if (m_hBitmap)
            DeleteBitmap(m_hBitmap);
        if (m_hMemDC)
            DeleteDC(m_hMemDC);
    }

protected:
    std::uint32_t* m_pBits; // see note below about initializing this

private:
    HDC CreateBitmapHDC(const int nWidth, const int nHeight) noexcept
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
        assert(m_hBitmap != nullptr);
        assert(pBits != nullptr);
        m_pBits = static_cast<std::uint32_t*>(pBits);

        SelectObject(m_hMemDC, m_hBitmap);
        return m_hMemDC;
    }

    // IMPORTANT: m_hBitmap, m_hMemDC, and m_pBits are set by CreateBitmapHDC, which is called from the constructor.
    // explicitly setting an initial value causes the constructor calculated values to be overridden.
    HBITMAP m_hBitmap;
    HDC m_hMemDC;
};

class GDIAlphaBitmapSurface : public GDIBitmapSurface
{
public:
    explicit GDIAlphaBitmapSurface(const int nWidth, const int nHeight,
                                   ResourceRepository& pResourceRepository) noexcept :
        GDIBitmapSurface(nWidth, nHeight, pResourceRepository)
    {}

    explicit GDIAlphaBitmapSurface(const int nWidth, const int nHeight) noexcept : GDIBitmapSurface(nWidth, nHeight) {}

    GDIAlphaBitmapSurface(const GDIAlphaBitmapSurface&) noexcept = delete;
    GDIAlphaBitmapSurface& operator=(const GDIAlphaBitmapSurface&) noexcept = delete;
    GDIAlphaBitmapSurface(GDIAlphaBitmapSurface&&) noexcept = delete;
    GDIAlphaBitmapSurface& operator=(GDIAlphaBitmapSurface&&) noexcept = delete;

    void FillRectangle(int nX, int nY, int nWidth, int nHeight, Color nColor) noexcept override;
    void WriteText(int nX, int nY, int nFont, Color nColor, const std::wstring& sText) override;

    void Blend(HDC hTargetDC, int nX, int nY) const;

    void SetOpacity(double fAlpha) override;
};

class GDISurfaceFactory : public ISurfaceFactory
{
public:
    std::unique_ptr<ISurface> CreateSurface(int nWidth, int nHeight) const override
    {
        auto pSurface = std::make_unique<GDIBitmapSurface>(nWidth, nHeight, m_oResourceRepository);
        return std::unique_ptr<ISurface>(pSurface.release());
    }

    std::unique_ptr<ISurface> CreateTransparentSurface(int nWidth, int nHeight) const override
    {
        auto pSurface = std::make_unique<GDIAlphaBitmapSurface>(nWidth, nHeight, m_oResourceRepository);
        return std::unique_ptr<ISurface>(pSurface.release());
    }

private:
    mutable ResourceRepository m_oResourceRepository;
};

} // namespace gdi
} // namespace drawing
} // namespace ui
} // namespace ra

#endif RA_UI_DRAWING_GDI_SURFACE_H
