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
    GSL_SUPPRESS_F6 MockConsoleContext() noexcept
        : MockConsoleContext(ConsoleID::UnknownConsoleID, L"")
    {
    }

    MockConsoleContext(ConsoleID nId, std::wstring&& sName) noexcept
        : ConsoleContext(nId, std::move(sName)), m_Override(this)
    {
    }

    void SetId(ConsoleID nId) noexcept { m_nId = nId; }

    void SetName(std::wstring&& sName) noexcept { m_sName = std::move(sName); }

    const std::vector<MemoryRegion>& MemoryRegions() const noexcept { return m_vRegions; }

    void ResetMemoryRegions() { m_vRegions.clear(); }

    void AddMemoryRegion(ra::ByteAddress nStartAddress, ra::ByteAddress nEndAddress, AddressType nAddressType) noexcept 
    { 
        auto& pRegion = m_vRegions.emplace_back();
        pRegion.StartAddress = nStartAddress;
        pRegion.EndAddress = nEndAddress;
        pRegion.Type = nAddressType;
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::data::ConsoleContext> m_Override;

    std::vector<MemoryRegion> m_vRegions;
};

} // namespace mocks
} // namespace data
} // namespace ra

#endif // !RA_DATA_MOCK_CONSOLECONTEXT_HH
