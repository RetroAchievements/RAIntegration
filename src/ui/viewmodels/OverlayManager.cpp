#include "OverlayManager.hh"

#include "RA_Log.h"

#include "services\IClock.hh"
#include "services\IConfiguration.hh"
#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

#include "ui\IDesktop.hh"
#include "ui\OverlayTheme.hh"

#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

static void RenderPopup(ra::ui::drawing::ISurface& pSurface, const PopupViewModelBase& vmPopup)
{
    if (!vmPopup.IsAnimationStarted())
        return;

    const auto nX = vmPopup.GetRenderLocationX();
    const auto nY = vmPopup.GetRenderLocationY();
    pSurface.DrawSurface(nX, nY, vmPopup.GetRenderImage());
}

static void RenderPopupClippedX(ra::ui::drawing::ISurface& pSurface, const PopupViewModelBase& vmPopup, int nClipX)
{
    if (!vmPopup.IsAnimationStarted())
        return;

    const auto nX = vmPopup.GetRenderLocationX();
    const auto nY = vmPopup.GetRenderLocationY();

    const auto& pImage = vmPopup.GetRenderImage();
    const int nVisibleWidth = nX + ra::to_signed(pImage.GetWidth()) - nClipX;
    if (nVisibleWidth > 0)
        pSurface.DrawSurface(nClipX, nY, pImage, pImage.GetWidth() - nVisibleWidth, 0, nVisibleWidth, pImage.GetHeight());
}

void OverlayManager::InitializeNotifyTargets()
{
    auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
    pGameContext.AddNotifyTarget(*this);
}

void OverlayManager::Update(const ControllerInput& pInput)
{
    if (m_vmOverlay.CurrentState() == OverlayViewModel::State::Visible)
        m_vmOverlay.ProcessInput(pInput);
}

bool OverlayManager::NeedsRender() const noexcept
{
    if (m_vmOverlay.CurrentState() != OverlayViewModel::State::Hidden)
    {
        // overlay is visible
        return true;
    }

    if (!m_vPopupMessages.empty() || !m_vScoreboards.empty() || !m_vScoreTrackers.empty() || !m_vChallengeIndicators.empty())
    {
        // a popup is visible
        return true;
    }

    return false;
}

