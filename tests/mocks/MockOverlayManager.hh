#ifndef RA_SERVICES_MOCK_OVERLAYMANAGER_HH
#define RA_SERVICES_MOCK_OVERLAYMANAGER_HH
#pragma once

#include "ui\viewmodels\OverlayManager.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace viewmodels {
namespace mocks {

class MockOverlayManager : public OverlayManager
{
public:
    MockOverlayManager() noexcept
        : m_Override(this)
    {
    }

    void ClearPopups() noexcept override
    {
        // base implementation only queues the popups to be destroyed

        while (!m_vPopupMessages.empty())
            m_vPopupMessages.pop_front();

        while (!m_vScoreboards.empty())
            m_vScoreboards.pop_front();

        m_vScoreTrackers.clear();
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::ui::viewmodels::OverlayManager> m_Override;
};

} // namespace mocks
} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_SERVICES_MOCK_OVERLAYMANAGER_HH
