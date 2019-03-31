#include "OverlayViewModel.hh"

#include "data\EmulatorContext.hh"

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

    m_pSurface->FillRectangle(0, 0, m_pSurface->GetWidth(), m_pSurface->GetHeight(), ra::ui::Color(0x60, 0x80, 0xA0));
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