void OverlayManager::RequestRender()
{
    if (!m_fHandleRenderRequest)
        return;

    auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
    const auto tNow = pClock.UpTime();
    const auto tTimeSinceLastRequest = tNow - m_tLastRequestRender;
    m_tLastRequestRender = tNow;

    const bool bNeedsRender = NeedsRender();
    if (m_bIsRendering != bNeedsRender)
    {
        if (bNeedsRender)
        {
            // when the overlay first becomes visible, capture the current time for
            // elapsed calculations in Render
            m_tLastRender = tNow;

            if (m_fHandleShowRequest)
                m_fHandleShowRequest();

            m_bIsRendering = true;
            m_fHandleRenderRequest();
        }
        else
        {
            m_bIsRendering = false;

            if (m_fHandleHideRequest)
                m_fHandleHideRequest();
        }
    }
    else if (m_bIsRendering && !m_bRenderRequestPending)
    {
        m_bRenderRequestPending = true;

#ifndef RA_UTEST
        // make sure not to call RenderRequest more than once every 10ms
        const auto tPacing = std::chrono::milliseconds(10);
        if (tTimeSinceLastRequest < tPacing)
        {
            const auto nDelay = tPacing - tTimeSinceLastRequest;
            ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().ScheduleAsync(
                std::chrono::duration_cast<std::chrono::milliseconds>(nDelay), [this]()
            {
                m_fHandleRenderRequest();
            });
        }
        else
#endif
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
        PopupLocations pPopupLocations;
        memset(&pPopupLocations, 0, sizeof(pPopupLocations));

        // layout pass - stack items in desired order

        if (!m_vPopupMessages.empty())
        {
            UpdateActiveMessage(pSurface, pPopupLocations, fElapsed);

            // a visible popup may not set m_bRedrawAll if it's in the pause portion of its animation loop
            // make sure we keep the render cycle going so it'll eventually handle when the pause ends
            bRequestRender = true;
        }

        if (!m_vScoreboards.empty())
        {
            UpdateActiveScoreboard(pSurface, pPopupLocations, fElapsed);

            // a visible scoreboard may not set m_bRedrawAll if it's in the pause portion of its animation loop
            // make sure we keep the render cycle going so it'll eventually handle when the pause ends
            bRequestRender = true;
        }

        if (!m_vScoreTrackers.empty())
            UpdateScoreTrackers(pSurface, pPopupLocations, fElapsed);

        if (!m_vChallengeIndicators.empty())
            UpdateChallengeIndicators(pSurface, pPopupLocations, fElapsed);

        // if anything changed, or caller requested a repaint, do so now
        if (m_bRedrawAll)
        {
            // render pass - items that should appear over other items should be drawn last
            if (!m_vScoreboards.empty())
                RenderPopup(pSurface, m_vScoreboards.front());
            for (const auto& pScoreTracker : m_vScoreTrackers)
                RenderPopup(pSurface, *pScoreTracker);
            for (const auto& pChallengeIndicator : m_vChallengeIndicators)
                RenderPopup(pSurface, *pChallengeIndicator);
            if (!m_vPopupMessages.empty())
                RenderPopup(pSurface, *m_vPopupMessages.front());

            bRequestRender = true;
        }
    }
    else
    {
        UpdateOverlay(pSurface, fElapsed);

        if (m_vmOverlay.CurrentState() != OverlayViewModel::State::Visible)
            bRequestRender = true;
    }

    // update state now, in case handlers cause recursion
    m_tLastRender = tNow;
    m_bRedrawAll = false;

    if (bRequestRender)
    {
        // something changed, render is explicit
        RequestRender();
    }
    else if (m_bIsRendering)
    {
        m_bIsRendering = false;

        if (!NeedsRender())
        {
            // render no longer needed, hide the overlay
            if (m_fHandleHideRequest)
                m_fHandleHideRequest();
        }
    }
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
        if (ra::to_unsigned((*pIter)->GetPopupId()) == nLeaderboardId && !(*pIter)->IsDestroyPending())
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

ChallengeIndicatorViewModel& OverlayManager::AddChallengeIndicator(ra::AchievementID nAchievementId,
    ra::ui::ImageType imageType, const std::string& sImageName)
{
    for (auto pIter = m_vChallengeIndicators.begin(); pIter != m_vChallengeIndicators.end(); ++pIter)
    {
        if (ra::to_unsigned((*pIter)->GetPopupId()) == nAchievementId && !(*pIter)->IsDestroyPending())
        {
            // already showing indicator for achievement, just return it
            return **pIter;
        }
    }

    auto pChallengeIndicator = std::make_unique<ChallengeIndicatorViewModel>();
    pChallengeIndicator->SetPopupId(nAchievementId);
    pChallengeIndicator->SetImage(imageType, sImageName);
    pChallengeIndicator->UpdateRenderImage(0.0);
    auto& pReturn = *m_vChallengeIndicators.emplace_back(std::move(pChallengeIndicator));

    RequestRender();

    return pReturn;
}

void OverlayManager::RemoveChallengeIndicator(ra::AchievementID nAchievementId)
{
    for (auto pIter = m_vChallengeIndicators.begin(); pIter != m_vChallengeIndicators.end(); ++pIter)
    {
        if (ra::to_unsigned((*pIter)->GetPopupId()) == nAchievementId && !(*pIter)->IsDestroyPending())
        {
            (*pIter)->SetDestroyPending();
            RequestRender();
            break;
        }
    }
}

int OverlayManager::QueueMessage(std::unique_ptr<PopupMessageViewModel>& pMessage)
{
    if (pMessage->GetImage().Type() != ra::ui::ImageType::None)
    {
        if (ra::services::ServiceLocator::Exists<ra::ui::IImageRepository>())
        {
            auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
            pImageRepository.FetchImage(pMessage->GetImage().Type(), pMessage->GetImage().Name());
        }
    }

    const auto nPopupId = ++m_nPopupId;
    pMessage->SetPopupId(nPopupId);

    m_vPopupMessages.emplace_back(std::move(pMessage));

    RequestRender();

    return nPopupId;
}

void OverlayManager::ClearPopups()
{
    for (auto& pPopup : m_vPopupMessages)
        pPopup->SetDestroyPending();

    for (auto& pScoreboard : m_vScoreboards)
        pScoreboard.SetDestroyPending();

    for (auto& pChallengeIndicator : m_vChallengeIndicators)
        pChallengeIndicator->SetDestroyPending();

    for (auto& pTracker : m_vScoreTrackers)
        pTracker->SetDestroyPending();

    RequestRender();
}

ra::ui::Position OverlayManager::GetRenderLocation(const ra::ui::viewmodels::PopupViewModelBase& vmPopup,
    int nX, int nY, const ra::ui::drawing::ISurface& pSurface, const PopupLocations& pPopupLocations)
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    const auto nPopupLocation = pConfiguration.GetPopupLocation(vmPopup.GetPopupType());

    Position nPos;
    nPos.X = nX;
    nPos.Y = nY;

    // horizontal alignment
    switch (nPopupLocation)
    {
        default:
            break;
        case PopupLocation::TopMiddle:
        case PopupLocation::BottomMiddle:
            nPos.X = (ra::to_signed(pSurface.GetWidth()) - ra::to_signed(vmPopup.GetRenderImage().GetWidth())) / 2;
            break;
        case PopupLocation::TopRight:
        case PopupLocation::BottomRight:
            nPos.X = (ra::to_signed(pSurface.GetWidth()) - ra::to_signed(vmPopup.GetRenderImage().GetWidth())) - nPos.X;
            break;
    }

    // vertical alignment
    switch (nPopupLocation)
    {
        default:
            break;
        case PopupLocation::TopLeft:
            nPos.Y += pPopupLocations.nTopLeftY;
            break;
        case PopupLocation::TopMiddle:
            nPos.Y += pPopupLocations.nTopMiddleY;
            break;
        case PopupLocation::TopRight:
            nPos.Y += pPopupLocations.nTopRightY;
            break;
        case PopupLocation::BottomLeft:
            nPos.Y = pSurface.GetHeight() - vmPopup.GetRenderImage().GetHeight() - nPos.Y - pPopupLocations.nBottomLeftY;
            break;
        case PopupLocation::BottomMiddle:
            nPos.Y = pSurface.GetHeight() - vmPopup.GetRenderImage().GetHeight() - nPos.Y - pPopupLocations.nBottomMiddleY;
            break;
        case PopupLocation::BottomRight:
            nPos.Y = pSurface.GetHeight() - vmPopup.GetRenderImage().GetHeight() - nPos.Y - pPopupLocations.nBottomRightY;
            break;
    }

    return nPos;
}

