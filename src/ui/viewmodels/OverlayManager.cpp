#include "OverlayManager.hh"

#include "services\IClock.hh"
#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace viewmodels {

void OverlayManager::Update(double fElapsed)
{
    if (!m_vPopupMessages.empty())
        UpdateActiveMessage(fElapsed);

    if (!m_vScoreboards.empty())
        UpdateActiveScoreboard(fElapsed);

    for (auto& pScoreTracker : m_vScoreTrackers)
        pScoreTracker->UpdateRenderImage(fElapsed);
}

static int GetAbsolutePosition(int nValue, RelativePosition nRelativePosition, size_t nFarValue) noexcept
{
    switch (nRelativePosition)
    {
        case RelativePosition::Before:
            return -nValue;
        default:
        case RelativePosition::Near:
            return nValue;
        case RelativePosition::Center:
            return ra::to_signed((nFarValue - nValue) / 2);
        case RelativePosition::Far:
            return ra::to_signed(nFarValue - nValue);
        case RelativePosition::After:
            return ra::to_signed(nFarValue + nValue);
    }
}

void OverlayManager::Render(ra::ui::drawing::ISurface& pSurface) const
{
    int nScoreboardHeight = 0;
    if (!m_vScoreboards.empty())
    {
        // if the animation hasn't started, it was queued after the last update was called.
        // ignore it for now as the location won't be correct, and the render image won't be ready.
        const auto& pActiveScoreboard = m_vScoreboards.front();
        if (pActiveScoreboard.IsAnimationStarted())
        {
            const int nX = GetAbsolutePosition(pActiveScoreboard.GetRenderLocationX(), pActiveScoreboard.GetRenderLocationXRelativePosition(), pSurface.GetWidth());
            const int nY = GetAbsolutePosition(pActiveScoreboard.GetRenderLocationY(), pActiveScoreboard.GetRenderLocationYRelativePosition(), pSurface.GetHeight());
            pSurface.DrawSurface(nX, nY, pActiveScoreboard.GetRenderImage());
            nScoreboardHeight = pActiveScoreboard.GetRenderImage().GetHeight() + 10;
        }
    }

    if (!m_vScoreTrackers.empty())
    {
        auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
        if (pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardCounters))
        {
            const auto nX = pSurface.GetWidth() - 10;
            int nY = pSurface.GetHeight() - 10 - nScoreboardHeight;

            for (auto& pTracker : m_vScoreTrackers)
            {
                const auto& pRenderImage = pTracker->GetRenderImage();
                nY -= pRenderImage.GetHeight();
                pSurface.DrawSurface(nX - pRenderImage.GetWidth(), nY, pRenderImage);
                nY -= 10;
            }
        }
    }

    if (!m_vPopupMessages.empty())
    {
        // if the animation hasn't started, it was queued after the last update was called.
        // ignore it for now as the location won't be correct, and the render image won't be ready.
        const auto& pActiveMessage = m_vPopupMessages.front();
        if (pActiveMessage.IsAnimationStarted())
        {
            const int nX = GetAbsolutePosition(pActiveMessage.GetRenderLocationX(), pActiveMessage.GetRenderLocationXRelativePosition(), pSurface.GetWidth());
            const int nY = GetAbsolutePosition(pActiveMessage.GetRenderLocationY(), pActiveMessage.GetRenderLocationYRelativePosition(), pSurface.GetHeight());
            pSurface.DrawSurface(nX, nY, pActiveMessage.GetRenderImage());
        }
    }
}

void OverlayManager::QueueScoreboard(ra::LeaderboardID nLeaderboardId, ScoreboardViewModel&& vmScoreboard)
{
    vmScoreboard.SetPopupId(nLeaderboardId);
    m_vScoreboards.push_back(std::move(vmScoreboard));
}

int OverlayManager::QueueMessage(PopupMessageViewModel&& pMessage)
{
#ifndef RA_UTEST
    if (pMessage.GetImage().Type() != ra::ui::ImageType::None)
    {
        auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
        pImageRepository.FetchImage(pMessage.GetImage().Type(), pMessage.GetImage().Name());
    }
#endif

    pMessage.SetPopupId(++m_nPopupId);

    m_vPopupMessages.emplace_back(std::move(pMessage));

    return pMessage.GetPopupId();
}

void OverlayManager::ClearPopups()
{
    while (!m_vPopupMessages.empty())
        m_vPopupMessages.pop_front();

    while (!m_vScoreboards.empty())
        m_vScoreboards.pop_front();

    m_vScoreTrackers.clear();
}

void OverlayManager::UpdateActiveMessage(double fElapsed)
{
    assert(!m_vPopupMessages.empty());
    auto* pActiveMessage = &m_vPopupMessages.front();

    if (!pActiveMessage->IsAnimationStarted())
    {
        pActiveMessage->BeginAnimation();
        fElapsed = 0.0;
    }

    pActiveMessage->UpdateRenderImage(fElapsed);

    while (pActiveMessage->IsAnimationComplete())
    {
        m_vPopupMessages.pop_front();
        if (m_vPopupMessages.empty())
            break;

        pActiveMessage = &m_vPopupMessages.front();
        pActiveMessage->BeginAnimation();
        pActiveMessage->UpdateRenderImage(0.0);
    }
}

void OverlayManager::UpdateActiveScoreboard(double fElapsed)
{
    assert(!m_vScoreboards.empty());
    auto* pActiveScoreboard = &m_vScoreboards.front();

    if (!pActiveScoreboard->IsAnimationStarted())
    {
        pActiveScoreboard->BeginAnimation();
        fElapsed = 0.0;
    }

    pActiveScoreboard->UpdateRenderImage(fElapsed);

    while (pActiveScoreboard->IsAnimationComplete())
    {
        m_vScoreboards.pop_front();
        if (m_vScoreboards.empty())
            break;

        pActiveScoreboard = &m_vScoreboards.front();
        pActiveScoreboard->BeginAnimation();
        pActiveScoreboard->UpdateRenderImage(0.0);
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
