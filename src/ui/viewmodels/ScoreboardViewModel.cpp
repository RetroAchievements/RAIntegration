#include "ScoreboardViewModel.hh"

#include "RA_StringUtils.h"

#include "ui\OverlayTheme.hh"

namespace ra {
namespace ui {
namespace viewmodels {

static _CONSTANT_VAR FONT_TO_USE = "Tahoma";
static _CONSTANT_VAR FONT_SIZE_TITLE = 22;
static _CONSTANT_VAR FONT_SIZE_TEXT = 22;

static _CONSTANT_VAR SCOREBOARD_WIDTH = 300;
static _CONSTANT_VAR SCOREBOARD_HEIGHT = 200;

const StringModelProperty ScoreboardViewModel::HeaderTextProperty("ScoreboardViewModel", "HeaderText", L"");
const IntModelProperty ScoreboardViewModel::EntryViewModel::RankProperty("ScoreboardViewModel::EntryViewModel", "Rank", 1);
const StringModelProperty ScoreboardViewModel::EntryViewModel::UserNameProperty("ScoreboardViewModel::EntryViewModel", "UserName", L"");
const StringModelProperty ScoreboardViewModel::EntryViewModel::ScoreProperty("ScoreboardViewModel::EntryViewModel", "Score", L"0");
const BoolModelProperty ScoreboardViewModel::EntryViewModel::IsHighlightedProperty("ScoreboardViewModel::EntryViewModel", "IsHighlighted", false);

void ScoreboardViewModel::BeginAnimation()
{
    m_fAnimationProgress = 0.0;

    // bottom margin 10px
    SetRenderLocationY(10 + SCOREBOARD_HEIGHT);
    SetRenderLocationYRelativePosition(RelativePosition::Far);

    // animate to right margin 10px.
    m_nInitialX = 0;
    m_nTargetX = 10 + SCOREBOARD_WIDTH;
    SetRenderLocationX(m_nInitialX);
    SetRenderLocationXRelativePosition(RelativePosition::Far);
}

bool ScoreboardViewModel::UpdateRenderImage(double fElapsed)
{
    const int nOldX = GetRenderLocationX();

    m_fAnimationProgress += fElapsed;
    const int nNewX = GetFadeOffset(m_fAnimationProgress, TOTAL_ANIMATION_TIME, INOUT_TIME, m_nInitialX, m_nTargetX);

    bool bUpdated = false;
    if (nNewX != nOldX)
    {
        SetRenderLocationX(nNewX);
        bUpdated = true;
    }

    if (m_pSurface == nullptr)
    {
        const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
        const auto nShadowOffset = pTheme.ShadowOffset();

        const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
        m_pSurface = pSurfaceFactory.CreateTransparentSurface(SCOREBOARD_WIDTH, SCOREBOARD_HEIGHT);
        auto nFontTitle = m_pSurface->LoadFont(FONT_TO_USE, FONT_SIZE_TITLE, ra::ui::FontStyles::Normal);
        auto nFontText = m_pSurface->LoadFont(FONT_TO_USE, FONT_SIZE_TEXT, ra::ui::FontStyles::Normal);

        // background
        m_pSurface->FillRectangle(0, 0, m_pSurface->GetWidth(), m_pSurface->GetHeight(), Color::Transparent);
        m_pSurface->FillRectangle(nShadowOffset, nShadowOffset, m_pSurface->GetWidth() - nShadowOffset, m_pSurface->GetHeight() - nShadowOffset,
                                  pTheme.ColorShadow());

        // frame
        m_pSurface->FillRectangle(0, 0, m_pSurface->GetWidth() - nShadowOffset, m_pSurface->GetHeight() - nShadowOffset,
                                  pTheme.ColorBackground());
        m_pSurface->FillRectangle(1, 1, m_pSurface->GetWidth() - nShadowOffset - 2, m_pSurface->GetHeight() - nShadowOffset - 2,
            pTheme.ColorBorder());
        m_pSurface->FillRectangle(2, 2, m_pSurface->GetWidth() - nShadowOffset - 4, m_pSurface->GetHeight() - nShadowOffset - 4,
            pTheme.ColorBackground());
        m_pSurface->FillRectangle(2, 25, m_pSurface->GetWidth() - nShadowOffset - 4, 1, pTheme.ColorBorder());

        // title
        const auto sResultsTitle = GetHeaderText();
        m_pSurface->WriteText(8, 1, nFontTitle, pTheme.ColorTitle(), sResultsTitle);

        // scoreboard
        size_t nY = 4 + FONT_SIZE_TITLE + 2;
        size_t i = 0;
        while (i < m_vEntries.Count() && nY + FONT_SIZE_TEXT < m_pSurface->GetHeight())
        {
            const auto* pEntry = m_vEntries.GetItemAt(i++);
            if (!pEntry)
                continue;

            const ra::ui::Color nTextColor = pEntry->IsHighlighted() ? pTheme.ColorLeaderboardPlayer() : pTheme.ColorLeaderboardEntry();
            m_pSurface->WriteText(8, nY, nFontText, nTextColor, ra::ToWString(i));
            m_pSurface->WriteText(24, nY, nFontText, nTextColor, pEntry->GetUserName());

            const auto& sScore = pEntry->GetScore();
            const auto szScore = m_pSurface->MeasureText(nFontText, sScore);
            m_pSurface->WriteText(m_pSurface->GetWidth() - 4 - szScore.Width - 8, nY, nFontText, nTextColor, sScore);

            nY += FONT_SIZE_TEXT + 2;
        }

        m_pSurface->SetOpacity(0.85);

        bUpdated = true;
    }

    return bUpdated;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
