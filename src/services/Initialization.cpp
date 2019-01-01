#include "Initialization.hh"

#include "api\impl\DisconnectedServer.hh"

#include "data\EmulatorContext.hh"
#include "data\GameContext.hh"
#include "data\SessionTracker.hh"
#include "data\UserContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\ServiceLocator.hh"
#include "services\impl\Clock.hh"
#include "services\impl\FileLocalStorage.hh"
#include "services\impl\JsonFileConfiguration.hh"
#include "services\impl\LeaderboardManager.hh"
#include "services\impl\ThreadPool.hh"
#include "services\impl\WindowsAudioSystem.hh"
#include "services\impl\WindowsClipboard.hh"
#include "services\impl\WindowsDebuggerFileLogger.hh"
#include "services\impl\WindowsFileSystem.hh"
#include "services\impl\WindowsHttpRequester.hh"

#include "ui\WindowViewModelBase.hh"
#include "ui\drawing\gdi\GDIBitmapSurface.hh"
#include "ui\drawing\gdi\ImageRepository.hh"
#include "ui\viewmodels\OverlayManager.hh"
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
    Expects(localtime_s(&tTimeStruct, &tTime) == 0);

    std::ostringstream oss;
    oss << std::put_time(&tTimeStruct, "%D %T %Z");

    const auto sLine = StringPrintf("Log started at %s", oss.str());
    pLogger.LogMessage(LogLevel::Info, sLine);

    pLogger.LogMessage(LogLevel::Info, StringPrintf("BaseDirectory: %s", ra::Narrow(pFileSystem.BaseDirectory())));
}

void Initialization::RegisterCoreServices()
{
    // check to see if already registered
    if (ra::services::ServiceLocator::Exists<ra::services::IConfiguration>())
        return;

    auto pClock = std::make_unique<ra::services::impl::Clock>();
    auto pFileSystem = std::make_unique<ra::services::impl::WindowsFileSystem>();
    auto pLogger = std::make_unique<ra::services::impl::WindowsDebuggerFileLogger>(*pFileSystem);
    LogHeader(*pLogger, *pFileSystem, *pClock);

    ra::services::ServiceLocator::Provide<ra::services::IClock>(std::move(pClock));
    ra::services::ServiceLocator::Provide<ra::services::IFileSystem>(std::move(pFileSystem));
    ra::services::ServiceLocator::Provide<ra::services::ILogger>(std::move(pLogger));

    auto pConfiguration = std::make_unique<ra::services::impl::JsonFileConfiguration>();
    ra::services::ServiceLocator::Provide<ra::services::IConfiguration>(std::move(pConfiguration));
}

void Initialization::RegisterServices(EmulatorID nEmulatorId)
{
    RegisterCoreServices();

    auto& pFileSystem = ra::services::ServiceLocator::GetMutable<ra::services::IFileSystem>();

    auto pEmulatorContext = std::make_unique<ra::data::EmulatorContext>();
    pEmulatorContext->Initialize(nEmulatorId);
    auto& sClientName = pEmulatorContext->GetClientName();
    ra::services::ServiceLocator::Provide<ra::data::EmulatorContext>(std::move(pEmulatorContext));

    auto* pConfiguration = dynamic_cast<ra::services::impl::JsonFileConfiguration*>(
        &ra::services::ServiceLocator::GetMutable<ra::services::IConfiguration>());
    const auto sFilename = ra::StringPrintf(L"%sRAPrefs_%s.cfg", pFileSystem.BaseDirectory(), sClientName);
    pConfiguration->Load(sFilename);

    auto pLocalStorage = std::make_unique<ra::services::impl::FileLocalStorage>(pFileSystem);
    ra::services::ServiceLocator::Provide<ra::services::ILocalStorage>(std::move(pLocalStorage));

    auto pThreadPool = std::make_unique<ra::services::impl::ThreadPool>();
    pThreadPool->Initialize(pConfiguration->GetNumBackgroundThreads());
    ra::services::ServiceLocator::Provide<ra::services::IThreadPool>(std::move(pThreadPool));

    auto pHttpRequester = std::make_unique<ra::services::impl::WindowsHttpRequester>();
    ra::services::ServiceLocator::Provide<ra::services::IHttpRequester>(std::move(pHttpRequester));

    auto pUserContext = std::make_unique<ra::data::UserContext>();
    ra::services::ServiceLocator::Provide<ra::data::UserContext>(std::move(pUserContext));

    auto pGameContext = std::make_unique<ra::data::GameContext>();
    ra::services::ServiceLocator::Provide<ra::data::GameContext>(std::move(pGameContext));

    auto pSessionTracker = std::make_unique<ra::data::SessionTracker>();
    ra::services::ServiceLocator::Provide<ra::data::SessionTracker>(std::move(pSessionTracker));

    auto pAchievementRuntime = std::make_unique<ra::services::AchievementRuntime>();
    ra::services::ServiceLocator::Provide<ra::services::AchievementRuntime>(std::move(pAchievementRuntime));

    auto pLeaderboardManager = std::make_unique<ra::services::impl::LeaderboardManager>(*pConfiguration);
    ra::services::ServiceLocator::Provide<ra::services::ILeaderboardManager>(std::move(pLeaderboardManager));

    auto pAudioSystem = std::make_unique<ra::services::impl::WindowsAudioSystem>();
    ra::services::ServiceLocator::Provide<ra::services::IAudioSystem>(std::move(pAudioSystem));

    auto pClipboard = std::make_unique<ra::services::impl::WindowsClipboard>();
    ra::services::ServiceLocator::Provide<ra::services::IClipboard>(std::move(pClipboard));

    auto pDesktop = std::make_unique<ra::ui::win32::Desktop>();
    ra::services::ServiceLocator::Provide<ra::ui::IDesktop>(std::move(pDesktop));
    ra::ui::WindowViewModelBase::WindowTitleProperty.SetDefaultValue(ra::Widen(sClientName));

    auto pSurfaceFactory = std::make_unique<ra::ui::drawing::gdi::GDISurfaceFactory>();
    ra::services::ServiceLocator::Provide<ra::ui::drawing::ISurfaceFactory>(std::move(pSurfaceFactory));

    auto pWindowManager = std::make_unique<ra::ui::viewmodels::WindowManager>();
    ra::services::ServiceLocator::Provide<ra::ui::viewmodels::WindowManager>(std::move(pWindowManager));

    auto pOverlayManager = std::make_unique<ra::ui::viewmodels::OverlayManager>();
    ra::services::ServiceLocator::Provide<ra::ui::viewmodels::OverlayManager>(std::move(pOverlayManager));

    auto pImageRepository = std::make_unique<ra::ui::drawing::gdi::ImageRepository>();
    pImageRepository->Initialize();
    ra::services::ServiceLocator::Provide<ra::ui::IImageRepository>(std::move(pImageRepository));

    auto pServer = std::make_unique<ra::api::impl::DisconnectedServer>(pConfiguration->GetHostName());
    ra::services::ServiceLocator::Provide<ra::api::IServer>(std::move(pServer));
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
