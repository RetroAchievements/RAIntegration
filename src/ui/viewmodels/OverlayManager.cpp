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
            return ra::to_signed((nFarValue - nValue) / 2);
        case RelativePosition::Far:
            return ra::to_signed(nFarValue - nValue);
        case RelativePosition::After:
            return ra::to_signed(nFarValue + nValue);
    }
}

void OverlayManager::Render(ra::ui::drawing::ISurface& pSurface) const
{
#ifndef RA_UTEST
    g_LeaderboardPopups.Render(pSurface);
#endif

    if (!m_vPopupMessages.empty())
    {
        const auto& pActiveMessage = m_vPopupMessages.front();
        const int nX = GetAbsolutePosition(pActiveMessage.GetRenderLocationX(), pActiveMessage.GetRenderLocationXRelativePosition(), pSurface.GetWidth());
        const int nY = GetAbsolutePosition(pActiveMessage.GetRenderLocationY(), pActiveMessage.GetRenderLocationYRelativePosition(), pSurface.GetHeight());
        pSurface.DrawSurface(nX, nY, pActiveMessage.GetRenderImage());
    }
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

} // namespace viewmodels
} // namespace ui
} // namespace ra
