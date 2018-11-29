#include "Initialization.hh"

#include "api\impl\DisconnectedServer.hh"

#include "data\GameContext.hh"
#include "data\SessionTracker.hh"
#include "data\UserContext.hh"

#include "services\ServiceLocator.hh"
#include "services\impl\Clock.hh"
#include "services\impl\FileLocalStorage.hh"
#include "services\impl\JsonFileConfiguration.hh"
#include "services\impl\LeaderboardManager.hh"
#include "services\impl\ThreadPool.hh"
#include "services\impl\WindowsClipboard.hh"
#include "services\impl\WindowsDebuggerFileLogger.hh"
#include "services\impl\WindowsFileSystem.hh"
#include "services\impl\WindowsHttpRequester.hh"

#include "ui\WindowViewModelBase.hh"
#include "ui\drawing\gdi\ImageRepository.hh"
#include "ui\drawing\gdi\GDIBitmapSurface.hh"
#include "ui\viewmodels\WindowManager.hh"
#include "ui\win32\Desktop.hh"

namespace ra {
namespace services {

static void LogHeader(_In_ const ra::services::ILogger& pLogger,
                      _In_ const ra::services::IFileSystem& pFileSystem,
                      _In_ const ra::services::IClock& pClock)
{
    pLogger.LogMessage(LogLevel::Info,
                       "================================================================================");

    const auto tNow = pClock.Now();
    const auto tTime = std::chrono::system_clock::to_time_t(tNow);

    std::tm tTimeStruct;
    localtime_s(&tTimeStruct, &tTime);

    char sBuffer[64];
    strftime(sBuffer, sizeof(sBuffer), "%D %T %Z", &tTimeStruct);

    std::string sLine = "Log started at ";
    sLine += sBuffer;

    pLogger.LogMessage(LogLevel::Info, sLine);

    pLogger.LogMessage(LogLevel::Info, "BaseDirectory: " + ra::Narrow(pFileSystem.BaseDirectory()));
}

void Initialization::RegisterCoreServices()
{
    // check to see if already registered
    if (ra::services::ServiceLocator::Exists<ra::services::IConfiguration>())
        return;

    auto* pClock = new ra::services::impl::Clock();
    ra::services::ServiceLocator::Provide<ra::services::IClock>(pClock);

    auto* pFileSystem = new ra::services::impl::WindowsFileSystem();
    ra::services::ServiceLocator::Provide<ra::services::IFileSystem>(pFileSystem);

    auto* pLogger = new ra::services::impl::WindowsDebuggerFileLogger(*pFileSystem);
    ra::services::ServiceLocator::Provide<ra::services::ILogger>(pLogger);

    LogHeader(*pLogger, *pFileSystem, *pClock);

    auto* pConfiguration = new ra::services::impl::JsonFileConfiguration();
    ra::services::ServiceLocator::Provide<ra::services::IConfiguration>(pConfiguration);
}

void Initialization::RegisterServices(const std::string& sClientName)
{
    RegisterCoreServices();

    auto& pFileSystem = ra::services::ServiceLocator::GetMutable<ra::services::IFileSystem>();

    auto* pConfiguration = dynamic_cast<ra::services::impl::JsonFileConfiguration*>(
        &ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>());
    std::wstring sFilename = pFileSystem.BaseDirectory() + L"RAPrefs_" + ra::Widen(sClientName) + L".cfg";
    pConfiguration->Load(sFilename);

    auto* pLocalStorage = new ra::services::impl::FileLocalStorage(pFileSystem);
    ra::services::ServiceLocator::Provide<ra::services::ILocalStorage>(pLocalStorage);

    auto* pThreadPool = new ra::services::impl::ThreadPool();
    pThreadPool->Initialize(pConfiguration->GetNumBackgroundThreads());
    ra::services::ServiceLocator::Provide<ra::services::IThreadPool>(pThreadPool);

    auto* pHttpRequester = new ra::services::impl::WindowsHttpRequester();
    ra::services::ServiceLocator::Provide<ra::services::IHttpRequester>(pHttpRequester);

    auto* pUserContext = new ra::data::UserContext();
    ra::services::ServiceLocator::Provide<ra::data::UserContext>(pUserContext);

    auto* pGameContext = new ra::data::GameContext();
    ra::services::ServiceLocator::Provide<ra::data::GameContext>(pGameContext);

    auto* pSessionTracker = new ra::data::SessionTracker();
    ra::services::ServiceLocator::Provide<ra::data::SessionTracker>(pSessionTracker);

    auto* pLeaderboardManager = new ra::services::impl::LeaderboardManager(*pConfiguration);
    ra::services::ServiceLocator::Provide<ra::services::ILeaderboardManager>(pLeaderboardManager);

    auto* pClipboard = new ra::services::impl::WindowsClipboard();
    ra::services::ServiceLocator::Provide<ra::services::IClipboard>(pClipboard);

    auto* pDesktop = new ra::ui::win32::Desktop();
    ra::services::ServiceLocator::Provide<ra::ui::IDesktop>(pDesktop);
    ra::ui::WindowViewModelBase::WindowTitleProperty.SetDefaultValue(ra::Widen(sClientName));

    auto* pSurfaceFactory = new ra::ui::drawing::gdi::GDISurfaceFactory();
    ra::services::ServiceLocator::Provide<ra::ui::drawing::ISurfaceFactory>(pSurfaceFactory);

    auto* pWindowManager = new ra::ui::viewmodels::WindowManager();
    ra::services::ServiceLocator::Provide<ra::ui::viewmodels::WindowManager>(pWindowManager);

    auto* pImageRepository = new ra::ui::drawing::gdi::ImageRepository();
    pImageRepository->Initialize();
    ra::services::ServiceLocator::Provide<ra::ui::IImageRepository>(pImageRepository);

    auto* pServer = new ra::api::impl::DisconnectedServer(pConfiguration->GetHostName());
    ra::services::ServiceLocator::Provide<ra::api::IServer>(pServer);
}

void Initialization::Shutdown()
{
    ra::services::ServiceLocator::GetMutable<ra::ui::IDesktop>().Shutdown();

    ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>().Shutdown(true);

    // ImageReference destructors will try to use the IImageRepository if they think it still exists.
    // explicitly deregister it to prevent exceptions when closing down the application.
    ra::services::ServiceLocator::Provide<ra::ui::IImageRepository>(nullptr);
}

} // namespace services
} // namespace ra
