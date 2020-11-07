#include "ScoreTrackerViewModel.hh"

#include "ra_math.h"

#include "ui\OverlayTheme.hh"
#include "ui\viewmodels\OverlayManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty ScoreTrackerViewModel::DisplayTextProperty("ScoreTrackerViewModel", "DisplayText", L"0");

ScoreTrackerViewModel::ScoreTrackerViewModel() noexcept
{
    GSL_SUPPRESS_F6 SetPopupType(ra::ui::viewmodels::Popup::LeaderboardTracker);

    GSL_SUPPRESS_F6 SetHorizontalOffset(10);
    GSL_SUPPRESS_F6 SetVerticalOffset(10);
}

void ScoreTrackerViewModel::SetDisplayText(const std::wstring& sValue)
{
    if (sValue != GetDisplayText())
    {
        SetValue(DisplayTextProperty, sValue);

        m_bSurfaceStale = true;
        ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().RequestRender();
    }
}

bool ScoreTrackerViewModel::UpdateRenderImage(_UNUSED double fElapsed)
{
    if (m_pSurface && !m_bSurfaceStale)
        return false;

    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
    const auto nShadowOffset = pTheme.ShadowOffset();

    // create a temporary surface so we can determine the size required for the actual surface
    const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
    auto pTempSurface = pSurfaceFactory.CreateSurface(1, 1);

    const auto nFontText = pTempSurface->LoadFont(pTheme.FontPopup(), pTheme.FontSizePopupLeaderboardTracker(), ra::ui::FontStyles::Normal);

    const auto sScoreSoFar = GetDisplayText();
    const auto szScoreSoFar = pTempSurface->MeasureText(nFontText, sScoreSoFar);

    m_pSurface = pSurfaceFactory.CreateSurface(szScoreSoFar.Width + 8 + nShadowOffset, szScoreSoFar.Height + nShadowOffset);

    // background
    m_pSurface->FillRectangle(0, 0, m_pSurface->GetWidth(), m_pSurface->GetHeight(), Color::Transparent);
    m_pSurface->FillRectangle(nShadowOffset, nShadowOffset, m_pSurface->GetWidth() - nShadowOffset, m_pSurface->GetHeight() - nShadowOffset, pTheme.ColorShadow());

    // frame
    m_pSurface->FillRectangle(nShadowOffset, nShadowOffset, szScoreSoFar.Width + 8,
        szScoreSoFar.Height, pTheme.ColorShadow());
    m_pSurface->FillRectangle(0, 0, szScoreSoFar.Width + 8, szScoreSoFar.Height, pTheme.ColorBackground());

    // text
    m_pSurface->WriteText(4, 0, nFontText, pTheme.ColorLeaderboardEntry(), sScoreSoFar);

    m_bSurfaceStale = false;
    return true;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
