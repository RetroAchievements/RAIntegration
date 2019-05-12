#ifndef RA_SERVICES_MOCK_WINDOW_MANAGER_HH
#define RA_SERVICES_MOCK_WINDOW_MANAGER_HH
#pragma once

#include "ui\viewmodels\WindowManager.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace ui {
namespace viewmodels {
namespace mocks {

class MockWindowManager : public WindowManager
{
public:
    MockWindowManager() noexcept
        : m_Override(this)
    {
    }

private:
    ra::services::ServiceLocator::ServiceOverride<WindowManager> m_Override;
};

} // namespace mocks
} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif // !RA_SERVICES_MOCK_WINDOW_MANAGER_HH
