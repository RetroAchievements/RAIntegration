#include "CppUnitTest.h"

#include "data\GameContext.hh"

#include "services\ILeaderboardManager.hh"

#include "tests\RA_UnitTestHelpers.h"

#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockThreadPool.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::services::StorageItemType;

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(GameContext_Tests)
{
private:
    static void RemoveFirstLine(std::string& sString)
    {
        auto nIndex = sString.find('\n');
        if (nIndex == std::string::npos)
            sString.clear();
        else
            sString.erase(0, nIndex + 1);
    }

public:
    class GameContextHarness : public GameContext
    {
    public:
        ra::api::mocks::MockServer mockServer;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockLocalStorage mockStorage;
        ra::services::mocks::MockThreadPool mockThreadPool;

        static const unsigned int FirstLocalId = GameContext::FirstLocalId;
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
        Assert::AreEqual(std::wstring(L"Game"), game.GameTitle());
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

        const auto* pAch1 = game.FindAchievement(5U);
        Assert::IsNotNull(pAch1);
        Ensures(pAch1 != nullptr);
        Assert::AreEqual(std::string("Ach1"), pAch1->Title());
        Assert::AreEqual(std::string("Desc1"), pAch1->Description());
        Assert::AreEqual(std::string("Auth1"), pAch1->Author());
        Assert::AreEqual(std::string("12345"), pAch1->BadgeImageURI());
        Assert::AreEqual(3, pAch1->Category());
        Assert::AreEqual(1234567890, (int)pAch1->CreatedDate());
        Assert::AreEqual(1234599999, (int)pAch1->ModifiedDate());
        Assert::AreEqual(5U, pAch1->Points());
        Assert::AreEqual(std::string("1=1"), pAch1->CreateMemString());

        const auto* pAch2 = game.FindAchievement(7U);
        Assert::IsNotNull(pAch2);
        Ensures(pAch2 != nullptr);
        Assert::AreEqual(std::string("Ach2"), pAch2->Title());
        Assert::AreEqual(std::string("Desc2"), pAch2->Description());
        Assert::AreEqual(std::string("Auth2"), pAch2->Author());
        Assert::AreEqual(std::string("12345"), pAch2->BadgeImageURI());
        Assert::AreEqual(5, pAch2->Category());
        Assert::AreEqual(1234567890, (int)pAch2->CreatedDate());
        Assert::AreEqual(1234599999, (int)pAch2->ModifiedDate());
        Assert::AreEqual(15U, pAch2->Points());
        Assert::AreEqual(std::string("1=1"), pAch2->CreateMemString());
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

        auto* pAch = game.FindAchievement(5U);
        Assert::IsNotNull(pAch);
        Ensures(pAch != nullptr);
        Assert::AreEqual(std::string("Ach1"), pAch->Title());
        Assert::AreEqual(std::string("Desc1"), pAch->Description());
        Assert::AreEqual(std::string("Auth1"), pAch->Author());
        Assert::AreEqual(std::string("12345"), pAch->BadgeImageURI());
        Assert::AreEqual(3, pAch->Category());
        Assert::AreEqual(1234567890, (int)pAch->CreatedDate());
        Assert::AreEqual(1234599999, (int)pAch->ModifiedDate());
        Assert::AreEqual(5U, pAch->Points());
        Assert::AreEqual(std::string("1=1"), pAch->CreateMemString());

        // local achievement data for 7 should be merged with server achievement data
        pAch = game.FindAchievement(7U);
        Assert::IsNotNull(pAch);
        Assert::AreEqual(std::string("Ach2b"), pAch->Title());
        Assert::AreEqual(std::string("Desc2b"), pAch->Description());
        Assert::AreEqual(std::string("Auth2"), pAch->Author()); // author not merged
        Assert::AreEqual(std::string("54321"), pAch->BadgeImageURI());
        Assert::AreEqual(5, pAch->Category()); // category not merged
        Assert::AreEqual(1234567890, (int)pAch->CreatedDate()); // created date not merged
        Assert::AreEqual(1234555555, (int)pAch->ModifiedDate());
        Assert::AreEqual(25U, pAch->Points());
        Assert::AreEqual(std::string("1=2"), pAch->CreateMemString());

        // no server achievement, assign FirstLocalId
        pAch = game.FindAchievement(GameContextHarness::FirstLocalId);
        Assert::IsNotNull(pAch);
        Assert::AreEqual(std::string("Ach3"), pAch->Title());
        Assert::AreEqual(std::string("Desc3"), pAch->Description());
        Assert::AreEqual(std::string("Auth3"), pAch->Author());
        Assert::AreEqual(std::string("555"), pAch->BadgeImageURI());
        Assert::AreEqual(0, pAch->Category());
        Assert::AreEqual(1234511111, (int)pAch->CreatedDate());
        Assert::AreEqual(1234500000, (int)pAch->ModifiedDate());
        Assert::AreEqual(20U, pAch->Points());
        Assert::AreEqual(std::string("1=1"), pAch->CreateMemString());

        // no server achievement, assign next local id
        pAch = game.FindAchievement(GameContextHarness::FirstLocalId + 1);
        Assert::IsNotNull(pAch);
        Assert::AreEqual(std::string("Ach4"), pAch->Title());
        Assert::AreEqual(std::string("Desc4"), pAch->Description());
        Assert::AreEqual(std::string("Auth4"), pAch->Author());
        Assert::AreEqual(std::string("556"), pAch->BadgeImageURI());
        Assert::AreEqual(0, pAch->Category());
        Assert::AreEqual(1234511111, (int)pAch->CreatedDate());
        Assert::AreEqual(1234500000, (int)pAch->ModifiedDate());
        Assert::AreEqual(10U, pAch->Points());
        Assert::AreEqual(std::string("R:1=1"), pAch->CreateMemString());

        // new achievement should be allocated an ID higher than the largest existing local ID
        const auto& pAch2 = game.NewAchievement(AchievementSet::Type::Local);
        Assert::AreEqual(GameContextHarness::FirstLocalId + 2, pAch2.ID());
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

        // 7 is not a known ID for this game, it should be loaded into a local achievement (we'll check later)
        auto* pAch = game.FindAchievement(7U);
        Assert::IsNotNull(pAch);
        Ensures(pAch != nullptr);
        Assert::AreEqual(std::string("Ach2b"), pAch->Title());
        Assert::AreEqual(std::string("Desc2b"), pAch->Description());
        Assert::AreEqual(std::string("Auth2b"), pAch->Author());
        Assert::AreEqual(std::string("54321"), pAch->BadgeImageURI());
        Assert::AreEqual(0, pAch->Category()); // local
        Assert::AreEqual(1234554321, (int)pAch->CreatedDate());
        Assert::AreEqual(1234555555, (int)pAch->ModifiedDate());
        Assert::AreEqual(25U, pAch->Points());
        Assert::AreEqual(std::string("1=2"), pAch->CreateMemString());

        // explicit ID should be recognized
        pAch = game.FindAchievement(999000001U);
        Assert::IsNotNull(pAch);
        Ensures(pAch != nullptr);
        Assert::AreEqual(std::string("Ach3"), pAch->Title());
        Assert::AreEqual(std::string("Desc3"), pAch->Description());
        Assert::AreEqual(std::string("Auth3"), pAch->Author());
        Assert::AreEqual(std::string("555"), pAch->BadgeImageURI());
        Assert::AreEqual(0, pAch->Category());
        Assert::AreEqual(1234511111, (int)pAch->CreatedDate());
        Assert::AreEqual(1234500000, (int)pAch->ModifiedDate());
        Assert::AreEqual(20U, pAch->Points());
        Assert::AreEqual(std::string("1=1"), pAch->CreateMemString());

        // explicit ID should be recognized
        pAch = game.FindAchievement(999000003U);
        Assert::IsNotNull(pAch);
        Ensures(pAch != nullptr);
        Assert::AreEqual(std::string("Ach4"), pAch->Title());
        Assert::AreEqual(std::string("Desc4"), pAch->Description());
        Assert::AreEqual(std::string("Auth4"), pAch->Author());
        Assert::AreEqual(std::string("556"), pAch->BadgeImageURI());
        Assert::AreEqual(0, pAch->Category());
        Assert::AreEqual(1234511111, (int)pAch->CreatedDate());
        Assert::AreEqual(1234500000, (int)pAch->ModifiedDate());
        Assert::AreEqual(10U, pAch->Points());
        Assert::AreEqual(std::string("1=1"), pAch->CreateMemString());

        // new achievement should be allocated an ID higher than the largest existing local
        // ID, even if intermediate values are available
        const auto& pAch2 = game.NewAchievement(AchievementSet::Type::Local);
        Assert::AreEqual(999000004U, pAch2.ID());
    }

    TEST_METHOD(TestSaveLocalEmpty)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            response.Title = L"GameTitle";
            response.Result = ra::api::ApiResult::Success;
            return true;
        });
        game.LoadGame(1U);
        Assert::IsTrue(game.SaveLocal());
        auto sContents = game.mockStorage.GetStoredData(ra::services::StorageItemType::UserAchievements, L"1");
        RemoveFirstLine(sContents);
        Assert::AreEqual(std::string("GameTitle\n"), sContents);
    }

    TEST_METHOD(TestSaveLocalOnlyLocal)
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

            response.Title = L"GameTitle";
            response.Result = ra::api::ApiResult::Success;
            return true;
        });
        game.LoadGame(1U);

        auto& ach2 = game.NewAchievement(AchievementSet::Type::Local);
        ach2.SetTitle("Ach2");
        ach2.SetDescription("Desc2");
        ach2.SetAuthor("Auth2");
        ach2.SetBadgeImage("54321");
        ach2.SetCreatedDate(1234567891);
        ach2.SetModifiedDate(1234599998);
        ach2.ParseTrigger("2=2");
        ach2.SetPoints(15);

        Assert::IsTrue(game.SaveLocal());
        auto sContents = game.mockStorage.GetStoredData(ra::services::StorageItemType::UserAchievements, L"1");
        RemoveFirstLine(sContents);
        Assert::AreEqual(ra::StringPrintf("GameTitle\n%u:\"2=2\":Ach2:Desc2::::Auth2:15:1234567891:1234599998:::54321\n", game.FirstLocalId), sContents);
    }

    TEST_METHOD(TestSaveLocalEscaped)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            response.Title = L"GameTitle";
            response.Result = ra::api::ApiResult::Success;
            return true;
        });
        game.LoadGame(1U);

        auto& ach2 = game.NewAchievement(AchievementSet::Type::Local);
        ach2.SetTitle("Ach:2");
        ach2.SetDescription("Desc \"2\"");
        ach2.SetAuthor("Auth2");
        ach2.SetBadgeImage("54321");
        ach2.SetCreatedDate(1234567891);
        ach2.SetModifiedDate(1234599998);
        ach2.ParseTrigger("2=2");
        ach2.SetPoints(15);

        Assert::IsTrue(game.SaveLocal());
        auto sContents = game.mockStorage.GetStoredData(ra::services::StorageItemType::UserAchievements, L"1");
        RemoveFirstLine(sContents);
        Assert::AreEqual(ra::StringPrintf("GameTitle\n%u:\"2=2\":\"Ach:2\":\"Desc \\\"2\\\"\"::::Auth2:15:1234567891:1234599998:::54321\n", game.FirstLocalId), sContents);
    }

    TEST_METHOD(TestLoadGameUserUnlocks)
    {
        GameContextHarness game;
        game.mockServer.HandleRequest<ra::api::FetchGameData>([](const ra::api::FetchGameData::Request&, ra::api::FetchGameData::Response& response)
        {
            auto& ach1 = response.Achievements.emplace_back();
            ach1.Id = 5;
            ach1.CategoryId = ra::etoi(AchievementSet::Type::Core);

            auto& ach2 = response.Achievements.emplace_back();
            ach2.Id = 7;
            ach1.CategoryId = ra::etoi(AchievementSet::Type::Core);
            return true;
        });

        game.mockServer.HandleRequest<ra::api::FetchUserUnlocks>([](const ra::api::FetchUserUnlocks::Request& request, ra::api::FetchUserUnlocks::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);
            Assert::IsFalse(request.Hardcore);

            response.UnlockedAchievements.insert(7U);
            return true;
        });

        game.LoadGame(1U);
        game.mockThreadPool.ExecuteNextTask(); // FetchUserUnlocks is async

        const auto* pAch1 = game.FindAchievement(5U);
        Assert::IsNotNull(pAch1);
        Ensures(pAch1 != nullptr);
        Assert::IsTrue(pAch1->Active());

        const auto* pAch2 = game.FindAchievement(7U);
        Assert::IsNotNull(pAch2);
        Ensures(pAch2 != nullptr);
        Assert::IsFalse(pAch2->Active());
    }
};

} // namespace tests
} // namespace data
} // namespace ra
