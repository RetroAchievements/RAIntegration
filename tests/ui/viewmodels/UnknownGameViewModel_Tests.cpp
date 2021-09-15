#include "CppUnitTest.h"

#include "ui\viewmodels\UnknownGameViewModel.hh"

#include "services\ServiceLocator.hh"

#include "tests\mocks\MockClipboard.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockThreadPool.hh"

#include "tests\ui\UIAsserts.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(UnknownGameViewModel_Tests)
{
private:
    class UnknownGameViewModelHarness : public UnknownGameViewModel
    {
    public:
        ra::api::mocks::MockServer mockServer;
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockDesktop mockDesktop;

        void MockGameTitles()
        {
            m_vGameTitles.Add(20, L"Game 20");
            m_vGameTitles.Add(30, L"Game 30");
            m_vGameTitles.Add(40, L"Game 40");
            m_vGameTitles.Add(50, L"Game 50");
            m_vGameTitles.Add(60, L"Game 60");
            m_vGameTitles.Freeze();
        }

        void TestAsyncHandle(ra::services::mocks::MockThreadPool& threadPool, std::function<void()>&& fCallback)
        {
            threadPool.RunAsync([pAsyncHandle = CreateAsyncHandle(), fCallback = std::move(fCallback)](){
                if (!pAsyncHandle->IsDestroyed())
                    fCallback();
            });
        }
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        UnknownGameViewModelHarness vmUnknownGame;
        Assert::AreEqual(std::wstring(L"Unknown Title"), vmUnknownGame.GetWindowTitle());
        Assert::AreEqual(0, vmUnknownGame.GetSelectedGameId());
        Assert::AreEqual(std::wstring(L""), vmUnknownGame.GetNewGameName());
        Assert::AreEqual(std::wstring(L""), vmUnknownGame.GetChecksum());
        Assert::AreEqual({ 0U }, vmUnknownGame.GameTitles().Count());
        Assert::IsTrue(vmUnknownGame.IsSelectedGameEnabled());
    }

    TEST_METHOD(TestInitializeGameTitles)
    {
        UnknownGameViewModelHarness vmUnknownGame;
        vmUnknownGame.mockConsoleContext.SetId(ConsoleID::C64);

        vmUnknownGame.mockServer.HandleRequest<ra::api::FetchGamesList>([]
            (const ra::api::FetchGamesList::Request& request, ra::api::FetchGamesList::Response& response)
        {
            Assert::AreEqual(30U, request.ConsoleId);

            response.Result = ra::api::ApiResult::Success;
            response.Games.emplace_back(33U, L"Game 33");
            response.Games.emplace_back(37U, L"Game 37");

            return true;
        });

        vmUnknownGame.InitializeGameTitles();

        // <New Title> item should be loaded immediately, but linking should be disabled
        Assert::AreEqual({ 1U }, vmUnknownGame.GameTitles().Count());
        Assert::AreEqual(std::wstring(L"<New Title>"), vmUnknownGame.GameTitles().GetLabelForId(0));
        Assert::IsFalse(vmUnknownGame.GameTitles().IsFrozen());
        Assert::IsFalse(vmUnknownGame.IsAssociateEnabled());
        Assert::IsFalse(vmUnknownGame.IsSelectedGameEnabled());

        // after server response, collection should have three items and be frozen. linking should be enabled
        vmUnknownGame.mockThreadPool.ExecuteNextTask();
        Assert::AreEqual({ 3U }, vmUnknownGame.GameTitles().Count());
        Assert::AreEqual(std::wstring(L"<New Title>"), vmUnknownGame.GameTitles().GetLabelForId(0));
        Assert::AreEqual(std::wstring(L"Game 33"), vmUnknownGame.GameTitles().GetLabelForId(33));
        Assert::AreEqual(std::wstring(L"Game 37"), vmUnknownGame.GameTitles().GetLabelForId(37));
        Assert::IsTrue(vmUnknownGame.GameTitles().IsFrozen());
        Assert::IsTrue(vmUnknownGame.IsAssociateEnabled());
        Assert::IsTrue(vmUnknownGame.IsSelectedGameEnabled());
    }

    TEST_METHOD(TestNameAndSelectionInteraction)
    {
        UnknownGameViewModelHarness vmUnknownGame;
        vmUnknownGame.MockGameTitles();

        // selecting an existing game should update the game name
        vmUnknownGame.SetSelectedGameId(30);
        Assert::AreEqual(std::wstring(L"Game 30"), vmUnknownGame.GetNewGameName());
        Assert::AreEqual(30, vmUnknownGame.GetSelectedGameId());

        // selecting <New Title> should not update the game name
        vmUnknownGame.SetSelectedGameId(0);
        Assert::AreEqual(std::wstring(L"Game 30"), vmUnknownGame.GetNewGameName());
        Assert::AreEqual(0, vmUnknownGame.GetSelectedGameId());

        // selecting an existing game should update the game name
        vmUnknownGame.SetSelectedGameId(50);
        Assert::AreEqual(std::wstring(L"Game 50"), vmUnknownGame.GetNewGameName());
        Assert::AreEqual(50, vmUnknownGame.GetSelectedGameId());

        // changing the game name should select <New Title>, but not update the game name
        vmUnknownGame.SetNewGameName(L"Game 50b");
        Assert::AreEqual(std::wstring(L"Game 50b"), vmUnknownGame.GetNewGameName());
        Assert::AreEqual(0, vmUnknownGame.GetSelectedGameId());

        // reselecting game should reset name
        vmUnknownGame.SetSelectedGameId(50);
        Assert::AreEqual(std::wstring(L"Game 50"), vmUnknownGame.GetNewGameName());
        Assert::AreEqual(50, vmUnknownGame.GetSelectedGameId());
    }

    TEST_METHOD(TestCopyToClipboard)
    {
        ra::services::mocks::MockClipboard mockClipboard;
        UnknownGameViewModelHarness vmUnknownGame;

        Assert::AreEqual(std::wstring(L""), vmUnknownGame.GetChecksum());
        Assert::AreEqual(std::wstring(), mockClipboard.GetText());

        vmUnknownGame.CopyChecksumToClipboard();
        Assert::AreEqual(std::wstring(L""), mockClipboard.GetText());

        vmUnknownGame.SetChecksum(L"abc123");
        Assert::AreEqual(std::wstring(L"abc123"), vmUnknownGame.GetChecksum());
        Assert::AreEqual(std::wstring(L""), mockClipboard.GetText());

        vmUnknownGame.CopyChecksumToClipboard();
        Assert::AreEqual(std::wstring(L"abc123"), vmUnknownGame.GetChecksum());
        Assert::AreEqual(std::wstring(L"abc123"), mockClipboard.GetText());

        vmUnknownGame.SetChecksum(L"def456");
        Assert::AreEqual(std::wstring(L"def456"), vmUnknownGame.GetChecksum());
        Assert::AreEqual(std::wstring(L"abc123"), mockClipboard.GetText());
    }

    TEST_METHOD(TestAssociateExisting)
    {
        UnknownGameViewModelHarness vmUnknownGame;
        vmUnknownGame.mockConsoleContext.SetId(ConsoleID::PSP);
        vmUnknownGame.MockGameTitles();
        vmUnknownGame.SetSelectedGameId(40);
        vmUnknownGame.SetChecksum(L"CHECKSUM");
        vmUnknownGame.SetEstimatedGameName(L"GAME40");

        vmUnknownGame.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Are you sure you want to add a new hash to 'Game 40'?"), vmMessageBox.GetHeader());
            return ra::ui::DialogResult::Yes;
        });

        vmUnknownGame.mockServer.HandleRequest<ra::api::SubmitNewTitle>([]
            (const ra::api::SubmitNewTitle::Request& request, ra::api::SubmitNewTitle::Response& response)
        {
            Assert::AreEqual(41U, request.ConsoleId);
            Assert::AreEqual(std::string("CHECKSUM"), request.Hash);
            Assert::AreEqual(std::wstring(L"Game 40"), request.GameName);
            Assert::AreEqual(std::wstring(L"GAME40"), request.Description);
            Assert::AreEqual(40U, request.GameId);

            response.Result = ra::api::ApiResult::Success;
            response.GameId = 40U;

            return true;
        });

        Assert::IsTrue(vmUnknownGame.Associate());
        Assert::AreEqual(40, vmUnknownGame.GetSelectedGameId());
        Assert::IsFalse(vmUnknownGame.GetTestMode());
    }

    TEST_METHOD(TestAssociateExistingDecline)
    {
        UnknownGameViewModelHarness vmUnknownGame;
        vmUnknownGame.mockConsoleContext.SetId(ConsoleID::PSP);
        vmUnknownGame.MockGameTitles();
        vmUnknownGame.SetSelectedGameId(40);
        vmUnknownGame.SetChecksum(L"CHECKSUM");

        vmUnknownGame.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            return ra::ui::DialogResult::No;
        });

        vmUnknownGame.mockServer.ExpectUncalled<ra::api::SubmitNewTitle>();

        Assert::IsFalse(vmUnknownGame.Associate());
    }

    TEST_METHOD(TestAssociateNew)
    {
        UnknownGameViewModelHarness vmUnknownGame;
        vmUnknownGame.mockConsoleContext.SetId(ConsoleID::VIC20);
        vmUnknownGame.MockGameTitles();
        vmUnknownGame.SetNewGameName(L"Game 96");
        vmUnknownGame.SetChecksum(L"CHECKSUM");
        vmUnknownGame.SetEstimatedGameName(L"GAME96");

        vmUnknownGame.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Are you sure you want to create a new entry for 'Game 96'?"), vmMessageBox.GetHeader());
            return ra::ui::DialogResult::Yes;
        });

        vmUnknownGame.mockServer.HandleRequest<ra::api::SubmitNewTitle>([]
            (const ra::api::SubmitNewTitle::Request& request, ra::api::SubmitNewTitle::Response& response)
        {
            Assert::AreEqual(34U, request.ConsoleId);
            Assert::AreEqual(std::string("CHECKSUM"), request.Hash);
            Assert::AreEqual(std::wstring(L"Game 96"), request.GameName);
            Assert::AreEqual(std::wstring(L"GAME96"), request.Description);
            Assert::AreEqual(0U, request.GameId);

            response.Result = ra::api::ApiResult::Success;
            response.GameId = 102U;

            return true;
        });

        Assert::IsTrue(vmUnknownGame.Associate());
        Assert::AreEqual(102, vmUnknownGame.GetSelectedGameId());
        Assert::IsFalse(vmUnknownGame.GetTestMode());
    }

    TEST_METHOD(TestAssociateNewTrimmed)
    {
        UnknownGameViewModelHarness vmUnknownGame;
        vmUnknownGame.mockConsoleContext.SetId(ConsoleID::VIC20);
        vmUnknownGame.MockGameTitles();
        vmUnknownGame.SetNewGameName(L"  Game 96  ");
        vmUnknownGame.SetChecksum(L"CHECKSUM");
        vmUnknownGame.SetEstimatedGameName(L"GAME96");

        // name should be trimmed in both the dialog and the request
        vmUnknownGame.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Are you sure you want to create a new entry for 'Game 96'?"), vmMessageBox.GetHeader());
            return ra::ui::DialogResult::Yes;
        });

        vmUnknownGame.mockServer.HandleRequest<ra::api::SubmitNewTitle>([]
        (const ra::api::SubmitNewTitle::Request& request, ra::api::SubmitNewTitle::Response& response)
        {
            Assert::AreEqual(34U, request.ConsoleId);
            Assert::AreEqual(std::string("CHECKSUM"), request.Hash);
            Assert::AreEqual(std::wstring(L"Game 96"), request.GameName);
            Assert::AreEqual(std::wstring(L"GAME96"), request.Description);
            Assert::AreEqual(0U, request.GameId);

            response.Result = ra::api::ApiResult::Success;
            response.GameId = 102U;

            return true;
        });

        Assert::IsTrue(vmUnknownGame.Associate());
        Assert::AreEqual(102, vmUnknownGame.GetSelectedGameId());
    }

    TEST_METHOD(TestAssociateNewDecline)
    {
        UnknownGameViewModelHarness vmUnknownGame;
        vmUnknownGame.mockConsoleContext.SetId(ConsoleID::VIC20);
        vmUnknownGame.MockGameTitles();
        vmUnknownGame.SetNewGameName(L"Game 96");
        vmUnknownGame.SetChecksum(L"CHECKSUM");

        vmUnknownGame.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            return ra::ui::DialogResult::No;
        });

        vmUnknownGame.mockServer.ExpectUncalled<ra::api::SubmitNewTitle>();

        Assert::IsFalse(vmUnknownGame.Associate());
    }

    TEST_METHOD(TestAssociateNewNoName)
    {
        UnknownGameViewModelHarness vmUnknownGame;
        vmUnknownGame.mockConsoleContext.SetId(ConsoleID::VIC20);
        vmUnknownGame.MockGameTitles();
        vmUnknownGame.SetNewGameName(L"");
        vmUnknownGame.SetChecksum(L"CHECKSUM");

        vmUnknownGame.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"New game name must be at least three characters long."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        vmUnknownGame.mockServer.ExpectUncalled<ra::api::SubmitNewTitle>();

        Assert::IsFalse(vmUnknownGame.Associate());
    }

    TEST_METHOD(TestAsyncHandle)
    {
        ra::services::mocks::MockThreadPool mockThreadPool;

        bool bCallbackCalled = false;
        {
            UnknownGameViewModelHarness vmViewModel;
            vmViewModel.TestAsyncHandle(mockThreadPool, [&bCallbackCalled] { bCallbackCalled = true; });

            Assert::IsFalse(bCallbackCalled);
            mockThreadPool.ExecuteNextTask();
            Assert::IsTrue(bCallbackCalled);

            bCallbackCalled = false;
            vmViewModel.TestAsyncHandle(mockThreadPool, [&bCallbackCalled] { bCallbackCalled = true; });
            // vmViewModel will be destroyed here, so when the next task is executed, the AsyncHandle will
            // prevent the callback and bCallbackCalled should remain false
        }

        Assert::IsFalse(bCallbackCalled);
        mockThreadPool.ExecuteNextTask();
        Assert::IsFalse(bCallbackCalled);
    }

    TEST_METHOD(TestBeginTestNewGame)
    {
        UnknownGameViewModelHarness vmUnknownGame;
        vmUnknownGame.mockConsoleContext.SetId(ConsoleID::VIC20);
        vmUnknownGame.MockGameTitles();
        vmUnknownGame.SetNewGameName(L"TestGame");
        vmUnknownGame.SetChecksum(L"CHECKSUM");

        vmUnknownGame.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"You must select an existing game to test compatibility."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        Assert::IsFalse(vmUnknownGame.BeginTest());
    }

    TEST_METHOD(TestBeginTestExistingGame)
    {
        UnknownGameViewModelHarness vmUnknownGame;
        vmUnknownGame.mockConsoleContext.SetId(ConsoleID::VIC20);
        vmUnknownGame.MockGameTitles();
        vmUnknownGame.SetNewGameName(L"TestGame");
        vmUnknownGame.SetSelectedGameId(40U);
        vmUnknownGame.SetChecksum(L"CHECKSUM");

        vmUnknownGame.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Play 'Game 40' in compatability test mode?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Achievements and leaderboards for the game will be loaded, but you will not be able to earn them."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::Yes;
        });

        Assert::IsTrue(vmUnknownGame.BeginTest());
        Assert::AreEqual(40, vmUnknownGame.GetSelectedGameId());
        Assert::IsTrue(vmUnknownGame.GetTestMode());
    }

    TEST_METHOD(TestBeginTestExistingGameCancel)
    {
        UnknownGameViewModelHarness vmUnknownGame;
        vmUnknownGame.mockConsoleContext.SetId(ConsoleID::VIC20);
        vmUnknownGame.MockGameTitles();
        vmUnknownGame.SetNewGameName(L"TestGame");
        vmUnknownGame.SetSelectedGameId(40U);
        vmUnknownGame.SetChecksum(L"CHECKSUM");

        vmUnknownGame.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Play 'Game 40' in compatability test mode?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Achievements and leaderboards for the game will be loaded, but you will not be able to earn them."), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::No;
        });

        Assert::IsFalse(vmUnknownGame.BeginTest());
    }

    TEST_METHOD(TestInitializeTestCompatibilityMode)
    {
        UnknownGameViewModelHarness vmUnknownGame;
        vmUnknownGame.mockConsoleContext.SetId(ConsoleID::C64);
        vmUnknownGame.mockGameContext.SetGameId(33U);
        vmUnknownGame.mockGameContext.SetGameTitle(L"Game 33");
        vmUnknownGame.mockGameContext.SetGameHash("CHECKSUM");

        vmUnknownGame.InitializeTestCompatibilityMode();

        Assert::AreEqual({ 1U }, vmUnknownGame.GameTitles().Count());
        Assert::AreEqual(33, vmUnknownGame.GameTitles().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"Game 33"), vmUnknownGame.GameTitles().GetItemAt(0)->GetLabel());
        Assert::AreEqual(33, vmUnknownGame.GetSelectedGameId());
        Assert::AreEqual(std::wstring(L"CHECKSUM"), vmUnknownGame.GetChecksum());
        Assert::IsTrue(vmUnknownGame.GameTitles().IsFrozen());

        Assert::IsFalse(vmUnknownGame.IsSelectedGameEnabled());
    }

    TEST_METHOD(TestAssociateExistingTestCompatibilityMode)
    {
        UnknownGameViewModelHarness vmUnknownGame;
        vmUnknownGame.mockConsoleContext.SetId(ConsoleID::PSP);
        vmUnknownGame.mockGameContext.SetGameId(40U);
        vmUnknownGame.mockGameContext.SetGameTitle(L"Game 40");
        vmUnknownGame.mockGameContext.SetGameHash("CHECKSUM");
        vmUnknownGame.SetEstimatedGameName(L"GAME40");
        vmUnknownGame.InitializeTestCompatibilityMode();

        vmUnknownGame.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Are you sure you want to add a new hash to 'Game 40'?"), vmMessageBox.GetHeader());
            return ra::ui::DialogResult::Yes;
        });

        vmUnknownGame.mockServer.HandleRequest<ra::api::SubmitNewTitle>([]
        (const ra::api::SubmitNewTitle::Request& request, ra::api::SubmitNewTitle::Response& response)
        {
            Assert::AreEqual(41U, request.ConsoleId);
            Assert::AreEqual(std::string("CHECKSUM"), request.Hash);
            Assert::AreEqual(std::wstring(L"Game 40"), request.GameName);
            Assert::AreEqual(std::wstring(L"GAME40"), request.Description);
            Assert::AreEqual(40U, request.GameId);

            response.Result = ra::api::ApiResult::Success;
            response.GameId = 40U;

            return true;
        });

        Assert::IsTrue(vmUnknownGame.Associate());
        Assert::AreEqual(40, vmUnknownGame.GetSelectedGameId());
        Assert::IsFalse(vmUnknownGame.GetTestMode());
    }};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
