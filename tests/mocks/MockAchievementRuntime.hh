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

    bool IsAchievementSupported(ra::AchievementID nAchievement) noexcept
    {
        for (unsigned i = 0; i < m_pRuntime.trigger_count; ++i)
        {
            if (m_pRuntime.triggers[i].id == nAchievement)
            {
                const auto* pTrigger = m_pRuntime.triggers[i].trigger;
                if (pTrigger)
                    return pTrigger->state != RC_TRIGGER_STATE_DISABLED && !m_pRuntime.triggers[i].invalid_memref;
            }
        }

        return false;
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::AchievementRuntime> m_Override;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_ACHIEVEMENT_RUNTIME_HH
