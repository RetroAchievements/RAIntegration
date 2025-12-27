#ifndef RA_SERVICES_MOCK_LOGGER_HH
#define RA_SERVICES_MOCK_LOGGER_HH
#pragma once

#include "services\ILogger.hh"
#include "services\ServiceLocator.hh"

#include "testutil\CppUnitTest.hh"

namespace ra {
namespace services {
namespace mocks {

class MockLogger : public ILogger
{
public:
    MockLogger() noexcept : m_Override(this)
    {
    }

    bool IsEnabled(LogLevel level) const noexcept override
    {
        return level >= m_nLevel;
    }

    void LogMessage(LogLevel level, const std::string& sMessage) const override
    {
        if (IsEnabled(level))
            m_vLog.push_back(sMessage);
    }

    void AssertContains(const std::string& sMessage) const
    {
        for (size_t i = 0; i < m_vLog.size(); ++i) {
            if (m_vLog.at(i) == sMessage)
                return;
        }

        Assert::Fail(ra::StringPrintf(L"Did not find log line containing: %s", sMessage).c_str());
    }

private:
    ServiceLocator::ServiceOverride<ILogger> m_Override;
    LogLevel m_nLevel = LogLevel::Info;

    mutable std::vector<std::string> m_vLog;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_LOGGER_HH
