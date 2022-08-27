#ifndef RA_DATA_MOCK_USERCONTEXT_HH
#define RA_DATA_MOCK_USERCONTEXT_HH
#pragma once

#include "data\context\UserContext.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace data {
namespace context {
namespace mocks {

class MockUserContext : public UserContext
{
public:
    MockUserContext() noexcept
        : m_Override(this)
    {
    }

    using UserContext::Initialize;

#pragma warning(push)           // the using (above) ensures the base implementation is not hidden, but
#pragma warning(disable:26434)  // the analyzer stills sees this non-overriding function as having the
                                // same name as a non-virtual function in the base class
    void Initialize(const std::string& sUsername, const std::string& sApiToken)
    {
        UserContext::Initialize(sUsername, sUsername + "_", sApiToken);
    }

    void SetUsername(const std::string& sUsername) 
    { 
        UserContext::Initialize(sUsername, sUsername, "APITOKEN");
    }
#pragma warning(pop)

    void Logout() noexcept override
    {
        m_sUsername.clear();
        m_sDisplayName.clear();
        m_sApiToken.clear();
        m_nScore = 0U;
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::data::context::UserContext> m_Override;
};

} // namespace mocks
} // namespace data
} // namespace context
} // namespace ra

#endif // !RA_DATA_MOCK_USERCONTEXT_HH
