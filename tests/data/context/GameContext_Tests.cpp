#include "CppUnitTest.h"

#include "data\context\GameContext.hh"

#include "data\models\AchievementModel.hh"

#include "services\AchievementRuntime.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#include <rcheevos\src\rc_version.h>

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"
#include "tests\ui\UIAsserts.hh"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockAudioSystem.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockFrameEventQueue.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockSessionTracker.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::services::StorageItemType;

namespace ra {
namespace data {
namespace context {
namespace tests {

TEST_CLASS(GameContext_Tests)
{
private:
    static void RemoveFirstLine(std::string& sString)
    {
        const auto nIndex = sString.find('\n');
        if (nIndex == std::string::npos)
            sString.clear();
        else
            sString.erase(0, nIndex + 1);
    }

public:
    class GameContextHarness : public GameContext
    {
    public:
        GameContextHarness() noexcept :
            m_OverrideGameContext(this)
        {
        }

        ra::api::mocks::MockServer mockServer;
        ra::services::mocks::MockClock mockClock;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockLocalStorage mockStorage;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::services::mocks::MockAudioSystem mockAudioSystem;
        ra::services::mocks::MockAchievementRuntime mockAchievementRuntime;
        ra::services::mocks::MockFrameEventQueue mockFrameEventQueue;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockEmulatorContext mockEmulator;
        ra::data::context::mocks::MockSessionTracker mockSessionTracker;
        ra::data::context::mocks::MockUserContext mockUser;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        void SetGameId(unsigned int nGameId) noexcept { m_nGameId = nGameId; }

        void SetRichPresenceFromFile(bool bValue)
        {
            auto* pRichPresence = GetRichPresence();
            Expects(pRichPresence != nullptr);

            if (bValue)
                pRichPresence->SetScript("Display:\nThis differs\n");
            else
                pRichPresence->SetScript("Display:\nTest\n");
        }

        void ReloadRichPresenceScript()
        {
            auto* pRichPresence = GetRichPresence();
            Expects(pRichPresence != nullptr);
            pRichPresence->ReloadRichPresenceScript();
        }

        bool HasRichPresence() const
        {
            return Assets().FindRichPresence() != nullptr;
        }

        std::wstring GetRichPresenceDisplayString() const
        {
            return mockAchievementRuntime.GetRichPresenceDisplayString();
        }

        bool IsRichPresenceFromFile() const
        {
            const auto* pRichPresence = Assets().FindRichPresence();
            return pRichPresence != nullptr && pRichPresence->GetChanges() != ra::data::models::AssetChanges::None;
        }

        ra::data::models::AchievementModel& MockAchievement()
        {
            auto& pAch = Assets().NewAchievement();
            pAch.SetCategory(ra::data::models::AssetCategory::Core);
            pAch.SetID(1U);
            pAch.SetName(L"AchievementTitle");
            pAch.SetDescription(L"AchievementDescription");
            pAch.SetBadge(L"12345");
            pAch.SetPoints(5);
            pAch.SetState(ra::data::models::AssetState::Active);
            pAch.UpdateServerCheckpoint();
            return pAch;
        }

        ra::data::models::LeaderboardModel& MockLeaderboard()
        {
            auto& pLeaderboard = Assets().NewLeaderboard();
            pLeaderboard.SetID(1U);
            pLeaderboard.SetName(L"LeaderboardTitle");
            pLeaderboard.SetDescription(L"LeaderboardDescription");
            pLeaderboard.UpdateServerCheckpoint();
            return pLeaderboard;
        }

        void RemoveNonAchievementAssets()
        {
            for (gsl::index nIndex = Assets().Count() - 1; nIndex >= 0; nIndex--)
            {
                auto* pAsset = Assets().GetItemAt(nIndex);
                if (!pAsset || pAsset->GetType() != ra::data::models::AssetType::Achievement)
                    Assets().RemoveAt(nIndex);
            }
        }

        void MockLoadGameAPIs(unsigned nGameID, const std::string& sHash,
            const std::string& sUnlocks = "", const std::string& sAchievements = "",
            const std::string& sLeaderboards = "", const std::string& sRichPresence = "",
            ConsoleID nConsoleID = ConsoleID::MegaDrive)
        {
            mockConsoleContext.SetId(nConsoleID);
            mockConsoleContext.SetName(ra::Widen(rc_console_name(static_cast<int>(nConsoleID))));

            mockAchievementRuntime.MockUser("Username", "ApiToken");
            mockAchievementRuntime.MockResponse("r=gameid&m=" + sHash,
                "{\"Success\":true,\"GameID\":" + std::to_string(nGameID) + "}");
            mockAchievementRuntime.MockResponse(
                "r=patch&u=Username&t=ApiToken&g=" + std::to_string(nGameID),
                "{\"Success\":true,\"PatchData\":{"
                    "\"ID\":" + std::to_string(nGameID) + ","
                    "\"Title\":\"GameTitle\","
                    "\"ConsoleID\":" + std::to_string(ra::etoi(nConsoleID)) + ","
                    "\"ImageIcon\":\"/Images/9743.png\","
                    "\"Achievements\":[" + sAchievements + "],"
                    "\"Leaderboards\":[" + sLeaderboards + "],"
                    "\"RichPresencePatch\":\"" + sRichPresence + "\""
                "}}");
            mockAchievementRuntime.MockResponse(
                "r=startsession&u=Username&t=ApiToken&g=" + std::to_string(nGameID) +
                    "&h=1&m=" + sHash + "&l=" RCHEEVOS_VERSION_STRING,
                sUnlocks.empty() ? "{\"Success\":true}" : "{\"Success\":true," + sUnlocks + "}");
        }

        std::string MockAchievementJson(unsigned nAchievementID, unsigned nPoints,
            const std::string sMemAddr = "1=1")
        {
            const std::string sAchievementID = std::to_string(nAchievementID);
            return "{\"ID\":" + sAchievementID + ",\"Title\":\"Ach" + sAchievementID + "\"," +
                "\"Description\":\"Desc" + sAchievementID + "\",\"Flags\":3," +
                "\"Points\":" + std::to_string(nPoints) + ",\"MemAddr\":\"" + sMemAddr + "\"," +
                "\"Author\":\"User1\",\"BadgeName\":\"00234\"," +
                "\"Created\":1367266583,\"Modified\":1376929305}";
        }

    private:
        ra::data::models::RichPresenceModel* GetRichPresence()
        {
            auto* pRichPresence = Assets().FindRichPresence();
            if (!pRichPresence)
            {
                auto pNewRichPresence = std::make_unique<ra::data::models::RichPresenceModel>();
                pNewRichPresence->SetScript("Display:\nTest\n");
                pNewRichPresence->CreateServerCheckpoint();
                pNewRichPresence->CreateLocalCheckpoint();
                Assets().Append(std::move(pNewRichPresence));

                pRichPresence = Assets().FindRichPresence();
            }

            return pRichPresence;
        }

        ra::services::ServiceLocator::ServiceOverride<ra::data::context::GameContext> m_OverrideGameContext;
    };

    class GameContextNotifyTarget : public GameContext::NotifyTarget
    {
    public:
        bool GetActiveGameChanged() const noexcept { return m_bActiveGameChanged; }

        const std::wstring* GetNewCodeNote(ra::ByteAddress nAddress)
        {
            const auto pIter = m_vCodeNotesChanged.find(nAddress);
            if (pIter != m_vCodeNotesChanged.end())
                return &pIter->second;

            return nullptr;
        }

    protected:
        void OnActiveGameChanged() noexcept override
        {
            m_bActiveGameChanged = true;
            m_vCodeNotesChanged.clear();
        }

        void OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote) override
        {
            m_vCodeNotesChanged.insert_or_assign(nAddress, sNewNote);
        }

    private:
        bool m_bActiveGameChanged = false;
        std::map<ra::ByteAddress, std::wstring> m_vCodeNotesChanged;
    };