void OverlayManager::UpdatePopup(ra::ui::drawing::ISurface& pSurface, const PopupLocations& pPopupLocations, double fElapsed, ra::ui::viewmodels::PopupViewModelBase& vmPopup)
{
    constexpr int nFudge = 4;
    unsigned nOldWidth = 0, nOldHeight = 0;

    if (!vmPopup.IsAnimationStarted())
    {
        vmPopup.BeginAnimation();
        fElapsed = 0.0;
        nOldWidth = nOldHeight = 0;
    }
    else
    {
        if (!vmPopup.HasRenderImage())
            vmPopup.UpdateRenderImage(0.0);

        const auto& pImage = vmPopup.GetRenderImage();
        nOldWidth = pImage.GetWidth();
        nOldHeight = pImage.GetHeight();
    }

    const auto nOldX = vmPopup.GetRenderLocationX();
    const auto nOldY = vmPopup.GetRenderLocationY();

    if (vmPopup.IsDestroyPending())
    {
        pSurface.FillRectangle(nOldX, nOldY, nOldWidth, nOldHeight, ra::ui::Color::Transparent);
        m_bRedrawAll = true;
        return;
    }

    m_bRedrawAll |= vmPopup.UpdateRenderImage(fElapsed);

    const auto& pNewImage = vmPopup.GetRenderImage();

    const auto nNewPos = GetRenderLocation(vmPopup, vmPopup.GetHorizontalOffset(),
        vmPopup.GetVerticalOffset(), pSurface, pPopupLocations);

    // eliminate any excess to the left or right of the popup
    if (nOldX != nNewPos.X || nOldWidth != pNewImage.GetWidth())
    {
        vmPopup.SetRenderLocationX(nNewPos.X);

        if (nOldWidth != 0)
        {
            const int nHeight = pNewImage.GetHeight() + std::abs(nNewPos.Y - nOldY);
            if (nNewPos.X > nOldX)
                pSurface.FillRectangle(nOldX - nFudge, nOldY, nNewPos.X - nOldX + nFudge, nHeight, ra::ui::Color::Transparent);

            const int nOldRightBound = nOldX + nOldWidth;
            const int nNewRightBound = nNewPos.X + pNewImage.GetWidth();
            if (nNewRightBound < nOldRightBound)
                pSurface.FillRectangle(nNewRightBound, nOldY, nOldRightBound - nNewRightBound + nFudge, nHeight, ra::ui::Color::Transparent);
        }

        m_bRedrawAll = true;
    }

    // eliminate any excess above or below the popup
    if (nOldY != nNewPos.Y || nOldHeight != pNewImage.GetHeight())
    {
        vmPopup.SetRenderLocationY(nNewPos.Y);

        if (nOldHeight != 0)
        {
            if (nNewPos.Y > nOldY)
                pSurface.FillRectangle(nNewPos.X, nOldY - nFudge, pNewImage.GetWidth(), nNewPos.Y - nOldY + nFudge, ra::ui::Color::Transparent);

            const int nOldLowerBound = nOldY + nOldHeight;
            const int nNewLowerBound = nNewPos.Y + pNewImage.GetHeight();
            if (nNewLowerBound < nOldLowerBound)
                pSurface.FillRectangle(nNewPos.X, nNewLowerBound, pNewImage.GetWidth(), nOldLowerBound - nNewLowerBound + nFudge, ra::ui::Color::Transparent);
        }

        m_bRedrawAll = true;
    }
}

