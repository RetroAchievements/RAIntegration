#include "Initialization.hh"

#include "services\ServiceLocator.hh"
#include "services\impl\JsonFileConfiguration.hh"
#include "services\impl\LeaderboardManager.hh"
#include "services\impl\WindowsFileSystem.hh"

#include "ui\win32\Desktop.hh"
#include "ui\WindowViewModelBase.hh"

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

    auto* pDesktop = new ra::ui::win32::Desktop();
    ra::services::ServiceLocator::Provide<ra::ui::IDesktop>(pDesktop);
    ra::ui::WindowViewModelBase::WindowTitleProperty.SetDefaultValue(ra::Widen(sClientName));
}

void Initialization::Shutdown()
{
    ra::services::ServiceLocator::GetMutable<ra::ui::IDesktop>().Shutdown();
}

} // namespace services
} // namespace ra
