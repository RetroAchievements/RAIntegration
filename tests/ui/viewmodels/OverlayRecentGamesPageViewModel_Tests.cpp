#include "CppUnitTest.h"

#include "ui\viewmodels\OverlayRecentGamesPageViewModel.hh"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockHttpRequester.hh"
#include "tests\mocks\MockImageRepository.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockSessionTracker.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(OverlayRecentGamesPageViewModel_Tests)
{
private:
    class OverlayRecentGamesPageViewModelHarness : public OverlayRecentGamesPageViewModel
    {
    public:
        ra::api::mocks::MockServer mockServer;
        ra::data::context::mocks::MockSessionTracker mockSessions;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockLocalStorage mockLocalStorage;
        ra::services::mocks::MockThreadPool mockTheadPool;
        ra::ui::mocks::MockImageRepository mockImageRepository;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;

        ItemViewModel* GetItem(gsl::index nIndex) { return m_vItems.GetItemAt(nIndex); }

        size_t GetItemCount() const noexcept { return m_vItems.Count(); }

        void MockGame(unsigned int nGameId, const std::wstring& sGameName, const std::string& sGameBadge)
        {
            m_mGameNames.emplace(nGameId, sGameName);
            m_mGameBadges.emplace(nGameId, sGameBadge);
        }
    };

public:
    TEST_METHOD(TestNavigationLabels)
    {
        OverlayRecentGamesPageViewModelHarness gamesPage;
        Assert::AreEqual(L"", gamesPage.GetAcceptButtonText());
        Assert::AreEqual(L"Close", gamesPage.GetCancelButtonText());
        Assert::AreEqual(L"Following", gamesPage.GetPrevButtonText());
        Assert::AreEqual(L"Achievements", gamesPage.GetNextButtonText());
    }

    TEST_METHOD(TestRefreshNoGames)
    {
        OverlayRecentGamesPageViewModelHarness gamesPage;

        gamesPage.Refresh();

        Assert::AreEqual(std::wstring(L"Recent Games"), gamesPage.GetTitle());
        Assert::AreEqual(std::wstring(), gamesPage.GetSubTitle());
        Assert::AreEqual(std::wstring(), gamesPage.GetTitleDetail());
        Assert::AreEqual({ 0U }, gamesPage.GetItemCount());
    }

    TEST_METHOD(TestRefreshOneGameDataAvailable)
    {
        OverlayRecentGamesPageViewModelHarness gamesPage;
        gamesPage.mockSessions.MockSession(3U, 1234567890U, std::chrono::seconds(5000));
        gamesPage.MockGame(3U, L"Game Name", "BADGE");

        gamesPage.Refresh();

        Assert::AreEqual(std::wstring(L"Recent Games"), gamesPage.GetTitle());
        Assert::AreEqual(std::wstring(), gamesPage.GetSubTitle());
        Assert::AreEqual(std::wstring(), gamesPage.GetTitleDetail());
        Assert::AreEqual({1U}, gamesPage.GetItemCount());

        const auto* pItem1 = gamesPage.GetItem(0);
        Assert::IsNotNull(pItem1);
        Ensures(pItem1 != nullptr);
        Assert::AreEqual(3, pItem1->GetId());
        Assert::AreEqual(std::wstring(L"Game Name - 1h23m"), pItem1->GetLabel());
        Assert::IsTrue(ra::StringStartsWith(pItem1->GetDetail(), std::wstring(L"Last played: Fri 13 Feb 2009 (")));
        Assert::IsTrue(ra::StringEndsWith(pItem1->GetDetail(), std::wstring(L" ago)")));
        Assert::AreEqual(std::string("BADGE"), pItem1->Image.Name());
    }

    TEST_METHOD(TestRefreshOneGameDataFetched)
    {
        OverlayRecentGamesPageViewModelHarness gamesPage;
        ra::services::mocks::MockAchievementRuntime achievementRuntime;
        gamesPage.mockSessions.MockSession(3U, 1234567890U, std::chrono::seconds(5000));
        gamesPage.mockUserContext.Initialize("Username", "APITOKEN");

        ra::services::mocks::MockHttpRequester mockHttp([](const ra::services::Http::Request&) {
            return ra::services::Http::Response(ra::services::Http::StatusCode::OK,
                                                "{\"Success\":true,\"Response\":["
                                                    "{\"ID\":3,\"Title\":\"Game Name\",\"ImageIcon\":\"/Images/BADGE.png\"}"
                                                "]}");
        });

        gamesPage.Refresh();

        Assert::AreEqual(std::wstring(L"Recent Games"), gamesPage.GetTitle());
        Assert::AreEqual(std::wstring(), gamesPage.GetSubTitle());
        Assert::AreEqual(std::wstring(), gamesPage.GetTitleDetail());
        Assert::AreEqual({1U}, gamesPage.GetItemCount());

        // data is not initially available
        auto* pItem1 = gamesPage.GetItem(0);
        Assert::IsNotNull(pItem1);
        Ensures(pItem1 != nullptr);
        Assert::AreEqual(3, pItem1->GetId());
        Assert::AreEqual(std::wstring(L"Loading - 1h23m"), pItem1->GetLabel());
        Assert::IsTrue(ra::StringStartsWith(pItem1->GetDetail(), std::wstring(L"Last played: Fri 13 Feb 2009 (")));
        Assert::IsTrue(ra::StringEndsWith(pItem1->GetDetail(), std::wstring(L" ago)")));
        Assert::AreEqual(std::string(""), pItem1->Image.Name());

        gamesPage.mockTheadPool.ExecuteNextTask(); // scanning cache occurs on background thread

        // data is not in the cache
        pItem1 = gamesPage.GetItem(0);
        Assert::IsNotNull(pItem1);
        Ensures(pItem1 != nullptr);
        Assert::AreEqual(3, pItem1->GetId());
        Assert::AreEqual(std::wstring(L"Loading - 1h23m"), pItem1->GetLabel());
        Assert::IsTrue(ra::StringStartsWith(pItem1->GetDetail(), std::wstring(L"Last played: Fri 13 Feb 2009 (")));
        Assert::IsTrue(ra::StringEndsWith(pItem1->GetDetail(), std::wstring(L" ago)")));
        Assert::AreEqual(std::string(""), pItem1->Image.Name());

        gamesPage.mockTheadPool.ExecuteNextTask(); // request is asynchronous

        // data was fetched from the server
        Assert::AreEqual({ 1U }, gamesPage.GetItemCount());
        pItem1 = gamesPage.GetItem(0);
        Assert::IsNotNull(pItem1);
        Ensures(pItem1 != nullptr);
        Assert::AreEqual(3, pItem1->GetId());
        Assert::AreEqual(std::wstring(L"Game Name - 1h23m"), pItem1->GetLabel());
        Assert::IsTrue(ra::StringStartsWith(pItem1->GetDetail(), std::wstring(L"Last played: Fri 13 Feb 2009 (")));
        Assert::IsTrue(ra::StringEndsWith(pItem1->GetDetail(), std::wstring(L" ago)")));
        Assert::AreEqual(std::string("BADGE"), pItem1->Image.Name());
    }

    TEST_METHOD(TestRefreshOneGameDataCached)
    {
        OverlayRecentGamesPageViewModelHarness gamesPage;
        gamesPage.mockGameContext.SetGameId(3U);
        gamesPage.mockSessions.MockSession(3U, 1234567890U, std::chrono::seconds(5000));

        bool bHttpRequesterCalled = false;
        ra::services::mocks::MockHttpRequester mockHttp([&bHttpRequesterCalled](const ra::services::Http::Request&) noexcept {
            bHttpRequesterCalled = true;
            return ra::services::Http::Response();
        });

        gamesPage.mockLocalStorage.MockStoredData(ra::services::StorageItemType::GameData, L"3",
            "{\"ID\":99, \"Title\":\"Game Name\", \"ConsoleID\":5, \"ImageIcon\":\"/Images/BADGE.png\", \"Achievements\":[], \"Leaderboards\":[]}");

        gamesPage.Refresh();

        Assert::AreEqual(std::wstring(L"Recent Games"), gamesPage.GetTitle());
        Assert::AreEqual(std::wstring(), gamesPage.GetSubTitle());
        Assert::AreEqual(std::wstring(), gamesPage.GetTitleDetail());
        Assert::AreEqual({1U}, gamesPage.GetItemCount());

        // data is not initially available
        auto* pItem1 = gamesPage.GetItem(0);
        Assert::IsNotNull(pItem1);
        Ensures(pItem1 != nullptr);
        Assert::AreEqual(3, pItem1->GetId());
        Assert::AreEqual(std::wstring(L"Loading - 1h23m"), pItem1->GetLabel());
        Assert::IsTrue(ra::StringStartsWith(pItem1->GetDetail(), std::wstring(L"Last played: Fri 13 Feb 2009 (")));
        Assert::IsTrue(ra::StringEndsWith(pItem1->GetDetail(), std::wstring(L" ago)")));
        Assert::AreEqual(std::string(""), pItem1->Image.Name());

        gamesPage.mockTheadPool.ExecuteNextTask(); // scanning cache occurs on background thread

        // data is in the cache
        Assert::AreEqual({ 1U }, gamesPage.GetItemCount());
        pItem1 = gamesPage.GetItem(0);
        Assert::IsNotNull(pItem1);
        Ensures(pItem1 != nullptr);
        Assert::AreEqual(3, pItem1->GetId());
        Assert::AreEqual(std::wstring(L"Game Name - 1h23m"), pItem1->GetLabel());
        Assert::IsTrue(ra::StringStartsWith(pItem1->GetDetail(), std::wstring(L"Last played: Fri 13 Feb 2009 (")));
        Assert::IsTrue(ra::StringEndsWith(pItem1->GetDetail(), std::wstring(L" ago)")));
        Assert::AreEqual(std::string("BADGE"), pItem1->Image.Name());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
