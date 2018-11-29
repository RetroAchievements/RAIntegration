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
    TEST_METHOD(TestInitialization)
    {
        RichPresenceMonitorViewModel vmRichPresence;
        Assert::AreEqual(std::wstring(L"Rich Presence Monitor"), vmRichPresence.GetWindowTitle());
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), vmRichPresence.GetDisplayString());
    }

    TEST_METHOD(TestUpdateDisplayString)
    {
        MockGameContext mockGameContext;
        MockThreadPool mockThreadPool;
        RichPresenceMonitorViewModel vmRichPresence;

        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"No game loaded."), vmRichPresence.GetDisplayString());
        Assert::AreEqual(0U, mockThreadPool.PendingTasks());

        mockGameContext.SetGameId(1);
        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), vmRichPresence.GetDisplayString());
        Assert::AreEqual(0U, mockThreadPool.PendingTasks());

        mockGameContext.SetRichPresenceDisplayString(L"Hello, world!");
        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(0U, mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestStartMonitoringNoGame)
    {
        MockGameContext mockGameContext;
        MockThreadPool mockThreadPool;
        MockLocalStorage mockLocalStorage;
        RichPresenceMonitorViewModel vmRichPresence;

        // with no game loaded, message should be static and no callback registered
        vmRichPresence.StartMonitoring();
        Assert::AreEqual(std::wstring(L"No game loaded."), vmRichPresence.GetDisplayString());
        Assert::AreEqual(0U, mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestStartMonitoringNoRichPresence)
    {
        MockGameContext mockGameContext;
        MockThreadPool mockThreadPool;
        MockLocalStorage mockLocalStorage;
        RichPresenceMonitorViewModel vmRichPresence;

        // with no rich presence, message should be static and no callback registered
        mockGameContext.SetGameId(1);
        vmRichPresence.StartMonitoring();
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), vmRichPresence.GetDisplayString());
        Assert::AreEqual(0U, mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestStartMonitoring)
    {
        MockGameContext mockGameContext;
        MockThreadPool mockThreadPool;
        MockLocalStorage mockLocalStorage;
        RichPresenceMonitorViewModel vmRichPresence;

        mockGameContext.SetGameId(1);
        mockGameContext.SetRichPresenceDisplayString(L"Initial Value");
        vmRichPresence.StartMonitoring();
        Assert::AreEqual(std::wstring(L"Initial Value"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(1U, mockThreadPool.PendingTasks());

        // display text shouldn't update until callback fires
        mockGameContext.SetRichPresenceDisplayString(L"Hello, world!");
        Assert::AreEqual(std::wstring(L"Initial Value"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(1U, mockThreadPool.PendingTasks());

        // callback fires, and schedules itself to be called again
        mockThreadPool.AdvanceTime(std::chrono::seconds(1));
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(1U, mockThreadPool.PendingTasks());

        mockGameContext.SetRichPresenceDisplayString(L"Hello, world 2!");
        mockThreadPool.AdvanceTime(std::chrono::seconds(1));
        Assert::AreEqual(std::wstring(L"Hello, world 2!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(1U, mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestStopMonitoring)
    {
        MockGameContext mockGameContext;
        MockThreadPool mockThreadPool;
        MockLocalStorage mockLocalStorage;
        RichPresenceMonitorViewModel vmRichPresence;

        mockGameContext.SetGameId(1);
        mockGameContext.SetRichPresenceDisplayString(L"Hello, world!");
        vmRichPresence.StartMonitoring();
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(1U, mockThreadPool.PendingTasks());

        // when monitoring stops, updated message should be ignored. callback will still be
        // called once, but won't reschedule itself
        vmRichPresence.StopMonitoring();
        Assert::AreEqual(1U, mockThreadPool.PendingTasks());

        mockGameContext.SetRichPresenceDisplayString(L"Hello, world 2!");
        mockThreadPool.AdvanceTime(std::chrono::seconds(1));
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(0U, mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestMonitoringStartsWhenGameLoaded)
    {
        MockGameContext mockGameContext;
        MockThreadPool mockThreadPool;
        MockLocalStorage mockLocalStorage;
        RichPresenceMonitorViewModel vmRichPresence;

        // with no game loaded, message should be static and no callback registered
        vmRichPresence.StartMonitoring();
        Assert::AreEqual(std::wstring(L"No game loaded."), vmRichPresence.GetDisplayString());
        Assert::AreEqual(0U, mockThreadPool.PendingTasks());

        // without a callback, display string is not automatically updated
        mockGameContext.SetGameId(1);
        mockGameContext.SetRichPresenceDisplayString(L"Hello, world!");
        mockThreadPool.AdvanceTime(std::chrono::seconds(1));
        Assert::AreEqual(std::wstring(L"No game loaded."), vmRichPresence.GetDisplayString());
        Assert::AreEqual(0U, mockThreadPool.PendingTasks());

        // explicit Update causes callback to be scheduled
        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(1U, mockThreadPool.PendingTasks());

        // now callback will be called regularly
        mockGameContext.SetRichPresenceDisplayString(L"Hello, world 2!");
        mockThreadPool.AdvanceTime(std::chrono::seconds(1));
        Assert::AreEqual(std::wstring(L"Hello, world 2!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(1U, mockThreadPool.PendingTasks());
    }

    TEST_METHOD(TestMonitoringStopsWhenGameUnloaded)
    {
        MockGameContext mockGameContext;
        MockThreadPool mockThreadPool;
        MockLocalStorage mockLocalStorage;
        RichPresenceMonitorViewModel vmRichPresence;

        mockGameContext.SetGameId(1);
        mockGameContext.SetRichPresenceDisplayString(L"Initial Value");
        vmRichPresence.StartMonitoring();
        Assert::AreEqual(std::wstring(L"Initial Value"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(1U, mockThreadPool.PendingTasks());

        mockGameContext.SetRichPresenceDisplayString(L"Hello, world!");
        mockThreadPool.AdvanceTime(std::chrono::seconds(1));
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(1U, mockThreadPool.PendingTasks());

        // game is unloaded - default text should be displayed and callback unscheduled
        mockGameContext.SetGameId(0);
        mockThreadPool.AdvanceTime(std::chrono::seconds(1));
        Assert::AreEqual(std::wstring(L"No game loaded."), vmRichPresence.GetDisplayString());
        Assert::AreEqual(0U, mockThreadPool.PendingTasks());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
