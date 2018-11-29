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

    size_t GetWidth() const override { return m_nWidth; }
    size_t GetHeight() const override { return m_nHeight; }

    void FillRectangle(int nX, int nY, int nWidth, int nHeight, Color nColor) override;

    int LoadFont(const std::string& sFont, int nFontSize, FontStyles nStyle) override;
    ra::ui::Size MeasureText(int nFont, const std::wstring& sText) const override;
    void WriteText(int nX, int nY, int nFont, Color nColor, const std::wstring& sText) override;

    void DrawImage(int nX, int nY, int nWidth, int nHeight, const ImageReference& pImage) override;
    void DrawSurface(int nX, int nY, const ISurface& pSurface) override;

    void SetOpacity(_UNUSED double) override { assert("This surface does not support opacity"); }

protected:
    void SwitchFont(int nFont) const;

    HDC m_hDC{};

    ResourceRepository& m_pResourceRepository;

private:
    int m_nWidth{};
    int m_nHeight{};

    ResourceRepository m_oResourceRepository;

    mutable int m_nCurrentFont{};
    Color m_nCurrentTextColor{0U};
};

} // namespace gdi
} // namespace drawing
} // namespace ui
} // namespace ra

#endif RA_UI_DRAWING_GDI_SURFACE_H
