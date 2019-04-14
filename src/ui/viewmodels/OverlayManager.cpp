#include "OverlayManager.hh"

#include "services\IClock.hh"
#include "services\IConfiguration.hh"
#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace viewmodels {

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

static void RenderPopup(ra::ui::drawing::ISurface& pSurface, const PopupViewModelBase& vmPopup)
{
    if (!vmPopup.IsAnimationStarted())
        return;

    const int nX = GetAbsolutePosition(vmPopup.GetRenderLocationX(), vmPopup.GetRenderLocationXRelativePosition(), pSurface.GetWidth());
    const int nY = GetAbsolutePosition(vmPopup.GetRenderLocationY(), vmPopup.GetRenderLocationYRelativePosition(), pSurface.GetHeight());
    pSurface.DrawSurface(nX, nY, vmPopup.GetRenderImage());
}

static void RenderPopupClippedX(ra::ui::drawing::ISurface& pSurface, const PopupViewModelBase& vmPopup, int nClipX)
{
    if (!vmPopup.IsAnimationStarted())
        return;

    const int nX = GetAbsolutePosition(vmPopup.GetRenderLocationX(), vmPopup.GetRenderLocationXRelativePosition(), pSurface.GetWidth());
    const int nY = GetAbsolutePosition(vmPopup.GetRenderLocationY(), vmPopup.GetRenderLocationYRelativePosition(), pSurface.GetHeight());

    if (nX + ra::to_signed(vmPopup.GetRenderImage().GetWidth()) > nClipX)
        pSurface.DrawSurface(nX, nY, vmPopup.GetRenderImage());
}

void OverlayManager::Update(const ControllerInput& pInput)
{
    if (m_vmOverlay.CurrentState() == OverlayViewModel::State::Visible)
        m_vmOverlay.ProcessInput(pInput);
}

void OverlayManager::RequestRender()
{
    if (!m_fHandleRenderRequest)
        return;

    const bool bNeedsRender = (m_vmOverlay.CurrentState() != OverlayViewModel::State::Hidden ||
        !m_vPopupMessages.empty() || !m_vScoreboards.empty() || !m_vScoreTrackers.empty());

    if (m_bIsRendering != bNeedsRender)
    {
        if (bNeedsRender)
        {
            auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
            m_tLastRender = pClock.UpTime();

            m_bIsRendering = true;
            m_fHandleRenderRequest();
        }
        else
        {
            m_bIsRendering = false;
        }
    }
    else if (m_bIsRendering && !m_bRenderRequestPending)
    {
        m_bRenderRequestPending = true;

        auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
        const auto tNow = pClock.UpTime();
        const auto tPacing = std::chrono::milliseconds(10);
        if (tNow - m_tLastRender < tPacing)
        {
            const auto nDelay = tPacing - (tNow - m_tLastRender);
            ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().ScheduleAsync(
                std::chrono::duration_cast<std::chrono::milliseconds>(nDelay), [this]()
            {
                m_fHandleRenderRequest();
            });
        }
        else
        {
            m_fHandleRenderRequest();
        }
    }
}

void OverlayManager::Render(ra::ui::drawing::ISurface& pSurface, bool bRedrawAll)
{
    m_bRenderRequestPending = false;
    m_bRedrawAll = bRedrawAll;

    auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
    const auto tNow = pClock.UpTime();
    const auto tElapsed = std::chrono::duration_cast<std::chrono::microseconds>(tNow - m_tLastRender);
    const double fElapsed = tElapsed.count() / 1000000.0;

    bool bRequestRender = false;

    if (m_vmOverlay.CurrentState() == OverlayViewModel::State::Hidden)
    {
        if (!m_vScoreboards.empty())
        {
            UpdateActiveScoreboard(pSurface, fElapsed);
            bRequestRender = true;
        }

        if (!m_vScoreTrackers.empty())
            UpdateScoreTrackers(pSurface, fElapsed);

        if (!m_vPopupMessages.empty())
        {
            UpdateActiveMessage(pSurface, fElapsed);
            bRequestRender = true;
        }
    }
    else
    {
        UpdateOverlay(pSurface, fElapsed);

        if (m_vmOverlay.CurrentState() != OverlayViewModel::State::Visible)
            bRequestRender = true;
    }

    if (bRequestRender)
        RequestRender();
    else
        m_bIsRendering = false;

    m_tLastRender = tNow;
    m_bRedrawAll = false;
}

ScoreTrackerViewModel& OverlayManager::AddScoreTracker(ra::LeaderboardID nLeaderboardId)
{
    auto pScoreTracker = std::make_unique<ScoreTrackerViewModel>();
    pScoreTracker->SetPopupId(nLeaderboardId);
    pScoreTracker->UpdateRenderImage(0.0);
    auto& pReturn = *m_vScoreTrackers.emplace_back(std::move(pScoreTracker));

    RequestRender();

    return pReturn;
}

