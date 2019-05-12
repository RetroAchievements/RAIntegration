#include "OverlayViewModel.hh"

#include "ra_math.h"
#include "RA_StringUtils.h"

#include "data\EmulatorContext.hh"
#include "data\UserContext.hh"

#include "services\IClock.hh"
#include "services\IConfiguration.hh"

#include "ui\IDesktop.hh"
#include "ui\OverlayTheme.hh"
#include "ui\viewmodels\OverlayAchievementsPageViewModel.hh"
#include "ui\viewmodels\OverlayLeaderboardsPageViewModel.hh"
#include "ui\viewmodels\OverlayManager.hh"
#include "ui\viewmodels\WindowManager.hh"

namespace ra {
namespace ui {
namespace viewmodels {

const StringModelProperty OverlayViewModel::PageViewModel::TitleProperty("PageViewModel", "Title", L"");

void OverlayViewModel::BeginAnimation()
{
    m_fAnimationProgress = 0.0;

    const auto& vmEmulator = ra::services::ServiceLocator::Get<ra::ui::viewmodels::WindowManager>().Emulator;
    const auto szEmulator = ra::services::ServiceLocator::Get<ra::ui::IDesktop>().GetClientSize(vmEmulator);

    Resize(szEmulator.Width, szEmulator.Height);

    SetRenderLocationX(-szEmulator.Width);
    SetRenderLocationY(0);
}

void OverlayViewModel::Resize(int nWidth, int nHeight)
{
    m_pSurface =
        ra::services::ServiceLocator::GetMutable<ra::ui::drawing::ISurfaceFactory>().CreateSurface(nWidth, nHeight);
    m_bSurfaceStale = true;
}

bool OverlayViewModel::UpdateRenderImage(double fElapsed)
{
    if (m_nState == State::Hidden)
        return false;

    if (CurrentPage().Update(fElapsed))
        m_bSurfaceStale = true;

    bool bUpdated = false;
    if (m_nState != State::Visible)
    {
        const int nWidth = m_pSurface->GetWidth();
        const int nOldX = GetRenderLocationX();

        m_fAnimationProgress += fElapsed;

        if (m_nState == State::FadeIn)
        {
            double nOffset = 0;
            if (m_fAnimationProgress < INOUT_TIME)
            {
                const auto fPercentage = (INOUT_TIME - m_fAnimationProgress) / INOUT_TIME;
                nOffset = nWidth * (fPercentage * fPercentage);
            }

            const int nNewX = ftol(-nOffset);
            if (nNewX != nOldX)
            {
                SetRenderLocationX(nNewX);
                bUpdated = true;

                if (nNewX == 0)
                    m_nState = State::Visible;
            }
        }
        else if (m_nState == State::FadeOut)
        {
            double nOffset = nWidth;
            if (m_fAnimationProgress < INOUT_TIME)
            {
                const auto fPercentage = m_fAnimationProgress / INOUT_TIME;
                nOffset = nWidth * (fPercentage * fPercentage);
            }

            const int nNewX = ftol(-nOffset);
            if (nNewX != nOldX)
            {
                SetRenderLocationX(nNewX);
                bUpdated = true;

                if (nNewX == -nWidth)
                {
                    m_nState = State::Hidden;
                    m_fAnimationProgress = -1.0;
                    ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().Unpause();
                    return true;
                }
            }
        }
    }

    if (m_bSurfaceStale)
    {
        m_bSurfaceStale = false;
        CreateRenderImage();
        bUpdated = true;
    }

    return bUpdated;
}

void OverlayViewModel::CreateRenderImage()
{
    Expects(m_pSurface != nullptr);

    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::OverlayTheme>();
    const int nWidth = m_pSurface->GetWidth();
    const int nHeight = m_pSurface->GetHeight();

    // background image
    const ImageReference pOverlayBackground(ra::ui::ImageType::Local, "Overlay\\overlayBG.png");
    m_pSurface->DrawImageStretched(0, 0, nWidth, nHeight, pOverlayBackground);

    // user frame
    const auto nFont =
        m_pSurface->LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlayHeader(), ra::ui::FontStyles::Normal);

    const auto& pUserContext = ra::services::ServiceLocator::Get<ra::data::UserContext>();
    const auto sUserName = ra::Widen(pUserContext.GetUsername());
    const auto szUsername = m_pSurface->MeasureText(nFont, sUserName);