void OverlayManager::AdjustLocationForPopup(PopupLocations& pPopupLocations, const ra::ui::viewmodels::PopupViewModelBase& vmPopup)
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    const auto nPopupLocation = pConfiguration.GetPopupLocation(vmPopup.GetPopupType());
    const auto nHeight = vmPopup.GetRenderImage().GetHeight();
    const auto nOffset = vmPopup.GetVerticalOffset() + nHeight;

    switch (nPopupLocation)
    {
        case PopupLocation::TopLeft:
            pPopupLocations.nTopLeftY += nOffset;
            break;
        case PopupLocation::TopMiddle:
            pPopupLocations.nTopMiddleY += nOffset;
            break;
        case PopupLocation::TopRight:
            pPopupLocations.nTopRightY += nOffset;
            break;
        case PopupLocation::BottomLeft:
            pPopupLocations.nBottomLeftY += nOffset;
            break;
        case PopupLocation::BottomMiddle:
            pPopupLocations.nBottomMiddleY += nOffset;
            break;
        case PopupLocation::BottomRight:
            pPopupLocations.nBottomRightY += nOffset;
            break;
    }
}

void OverlayManager::UpdateActiveMessage(ra::ui::drawing::ISurface& pSurface, PopupLocations& pPopupLocations, double fElapsed)
{
    assert(!m_vPopupMessages.empty());

    auto* pActiveMessage = m_vPopupMessages.front().get();
    Expects(pActiveMessage != nullptr);
    UpdatePopup(pSurface, pPopupLocations, fElapsed, *pActiveMessage);

    while (pActiveMessage->IsAnimationComplete() || pActiveMessage->IsDestroyPending())
    {
        m_vPopupMessages.pop_front();
        if (m_vPopupMessages.empty())
            break;

        pActiveMessage = m_vPopupMessages.front().get();
        Expects(pActiveMessage != nullptr);
        UpdatePopup(pSurface, pPopupLocations, fElapsed, *pActiveMessage);
    }

    if (!m_vPopupMessages.empty())
        AdjustLocationForPopup(pPopupLocations, *m_vPopupMessages.front());
}

