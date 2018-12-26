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

private:
    ra::services::ServiceLocator::ServiceOverride<ra::ui::viewmodels::OverlayManager> m_Override;
};

} // namespace mocks
} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_SERVICES_MOCK_OVERLAYMANAGER_HH
