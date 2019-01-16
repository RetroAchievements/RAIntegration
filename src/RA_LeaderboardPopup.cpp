#include "RA_LeaderboardPopup.h"

#include "RA_AchievementOverlay.h"
#include "RA_ImageFactory.h"

#include "ra_math.h"

#include "services\IConfiguration.hh"
#include "services\ILeaderboardManager.hh"
#include "services\ServiceLocator.hh"

#include "ui\drawing\ISurface.hh"

//	No emulator-specific code here please!

namespace {

const float SCOREBOARD_APPEAR_AT = 0.8f;
const float SCOREBOARD_FADEOUT_AT = 6.2f;
const float SCOREBOARD_FINISH_AT = 7.0f;

const char* FONT_TO_USE = "Tahoma";

const int FONT_SIZE_TITLE = 22;
const int FONT_SIZE_TEXT = 22;

} // namespace

void LeaderboardPopup::ShowScoreboard(ra::LeaderboardID nID)
{
    m_vScoreboardQueue.push(nID);

    if (m_fScoreboardShowTimer == SCOREBOARD_FINISH_AT)
    {
        m_fScoreboardShowTimer = 0.0f;
        m_nState = PopupState::ShowingScoreboard;
    }
}

void LeaderboardPopup::Reset()
{
    m_fScoreboardShowTimer = SCOREBOARD_FINISH_AT;
    while (!m_vScoreboardQueue.empty())
        m_vScoreboardQueue.pop();

    m_vActiveLBIDs.clear();
    m_nState = PopupState::ShowingProgress;
}

_Use_decl_annotations_
void LeaderboardPopup::Update(double fDelta)
{
    auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards)) //	If not, simply ignore them.
        return;

    if (m_fScoreboardShowTimer >= SCOREBOARD_FINISH_AT)
    {
        m_pScoreboardSurface.reset();

        //	No longer showing the scoreboard
        if (!m_vScoreboardQueue.empty())
        {
            m_vScoreboardQueue.pop();

            if (!m_vScoreboardQueue.empty())
            {
                //	Still not empty: restart timer
                m_fScoreboardShowTimer = 0.0f; //	Show next scoreboard
            }
            else
            {
                m_fScoreboardShowTimer = SCOREBOARD_FINISH_AT;
                m_nState = PopupState::ShowingProgress;
            }
        }
        else
        {
            m_fScoreboardShowTimer = SCOREBOARD_FINISH_AT;
            m_nState = PopupState::ShowingProgress;
        }
    }
    else
    {
        m_fScoreboardShowTimer += fDelta;
    }
}

_Use_decl_annotations_ BOOL LeaderboardPopup::Activate(ra::LeaderboardID nLBID)
{
    std::vector<unsigned int>::iterator iter = m_vActiveLBIDs.begin();
    while (iter != m_vActiveLBIDs.end())
    {
        if ((*iter) == nLBID)
            return FALSE;
        iter++;
    }

    m_vActiveLBIDs.push_back(nLBID);
    return TRUE;
}

_Use_decl_annotations_ BOOL LeaderboardPopup::Deactivate(ra::LeaderboardID nLBID)
{
    std::vector<unsigned int>::iterator iter = m_vActiveLBIDs.begin();
    while (iter != m_vActiveLBIDs.end())
    {
        if ((*iter) == nLBID)
        {
            m_vActiveLBIDs.erase(iter);
            return TRUE;
        }
        iter++;
    }

    RA_LOG("Could not deactivate leaderboard %u", nLBID);

    return FALSE;
}

double LeaderboardPopup::GetOffsetPct() const noexcept
{
    double fVal = 0.0f;

    if (m_fScoreboardShowTimer < SCOREBOARD_APPEAR_AT)
    {
        // Fading in.
        const auto fDelta = (SCOREBOARD_APPEAR_AT - m_fScoreboardShowTimer);

        fVal = fDelta * fDelta; // Quadratic
    }
    else if (m_fScoreboardShowTimer < (SCOREBOARD_FADEOUT_AT))
    {
        //	Faded in - held
        fVal = 0.0f;
    }
    else if (m_fScoreboardShowTimer < (SCOREBOARD_FINISH_AT))
    {
        //	Fading out
        const auto fDelta = (SCOREBOARD_FADEOUT_AT - m_fScoreboardShowTimer);

        fVal = fDelta * fDelta; // Quadratic
    }
    else
    {
        //	Finished!
        fVal = 1.0f;
    }

    return fVal;
}

