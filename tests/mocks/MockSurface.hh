#ifndef RA_SERVICES_MOCK_SURFACE_HH
#define RA_SERVICES_MOCK_SURFACE_HH
#pragma once

#include "ra_fwd.h"

#include "ui\drawing\ISurface.hh"

namespace ra {
namespace ui {
namespace drawing {
namespace mocks {

class MockSurface : public ISurface
{
public:
    MockSurface(unsigned int nWidth, unsigned int nHeight) noexcept
        : m_nWidth(nWidth), m_nHeight(nHeight)
    {
    }

    unsigned int GetWidth() const noexcept override { return m_nWidth; }
    unsigned int GetHeight() const noexcept override { return m_nHeight; }

    void FillRectangle(int, int, int, int, Color) noexcept override {}
    int LoadFont(const std::string&, int, FontStyles) noexcept override { return 1; }

    ra::ui::Size MeasureText(int, const std::wstring& sText) const noexcept override
    {
        return { gsl::narrow_cast<int>(sText.length()), 1 };
    }

    void WriteText(int, int, int, Color, const std::wstring&) noexcept override {}
    void DrawImage(int, int, int, int, const ImageReference&) noexcept override {}
    void DrawImageStretched(int, int, int, int, const ImageReference&) noexcept override {}
    void DrawSurface(int, int, const ISurface&) noexcept override {}
    void DrawSurface(int, int, const ISurface&, int, int, int, int) noexcept override {}
    void SetOpacity(double) noexcept override {}
    void SetPixels(int, int, int, int, uint32_t*) noexcept override {}

private:
    unsigned int m_nWidth;
    unsigned int m_nHeight;
};

class MockSurfaceFactory : public ISurfaceFactory
{
public:
    MockSurfaceFactory() noexcept : m_Override(this) {}

    std::unique_ptr<ISurface> CreateSurface(int nWidth, int nHeight) const override
    {
        return std::make_unique<MockSurface>(nWidth, nHeight);
    }

    std::unique_ptr<ISurface> CreateTransparentSurface(int nWidth, int nHeight) const override
    {
        return std::make_unique<MockSurface>(nWidth, nHeight);
    }

    bool SaveImage(const ISurface&, const std::wstring&) const noexcept override
    {
        return false;
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ISurfaceFactory> m_Override;
};

} // namespace mocks
} // namespace drawing
} // namespace ui
} // namespace ra

#endif // !RA_SERVICES_MOCK_SURFACE_HH
