#ifndef RA_SERVICES_ICONFIGURATION
#define RA_SERVICES_ICONFIGURATION
#pragma once

#include "ui\Types.hh"

namespace ra {
namespace services {

enum class Feature
{
    None = 0,
    Hardcore,
    Leaderboards,
    LeaderboardNotifications,
    LeaderboardCounters,
    LeaderboardScoreboards,
    PreferDecimal,
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

    virtual const std::string& GetRomDirectory() const = 0;
    virtual void SetRomDirectory(const std::string& sValue) = 0;

    /// <summary>
    /// Gets the remembered position of the window identified by <paramref name="sPositionKey"/>.
    /// </summary>
    virtual ra::ui::Position GetWindowPosition(const std::string& sPositionKey) const = 0;

    /// <summary>
    /// Sets the position to remember for the window identified by <paramref name="sPositionKey"/>.
    /// </summary>
    virtual void SetWindowPosition(const std::string& sPositionKey, const ra::ui::Position& oPosition) = 0;

    /// <summary>
    /// Gets the remembered size of the window identified by <paramref name="sPositionKey"/>.
    /// </summary>
    virtual ra::ui::Size GetWindowSize(const std::string& sPositionKey) const = 0;

    /// <summary>
    /// Sets the size to remember for the window identified by <paramref name="sPositionKey"/>.
    /// </summary>
    virtual void SetWindowSize(const std::string& sPositionKey, const ra::ui::Size& oSize) = 0;
    
    /// <summary>
    /// Gets the name of the host to communicate with.
    /// </summary>
    virtual const std::string& GetHostName() const = 0;

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