_Use_decl_annotations_ void LeaderboardPopup::Render(ra::ui::drawing::ISurface& pSurface)
{
    auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Leaderboards)) //	If not, simply ignore them.
        return;

    const int nWidth = pSurface.GetWidth();
    const int nHeight = pSurface.GetHeight();

    const ra::ui::Color nColorBackground(0, 255, 0, 255);
    const ra::ui::Color nColorBlack(0, 0, 0);
    const ra::ui::Color nColorPopup(251, 102, 0);
    constexpr int nShadowOffset = 2;

    switch (m_nState)
    {
        case PopupState::ShowingProgress:
        {
            if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardCounters))
                break;

            const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
            auto pTempSurface = pSurfaceFactory.CreateSurface(1, 1);
            auto nFontText = pTempSurface->LoadFont(FONT_TO_USE, FONT_SIZE_TEXT, ra::ui::FontStyles::Normal);

            auto& pLeaderboardManager = ra::services::ServiceLocator::Get<ra::services::ILeaderboardManager>();
            int nY = pSurface.GetHeight() - 10;
            std::vector<unsigned int>::const_iterator iter = m_vActiveLBIDs.begin();
            while (iter != m_vActiveLBIDs.end())
            {
                const RA_Leaderboard* pLB = pLeaderboardManager.FindLB(*iter);
                if (pLB != nullptr)
                {
                    const auto sScoreSoFar = ra::Widen(pLB->FormatScore(pLB->GetCurrentValue()));
                    const auto szScoreSoFar = pTempSurface->MeasureText(nFontText, sScoreSoFar);

                    auto pRenderSurface =
                        pSurfaceFactory.CreateTransparentSurface(szScoreSoFar.Width + 8 + 2, szScoreSoFar.Height + 2);

                    // background
                    pRenderSurface->FillRectangle(0, 0, pRenderSurface->GetWidth(), pRenderSurface->GetHeight(),
                                                  nColorBackground);

                    // text
                    pRenderSurface->FillRectangle(nShadowOffset, nShadowOffset, szScoreSoFar.Width + 8,
                                                  szScoreSoFar.Height, nColorBlack);
                    pRenderSurface->FillRectangle(0, 0, szScoreSoFar.Width + 8, szScoreSoFar.Height, nColorPopup);
                    pRenderSurface->WriteText(4, 0, nFontText, nColorBlack, sScoreSoFar);

                    pRenderSurface->SetOpacity(0.85);

                    nY -= pRenderSurface->GetHeight();
                    pSurface.DrawSurface(nWidth - pRenderSurface->GetWidth() - 10, nY, *pRenderSurface);
                    nY -= 2;
                }

                iter++;
            }
        }
        break;

        case PopupState::ShowingScoreboard:
        {
            if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardScoreboards))
                break;

            if (m_pScoreboardSurface == nullptr)
            {
                auto& pLeaderboardManager = ra::services::ServiceLocator::Get<ra::services::ILeaderboardManager>();
                const RA_Leaderboard* pLB = pLeaderboardManager.FindLB(m_vScoreboardQueue.front());
                if (pLB != nullptr)
                {
                    const ra::ui::Color nColorBackgroundFill(32, 32, 32);

                    const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
                    m_pScoreboardSurface = pSurfaceFactory.CreateTransparentSurface(300, 200);
                    auto nFontTitle =
                        m_pScoreboardSurface->LoadFont(FONT_TO_USE, FONT_SIZE_TITLE, ra::ui::FontStyles::Normal);
                    auto nFontText =
                        m_pScoreboardSurface->LoadFont(FONT_TO_USE, FONT_SIZE_TEXT, ra::ui::FontStyles::Normal);

                    // background
                    m_pScoreboardSurface->FillRectangle(0, 0, m_pScoreboardSurface->GetWidth(),
                                                        m_pScoreboardSurface->GetHeight(), nColorBackground);
                    m_pScoreboardSurface->FillRectangle(2, 2, m_pScoreboardSurface->GetWidth() - nShadowOffset,
                                                        m_pScoreboardSurface->GetHeight() - nShadowOffset, nColorBlack);
                    m_pScoreboardSurface->FillRectangle(0, 0, m_pScoreboardSurface->GetWidth() - nShadowOffset,
                                                        m_pScoreboardSurface->GetHeight() - nShadowOffset,
                                                        nColorBackgroundFill);

                    // title
                    const auto sResultsTitle = ra::StringPrintf(L"Results: %s", pLB->Title());
                    m_pScoreboardSurface->FillRectangle(4, 4, m_pScoreboardSurface->GetWidth() - nShadowOffset - 8,
                                                        FONT_SIZE_TITLE, nColorPopup);
                    m_pScoreboardSurface->WriteText(8, 3, nFontTitle, nColorBlack, sResultsTitle);

                    // scoreboard
                    size_t nY = 4 + FONT_SIZE_TITLE + 4;
                    size_t i = 0;
                    while (i < pLB->GetRankInfoCount() && nY + FONT_SIZE_TEXT < m_pScoreboardSurface->GetHeight())
                    {
                        const RA_Leaderboard::Entry& lbInfo = pLB->GetRankInfo(i++);
                        // Suppress isn't working, suppress this by function
                        GSL_SUPPRESS_CON4 ra::ui::Color nTextColor = nColorPopup;

                        if (lbInfo.m_sUsername.compare(RAUsers::LocalUser().Username()) == 0)
                            nTextColor = ra::ui::Color(255, 192, 128);

                        m_pScoreboardSurface->WriteText(8, nY, nFontText, nTextColor, ra::ToWString(i));
                        m_pScoreboardSurface->WriteText(24, nY, nFontText, nTextColor, ra::Widen(lbInfo.m_sUsername));

                        const auto sScore = ra::Widen(pLB->FormatScore(lbInfo.m_nScore));
                        const auto szScore = m_pScoreboardSurface->MeasureText(nFontText, sScore);
                        m_pScoreboardSurface->WriteText(m_pScoreboardSurface->GetWidth() - 4 - szScore.Width - 8, nY,
                                                        nFontText, nTextColor, sScore);

                        nY += FONT_SIZE_TEXT + 2;
                    }

                    m_pScoreboardSurface->SetOpacity(0.85);
                }
            }

            if (m_pScoreboardSurface != nullptr)
            {
                const auto fOffscreenAmount = (GetOffsetPct() * 600);
                const auto fFadeOffs = (ra::to_unsigned(nWidth) - 300 - 10 + fOffscreenAmount);
                const auto nScoreboardX = ra::ftoi(fFadeOffs);
                const auto nScoreboardY = nHeight - 200 - 10;
                pSurface.DrawSurface(nScoreboardX, nScoreboardY, *m_pScoreboardSurface);
            }
        }
        break;

        default:
            break;
    }
}
