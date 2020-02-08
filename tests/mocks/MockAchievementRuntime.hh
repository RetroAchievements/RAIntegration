#ifndef RA_SERVICES_MOCK_ACHIEVEMENT_RUNTIME_HH
#define RA_SERVICES_MOCK_ACHIEVEMENT_RUNTIME_HH
#pragma once

#include "services\AchievementRuntime.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace services {
namespace mocks {

class MockAchievementRuntime : public AchievementRuntime
{
public:
    MockAchievementRuntime() noexcept : m_Override(this)
    {
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::AchievementRuntime> m_Override;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_ACHIEVEMENT_RUNTIME_HH