    const auto sPoints = ra::StringPrintf(L"%u Points", pUserContext.GetScore());
    const auto szPoints = m_pSurface->MeasureText(nFont, sPoints);

    const auto nImageSize = 64;
    const auto nMargin = 8;
    const auto nPadding = 4;
    const auto nUserFrameHeight = nImageSize + nPadding * 2;
    const auto nUserFrameWidth = nImageSize + nPadding * 4 + std::max(szUsername.Width, szPoints.Width);

    const int nX = nWidth - nUserFrameWidth - nMargin;
    if (nX > 200)
    {
        const int nY = nMargin;

        m_pSurface->FillRectangle(nX, nY, nUserFrameWidth, nUserFrameHeight, pTheme.ColorOverlayPanel());

        const ImageReference pUserImage(ra::ui::ImageType::UserPic, pUserContext.GetUsername());
        m_pSurface->DrawImage(nWidth - nMargin - nPadding - nImageSize, nY + nPadding, nImageSize, nImageSize,
                              pUserImage);

        m_pSurface->WriteText(nX + nPadding, nY + nPadding, nFont, pTheme.ColorOverlayText(), sUserName);
        m_pSurface->WriteText(nX + nPadding, nY + nUserFrameHeight - nPadding - szPoints.Height, nFont,
                              pTheme.ColorOverlayText(), sPoints);
    }

    // hardcore indicator
    if (ra::services::ServiceLocator::Get<ra::services::IConfiguration>().IsFeatureEnabled(
            ra::services::Feature::Hardcore))
    {
        const auto nHardcoreFont =
            m_pSurface->LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlaySummary(), ra::ui::FontStyles::Normal);
        const std::wstring sHardcore = L"HARDCORE";
        const auto szHardcore = m_pSurface->MeasureText(nHardcoreFont, sHardcore);
        m_pSurface->WriteText(nWidth - nMargin - szHardcore.Width - nPadding, nMargin + nUserFrameHeight, nHardcoreFont,
                              pTheme.ColorError(), sHardcore);
    }

    // page header
    const auto& pCurrentPage = CurrentPage();
    const auto nPageHeaderFont =
        m_pSurface->LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlayTitle(), ra::ui::FontStyles::Normal);
    m_pSurface->WriteText(nMargin, nMargin, nPageHeaderFont, pTheme.ColorOverlayText(), pCurrentPage.GetTitle());

    // page content
    pCurrentPage.Render(*m_pSurface, nMargin, nMargin + nUserFrameHeight, nWidth - nMargin * 2,
                        nHeight - nMargin * 2 - nUserFrameHeight);

    // navigation controls
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    const auto sBack = ra::StringPrintf(L"%s: Back", pEmulatorContext.GetCancelButtonText());
    const auto sSelect = ra::StringPrintf(L"%s: Select", pEmulatorContext.GetAcceptButtonText());
    const auto sNext = std::wstring(L"\u2BC8: Next");
    const auto sPrev = std::wstring(L"\u2BC7: Prev");

    const auto nNavFont =
        m_pSurface->LoadFont(pTheme.FontOverlay(), pTheme.FontSizeOverlaySummary(), ra::ui::FontStyles::Normal);
    const auto szBack = m_pSurface->MeasureText(nNavFont, sBack);
    const auto szSelect = m_pSurface->MeasureText(nNavFont, sSelect);

    const int nControlsX2 = std::max(szBack.Width, szSelect.Width) + nMargin + nPadding + 2;
    const int nControlsX1 = nControlsX2 + 72;
    const int nControlsY2 = nHeight - nMargin - nPadding - szSelect.Height;
    const int nControlsY1 = nControlsY2 - szBack.Height - nPadding;

    m_pSurface->FillRectangle(nWidth - nControlsX1 - nPadding, nControlsY1 - nPadding, nControlsX1 - nPadding,
                              nHeight - nControlsY1 - nPadding, pTheme.ColorOverlayPanel());
    m_pSurface->WriteText(nWidth - nControlsX1, nControlsY1, nNavFont, pTheme.ColorOverlayText(), sNext);
    m_pSurface->WriteText(nWidth - nControlsX1, nControlsY2, nNavFont, pTheme.ColorOverlayText(), sPrev);
    m_pSurface->WriteText(nWidth - nControlsX2, nControlsY1, nNavFont, pTheme.ColorOverlayText(), sSelect);
    m_pSurface->WriteText(nWidth - nControlsX2, nControlsY2, nNavFont, pTheme.ColorOverlayText(), sBack);
}

