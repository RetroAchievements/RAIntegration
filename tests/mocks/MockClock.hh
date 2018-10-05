#ifndef RA_SERVICES_MOCK_CLOCK_HH
#define RA_SERVICES_MOCK_CLOCK_HH
#pragma once

#include "services\IClock.hh"
#include "services\ServiceLocator.hh"

#include <unordered_map>

namespace ra {
namespace services {
namespace mocks {

class MockClock : public IClock
{
public:
    MockClock() noexcept
        : m_Override(this)
    {
        m_tNow = std::chrono::system_clock::from_time_t(1534889323); // 16:08:43 08/21/18
        m_tUpTime = std::chrono::steady_clock::time_point();
    }

    std::chrono::system_clock::time_point Now() const
    {
        return m_tNow;
    }

    std::chrono::steady_clock::time_point UpTime() const
    {
        return m_tUpTime;
    }

    void AdvanceTime(std::chrono::milliseconds nDuration)
    {
        m_tNow += nDuration;
        m_tUpTime += nDuration;
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::IClock> m_Override;

    std::chrono::system_clock::time_point m_tNow;
    std::chrono::steady_clock::time_point m_tUpTime;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_CLOCK_HH
