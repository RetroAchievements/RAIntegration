#ifndef RA_SERVICES_MOCK_SESSION_TRACKER_HH
#define RA_SERVICES_MOCK_SESSION_TRACKER_HH
#pragma once

#include "data\SessionTracker.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace data {
namespace mocks {

class MockSessionTracker : public SessionTracker
{
public:
    MockSessionTracker() noexcept
        : m_Override(this)
    {
    }

    void LoadSessions() override
    {
    }

    const std::wstring& GetUsername() const { return m_sUsername; }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::data::SessionTracker> m_Override;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_SESSION_TRACKER_HH