void OverlayManager::RemoveScoreTracker(ra::LeaderboardID nLeaderboardId)
{
    for (auto pIter = m_vScoreTrackers.begin(); pIter != m_vScoreTrackers.end(); ++pIter)
    {
        if (ra::to_unsigned((*pIter)->GetPopupId()) == nLeaderboardId)
        {
            (*pIter)->SetDestroyPending();
            RequestRender();
            break;
        }
    }
}

void OverlayManager::QueueScoreboard(ra::LeaderboardID nLeaderboardId, ScoreboardViewModel&& vmScoreboard)
{
    vmScoreboard.SetPopupId(nLeaderboardId);
    m_vScoreboards.push_back(std::move(vmScoreboard));

    RequestRender();
}

int OverlayManager::QueueMessage(PopupMessageViewModel&& pMessage)
{
    if (pMessage.GetImage().Type() != ra::ui::ImageType::None)
    {
        if (ra::services::ServiceLocator::Exists<ra::ui::IImageRepository>())
        {
            auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
            pImageRepository.FetchImage(pMessage.GetImage().Type(), pMessage.GetImage().Name());
        }
    }

    pMessage.SetPopupId(++m_nPopupId);

    m_vPopupMessages.emplace_back(std::move(pMessage));

    RequestRender();

    return pMessage.GetPopupId();
}

void OverlayManager::ClearPopups()
{
    for (auto& pPopup : m_vPopupMessages)
        pPopup.SetDestroyPending();

    for (auto& pScoreboard : m_vScoreboards)
        pScoreboard.SetDestroyPending();

    for (auto& pTracker : m_vScoreTrackers)
        pTracker->SetDestroyPending();

    m_vmOverlay.Deactivate();

    RequestRender();
}

void OverlayManager::UpdatePopup(ra::ui::drawing::ISurface& pSurface, double fElapsed, ra::ui::viewmodels::PopupViewModelBase& vmPopup)
{
    if (!vmPopup.IsAnimationStarted())
    {
        vmPopup.BeginAnimation();
        vmPopup.UpdateRenderImage(0.0);
        fElapsed = 0.0;
    }

    const int nOldX = GetAbsolutePosition(vmPopup.GetRenderLocationX(), vmPopup.GetRenderLocationXRelativePosition(), pSurface.GetWidth());
    const int nOldY = GetAbsolutePosition(vmPopup.GetRenderLocationY(), vmPopup.GetRenderLocationYRelativePosition(), pSurface.GetHeight());

    if (vmPopup.IsDestroyPending())
    {
        const auto& pImage = vmPopup.GetRenderImage();
        pSurface.FillRectangle(nOldX, nOldY, pImage.GetWidth(), pImage.GetHeight(), ra::ui::Color::Transparent);
        return;
    }

    const bool bUpdated = vmPopup.UpdateRenderImage(fElapsed);

    const int nNewX = GetAbsolutePosition(vmPopup.GetRenderLocationX(), vmPopup.GetRenderLocationXRelativePosition(), pSurface.GetWidth());
    const int nNewY = GetAbsolutePosition(vmPopup.GetRenderLocationY(), vmPopup.GetRenderLocationYRelativePosition(), pSurface.GetHeight());

    if (bUpdated)
    {
        const auto& pImage = vmPopup.GetRenderImage();

        // if the image moved, we have to blank out the area it previously covered
        if (nOldY != nNewY || nOldX != nNewX)
        {
            const int nWidth = pImage.GetWidth() + std::abs(nNewX - nOldX);
            const int nHeight = pImage.GetHeight() + std::abs(nNewY - nOldY);

            if (nNewX > nOldX)
                pSurface.FillRectangle(nOldX, nOldY, nNewX - nOldX, nHeight, ra::ui::Color::Transparent);
            else if (nNewX < nOldX)
                pSurface.FillRectangle(nNewX + pImage.GetWidth() - 1, nOldY, nOldX - nNewX + 1, nHeight, ra::ui::Color::Transparent);

            if (nNewY > nOldY)
                pSurface.FillRectangle(nOldX, nOldY, nWidth, nNewY - nOldY, ra::ui::Color::Transparent);
            else if (nNewY < nOldY)
                pSurface.FillRectangle(nOldX, nNewY + pImage.GetHeight() - 1, nWidth, nOldY - nNewY + 1, ra::ui::Color::Transparent);
        }

        if (!vmPopup.IsAnimationComplete())
            pSurface.DrawSurface(nNewX, nNewY, pImage);
    }
    else if (m_bRedrawAll)
    {
        if (!vmPopup.IsAnimationComplete())
            pSurface.DrawSurface(nNewX, nNewY, vmPopup.GetRenderImage());
    }
}