void OverlayManager::UpdateActiveScoreboard(ra::ui::drawing::ISurface& pSurface, PopupLocations& pPopupLocations, double fElapsed)
{
    assert(!m_vScoreboards.empty());

    auto* pActiveScoreboard = &m_vScoreboards.front();
    UpdatePopup(pSurface, pPopupLocations, fElapsed, *pActiveScoreboard);

    while (pActiveScoreboard->IsAnimationComplete() || pActiveScoreboard->IsDestroyPending())
    {
        m_vScoreboards.pop_front();
        if (m_vScoreboards.empty())
            break;

        pActiveScoreboard = &m_vScoreboards.front();
        pActiveScoreboard->BeginAnimation();
        pActiveScoreboard->UpdateRenderImage(0.0);
    }

    if (!m_vScoreboards.empty())
        AdjustLocationForPopup(pPopupLocations, m_vScoreboards.front());
}

void OverlayManager::UpdateScoreTrackers(ra::ui::drawing::ISurface& pSurface, PopupLocations& pPopupLocations, double fElapsed)
{
    assert(!m_vScoreTrackers.empty());

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    const auto bEnabled = (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardTracker) != ra::ui::viewmodels::PopupLocation::None);

    auto pIter = m_vScoreTrackers.begin();
    while (pIter != m_vScoreTrackers.end())
    {
        auto& vmTracker = **pIter;

        if (bEnabled)
            UpdatePopup(pSurface, pPopupLocations, fElapsed, vmTracker);

        if (vmTracker.IsDestroyPending())
        {
            pIter = m_vScoreTrackers.erase(pIter);
        }
        else
        {
            if (bEnabled)
                AdjustLocationForPopup(pPopupLocations, vmTracker);

            ++pIter;
        }
    }
}

void OverlayManager::UpdateChallengeIndicators(ra::ui::drawing::ISurface& pSurface, PopupLocations& pPopupLocations, double fElapsed)
{
    assert(!m_vChallengeIndicators.empty());

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    const auto bEnabled = (pConfiguration.GetPopupLocation(ra::ui::viewmodels::Popup::Challenge) != ra::ui::viewmodels::PopupLocation::None);
    if (bEnabled)
    {
        // adjust offsets so the items appear horizontally stacked
        constexpr int nSpacing = 10;
        auto nRenderX = nSpacing;
        for (auto pIter = m_vChallengeIndicators.begin(); pIter != m_vChallengeIndicators.end(); ++pIter)
        {
            auto& vmNotification = **pIter;

            if (!vmNotification.IsDestroyPending())
            {
                vmNotification.SetHorizontalOffset(nRenderX);

                if (!vmNotification.HasRenderImage())
                    vmNotification.UpdateRenderImage(0.0f);

                nRenderX += vmNotification.GetRenderImage().GetWidth() + nSpacing;
            }
        }
    }

    auto pIter = m_vChallengeIndicators.begin();
    while (pIter != m_vChallengeIndicators.end())
    {
        auto& vmNotification = **pIter;

        if (bEnabled)
            UpdatePopup(pSurface, pPopupLocations, fElapsed, vmNotification);

        if (vmNotification.IsDestroyPending())
            pIter = m_vChallengeIndicators.erase(pIter);
        else
            ++pIter;
    }

    if (bEnabled && !m_vChallengeIndicators.empty())
        AdjustLocationForPopup(pPopupLocations, *m_vChallengeIndicators.front());
}

