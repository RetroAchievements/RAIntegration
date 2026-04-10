#ifndef RA_SERVICES_MOCK_MESSAGEDISPATCHER_HH
#define RA_SERVICES_MOCK_MESSAGEDISPATCHER_HH
#pragma once

#include "services/IMessageDispatcher.hh"
#include "services/ServiceLocator.hh"

#include "tests/devkit/testutil/CppUnitTest.hh"

namespace ra {
namespace services {
namespace mocks {

class MockMessageDispatcher : public IMessageDispatcher
{
public:
    MockMessageDispatcher() noexcept : m_Override(this)
    {}

    /// <summary>
    /// Dispatches a message.
    /// </summary>
    void ReportMessage(const std::wstring& sSummary, const std::wstring& sDetail) const
    {
        m_vMessages.emplace_back(sSummary, sDetail);
    }

    /// <summary>
    /// Dispatches an error message.
    /// </summary>
    void ReportErrorMessage(const std::wstring& sSummary, const std::wstring& sDetail) const
    {
        m_vErrorMessages.emplace_back(sSummary, sDetail);
    }

    void AssertMessageReported(const std::wstring& sSummary, const std::wstring& sDetail) const
    {
        const std::wstring* sMatch = nullptr;

        for (size_t i = 0; i < m_vMessages.size(); ++i)
        {
            if (m_vMessages.at(i).first == sSummary)
            {
                sMatch = &m_vMessages.at(i).second;
                if (*sMatch == sDetail)
                    return;
            }
        }

        if (sMatch != nullptr)
            Assert::AreEqual(sDetail, *sMatch);
        else
            Assert::Fail(ra::util::String::Printf(L"Did not find message containing: %s", sSummary).c_str());
    }

    void AssertErrorMessageReported(const std::wstring& sSummary, const std::wstring& sDetail) const
    {
        const std::wstring* sMatch = nullptr;

        for (size_t i = 0; i < m_vErrorMessages.size(); ++i)
        {
            if (m_vErrorMessages.at(i).first == sSummary)
            {
                sMatch = &m_vErrorMessages.at(i).second;
                if (*sMatch == sDetail)
                    return;
            }
        }

        if (sMatch != nullptr)
            Assert::AreEqual(sDetail, *sMatch);
        else
            Assert::Fail(ra::util::String::Printf(L"Did not find message containing: %s", sSummary).c_str());
    }

private:
    ServiceLocator::ServiceOverride<IMessageDispatcher> m_Override;

    mutable std::vector<std::pair<std::wstring, std::wstring>> m_vMessages;
    mutable std::vector<std::pair<std::wstring, std::wstring>> m_vErrorMessages;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_MESSAGEDISPATCHER_HH
