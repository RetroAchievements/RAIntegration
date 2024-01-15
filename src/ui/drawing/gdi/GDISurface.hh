#ifndef RA_UI_DRAWING_GDI_SURFACE_H
#define RA_UI_DRAWING_GDI_SURFACE_H
#pragma once

#include "ui\drawing\ISurface.hh"
#include "ui\drawing\gdi\ResourceRepository.hh"

namespace ra {
namespace ui {
namespace drawing {
namespace gdi {

class GDISurface : public ISurface
{
public:
    explicit GDISurface(HDC hDC, const RECT& rcDEST, ResourceRepository& pResourceRepository) noexcept;

    explicit GDISurface(HDC hDC, const RECT& rcDEST) noexcept : GDISurface(hDC, rcDEST, m_oResourceRepository) {}

    GDISurface(const GDISurface&) noexcept = delete;
    GDISurface& operator=(const GDISurface&) noexcept = delete;
    GDISurface(GDISurface&&) noexcept = delete;
    GDISurface& operator=(GDISurface&&) noexcept = delete;

    ~GDISurface() noexcept = default;

    unsigned int GetWidth() const noexcept override { return m_nWidth; }
    unsigned int GetHeight() const noexcept override { return m_nHeight; }

    void FillRectangle(int nX, int nY, int nWidth, int nHeight, Color nColor) noexcept override;

    int LoadFont(const std::string& sFont, int nFontSize, FontStyles nStyle) override;
    ra::ui::Size MeasureText(int nFont, const std::wstring& sText) const override;
    void WriteText(int nX, int nY, int nFont, Color nColor, const std::wstring& sText) override;

    void DrawImage(int nX, int nY, int nWidth, int nHeight, const ImageReference& pImage) override;
    void DrawImageStretched(int nX, int nY, int nWidth, int nHeight, const ImageReference& pImage) override;
    void DrawSurface(int nX, int nY, const ISurface& pSurface) override;
    void DrawSurface(int nX, int nY, const ISurface& pSurface, int nSurfaceX, int nSurfaceY, int nWidth, int nHeight) noexcept override;

    GSL_SUPPRESS_F6 void SetOpacity(_UNUSED double) override { assert("This surface does not support opacity"); }
    GSL_SUPPRESS_F6 void SetPixels(_UNUSED int, _UNUSED int, _UNUSED int, _UNUSED int, _UNUSED uint32_t*) override
    {
        assert("This surface does not support setting pixels");
    }

    HDC GetHDC() const noexcept { return m_hDC; }

protected:
    void SwitchFont(int nFont) const;

    HDC m_hDC{};

    ResourceRepository& m_pResourceRepository;

private:
    unsigned int m_nWidth{};
    unsigned int m_nHeight{};

    ResourceRepository m_oResourceRepository;

    mutable int m_nCurrentFont{};
    Color m_nCurrentTextColor{0U};
};

} // namespace gdi
} // namespace drawing
} // namespace ui
} // namespace ra

#endif RA_UI_DRAWING_GDI_SURFACE_H
