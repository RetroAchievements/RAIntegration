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

    ra::ui::viewmodels::PopupLocation GetPopupLocation(ra::ui::viewmodels::Popup nPopup) const override;
    void SetPopupLocation(ra::ui::viewmodels::Popup nPopup, ra::ui::viewmodels::PopupLocation nPopupLocation) override;

    unsigned int GetNumBackgroundThreads() const noexcept override { return m_nBackgroundThreads; }

    const std::wstring& GetRomDirectory() const noexcept override { return m_sRomDirectory; }
    void SetRomDirectory(const std::wstring& sValue) override { m_sRomDirectory = sValue; }

    const std::wstring& GetScreenshotDirectory() const noexcept override { return m_sScreenshotDirectory; }
    void SetScreenshotDirectory(const std::wstring& sValue) override { m_sScreenshotDirectory = sValue; }

    ra::ui::Position GetWindowPosition(const std::string& sPositionKey) const override;
    void SetWindowPosition(const std::string& sPositionKey, const ra::ui::Position& oPosition) override;

    ra::ui::Size GetWindowSize(const std::string& sPositionKey) const override;
    void SetWindowSize(const std::string& sPositionKey, const ra::ui::Size& oSize) override;

    bool IsCustomHost() const noexcept override { return m_bCustomHost; }
    const std::string& GetHostName() const override;
    const std::string& GetHostUrl() const override;
    const std::string& GetImageHostUrl() const override;

    void Save() const override;

private:
    void UpdateHost();

    std::string m_sUsername;
    std::string m_sApiToken;

    int m_vEnabledFeatures = 0;

    std::array<ra::ui::viewmodels::PopupLocation, ra::etoi(ra::ui::viewmodels::Popup::NumPopups)> m_vPopupLocations;

    unsigned int m_nBackgroundThreads = 8;
    std::wstring m_sRomDirectory;
    std::wstring m_sScreenshotDirectory;

    typedef struct WindowPosition
    {
        ra::ui::Position oPosition{ INT32_MIN, INT32_MIN };
        ra::ui::Size oSize{ INT32_MIN, INT32_MIN };
    } WindowPosition;

    typedef std::map<std::string, WindowPosition> WindowPositionMap;
    WindowPositionMap m_mWindowPositions;

    bool m_bCustomHost = false;
    std::string m_sHostName;
    std::string m_sHostUrl;
    std::string m_sImageHostUrl;

    std::wstring m_sFilename;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_JSON_FILE_CONFIGUATION_HH