void OverlayViewModel::RefreshOverlay()
{
    ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::OverlayManager>().RequestRender();
}

void OverlayViewModel::ProcessInput(const ControllerInput& pInput)
{
    // only process input when fully visible
    if (m_nState != State::Visible)
        return;

    NavButton nPrioritizedButton = NavButton::None;
    if (pInput.m_bConfirmPressed)
        nPrioritizedButton = NavButton::Confirm;
    else if (pInput.m_bCancelPressed)
        nPrioritizedButton = NavButton::Cancel;
    else if (pInput.m_bDownPressed)
        nPrioritizedButton = NavButton::Down;
    else if (pInput.m_bUpPressed)
        nPrioritizedButton = NavButton::Up;
    else if (pInput.m_bRightPressed)
        nPrioritizedButton = NavButton::Right;
    else if (pInput.m_bLeftPressed)
        nPrioritizedButton = NavButton::Left;
    else if (pInput.m_bQuitPressed)
        nPrioritizedButton = NavButton::Quit;

    if (m_nCurrentButton == nPrioritizedButton)
    {
        if (nPrioritizedButton == NavButton::None) // nothing pressed, nothing to do
            return;

        // if the user holds the button down for 600ms, start pulsing the button every 200ms
        const auto tNow = ra::services::ServiceLocator::Get<ra::services::IClock>().UpTime();
        if (tNow - m_tCurrentButtonPressed < std::chrono::milliseconds(600))
            return;

        m_tCurrentButtonPressed += std::chrono::milliseconds(200);
    }
    else
    {
        // button changed, start the hold timer
        m_nCurrentButton = nPrioritizedButton;
        if (nPrioritizedButton == NavButton::None) // nothing pressed, nothing to do
            return;

        m_tCurrentButtonPressed = ra::services::ServiceLocator::Get<ra::services::IClock>().UpTime();
    }

    if (nPrioritizedButton == NavButton::Quit)
    {
        Deactivate();
    }
    else
    {
        bool bHandled = CurrentPage().ProcessInput(pInput);
        if (!bHandled)
        {
            if (nPrioritizedButton == NavButton::Left)
            {
                if (m_nSelectedPage-- == 0)
                    m_nSelectedPage = m_vPages.size() - 1;

                CurrentPage().Refresh();
                bHandled = true;
            }
            else if (nPrioritizedButton == NavButton::Right)
            {
                if (++m_nSelectedPage == ra::to_signed(m_vPages.size()))
                    m_nSelectedPage = 0;

                CurrentPage().Refresh();
                bHandled = true;
            }
            else if (nPrioritizedButton == NavButton::Cancel)
            {
                // if the current page didn't handle the keypress and cancel is pressed, close the overlay
                Deactivate();
            }
        }

        if (bHandled)
        {
            m_bSurfaceStale = true;
            RefreshOverlay();
        }
    }
}

void OverlayViewModel::Activate()
{
    switch (m_nState)
    {
        case State::Hidden:
            m_nState = State::FadeIn;
            BeginAnimation();

            if (m_vPages.empty())
                PopulatePages();

            CurrentPage().Refresh();
            RefreshOverlay();
            break;

        case State::FadeOut:
            m_nState = State::FadeIn;
            m_fAnimationProgress = INOUT_TIME - m_fAnimationProgress;
            break;
    }
}

void OverlayViewModel::Deactivate()
{
    switch (m_nState)
    {
        case State::Visible:
            m_nState = State::FadeOut;
            m_fAnimationProgress = 0.0;
            RefreshOverlay();
            break;

        case State::FadeIn:
            m_nState = State::FadeOut;
            m_fAnimationProgress = INOUT_TIME - m_fAnimationProgress;
            SetRenderLocationX(GetRenderImage().GetWidth() - GetRenderLocationX());
            break;
    }
}

void OverlayViewModel::PopulatePages()
{
    m_vPages.resize(2);

    auto pAchievementsPage = std::make_unique<OverlayAchievementsPageViewModel>();
    m_vPages.at(0) = std::move(pAchievementsPage);

    auto pLeaderboardsPage = std::make_unique<OverlayLeaderboardsPageViewModel>();
    m_vPages.at(1) = std::move(pLeaderboardsPage);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
