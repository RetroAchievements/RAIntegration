#ifndef RA_SERVICES_ICONFIGURATION
#define RA_SERVICES_ICONFIGURATION
#pragma once

#include <string>

namespace ra {
namespace services {

enum class Feature
{
    None = 0,
    Hardcore,
    Leaderboards,
    PreferDecimal,
    NonHardcoreWarning,
    OnlyHardcoreUnlocks,
    AchievementTriggeredScreenshot,
    MasteryNotificationScreenshot,
    Offline,
};

class IConfiguration
{
public:
    virtual ~IConfiguration() noexcept = default;
    IConfiguration(const IConfiguration&) noexcept = delete;
    IConfiguration& operator=(const IConfiguration&) noexcept = delete;
    IConfiguration(IConfiguration&&) noexcept = delete;
    IConfiguration& operator=(IConfiguration&&) noexcept = delete;

    /// <summary>
    /// Gets the user name to use for automatic login.
    /// </summary>
    virtual const std::string& GetUsername() const = 0;

    /// <summary>
    /// Sets the user name to use for automatic login.
    /// </summary>
    virtual void SetUsername(const std::string& sValue) = 0;

    /// <summary>
    /// Gets the token to use for automatic login.
    /// </summary>
    virtual const std::string& GetApiToken() const = 0;

    /// <summary>
    /// Sets the token to use for automatic login.
    /// </summary>
    virtual void SetApiToken(const std::string& sValue) = 0;

    /// <summary>
    /// Gets whether the specified feature is enabled.
    /// </summary>
    virtual bool IsFeatureEnabled(Feature nFeature) const = 0;

    /// <summary>
    /// Sets whether the specified feature is enabled.
    /// </summary>
    virtual void SetFeatureEnabled(Feature nFeature, bool bEnabled) = 0;

    /// <summary>
    /// Gets the number of background threads to spawn.
    /// </summary>
    virtual unsigned int GetNumBackgroundThreads() const = 0;

    /// <summary>
    /// Gets the directory where screenshots should be stored.
    /// </summary>
    virtual const std::wstring& GetScreenshotDirectory() const = 0;

    /// <summary>
    /// Sets the directory where screenshots should be stored.
    /// </summary>
    virtual void SetScreenshotDirectory(const std::wstring& sValue) = 0;

    /// <summary>
    /// Gets whether or not a custom host was provided.
    /// </summary>
    virtual bool IsCustomHost() const = 0;

    /// <summary>
    /// Gets the name of the host to communicate with (no protocol).
    /// </summary>
    virtual const std::string& GetHostName() const = 0;

    /// <summary>
    /// Gets the URL to the host to communicate with (includes protocol).
    /// </summary>
    virtual const std::string& GetHostUrl() const = 0;

    /// <summary>
    /// Saves the current configuration so it can be used in a future session.
    /// </summary>
    virtual void Save() const = 0;

protected:
    IConfiguration() noexcept = default;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_ICONFIGURATION