    TEST_METHOD(TestLoadGameTitle)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321");

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        Assert::AreEqual(1U, game.GameId());
        Assert::AreEqual(ra::data::context::GameContext::Mode::Normal, game.GetMode());
        Assert::AreEqual(std::wstring(L"GameTitle"), game.GameTitle());
    }

    TEST_METHOD(TestLoadGamePopup)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "",
            game.MockAchievementJson(123U, 5U));

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(std::wstring(L"Overlay\\info.wav")));

        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Loaded GameTitle"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"1 achievements, 5 points"), pPopup->GetDescription());
        Assert::AreEqual(std::string("9743"), pPopup->GetImage().Name());
    }

    TEST_METHOD(TestLoadGameTitleConsoleMismatch)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321");

        game.mockConsoleContext.SetId(ConsoleID::MasterSystem);
        game.mockConsoleContext.SetName(L"Master System");

        bool bDialogShown = false;
        game.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Identified game does not match expected console."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The game being loaded is associated to the Sega Genesis console, but the emulator has initialized "
                "the Master System console. This is not allowed as the memory maps may not be compatible between consoles."), vmMessageBox.GetMessage());
            bDialogShown = true;
            return ra::ui::DialogResult::OK;
        });

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        Assert::AreEqual(0U, game.GameId());
        Assert::AreEqual(ra::data::context::GameContext::Mode::Normal, game.GetMode());
        Assert::AreEqual(std::wstring(L""), game.GameTitle());
    }

    TEST_METHOD(TestLoadGameTitleConsoleMismatchGBvGBC)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "", "", "", "", ConsoleID::GB);

        game.mockConsoleContext.SetId(ConsoleID::GBC);
        game.mockConsoleContext.SetName(L"GameBoy Color");

        bool bDialogShown = false;
        game.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            bDialogShown = true;
            return ra::ui::DialogResult::OK;
        });

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(std::wstring(L"Overlay\\info.wav")));

        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Loaded GameTitle"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"0 achievements, 0 points"), pPopup->GetDescription());
        Assert::AreEqual(std::string("9743"), pPopup->GetImage().Name());

        Assert::IsFalse(bDialogShown);
    }

    TEST_METHOD(TestLoadGameTitleConsoleMismatchDSvDSi)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "", "", "", "", ConsoleID::DS);

        game.mockConsoleContext.SetId(ConsoleID::DSi);
        game.mockConsoleContext.SetName(L"Nintendo DSi");

        bool bDialogShown = false;
        game.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            bDialogShown = true;
            return ra::ui::DialogResult::OK;
        });

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(std::wstring(L"Overlay\\info.wav")));

        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Loaded GameTitle"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"0 achievements, 0 points"), pPopup->GetDescription());
        Assert::AreEqual(std::string("9743"), pPopup->GetImage().Name());

        Assert::IsFalse(bDialogShown);
    }

    TEST_METHOD(TestLoadGameTitleConsoleMismatchNESvEvent)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "", "", "", "", static_cast<ConsoleID>(RC_CONSOLE_EVENTS));

        game.mockConsoleContext.SetId(ConsoleID::NES);
        game.mockConsoleContext.SetName(L"Nintendo");

        bool bDialogShown = false;
        game.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel&) {
                bDialogShown = true;
                return ra::ui::DialogResult::OK;
            });

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(std::wstring(L"Overlay\\info.wav")));

        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Loaded GameTitle"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"0 achievements, 0 points"), pPopup->GetDescription());
        Assert::AreEqual(std::string("9743"), pPopup->GetImage().Name());

        Assert::IsFalse(bDialogShown);
    }

    TEST_METHOD(TestLoadGameTitleConsoleMismatchNESvStandalone)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "", "", "", "",
                              static_cast<ConsoleID>(RC_CONSOLE_STANDALONE));

        game.mockConsoleContext.SetId(ConsoleID::NES);
        game.mockConsoleContext.SetName(L"Nintendo");

        bool bDialogShown = false;
        game.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel&) {
                bDialogShown = true;
                return ra::ui::DialogResult::OK;
            });

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(std::wstring(L"Overlay\\info.wav")));

        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Loaded GameTitle"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"0 achievements, 0 points"), pPopup->GetDescription());
        Assert::AreEqual(std::string("9743"), pPopup->GetImage().Name());

        Assert::IsFalse(bDialogShown);
    }

    TEST_METHOD(TestLoadGameNotify)
    {
        class NotifyHarness : public GameContext::NotifyTarget
        {
        public:
            bool m_bNotified = false;

        protected:
            void OnActiveGameChanged() noexcept override { m_bNotified = true; }
        };
        NotifyHarness notifyHarness;

        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321");
        game.MockLoadGameAPIs(2U, "fedcba9876543210123456789abcdef");

        game.AddNotifyTarget(notifyHarness);
        game.LoadGame(0U, "");

        Assert::AreEqual(0U, game.GameId());
        Assert::IsFalse(notifyHarness.m_bNotified);

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");
        Assert::AreEqual(1U, game.GameId());
        Assert::IsTrue(notifyHarness.m_bNotified);

        notifyHarness.m_bNotified = false;
        game.LoadGame(2U, "fedcba9876543210123456789abcdef");
        Assert::AreEqual(2U, game.GameId());
        Assert::IsTrue(notifyHarness.m_bNotified);

        notifyHarness.m_bNotified = false;
        game.LoadGame(0U, "");
        Assert::AreEqual(0U, game.GameId());
        Assert::IsTrue(notifyHarness.m_bNotified);

        notifyHarness.m_bNotified = false;
        game.RemoveNotifyTarget(notifyHarness);
        game.LoadGame(2U, "fedcba9876543210123456789abcdef");
        Assert::AreEqual(2U, game.GameId());
        Assert::IsFalse(notifyHarness.m_bNotified);
    }

    TEST_METHOD(TestLoadGameRichPresence)
    {
        GameContextHarness game;
        const std::string sRichPresence = "Display:\nHello, World";
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "", "", "", sRichPresence);

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::string("Display:\nHello, World\n"),
            game.mockStorage.GetStoredData(ra::services::StorageItemType::RichPresence, L"1"));
        Assert::IsFalse(game.IsRichPresenceFromFile());

        const auto* pRichPresence = game.Assets().FindRichPresence();
        Assert::IsNotNull(pRichPresence);
        Ensures(pRichPresence != nullptr);
        Assert::AreEqual(ra::data::models::AssetCategory::Core, pRichPresence->GetCategory());
        Assert::AreEqual(ra::data::models::AssetChanges::None, pRichPresence->GetChanges());
        Assert::IsTrue(pRichPresence->IsActive());
    }

    TEST_METHOD(TestLoadGameDoesNotOverwriteRichPresenceFromFile)
    {
        GameContextHarness game;
        const std::string sRichPresence = "Display:\nHello, World";
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "", "", "", sRichPresence);

        game.mockStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "Display:\nFrom File\n");
        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::string("Display:\nFrom File\n"),
            game.mockStorage.GetStoredData(ra::services::StorageItemType::RichPresence, L"1"));
        Assert::IsTrue(game.IsRichPresenceFromFile());

        const auto* pRichPresence = game.Assets().FindRichPresence();
        Assert::IsNotNull(pRichPresence);
        Ensures(pRichPresence != nullptr);
        Assert::AreEqual(ra::data::models::AssetCategory::Core, pRichPresence->GetCategory());
        Assert::AreEqual(ra::data::models::AssetChanges::Unpublished, pRichPresence->GetChanges());

        // modified rich presence should not be active unless monitor is opened
        Assert::IsFalse(pRichPresence->IsActive());
    }

    TEST_METHOD(TestLoadGameRichPresenceUpdatedFromServer)
    {
        GameContextHarness game;
        const std::string sRichPresence = "Display:\nHello, World";
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "", "", "", sRichPresence);

        game.mockStorage.MockStoredData(ra::services::StorageItemType::GameData, L"1", "{\"RichPresencePatch\": \"Display:\\nOld\\n\"}");
        game.mockStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "Display:\nOld\n");

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::string("Display:\nHello, World\n"),
            game.mockStorage.GetStoredData(ra::services::StorageItemType::RichPresence, L"1"));
        Assert::IsFalse(game.IsRichPresenceFromFile());

        const auto* pRichPresence = game.Assets().FindRichPresence();
        Assert::IsNotNull(pRichPresence);
        Ensures(pRichPresence != nullptr);
        Assert::AreEqual(ra::data::models::AssetCategory::Core, pRichPresence->GetCategory());
        Assert::AreEqual(ra::data::models::AssetChanges::None, pRichPresence->GetChanges());
        Assert::IsTrue(pRichPresence->IsActive());
    }

    TEST_METHOD(TestLoadGameRichPresenceUpdatedFromServerLocalModifications)
    {
        GameContextHarness game;
        const std::string sRichPresence = "Display:\nHello, World";
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "", "", "", sRichPresence);

        game.mockStorage.MockStoredData(ra::services::StorageItemType::GameData, L"1", "{\"RichPresencePatch\": \"Display:\\nOld\\n\"}");
        game.mockStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "Display:\nLocal\n");

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::string("Display:\nLocal\n"),
            game.mockStorage.GetStoredData(ra::services::StorageItemType::RichPresence, L"1"));
        Assert::IsTrue(game.IsRichPresenceFromFile());

        const auto* pRichPresence = game.Assets().FindRichPresence();
        Assert::IsNotNull(pRichPresence);
        Ensures(pRichPresence != nullptr);
        Assert::AreEqual(ra::data::models::AssetCategory::Core, pRichPresence->GetCategory());
        Assert::AreEqual(ra::data::models::AssetChanges::Unpublished, pRichPresence->GetChanges());
        Assert::IsFalse(pRichPresence->IsActive());
    }

    TEST_METHOD(TestLoadGameRichPresenceNotOnServer)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321");

        game.mockStorage.MockStoredData(ra::services::StorageItemType::GameData, L"1", "{\"RichPresencePatch\": null}");

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        Assert::IsFalse(game.HasRichPresence());
        Assert::IsFalse(game.mockStorage.HasStoredData(ra::services::StorageItemType::RichPresence, L"1"));
        Assert::IsFalse(game.IsRichPresenceFromFile());

        const auto* pRichPresence = game.Assets().FindRichPresence();
        Assert::IsNull(pRichPresence);
    }

    TEST_METHOD(TestLoadGameNoRichPresence)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321");

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        Assert::IsFalse(game.HasRichPresence());
        Assert::AreEqual(std::string(),
                         game.mockStorage.GetStoredData(ra::services::StorageItemType::RichPresence, L"1"));
        Assert::IsFalse(game.IsRichPresenceFromFile());

        const auto* pRichPresence = game.Assets().FindRichPresence();
        Assert::IsNull(pRichPresence);
    }

    TEST_METHOD(TestLoadGameRichPresenceOnlyFromFile)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321");

        game.mockStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "Display:\nFrom File\n");
        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::string("Display:\nFrom File\n"),
                         game.mockStorage.GetStoredData(ra::services::StorageItemType::RichPresence, L"1"));
        Assert::IsTrue(game.IsRichPresenceFromFile());

        const auto* pRichPresence = game.Assets().FindRichPresence();
        Assert::IsNotNull(pRichPresence);
        Ensures(pRichPresence != nullptr);
        Assert::AreEqual(ra::data::models::AssetCategory::Local, pRichPresence->GetCategory());
        Assert::AreEqual(ra::data::models::AssetChanges::Unpublished, pRichPresence->GetChanges());
        Assert::IsFalse(pRichPresence->IsActive());
    }

    TEST_METHOD(TestLoadGameAchievements)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "",
             "{\"ID\":5,\"Title\":\"Ach1\",\"Description\":\"Desc1\",\"Flags\":3,"
               "\"Points\":5,\"MemAddr\":\"1=1\",\"Author\":\"Auth1\",\"BadgeName\":\"12345\","
               "\"Created\":1234567890,\"Modified\":123459999},"
             "{\"ID\":7,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":5,"
               "\"Points\":15,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
               "\"Created\":1234567890,\"Modified\":123459999}"
        );

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        game.RemoveNonAchievementAssets();
        Assert::AreEqual({ 2U }, game.Assets().Count());

        const auto* vmAch1 = game.Assets().FindAchievement(5U);
        Assert::IsNotNull(vmAch1);
        Ensures(vmAch1 != nullptr);
        Assert::AreEqual(5U, vmAch1->GetID());
        Assert::AreEqual(std::wstring(L"Ach1"), vmAch1->GetName());
        Assert::AreEqual(std::wstring(L"Desc1"), vmAch1->GetDescription());
        Assert::AreEqual(std::wstring(L"Auth1"), vmAch1->GetAuthor());
        Assert::AreEqual(ra::data::models::AssetCategory::Core, vmAch1->GetCategory());
        Assert::AreEqual(5, vmAch1->GetPoints());
        Assert::AreEqual(std::wstring(L"12345"), vmAch1->GetBadge());
        Assert::AreEqual(std::string("1=1"), vmAch1->GetTrigger());
        Assert::IsFalse(vmAch1->IsModified());

        const auto* vmAch2 = game.Assets().FindAchievement(7U);
        Assert::IsNotNull(vmAch2);
        Ensures(vmAch2 != nullptr);
        Assert::AreEqual(7U, vmAch2->GetID());
        Assert::AreEqual(std::wstring(L"Ach2"), vmAch2->GetName());
        Assert::AreEqual(std::wstring(L"Desc2"), vmAch2->GetDescription());
        Assert::AreEqual(std::wstring(L"Auth2"), vmAch2->GetAuthor());
        Assert::AreEqual(ra::data::models::AssetCategory::Unofficial, vmAch2->GetCategory());
        Assert::AreEqual(15, vmAch2->GetPoints());
        Assert::AreEqual(std::wstring(L"12345"), vmAch2->GetBadge());
        Assert::AreEqual(std::string("1=1"), vmAch2->GetTrigger());
        Assert::IsFalse(vmAch2->IsModified());

        const auto pPatchData = game.mockStorage.GetStoredData(ra::services::StorageItemType::GameData, L"1");
        Assert::AreNotEqual(pPatchData.find("\"ID\":1,"), std::string::npos);
        Assert::AreEqual(pPatchData.find("\"PatchData\":"), std::string::npos); /* PatchData should be stripped out */
    }

    TEST_METHOD(TestLoadGameInvalidAchievementFlags)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "",
             "{\"ID\":5,\"Title\":\"Ach1\",\"Description\":\"Desc1\",\"Flags\":0," // not a valid Flags
               "\"Points\":5,\"MemAddr\":\"1=1\",\"Author\":\"Auth1\",\"BadgeName\":\"12345\","
               "\"Created\":1234567890,\"Modified\":123459999},"
             "{\"ID\":7,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":5,"
               "\"Points\":15,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
               "\"Created\":1234567890,\"Modified\":123459999}"
        );

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        game.RemoveNonAchievementAssets();
        Assert::AreEqual({ 2U }, game.Assets().Count());

        const auto* vmAch1 = game.Assets().FindAchievement(5U);
        Assert::IsNotNull(vmAch1);
        Ensures(vmAch1 != nullptr);
        Assert::AreEqual(5U, vmAch1->GetID());
        Assert::AreEqual(std::wstring(L"Ach1"), vmAch1->GetName());
        Assert::AreEqual(std::wstring(L"Desc1"), vmAch1->GetDescription());
        Assert::AreEqual(std::wstring(L"Auth1"), vmAch1->GetAuthor());
        Assert::AreEqual(ra::data::models::AssetCategory::Unofficial, vmAch1->GetCategory());
        Assert::AreEqual(5, vmAch1->GetPoints());
        Assert::AreEqual(std::wstring(L"12345"), vmAch1->GetBadge());
        Assert::AreEqual(std::string("1=1"), vmAch1->GetTrigger());
        Assert::IsFalse(vmAch1->IsModified());

        const auto* vmAch2 = game.Assets().FindAchievement(7U);
        Assert::IsNotNull(vmAch2);
        Ensures(vmAch2 != nullptr);
        Assert::AreEqual(7U, vmAch2->GetID());
        Assert::AreEqual(std::wstring(L"Ach2"), vmAch2->GetName());
        Assert::AreEqual(std::wstring(L"Desc2"), vmAch2->GetDescription());
        Assert::AreEqual(ra::data::models::AssetCategory::Unofficial, vmAch2->GetCategory());
        Assert::AreEqual(15, vmAch2->GetPoints());
        Assert::AreEqual(std::wstring(L"12345"), vmAch2->GetBadge());
        Assert::AreEqual(std::string("1=1"), vmAch2->GetTrigger());
        Assert::IsFalse(vmAch2->IsModified());
    }

    TEST_METHOD(TestLoadGameMergeLocalAchievements)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "",
             "{\"ID\":5,\"Title\":\"Ach1\",\"Description\":\"Desc1\",\"Flags\":3,"
               "\"Points\":5,\"MemAddr\":\"1=1\",\"Author\":\"Auth1\",\"BadgeName\":\"12345\","
               "\"Created\":1234567890,\"Modified\":123459999},"
             "{\"ID\":7,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":5,"
               "\"Points\":15,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
               "\"Created\":1234567890,\"Modified\":123459999}"
        );

        game.mockStorage.MockStoredData(ra::services::StorageItemType::UserAchievements, L"1",
            "Version\n"
            "Game\n"
            "7:1=2:Ach2b:Desc2b::::Auth2b:25:1234554321:1234555555:::54321\n"
            "0:\"1=1\":\"Ach3\":\"Desc3\"::::Auth3:20:1234511111:1234500000:::555\n"
            "0:R:1=1:Ach4:Desc4::::Auth4:10:1234511111:1234500000:::556\n"
        );

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        auto* pAch = game.Assets().FindAchievement(5U);
        Assert::IsNotNull(pAch);
        Ensures(pAch != nullptr);
        Assert::AreEqual(std::wstring(L"Ach1"), pAch->GetName());
        Assert::AreEqual(std::wstring(L"Desc1"), pAch->GetDescription());
        Assert::AreEqual(std::wstring(L"Auth1"), pAch->GetAuthor());
        Assert::AreEqual(std::wstring(L"12345"), pAch->GetBadge());
        Assert::AreEqual(ra::data::models::AssetCategory::Core, pAch->GetCategory());
        Assert::AreEqual(5, pAch->GetPoints());
        Assert::AreEqual(std::string("1=1"), pAch->GetTrigger());

        // local achievement data for 7 should be merged with server achievement data
        pAch = game.Assets().FindAchievement(7U);
        Assert::IsNotNull(pAch);
        Ensures(pAch != nullptr);
        Assert::AreEqual(std::wstring(L"Ach2b"), pAch->GetName());
        Assert::AreEqual(std::wstring(L"Desc2b"), pAch->GetDescription());
        Assert::AreEqual(std::wstring(L"Auth2"), pAch->GetAuthor()); // author not merged
        Assert::AreEqual(std::wstring(L"54321"), pAch->GetBadge());
        Assert::AreEqual(ra::data::models::AssetCategory::Unofficial, pAch->GetCategory()); // category not merged
        Assert::AreEqual(25, pAch->GetPoints());
        Assert::AreEqual(std::string("1=2"), pAch->GetTrigger());

        // no server achievement, assign FirstLocalId
        pAch = game.Assets().FindAchievement(GameAssets::FirstLocalId);
        Assert::IsNotNull(pAch);
        Ensures(pAch != nullptr);
        Assert::AreEqual(std::wstring(L"Ach3"), pAch->GetName());
        Assert::AreEqual(std::wstring(L"Desc3"), pAch->GetDescription());
        Assert::AreEqual(std::wstring(L"Auth3"), pAch->GetAuthor());
        Assert::AreEqual(std::wstring(L"00555"), pAch->GetBadge());
        Assert::AreEqual(ra::data::models::AssetCategory::Local, pAch->GetCategory());
        Assert::AreEqual(20, pAch->GetPoints());
        Assert::AreEqual(std::string("1=1"), pAch->GetTrigger());

        // no server achievement, assign next local id
        pAch = game.Assets().FindAchievement(GameAssets::FirstLocalId + 1);
        Assert::IsNotNull(pAch);
        Ensures(pAch != nullptr);
        Assert::AreEqual(std::wstring(L"Ach4"), pAch->GetName());
        Assert::AreEqual(std::wstring(L"Desc4"), pAch->GetDescription());
        Assert::AreEqual(std::wstring(L"Auth4"), pAch->GetAuthor());
        Assert::AreEqual(std::wstring(L"00556"), pAch->GetBadge());
        Assert::AreEqual(ra::data::models::AssetCategory::Local, pAch->GetCategory());
        Assert::AreEqual(10, pAch->GetPoints());
        Assert::AreEqual(std::string("R:1=1"), pAch->GetTrigger());
    }

    TEST_METHOD(TestLoadGameMergeLocalAchievementsWithIds)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321");

        game.mockStorage.MockStoredData(ra::services::StorageItemType::UserAchievements, L"1",
            "Version\n"
            "Game\n"
            "7:1=2:Ach2b:Desc2b::::Auth2b:25:1234554321:1234555555:::54321\n"
            "999000001:1=1:Ach3:Desc3::::Auth3:20:1234511111:1234500000:::555\n"
            "999000003:1=1:Ach4:Desc4::::Auth4:10:1234511111:1234500000:::556\n"
        );

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        game.RemoveNonAchievementAssets();
        Assert::AreEqual({ 3U }, game.Assets().Count());

        // 7 is not a known ID for this game, it should be loaded into a local achievement
        auto* vmAch = game.Assets().FindAchievement(7U);
        Assert::IsNotNull(vmAch);
        Ensures(vmAch != nullptr);
        Assert::AreEqual(7U, vmAch->GetID());
        Assert::AreEqual(std::wstring(L"Ach2b"), vmAch->GetName());
        Assert::AreEqual(std::wstring(L"Desc2b"), vmAch->GetDescription());
        Assert::AreEqual(ra::data::models::AssetCategory::Local, vmAch->GetCategory());
        Assert::AreEqual(25, vmAch->GetPoints());
        Assert::AreEqual(std::wstring(L"54321"), vmAch->GetBadge());
        Assert::AreEqual(std::string("1=2"), vmAch->GetTrigger());
        Assert::IsFalse(vmAch->IsModified());
        Assert::AreEqual(ra::data::models::AssetChanges::Unpublished, vmAch->GetChanges());

        // explicit ID should be honored
        vmAch = game.Assets().FindAchievement(999000001U);
        Assert::IsNotNull(vmAch);
        Ensures(vmAch != nullptr);
        Assert::AreEqual(999000001U, vmAch->GetID()); // non-vms get first id and first id + 1
        Assert::AreEqual(std::wstring(L"Ach3"), vmAch->GetName());
        Assert::AreEqual(std::wstring(L"Desc3"), vmAch->GetDescription());
        Assert::AreEqual(ra::data::models::AssetCategory::Local, vmAch->GetCategory());
        Assert::AreEqual(20, vmAch->GetPoints());
        Assert::AreEqual(std::wstring(L"00555"), vmAch->GetBadge());
        Assert::AreEqual(std::string("1=1"), vmAch->GetTrigger());
        Assert::IsFalse(vmAch->IsModified());
        Assert::AreEqual(ra::data::models::AssetChanges::Unpublished, vmAch->GetChanges());

        // explicit ID should be honored
        vmAch = game.Assets().FindAchievement(999000003U);
        Assert::IsNotNull(vmAch);
        Ensures(vmAch != nullptr);
        Assert::AreEqual(999000003U, vmAch->GetID()); // non-vms get first id and first id + 1
        Assert::AreEqual(std::wstring(L"Ach4"), vmAch->GetName());
        Assert::AreEqual(std::wstring(L"Desc4"), vmAch->GetDescription());
        Assert::AreEqual(ra::data::models::AssetCategory::Local, vmAch->GetCategory());
        Assert::AreEqual(10, vmAch->GetPoints());
        Assert::AreEqual(std::wstring(L"00556"), vmAch->GetBadge());
        Assert::AreEqual(std::string("1=1"), vmAch->GetTrigger());
        Assert::IsFalse(vmAch->IsModified());
        Assert::AreEqual(ra::data::models::AssetChanges::Unpublished, vmAch->GetChanges());


        // new achievement should be allocated an ID higher than the largest existing local
        // ID, even if intermediate values are available
        const auto& pAch2 = game.Assets().NewAchievement();
        Assert::AreEqual(999000004U, pAch2.GetID());
    }

    TEST_METHOD(TestLoadGameLeaderboards)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "", "",
            "{\"ID\":7,\"Title\":\"LB1\",\"Description\":\"Desc1\","
             "\"Mem\":\"STA:1=1::CAN:1=1::SUB:1=1::VAL:1\",\"Format\":\"SECS\"},"
            "{\"ID\":8,\"Title\":\"LB2\",\"Description\":\"Desc2\","
             "\"Mem\":\"STA:1=1::CAN:1=1::SUB:1=1::VAL:1\",\"Format\":\"FRAMES\"}"
        );

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        const auto* pLb1 = game.Assets().FindLeaderboard(7U);
        Assert::IsNotNull(pLb1);
        Ensures(pLb1 != nullptr);
        Assert::AreEqual(std::wstring(L"LB1"), pLb1->GetName());
        Assert::AreEqual(std::wstring(L"Desc1"), pLb1->GetDescription());

        const auto* pLb2 = game.Assets().FindLeaderboard(8U);
        Assert::IsNotNull(pLb2);
        Ensures(pLb2 != nullptr);
        Assert::AreEqual(std::wstring(L"LB2"), pLb2->GetName());
        Assert::AreEqual(std::wstring(L"Desc2"), pLb2->GetDescription());
    }

    TEST_METHOD(TestLoadGameReplacesAchievements)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "",
             "{\"ID\":5,\"Title\":\"Ach1\",\"Description\":\"Desc1\",\"Flags\":3,"
               "\"Points\":5,\"MemAddr\":\"1=1\",\"Author\":\"Auth1\",\"BadgeName\":\"12345\","
               "\"Created\":1234567890,\"Modified\":123459999},"
             "{\"ID\":7,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":5,"
               "\"Points\":15,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
               "\"Created\":1234567890,\"Modified\":123459999}"
        );

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        game.MockLoadGameAPIs(2U, "fedcba9876543210123456789abcdef", "",
             "{\"ID\":9,\"Title\":\"Ach9\",\"Description\":\"Desc9\",\"Flags\":3,"
               "\"Points\":9,\"MemAddr\":\"1=1\",\"Author\":\"Auth1\",\"BadgeName\":\"12345\","
               "\"Created\":1234567890,\"Modified\":123459999},"
             "{\"ID\":11,\"Title\":\"Ach11\",\"Description\":\"Desc11\",\"Flags\":5,"
               "\"Points\":11,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
               "\"Created\":1234567890,\"Modified\":123459999}"
        );

        game.LoadGame(2U, "fedcba9876543210123456789abcdef");

        Assert::IsNull(game.Assets().FindAchievement(5U));
        Assert::IsNull(game.Assets().FindAchievement(7U));

        game.RemoveNonAchievementAssets();
        Assert::AreEqual({ 2U }, game.Assets().Count());
        Assert::IsFalse(game.Assets().IsUpdating());

        const auto* vmAch1 = game.Assets().FindAchievement(9U);
        Assert::IsNotNull(vmAch1);
        Ensures(vmAch1 != nullptr);
        Assert::AreEqual(9U, vmAch1->GetID());
        Assert::AreEqual(std::wstring(L"Ach9"), vmAch1->GetName());
        Assert::AreEqual(std::wstring(L"Desc9"), vmAch1->GetDescription());
        Assert::AreEqual(ra::data::models::AssetCategory::Core, vmAch1->GetCategory());
        Assert::AreEqual(9, vmAch1->GetPoints());
        Assert::AreEqual(std::wstring(L"12345"), vmAch1->GetBadge());
        Assert::AreEqual(std::string("1=1"), vmAch1->GetTrigger());
        Assert::IsFalse(vmAch1->IsModified());

        const auto* vmAch2 = game.Assets().FindAchievement(11U);
        Assert::IsNotNull(vmAch2);
        Ensures(vmAch2 != nullptr);
        Assert::AreEqual(11U, vmAch2->GetID());
        Assert::AreEqual(std::wstring(L"Ach11"), vmAch2->GetName());
        Assert::AreEqual(std::wstring(L"Desc11"), vmAch2->GetDescription());
        Assert::AreEqual(ra::data::models::AssetCategory::Unofficial, vmAch2->GetCategory());
        Assert::AreEqual(11, vmAch2->GetPoints());
        Assert::AreEqual(std::wstring(L"12345"), vmAch2->GetBadge());
        Assert::AreEqual(std::string("1=1"), vmAch2->GetTrigger());
        Assert::IsFalse(vmAch2->IsModified());
    }

    TEST_METHOD(TestLoadGameZeroRemovesAchievements)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "",
             "{\"ID\":5,\"Title\":\"Ach1\",\"Description\":\"Desc1\",\"Flags\":3,"
               "\"Points\":5,\"MemAddr\":\"1=1\",\"Author\":\"Auth1\",\"BadgeName\":\"12345\","
               "\"Created\":1234567890,\"Modified\":123459999},"
             "{\"ID\":7,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":5,"
               "\"Points\":15,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
               "\"Created\":1234567890,\"Modified\":123459999}"
        );

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        game.RemoveNonAchievementAssets();
        Assert::AreEqual({ 2U }, game.Assets().Count());

        game.LoadGame(0U, "");

        Assert::IsNull(game.Assets().FindAchievement(5U));
        Assert::IsNull(game.Assets().FindAchievement(7U));

        Assert::AreEqual({ 0U }, game.Assets().Count());
        Assert::IsFalse(game.Assets().IsUpdating());
    }

    TEST_METHOD(TestLoadGameUserUnlocks)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321",
            "\"HardcoreUnlocks\":[{\"ID\":7,\"When\":1234567890}]",
            "{\"ID\":5,\"Title\":\"Ach1\",\"Description\":\"Desc1\",\"Flags\":3,"
              "\"Points\":5,\"MemAddr\":\"1=1\",\"Author\":\"Auth1\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999},"
            "{\"ID\":7,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":3,"
              "\"Points\":10,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999}"
        );

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        const auto* pAch1 = game.Assets().FindAchievement(5U);
        Assert::IsNotNull(pAch1);
        Ensures(pAch1 != nullptr);
        Assert::IsTrue(pAch1->IsActive());

        const auto* pAch2 = game.Assets().FindAchievement(7U);
        Assert::IsNotNull(pAch2);
        Ensures(pAch2 != nullptr);
        Assert::IsFalse(pAch2->IsActive());

        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Loaded GameTitle"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"2 achievements, 15 points"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"You have earned 1 achievements"), pPopup->GetDetail());
        Assert::AreEqual(std::string("9743"), pPopup->GetImage().Name());
    }

    TEST_METHOD(TestLoadGameUserUnlocksUnofficial)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321",
            "\"HardcoreUnlocks\":[{\"ID\":7,\"When\":1234567890},{\"ID\":9,\"When\":1234567890}]",
            "{\"ID\":5,\"Title\":\"Ach1\",\"Description\":\"Desc1\",\"Flags\":3,"
              "\"Points\":5,\"MemAddr\":\"1=1\",\"Author\":\"Auth1\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999},"
            "{\"ID\":7,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":3,"
              "\"Points\":10,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999},"
            "{\"ID\":9,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":5,"
              "\"Points\":25,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999},"
            "{\"ID\":11,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":5,"
              "\"Points\":50,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999}"
        );

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        const auto* pAch1 = game.Assets().FindAchievement(5U);
        Assert::IsNotNull(pAch1);
        Ensures(pAch1 != nullptr);
        Assert::IsTrue(pAch1->IsActive());

        const auto* pAch2 = game.Assets().FindAchievement(7U);
        Assert::IsNotNull(pAch2);
        Ensures(pAch2 != nullptr);
        Assert::IsFalse(pAch2->IsActive());

        const auto* pAch3 = game.Assets().FindAchievement(9U);
        Assert::IsNotNull(pAch3);
        Ensures(pAch3 != nullptr);
        Assert::IsFalse(pAch3->IsActive());

        // unofficial achievement should not be activated even if it wasn't unlocked
        // (expect unofficial achievements to not be unlocked)
        const auto* pAch4 = game.Assets().FindAchievement(11U);
        Assert::IsNotNull(pAch4);
        Ensures(pAch4 != nullptr);
        Assert::IsFalse(pAch4->IsActive());

        // only core achievements should be tallied for the popup
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Loaded GameTitle"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"2 achievements, 15 points"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"You have earned 1 achievements"), pPopup->GetDetail());
        Assert::AreEqual(std::string("9743"), pPopup->GetImage().Name());
    }

    TEST_METHOD(TestLoadGameUserUnlocksCompatibilityMode)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321",
            "\"HardcoreUnlocks\":[{\"ID\":7,\"When\":1234567890}]",
            "{\"ID\":5,\"Title\":\"Ach1\",\"Description\":\"Desc1\",\"Flags\":3,"
              "\"Points\":5,\"MemAddr\":\"1=1\",\"Author\":\"Auth1\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999},"
            "{\"ID\":7,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":3,"
              "\"Points\":10,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999}"
        );

        game.LoadGame(1U, "0123456789abcdeffedcba987654321", ra::data::context::GameContext::Mode::CompatibilityTest);

        const auto* pAch1 = game.Assets().FindAchievement(5U);
        Assert::IsNotNull(pAch1);
        Ensures(pAch1 != nullptr);
        Assert::IsTrue(pAch1->IsActive());

        const auto* pAch2 = game.Assets().FindAchievement(7U);
        Assert::IsNotNull(pAch2);
        Ensures(pAch2 != nullptr);
        Assert::IsTrue(pAch2->IsActive());
    }

    TEST_METHOD(TestLoadGameUserUnlocksLocalModifications)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "",
            "{\"ID\":5,\"Title\":\"Ach1\",\"Description\":\"Desc1\",\"Flags\":3,"
              "\"Points\":5,\"MemAddr\":\"1=1\",\"Author\":\"Auth1\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999},"
            "{\"ID\":7,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":3,"
              "\"Points\":10,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999}"
        );

        game.mockStorage.MockStoredData(ra::services::StorageItemType::UserAchievements, L"1",
            "Version\n"
            "Game\n"
            "7:1=2:Ach2b:Desc2b::::Auth2b:25:1234554321:1234555555:::54321\n"
        );

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        const auto* pAch1 = game.Assets().FindAchievement(5U);
        Assert::IsNotNull(pAch1);
        Ensures(pAch1 != nullptr);
        Assert::AreEqual(ra::data::models::AssetChanges::None, pAch1->GetChanges());
        Assert::IsTrue(pAch1->IsActive());

        const auto* pAch2 = game.Assets().FindAchievement(7U);
        Assert::IsNotNull(pAch2);
        Ensures(pAch2 != nullptr);
        Assert::AreEqual(ra::data::models::AssetChanges::Unpublished, pAch2->GetChanges());
        Assert::IsFalse(pAch2->IsActive()); // should not be active because it's modified

        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Loaded GameTitle"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"2 achievements, 15 points"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"You have earned 0 achievements"), pPopup->GetDetail());
        Assert::AreEqual(std::string("9743"), pPopup->GetImage().Name());
    }

    TEST_METHOD(TestLoadGamePausesRuntime)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321");

        bool bBeforeResponseCalled = false;
        game.mockAchievementRuntime.OnBeforeResponse("r=patch&u=Username&t=ApiToken&g=1",
            [&game, &bBeforeResponseCalled]() {
                bBeforeResponseCalled = true;
                Assert::IsTrue(game.mockAchievementRuntime.IsPaused());
            });

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        Assert::IsTrue(bBeforeResponseCalled); // paused during load
        Assert::IsFalse(game.mockAchievementRuntime.IsPaused()); // not paused after load
    }

    TEST_METHOD(TestLoadGameWhileRuntimePaused)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321");

        bool bBeforeResponseCalled = false;
        game.mockAchievementRuntime.OnBeforeResponse("r=patch&u=Username&t=ApiToken&g=1",
            [&game, &bBeforeResponseCalled]() {
                bBeforeResponseCalled = true;
                Assert::IsTrue(game.mockAchievementRuntime.IsPaused());
            });

        game.mockAchievementRuntime.SetPaused(true);

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        Assert::IsTrue(bBeforeResponseCalled); // paused during load
        Assert::IsTrue(game.mockAchievementRuntime.IsPaused()); // paused after load
    }

    TEST_METHOD(TestReloadRichPresenceScriptNoFile)
    {
        GameContextHarness game;
        const std::string sRichPresence = "Display:\nHello, World";
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "", "", "", sRichPresence);

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        /* load game will write the server RP to storage */
        game.ReloadRichPresenceScript();

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::wstring(L"Hello, World"), game.GetRichPresenceDisplayString());
        Assert::IsFalse(game.IsRichPresenceFromFile());

        /* replace written server RP with empty file */
        game.mockStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "");
        game.ReloadRichPresenceScript();

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), game.GetRichPresenceDisplayString());
        Assert::IsTrue(game.IsRichPresenceFromFile());
    }

    TEST_METHOD(TestReloadRichPresenceScriptWindowsLineEndings)
    {
        GameContextHarness game;
        const std::string sRichPresence = "Display:\nHello, World";
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "", "", "", sRichPresence);

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        /* load game will write the server RP to storage */
        game.ReloadRichPresenceScript();

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::wstring(L"Hello, World"), game.GetRichPresenceDisplayString());
        Assert::IsFalse(game.IsRichPresenceFromFile());

        /* replace written server RP with a different file */
        game.mockStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "Display:\r\nFrom File\r\n");
        game.ReloadRichPresenceScript();

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::wstring(L"From File"), game.GetRichPresenceDisplayString());
        Assert::IsTrue(game.IsRichPresenceFromFile());
    }

    TEST_METHOD(TestReloadRichPresenceScript)
    {
        GameContextHarness game;
        const std::string sRichPresence = "Display:\nHello, World";
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "", "", "", sRichPresence);

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        /* load game will write the server RP to storage, so overwrite it now */
        game.mockStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", "Display:\nFrom File");
        game.ReloadRichPresenceScript();

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::wstring(L"From File"), game.GetRichPresenceDisplayString());
        Assert::IsTrue(game.IsRichPresenceFromFile());
    }

    TEST_METHOD(TestReloadRichPresenceScriptBOM)
    {
        GameContextHarness game;
        const std::string sRichPresence = "Display:\nHello, World";
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "", "", "", sRichPresence);
        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        /* load game will write the server RP to storage, so overwrite it now */
        std::string sFileContents = "\xEF\xBB\xBF" "Display:\nFrom File";
        sFileContents.at(0) = gsl::narrow_cast<char>(0xef);
        sFileContents.at(1) = gsl::narrow_cast<char>(0xbb);
        sFileContents.at(2) = gsl::narrow_cast<char>(0xbf);

        game.mockStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", sFileContents);
        game.ReloadRichPresenceScript();

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::wstring(L"From File"), game.GetRichPresenceDisplayString());
        Assert::IsTrue(game.IsRichPresenceFromFile());

        /* hash should ignore BOM and convert windows newlines to unix newlines */
        sFileContents = "\xEF\xBB\xBF" "Display:\r\nHello, World";
        sFileContents.at(0) = gsl::narrow_cast<char>(0xef);
        sFileContents.at(1) = gsl::narrow_cast<char>(0xbb);
        sFileContents.at(2) = gsl::narrow_cast<char>(0xbf);
        game.mockStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", sFileContents);
        game.ReloadRichPresenceScript();

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::wstring(L"Hello, World"), game.GetRichPresenceDisplayString());
        Assert::IsFalse(game.IsRichPresenceFromFile());
    }

    TEST_METHOD(TestSetModeNotify)
    {
        class NotifyHarness : public GameContext::NotifyTarget
        {
        public:
            bool m_bNotified = false;

        protected:
            void OnActiveGameChanged() noexcept override { m_bNotified = true; }
        };
        NotifyHarness notifyHarness;

        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321");

        Assert::AreEqual(GameContext::Mode::Normal, game.GetMode());

        game.AddNotifyTarget(notifyHarness);
        game.LoadGame(0U, "");

        Assert::AreEqual(0U, game.GameId());
        Assert::IsFalse(notifyHarness.m_bNotified);

        game.SetMode(GameContext::Mode::CompatibilityTest);
        Assert::AreEqual(GameContext::Mode::CompatibilityTest, game.GetMode());
        Assert::IsFalse(notifyHarness.m_bNotified);

        game.LoadGame(1U, "0123456789abcdeffedcba987654321", GameContext::Mode::CompatibilityTest);
        Assert::AreEqual(1U, game.GameId());
        Assert::AreEqual(GameContext::Mode::CompatibilityTest, game.GetMode());
        Assert::IsTrue(notifyHarness.m_bNotified);

        notifyHarness.m_bNotified = false;
        game.SetMode(GameContext::Mode::Normal);
        Assert::AreEqual(GameContext::Mode::Normal, game.GetMode());
        Assert::IsTrue(notifyHarness.m_bNotified);

        // not changing mode should not notify again
        notifyHarness.m_bNotified = false;
        game.SetMode(GameContext::Mode::Normal);
        Assert::AreEqual(GameContext::Mode::Normal, game.GetMode());
        Assert::IsFalse(notifyHarness.m_bNotified);
    }


    TEST_METHOD(TestLoadGameNonHardcoreWarningHardcore)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "",
            "{\"ID\":5,\"Title\":\"Ach1\",\"Description\":\"Desc1\",\"Flags\":3,"
              "\"Points\":5,\"MemAddr\":\"1=1\",\"Author\":\"Auth1\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999},"
            "{\"ID\":7,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":3,"
              "\"Points\":10,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999}"
        );

        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::NonHardcoreWarning, true);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        // no dialogs when game with achievements loaded in hardcore mode
        game.mockFrameEventQueue.DoFrame(); // hardcore warning gets queued for the UI thread
        Assert::IsFalse(game.mockDesktop.WasDialogShown());
        Assert::IsNull(game.mockOverlayManager.GetMessage(2U)); // message 1 will be game placard
    }

    TEST_METHOD(TestLoadGameNonHardcoreWarningLeaderboards)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "",
            "{\"ID\":5,\"Title\":\"Ach1\",\"Description\":\"Desc1\",\"Flags\":3,"
              "\"Points\":5,\"MemAddr\":\"1=1\",\"Author\":\"Auth1\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999},"
            "{\"ID\":7,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":3,"
              "\"Points\":10,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999}"
        );

        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::NonHardcoreWarning, false);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Leaderboards, true);
        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        // should get popup message indicating leaderboards not supported in softcore
        game.mockFrameEventQueue.DoFrame(); // hardcore warning gets queued for the UI thread
        Assert::IsFalse(game.mockDesktop.WasDialogShown());

        const auto* pPopup = game.mockOverlayManager.GetMessage(2U);
        Expects(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Playing in Softcore Mode"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Leaderboard entries will not be submitted."), pPopup->GetDescription());
    }

    TEST_METHOD(TestLoadGameNonHardcoreWarningLeaderboardsOff)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "",
            "{\"ID\":5,\"Title\":\"Ach1\",\"Description\":\"Desc1\",\"Flags\":3,"
              "\"Points\":5,\"MemAddr\":\"1=1\",\"Author\":\"Auth1\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999},"
            "{\"ID\":7,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":3,"
              "\"Points\":10,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999}"
        );

        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::NonHardcoreWarning, false);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Leaderboards, false);
        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        // should get popup message indicating playing in softcore
        game.mockFrameEventQueue.DoFrame(); // hardcore warning gets queued for the UI thread
        Assert::IsFalse(game.mockDesktop.WasDialogShown());

        const auto* pPopup = game.mockOverlayManager.GetMessage(2U);
        Expects(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Playing in Softcore Mode"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L""), pPopup->GetDescription());
    }

    TEST_METHOD(TestLoadGameNonHardcoreWarningCancel)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "",
            "{\"ID\":5,\"Title\":\"Ach1\",\"Description\":\"Desc1\",\"Flags\":3,"
              "\"Points\":5,\"MemAddr\":\"1=1\",\"Author\":\"Auth1\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999},"
            "{\"ID\":7,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":3,"
              "\"Points\":10,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999}"
        );

        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::NonHardcoreWarning, true);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        game.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox) {
                Assert::AreEqual(std::wstring(L"Enable Hardcore mode?"), vmMessageBox.GetHeader());
                Assert::AreEqual(std::wstring(L"You are loading a game with achievements and do not currently have hardcore mode enabled."), vmMessageBox.GetMessage());
                Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning, vmMessageBox.GetIcon());
                Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
                return ra::ui::DialogResult::No;
            });

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        // should get confirmation dialog about hardcore and no popups
        game.mockFrameEventQueue.DoFrame(); // hardcore warning gets queued for the UI thread
        Assert::IsTrue(game.mockDesktop.WasDialogShown());
        Assert::IsNull(game.mockOverlayManager.GetMessage(2U));

        // hardcore should remain disabled
        Assert::IsFalse(game.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestLoadGameNonHardcoreWarningAccept)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "",
            "{\"ID\":5,\"Title\":\"Ach1\",\"Description\":\"Desc1\",\"Flags\":3,"
              "\"Points\":5,\"MemAddr\":\"1=1\",\"Author\":\"Auth1\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999},"
            "{\"ID\":7,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":3,"
              "\"Points\":10,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
              "\"Created\":1234567890,\"Modified\":123459999}"
        );

        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::NonHardcoreWarning, true);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        game.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox) {
                Assert::AreEqual(std::wstring(L"Enable Hardcore mode?"), vmMessageBox.GetHeader());
                Assert::AreEqual(std::wstring(L"You are loading a game with achievements and do not currently have hardcore mode enabled."), vmMessageBox.GetMessage());
                Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning, vmMessageBox.GetIcon());
                Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
                return ra::ui::DialogResult::Yes;
            });

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        // should get confirmation dialog about hardcore and no popups
        game.mockFrameEventQueue.DoFrame(); // hardcore warning gets queued for the UI thread
        Assert::IsTrue(game.mockDesktop.WasDialogShown());
        Assert::IsNull(game.mockOverlayManager.GetMessage(2U));

        // hardcore should remain disabled
        Assert::IsTrue(game.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }

    TEST_METHOD(TestLoadGameNonHardcoreWarningNoCoreAchievements)
    {
        GameContextHarness game;
        game.MockLoadGameAPIs(1U, "0123456789abcdeffedcba987654321", "",
                              "{\"ID\":5,\"Title\":\"Ach1\",\"Description\":\"Desc1\",\"Flags\":5,"
                              "\"Points\":5,\"MemAddr\":\"1=1\",\"Author\":\"Auth1\",\"BadgeName\":\"12345\","
                              "\"Created\":1234567890,\"Modified\":123459999},"
                              "{\"ID\":7,\"Title\":\"Ach2\",\"Description\":\"Desc2\",\"Flags\":5,"
                              "\"Points\":10,\"MemAddr\":\"1=1\",\"Author\":\"Auth2\",\"BadgeName\":\"12345\","
                              "\"Created\":1234567890,\"Modified\":123459999}");

        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::NonHardcoreWarning, true);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);

        game.LoadGame(1U, "0123456789abcdeffedcba987654321");

        // should only get softcore popup
        game.mockFrameEventQueue.DoFrame(); // hardcore warning gets queued for the UI thread
        Assert::IsFalse(game.mockDesktop.WasDialogShown());

        const auto* pPopup = game.mockOverlayManager.GetMessage(2U);
        Expects(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Playing in Softcore Mode"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L""), pPopup->GetDescription());

        // hardcore should remain disabled
        Assert::IsFalse(game.mockConfiguration.IsFeatureEnabled(ra::services::Feature::Hardcore));
    }
};

} // namespace tests
} // namespace context
} // namespace data
} // namespace ra