void OverlayManager::UpdateActiveMessage(ra::ui::drawing::ISurface& pSurface, double fElapsed)
{
    assert(!m_vPopupMessages.empty());

    auto* pActiveMessage = &m_vPopupMessages.front();
    UpdatePopup(pSurface, fElapsed, *pActiveMessage);

    while (pActiveMessage->IsAnimationComplete() || pActiveMessage->IsDestroyPending())
    {
        m_vPopupMessages.pop_front();
        if (m_vPopupMessages.empty())
            break;

        pActiveMessage = &m_vPopupMessages.front();
        pActiveMessage->BeginAnimation();
        pActiveMessage->UpdateRenderImage(0.0);
    }
}

void OverlayManager::UpdateActiveScoreboard(ra::ui::drawing::ISurface& pSurface, double fElapsed)
{
    assert(!m_vScoreboards.empty());

    auto* pActiveScoreboard = &m_vScoreboards.front();
    UpdatePopup(pSurface, fElapsed, *pActiveScoreboard);

    while (pActiveScoreboard->IsAnimationComplete() || pActiveScoreboard->IsDestroyPending())
    {
        m_vScoreboards.pop_front();
        if (m_vScoreboards.empty())
            break;

        pActiveScoreboard = &m_vScoreboards.front();
        pActiveScoreboard->BeginAnimation();
        pActiveScoreboard->UpdateRenderImage(0.0);
    }
}

void OverlayManager::UpdateScoreTrackers(ra::ui::drawing::ISurface& pSurface, double fElapsed)
{
    assert(!m_vScoreTrackers.empty());

    auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::LeaderboardCounters))
    {
        auto pIter = m_vScoreTrackers.begin();
        while (pIter != m_vScoreTrackers.end())
        {
            if ((*pIter)->IsDestroyPending())
                pIter = m_vScoreTrackers.erase(pIter);
            else
                ++pIter;
        }

        return;
    }

    int nOffset = 10;
    if (!m_vScoreboards.empty())
        nOffset += m_vScoreboards.front().GetRenderImage().GetHeight() + 10;

    bool bPopupMoved = false;
    auto pIter = m_vScoreTrackers.begin();
    while (pIter != m_vScoreTrackers.end())
    {
        auto& vmTracker = **pIter;
        if (vmTracker.IsDestroyPending())
        {
            bPopupMoved = true;
            UpdatePopup(pSurface, fElapsed, vmTracker);
            pIter = m_vScoreTrackers.erase(pIter);
        }
        else
        {
            bPopupMoved |= vmTracker.SetOffset(nOffset);
            UpdatePopup(pSurface, fElapsed, vmTracker);
            nOffset += vmTracker.GetRenderImage().GetHeight() + 10;
            ++pIter;
        }
    }

    // if one or more of the popups moved, we will have drawn a transparent square where the popup used to be. redraw all the popups to make sure we didn't erase one
    if (bPopupMoved)
    {
        for (auto& vmTracker : m_vScoreTrackers)
            RenderPopup(pSurface, *vmTracker);

        if (!m_vScoreboards.empty())
            RenderPopup(pSurface, m_vScoreboards.front());
    }
}

void OverlayManager::UpdateOverlay(ra::ui::drawing::ISurface& pSurface, double fElapsed)
{
    if (pSurface.GetWidth() != m_vmOverlay.GetRenderImage().GetWidth() ||
        pSurface.GetHeight() != m_vmOverlay.GetRenderImage().GetHeight())
    {
        m_vmOverlay.Resize(pSurface.GetWidth(), pSurface.GetHeight());
    }

    if (m_vmOverlay.CurrentState() == OverlayViewModel::State::FadeOut)
    {
        // when fading out, have to redraw the popups that were obscured by the overlay
        const int nOverlayRight = GetAbsolutePosition(m_vmOverlay.GetRenderLocationX(),
            m_vmOverlay.GetRenderLocationXRelativePosition(), pSurface.GetWidth()) +
            m_vmOverlay.GetRenderImage().GetWidth();

        for (auto& pTracker : m_vScoreTrackers)
            RenderPopupClippedX(pSurface, *pTracker, nOverlayRight);

        if (!m_vScoreboards.empty())
            RenderPopupClippedX(pSurface, m_vScoreboards.front(), nOverlayRight);

        if (!m_vPopupMessages.empty())
            RenderPopupClippedX(pSurface, m_vPopupMessages.front(), nOverlayRight);
    }

    UpdatePopup(pSurface, fElapsed, m_vmOverlay);

    if (m_vmOverlay.CurrentState() == OverlayViewModel::State::Hidden)
    {
        m_vmOverlay.DestroyRenderImage();

        // fade out completed, render everything that was obscured by the overlay
        for (auto& pTracker : m_vScoreTrackers)
            RenderPopup(pSurface, *pTracker);

        if (!m_vScoreboards.empty())
            RenderPopup(pSurface, m_vScoreboards.front());

        if (!m_vPopupMessages.empty())
            RenderPopup(pSurface, m_vPopupMessages.front());
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
