#ifndef RA_DATA_MOCK_EMULATORCONTEXT_HH
#define RA_DATA_MOCK_EMULATORCONTEXT_HH
#pragma once

#include "data\EmulatorContext.hh"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include <GSL\span>

namespace ra {
namespace data {
namespace mocks {

class MockEmulatorContext : public EmulatorContext
{
public:
    MockEmulatorContext() noexcept
        : m_Override(this)
    {
    }

    void MockClient(const std::string& sClientName, const std::string& sClientVersion)
    {
        m_sClientName = sClientName;
        m_sVersion = sClientVersion;
    }

    void MockGameTitle(const char* sTitle)
    {
        SetGetGameTitleFunction([sTitle](char* sBuffer) noexcept { strcpy_s(sBuffer, 64, sTitle); });
    }

    void DisableHardcoreMode() override
    {
        auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
        pConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
    }

    bool EnableHardcoreMode(bool) override
    {
        auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
        pConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        return true;
    }

    void MockMemory(gsl::span<unsigned char> pMemory)
    {
        s_pMemory = pMemory;

        ClearMemoryBlocks();
        AddMemoryBlock(0, s_pMemory.size_bytes(), ReadMemoryHelper, WriteMemoryHelper);
    }

    void MockMemory(unsigned char pMemory[], size_t nBytes)
    {
        s_pMemory = gsl::make_span(pMemory, nBytes);

        ClearMemoryBlocks();
        AddMemoryBlock(0, nBytes, ReadMemoryHelper, WriteMemoryHelper);
    }

private:
    static uint8_t ReadMemoryHelper(uint32_t nAddress) noexcept
    {
        return s_pMemory.at(nAddress);
    }

    static void WriteMemoryHelper(uint32_t nAddress, uint8_t nValue) noexcept
    {
        s_pMemory.at(nAddress) = nValue;
    }

    ra::services::ServiceLocator::ServiceOverride<ra::data::EmulatorContext> m_Override;
    static gsl::span<uint8_t> s_pMemory;
};

} // namespace mocks
} // namespace data
} // namespace ra

#endif // !RA_DATA_MOCK_EMULATORCONTEXT_HH
