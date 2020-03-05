#include "CppUnitTest.h"

#include "ui\viewmodels\RichPresenceMonitorViewModel.hh"

#include "services\ServiceLocator.hh"

#include "ui\IDesktop.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockThreadPool.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::data::mocks::MockGameContext;
using ra::services::mocks::MockLocalStorage;
using ra::services::mocks::MockThreadPool;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(RichPresenceMonitorViewModel_Tests)
{
    class RichPresenceMonitorViewModelHarness : public RichPresenceMonitorViewModel
    {
    public:
        RichPresenceMonitorViewModelHarness()
        {
            InitializeNotifyTargets();
        }

        ra::data::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockLocalStorage mockLocalStorage;
        ra::services::mocks::MockThreadPool mockThreadPool;

        void UpdateDisplayString() { RichPresenceMonitorViewModel::UpdateDisplayString(); }
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
        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
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

    TEST_METHOD(TestStopMonitoring)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;

        vmRichPresence.mockGameContext.SetGameId(1);
        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Hello, world!");
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

        // with no game loaded, message should be static and no callback registered
        vmRichPresence.SetIsVisible(true);
        Assert::AreEqual(std::wstring(L"No game loaded."), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());

        // without a callback, display string is not automatically updated
        vmRichPresence.mockGameContext.SetGameId(1);
        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Hello, world!");
        vmRichPresence.mockThreadPool.AdvanceTime(std::chrono::seconds(1));
        Assert::AreEqual(std::wstring(L"No game loaded."), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 0U }, vmRichPresence.mockThreadPool.PendingTasks());

        // explicit Update causes callback to be scheduled
        vmRichPresence.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 1U }, vmRichPresence.mockThreadPool.PendingTasks());

        // now callback will be called regularly
        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Hello, world 2!");
        vmRichPresence.mockThreadPool.AdvanceTime(std::chrono::seconds(1));
        Assert::AreEqual(std::wstring(L"Hello, world 2!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual({ 1U }, vmRichPresence.mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestMonitoringStopsWhenGameUnloaded)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;

        vmRichPresence.mockGameContext.SetGameId(1);
        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Initial Value");
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
