#include "CppUnitTest.h"

#include "data\context\GameContext.hh"

#include "data\models\AchievementModel.hh"

#include "services\AchievementRuntime.hh"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"

#include "tests\mocks\MockAudioSystem.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
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
            m_OverrideGameContext(this),
            m_OverrideRuntime(&runtime)
        {
        }

        ra::api::mocks::MockServer mockServer;
        ra::services::mocks::MockClock mockClock;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockLocalStorage mockStorage;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::services::mocks::MockAudioSystem mockAudioSystem;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockEmulatorContext mockEmulator;
        ra::data::context::mocks::MockSessionTracker mockSessionTracker;
        ra::data::context::mocks::MockUserContext mockUser;
        ra::services::AchievementRuntime runtime;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        void SetGameId(unsigned int nGameId) noexcept { m_nGameId = nGameId; }

        void SetRichPresenceFromFile(bool bValue) noexcept { m_bRichPresenceFromFile = bValue; }

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

    private:
        ra::services::ServiceLocator::ServiceOverride<ra::data::context::GameContext> m_OverrideGameContext;
        ra::services::ServiceLocator::ServiceOverride<ra::services::AchievementRuntime> m_OverrideRuntime;
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
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request& request, ra::api::FetchGameData::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Title = L"Game";
            return true;
        });

        game.LoadGame(1U);

        Assert::AreEqual(1U, game.GameId());
        Assert::AreEqual(ra::data::context::GameContext::Mode::Normal, game.GetMode());
        Assert::AreEqual(std::wstring(L"Game"), game.GameTitle());
    }

    TEST_METHOD(TestLoadGamePopup)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request& request, ra::api::FetchGameData::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            auto& ach1 = response.Achievements.emplace_back();
            ach1.Id = 5;
            ach1.Title = "Ach1";
            ach1.Description = "Desc1";
            ach1.Author = "Auth1";
            ach1.BadgeName = "12345";
            ach1.CategoryId = 3;
            ach1.Created = 1234567890;
            ach1.Updated = 1234599999;
            ach1.Definition = "1=1";
            ach1.Points = 5;

            response.Title = L"GameTitle";
            response.ImageIcon = "9743";
            return true;
        });

        game.LoadGame(1U);

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
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request& request, ra::api::FetchGameData::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Title = L"Game";
            response.ConsoleId = ra::etoi(ConsoleID::MegaDrive);
            return true;
        });

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

        game.LoadGame(1U);

        Assert::AreEqual(0U, game.GameId());
        Assert::AreEqual(ra::data::context::GameContext::Mode::Normal, game.GetMode());
        Assert::AreEqual(std::wstring(L""), game.GameTitle());
    }

    TEST_METHOD(TestLoadGameTitleConsoleMismatchGBvGBC)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request& request, ra::api::FetchGameData::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Title = L"Game";
            response.ConsoleId = ra::etoi(ConsoleID::GB);
            response.ImageIcon = "9743";
            return true;
        });

        game.mockConsoleContext.SetId(ConsoleID::GBC);
        game.mockConsoleContext.SetName(L"GameBoy Color");

        bool bDialogShown = false;
        game.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            bDialogShown = true;
            return ra::ui::DialogResult::OK;
        });

        game.LoadGame(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(std::wstring(L"Overlay\\info.wav")));

        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Loaded Game"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"0 achievements, 0 points"), pPopup->GetDescription());
        Assert::AreEqual(std::string("9743"), pPopup->GetImage().Name());

        Assert::IsFalse(bDialogShown);
    }

    TEST_METHOD(TestLoadGameTitleConsoleMismatchNESvEvent)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>(
            [](const ra::api::FetchGameData::Request& request, ra::api::FetchGameData::Response& response) {
                Assert::AreEqual(1U, request.GameId);

                response.Title = L"Event";
                response.ConsoleId = RC_CONSOLE_EVENTS;
                response.ImageIcon = "9743";
                return true;
            });

        game.mockConsoleContext.SetId(ConsoleID::NES);
        game.mockConsoleContext.SetName(L"Nintendo");

        bool bDialogShown = false;
        game.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel&) {
                bDialogShown = true;
                return ra::ui::DialogResult::OK;
            });

        game.LoadGame(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(std::wstring(L"Overlay\\info.wav")));

        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Loaded Event"), pPopup->GetTitle());
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
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            response.Title = L"GameTitle";
            response.ImageIcon = "9743";
            return true;
        });

        game.AddNotifyTarget(notifyHarness);
        game.LoadGame(0U);

        Assert::AreEqual(0U, game.GameId());
        Assert::IsFalse(notifyHarness.m_bNotified);

        game.LoadGame(1U);
        Assert::AreEqual(1U, game.GameId());
        Assert::IsTrue(notifyHarness.m_bNotified);

        notifyHarness.m_bNotified = false;
        game.LoadGame(2U);
        Assert::AreEqual(2U, game.GameId());
        Assert::IsTrue(notifyHarness.m_bNotified);

        notifyHarness.m_bNotified = false;
        game.LoadGame(0U);
        Assert::AreEqual(0U, game.GameId());
        Assert::IsTrue(notifyHarness.m_bNotified);

        notifyHarness.m_bNotified = false;
        game.RemoveNotifyTarget(notifyHarness);
        game.LoadGame(2U);
        Assert::AreEqual(2U, game.GameId());
        Assert::IsFalse(notifyHarness.m_bNotified);
    }

    TEST_METHOD(TestLoadGameRichPresence)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            response.RichPresence = "Display:\nHello, World";
            return true;
        });

        game.LoadGame(1U);

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::string("Display:\nHello, World"),
            game.mockStorage.GetStoredData(ra::services::StorageItemType::RichPresence, L"1"));
        Assert::IsFalse(game.IsRichPresenceFromFile());
    }

    TEST_METHOD(TestLoadGameResetsRichPresenceFromFile)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            response.RichPresence = "Display:\nHello, World";
            return true;
        });

        game.SetRichPresenceFromFile(true);
        game.LoadGame(1U);

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::string("Display:\nHello, World"),
            game.mockStorage.GetStoredData(ra::services::StorageItemType::RichPresence, L"1"));
        Assert::IsFalse(game.IsRichPresenceFromFile());
    }

    TEST_METHOD(TestLoadGameAchievements)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            auto& ach1 = response.Achievements.emplace_back();
            ach1.Id = 5;
            ach1.Title = "Ach1";
            ach1.Description = "Desc1";
            ach1.Author = "Auth1";
            ach1.BadgeName = "12345";
            ach1.CategoryId = 3;
            ach1.Created = 1234567890;
            ach1.Updated = 1234599999;
            ach1.Definition = "1=1";
            ach1.Points = 5;

            auto& ach2 = response.Achievements.emplace_back();
            ach2.Id = 7;
            ach2.Title = "Ach2";
            ach2.Description = "Desc2";
            ach2.Author = "Auth2";
            ach2.BadgeName = "12345";
            ach2.CategoryId = 5;
            ach2.Created = 1234567890;
            ach2.Updated = 1234599999;
            ach2.Definition = "1=1";
            ach2.Points = 15;
            return true;
        });

        game.LoadGame(1U);

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
    }

    TEST_METHOD(TestLoadGameInvalidAchievementFlags)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            auto& ach1 = response.Achievements.emplace_back();
            ach1.Id = 5;
            ach1.Title = "Ach1";
            ach1.Description = "Desc1";
            ach1.Author = "Auth1";
            ach1.BadgeName = "12345";
            ach1.CategoryId = 0; // not a valid category
            ach1.Created = 1234567890;
            ach1.Updated = 1234599999;
            ach1.Definition = "1=1";
            ach1.Points = 5;

            auto& ach2 = response.Achievements.emplace_back();
            ach2.Id = 7;
            ach2.Title = "Ach2";
            ach2.Description = "Desc2";
            ach2.Author = "Auth2";
            ach2.BadgeName = "12345";
            ach2.CategoryId = 5;
            ach2.Created = 1234567890;
            ach2.Updated = 1234599999;
            ach2.Definition = "1=1";
            ach2.Points = 15;
            return true;
        });

        game.LoadGame(1U);

        game.RemoveNonAchievementAssets();
        Assert::AreEqual({ 1U }, game.Assets().Count());

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
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            auto& ach1 = response.Achievements.emplace_back();
            ach1.Id = 5;
            ach1.Title = "Ach1";
            ach1.Description = "Desc1";
            ach1.Author = "Auth1";
            ach1.BadgeName = "12345";
            ach1.CategoryId = 3;
            ach1.Created = 1234567890;
            ach1.Updated = 1234599999;
            ach1.Definition = "1=1";
            ach1.Points = 5;

            auto& ach2 = response.Achievements.emplace_back();
            ach2.Id = 7;
            ach2.Title = "Ach2";
            ach2.Description = "Desc2";
            ach2.Author = "Auth2";
            ach2.BadgeName = "12345";
            ach2.CategoryId = 5;
            ach2.Created = 1234567890;
            ach2.Updated = 1234599999;
            ach2.Definition = "1=1";
            ach2.Points = 15;
            return true;
        });

        game.mockStorage.MockStoredData(ra::services::StorageItemType::UserAchievements, L"1",
            "Version\n"
            "Game\n"
            "7:1=2:Ach2b:Desc2b::::Auth2b:25:1234554321:1234555555:::54321\n"
            "0:\"1=1\":\"Ach3\":\"Desc3\"::::Auth3:20:1234511111:1234500000:::555\n"
            "0:R:1=1:Ach4:Desc4::::Auth4:10:1234511111:1234500000:::556\n"
        );

        game.LoadGame(1U);

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
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response&)
        {
            return true;
        });

        game.mockStorage.MockStoredData(ra::services::StorageItemType::UserAchievements, L"1",
            "Version\n"
            "Game\n"
            "7:1=2:Ach2b:Desc2b::::Auth2b:25:1234554321:1234555555:::54321\n"
            "999000001:1=1:Ach3:Desc3::::Auth3:20:1234511111:1234500000:::555\n"
            "999000003:1=1:Ach4:Desc4::::Auth4:10:1234511111:1234500000:::556\n"
        );

        game.LoadGame(1U);

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
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            auto& lb1 = response.Leaderboards.emplace_back();
            lb1.Id = 7U;
            lb1.Title = "LB1";
            lb1.Description = "Desc1";
            lb1.Definition = "STA:1=1::CAN:1=1::SUB:1=1::VAL:1";
            lb1.Format = RC_FORMAT_SECONDS;

            auto& lb2 = response.Leaderboards.emplace_back();
            lb2.Id = 8U;
            lb2.Title = "LB2";
            lb2.Description = "Desc2";
            lb2.Definition = "STA:1=1::CAN:1=1::SUB:1=1::VAL:1";
            lb2.Format = RC_FORMAT_FRAMES;

            return true;
        });

        game.LoadGame(1U);

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
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            auto& ach1 = response.Achievements.emplace_back();
            ach1.Id = 5;
            ach1.Title = "Ach1";
            ach1.Description = "Desc1";
            ach1.Author = "Auth1";
            ach1.BadgeName = "12345";
            ach1.CategoryId = 3;
            ach1.Created = 1234567890;
            ach1.Updated = 1234599999;
            ach1.Definition = "1=1";
            ach1.Points = 5;

            auto& ach2 = response.Achievements.emplace_back();
            ach2.Id = 7;
            ach2.Title = "Ach2";
            ach2.Description = "Desc2";
            ach2.Author = "Auth2";
            ach2.BadgeName = "12345";
            ach2.CategoryId = 5;
            ach2.Created = 1234567890;
            ach2.Updated = 1234599999;
            ach2.Definition = "1=1";
            ach2.Points = 15;
            return true;
        });

        game.LoadGame(1U);

        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            auto& ach1 = response.Achievements.emplace_back();
            ach1.Id = 9;
            ach1.Title = "Ach9";
            ach1.Description = "Desc9";
            ach1.Author = "Auth9";
            ach1.BadgeName = "12345";
            ach1.CategoryId = 3;
            ach1.Created = 1234567890;
            ach1.Updated = 1234599999;
            ach1.Definition = "1=1";
            ach1.Points = 9;

            auto& ach2 = response.Achievements.emplace_back();
            ach2.Id = 11;
            ach2.Title = "Ach11";
            ach2.Description = "Desc11";
            ach2.Author = "Auth11";
            ach2.BadgeName = "12345";
            ach2.CategoryId = 5;
            ach2.Created = 1234567890;
            ach2.Updated = 1234599999;
            ach2.Definition = "1=1";
            ach2.Points = 11;
            return true;
        });

        game.LoadGame(2U);

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
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            auto& ach1 = response.Achievements.emplace_back();
            ach1.Id = 5;
            ach1.Title = "Ach1";
            ach1.Description = "Desc1";
            ach1.Author = "Auth1";
            ach1.BadgeName = "12345";
            ach1.CategoryId = 3;
            ach1.Created = 1234567890;
            ach1.Updated = 1234599999;
            ach1.Definition = "1=1";
            ach1.Points = 5;

            auto& ach2 = response.Achievements.emplace_back();
            ach2.Id = 7;
            ach2.Title = "Ach2";
            ach2.Description = "Desc2";
            ach2.Author = "Auth2";
            ach2.BadgeName = "12345";
            ach2.CategoryId = 5;
            ach2.Created = 1234567890;
            ach2.Updated = 1234599999;
            ach2.Definition = "1=1";
            ach2.Points = 15;
            return true;
        });

        game.LoadGame(1U);

        game.RemoveNonAchievementAssets();
        Assert::AreEqual({ 2U }, game.Assets().Count());

        game.LoadGame(0U);

        Assert::IsNull(game.Assets().FindAchievement(5U));
        Assert::IsNull(game.Assets().FindAchievement(7U));

        Assert::AreEqual({ 0U }, game.Assets().Count());
        Assert::IsFalse(game.Assets().IsUpdating());
    }

    TEST_METHOD(TestLoadGameUserUnlocks)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            response.Title = L"My Game";
            response.ImageIcon = "3333";

            auto& ach1 = response.Achievements.emplace_back();
            ach1.Id = 5;
            ach1.Points = 5;
            ach1.CategoryId = ra::etoi(ra::data::models::AssetCategory::Core);

            auto& ach2 = response.Achievements.emplace_back();
            ach2.Id = 7;
            ach2.Points = 10;
            ach2.CategoryId = ra::etoi(ra::data::models::AssetCategory::Core);
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchUserUnlocks>([](const ra::api::FetchUserUnlocks::Request& request, ra::api::FetchUserUnlocks::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);
            Assert::IsFalse(request.Hardcore);

            response.UnlockedAchievements.insert(7U);
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request&, ra::api::FetchCodeNotes::Response&)
        {
            return true;
        });

        game.LoadGame(1U);
        game.mockThreadPool.ExecuteNextTask(); // FetchUserUnlocks and FetchCodeNotes are async
        game.mockThreadPool.ExecuteNextTask();

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
        Assert::AreEqual(std::wstring(L"Loaded My Game"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"2 achievements, 15 points"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"You have earned 1 achievements"), pPopup->GetDetail());
        Assert::AreEqual(std::string("3333"), pPopup->GetImage().Name());
    }

    TEST_METHOD(TestLoadGameUserUnlocksUnofficial)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            response.Title = L"My Game";
            response.ImageIcon = "3333";

            auto& ach1 = response.Achievements.emplace_back();
            ach1.Id = 5;
            ach1.Points = 5;
            ach1.CategoryId = ra::etoi(ra::data::models::AssetCategory::Core);

            auto& ach2 = response.Achievements.emplace_back();
            ach2.Id = 7;
            ach2.Points = 10;
            ach2.CategoryId = ra::etoi(ra::data::models::AssetCategory::Core);

            auto& ach3 = response.Achievements.emplace_back();
            ach3.Id = 9;
            ach3.Points = 25;
            ach3.CategoryId = ra::etoi(ra::data::models::AssetCategory::Unofficial);

            auto& ach4 = response.Achievements.emplace_back();
            ach4.Id = 11;
            ach4.Points = 50;
            ach4.CategoryId = ra::etoi(ra::data::models::AssetCategory::Unofficial);
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchUserUnlocks>([](const ra::api::FetchUserUnlocks::Request& request, ra::api::FetchUserUnlocks::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);
            Assert::IsFalse(request.Hardcore);

            response.UnlockedAchievements.insert(7U); // core achievement
            response.UnlockedAchievements.insert(9U); // unofficial achievement
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request&, ra::api::FetchCodeNotes::Response&)
        {
            return true;
        });

        game.LoadGame(1U);
        game.mockThreadPool.ExecuteNextTask(); // FetchUserUnlocks and FetchCodeNotes are async
        game.mockThreadPool.ExecuteNextTask();

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
        Assert::AreEqual(std::wstring(L"Loaded My Game"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"2 achievements, 15 points"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"You have earned 1 achievements"), pPopup->GetDetail());
        Assert::AreEqual(std::string("3333"), pPopup->GetImage().Name());
    }

    TEST_METHOD(TestLoadGameUserUnlocksCompatibilityMode)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            auto& ach1 = response.Achievements.emplace_back();
            ach1.Id = 5;
            ach1.CategoryId = ra::etoi(ra::data::models::AssetCategory::Core);

            auto& ach2 = response.Achievements.emplace_back();
            ach2.Id = 7;
            ach2.CategoryId = ra::etoi(ra::data::models::AssetCategory::Core);
            return true;
        });

        game.mockServer.ExpectUncalled<ra::api::FetchUserUnlocks>();

        game.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request&, ra::api::FetchCodeNotes::Response&)
        {
            return true;
        });

        game.LoadGame(1U, ra::data::context::GameContext::Mode::CompatibilityTest);
        game.mockThreadPool.ExecuteNextTask(); // FetchUserUnlocks and FetchCodeNotes are async
        game.mockThreadPool.ExecuteNextTask();

        const auto* pAch1 = game.Assets().FindAchievement(5U);
        Assert::IsNotNull(pAch1);
        Ensures(pAch1 != nullptr);
        Assert::IsTrue(pAch1->IsActive());

        const auto* pAch2 = game.Assets().FindAchievement(7U);
        Assert::IsNotNull(pAch2);
        Ensures(pAch2 != nullptr);
        Assert::IsTrue(pAch2->IsActive());
    }

    TEST_METHOD(TestLoadGamePausesRuntime)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response&)
        {
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchUserUnlocks>([](const ra::api::FetchUserUnlocks::Request&, ra::api::FetchUserUnlocks::Response&)
        {
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request&, ra::api::FetchCodeNotes::Response&)
        {
            return true;
        });

        game.LoadGame(1U);
        Assert::IsTrue(game.runtime.IsPaused());

        game.mockThreadPool.ExecuteNextTask(); // FetchUserUnlocks and FetchCodeNotes are async
        game.mockThreadPool.ExecuteNextTask();
        Assert::IsFalse(game.runtime.IsPaused());
    }

    TEST_METHOD(TestLoadGameWhileRuntimePaused)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response&)
        {
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchUserUnlocks>([](const ra::api::FetchUserUnlocks::Request&, ra::api::FetchUserUnlocks::Response&)
        {
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request&, ra::api::FetchCodeNotes::Response&)
        {
            return true;
        });

        game.runtime.SetPaused(true);

        game.LoadGame(1U);
        Assert::IsTrue(game.runtime.IsPaused());

        game.mockThreadPool.ExecuteNextTask(); // FetchUserUnlocks and FetchCodeNotes are async
        game.mockThreadPool.ExecuteNextTask();
        Assert::IsTrue(game.runtime.IsPaused());
    }

    TEST_METHOD(TestReloadRichPresenceScriptNoFile)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            response.RichPresence = "Display:\nHello, World";
            return true;
        });

        game.LoadGame(1U);

        /* load game will write the server RP to storage */
        game.ReloadRichPresenceScript();

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::wstring(L"Hello, World"), game.GetRichPresenceDisplayString());
        Assert::IsFalse(game.IsRichPresenceFromFile());

        /* replace written server RP with empty file */
        game.mockStorage.DeleteStoredData(ra::services::StorageItemType::RichPresence, L"1");
        game.ReloadRichPresenceScript();

        Assert::IsFalse(game.HasRichPresence());
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), game.GetRichPresenceDisplayString());
        Assert::IsTrue(game.IsRichPresenceFromFile());
    }

    TEST_METHOD(TestReloadRichPresenceScriptWindowsLineEndings)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            response.RichPresence = "Display:\r\nHello, World\r\n";
            return true;
        });

        game.LoadGame(1U);

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
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            response.RichPresence = "Display:\nHello, World";
            return true;
        });

        game.LoadGame(1U);

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
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            response.RichPresence = "Display:\nHello, World";
            return true;
        });

        game.LoadGame(1U);

        /* load game will write the server RP to storage, so overwrite it now */
        std::string sFileContents = "...Display:\nFrom File";
        sFileContents.at(0) = gsl::narrow_cast<char>(0xef);
        sFileContents.at(1) = gsl::narrow_cast<char>(0xbb);
        sFileContents.at(2) = gsl::narrow_cast<char>(0xbf);

        game.mockStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", sFileContents);
        game.ReloadRichPresenceScript();

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::wstring(L"From File"), game.GetRichPresenceDisplayString());
        Assert::IsTrue(game.IsRichPresenceFromFile());

        /* hash should ignore BOM and convert windows newlines to unix newlines */
        sFileContents = "...Display:\r\nHello, World";
        sFileContents.at(0) = gsl::narrow_cast<char>(0xef);
        sFileContents.at(1) = gsl::narrow_cast<char>(0xbb);
        sFileContents.at(2) = gsl::narrow_cast<char>(0xbf);
        game.mockStorage.MockStoredData(ra::services::StorageItemType::RichPresence, L"1", sFileContents);
        game.ReloadRichPresenceScript();

        Assert::IsTrue(game.HasRichPresence());
        Assert::AreEqual(std::wstring(L"Hello, World"), game.GetRichPresenceDisplayString());
        Assert::IsFalse(game.IsRichPresenceFromFile());
    }

    TEST_METHOD(TestAwardAchievementNonExistant)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockServer.ExpectUncalled<ra::api::AwardAchievement>();

        game.AwardAchievement(1U);

        Assert::IsFalse(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        Assert::IsFalse(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\acherror.wav"));
        Assert::IsNull(game.mockOverlayManager.GetMessage(1));

        // AwardAchievement API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();
    }

    TEST_METHOD(TestAwardAchievement)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.SetGameHash("hash");
        game.mockServer.HandleRequest<ra::api::AwardAchievement>([](const ra::api::AwardAchievement::Request& request, ra::api::AwardAchievement::Response& response)
        {
            Assert::AreEqual(1U, request.AchievementId);
            Assert::AreEqual(false, request.Hardcore);
            Assert::AreEqual(std::string("hash"), request.GameHash);

            response.NewPlayerScore = 125U;
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        game.MockAchievement();
        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"AchievementDescription"), pPopup->GetDetail());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());

        game.mockThreadPool.ExecuteNextTask();
        Assert::AreEqual(125U, game.mockUser.GetScore());
    }

    TEST_METHOD(TestAwardAchievementHardcore)
    {
        GameContextHarness game;
        game.SetGameHash("hash");
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        game.mockServer.HandleRequest<ra::api::AwardAchievement>([](const ra::api::AwardAchievement::Request& request, ra::api::AwardAchievement::Response& response)
        {
            Assert::AreEqual(1U, request.AchievementId);
            Assert::AreEqual(true, request.Hardcore);
            Assert::AreEqual(std::string("hash"), request.GameHash);

            response.NewPlayerScore = 125U;
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        game.MockAchievement();
        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"AchievementDescription"), pPopup->GetDetail());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());

        game.mockThreadPool.ExecuteNextTask();
        Assert::AreEqual(125U, game.mockUser.GetScore());
    }

    TEST_METHOD(TestAwardAchievementNoPopup)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::None);
        game.SetGameHash("hash");
        game.mockServer.HandleRequest<ra::api::AwardAchievement>([](const ra::api::AwardAchievement::Request& request, ra::api::AwardAchievement::Response& response)
        {
            Assert::AreEqual(1U, request.AchievementId);
            Assert::AreEqual(false, request.Hardcore);
            Assert::AreEqual(std::string("hash"), request.GameHash);

            response.NewPlayerScore = 125U;
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        game.MockAchievement();
        game.AwardAchievement(1U);

        // sound should still be played
        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));

        // popup should not be displayed
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Assert::IsNull(pPopup);

        // API call should still occur
        game.mockThreadPool.ExecuteNextTask();
        Assert::AreEqual(125U, game.mockUser.GetScore());
    }

    TEST_METHOD(TestAwardAchievementLocal)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockServer.ExpectUncalled<ra::api::AwardAchievement>();

        auto& pAch = game.MockAchievement();
        pAch.SetCategory(ra::data::models::AssetCategory::Local);
        pAch.UpdateLocalCheckpoint();
        Assert::IsFalse(pAch.IsModified());

        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Local Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"AchievementDescription"), pPopup->GetDetail());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());

        // AwardAchievement API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();
    }

    TEST_METHOD(TestAwardAchievementUnofficial)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockServer.ExpectUncalled<ra::api::AwardAchievement>();

        auto& pAch = game.MockAchievement();
        pAch.SetCategory(ra::data::models::AssetCategory::Unofficial);
        pAch.UpdateServerCheckpoint();
        Assert::IsFalse(pAch.IsModified());

        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Unofficial Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"AchievementDescription"), pPopup->GetDetail());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());

        // AwardAchievement API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();
    }

    TEST_METHOD(TestAwardAchievementModified)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockServer.ExpectUncalled<ra::api::AwardAchievement>();

        auto& pAch = game.MockAchievement();
        pAch.SetPoints(99);
        Assert::IsTrue(pAch.IsModified());
        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\acherror.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Modified Achievement NOT Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (99)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"AchievementDescription"), pPopup->GetDetail());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());

        // AwardAchievement API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();
    }

    TEST_METHOD(TestAwardAchievementModifiedLocal)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockServer.ExpectUncalled<ra::api::AwardAchievement>();

        auto& pAch = game.MockAchievement();
        pAch.SetCategory(ra::data::models::AssetCategory::Local);
        pAch.SetPoints(99);
        Assert::IsTrue(pAch.IsModified());
        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\acherror.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Modified Local Achievement NOT Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (99)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"AchievementDescription"), pPopup->GetDetail());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());

        // AwardAchievement API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();
    }

    TEST_METHOD(TestAwardAchievementModifiedUnofficial)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockServer.ExpectUncalled<ra::api::AwardAchievement>();

        auto& pAch = game.MockAchievement();
        pAch.SetCategory(ra::data::models::AssetCategory::Unofficial);
        pAch.SetPoints(99);
        Assert::IsTrue(pAch.IsModified());
        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\acherror.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Modified Unofficial Achievement NOT Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (99)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"AchievementDescription"), pPopup->GetDetail());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());

        // AwardAchievement API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();
    }

    TEST_METHOD(TestAwardAchievementUnpublished)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockServer.ExpectUncalled<ra::api::AwardAchievement>();

        auto& pAch = game.MockAchievement();
        pAch.SetPoints(99);
        pAch.UpdateLocalCheckpoint();
        Assert::IsFalse(pAch.IsModified());
        Assert::AreEqual(ra::data::models::AssetChanges::Unpublished, pAch.GetChanges());
        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\acherror.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Modified Achievement NOT Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (99)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"AchievementDescription"), pPopup->GetDetail());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());

        // AwardAchievement API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();
    }

    TEST_METHOD(TestAwardAchievementMemoryModified)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockServer.ExpectUncalled<ra::api::AwardAchievement>();

        game.MockAchievement();
        game.mockEmulator.MockMemoryModified(true);
        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\acherror.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Achievement NOT Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Error: RAM tampered with"), pPopup->GetDetail());
        Assert::IsTrue(pPopup->IsDetailError());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());

        // AwardAchievement API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();
    }

    TEST_METHOD(TestAwardAchievementMemoryInsecureHardcore)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        game.mockServer.ExpectUncalled<ra::api::AwardAchievement>();

        game.MockAchievement();
        game.mockEmulator.MockMemoryInsecure(true);
        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\acherror.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Achievement NOT Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Error: RAM insecure"), pPopup->GetDetail());
        Assert::IsTrue(pPopup->IsDetailError());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());

        // AwardAchievement API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();
    }

    TEST_METHOD(TestAwardAchievementMemoryInsecureNonHardcore)
    {
        GameContextHarness game;
        game.SetGameHash("hash");
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        game.mockServer.HandleRequest<ra::api::AwardAchievement>([](const ra::api::AwardAchievement::Request& request, ra::api::AwardAchievement::Response& response)
        {
            Assert::AreEqual(1U, request.AchievementId);
            Assert::AreEqual(false, request.Hardcore);
            Assert::AreEqual(std::string("hash"), request.GameHash);

            response.NewPlayerScore = 125U;
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        game.MockAchievement();
        game.mockEmulator.MockMemoryInsecure(true);
        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"AchievementDescription"), pPopup->GetDetail());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());

        // AwardAchievement API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();
    }

    TEST_METHOD(TestAwardAchievementDuplicate)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockServer.HandleRequest<ra::api::AwardAchievement>([](const ra::api::AwardAchievement::Request&, ra::api::AwardAchievement::Response& response)
        {
            response.ErrorMessage = "User already has this achievement awarded.";
            response.Result = ra::api::ApiResult::Error;
            return true;
        });

        game.MockAchievement();
        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"AchievementDescription"), pPopup->GetDetail());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());

        game.mockThreadPool.ExecuteNextTask();

        // special error for "already unlocked" should not be reported
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());
    }

    TEST_METHOD(TestAwardAchievementDuplicateHardcore)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        game.mockServer.HandleRequest<ra::api::AwardAchievement>([](const ra::api::AwardAchievement::Request&, ra::api::AwardAchievement::Response& response)
        {
            response.ErrorMessage = "User already has hardcore and regular achievements awarded.";
            response.Result = ra::api::ApiResult::Error;
            return true;
        });

        game.MockAchievement();
        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"AchievementDescription"), pPopup->GetDetail());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());

        game.mockThreadPool.ExecuteNextTask();

        // special error for "already unlocked" should not be reported
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());
    }

    TEST_METHOD(TestAwardAchievementError)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        game.mockServer.HandleRequest<ra::api::AwardAchievement>([](const ra::api::AwardAchievement::Request&, ra::api::AwardAchievement::Response& response)
        {
            response.ErrorMessage = "Achievement data cannot be found for 1";
            response.Result = ra::api::ApiResult::Error;
            return true;
        });

        game.MockAchievement();
        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"AchievementDescription"), pPopup->GetDetail());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());

        game.mockThreadPool.ExecuteNextTask();

        // error message should be reported
        Assert::AreEqual(std::wstring(L"Achievement NOT Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Achievement data cannot be found for 1"), pPopup->GetDetail());
        Assert::IsTrue(pPopup->IsDetailError());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());
    }

    TEST_METHOD(TestAwardAchievementErrorAfterPopupDismissed)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        game.mockServer.HandleRequest<ra::api::AwardAchievement>([](const ra::api::AwardAchievement::Request&, ra::api::AwardAchievement::Response& response)
        {
            response.ErrorMessage = "Achievement data cannot be found for 1";
            response.Result = ra::api::ApiResult::Error;
            return true;
        });

        game.MockAchievement();
        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"AchievementDescription"), pPopup->GetDetail());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());

        // if error occurs after original popup is gone, a new one should be created to display the error
        ra::services::ServiceLocator::ServiceOverride<ra::data::context::GameContext> contextOverride(&game, false);
        game.mockOverlayManager.ClearPopups();
        game.mockThreadPool.ExecuteNextTask();

        // error message should be reported
        pPopup = game.mockOverlayManager.GetMessage(2);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Achievement NOT Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Achievement data cannot be found for 1"), pPopup->GetDetail());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());
    }

    TEST_METHOD(TestAwardAchievementCompatibilityMode)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockServer.ExpectUncalled<ra::api::AwardAchievement>();
        game.SetMode(ra::data::context::GameContext::Mode::CompatibilityTest);

        game.MockAchievement();
        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Test Achievement Unlocked"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5)"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"AchievementDescription"), pPopup->GetDetail());
        Assert::AreEqual(std::string("12345"), pPopup->GetImage().Name());

        // AwardAchievement API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();
    }

    TEST_METHOD(TestAwardAchievementMasteryHardcore)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Mastery, ra::ui::viewmodels::PopupLocation::TopMiddle);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        game.SetGameId(1U);
        game.SetGameHash("hash");
        game.SetGameTitle(L"GameName");
        game.mockServer.HandleRequest<ra::api::AwardAchievement>([](const ra::api::AwardAchievement::Request&, ra::api::AwardAchievement::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            response.AchievementsRemaining = 0;
            return true;
        });

        game.mockConfiguration.SetUsername("Username");
        game.mockSessionTracker.MockSession(1, 123456789, std::chrono::seconds(78 * 60));

        game.MockAchievement();
        auto& pAch2 = game.MockAchievement();
        pAch2.SetID(2U);

        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());

        pPopup = game.mockOverlayManager.GetMessage(2);
        Assert::IsNull(pPopup);

        game.AwardAchievement(2U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        pPopup = game.mockOverlayManager.GetMessage(2);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());

        pPopup = game.mockOverlayManager.GetMessage(3);
        Assert::IsNull(pPopup);

        game.mockThreadPool.ExecuteNextTask(); // award achievement 1
        game.mockThreadPool.ExecuteNextTask(); // award achievement 2
        game.mockThreadPool.ExecuteNextTask(); // check unlocks

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        pPopup = game.mockOverlayManager.GetMessage(3);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Mastered GameName"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"2 achievements, 10 points"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Username | Play time: 1h18m"), pPopup->GetDetail());

        pPopup = game.mockOverlayManager.GetMessage(4); // mastery should only have been shown once
        Expects(pPopup == nullptr);
    }

    TEST_METHOD(TestAwardAchievementMasteryNonHardcore)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Mastery, ra::ui::viewmodels::PopupLocation::TopMiddle);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        game.SetGameId(1U);
        game.SetGameHash("hash");
        game.SetGameTitle(L"GameName");
        game.mockServer.HandleRequest<ra::api::AwardAchievement>([](const ra::api::AwardAchievement::Request&, ra::api::AwardAchievement::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            response.AchievementsRemaining = 0;
            return true;
        });

        game.mockConfiguration.SetUsername("Player");
        game.mockSessionTracker.MockSession(1, 123456789, std::chrono::seconds((102*60 + 3) * 60));

        game.MockAchievement();
        auto& pAch2 = game.MockAchievement();
        pAch2.SetID(2U);

        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());

        pPopup = game.mockOverlayManager.GetMessage(2);
        Assert::IsNull(pPopup);

        game.AwardAchievement(2U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        pPopup = game.mockOverlayManager.GetMessage(2);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());

        pPopup = game.mockOverlayManager.GetMessage(3);
        Assert::IsNull(pPopup);

        game.mockThreadPool.ExecuteNextTask(); // award achievement 1
        game.mockThreadPool.ExecuteNextTask(); // award achievement 2
        game.mockThreadPool.ExecuteNextTask(); // check unlocks

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        pPopup = game.mockOverlayManager.GetMessage(3);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Completed GameName"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"2 achievements, 10 points"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Player | Play time: 102h03m"), pPopup->GetDetail());

        pPopup = game.mockOverlayManager.GetMessage(4); // mastery should only have been shown once
        Expects(pPopup == nullptr);
    }

    TEST_METHOD(TestAwardAchievementMasteryIncomplete)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::AchievementTriggered, ra::ui::viewmodels::PopupLocation::BottomLeft);
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::Mastery, ra::ui::viewmodels::PopupLocation::TopMiddle);
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        game.SetGameHash("hash");
        game.SetGameTitle(L"GameName");
        game.mockServer.HandleRequest<ra::api::AwardAchievement>([](const ra::api::AwardAchievement::Request&, ra::api::AwardAchievement::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            response.AchievementsRemaining = 1;
            return true;
        });

        game.MockAchievement();
        auto& pAch2 = game.MockAchievement();
        pAch2.SetID(2U);

        game.AwardAchievement(1U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());

        pPopup = game.mockOverlayManager.GetMessage(2);
        Assert::IsNull(pPopup);

        game.AwardAchievement(2U);

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\unlock.wav"));
        pPopup = game.mockOverlayManager.GetMessage(2);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Achievement Unlocked"), pPopup->GetTitle());

        pPopup = game.mockOverlayManager.GetMessage(3);
        Assert::IsNull(pPopup);

        game.mockThreadPool.ExecuteNextTask(); // award achievement 1
        game.mockThreadPool.ExecuteNextTask(); // award achievement 2
        game.mockThreadPool.ExecuteNextTask(); // check unlocks

        // server indicates at least one achievement is not unlocked - reset by user?
        pPopup = game.mockOverlayManager.GetMessage(3);
        Assert::IsNull(pPopup);
    }

    TEST_METHOD(TestSubmitLeaderboardEntryNonExistant)
    {
        GameContextHarness game;
        game.mockServer.ExpectUncalled<ra::api::SubmitLeaderboardEntry>();

        game.SubmitLeaderboardEntry(1U, 1234U);

        Assert::IsFalse(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\info.wav"));
        Assert::IsNull(game.mockOverlayManager.GetMessage(1));

        // SubmitLeaderboardEntry API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();
    }

    TEST_METHOD(TestSubmitLeaderboardEntry)
    {
        GameContextHarness game;
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard, ra::ui::viewmodels::PopupLocation::BottomRight);
        game.mockUser.Initialize("Player", "ApiToken");
        game.SetGameHash("hash");

        unsigned int nNewScore = 0U;
        game.mockServer.HandleRequest<ra::api::SubmitLeaderboardEntry>([&nNewScore]
            (const ra::api::SubmitLeaderboardEntry::Request& request, ra::api::SubmitLeaderboardEntry::Response& response)
        {
            Assert::AreEqual(1U, request.LeaderboardId);
            Assert::AreEqual(1234, request.Score);
            Assert::AreEqual(std::string("hash"), request.GameHash);
            nNewScore = request.Score;

            response.Result = ra::api::ApiResult::Success;
            response.TopEntries.push_back({ 1, "George", 1000U });
            response.TopEntries.push_back({ 2, "Player", 1234U });
            response.TopEntries.push_back({ 3, "Steve", 1500U });
            response.TopEntries.push_back({ 4, "Bill", 1700U });

            response.Score = 1234U;
            response.BestScore = 1234U;
            response.NewRank = 2;
            return true;
        });

        game.MockLeaderboard();
        game.SubmitLeaderboardEntry(1U, 1234U);

        game.mockThreadPool.ExecuteNextTask();
        Assert::AreEqual(1234U, nNewScore);

        const auto* vmScoreboard = game.mockOverlayManager.GetScoreboard(1U);
        Assert::IsNotNull(vmScoreboard);
        Ensures(vmScoreboard != nullptr);
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), vmScoreboard->GetHeaderText());
        Assert::AreEqual({ 4U }, vmScoreboard->Entries().Count());

        const auto* vmEntry1 = vmScoreboard->Entries().GetItemAt(0);
        Assert::IsNotNull(vmEntry1);
        Ensures(vmEntry1 != nullptr);
        Assert::AreEqual(1, vmEntry1->GetRank());
        Assert::AreEqual(std::wstring(L"George"), vmEntry1->GetUserName());
        Assert::AreEqual(std::wstring(L"1000"), vmEntry1->GetScore());
        Assert::IsFalse(vmEntry1->IsHighlighted());

        const auto* vmEntry2 = vmScoreboard->Entries().GetItemAt(1);
        Assert::IsNotNull(vmEntry2);
        Ensures(vmEntry2 != nullptr);
        Assert::AreEqual(2, vmEntry2->GetRank());
        Assert::AreEqual(std::wstring(L"Player"), vmEntry2->GetUserName());
        Assert::AreEqual(std::wstring(L"1234"), vmEntry2->GetScore());
        Assert::IsTrue(vmEntry2->IsHighlighted());

        const auto* vmEntry3 = vmScoreboard->Entries().GetItemAt(2);
        Assert::IsNotNull(vmEntry3);
        Ensures(vmEntry3 != nullptr);
        Assert::AreEqual(3, vmEntry3->GetRank());
        Assert::AreEqual(std::wstring(L"Steve"), vmEntry3->GetUserName());
        Assert::AreEqual(std::wstring(L"1500"), vmEntry3->GetScore());
        Assert::IsFalse(vmEntry3->IsHighlighted());

        const auto* vmEntry4 = vmScoreboard->Entries().GetItemAt(3);
        Assert::IsNotNull(vmEntry4);
        Ensures(vmEntry4 != nullptr);
        Assert::AreEqual(4, vmEntry4->GetRank());
        Assert::AreEqual(std::wstring(L"Bill"), vmEntry4->GetUserName());
        Assert::AreEqual(std::wstring(L"1700"), vmEntry4->GetScore());
        Assert::IsFalse(vmEntry4->IsHighlighted());
    }

    TEST_METHOD(TestSubmitLeaderboardEntryScoreboardDisabled)
    {
        GameContextHarness game;
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard, ra::ui::viewmodels::PopupLocation::None);
        game.mockUser.Initialize("Player", "ApiToken");
        game.SetGameHash("hash");

        unsigned int nNewScore = 0U;
        game.mockServer.HandleRequest<ra::api::SubmitLeaderboardEntry>([&nNewScore]
        (const ra::api::SubmitLeaderboardEntry::Request& request, ra::api::SubmitLeaderboardEntry::Response& response)
        {
            nNewScore = request.Score;

            response.Result = ra::api::ApiResult::Success;
            response.TopEntries.push_back({ 1, "George", 1000U });
            response.TopEntries.push_back({ 2, "Player", 1234U });
            response.TopEntries.push_back({ 3, "Steve", 1500U });
            response.TopEntries.push_back({ 4, "Bill", 1700U });
            return true;
        });

        game.MockLeaderboard();
        game.SubmitLeaderboardEntry(1U, 1234U);

        game.mockThreadPool.ExecuteNextTask();
        Assert::AreEqual(1234U, nNewScore);

        const auto* vmScoreboard = game.mockOverlayManager.GetScoreboard(1U);
        Assert::IsNull(vmScoreboard);
    }

    TEST_METHOD(TestSubmitLeaderboardEntryNonHardcore)
    {
        GameContextHarness game;
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard, ra::ui::viewmodels::PopupLocation::BottomRight);
        game.mockUser.Initialize("Player", "ApiToken");
        game.SetGameHash("hash");

        game.mockServer.ExpectUncalled<ra::api::SubmitLeaderboardEntry>();

        game.MockLeaderboard();
        game.SubmitLeaderboardEntry(1U, 1234U);

        // SubmitLeaderboardEntry API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\info.wav"));

        // error message should be reported
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Leaderboard NOT Submitted"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Submission requires Hardcore mode"), pPopup->GetDetail());
        Assert::IsTrue(pPopup->IsDetailError());

        // empty leaderboard should be displayed with the non-submitted score
        const auto* vmScoreboard = game.mockOverlayManager.GetScoreboard(1U);
        Assert::IsNotNull(vmScoreboard);
        Ensures(vmScoreboard != nullptr);
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), vmScoreboard->GetHeaderText());
        Assert::AreEqual({ 1U }, vmScoreboard->Entries().Count());

        const auto* vmEntry = vmScoreboard->Entries().GetItemAt(0);
        Assert::IsNotNull(vmEntry);
        Ensures(vmEntry != nullptr);
        Assert::AreEqual(0, vmEntry->GetRank());
        Assert::AreEqual(std::wstring(L"Player"), vmEntry->GetUserName());
        Assert::AreEqual(std::wstring(L"1234"), vmEntry->GetScore());
        Assert::IsTrue(vmEntry->IsHighlighted());
    }

    TEST_METHOD(TestSubmitLeaderboardEntryModified)
    {
        GameContextHarness game;
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard, ra::ui::viewmodels::PopupLocation::BottomRight);
        game.mockUser.Initialize("Player", "ApiToken");
        game.SetMode(ra::data::context::GameContext::Mode::CompatibilityTest);
        game.SetGameHash("hash");

        game.mockServer.ExpectUncalled<ra::api::SubmitLeaderboardEntry>();

        game.MockLeaderboard();
        game.Assets().FindLeaderboard(1U)->SetName(L"Name2");
        game.SubmitLeaderboardEntry(1U, 1234U);

        // SubmitLeaderboardEntry API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\info.wav"));

        // error message should be reported
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Modified Leaderboard NOT Submitted"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"Name2"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Leaderboards are not submitted in test mode."), pPopup->GetDetail());
        Assert::IsFalse(pPopup->IsDetailError());

        // empty leaderboard should be displayed with the non-submitted score
        const auto* vmScoreboard = game.mockOverlayManager.GetScoreboard(1U);
        Assert::IsNotNull(vmScoreboard);
        Ensures(vmScoreboard != nullptr);
        Assert::AreEqual(std::wstring(L"Name2"), vmScoreboard->GetHeaderText());
        Assert::AreEqual({ 1U }, vmScoreboard->Entries().Count());

        const auto* vmEntry = vmScoreboard->Entries().GetItemAt(0);
        Assert::IsNotNull(vmEntry);
        Ensures(vmEntry != nullptr);
        Assert::AreEqual(0, vmEntry->GetRank());
        Assert::AreEqual(std::wstring(L"Player"), vmEntry->GetUserName());
        Assert::AreEqual(std::wstring(L"1234"), vmEntry->GetScore());
        Assert::IsTrue(vmEntry->IsHighlighted());
    }

    TEST_METHOD(TestSubmitLeaderboardEntryCompatibilityMode)
    {
        GameContextHarness game;
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard, ra::ui::viewmodels::PopupLocation::BottomRight);
        game.mockUser.Initialize("Player", "ApiToken");
        game.SetMode(ra::data::context::GameContext::Mode::CompatibilityTest);
        game.SetGameHash("hash");

        game.mockServer.ExpectUncalled<ra::api::SubmitLeaderboardEntry>();

        game.MockLeaderboard();
        game.SubmitLeaderboardEntry(1U, 1234U);

        // SubmitLeaderboardEntry API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\info.wav"));

        // error message should be reported
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Leaderboard NOT Submitted"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Leaderboards are not submitted in test mode."), pPopup->GetDetail());
        Assert::IsFalse(pPopup->IsDetailError());

        // empty leaderboard should be displayed with the non-submitted score
        const auto* vmScoreboard = game.mockOverlayManager.GetScoreboard(1U);
        Assert::IsNotNull(vmScoreboard);
        Ensures(vmScoreboard != nullptr);
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), vmScoreboard->GetHeaderText());
        Assert::AreEqual({ 1U }, vmScoreboard->Entries().Count());

        const auto* vmEntry = vmScoreboard->Entries().GetItemAt(0);
        Assert::IsNotNull(vmEntry);
        Ensures(vmEntry != nullptr);
        Assert::AreEqual(0, vmEntry->GetRank());
        Assert::AreEqual(std::wstring(L"Player"), vmEntry->GetUserName());
        Assert::AreEqual(std::wstring(L"1234"), vmEntry->GetScore());
        Assert::IsTrue(vmEntry->IsHighlighted());
    }

    TEST_METHOD(TestSubmitLeaderboardEntryLowRank)
    {
        GameContextHarness game;
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard, ra::ui::viewmodels::PopupLocation::BottomRight);
        game.mockUser.Initialize("Player", "ApiToken");
        game.SetGameHash("hash");

        game.mockServer.HandleRequest<ra::api::SubmitLeaderboardEntry>([]
            (const ra::api::SubmitLeaderboardEntry::Request&, ra::api::SubmitLeaderboardEntry::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            response.TopEntries.push_back({ 1, "George", 1000U });
            response.TopEntries.push_back({ 2, "Philip", 1100U });
            response.TopEntries.push_back({ 3, "Steve", 1200U });
            response.TopEntries.push_back({ 4, "Bill", 1300U });
            response.TopEntries.push_back({ 5, "Roger", 1400U });
            response.TopEntries.push_back({ 6, "Andy", 1500U });
            response.TopEntries.push_back({ 7, "Jason", 1600U });
            response.TopEntries.push_back({ 8, "Jeff", 1700U });

            response.Score = 2000U;
            response.BestScore = 1900U;
            response.NewRank = 11;
            return true;
        });

        game.MockLeaderboard();
        game.SubmitLeaderboardEntry(1U, 2000U);

        game.mockThreadPool.ExecuteNextTask();

        const auto* vmScoreboard = game.mockOverlayManager.GetScoreboard(1U);
        Assert::IsNotNull(vmScoreboard);
        Ensures(vmScoreboard != nullptr);
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), vmScoreboard->GetHeaderText());
        Assert::AreEqual({ 7U }, vmScoreboard->Entries().Count());

        const auto* vmEntry1 = vmScoreboard->Entries().GetItemAt(0);
        Assert::IsNotNull(vmEntry1);
        Ensures(vmEntry1 != nullptr);
        Assert::AreEqual(1, vmEntry1->GetRank());
        Assert::AreEqual(std::wstring(L"George"), vmEntry1->GetUserName());
        Assert::AreEqual(std::wstring(L"1000"), vmEntry1->GetScore());
        Assert::IsFalse(vmEntry1->IsHighlighted());

        const auto* vmEntry6 = vmScoreboard->Entries().GetItemAt(5);
        Assert::IsNotNull(vmEntry6);
        Ensures(vmEntry6 != nullptr);
        Assert::AreEqual(6, vmEntry6->GetRank());
        Assert::AreEqual(std::wstring(L"Andy"), vmEntry6->GetUserName());
        Assert::AreEqual(std::wstring(L"1500"), vmEntry6->GetScore());
        Assert::IsFalse(vmEntry6->IsHighlighted());

        const auto* vmEntry7 = vmScoreboard->Entries().GetItemAt(6);
        Assert::IsNotNull(vmEntry7);
        Ensures(vmEntry7 != nullptr);
        Assert::AreEqual(11, vmEntry7->GetRank());
        Assert::AreEqual(std::wstring(L"Player"), vmEntry7->GetUserName());
        Assert::AreEqual(std::wstring(L"(2000) 1900"), vmEntry7->GetScore());
        Assert::IsTrue(vmEntry7->IsHighlighted());
    }

    TEST_METHOD(TestSubmitLeaderboardEntryScoreNotImproved)
    {
        GameContextHarness game;
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard, ra::ui::viewmodels::PopupLocation::BottomRight);
        game.mockUser.Initialize("Player", "ApiToken");
        game.SetGameHash("hash");

        unsigned int nNewScore = 0U;
        game.mockServer.HandleRequest<ra::api::SubmitLeaderboardEntry>([&nNewScore]
        (const ra::api::SubmitLeaderboardEntry::Request & request, ra::api::SubmitLeaderboardEntry::Response & response)
        {
            Assert::AreEqual(1U, request.LeaderboardId);
            Assert::AreEqual(1234, request.Score);
            Assert::AreEqual(std::string("hash"), request.GameHash);
            nNewScore = request.Score;

            response.Result = ra::api::ApiResult::Success;
            response.TopEntries.push_back({ 1, "George", 1000U });
            response.TopEntries.push_back({ 2, "Player", 1200U });
            response.TopEntries.push_back({ 3, "Steve", 1500U });
            response.TopEntries.push_back({ 4, "Bill", 1700U });

            response.Score = 1234U;
            response.BestScore = 1200U;
            response.NewRank = 2;
            return true;
        });

        game.MockLeaderboard();
        game.SubmitLeaderboardEntry(1U, 1234U);

        game.mockThreadPool.ExecuteNextTask();
        Assert::AreEqual(1234U, nNewScore);

        const auto* vmScoreboard = game.mockOverlayManager.GetScoreboard(1U);
        Assert::IsNotNull(vmScoreboard);
        Ensures(vmScoreboard != nullptr);
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), vmScoreboard->GetHeaderText());
        Assert::AreEqual({ 4U }, vmScoreboard->Entries().Count());

        const auto* vmEntry2 = vmScoreboard->Entries().GetItemAt(1);
        Assert::IsNotNull(vmEntry2);
        Ensures(vmEntry2 != nullptr);
        Assert::AreEqual(2, vmEntry2->GetRank());
        Assert::AreEqual(std::wstring(L"Player"), vmEntry2->GetUserName());
        Assert::AreEqual(std::wstring(L"(1234) 1200"), vmEntry2->GetScore());
        Assert::IsTrue(vmEntry2->IsHighlighted());
    }

    TEST_METHOD(TestSubmitLeaderboardEntryScoreNotImprovedSharedRank)
    {
        GameContextHarness game;
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard, ra::ui::viewmodels::PopupLocation::BottomRight);
        game.mockUser.Initialize("Player", "ApiToken");
        game.SetGameHash("hash");

        unsigned int nNewScore = 0U;
        game.mockServer.HandleRequest<ra::api::SubmitLeaderboardEntry>([&nNewScore]
        (const ra::api::SubmitLeaderboardEntry::Request& request, ra::api::SubmitLeaderboardEntry::Response& response)
        {
            Assert::AreEqual(1U, request.LeaderboardId);
            Assert::AreEqual(1234, request.Score);
            Assert::AreEqual(std::string("hash"), request.GameHash);
            nNewScore = request.Score;

            response.Result = ra::api::ApiResult::Success;
            response.TopEntries.push_back({ 1, "George", 1000U });
            response.TopEntries.push_back({ 2, "Steve", 1200U });
            response.TopEntries.push_back({ 2, "Player", 1200U });
            response.TopEntries.push_back({ 4, "Bill", 1700U });

            response.Score = 1234U;
            response.BestScore = 1200U;
            response.NewRank = 2;
            return true;
        });

        game.MockLeaderboard();
        game.SubmitLeaderboardEntry(1U, 1234U);

        game.mockThreadPool.ExecuteNextTask();
        Assert::AreEqual(1234U, nNewScore);

        const auto* vmScoreboard = game.mockOverlayManager.GetScoreboard(1U);
        Assert::IsNotNull(vmScoreboard);
        Ensures(vmScoreboard != nullptr);
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), vmScoreboard->GetHeaderText());
        Assert::AreEqual({ 4U }, vmScoreboard->Entries().Count());

        const auto* vmEntry2 = vmScoreboard->Entries().GetItemAt(1);
        Assert::IsNotNull(vmEntry2);
        Ensures(vmEntry2 != nullptr);
        Assert::AreEqual(2, vmEntry2->GetRank());
        Assert::AreEqual(std::wstring(L"Steve"), vmEntry2->GetUserName());
        Assert::AreEqual(std::wstring(L"1200"), vmEntry2->GetScore());
        Assert::IsFalse(vmEntry2->IsHighlighted());

        const auto* vmEntry3 = vmScoreboard->Entries().GetItemAt(2);
        Assert::IsNotNull(vmEntry3);
        Ensures(vmEntry3 != nullptr);
        Assert::AreEqual(2, vmEntry3->GetRank());
        Assert::AreEqual(std::wstring(L"Player"), vmEntry3->GetUserName());
        Assert::AreEqual(std::wstring(L"(1234) 1200"), vmEntry3->GetScore());
        Assert::IsTrue(vmEntry3->IsHighlighted());
    }

    TEST_METHOD(TestSubmitLeaderboardEntryScoreNotImprovedSharedRankLastEntry)
    {
        GameContextHarness game;
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard, ra::ui::viewmodels::PopupLocation::BottomRight);
        game.mockUser.Initialize("Player", "ApiToken");
        game.SetGameHash("hash");

        unsigned int nNewScore = 0U;
        game.mockServer.HandleRequest<ra::api::SubmitLeaderboardEntry>([&nNewScore]
        (const ra::api::SubmitLeaderboardEntry::Request& request, ra::api::SubmitLeaderboardEntry::Response& response)
        {
            Assert::AreEqual(1U, request.LeaderboardId);
            Assert::AreEqual(1234, request.Score);
            Assert::AreEqual(std::string("hash"), request.GameHash);
            nNewScore = request.Score;

            response.Result = ra::api::ApiResult::Success;
            response.TopEntries.push_back({ 1, "George", 1000U });
            response.TopEntries.push_back({ 2, "Harold", 1100U });
            response.TopEntries.push_back({ 2, "Roger", 1100U });
            response.TopEntries.push_back({ 4, "Phil", 1150U });
            response.TopEntries.push_back({ 5, "Steve", 1175U });
            response.TopEntries.push_back({ 6, "Terry", 1200U });
            response.TopEntries.push_back({ 6, "Edward", 1200U });
            response.TopEntries.push_back({ 6, "Player", 1200U });
            response.TopEntries.push_back({ 9, "Bill", 1300U });

            response.Score = 1234U;
            response.BestScore = 1200U;
            response.NewRank = 6;
            return true;
        });

        game.MockLeaderboard();
        game.SubmitLeaderboardEntry(1U, 1234U);

        game.mockThreadPool.ExecuteNextTask();
        Assert::AreEqual(1234U, nNewScore);

        const auto* vmScoreboard = game.mockOverlayManager.GetScoreboard(1U);
        Assert::IsNotNull(vmScoreboard);
        Ensures(vmScoreboard != nullptr);
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), vmScoreboard->GetHeaderText());
        Assert::AreEqual({ 7U }, vmScoreboard->Entries().Count());

        const auto* vmEntry6 = vmScoreboard->Entries().GetItemAt(5);
        Assert::IsNotNull(vmEntry6);
        Ensures(vmEntry6 != nullptr);
        Assert::AreEqual(6, vmEntry6->GetRank());
        Assert::AreEqual(std::wstring(L"Terry"), vmEntry6->GetUserName());
        Assert::AreEqual(std::wstring(L"1200"), vmEntry6->GetScore());
        Assert::IsFalse(vmEntry6->IsHighlighted());

        // player is entry 8, but rank 6. 7 items will be shown, 7th item should be replaced with player entry
        const auto* vmEntry7 = vmScoreboard->Entries().GetItemAt(6);
        Assert::IsNotNull(vmEntry7);
        Ensures(vmEntry7 != nullptr);
        Assert::AreEqual(6, vmEntry7->GetRank());
        Assert::AreEqual(std::wstring(L"Player"), vmEntry7->GetUserName());
        Assert::AreEqual(std::wstring(L"(1234) 1200"), vmEntry7->GetScore());
        Assert::IsTrue(vmEntry7->IsHighlighted());
    }

    TEST_METHOD(TestSubmitLeaderboardEntryMemoryModified)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard, ra::ui::viewmodels::PopupLocation::BottomRight);
        game.mockUser.Initialize("Player", "ApiToken");
        game.SetGameHash("hash");

        game.mockServer.ExpectUncalled<ra::api::SubmitLeaderboardEntry>();

        game.MockLeaderboard();
        game.mockEmulator.MockMemoryModified(true);
        game.SubmitLeaderboardEntry(1U, 1234U);

        // SubmitLeaderboardEntry API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\info.wav"));

        // error message should be reported
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Leaderboard NOT Submitted"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Error: RAM tampered with"), pPopup->GetDetail());
        Assert::IsTrue(pPopup->IsDetailError());

        // empty leaderboard should be displayed with the non-submitted score
        const auto* vmScoreboard = game.mockOverlayManager.GetScoreboard(1U);
        Assert::IsNotNull(vmScoreboard);
        Ensures(vmScoreboard != nullptr);
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), vmScoreboard->GetHeaderText());
        Assert::AreEqual({ 1U }, vmScoreboard->Entries().Count());

        const auto* vmEntry = vmScoreboard->Entries().GetItemAt(0);
        Assert::IsNotNull(vmEntry);
        Ensures(vmEntry != nullptr);
        Assert::AreEqual(0, vmEntry->GetRank());
        Assert::AreEqual(std::wstring(L"Player"), vmEntry->GetUserName());
        Assert::AreEqual(std::wstring(L"1234"), vmEntry->GetScore());
        Assert::IsTrue(vmEntry->IsHighlighted());
    }

    TEST_METHOD(TestSubmitLeaderboardEntryMemoryInsecure)
    {
        GameContextHarness game;
        game.mockConfiguration.SetPopupLocation(ra::ui::viewmodels::Popup::LeaderboardScoreboard, ra::ui::viewmodels::PopupLocation::BottomRight);
        game.mockUser.Initialize("Player", "ApiToken");
        game.SetGameHash("hash");
        game.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);

        game.mockServer.ExpectUncalled<ra::api::SubmitLeaderboardEntry>();

        game.MockLeaderboard();
        game.mockEmulator.MockMemoryInsecure(true);
        game.SubmitLeaderboardEntry(1U, 1234U);

        // SubmitLeaderboardEntry API call is async, try to execute it - expect no tasks queued
        game.mockThreadPool.ExecuteNextTask();

        Assert::IsTrue(game.mockAudioSystem.WasAudioFilePlayed(L"Overlay\\info.wav"));

        // error message should be reported
        const auto* pPopup = game.mockOverlayManager.GetMessage(1);
        Expects(pPopup != nullptr);
        Assert::IsNotNull(pPopup);
        Assert::AreEqual(std::wstring(L"Leaderboard NOT Submitted"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), pPopup->GetDescription());
        Assert::AreEqual(std::wstring(L"Error: RAM insecure"), pPopup->GetDetail());
        Assert::IsTrue(pPopup->IsDetailError());

        // empty leaderboard should be displayed with the non-submitted score
        const auto* vmScoreboard = game.mockOverlayManager.GetScoreboard(1U);
        Assert::IsNotNull(vmScoreboard);
        Ensures(vmScoreboard != nullptr);
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), vmScoreboard->GetHeaderText());
        Assert::AreEqual({ 1U }, vmScoreboard->Entries().Count());

        const auto* vmEntry = vmScoreboard->Entries().GetItemAt(0);
        Assert::IsNotNull(vmEntry);
        Ensures(vmEntry != nullptr);
        Assert::AreEqual(0, vmEntry->GetRank());
        Assert::AreEqual(std::wstring(L"Player"), vmEntry->GetUserName());
        Assert::AreEqual(std::wstring(L"1234"), vmEntry->GetScore());
        Assert::IsTrue(vmEntry->IsHighlighted());
    }

    TEST_METHOD(TestLoadCodeNotes)
    {
        GameContextNotifyTarget notifyTarget;

        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response&)
        {
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchUserUnlocks>([](const ra::api::FetchUserUnlocks::Request&, ra::api::FetchUserUnlocks::Response&)
        {
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request& request, ra::api::FetchCodeNotes::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1234, L"Note1", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 2345, L"Note2", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 3456, L"Note3", "Author" });
            return true;
        });

        game.AddNotifyTarget(notifyTarget);

        game.LoadGame(1U);
        game.mockThreadPool.ExecuteNextTask(); // FetchUserUnlocks and FetchCodeNotes are async
        game.mockThreadPool.ExecuteNextTask();

        const auto* pNote1 = game.FindCodeNote(1234U);
        Assert::IsNotNull(pNote1);
        Ensures(pNote1 != nullptr);
        Assert::AreEqual(std::wstring(L"Note1"), *pNote1);
        const auto* pNote1b = notifyTarget.GetNewCodeNote(1234U);
        Assert::IsNotNull(pNote1b);
        Ensures(pNote1b != nullptr);
        Assert::AreEqual(std::wstring(L"Note1"), *pNote1b);

        const auto* pNote2 = game.FindCodeNote(2345U);
        Assert::IsNotNull(pNote2);
        Ensures(pNote2 != nullptr);
        Assert::AreEqual(std::wstring(L"Note2"), *pNote2);
        const auto* pNote2b = notifyTarget.GetNewCodeNote(2345U);
        Assert::IsNotNull(pNote2b);
        Ensures(pNote2b != nullptr);
        Assert::AreEqual(std::wstring(L"Note2"), *pNote2b);

        const auto* pNote3 = game.FindCodeNote(3456U);
        Assert::IsNotNull(pNote3);
        Ensures(pNote3 != nullptr);
        Assert::AreEqual(std::wstring(L"Note3"), *pNote3);
        const auto* pNote3b = notifyTarget.GetNewCodeNote(3456U);
        Assert::IsNotNull(pNote3b);
        Ensures(pNote3b != nullptr);
        Assert::AreEqual(std::wstring(L"Note3"), *pNote3b);

        const auto* pNote4 = game.FindCodeNote(4567U);
        Assert::IsNull(pNote4);
        const auto* pNote4b = notifyTarget.GetNewCodeNote(4567U);
        Assert::IsNull(pNote4b);

        game.LoadGame(0U);
        const auto* pNote5 = game.FindCodeNote(1234U);
        Assert::IsNull(pNote5);
        const auto* pNote5b = notifyTarget.GetNewCodeNote(1234U);
        Assert::IsNull(pNote5b);
    }

    void TestCodeNoteSize(const std::wstring& sNote, unsigned int nExpectedBytes, MemSize nExpectedSize)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response&)
        {
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchUserUnlocks>([](const ra::api::FetchUserUnlocks::Request&, ra::api::FetchUserUnlocks::Response&)
        {
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchCodeNotes>([&sNote](const ra::api::FetchCodeNotes::Request& request, ra::api::FetchCodeNotes::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1234, sNote, "Author" });
            return true;
        });

        game.LoadGame(1U);
        game.mockThreadPool.ExecuteNextTask(); // FetchUserUnlocks and FetchCodeNotes are async
        game.mockThreadPool.ExecuteNextTask();

        Assert::AreEqual(nExpectedBytes, game.GetCodeNoteBytes(1234U), sNote.c_str());
        Assert::AreEqual(nExpectedSize, game.GetCodeNoteMemSize(1234U), sNote.c_str());
    }

    TEST_METHOD(TestLoadCodeNotesSized)
    {
        TestCodeNoteSize(L"", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"16-bit Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Test 16-bit", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Test 16-bi", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[16-bit] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[16 bit] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[16 Bit] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[24-bit] Test", 3U, MemSize::TwentyFourBit);
        TestCodeNoteSize(L"[32-bit] Test", 4U, MemSize::ThirtyTwoBit);
        TestCodeNoteSize(L"[32 bit] Test", 4U, MemSize::ThirtyTwoBit);
        TestCodeNoteSize(L"[32bit] Test", 4U, MemSize::ThirtyTwoBit);
        TestCodeNoteSize(L"Test [16-bit]", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Test (16-bit)", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Test (16 bits)", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[64-bit] Test", 8U, MemSize::Array);
        TestCodeNoteSize(L"[128-bit] Test", 16U, MemSize::Array);
        TestCodeNoteSize(L"[17-bit] Test", 3U, MemSize::TwentyFourBit);
        TestCodeNoteSize(L"[100-bit] Test", 13U, MemSize::Array);
        TestCodeNoteSize(L"[0-bit] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[1-bit] Test", 1U, MemSize::EightBit);
        TestCodeNoteSize(L"[4-bit] Test", 1U, MemSize::EightBit);
        TestCodeNoteSize(L"[8-bit] Test", 1U, MemSize::EightBit);
        TestCodeNoteSize(L"[9-bit] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"bit", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"9bit", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"-bit", 1U, MemSize::Unknown);

        TestCodeNoteSize(L"[16-bit BE] Test", 2U, MemSize::SixteenBitBigEndian);
        TestCodeNoteSize(L"[24-bit BE] Test", 3U, MemSize::TwentyFourBitBigEndian);
        TestCodeNoteSize(L"[32-bit BE] Test", 4U, MemSize::ThirtyTwoBitBigEndian);
        TestCodeNoteSize(L"Test [32-bit BE]", 4U, MemSize::ThirtyTwoBitBigEndian);
        TestCodeNoteSize(L"Test (32-bit BE)", 4U, MemSize::ThirtyTwoBitBigEndian);
        TestCodeNoteSize(L"Test 32-bit BE", 4U, MemSize::ThirtyTwoBitBigEndian);
        TestCodeNoteSize(L"[16-bit BigEndian] Test", 2U, MemSize::SixteenBitBigEndian);
        TestCodeNoteSize(L"[16-bit-BE] Test", 2U, MemSize::SixteenBitBigEndian);
        TestCodeNoteSize(L"[4-bit BE] Test", 1U, MemSize::EightBit);

        TestCodeNoteSize(L"8 BYTE Test", 8U, MemSize::Array);
        TestCodeNoteSize(L"Test 8 BYTE", 8U, MemSize::Array);
        TestCodeNoteSize(L"Test 8 BYT", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[2 Byte] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[4 Byte] Test", 4U, MemSize::ThirtyTwoBit);
        TestCodeNoteSize(L"[4 Byte - Float] Test", 4U, MemSize::Float);
        TestCodeNoteSize(L"[8 Byte] Test", 8U, MemSize::Array);
        TestCodeNoteSize(L"[100 Bytes] Test", 100U, MemSize::Array);
        TestCodeNoteSize(L"[2 byte] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[2-byte] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Test (6 bytes)", 6U, MemSize::Array);
        TestCodeNoteSize(L"[2byte] Test", 2U, MemSize::SixteenBit);

        TestCodeNoteSize(L"[float] Test", 4U, MemSize::Float);
        TestCodeNoteSize(L"[float32] Test", 4U, MemSize::Float);
        TestCodeNoteSize(L"Test float", 4U, MemSize::Float);
        TestCodeNoteSize(L"Test floa", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"is floating", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"has floated", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"16-afloat", 1U, MemSize::Unknown);

        TestCodeNoteSize(L"[MBF32] Test", 4U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF40] Test", 5U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF32 float] Test", 4U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF80] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[MBF320] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[MBF-32] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[32-bit MBF] Test", 4U, MemSize::MBF32);
        TestCodeNoteSize(L"[40-bit MBF] Test", 5U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"Test MBF32", 4U, MemSize::MBF32);

        TestCodeNoteSize(L"42=bitten", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"42-bitten", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"bit by bit", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"bit1=chest", 1U, MemSize::Unknown);

        TestCodeNoteSize(L"Bite count (16-bit)", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Number of bits collected (32 bits)", 4U, MemSize::ThirtyTwoBit);

        TestCodeNoteSize(L"100 32-bit pointers [400 bytes]", 400U, MemSize::Array);
        TestCodeNoteSize(L"[400 bytes] 100 32-bit pointers", 400U, MemSize::Array);
    }

    TEST_METHOD(TestFindCodeNoteSized)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response&)
        {
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchUserUnlocks>([](const ra::api::FetchUserUnlocks::Request&, ra::api::FetchUserUnlocks::Response&)
        {
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request& request, ra::api::FetchCodeNotes::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1000, L"[32-bit] Location", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1100, L"Level", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1110, L"[16-bit] Strength", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1120, L"[8 byte] Exp", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1200, L"[20 bytes] Items\r\nMultiline ignored", "Author" });
            return true;
        });

        game.LoadGame(1U);
        game.mockThreadPool.ExecuteNextTask(); // FetchUserUnlocks and FetchCodeNotes are async
        game.mockThreadPool.ExecuteNextTask();

        Assert::AreEqual(std::wstring(L""), game.FindCodeNote(100, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L""), game.FindCodeNote(999, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [1/4]"), game.FindCodeNote(1000, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [2/4]"), game.FindCodeNote(1001, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [3/4]"), game.FindCodeNote(1002, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [4/4]"), game.FindCodeNote(1003, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L""), game.FindCodeNote(1004, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"Level"), game.FindCodeNote(1100, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [1/2]"), game.FindCodeNote(1110, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [2/2]"), game.FindCodeNote(1111, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [1/8]"), game.FindCodeNote(1120, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [2/8]"), game.FindCodeNote(1121, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [3/8]"), game.FindCodeNote(1122, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [4/8]"), game.FindCodeNote(1123, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [5/8]"), game.FindCodeNote(1124, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [6/8]"), game.FindCodeNote(1125, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [7/8]"), game.FindCodeNote(1126, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [8/8]"), game.FindCodeNote(1127, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L""), game.FindCodeNote(1128, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[20 bytes] Items [1/20]"), game.FindCodeNote(1200, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[20 bytes] Items [10/20]"), game.FindCodeNote(1209, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[20 bytes] Items [20/20]"), game.FindCodeNote(1219, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L""), game.FindCodeNote(1300, MemSize::EightBit));

        Assert::AreEqual(std::wstring(L""), game.FindCodeNote(100, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L""), game.FindCodeNote(998, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), game.FindCodeNote(999, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), game.FindCodeNote(1000, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), game.FindCodeNote(1001, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), game.FindCodeNote(1002, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), game.FindCodeNote(1003, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L""), game.FindCodeNote(1004, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"Level [partial]"), game.FindCodeNote(1099, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"Level [partial]"), game.FindCodeNote(1100, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), game.FindCodeNote(1109, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength"), game.FindCodeNote(1110, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), game.FindCodeNote(1111, MemSize::SixteenBit));

        Assert::AreEqual(std::wstring(L""), game.FindCodeNote(100, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L""), game.FindCodeNote(996, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), game.FindCodeNote(997, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location"), game.FindCodeNote(1000, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), game.FindCodeNote(1001, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), game.FindCodeNote(1002, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), game.FindCodeNote(1003, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L""), game.FindCodeNote(1004, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"Level [partial]"), game.FindCodeNote(1097, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"Level [partial]"), game.FindCodeNote(1100, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), game.FindCodeNote(1107, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), game.FindCodeNote(1110, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), game.FindCodeNote(1111, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L""), game.FindCodeNote(1112, MemSize::ThirtyTwoBit));
    }

    TEST_METHOD(TestFindCodeNoteStart)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response&)
        {
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchUserUnlocks>([](const ra::api::FetchUserUnlocks::Request&, ra::api::FetchUserUnlocks::Response&)
        {
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request& request, ra::api::FetchCodeNotes::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1000, L"[32-bit] Location", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1100, L"Level", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1110, L"[16-bit] Strength", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1120, L"[8 byte] Exp", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1200, L"[20 bytes] Items", "Author" });
            return true;
        });

        game.LoadGame(1U);
        game.mockThreadPool.ExecuteNextTask(); // FetchUserUnlocks and FetchCodeNotes are async
        game.mockThreadPool.ExecuteNextTask();

        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(100));

        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(999));
        Assert::AreEqual(1000U, game.FindCodeNoteStart(1000));
        Assert::AreEqual(1000U, game.FindCodeNoteStart(1001));
        Assert::AreEqual(1000U, game.FindCodeNoteStart(1002));
        Assert::AreEqual(1000U, game.FindCodeNoteStart(1003));
        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(1004));

        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(1099));
        Assert::AreEqual(1100U, game.FindCodeNoteStart(1100));
        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(1101));

        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(1109));
        Assert::AreEqual(1110U, game.FindCodeNoteStart(1110));
        Assert::AreEqual(1110U, game.FindCodeNoteStart(1111));
        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(1112));

        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(1119));
        Assert::AreEqual(1120U, game.FindCodeNoteStart(1120));
        Assert::AreEqual(1120U, game.FindCodeNoteStart(1127));
        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(1128));

        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(1199));
        Assert::AreEqual(1200U, game.FindCodeNoteStart(1200));
        Assert::AreEqual(1200U, game.FindCodeNoteStart(1219));
        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(1220));
    }

    TEST_METHOD(TestFindCodeNoteStartOverlap)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response&)
        {
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchUserUnlocks>([](const ra::api::FetchUserUnlocks::Request&, ra::api::FetchUserUnlocks::Response&)
        {
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request& request, ra::api::FetchCodeNotes::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1000, L"[100 bytes] Outer", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1010, L"[10 bytes] Inner", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1015, L"Individual", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1120, L"[10 bytes] Secondary", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1125, L"[10 bytes] Overlap", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1200, L"Extra", "Author" });
            return true;
        });

        game.LoadGame(1U);
        game.mockThreadPool.ExecuteNextTask(); // FetchUserUnlocks and FetchCodeNotes are async
        game.mockThreadPool.ExecuteNextTask();

        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(999));
        Assert::AreEqual(1000U, game.FindCodeNoteStart(1000));
        Assert::AreEqual(1000U, game.FindCodeNoteStart(1009));
        Assert::AreEqual(1010U, game.FindCodeNoteStart(1010));
        Assert::AreEqual(1010U, game.FindCodeNoteStart(1014));
        Assert::AreEqual(1015U, game.FindCodeNoteStart(1015));
        Assert::AreEqual(1010U, game.FindCodeNoteStart(1016));
        Assert::AreEqual(1010U, game.FindCodeNoteStart(1019));
        Assert::AreEqual(1000U, game.FindCodeNoteStart(1020));
        Assert::AreEqual(1000U, game.FindCodeNoteStart(1099));
        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(1100));
        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(1119));
        Assert::AreEqual(1120U, game.FindCodeNoteStart(1120));
        Assert::AreEqual(1120U, game.FindCodeNoteStart(1124));
        Assert::AreEqual(1125U, game.FindCodeNoteStart(1125));
        Assert::AreEqual(1125U, game.FindCodeNoteStart(1126));
        Assert::AreEqual(1125U, game.FindCodeNoteStart(1130));
        Assert::AreEqual(1125U, game.FindCodeNoteStart(1134));
        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(1135));
        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(1199));
        Assert::AreEqual(1200U, game.FindCodeNoteStart(1200));
        Assert::AreEqual(0xFFFFFFFF, game.FindCodeNoteStart(1201));
    }

    TEST_METHOD(TestSetCodeNote)
    {
        GameContextNotifyTarget notifyTarget;
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::UpdateCodeNote>([](const ra::api::UpdateCodeNote::Request& request, ra::api::UpdateCodeNote::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);
            Assert::AreEqual(1234U, request.Address);
            Assert::AreEqual(std::wstring(L"Note1"), request.Note);

            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        game.AddNotifyTarget(notifyTarget);

        game.SetGameId(1U);
        Assert::IsTrue(game.SetCodeNote(1234, L"Note1"));

        const auto* pNote1 = game.FindCodeNote(1234U);
        Assert::IsNotNull(pNote1);
        Ensures(pNote1 != nullptr);
        Assert::AreEqual(std::wstring(L"Note1"), *pNote1);
        const auto* pNote1b = notifyTarget.GetNewCodeNote(1234U);
        Assert::IsNotNull(pNote1b);
        Ensures(pNote1b != nullptr);
        Assert::AreEqual(std::wstring(L"Note1"), *pNote1b);
    }

    TEST_METHOD(TestSetCodeNoteGameNotLoaded)
    {
        GameContextHarness game;
        game.mockServer.ExpectUncalled<ra::api::UpdateCodeNote>();

        game.SetGameId(0U);
        Assert::IsFalse(game.SetCodeNote(1234, L"Note1"));

        const auto* pNote1 = game.FindCodeNote(1234U);
        Assert::IsNull(pNote1);
    }

    TEST_METHOD(TestUpdateCodeNote)
    {
        GameContextNotifyTarget notifyTarget;

        int nCalls = 0;
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::UpdateCodeNote>([&nCalls](const ra::api::UpdateCodeNote::Request&, ra::api::UpdateCodeNote::Response& response)
        {
            ++nCalls;
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        game.AddNotifyTarget(notifyTarget);

        game.SetGameId(1U);
        Assert::IsTrue(game.SetCodeNote(1234, L"Note1"));

        const auto* pNote1 = game.FindCodeNote(1234U);
        Assert::IsNotNull(pNote1);
        Ensures(pNote1 != nullptr);
        Assert::AreEqual(std::wstring(L"Note1"), *pNote1);

        Assert::IsTrue(game.SetCodeNote(1234, L"Note1b"));
        const auto* pNote1b = game.FindCodeNote(1234U);
        Assert::IsNotNull(pNote1b);
        Ensures(pNote1b != nullptr);
        Assert::AreEqual(std::wstring(L"Note1b"), *pNote1b);

        const auto* pNote1x = notifyTarget.GetNewCodeNote(1234U);
        Assert::IsNotNull(pNote1x);
        Ensures(pNote1x != nullptr);
        Assert::AreEqual(std::wstring(L"Note1b"), *pNote1x);

        Assert::AreEqual(2, nCalls);
    }

    TEST_METHOD(TestDeleteCodeNote)
    {
        GameContextNotifyTarget notifyTarget;

        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::UpdateCodeNote>([](const ra::api::UpdateCodeNote::Request&, ra::api::UpdateCodeNote::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        game.mockServer.HandleRequest<ra::api::DeleteCodeNote>([](const ra::api::DeleteCodeNote::Request& request, ra::api::DeleteCodeNote::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);
            Assert::AreEqual(1234U, request.Address);

            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        game.AddNotifyTarget(notifyTarget);

        game.SetGameId(1U);
        Assert::IsTrue(game.SetCodeNote(1234, L"Note1"));

        const auto* pNote1 = game.FindCodeNote(1234U);
        Assert::IsNotNull(pNote1);
        Ensures(pNote1 != nullptr);
        Assert::AreEqual(std::wstring(L"Note1"), *pNote1);

        Assert::IsTrue(game.DeleteCodeNote(1234));
        const auto* pNote1b = game.FindCodeNote(1234U);
        Assert::IsNull(pNote1b);

        const auto* pNote1x = notifyTarget.GetNewCodeNote(1234U);
        Assert::IsNotNull(pNote1x);
        Ensures(pNote1x != nullptr);
        Assert::AreEqual(std::wstring(), *pNote1x);
    }

    TEST_METHOD(TestDeleteCodeNoteNonExistant)
    {
        GameContextHarness game;
        game.mockServer.ExpectUncalled<ra::api::DeleteCodeNote>();

        game.SetGameId(1U);

        Assert::IsTrue(game.DeleteCodeNote(1234));
        const auto* pNote1 = game.FindCodeNote(1234U);
        Assert::IsNull(pNote1);
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
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            response.Title = L"GameTitle";
            response.ImageIcon = "9743";
            return true;
        });

        Assert::AreEqual(GameContext::Mode::Normal, game.GetMode());

        game.AddNotifyTarget(notifyHarness);
        game.LoadGame(0U);

        Assert::AreEqual(0U, game.GameId());
        Assert::IsFalse(notifyHarness.m_bNotified);

        game.SetMode(GameContext::Mode::CompatibilityTest);
        Assert::AreEqual(GameContext::Mode::CompatibilityTest, game.GetMode());
        Assert::IsFalse(notifyHarness.m_bNotified);

        game.LoadGame(1U, GameContext::Mode::CompatibilityTest);
        Assert::AreEqual(1U, game.GameId());
        Assert::AreEqual(GameContext::Mode::CompatibilityTest, game.GetMode());
        Assert::IsTrue(notifyHarness.m_bNotified);

        notifyHarness.m_bNotified = false;
        game.SetMode(GameContext::Mode::Normal);
        Assert::AreEqual(GameContext::Mode::Normal, game.GetMode());
        Assert::IsTrue(notifyHarness.m_bNotified);
    }
};

} // namespace tests
} // namespace context
} // namespace data
} // namespace ra
