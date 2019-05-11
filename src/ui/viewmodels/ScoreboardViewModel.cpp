#include "ScoreboardViewModel.hh"

#include "RA_StringUtils.h"

#include "ui\OverlayTheme.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty ScoreboardViewModel::HeaderTextProperty("ScoreboardViewModel", "HeaderText", L"");
const IntModelProperty ScoreboardViewModel::EntryViewModel::RankProperty("ScoreboardViewModel::EntryViewModel", "Rank", 1);
const StringModelProperty ScoreboardViewModel::EntryViewModel::UserNameProperty("ScoreboardViewModel::EntryViewModel", "UserName", L"");
const StringModelProperty ScoreboardViewModel::EntryViewModel::ScoreProperty("ScoreboardViewModel::EntryViewModel", "Score", L"0");
const BoolModelProperty ScoreboardViewModel::EntryViewModel::IsHighlightedProperty("ScoreboardViewModel::EntryViewModel", "IsHighlighted", false);

constexpr int NUM_ENTRIES = 7;

static int CalculateScoreboardHeight(const ra::ui::OverlayTheme& pTheme) noexcept
{
    return 4 + pTheme.FontSizePopupLeaderboardTitle() + 2 +
        (pTheme.FontSizePopupLeaderboardEntry() + 2) * NUM_ENTRIES + 4;
}

static int CalculateScoreboardWidth(const ra::ui::OverlayTheme& pTheme) noexcept
{
    return 4 + std::max(pTheme.FontSizePopupLeaderboardEntry() * 15, pTheme.FontSizePopupLeaderboardTitle() * 10) + 4;
}

void ScoreboardViewModel::BeginAnimation()
{
    m_fAnimationProgress = 0.0;

    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
    const auto nHeight = CalculateScoreboardHeight(pTheme);
    const auto nWidth = CalculateScoreboardWidth(pTheme);

    // bottom margin 10px
    SetRenderLocationY(10 + nHeight);
    SetRenderLocationYRelativePosition(RelativePosition::Far);

    // animate to right margin 10px.
    m_nInitialX = 0;
    m_nTargetX = 10 + nWidth;
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

        const auto nHeight = CalculateScoreboardHeight(pTheme) + nShadowOffset;
        const auto nWidth = CalculateScoreboardWidth(pTheme) + nShadowOffset;

        const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
        m_pSurface = pSurfaceFactory.CreateSurface(nWidth, nHeight);
        const auto nFontTitle = m_pSurface->LoadFont(pTheme.FontPopup(), pTheme.FontSizePopupLeaderboardTitle(), ra::ui::FontStyles::Normal);
        const auto nFontText = m_pSurface->LoadFont(pTheme.FontPopup(), pTheme.FontSizePopupLeaderboardEntry(), ra::ui::FontStyles::Normal);

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
        if (m_vEntries.Count() > 0)
        {
            // get width of largest displayed rank so other ranks can be right-aligned with it
            const auto nRankSize = m_pSurface->MeasureText(nFontText, ra::ToWString(m_vEntries.GetItemAt(m_vEntries.Count() - 1)->GetRank()));

            size_t nY = 4 + pTheme.FontSizePopupLeaderboardTitle() + 2;
            size_t i = 0;
            while (i < NUM_ENTRIES && i < m_vEntries.Count())
            {
                const auto* pEntry = m_vEntries.GetItemAt(i++);
                if (!pEntry)
                    continue;

                const ra::ui::Color nTextColor = pEntry->IsHighlighted() ? pTheme.ColorLeaderboardPlayer() : pTheme.ColorLeaderboardEntry();

                // rank (right aligned)
                const auto sRank = ra::ToWString(pEntry->GetRank());
                const auto nEntryRankSize = m_pSurface->MeasureText(nFontText, sRank);
                m_pSurface->WriteText(8 + nRankSize.Width - nEntryRankSize.Width, nY, nFontText, nTextColor, sRank);

                // player name
                m_pSurface->WriteText(8 + nRankSize.Width + 8, nY, nFontText, nTextColor, pEntry->GetUserName());

                // score (right aligned)
                const auto& sScore = pEntry->GetScore();
                const auto szScore = m_pSurface->MeasureText(nFontText, sScore);
                m_pSurface->WriteText(m_pSurface->GetWidth() - 4 - szScore.Width - 8, nY, nFontText, nTextColor, sScore);

                nY += pTheme.FontSizePopupLeaderboardEntry() + 2;
            }
        }

        bUpdated = true;
    }

    return bUpdated;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
