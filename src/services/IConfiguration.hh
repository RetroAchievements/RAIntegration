#ifndef RA_SERVICES_ICONFIGURATION
#define RA_SERVICES_ICONFIGURATION
#pragma once

#include "ui\WindowViewModelBase.hh"

#include <string>

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

class IConfiguration {
public:
    virtual const std::string& GetUsername() const = 0;
    virtual void SetUsername(const std::string& sValue) = 0;
    virtual const std::string& GetApiToken() const = 0;
    virtual void SetApiToken(const std::string& sValue) = 0;

    virtual bool IsFeatureEnabled(Feature nFeature) const = 0;
    virtual void SetFeatureEnabled(Feature nFeature, bool bEnabled) = 0;

    virtual unsigned int GetNumBackgroundThreads() const = 0;
    virtual const std::string& GetRomDirectory() const = 0;
    virtual void SetRomDirectory(const std::string& sValue) = 0;

    virtual ra::ui::Position GetWindowPosition(const std::string& sPositionKey) const = 0;
    virtual void SetWindowPosition(const std::string& sPositionKey, const ra::ui::Position& oPosition) = 0;

    virtual ra::ui::Size GetWindowSize(const std::string& sPositionKey) const = 0;
    virtual void SetWindowSize(const std::string& sPositionKey, const ra::ui::Size& oSize) = 0;

    virtual void Save() const = 0;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_ICONFIGURATION
