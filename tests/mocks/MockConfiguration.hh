#ifndef RA_SERVICES_MOCK_CONFIGURATION_HH
#define RA_SERVICES_MOCK_CONFIGURATION_HH
#pragma once

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace services {
namespace mocks {

class MockConfiguration : public IConfiguration
{
public:
    MockConfiguration() noexcept
        : m_Override(this)
    {
    }

    const std::string& GetUsername() const override { return m_sUsername; }
    void SetUsername(const std::string& sValue) override { m_sUsername = sValue; }

    const std::string& GetApiToken() const override { return m_sApiToken; }
    void SetApiToken(const std::string& sValue) override { m_sApiToken = sValue; }

    bool IsFeatureEnabled(Feature nFeature) const override
    {
        return (m_vEnabledFeatures.find(nFeature) != m_vEnabledFeatures.end());
    }

    void SetFeatureEnabled(Feature nFeature, bool bEnabled) override
    {
        if (bEnabled)
            m_vEnabledFeatures.insert(nFeature);
        else
            m_vEnabledFeatures.erase(nFeature);
    }

    unsigned int GetNumBackgroundThreads() const override { return m_nBackgroundThreads; }

    const std::string& GetRomDirectory() const override { return m_sRomDirectory; }
    void SetRomDirectory(const std::string& sValue) override { m_sRomDirectory = sValue; }

    ra::ui::Position GetWindowPosition(const std::string& sPositionKey) const override
    {
        assert(!"Not implemented");
        return ra::ui::Position();
    }

    void SetWindowPosition(const std::string& sPositionKey, const ra::ui::Position& oPosition) override
    {
        assert(!"Not implemented");
    }

    ra::ui::Size GetWindowSize(const std::string& sPositionKey) const override
    {
        assert(!"Not implemented");
        return ra::ui::Size();
    }

    void SetWindowSize(const std::string& sPositionKey, const ra::ui::Size& oSize) override
    {
        assert(!"Not implemented");
    }

    const std::string& GetHostName() const override { return m_sHostName; }
    void SetHostName(const std::string& sHostName) { m_sHostName = sHostName; }

    void Save() const override
    {
        assert(!"Not implemented");
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::IConfiguration> m_Override;

    std::string m_sUsername;
    std::string m_sApiToken;
    std::string m_sRomDirectory;
    std::string m_sHostName;

    unsigned int m_nBackgroundThreads = 0;

    std::set<Feature> m_vEnabledFeatures;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_CONFIGURATION_HH
