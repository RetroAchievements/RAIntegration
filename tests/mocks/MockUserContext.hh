#ifndef RA_DATA_MOCK_USERCONTEXT_HH
#define RA_DATA_MOCK_USERCONTEXT_HH
#pragma once

#include "data\UserContext.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace data {
namespace mocks {

class MockUserContext : public UserContext
{
public:
    MockUserContext() noexcept
        : m_Override(this)
    {
    }

    void Logout() noexcept override
    {
        m_sUsername.clear();
        m_sApiToken.clear();
        m_nScore = 0U;
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::data::UserContext> m_Override;
};

} // namespace mocks
} // namespace data
} // namespace ra

#endif // !RA_DATA_MOCK_USERCONTEXT_HH
