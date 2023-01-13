#include "ProgressTrackerViewModel.hh"

#include "ra_math.h"

#include "ui\OverlayTheme.hh"
#include "ui\viewmodels\OverlayManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

ProgressTrackerViewModel::ProgressTrackerViewModel() noexcept
{
    GSL_SUPPRESS_F6 SetPopupType(ra::ui::viewmodels::Popup::Progress);

    GSL_SUPPRESS_F6 SetHorizontalOffset(10);
    GSL_SUPPRESS_F6 SetVerticalOffset(10);
}

bool ProgressTrackerViewModel::UpdateRenderImage(double fElapsed)
{
    if (m_fAnimationProgress >= TOTAL_ANIMATION_TIME)
        return false;

    m_fAnimationProgress += fElapsed;

    bool bUpdated = (m_fAnimationProgress >= HOLD_TIME);

    if (m_pSurface == nullptr)
    {
        const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
        const auto nShadowOffset = pTheme.ShadowOffset();
        constexpr int nImageSize = 32;

        const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
        m_pSurface = pSurfaceFactory.CreateSurface(nImageSize + nShadowOffset, nImageSize + nShadowOffset);

        // background
        m_pSurface->FillRectangle(0, 0, m_pSurface->GetWidth(), m_pSurface->GetHeight(), Color::Transparent);
        m_pSurface->FillRectangle(nShadowOffset, nShadowOffset, nImageSize, nImageSize, pTheme.ColorShadow());

        // image
        if (m_hImage.Type() != ra::ui::ImageType::None)
            m_pSurface->DrawImageStretched(0, 0, nImageSize, nImageSize, m_hImage);

        if (m_hImageGrayscale.Type() != ra::ui::ImageType::None)
        {
            std::unique_ptr<ra::ui::drawing::ISurface> pGrayscaleSurface;
            pGrayscaleSurface = pSurfaceFactory.CreateSurface(nImageSize, nImageSize);
            pGrayscaleSurface->DrawImageStretched(0, 0, nImageSize, nImageSize, m_hImageGrayscale);

            const auto nGraySize = (int)((1.0 - m_fProgress) * nImageSize);
            m_pSurface->DrawSurface(0, 0, *pGrayscaleSurface, 0, 0, nImageSize, nGraySize);
        }

        bUpdated = true;
    }

    return bUpdated;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
