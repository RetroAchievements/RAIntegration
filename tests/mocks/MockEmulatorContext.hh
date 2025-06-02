#ifndef RA_DATA_MOCK_EMULATORCONTEXT_HH
#define RA_DATA_MOCK_EMULATORCONTEXT_HH
#pragma once

#include "data\context\EmulatorContext.hh"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include <GSL\span>

namespace ra {
namespace data {
namespace context {
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

    void MockInvalidClient(bool bInvalid) noexcept
    {
        m_bInvalid = bInvalid;
    }

    bool ValidateClientVersion(bool&) noexcept override
    {
        return !m_bInvalid;
    }

    void MockGameTitle(const char* sTitle)
    {
        SetGetGameTitleFunction([sTitle](char* sBuffer) noexcept { strcpy_s(sBuffer, 64, sTitle); });
    }

    void MockDisableHardcoreWarning(ra::ui::DialogResult nPromptResult) noexcept
    {
        m_nWarnDisableHardcoreResult = nPromptResult;
        m_sWarnDisableHardcoreActivity.clear();
    }

    const std::string& GetDisableHardcoreWarningMessage() const noexcept
    {
        return m_sWarnDisableHardcoreActivity;
    }

    bool WarnDisableHardcoreMode(const std::string& sActivity) override
    {
        auto& pConfiguration = ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>();
        if (!pConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore))
            return true;

        m_sWarnDisableHardcoreActivity = sActivity;
        if (m_nWarnDisableHardcoreResult != ra::ui::DialogResult::Yes)
            return false;

        pConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        return true;
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
        m_pMemory = pMemory;
        m_mMemoryValues.clear();

        ClearMemoryBlocks();
        AddMemoryBlock(0, m_pMemory.size_bytes(), ReadMemoryHelper, WriteMemoryHelper);

        SetRuntimeMemorySize(m_pMemory.size_bytes());
    }

    void MockMemory(unsigned char pMemory[], size_t nBytes)
    {
        m_pMemory = gsl::make_span(pMemory, nBytes);
        m_mMemoryValues.clear();

        ClearMemoryBlocks();
        AddMemoryBlock(0, nBytes, ReadMemoryHelper, WriteMemoryHelper);

        SetRuntimeMemorySize(nBytes);
    }

    void MockMemoryValue(uint32_t nAddress, uint32_t nValue) 
    { 
        m_mMemoryValues[nAddress] = nValue;

        if (nAddress + 4 > m_nTotalMemorySize)
        {
            const size_t nNewMemorySize = gsl::narrow_cast<size_t>(nAddress) + 4;
            ClearMemoryBlocks();
            AddMemoryBlock(0, nNewMemorySize, ReadMemoryHelper, WriteMemoryHelper);
        }
    }

    void MockMemoryModified(bool bModified) noexcept
    {
        m_bMemoryModified = bModified;
    }

    void MockTotalMemorySizeChanged(size_t nNewSize)
    {
        m_nTotalMemorySize = nNewSize;
        OnTotalMemorySizeChanged();
    }

    void MockMemoryInsecure(bool bValue) noexcept { m_bMemoryInsecure = bValue; }
    bool IsMemoryInsecure() const noexcept override { return m_bMemoryInsecure; }

private:
    static uint8_t ReadMemoryHelper(uint32_t nAddress) noexcept
    {
        const auto* mockEmulator =
            dynamic_cast<const MockEmulatorContext*>(&ra::services::ServiceLocator::Get<EmulatorContext>());

        if (!mockEmulator)
            return 0;

        if (!mockEmulator->m_mMemoryValues.empty())
        {
            auto iter = mockEmulator->m_mMemoryValues.lower_bound(nAddress);
            if (iter != mockEmulator->m_mMemoryValues.end() && nAddress == iter->first)
                return iter->second & 0xFF;

            if (iter != mockEmulator->m_mMemoryValues.begin())
            {
                --iter;
                if (nAddress - iter->first < 4)
                {
                    uint32_t nValue = iter->second;
                    while (nAddress > iter->first)
                    {
                        nValue >>= 8;
                        nAddress--;
                    }

                    return nValue & 0xFF;
                }
            }

            return 0;
        }

        return mockEmulator->m_pMemory.at(nAddress);
    }

    static void WriteMemoryHelper(uint32_t nAddress, uint8_t nValue) noexcept
    {
        auto* mockEmulator =
            dynamic_cast<MockEmulatorContext*>(&ra::services::ServiceLocator::GetMutable<EmulatorContext>());

        if (!mockEmulator)
            return;

        mockEmulator->m_pMemory.at(nAddress) = nValue;
    }

    static void SetRuntimeMemorySize(size_t nBytes);

    bool m_bInvalid = false;

    std::string m_sWarnDisableHardcoreActivity;
    ra::ui::DialogResult m_nWarnDisableHardcoreResult = ra::ui::DialogResult::No;

    ra::services::ServiceLocator::ServiceOverride<ra::data::context::EmulatorContext> m_Override;
    gsl::span<uint8_t> m_pMemory;

    std::map<uint32_t, uint32_t> m_mMemoryValues;
};

} // namespace mocks
} // namespace context
} // namespace data
} // namespace ra

#endif // !RA_DATA_MOCK_EMULATORCONTEXT_HH
