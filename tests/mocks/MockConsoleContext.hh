#ifndef RA_DATA_MOCK_CONSOLECONTEXT_HH
#define RA_DATA_MOCK_CONSOLECONTEXT_HH
#pragma once

#include "data\context\ConsoleContext.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace data {
namespace context {
namespace mocks {

class MockConsoleContext : public ConsoleContext
{
public:
    GSL_SUPPRESS_F6 MockConsoleContext() noexcept
        : MockConsoleContext(ConsoleID::UnknownConsoleID, L"")
    {
    }

    GSL_SUPPRESS_F6 MockConsoleContext(ConsoleID nId, std::wstring&& sName) noexcept
        : ConsoleContext(nId), m_Override(this)
    {
        m_sName = sName;
    }

    void SetId(ConsoleID nId) noexcept { m_nId = nId; }

    void SetName(std::wstring&& sName) noexcept { m_sName = std::move(sName); }

    void ResetMemoryRegions() noexcept { m_vRegions.clear(); }

    void AddMemoryRegion(ra::data::ByteAddress nStartAddress, ra::data::ByteAddress nEndAddress, 
        AddressType nAddressType, const std::string& sDescription = "")
    { 
        AddMemoryRegion(nStartAddress, nEndAddress, nAddressType, nStartAddress, sDescription);
    }

    void AddMemoryRegion(ra::data::ByteAddress nStartAddress, ra::data::ByteAddress nEndAddress, 
        AddressType nAddressType, ra::data::ByteAddress nRealAddress, const std::string& sDescription = "")
    { 
        auto& pRegion = m_vRegions.emplace_back();
        pRegion.StartAddress = nStartAddress;
        pRegion.EndAddress = nEndAddress;
        pRegion.RealAddress = nRealAddress;
        pRegion.Type = nAddressType;
        pRegion.Description = sDescription;
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::data::context::ConsoleContext> m_Override;
};

} // namespace mocks
} // namespace context
} // namespace data
} // namespace ra

#endif // !RA_DATA_MOCK_CONSOLECONTEXT_HH
