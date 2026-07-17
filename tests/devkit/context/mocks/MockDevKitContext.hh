#ifndef RA_CONTEXT_MOCK_DEVKITCONTEXT_HH
#define RA_CONTEXT_MOCK_DEVKITCONTEXT_HH
#pragma once

#include "context/IDevKitContext.hh"

#include "services/ServiceLocator.hh"

namespace ra {
namespace context {
namespace mocks {

class MockDevKitContext : public IDevKitContext
{
public:
    GSL_SUPPRESS_F6 MockDevKitContext() noexcept
        : m_Override(this)
    {
    }

    const std::string& Version() const noexcept override { return m_sVersion; }
    void SetVersion(const std::string& sValue) { m_sVersion = sValue; }

private:
    std::string m_sVersion = "0.0.0.0";

    ra::services::ServiceLocator::ServiceOverride<ra::context::IDevKitContext> m_Override;
};

} // namespace mocks
} // namespace context
} // namespace ra

#endif // !RA_CONTEXT_MOCK_DEVKITCONTEXT_HH
