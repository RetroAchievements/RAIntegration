#ifndef RA_DATA_MOCK_EMULATORCONTEXT_HH
#define RA_DATA_MOCK_EMULATORCONTEXT_HH
#pragma once

#include "data\EmulatorContext.hh"

#include "services\ServiceLocator.hh"

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

private:
    ra::services::ServiceLocator::ServiceOverride<ra::data::EmulatorContext> m_Override;
};

} // namespace mocks
} // namespace data
} // namespace ra

#endif // !RA_DATA_MOCK_EMULATORCONTEXT_HH
