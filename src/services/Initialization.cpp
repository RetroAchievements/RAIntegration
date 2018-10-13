#include "Initialization.hh"

#include "data\GameContext.hh"

#include "services\ServiceLocator.hh"
#include "services\impl\Clock.hh"
#include "services\impl\FileLocalStorage.hh"
#include "services\impl\JsonFileConfiguration.hh"
#include "services\impl\LeaderboardManager.hh"
#include "services\impl\ThreadPool.hh"
#include "services\impl\WindowsFileSystem.hh"
#include "services\impl\WindowsDebuggerFileLogger.hh"

#include "ui\viewmodels\WindowManager.hh"
#include "ui\win32\Desktop.hh"
#include "ui\WindowViewModelBase.hh"

namespace ra {
namespace services {

static void LogHeader(ra::services::ILogger& pLogger, ra::services::IFileSystem& pFileSystem, ra::services::IClock& pClock)
{
    pLogger.LogMessage(LogLevel::Info, "================================================================================");

    auto tNow = pClock.Now();
    auto tTime = std::chrono::system_clock::to_time_t(tNow);

    std::tm tTimeStruct;
    localtime_s(&tTimeStruct, &tTime);

    char sBuffer[64];
    strftime(sBuffer, sizeof(sBuffer), "%D %T %Z", &tTimeStruct);

    std::string sLine = "Log started at ";
    sLine += sBuffer;

    pLogger.LogMessage(LogLevel::Info, sLine);

    pLogger.LogMessage(LogLevel::Info, "BaseDirectory: " + ra::Narrow(pFileSystem.BaseDirectory()));
}

void Initialization::RegisterServices(const std::string& sClientName)
{
    auto* pClock = new ra::services::impl::Clock();
    ra::services::ServiceLocator::Provide<ra::services::IClock>(pClock);

    auto* pFileSystem = new ra::services::impl::WindowsFileSystem();
    ra::services::ServiceLocator::Provide<ra::services::IFileSystem>(pFileSystem);

    auto* pLogger = new ra::services::impl::WindowsDebuggerFileLogger(*pFileSystem);
    ra::services::ServiceLocator::Provide<ra::services::ILogger>(pLogger);

    LogHeader(*pLogger, *pFileSystem, *pClock);  

    auto* pConfiguration = new ra::services::impl::JsonFileConfiguration();
    std::wstring sFilename = pFileSystem->BaseDirectory() + L"RAPrefs_" + ra::Widen(sClientName) + L".cfg";
    pConfiguration->Load(sFilename);
    ra::services::ServiceLocator::Provide<ra::services::IConfiguration>(pConfiguration);

    auto *pLocalStorage = new ra::services::impl::FileLocalStorage(*pFileSystem);
    ra::services::ServiceLocator::Provide<ra::services::ILocalStorage>(pLocalStorage);

    auto* pThreadPool = new ra::services::impl::ThreadPool();
    pThreadPool->Initialize(pConfiguration->GetNumBackgroundThreads());
    ra::services::ServiceLocator::Provide<ra::services::IThreadPool>(pThreadPool);

    auto* pGameContext = new ra::data::GameContext();
    ra::services::ServiceLocator::Provide<ra::data::GameContext>(pGameContext);

    auto* pLeaderboardManager = new ra::services::impl::LeaderboardManager(*pConfiguration);
    ra::services::ServiceLocator::Provide<ra::services::ILeaderboardManager>(pLeaderboardManager);

    auto* pDesktop = new ra::ui::win32::Desktop();
    ra::services::ServiceLocator::Provide<ra::ui::IDesktop>(pDesktop);
    ra::ui::WindowViewModelBase::WindowTitleProperty.SetDefaultValue(ra::Widen(sClientName));

    auto* pWindowManager = new ra::ui::viewmodels::WindowManager();
    ra::services::ServiceLocator::Provide<ra::ui::viewmodels::WindowManager>(pWindowManager);
}

void Initialization::Shutdown()
{
    ra::services::ServiceLocator::GetMutable<ra::ui::IDesktop>().Shutdown();

    ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().Shutdown(true);
}

} // namespace services
} // namespace ra
