#include "ScoreTrackerViewModel.hh"

#include "ra_math.h"

#include "ui\OverlayTheme.hh"

namespace ra {
namespace ui {
namespace viewmodels {

static _CONSTANT_VAR FONT_TO_USE = "Tahoma";
static _CONSTANT_VAR FONT_SIZE_TEXT = 22;

const StringModelProperty ScoreTrackerViewModel::DisplayTextProperty("ScoreTrackerViewModel", "DisplayText", L"0");
#ifdef RA_UTEST
GSL_SUPPRESS_F6
#endif
bool ScoreTrackerViewModel::UpdateRenderImage(_UNUSED double fElapsed)
{
    if (m_pSurface && !m_bSurfaceStale)
    {
        const auto nOffset = GetRenderLocationY() + ra::to_signed(GetRenderImage().GetHeight());
        if (m_nOffset != nOffset)
        {
            SetRenderLocationY(m_nOffset + GetRenderImage().GetHeight());
            return true;
        }

        return false;
    }

#ifndef RA_UTEST
    // create a temporary surface so we can determine the size required for the actual surface
    const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
    auto pTempSurface = pSurfaceFactory.CreateSurface(1, 1);

    const auto nFontText = pTempSurface->LoadFont(FONT_TO_USE, FONT_SIZE_TEXT, ra::ui::FontStyles::Normal);

    const auto sScoreSoFar = GetDisplayText();
    const auto szScoreSoFar = pTempSurface->MeasureText(nFontText, sScoreSoFar);

    m_pSurface = pSurfaceFactory.CreateSurface(szScoreSoFar.Width + 8 + 2, szScoreSoFar.Height + 2);

    // background
    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
    const auto nShadowOffset = pTheme.ShadowOffset();

    m_pSurface->FillRectangle(0, 0, m_pSurface->GetWidth(), m_pSurface->GetHeight(), Color::Transparent);

    // frame
    m_pSurface->FillRectangle(nShadowOffset, nShadowOffset, szScoreSoFar.Width + 8,
        szScoreSoFar.Height, pTheme.ColorShadow());
    m_pSurface->FillRectangle(0, 0, szScoreSoFar.Width + 8, szScoreSoFar.Height, pTheme.ColorBackground());

    // text
    m_pSurface->WriteText(4, 0, nFontText, pTheme.ColorLeaderboardEntry(), sScoreSoFar);
#endif

    SetRenderLocationX(10 + m_pSurface->GetWidth());
    SetRenderLocationXRelativePosition(RelativePosition::Far);

    SetRenderLocationY(m_nOffset + m_pSurface->GetHeight());
    SetRenderLocationYRelativePosition(RelativePosition::Far);

    m_bSurfaceStale = false;
    return true;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
