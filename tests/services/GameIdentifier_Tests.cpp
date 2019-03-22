#include "services\GameIdentifier.hh"

#include "ui\viewmodels\UnknownGameViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockAudioSystem.hh"
#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockSessionTracker.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace services {
namespace tests {

std::array<BYTE, 16> ROM = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };
std::string ROM_HASH = "190c4c105786a2121d85018939108a6c";

class GameIdentifierHarness : public GameIdentifier
{
public:
    GameIdentifierHarness()
        : mockConsoleContext(ConsoleID::NES, L"NES")
    {
    }

    void MockResolveHashResponse(unsigned int nGameId)
    {
        mockServer.HandleRequest<ra::api::ResolveHash>(
            [nGameId](const ra::api::ResolveHash::Request& request, ra::api::ResolveHash::Response& response)
        {
            Assert::AreEqual(ROM_HASH, request.Hash);
            response.Result = ra::api::ApiResult::Success;
            response.GameId = nGameId;
            return true;
        });
    }

    ra::api::mocks::MockServer mockServer;
    ra::data::mocks::MockConsoleContext mockConsoleContext;
    ra::data::mocks::MockEmulatorContext mockEmulatorContext;
    ra::data::mocks::MockGameContext mockGameContext;
    ra::data::mocks::MockSessionTracker mockSessionTracker;
    ra::data::mocks::MockUserContext mockUserContext;
    ra::services::mocks::MockConfiguration mockConfiguration;
    ra::ui::mocks::MockDesktop mockDesktop;
    ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;

private:
    ra::services::mocks::MockAudioSystem mockAudioSystem;
    ra::services::mocks::MockClock mockClock;
    ra::services::mocks::MockLocalStorage mockLocalStorage;
    ra::services::mocks::MockThreadPool mockThreadPool;
};

