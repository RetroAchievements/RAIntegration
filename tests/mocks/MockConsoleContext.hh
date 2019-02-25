#ifndef RA_DATA_MOCK_CONSOLECONTEXT_HH
#define RA_DATA_MOCK_CONSOLECONTEXT_HH
#pragma once

#include "data\ConsoleContext.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace data {
namespace mocks {

class MockConsoleContext : public ConsoleContext
{
public:
    MockConsoleContext() noexcept
        : MockConsoleContext(ConsoleID::UnknownConsoleID, L"")
    {
    }

    MockConsoleContext(ConsoleID nId, const std::wstring&& sName) noexcept
        : ConsoleContext(nId, std::move(sName)), m_Override(this)
    {
    }

    void SetId(ConsoleID nId) noexcept { m_nId = nId; }

    void SetName(const std::wstring&& sName) { m_sName = std::move(sName); }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::data::ConsoleContext> m_Override;
};

} // namespace mocks
} // namespace data
} // namespace ra

#endif // !RA_DATA_MOCK_CONSOLECONTEXT_HH
