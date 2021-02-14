#include "ChallengeIndicatorViewModel.hh"

#include "ra_math.h"

#include "ui\OverlayTheme.hh"
#include "ui\viewmodels\OverlayManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

ChallengeIndicatorViewModel::ChallengeIndicatorViewModel() noexcept
{
    GSL_SUPPRESS_F6 SetPopupType(ra::ui::viewmodels::Popup::Challenge);

    GSL_SUPPRESS_F6 SetHorizontalOffset(10);
    GSL_SUPPRESS_F6 SetVerticalOffset(10);
}

bool ChallengeIndicatorViewModel::UpdateRenderImage(_UNUSED double fElapsed)
{
    if (m_pSurface)
        return false;

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

    return true;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