TEST_CLASS(GameIdentifier_Tests)
{
private:

public:
    TEST_METHOD(TestIdentifyGameKnown)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(23U);

        Assert::AreEqual(23U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        Assert::AreEqual(0U, identifier.mockGameContext.GameId());
    }

    TEST_METHOD(TestIdentifyGameNull)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(23U);

        Assert::AreEqual(0U, identifier.IdentifyGame(nullptr, ROM.size()));
        Assert::AreEqual(0U, identifier.IdentifyGame(&ROM.at(0), 0U));

        // matching game ID should only update hash if it's non-zero
        Assert::AreEqual(std::string(), identifier.mockGameContext.GameHash());
    }

    TEST_METHOD(TestIdentifyGameUnknownCancel)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(0U);
        identifier.mockEmulatorContext.SetGetGameTitleFunction([](char* sBuffer) { strcpy_s(sBuffer, 16, "TestGame"); });
        identifier.mockUserContext.Initialize("User", "ApiToken");

        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::UnknownGameViewModel>(
            [&bDialogShown](ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame)
        {
            bDialogShown = true;
            Assert::AreEqual(std::wstring(L"NES"), vmUnknownGame.GetSystemName());
            Assert::AreEqual(std::wstring(L"190c4c105786a2121d85018939108a6c"), vmUnknownGame.GetChecksum());
            Assert::AreEqual(std::wstring(L"TestGame"), vmUnknownGame.GetEstimatedGameName());
            Assert::AreEqual(std::wstring(L"TestGame"), vmUnknownGame.GetNewGameName());
            return ra::ui::DialogResult::Cancel;
        });

        Assert::AreEqual(0U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        Assert::IsTrue(bDialogShown);
    }

    TEST_METHOD(TestIdentifyGameUnknownSelected)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(0U);
        identifier.mockEmulatorContext.SetGetGameTitleFunction([](char* sBuffer) { strcpy_s(sBuffer, 16, "TestGame"); });
        identifier.mockUserContext.Initialize("User", "ApiToken");

        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::UnknownGameViewModel>(
            [&bDialogShown](ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame)
        {
            bDialogShown = true;
            Assert::AreEqual(std::wstring(L"NES"), vmUnknownGame.GetSystemName());
            Assert::AreEqual(std::wstring(L"190c4c105786a2121d85018939108a6c"), vmUnknownGame.GetChecksum());
            Assert::AreEqual(std::wstring(L"TestGame"), vmUnknownGame.GetEstimatedGameName());
            Assert::AreEqual(std::wstring(L"TestGame"), vmUnknownGame.GetNewGameName());
            vmUnknownGame.SetSelectedGameId(23);
            return ra::ui::DialogResult::OK;
        });

        Assert::AreEqual(23U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        Assert::IsTrue(bDialogShown);
    }

    TEST_METHOD(TestIdentifyGameUnknownNotLoggedIn)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(0U);
        identifier.mockEmulatorContext.SetGetGameTitleFunction([](char* sBuffer) { strcpy_s(sBuffer, 16, "TestGame"); });

        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::UnknownGameViewModel>(
            [&bDialogShown](ra::ui::viewmodels::UnknownGameViewModel&)
        {
            bDialogShown = true;
            return ra::ui::DialogResult::Cancel;
        });

        Assert::AreEqual(0U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        Assert::IsFalse(bDialogShown);
    }

    TEST_METHOD(TestIdentifyGameNoConsole)
    {
        GameIdentifierHarness identifier;
        identifier.mockConsoleContext.SetId(ConsoleID::UnknownConsoleID);

        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Cannot identify game for unknown console."), vmMessageBox.GetMessage());
            bDialogShown = true;
            return ra::ui::DialogResult::OK;
        });

        Assert::AreEqual(0U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
    }

    TEST_METHOD(TestIdentifyGameHashChange)
    {
        // calling IdentifyGame with a different block of memory that resolves to the currently
        // loaded game should update the game's hash
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(23U);
        identifier.mockGameContext.SetGameId(23U);
        identifier.mockGameContext.SetGameHash("HASH");

        Assert::AreEqual(23U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        Assert::AreEqual(ROM_HASH, identifier.mockGameContext.GameHash());
    }

    TEST_METHOD(TestActivateGameZeroGameNotLoaded)
    {
        GameIdentifierHarness identifier;
        identifier.ActivateGame(0U);

        Assert::AreEqual(0U, identifier.mockGameContext.GameId());
        Assert::AreEqual(std::string(), identifier.mockGameContext.GameHash());
        Assert::AreEqual(0U, identifier.mockSessionTracker.CurrentSessionGameId());
    }

    TEST_METHOD(TestActivateGameZeroGameLoaded)
    {
        GameIdentifierHarness identifier;
        identifier.mockGameContext.SetGameId(23U);
        identifier.mockGameContext.SetGameHash("ABCDEF");
        identifier.mockSessionTracker.BeginSession(23U);
        Assert::AreEqual(23U, identifier.mockSessionTracker.CurrentSessionGameId());

        identifier.ActivateGame(0U);

        Assert::AreEqual(0U, identifier.mockGameContext.GameId());
        Assert::AreEqual(std::string(), identifier.mockGameContext.GameHash());
        Assert::AreEqual(0U, identifier.mockSessionTracker.CurrentSessionGameId());
    }

    TEST_METHOD(TestActivateGameNotLoggedIn)
    {
        GameIdentifierHarness identifier;
        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            bDialogShown = true;
            return ra::ui::DialogResult::OK;
        });

        identifier.ActivateGame(23U);

        Assert::AreEqual(0U, identifier.mockGameContext.GameId());
        Assert::IsTrue(bDialogShown);
    }

    TEST_METHOD(TestActivateGamePendingHardcore)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(23U);
        identifier.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        identifier.mockUserContext.Initialize("User", "ApiToken");

        Assert::AreEqual(23U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        identifier.ActivateGame(23U);

        Assert::AreEqual(23U, identifier.mockGameContext.GameId());
        Assert::AreEqual(ROM_HASH, identifier.mockGameContext.GameHash());
        Assert::AreEqual(23U, identifier.mockSessionTracker.CurrentSessionGameId());
        Assert::IsNull(identifier.mockOverlayManager.GetMessage(1U));
    }

    TEST_METHOD(TestActivateGamePendingNonHardcoreLeaderboards)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(23U);
        identifier.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        identifier.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Leaderboards, true);
        identifier.mockUserContext.Initialize("User", "ApiToken");

        Assert::AreEqual(23U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        identifier.ActivateGame(23U);

        Assert::AreEqual(23U, identifier.mockGameContext.GameId());
        Assert::AreEqual(ROM_HASH, identifier.mockGameContext.GameHash());
        Assert::AreEqual(23U, identifier.mockSessionTracker.CurrentSessionGameId());
        const auto* pPopup = identifier.mockOverlayManager.GetMessage(1U);
        Assert::AreEqual(std::wstring(L"Playing in Softcore Mode"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Leaderboard submissions will be canceled."), pPopup->GetDescription());
    }

    TEST_METHOD(TestActivateGamePendingNonHardcoreNoLeaderboards)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(23U);
        identifier.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        identifier.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Leaderboards, false);
        identifier.mockUserContext.Initialize("User", "ApiToken");

        Assert::AreEqual(23U, identifier.IdentifyGame(&ROM.at(0), ROM.size()));
        identifier.ActivateGame(23U);

        Assert::AreEqual(23U, identifier.mockGameContext.GameId());
        Assert::AreEqual(ROM_HASH, identifier.mockGameContext.GameHash());
        Assert::AreEqual(23U, identifier.mockSessionTracker.CurrentSessionGameId());
        const auto* pPopup = identifier.mockOverlayManager.GetMessage(1U);
        Assert::AreEqual(std::wstring(L"Playing in Softcore Mode"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L""), pPopup->GetDescription());
    }

    TEST_METHOD(TestIdentifyAndActivateGameHardcore)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(23U);
        identifier.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        identifier.mockUserContext.Initialize("User", "ApiToken");

        identifier.IdentifyAndActivateGame(&ROM.at(0), ROM.size());

        Assert::AreEqual(23U, identifier.mockGameContext.GameId());
        Assert::AreEqual(ROM_HASH, identifier.mockGameContext.GameHash());
        Assert::AreEqual(std::wstring(), identifier.mockGameContext.GameTitle());
        Assert::AreEqual(23U, identifier.mockSessionTracker.CurrentSessionGameId());
        Assert::IsNull(identifier.mockOverlayManager.GetMessage(1U));
    }

    TEST_METHOD(TestIdentifyAndActivateGameNull)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(23U);

        identifier.IdentifyAndActivateGame(nullptr, ROM.size());

        Assert::AreEqual(0U, identifier.mockGameContext.GameId());
        Assert::AreEqual(std::string(), identifier.mockGameContext.GameHash());
        Assert::AreEqual(std::wstring(), identifier.mockGameContext.GameTitle());
    }

    TEST_METHOD(TestIdentifyAndActivateGameUnknownCancel)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(0U);
        identifier.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        identifier.mockEmulatorContext.SetGetGameTitleFunction([](char* sBuffer) { strcpy_s(sBuffer, 16, "TestGame"); });
        identifier.mockUserContext.Initialize("User", "ApiToken");

        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::UnknownGameViewModel>(
            [&bDialogShown](ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame)
        {
            bDialogShown = true;
            Assert::AreEqual(std::wstring(L"NES"), vmUnknownGame.GetSystemName());
            Assert::AreEqual(std::wstring(L"190c4c105786a2121d85018939108a6c"), vmUnknownGame.GetChecksum());
            Assert::AreEqual(std::wstring(L"TestGame"), vmUnknownGame.GetEstimatedGameName());
            Assert::AreEqual(std::wstring(L"TestGame"), vmUnknownGame.GetNewGameName());
            return ra::ui::DialogResult::Cancel;
        });

        identifier.IdentifyAndActivateGame(&ROM.at(0), ROM.size());

        Assert::AreEqual(0U, identifier.mockGameContext.GameId());
        Assert::AreEqual(ROM_HASH, identifier.mockGameContext.GameHash());
        Assert::AreEqual(std::wstring(L"TestGame"), identifier.mockGameContext.GameTitle());
    }

    TEST_METHOD(TestIdentifyAndActivateGameUnknownSelected)
    {
        GameIdentifierHarness identifier;
        identifier.MockResolveHashResponse(0U);
        identifier.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        identifier.mockEmulatorContext.SetGetGameTitleFunction([](char* sBuffer) { strcpy_s(sBuffer, 16, "TestGame"); });
        identifier.mockUserContext.Initialize("User", "ApiToken");

        bool bDialogShown = false;
        identifier.mockDesktop.ExpectWindow<ra::ui::viewmodels::UnknownGameViewModel>(
            [&bDialogShown](ra::ui::viewmodels::UnknownGameViewModel& vmUnknownGame)
        {
            bDialogShown = true;
            Assert::AreEqual(std::wstring(L"NES"), vmUnknownGame.GetSystemName());
            Assert::AreEqual(std::wstring(L"190c4c105786a2121d85018939108a6c"), vmUnknownGame.GetChecksum());
            Assert::AreEqual(std::wstring(L"TestGame"), vmUnknownGame.GetEstimatedGameName());
            Assert::AreEqual(std::wstring(L"TestGame"), vmUnknownGame.GetNewGameName());
            vmUnknownGame.SetSelectedGameId(23);
            return ra::ui::DialogResult::OK;
        });

        identifier.IdentifyAndActivateGame(&ROM.at(0), ROM.size());

        Assert::IsTrue(bDialogShown);
        Assert::AreEqual(23U, identifier.mockGameContext.GameId());
        Assert::AreEqual(ROM_HASH, identifier.mockGameContext.GameHash());
        Assert::AreEqual(std::wstring(), identifier.mockGameContext.GameTitle());
    }
};

} // namespace tests
} // namespace services
} // namespace ra
