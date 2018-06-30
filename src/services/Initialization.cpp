#include "Initialization.hh"

#include "services\ServiceLocator.hh"
#include "services\impl\JsonFileConfiguration.hh"
#include "services\impl\LeaderboardManager.hh"

namespace ra {
namespace services {

void Initialization::RegisterServices(const std::string& sHomeDir, const std::string& sClientName)
{
    std::string sFilename = sHomeDir + "RAPrefs_" + sClientName + ".cfg";
    auto* pConfiguration = new ra::services::impl::JsonFileConfiguration();
    pConfiguration->Load(sFilename);
    ra::services::ServiceLocator::Provide<ra::services::IConfiguration>(pConfiguration);

    auto* pLeaderboardManager = new ra::services::impl::LeaderboardManager(*pConfiguration);
    ra::services::ServiceLocator::Provide<ra::services::ILeaderboardManager>(pLeaderboardManager);
}

} // namespace services
} // namespace ra
