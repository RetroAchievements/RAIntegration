#include "Initialization.hh"

#include "services\ServiceLocator.hh"
#include "services\impl\JsonFileConfiguration.hh"
#include "services\impl\LeaderboardManager.hh"
#include "services\impl\WindowsFileSystem.hh"

namespace ra {
namespace services {

void Initialization::RegisterServices(const std::string& sClientName)
{
    auto* pFileSystem = new ra::services::impl::WindowsFileSystem();
    ra::services::ServiceLocator::Provide<ra::services::IFileSystem>(pFileSystem);

    std::wstring sFilename = pFileSystem->BaseDirectory() + L"RAPrefs_" + ra::Widen(sClientName) + L".cfg";
    auto* pConfiguration = new ra::services::impl::JsonFileConfiguration();
    pConfiguration->Load(sFilename);
    ra::services::ServiceLocator::Provide<ra::services::IConfiguration>(pConfiguration);

    auto* pLeaderboardManager = new ra::services::impl::LeaderboardManager(*pConfiguration);
    ra::services::ServiceLocator::Provide<ra::services::ILeaderboardManager>(pLeaderboardManager);
}

} // namespace services
} // namespace ra
