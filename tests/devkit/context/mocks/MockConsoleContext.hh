#ifndef RA_CONTEXT_MOCK_CONSOLECONTEXT_HH
#define RA_CONTEXT_MOCK_CONSOLECONTEXT_HH
#pragma once

#include "context/impl/ConsoleContext.hh"

#include "services/ServiceLocator.hh"

namespace ra {
namespace context {
namespace mocks {

class MockConsoleContext : public impl::ConsoleContext
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
        ra::data::MemoryRegion::Type nAddressType, const std::wstring& sDescription = L"")
    { 
        AddMemoryRegion(nStartAddress, nEndAddress, nAddressType, nStartAddress, sDescription);
    }

    void AddMemoryRegion(ra::data::ByteAddress nStartAddress, ra::data::ByteAddress nEndAddress, 
        ra::data::MemoryRegion::Type nAddressType, ra::data::ByteAddress nRealAddress, const std::wstring& sDescription = L"")
    { 
        auto& pRegion = m_vRegions.emplace_back(nStartAddress, nEndAddress, sDescription);
        pRegion.SetRealStartAddress(nRealAddress);
        pRegion.SetType(nAddressType);
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::context::IConsoleContext> m_Override;
};

} // namespace mocks
} // namespace context
} // namespace ra

#endif // !RA_CONTEXT_MOCK_CONSOLECONTEXT_HH
