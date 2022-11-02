#include "CppUnitTest.h"

#include "ui\viewmodels\RichPresenceMonitorViewModel.hh"

#include "services\ServiceLocator.hh"

#include "ui\IDesktop.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"
#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockThreadPool.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(RichPresenceMonitorViewModel_Tests)
{
    class RichPresenceMonitorViewModelHarness : public RichPresenceMonitorViewModel
    {
    public:
        RichPresenceMonitorViewModelHarness() noexcept
        {
            GSL_SUPPRESS_F6 InitializeNotifyTargets();
        }

        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockAchievementRuntime mockRuntime;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockLocalStorage mockLocalStorage;
        ra::services::mocks::MockThreadPool mockThreadPool;

        GSL_SUPPRESS_C128 void UpdateDisplayString() { RichPresenceMonitorViewModel::UpdateDisplayString(); }
    };

    TEST_METHOD(TestInitialization)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;
        Assert::AreEqual(std::wstring(L"Rich Presence Monitor"), vmRichPresence.GetWindowTitle());
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), vmRichPresence.GetDisplayString());
    }

    TEST_METHOD(TestUpdateDisplayString)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;

        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"No game loaded."), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());

        vmRichPresence.mockGameContext.SetGameId(1);
        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());

        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Hello, world!");
        vmRichPresence.mockGameContext.Assets().FindRichPresence()->Activate();
        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestUpdateDisplayStringParseError)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;

        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"No game loaded."), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());

        vmRichPresence.mockGameContext.SetGameId(1);
        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());

        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Hello, world!");
        vmRichPresence.mockGameContext.Assets().FindRichPresence()->SetScript(
            "Lookup:a\n0=Zero\n1One\n\nDisplay:@a(0xH1234)");
        vmRichPresence.mockGameContext.Assets().FindRichPresence()->Activate();
        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Parse error -16 (line 3): Missing value expression"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());

        vmRichPresence.mockGameContext.Assets().FindRichPresence()->SetScript("Display:Not Fixed");
        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Parse error -18 (line 2): Missing display string"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());

        vmRichPresence.mockGameContext.Assets().FindRichPresence()->SetScript("Display:\nFixed");
        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Fixed"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestStartMonitoringNoGame)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;

        // with no game loaded, message should be static and no callback registered
        vmRichPresence.SetIsVisible(true);
        Assert::AreEqual(std::wstring(L"No game loaded."), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestStartMonitoringNoRichPresence)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;

        // with no rich presence, message should be static and no callback registered
        vmRichPresence.mockGameContext.SetGameId(1);
        vmRichPresence.SetIsVisible(true);
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestStartMonitoring)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;

        vmRichPresence.mockGameContext.SetGameId(1);
        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Initial Value");
        vmRichPresence.mockGameContext.Assets().FindRichPresence()->Activate();
        vmRichPresence.SetIsVisible(true);
        Assert::AreEqual(std::wstring(L"Initial Value"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 1U }, vmRichPresence.mockThreadPool.PendingTasks());

        // display text shouldn't update until callback fires
        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Hello, world!");
        Assert::AreEqual(std::wstring(L"Initial Value"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 1U }, vmRichPresence.mockThreadPool.PendingTasks());

        // callback fires, and schedules itself to be called again
        vmRichPresence.mockThreadPool.AdvanceTime(std::chrono::seconds(1));
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 1U }, vmRichPresence.mockThreadPool.PendingTasks());

        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Hello, world 2!");
        vmRichPresence.mockThreadPool.AdvanceTime(std::chrono::seconds(1));
        Assert::AreEqual(std::wstring(L"Hello, world 2!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 1U }, vmRichPresence.mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestStartMonitoringModifiedLocalRichPresence)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;

        vmRichPresence.mockGameContext.SetGameId(1);
        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Initial Value");
        vmRichPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "Display:\nFrom File\n");
        vmRichPresence.mockLocalStorage.MockLastModified(ra::services::StorageItemType::RichPresence, L"1", std::chrono::system_clock::now());
        vmRichPresence.mockGameContext.Assets().FindRichPresence()->ReloadRichPresenceScript();
        vmRichPresence.mockGameContext.Assets().FindRichPresence()->Activate();
        vmRichPresence.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        vmRichPresence.SetIsVisible(true);
        Assert::AreEqual(std::wstring(L"From File"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 1U }, vmRichPresence.mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestStartMonitoringModifiedLocalRichPresenceHardcore)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;

        vmRichPresence.mockGameContext.SetGameId(1);
        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Initial Value");
        vmRichPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "Display:\nFrom File\n");
        vmRichPresence.mockLocalStorage.MockLastModified(ra::services::StorageItemType::RichPresence, L"1", std::chrono::system_clock::now());
        vmRichPresence.mockGameContext.Assets().FindRichPresence()->ReloadRichPresenceScript();
        vmRichPresence.mockGameContext.Assets().FindRichPresence()->Activate();
        vmRichPresence.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        vmRichPresence.SetIsVisible(true);
        Assert::AreEqual(std::wstring(L"Rich Presence not active."), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestStopMonitoring)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;

        vmRichPresence.mockGameContext.SetGameId(1);
        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Hello, world!");
        vmRichPresence.mockGameContext.Assets().FindRichPresence()->Activate();
        vmRichPresence.SetIsVisible(true);
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 1U }, vmRichPresence.mockThreadPool.PendingTasks());

        // when monitoring stops, updated message should be ignored. callback will still be
        // called once, but won't reschedule itself
        vmRichPresence.SetIsVisible(false);
        Assert::AreEqual({ 1U }, vmRichPresence.mockThreadPool.PendingTasks());

        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Hello, world 2!");
        vmRichPresence.mockThreadPool.AdvanceTime(std::chrono::seconds(1));
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());

        // when monitoring restarts, updated message should be immediately displayed and
        // callback will be scheduled
        vmRichPresence.SetIsVisible(true);
        Assert::AreEqual(std::wstring(L"Hello, world 2!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 1U }, vmRichPresence.mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestMonitoringStartsWhenGameLoaded)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;

        const auto tNow = std::chrono::system_clock::now();
        const auto tThen = tNow - std::chrono::minutes(5);
        vmRichPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "Display:\nHello, world!\n");
        vmRichPresence.mockLocalStorage.MockLastModified(ra::services::StorageItemType::RichPresence, L"1", tThen);
        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Server");

        // with no game loaded, message should be static and no callback registered
        vmRichPresence.SetIsVisible(true);
        Assert::AreEqual(std::wstring(L"No game loaded."), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());
        const auto* pRichPresence = vmRichPresence.mockGameContext.Assets().FindRichPresence();
        Expects(pRichPresence != nullptr);

        // without a callback, display string is not automatically updated
        vmRichPresence.mockGameContext.SetGameId(1);
        vmRichPresence.mockThreadPool.AdvanceTime(std::chrono::seconds(1));
        Assert::AreEqual(std::wstring(L"No game loaded."), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());
        Assert::AreEqual(ra::data::models::AssetState::Inactive, pRichPresence->GetState());

        // if game changes while monitor is loaded, it will reload and activate the script
        vmRichPresence.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual(ra::data::models::AssetState::Active, pRichPresence->GetState());
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 1U }, vmRichPresence.mockThreadPool.PendingTasks());

        // now callback will be called regularly
        vmRichPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "Display:\nHello, world 2!\n");
        vmRichPresence.mockLocalStorage.MockLastModified(ra::services::StorageItemType::RichPresence, L"1", tNow);
        vmRichPresence.mockThreadPool.AdvanceTime(std::chrono::seconds(1));
        Assert::AreEqual(std::wstring(L"Hello, world 2!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 1U }, vmRichPresence.mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestMonitoringStopsWhenGameUnloaded)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;

        vmRichPresence.mockGameContext.SetGameId(1);
        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Initial Value");
        vmRichPresence.mockGameContext.Assets().FindRichPresence()->Activate();
        vmRichPresence.SetIsVisible(true);
        Assert::AreEqual(std::wstring(L"Initial Value"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 1U }, vmRichPresence.mockThreadPool.PendingTasks());

        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Hello, world!");
        vmRichPresence.mockThreadPool.AdvanceTime(std::chrono::seconds(1));
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 1U }, vmRichPresence.mockThreadPool.PendingTasks());

        // game is unloaded - default text should be displayed and callback unscheduled
        vmRichPresence.mockGameContext.SetGameId(0);
        vmRichPresence.mockThreadPool.AdvanceTime(std::chrono::seconds(1));
        Assert::AreEqual(std::wstring(L"No game loaded."), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
