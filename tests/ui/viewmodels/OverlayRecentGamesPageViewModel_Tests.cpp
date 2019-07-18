#include "CppUnitTest.h"

#include "ui\viewmodels\OverlayRecentGamesPageViewModel.hh"

#include "tests\mocks\MockImageRepository.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockSessionTracker.hh"
#include "tests\mocks\MockThreadPool.hh"
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
        ra::data::mocks::MockSessionTracker mockSessions;
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
    TEST_METHOD(TestRefreshNoGames)
    {
        OverlayRecentGamesPageViewModelHarness gamesPage;

        gamesPage.Refresh();

        Assert::AreEqual(std::wstring(L"Recent Games"), gamesPage.GetTitle());
        Assert::AreEqual(std::wstring(), gamesPage.GetSummary());
        Assert::AreEqual(0U, gamesPage.GetItemCount());
    }

    TEST_METHOD(TestRefreshOneGameDataAvailable)
    {
        OverlayRecentGamesPageViewModelHarness gamesPage;
        gamesPage.mockSessions.MockSession(3U, 1234567890U, std::chrono::seconds(5000));
        gamesPage.MockGame(3U, L"Game Name", "BADGE");

        gamesPage.Refresh();

        Assert::AreEqual(std::wstring(L"Recent Games"), gamesPage.GetTitle());
        Assert::AreEqual(std::wstring(), gamesPage.GetSummary());
        Assert::AreEqual(1U, gamesPage.GetItemCount());

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
        gamesPage.mockSessions.MockSession(3U, 1234567890U, std::chrono::seconds(5000));
        gamesPage.mockServer.HandleRequest<ra::api::FetchGameData>(
            [](const ra::api::FetchGameData::Request& request, ra::api::FetchGameData::Response & response)
        {
            Assert::AreEqual(3U, request.GameId);

            response.Result = ra::api::ApiResult::Success;
            response.Title = L"Game Name";
            response.ImageIcon = "BADGE";
            return true;
        });

        gamesPage.Refresh();

        Assert::AreEqual(std::wstring(L"Recent Games"), gamesPage.GetTitle());
        Assert::AreEqual(std::wstring(), gamesPage.GetSummary());
        Assert::AreEqual(1U, gamesPage.GetItemCount());

        // data is not initially available
        auto* pItem1 = gamesPage.GetItem(0);
        Assert::IsNotNull(pItem1);
        Ensures(pItem1 != nullptr);
        Assert::AreEqual(3, pItem1->GetId());
        Assert::AreEqual(std::wstring(L"Loading - 1h23m"), pItem1->GetLabel());
        Assert::IsTrue(ra::StringStartsWith(pItem1->GetDetail(), std::wstring(L"Last played: Fri 13 Feb 2009 (")));
        Assert::IsTrue(ra::StringEndsWith(pItem1->GetDetail(), std::wstring(L" ago)")));
        Assert::AreEqual(std::string("00000"), pItem1->Image.Name());

        gamesPage.mockTheadPool.ExecuteNextTask(); // scanning cache occurs on background thread

        // data is not in the cache
        pItem1 = gamesPage.GetItem(0);
        Assert::IsNotNull(pItem1);
        Ensures(pItem1 != nullptr);
        Assert::AreEqual(3, pItem1->GetId());
        Assert::AreEqual(std::wstring(L"Loading - 1h23m"), pItem1->GetLabel());
        Assert::IsTrue(ra::StringStartsWith(pItem1->GetDetail(), std::wstring(L"Last played: Fri 13 Feb 2009 (")));
        Assert::IsTrue(ra::StringEndsWith(pItem1->GetDetail(), std::wstring(L" ago)")));
        Assert::AreEqual(std::string("00000"), pItem1->Image.Name());

        gamesPage.mockTheadPool.ExecuteNextTask(); // request is asynchronous

        // data was fetched from the server
        Assert::AreEqual(1U, gamesPage.GetItemCount());
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
        gamesPage.mockSessions.MockSession(3U, 1234567890U, std::chrono::seconds(5000));
        gamesPage.mockServer.ExpectUncalled<ra::api::FetchGameData>();
        gamesPage.mockLocalStorage.MockStoredData(ra::services::StorageItemType::GameData, L"3",
            "{\"Title\":\"Game Name\", \"ConsoleID\":5, \"ImageIcon\":\"/Images/BADGE.png\", \"Achievements\":[], \"Leaderboards\":[]}");

        gamesPage.Refresh();

        Assert::AreEqual(std::wstring(L"Recent Games"), gamesPage.GetTitle());
        Assert::AreEqual(std::wstring(), gamesPage.GetSummary());
        Assert::AreEqual(1U, gamesPage.GetItemCount());

        // data is not initially available
        auto* pItem1 = gamesPage.GetItem(0);
        Assert::IsNotNull(pItem1);
        Ensures(pItem1 != nullptr);
        Assert::AreEqual(3, pItem1->GetId());
        Assert::AreEqual(std::wstring(L"Loading - 1h23m"), pItem1->GetLabel());
        Assert::IsTrue(ra::StringStartsWith(pItem1->GetDetail(), std::wstring(L"Last played: Fri 13 Feb 2009 (")));
        Assert::IsTrue(ra::StringEndsWith(pItem1->GetDetail(), std::wstring(L" ago)")));
        Assert::AreEqual(std::string("00000"), pItem1->Image.Name());

        gamesPage.mockTheadPool.ExecuteNextTask(); // scanning cache occurs on background thread

        // data is in the cache
        Assert::AreEqual(1U, gamesPage.GetItemCount());
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