void OverlayManager::UpdateOverlay(ra::ui::drawing::ISurface& pSurface, double fElapsed)
{
    PopupLocations pPopupLocations;
    memset(&pPopupLocations, 0, sizeof(pPopupLocations));

    if (pSurface.GetWidth() != m_vmOverlay.GetRenderImage().GetWidth() ||
        pSurface.GetHeight() != m_vmOverlay.GetRenderImage().GetHeight())
    {
        m_vmOverlay.Resize(pSurface.GetWidth(), pSurface.GetHeight());
    }

    if (m_vmOverlay.CurrentState() == OverlayViewModel::State::FadeOut)
    {
        // when fading out, have to redraw the popups that were obscured by the overlay
        const int nOverlayRight = m_vmOverlay.GetHorizontalOffset() +
            m_vmOverlay.GetRenderImage().GetWidth();

        for (const auto& pChallengeIndicator : m_vChallengeIndicators)
            RenderPopupClippedX(pSurface, *pChallengeIndicator, nOverlayRight);

        for (const auto& pTracker : m_vScoreTrackers)
            RenderPopupClippedX(pSurface, *pTracker, nOverlayRight);

        if (!m_vScoreboards.empty())
            RenderPopupClippedX(pSurface, m_vScoreboards.front(), nOverlayRight);

        if (!m_vPopupMessages.empty())
            RenderPopupClippedX(pSurface, *m_vPopupMessages.front(), nOverlayRight);

        m_bRedrawAll = true;
    }

    UpdatePopup(pSurface, pPopupLocations, fElapsed, m_vmOverlay);
    if (m_bRedrawAll)
        RenderPopup(pSurface, m_vmOverlay);

    if (m_vmOverlay.CurrentState() == OverlayViewModel::State::Hidden)
    {
        m_vmOverlay.DestroyRenderImage();

        // fade out completed, render everything that was obscured by the overlay
        for (auto& pTracker : m_vScoreTrackers)
            RenderPopup(pSurface, *pTracker);

        if (!m_vScoreboards.empty())
            RenderPopup(pSurface, m_vScoreboards.front());

        if (!m_vPopupMessages.empty())
            RenderPopup(pSurface, *m_vPopupMessages.front());
    }
}

void OverlayManager::CaptureScreenshot(int nMessageId, const std::wstring& sPath)
{
    {
        std::lock_guard<std::mutex> pGuard(m_pScreenshotQueueMutex);

        if (!m_vScreenshotQueue.empty() && m_vScreenshotQueue.back().nFrameId == m_nFrameId)
        {
            m_vScreenshotQueue.back().vMessages.insert_or_assign(nMessageId, sPath);
            return;
        }

        auto& pScreenshot = m_vScreenshotQueue.emplace_back();
        pScreenshot.nFrameId = m_nFrameId;
        pScreenshot.vMessages.insert_or_assign(nMessageId, sPath);

        const auto& pWindowManager = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>();
        const auto& pDesktop = ra::services::ServiceLocator::Get<ra::ui::IDesktop>();
        pScreenshot.pScreen = pDesktop.CaptureClientArea(pWindowManager.Emulator);

        RA_LOG_INFO("Queued screenshot %s", sPath);

        if (m_bProcessingScreenshots)
            return;

        m_bProcessingScreenshots = true;
    }

    ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().RunAsync([this]()
    {
        ProcessScreenshots();
    });
}

