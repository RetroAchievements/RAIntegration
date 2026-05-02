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

private:
    bool m_bInvalid = false;

    std::string m_sWarnDisableHardcoreActivity;
    ra::ui::DialogResult m_nWarnDisableHardcoreResult = ra::ui::DialogResult::No;

    ra::services::ServiceLocator::ServiceOverride<ra::data::context::EmulatorContext> m_Override;
};

} // namespace mocks
} // namespace context
} // namespace data
} // namespace ra

#endif // !RA_DATA_MOCK_EMULATORCONTEXT_HH
