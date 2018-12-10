#include "OverlayManager.hh"

#include "RA_LeaderboardPopup.h"

#include "services\IClock.hh"
#include "services\ServiceLocator.hh"

#ifndef RA_UTEST
LeaderboardPopup g_LeaderboardPopups;
#endif

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty PopupViewModelBase::RenderLocationXProperty("PopupViewModelBase", "RenderLocationX", 0);
const IntModelProperty PopupViewModelBase::RenderLocationYProperty("PopupViewModelBase", "RenderLocationY", 0);
const IntModelProperty PopupViewModelBase::RenderLocationXRelativePositionProperty("PopupViewModelBase", "RenderLocationXRelativePosition", ra::etoi(RelativePosition::Near));
const IntModelProperty PopupViewModelBase::RenderLocationYRelativePositionProperty("PopupViewModelBase", "RenderLocationYRelativePosition", ra::etoi(RelativePosition::Near));

void OverlayManager::Update(double fElapsed)
{
    if (!m_vPopupMessages.empty())
        UpdateActiveMessage(fElapsed);

#ifndef RA_UTEST
    g_LeaderboardPopups.Update(fElapsed);
#endif
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
            return static_cast<int>((nFarValue - nValue) / 2);
        case RelativePosition::Far:
            return static_cast<int>(nFarValue - nValue);
        case RelativePosition::After:
            return static_cast<int>(nFarValue + nValue);
    }
}

void OverlayManager::Render(ra::ui::drawing::ISurface& pSurface) const
{
    if (!m_vPopupMessages.empty())
    {
        const auto& pActiveMessage = m_vPopupMessages.front();
        int nX = GetAbsolutePosition(pActiveMessage.GetRenderLocationX(), pActiveMessage.GetRenderLocationXRelativePosition(), pSurface.GetWidth());
        int nY = GetAbsolutePosition(pActiveMessage.GetRenderLocationY(), pActiveMessage.GetRenderLocationYRelativePosition(), pSurface.GetHeight());
        pSurface.DrawSurface(nX, nY, pActiveMessage.GetRenderImage());
    }

#ifndef RA_UTEST
    g_LeaderboardPopups.Render(pSurface);
#endif
}

void OverlayManager::QueueMessage(PopupMessageViewModel&& pMessage)
{
    if (m_vPopupMessages.empty())
        pMessage.BeginAnimation();

    if (pMessage.GetImage().Type() != ra::ui::ImageType::None)
    {
        auto& pImageRepository = ra::services::ServiceLocator::GetMutable<ra::ui::IImageRepository>();
        pImageRepository.FetchImage(pMessage.GetImage().Type(), pMessage.GetImage().Name());
    }

    m_vPopupMessages.emplace(std::move(pMessage));
}

void OverlayManager::ClearPopups()
{
    while (!m_vPopupMessages.empty())
        m_vPopupMessages.pop();
}

void OverlayManager::UpdateActiveMessage(double fElapsed)
{
    assert(!m_vPopupMessages.empty());
    auto* pActiveMessage = &m_vPopupMessages.front();
    do
    {
        pActiveMessage->UpdateRenderImage(fElapsed);
        if (!pActiveMessage->IsAnimationComplete())
            return;

        m_vPopupMessages.pop();
        if (m_vPopupMessages.empty())
            return;

        pActiveMessage = &m_vPopupMessages.front();
        pActiveMessage->BeginAnimation();
    } while (true);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