void OverlayManager::ProcessScreenshots()
{
    std::map<std::wstring, std::unique_ptr<ra::ui::drawing::ISurface>> mReady;
    const auto& pImageRepository = ra::services::ServiceLocator::Get<ra::ui::IImageRepository>();

    {
        std::lock_guard<std::mutex> pGuard(m_pScreenshotQueueMutex);

        auto pScreenshotIter = m_vScreenshotQueue.begin();
        while (pScreenshotIter != m_vScreenshotQueue.end())
        {
            auto& pScreenshot = *pScreenshotIter;
            auto iter = pScreenshot.vMessages.begin();
            while (iter != pScreenshot.vMessages.end())
            {
                auto* pMessage = GetMessage(iter->first);
                if (pMessage == nullptr)
                {
                    // popup no longer available, discard request
                    iter = pScreenshot.vMessages.erase(iter);
                    RA_LOG_INFO("Popup no longer available for %s", iter->second);
                }
                else
                {
                    const auto& pImageReference = pMessage->GetImage();
                    if (pImageRepository.IsImageAvailable(pImageReference.Type(), pImageReference.Name()))
                    {
                        RA_LOG_INFO("Rendering screenshot for %s", iter->second);

                        if (!pMessage->IsAnimationStarted())
                            pMessage->BeginAnimation();

                        pMessage->UpdateRenderImage(0.0);

                        mReady.insert_or_assign(iter->second, RenderScreenshot(*pScreenshot.pScreen, *pMessage));
                        iter = pScreenshot.vMessages.erase(iter);
                    }
                    else
                    {
                        RA_LOG_INFO("Image no longer available for %s", iter->second);
                        ++iter;
                    }
                }
            }

            if (pScreenshot.vMessages.empty())
                pScreenshotIter = m_vScreenshotQueue.erase(pScreenshotIter);
            else
                ++pScreenshotIter;
        }

        if (m_vScreenshotQueue.empty())
        {
            m_bProcessingScreenshots = false;
        }
        else
        {
            ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().ScheduleAsync(std::chrono::milliseconds(200), [this]()
            {
                ProcessScreenshots();
            });
        }
    }

    const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
    for (const auto& pFile : mReady)
    {
        RA_LOG_INFO("Saving screenshot %s", pFile.first);
        pSurfaceFactory.SaveImage(*pFile.second, pFile.first);
    }
}

std::unique_ptr<ra::ui::drawing::ISurface> OverlayManager::RenderScreenshot(const ra::ui::drawing::ISurface& pClientSurface, const PopupMessageViewModel& vmPopup)
{
    const auto& pSurfaceFactory = ra::services::ServiceLocator::Get<ra::ui::drawing::ISurfaceFactory>();
    auto pSurface = pSurfaceFactory.CreateSurface(pClientSurface.GetWidth(), pClientSurface.GetHeight());

    pSurface->DrawSurface(0, 0, pClientSurface);

    PopupLocations pPopupLocations;
    memset(&pPopupLocations, 0, sizeof(pPopupLocations));
    const auto nPos = GetRenderLocation(vmPopup, vmPopup.GetHorizontalOffset(), vmPopup.GetTargetVerticalOffset(), *pSurface, pPopupLocations);
    const auto& pPopupImage = vmPopup.GetRenderImage();

    if (ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>().Transparent())
    {
        auto pTransparentPopup = pSurfaceFactory.CreateTransparentSurface(pPopupImage.GetWidth(), pPopupImage.GetHeight());
        pTransparentPopup->DrawSurface(0, 0, pPopupImage);
        pTransparentPopup->SetOpacity(0.90);
        pSurface->DrawSurface(nPos.X, nPos.Y, *pTransparentPopup);
    }
    else
    {
        pSurface->DrawSurface(nPos.X, nPos.Y, pPopupImage);
    }

    return pSurface;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
