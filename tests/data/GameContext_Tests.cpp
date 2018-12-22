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
public:
    class GameContextHarness : public GameContext
    {
    public:
        ra::api::mocks::MockServer mockServer;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockLocalStorage mockStorage;
        ra::services::mocks::MockThreadPool mockThreadPool;
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

        auto* pAch1 = game.FindAchievement(5U);
        Assert::IsNotNull(pAch1);
        Assert::AreEqual(std::string("Ach1"), pAch1->Title());
        Assert::AreEqual(std::string("Desc1"), pAch1->Description());
        Assert::AreEqual(std::string("Auth1"), pAch1->Author());
        Assert::AreEqual(std::string("12345"), pAch1->BadgeImageURI());
        Assert::AreEqual(3U, pAch1->Category());
        Assert::AreEqual(1234567890, (int)pAch1->CreatedDate());
        Assert::AreEqual(1234599999, (int)pAch1->ModifiedDate());
        Assert::AreEqual(5U, pAch1->Points());
        Assert::AreEqual(std::string("1=1"), pAch1->CreateMemString());

        auto* pAch2 = game.FindAchievement(7U);
        Assert::IsNotNull(pAch2);
        Assert::AreEqual(std::string("Ach2"), pAch2->Title());
        Assert::AreEqual(std::string("Desc2"), pAch2->Description());
        Assert::AreEqual(std::string("Auth2"), pAch2->Author());
        Assert::AreEqual(std::string("12345"), pAch2->BadgeImageURI());
        Assert::AreEqual(5U, pAch2->Category());
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
            "0:1=1:Ach3:Desc3::::Auth3:20:1234511111:1234500000:::555\n"
        );

        game.LoadGame(1U);

        auto* pAch1 = game.FindAchievement(5U);
        Assert::IsNotNull(pAch1);
        Assert::AreEqual(std::string("Ach1"), pAch1->Title());
        Assert::AreEqual(std::string("Desc1"), pAch1->Description());
        Assert::AreEqual(std::string("Auth1"), pAch1->Author());
        Assert::AreEqual(std::string("12345"), pAch1->BadgeImageURI());
        Assert::AreEqual(3U, pAch1->Category());
        Assert::AreEqual(1234567890, (int)pAch1->CreatedDate());
        Assert::AreEqual(1234599999, (int)pAch1->ModifiedDate());
        Assert::AreEqual(5U, pAch1->Points());
        Assert::AreEqual(std::string("1=1"), pAch1->CreateMemString());

        auto* pAch2 = game.FindAchievement(7U);
        Assert::IsNotNull(pAch2);
        Assert::AreEqual(std::string("Ach2b"), pAch2->Title());
        Assert::AreEqual(std::string("Desc2b"), pAch2->Description());
        Assert::AreEqual(std::string("Auth2"), pAch2->Author()); // author not merged
        Assert::AreEqual(std::string("54321"), pAch2->BadgeImageURI());
        Assert::AreEqual(5U, pAch2->Category()); // category not merged
        Assert::AreEqual(1234567890, (int)pAch2->CreatedDate()); // created date not merged
        Assert::AreEqual(1234555555, (int)pAch2->ModifiedDate());
        Assert::AreEqual(25U, pAch2->Points());
        Assert::AreEqual(std::string("1=2"), pAch2->CreateMemString());

        auto* pAch3 = game.FindAchievement(0U);
        Assert::IsNotNull(pAch3);
        Assert::AreEqual(std::string("Ach3"), pAch3->Title());
        Assert::AreEqual(std::string("Desc3"), pAch3->Description());
        Assert::AreEqual(std::string("Auth3"), pAch3->Author());
        Assert::AreEqual(std::string("555"), pAch3->BadgeImageURI());
        Assert::AreEqual(0U, pAch3->Category());
        Assert::AreEqual(1234511111, (int)pAch3->CreatedDate());
        Assert::AreEqual(1234500000, (int)pAch3->ModifiedDate());
        Assert::AreEqual(20U, pAch3->Points());
        Assert::AreEqual(std::string("1=1"), pAch3->CreateMemString());
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

        auto* pAch1 = game.FindAchievement(5U);
        Assert::IsNotNull(pAch1);
        Assert::IsTrue(pAch1->Active());

        auto* pAch2 = game.FindAchievement(7U);
        Assert::IsNotNull(pAch2);
        Assert::IsFalse(pAch2->Active());
    }
};

} // namespace tests
} // namespace data
} // namespace ra
