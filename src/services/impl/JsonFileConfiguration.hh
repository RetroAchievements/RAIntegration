#ifndef RA_SERVICES_JSON_FILE_CONFIGUATION_HH
#define RA_SERVICES_JSON_FILE_CONFIGUATION_HH
#pragma once

#include "services\IConfiguration.hh"

namespace ra {
namespace services {
namespace impl {

class JsonFileConfiguration : public IConfiguration
{
public:
    bool Load(const std::wstring& sFilename);

    const std::string& GetUsername() const noexcept override { return m_sUsername; }
    void SetUsername(const std::string& sValue) override { m_sUsername = sValue; }
    const std::string& GetApiToken() const noexcept override { return m_sApiToken; }
    void SetApiToken(const std::string& sValue) override { m_sApiToken = sValue; }

    bool IsFeatureEnabled(Feature nFeature) const noexcept override;
    void SetFeatureEnabled(Feature nFeature, bool bEnabled) noexcept override;

    unsigned int GetNumBackgroundThreads() const noexcept override { return m_nBackgroundThreads; }
    const std::string& GetRomDirectory() const noexcept override { return m_sRomDirectory; }
    void SetRomDirectory(const std::string& sValue) override { m_sRomDirectory = sValue; }

    ra::ui::Position GetWindowPosition(const std::string& sPositionKey) const override;
    void SetWindowPosition(const std::string& sPositionKey, const ra::ui::Position& oPosition) override;

    ra::ui::Size GetWindowSize(const std::string& sPositionKey) const override;
    void SetWindowSize(const std::string& sPositionKey, const ra::ui::Size& oSize) override;

    const std::string& GetHostName() const override;

    void Save() const override;

private:
    std::string m_sUsername;
    std::string m_sApiToken;

    int m_vEnabledFeatures = 0;

    unsigned int m_nBackgroundThreads = 8;
    std::string m_sRomDirectory;

    typedef struct WindowPosition
    {
        ra::ui::Position oPosition{ INT32_MIN, INT32_MIN };
        ra::ui::Size oSize{ INT32_MIN, INT32_MIN };
    } WindowPosition;

    typedef std::map<std::string, WindowPosition> WindowPositionMap;
    WindowPositionMap m_mWindowPositions;

    mutable std::string m_sHostName;

    std::wstring m_sFilename;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_JSON_FILE_CONFIGUATION_HH
