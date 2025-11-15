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
            GSL_SUPPRESS_F6 mockRuntime.MockGame();

            GSL_SUPPRESS_F6 InitializeNotifyTargets();
        }

        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockAchievementRuntime mockRuntime;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockLocalStorage mockLocalStorage;
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

        vmRichPresence.mockGameContext.SetGameId(1);
        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), vmRichPresence.GetDisplayString());

        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Hello, world!");
        vmRichPresence.mockGameContext.Assets().FindRichPresence()->Activate();
        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
    }

    TEST_METHOD(TestUpdateDisplayStringParseError)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;

        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"No game loaded."), vmRichPresence.GetDisplayString());

        vmRichPresence.mockGameContext.SetGameId(1);
        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), vmRichPresence.GetDisplayString());

        vmRichPresence.mockGameContext.SetRichPresenceDisplayString(L"Hello, world!");
        vmRichPresence.mockGameContext.Assets().FindRichPresence()->SetScript(
            "Lookup:a\n0=Zero\n1One\n\nDisplay:@a(0xH1234)");
        vmRichPresence.mockGameContext.Assets().FindRichPresence()->Activate();
        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Parse error -16 (line 3): Missing value expression"), vmRichPresence.GetDisplayString());

        vmRichPresence.mockGameContext.Assets().FindRichPresence()->SetScript("Display:Not Fixed");
        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Parse error -18 (line 2): Missing display string"), vmRichPresence.GetDisplayString());

        vmRichPresence.mockGameContext.Assets().FindRichPresence()->SetScript("Display:\nFixed");
        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Fixed"), vmRichPresence.GetDisplayString());
    }

    TEST_METHOD(TestUpdateDisplayStringNewFile)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;
        vmRichPresence.mockGameContext.SetGameId(1U);

        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), vmRichPresence.GetDisplayString());
        Assert::AreEqual(std::wstring(L"Rich Presence Monitor"), vmRichPresence.GetWindowTitle());

        // create new file
        const auto now = std::chrono::system_clock::now();
        vmRichPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1",
                                                       "Display:\nHello, world!");
        vmRichPresence.mockLocalStorage.MockLastModified(ra::services::StorageItemType::RichPresence, L"1", now);

        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(std::wstring(L"Rich Presence Monitor (local)"), vmRichPresence.GetWindowTitle());

        // update new file
        vmRichPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1",
                                                       "Display:\nHello, world 2!");
        vmRichPresence.mockLocalStorage.MockLastModified(ra::services::StorageItemType::RichPresence, L"1", now + std::chrono::minutes(2));

        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Hello, world 2!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(std::wstring(L"Rich Presence Monitor (local)"), vmRichPresence.GetWindowTitle());
    }

    TEST_METHOD(TestUpdateDisplayStringNewFileHardcore)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;
        vmRichPresence.mockGameContext.SetGameId(1U);
        vmRichPresence.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);

        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), vmRichPresence.GetDisplayString());
        Assert::AreEqual(std::wstring(L"Rich Presence Monitor"), vmRichPresence.GetWindowTitle());

        // create new file
        const auto now = std::chrono::system_clock::now();
        vmRichPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1",
                                                       "Display:\nHello, world!");
        vmRichPresence.mockLocalStorage.MockLastModified(ra::services::StorageItemType::RichPresence, L"1", now);

        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Rich Presence not active."), vmRichPresence.GetDisplayString());
        Assert::AreEqual(std::wstring(L"Rich Presence Monitor (local)"), vmRichPresence.GetWindowTitle());

        // update new file
        vmRichPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1",
                                                       "Display:\nHello, world 2!");
        vmRichPresence.mockLocalStorage.MockLastModified(ra::services::StorageItemType::RichPresence, L"1",
                                                         now + std::chrono::minutes(2));

        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Rich Presence not active."), vmRichPresence.GetDisplayString());
        Assert::AreEqual(std::wstring(L"Rich Presence Monitor (local)"), vmRichPresence.GetWindowTitle());
    }

    TEST_METHOD(TestUpdateDisplayStringGameChanged)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;
        vmRichPresence.mockGameContext.SetGameId(1U);

        const auto now = std::chrono::system_clock::now();
        vmRichPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1",
                                                       "Display:\nHello, world!");
        vmRichPresence.mockLocalStorage.MockLastModified(ra::services::StorageItemType::RichPresence, L"1", now);
        vmRichPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"2",
                                                       "Display:\nHello, world 2!");
        vmRichPresence.mockLocalStorage.MockLastModified(ra::services::StorageItemType::RichPresence, L"2", now + std::chrono::minutes(2));

        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(std::wstring(L"Rich Presence Monitor (local)"), vmRichPresence.GetWindowTitle());

        // change game
        vmRichPresence.mockGameContext.SetGameId(2U);
        vmRichPresence.mockGameContext.Assets().FindRichPresence()->ReloadRichPresenceScript();
        vmRichPresence.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual(std::wstring(L"Hello, world 2!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(std::wstring(L"Rich Presence Monitor (local)"), vmRichPresence.GetWindowTitle());

        // unload game
        vmRichPresence.mockGameContext.SetGameId(0U);
        vmRichPresence.mockGameContext.Assets().FindRichPresence()->ReloadRichPresenceScript();
        vmRichPresence.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual(std::wstring(L"No game loaded."), vmRichPresence.GetDisplayString());
        Assert::AreEqual(std::wstring(L"Rich Presence Monitor"), vmRichPresence.GetWindowTitle());
    }

    TEST_METHOD(TestUpdateDisplayStringNewFileExclusiveSubset)
    {
        RichPresenceMonitorViewModelHarness vmRichPresence;
        vmRichPresence.mockGameContext.SetGameId(1U);
        vmRichPresence.mockGameContext.SetActiveGameId(2U);

        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), vmRichPresence.GetDisplayString());
        Assert::AreEqual(std::wstring(L"Rich Presence Monitor"), vmRichPresence.GetWindowTitle());

        // create new file
        const auto now = std::chrono::system_clock::now();
        vmRichPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"2", "Display:\nHello, world!");
        vmRichPresence.mockLocalStorage.MockLastModified(ra::services::StorageItemType::RichPresence, L"2", now);

        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Hello, world!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(std::wstring(L"Rich Presence Monitor (local)"), vmRichPresence.GetWindowTitle());

        // update new file
        vmRichPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"2", "Display:\nHello, world 2!");
        vmRichPresence.mockLocalStorage.MockLastModified(ra::services::StorageItemType::RichPresence, L"2", now + std::chrono::minutes(2));

        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Hello, world 2!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(std::wstring(L"Rich Presence Monitor (local)"), vmRichPresence.GetWindowTitle());

        // update file for core game
        vmRichPresence.mockLocalStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "Display:\nYou're not supposed to see this!");
        vmRichPresence.mockLocalStorage.MockLastModified(ra::services::StorageItemType::RichPresence, L"1", now + std::chrono::minutes(2));

        vmRichPresence.UpdateDisplayString();
        Assert::AreEqual(std::wstring(L"Hello, world 2!"), vmRichPresence.GetDisplayString());
        Assert::AreEqual(std::wstring(L"Rich Presence Monitor (local)"), vmRichPresence.GetWindowTitle());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
