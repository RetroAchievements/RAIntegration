#ifndef RA_SERVICES_MOCK_DEBUGGER_DETECTOR_HH
#define RA_SERVICES_MOCK_DEBUGGER_DETECTOR_HH
#pragma once

#include "services/IDebuggerDetector.hh"
#include "services/ServiceLocator.hh"

namespace ra {
namespace services {
namespace mocks {

class MockDebuggerDetector : public IDebuggerDetector
{
public:
    explicit MockDebuggerDetector() noexcept
        : m_Override(this)
    {
    }

    bool IsDebuggerPresent() const noexcept override { return m_bDebuggerPresent; }
    void SetDebuggerPresent(bool bValue) noexcept { m_bDebuggerPresent = bValue; }

private:
    ra::services::ServiceLocator::ServiceOverride<IDebuggerDetector> m_Override;

    bool m_bDebuggerPresent = false;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_DEBUGGER_DETECTOR_HH
