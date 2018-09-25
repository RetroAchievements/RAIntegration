#include "Initialization.hh"

#include "services\ServiceLocator.hh"
#include "services\impl\JsonFileConfiguration.hh"
#include "services\impl\LeaderboardManager.hh"

#include "ui\win32\Desktop.hh"
#include "ui\WindowViewModelBase.hh"

namespace ra {
namespace services {

void Initialization::RegisterServices(const std::wstring& sHomeDir, const std::string& sClientName)
{
    std::wstring sFilename = sHomeDir + L"RAPrefs_" + ra::Widen(sClientName) + L".cfg";
    auto* pConfiguration = new ra::services::impl::JsonFileConfiguration();
    pConfiguration->Load(sFilename);
    ra::services::ServiceLocator::Provide<ra::services::IConfiguration>(pConfiguration);

    auto* pLeaderboardManager = new ra::services::impl::LeaderboardManager(*pConfiguration);
    ra::services::ServiceLocator::Provide<ra::services::ILeaderboardManager>(pLeaderboardManager);

	auto* pDesktop = new ra::ui::win32::Desktop();
	ra::services::ServiceLocator::Provide<ra::ui::IDesktop>(pDesktop);
	ra::ui::WindowViewModelBase::WindowTitleProperty.SetDefaultValue(ra::Widen(sClientName));
}

} // namespace services
} // namespace ra
