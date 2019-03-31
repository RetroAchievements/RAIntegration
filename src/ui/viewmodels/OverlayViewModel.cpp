#include "OverlayViewModel.hh"

#include "data\EmulatorContext.hh"
#include "data\UserContext.hh"

#include "services\IConfiguration.hh"

#include "ui\IDesktop.hh"
#include "ui\viewmodels\WindowManager.hh"

#include "ra_math.h"

namespace ra {
namespace ui {
namespace viewmodels {

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
    else
    {
        //const auto& pImageRepository = ra::services::ServiceLocator::Get<ra::ui::IImageRepository>();
        //if (pImageRepository.HasReferencedImageChanged(m_hImage))
        //{
        //    CreateRenderImage();
        //    bUpdated = true;
        //}
    }

    return bUpdated;
}

void OverlayViewModel::CreateRenderImage()
{
    Expects(m_pSurface != nullptr);

    const int nWidth = m_pSurface->GetWidth();

    // background image
    const ImageReference pOverlayBackground(ra::ui::ImageType::Local, "Overlay\\overlayBG.png");
    m_pSurface->DrawImageStretched(0, 0, nWidth, m_pSurface->GetHeight(), pOverlayBackground);

    // user frame
    const auto nFont = m_pSurface->LoadFont("Tahoma", 26, ra::ui::FontStyles::Normal);

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

    int nX = nWidth - nUserFrameWidth - nMargin;
    if (nX > 200)
    {
        int nY = nMargin;

        m_pSurface->FillRectangle(nX, nY, nUserFrameWidth, nUserFrameHeight, ra::ui::Color(32, 32, 32));

        const ImageReference pUserImage(ra::ui::ImageType::UserPic, pUserContext.GetUsername());
        m_pSurface->DrawImage(nWidth - nMargin - nPadding - nImageSize, nY + nPadding, nImageSize, nImageSize, pUserImage);

        const auto nTextColor = ra::ui::Color(17, 102, 221);
        m_pSurface->WriteText(nX + nPadding, nY + nPadding, nFont, nTextColor, sUserName);
        m_pSurface->WriteText(nX + nPadding, nY + nUserFrameHeight - nPadding - szPoints.Height, nFont, nTextColor, sPoints);
    }

    // hardcore indicator
    if (ra::services::ServiceLocator::Get<ra::services::IConfiguration>().IsFeatureEnabled(ra::services::Feature::Hardcore))
    {
        const auto nHardcoreFont = m_pSurface->LoadFont("Tahoma", 22, ra::ui::FontStyles::Normal);
        const std::wstring sHardcore = L"HARDCORE";
        const auto szHardcore = m_pSurface->MeasureText(nHardcoreFont, sHardcore);
        m_pSurface->WriteText(nWidth - nMargin - szHardcore.Width - nPadding, nMargin + nUserFrameHeight,
            nHardcoreFont, ra::ui::Color(255, 0, 0), sHardcore);
    }
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
        // TODO: let current page handle keypress.
        const bool bHandled = false;

        // if the current page didn't handle the keypress and cancel is pressed, close the overlay
        if (!bHandled && pInput.m_bCancelPressed)
            Deactivate();
    }
}

void OverlayViewModel::Activate()
{
    switch (m_nState)
    {
        case State::Hidden:
            m_nState = State::FadeIn;
            BeginAnimation();
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

} // namespace viewmodels
} // namespace ui
} // namespace ra
