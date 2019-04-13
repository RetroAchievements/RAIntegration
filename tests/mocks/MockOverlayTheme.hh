#ifndef RA_SERVICES_MOCK_OVERLAY_THEME_HH
#define RA_SERVICES_MOCK_OVERLAY_THEME_HH
#pragma once

#include "ui\OverlayTheme.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace mocks {

class MockOverlayTheme : public OverlayTheme
{
public:
    MockOverlayTheme() noexcept
        : m_Override(this)
    {
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::ui::OverlayTheme> m_Override;
};

} // namespace mocks
} // namespace ui
} // namespace ra

#endif // !RA_SERVICES_MOCK_OVERLAY_THEME_HH
