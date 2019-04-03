#include "OverlayViewModel.hh"

#include "ra_math.h"

#include "data\EmulatorContext.hh"
#include "data\UserContext.hh"

#include "services\IConfiguration.hh"

#include "ui\IDesktop.hh"
#include "ui\OverlayTheme.hh"
#include "ui\viewmodels\OverlayAchievementsPageViewModel.hh"
#include "ui\viewmodels\OverlayLeaderboardsPageViewModel.hh"
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

    m_pSurface = ra::services::ServiceLocator::GetMutable<ra::ui::drawing::ISurfaceFactory>().CreateSurface(szEmulator.Width, szEmulator.Height);
    m_bSurfaceStale = true;

    SetRenderLocationX(-szEmulator.Width);
    SetRenderLocationY(0);
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
                {
                    m_nState = State::Visible;
                    m_fAnimationProgress = -1.0;
                }
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
                    m_bSurfaceStale = false;
                    m_pSurface.reset();
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
    const auto nFont = m_pSurface->LoadFont(FONT_TO_USE, FONT_SIZE_HEADER, ra::ui::FontStyles::Normal);

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
        m_pSurface->DrawImage(nWidth - nMargin - nPadding - nImageSize, nY + nPadding, nImageSize, nImageSize, pUserImage);

        m_pSurface->WriteText(nX + nPadding, nY + nPadding, nFont, pTheme.ColorOverlayText(), sUserName);
        m_pSurface->WriteText(nX + nPadding, nY + nUserFrameHeight - nPadding - szPoints.Height, nFont, pTheme.ColorOverlayText(), sPoints);
    }

    // hardcore indicator
    if (ra::services::ServiceLocator::Get<ra::services::IConfiguration>().IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        const auto nHardcoreFont = m_pSurface->LoadFont(FONT_TO_USE, FONT_SIZE_SUMMARY, ra::ui::FontStyles::Normal);
        const std::wstring sHardcore = L"HARDCORE";
        const auto szHardcore = m_pSurface->MeasureText(nHardcoreFont, sHardcore);
        m_pSurface->WriteText(nWidth - nMargin - szHardcore.Width - nPadding, nMargin + nUserFrameHeight,
            nHardcoreFont, pTheme.ColorError(), sHardcore);
    }

    // page header
    const auto& pCurrentPage = CurrentPage();
    const auto nPageHeaderFont = m_pSurface->LoadFont(FONT_TO_USE, FONT_SIZE_TITLE, ra::ui::FontStyles::Normal);
    m_pSurface->WriteText(nMargin, nMargin, nPageHeaderFont, pTheme.ColorOverlayText(), pCurrentPage.GetTitle());

    // page content
    pCurrentPage.Render(*m_pSurface, nMargin, nMargin + nUserFrameHeight, nWidth - nMargin * 2, nHeight - nMargin * 2 - nUserFrameHeight);

    // navigation controls
    const auto& pEmulatorContext = ra::services::ServiceLocator::Get<ra::data::EmulatorContext>();
    const auto sBack = ra::StringPrintf(L"%s: Back", pEmulatorContext.GetCancelButtonText());
    const auto sSelect = ra::StringPrintf(L"%s: Select", pEmulatorContext.GetAcceptButtonText());
    const auto sNext = std::wstring(L"\u2BC8: Next");
    const auto sPrev = std::wstring(L"\u2BC7: Prev");

    const auto nNavFont = m_pSurface->LoadFont(FONT_TO_USE, FONT_SIZE_SUMMARY, ra::ui::FontStyles::Normal);
    const auto szBack = m_pSurface->MeasureText(nNavFont, sBack);
    const auto szSelect = m_pSurface->MeasureText(nNavFont, sSelect);

    const int nControlsX2 = std::max(szBack.Width, szSelect.Width) + nMargin + nPadding + 2;
    const int nControlsX1 = nControlsX2 + 72;
    const int nControlsY2 = nHeight - nMargin - nPadding - szSelect.Height;
    const int nControlsY1 = nControlsY2 - szBack.Height - nPadding;

    m_pSurface->FillRectangle(nWidth - nControlsX1 - nPadding, nControlsY1 - nPadding,
        nControlsX1 - nPadding, nHeight - nControlsY1 - nPadding, pTheme.ColorOverlayPanel());
    m_pSurface->WriteText(nWidth - nControlsX1, nControlsY1, nNavFont, pTheme.ColorOverlayText(), sNext);
    m_pSurface->WriteText(nWidth - nControlsX1, nControlsY2, nNavFont, pTheme.ColorOverlayText(), sPrev);
    m_pSurface->WriteText(nWidth - nControlsX2, nControlsY1, nNavFont, pTheme.ColorOverlayText(), sSelect);
    m_pSurface->WriteText(nWidth - nControlsX2, nControlsY2, nNavFont, pTheme.ColorOverlayText(), sBack);
}

void OverlayViewModel::ProcessInput(const ControllerInput& pInput)
{
    if (m_nState == State::Hidden)
        return;

    if (m_bInputLock)
    {
        if (!pInput.m_bUpPressed && !pInput.m_bDownPressed &&
            !pInput.m_bLeftPressed && !pInput.m_bRightPressed &&
            !pInput.m_bConfirmPressed && !pInput.m_bCancelPressed)
        {
            m_bInputLock = false;
        }

        return;
    }

    if (pInput.m_bQuitPressed)
    {
        Deactivate();
    }
    else
    {
        bool bHandled = CurrentPage().ProcessInput(pInput);
        if (!bHandled)
        {
            if (pInput.m_bLeftPressed)
            {
                if (m_nSelectedPage-- == 0)
                    m_nSelectedPage = m_vPages.size() - 1;

                CurrentPage().Refresh();
                bHandled = true;
            }
            else if (pInput.m_bRightPressed)
            {
                if (++m_nSelectedPage == ra::to_signed(m_vPages.size()))
                    m_nSelectedPage = 0;

                CurrentPage().Refresh();
                bHandled = true;
            }
            else if (pInput.m_bCancelPressed)
            {
                // if the current page didn't handle the keypress and cancel is pressed, close the overlay
                Deactivate();
                m_bInputLock = true;
            }
        }

        if (bHandled)
        {
            m_bSurfaceStale = true;
            m_bInputLock = true;
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
            ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().Unpause();
            break;
            
        case State::FadeIn:
            m_nState = State::FadeOut;
            m_fAnimationProgress = INOUT_TIME - m_fAnimationProgress;
            ra::services::ServiceLocator::Get<ra::data::EmulatorContext>().Unpause();
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
