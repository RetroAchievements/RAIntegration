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

    const std::string& GetUsername() const noexcept override { return m_sUsername; }
    void SetUsername(const std::string& sValue) override { m_sUsername = sValue; }

    const std::string& GetApiToken() const noexcept override { return m_sApiToken; }
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

    unsigned int GetNumBackgroundThreads() const noexcept override { return m_nBackgroundThreads; }

    const std::wstring& GetRomDirectory() const noexcept override { return m_sRomDirectory; }
    void SetRomDirectory(const std::wstring& sValue) override { m_sRomDirectory = sValue; }

    const std::wstring& GetScreenshotDirectory() const noexcept override { return m_sScreenshotDirectory; }
    void SetScreenshotDirectory(const std::wstring& sValue) override { m_sScreenshotDirectory = sValue; }

    ra::ui::Position GetWindowPosition([[maybe_unused]] const std::string& /*sPositionKey*/) const override
    {
        assert(!"Not implemented");
        return ra::ui::Position();
    }

    void SetWindowPosition([[maybe_unused]] const std::string& /*sPositionKey*/,
                           [[maybe_unused]] const ra::ui::Position& /*oPosition*/) noexcept override
    {
        assert(!"Not implemented");
    }

    ra::ui::Size GetWindowSize([[maybe_unused]] const std::string& /*sPositionKey*/) const noexcept override
    {
        assert(!"Not implemented");
        return ra::ui::Size();
    }

    void SetWindowSize([[maybe_unused]] const std::string& /*sPositionKey*/,
                       [[maybe_unused]] const ra::ui::Size& /*oSize*/) noexcept override
    {
        assert(!"Not implemented");
    }

    const std::string& GetHostName() const noexcept override { return m_sHostName; }
    void SetHostName(const std::string& sHostName)
    {
        m_sHostName = sHostName;
        m_sHostUrl = m_sImageHostUrl = "http://" + sHostName;
    }

    const std::string& GetHostUrl() const noexcept override { return m_sHostUrl; }
    void SetHostUrl(const std::string& sHostUrl) { m_sHostUrl = sHostUrl; }

    const std::string& GetImageHostUrl() const noexcept override { return m_sImageHostUrl; }
    void SetImageHostUrl(const std::string& sHostUrl) { m_sImageHostUrl = sHostUrl; }

    void Save() const noexcept override
    {
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::IConfiguration> m_Override;

    std::string m_sUsername;
    std::string m_sApiToken;
    std::wstring m_sRomDirectory;
    std::wstring m_sScreenshotDirectory;
    std::string m_sHostName;
    std::string m_sHostUrl;
    std::string m_sImageHostUrl;

    unsigned int m_nBackgroundThreads = 0;

    std::set<Feature> m_vEnabledFeatures;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_CONFIGURATION_HH
