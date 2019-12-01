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

static int GetAbsolutePosition(int nValue, RelativePosition nRelativePosition, unsigned int nFarValue) noexcept
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

    const auto& pImage = vmPopup.GetRenderImage();
    const int nVisibleWidth = nX + ra::to_signed(pImage.GetWidth()) - nClipX;
    if (nVisibleWidth > 0)
        pSurface.DrawSurface(nClipX, nY, pImage, pImage.GetWidth() - nVisibleWidth, 0, nVisibleWidth, pImage.GetHeight());
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

    if (!m_vPopupMessages.empty() || !m_vScoreboards.empty() || !m_vScoreTrackers.empty())
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

    const bool bNeedsRender = NeedsRender();
    if (m_bIsRendering != bNeedsRender)
    {
        if (bNeedsRender)
        {
            auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
            m_tLastRender = pClock.UpTime();

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
        // update the visible popups - if any are updated, m_bRedrawAll will be set
        if (!m_vScoreboards.empty())
        {
            UpdateActiveScoreboard(pSurface, fElapsed);

            // a visible scoreboard may not set m_bRedrawAll if it's in the pause portion of its animation loop
            // make sure we keep the render cycle going so it'll eventually handle when the pause ends
            bRequestRender = true;
        }

        if (!m_vScoreTrackers.empty())
            UpdateScoreTrackers(pSurface, fElapsed);

        if (!m_vPopupMessages.empty())
        {
            UpdateActiveMessage(pSurface, fElapsed);

            // a visible popup may not set m_bRedrawAll if it's in the pause portion of its animation loop
            // make sure we keep the render cycle going so it'll eventually handle when the pause ends
            bRequestRender = true;
        }

        // if anything changed, or caller requested a repaint, do so now
        if (m_bRedrawAll)
        {
            if (!m_vScoreboards.empty())
                RenderPopup(pSurface, m_vScoreboards.front());
            for (const auto& pScoreTracker : m_vScoreTrackers)
                RenderPopup(pSurface, *pScoreTracker);
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

    for (auto& pTracker : m_vScoreTrackers)
        pTracker->SetDestroyPending();

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
        m_bRedrawAll = true;
        return;
    }

    const bool bUpdated = vmPopup.UpdateRenderImage(fElapsed);

    const int nNewX = GetAbsolutePosition(vmPopup.GetRenderLocationX(), vmPopup.GetRenderLocationXRelativePosition(), pSurface.GetWidth());
    const int nNewY = GetAbsolutePosition(vmPopup.GetRenderLocationY(), vmPopup.GetRenderLocationYRelativePosition(), pSurface.GetHeight());

    if (bUpdated)
    {
        // if the image moved, we have to blank out the area it previously covered
        if (nOldY != nNewY || nOldX != nNewX)
        {
            const auto& pImage = vmPopup.GetRenderImage();
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

        m_bRedrawAll = true;
    }
}

void OverlayManager::UpdateActiveMessage(ra::ui::drawing::ISurface& pSurface, double fElapsed)
{
    assert(!m_vPopupMessages.empty());

    auto* pActiveMessage = m_vPopupMessages.front().get();
    Expects(pActiveMessage != nullptr);
    UpdatePopup(pSurface, fElapsed, *pActiveMessage);

    while (pActiveMessage->IsAnimationComplete() || pActiveMessage->IsDestroyPending())
    {
        m_vPopupMessages.pop_front();
        if (m_vPopupMessages.empty())
            break;

        pActiveMessage = m_vPopupMessages.front().get();
        Expects(pActiveMessage != nullptr);
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
            RenderPopupClippedX(pSurface, *m_vPopupMessages.front(), nOverlayRight);

        m_bRedrawAll = true;
    }

    UpdatePopup(pSurface, fElapsed, m_vmOverlay);
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

                        PopupMessageViewModel::RenderImageLock lock(*pMessage);

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

    const int nX = GetAbsolutePosition(vmPopup.GetRenderLocationX(), vmPopup.GetRenderLocationXRelativePosition(), pSurface->GetWidth());
    const int nY = GetAbsolutePosition(vmPopup.GetRenderTargetY(), vmPopup.GetRenderLocationYRelativePosition(), pSurface->GetHeight());
    const auto& pPopupImage = vmPopup.GetRenderImage();

    if (ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>().Transparent())
    {
        auto pTransparentPopup = pSurfaceFactory.CreateTransparentSurface(pPopupImage.GetWidth(), pPopupImage.GetHeight());
        pTransparentPopup->DrawSurface(0, 0, pPopupImage);
        pTransparentPopup->SetOpacity(0.90);
        pSurface->DrawSurface(nX, nY, *pTransparentPopup);
    }
    else
    {
        pSurface->DrawSurface(nX, nY, pPopupImage);
    }

    return pSurface;
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
