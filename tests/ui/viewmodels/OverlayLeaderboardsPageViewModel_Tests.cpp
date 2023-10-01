#include "CppUnitTest.h"

#include "ui\viewmodels\OverlayLeaderboardsPageViewModel.hh"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockImageRepository.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"
#include "tests\RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(OverlayLeaderboardsPageViewModel_Tests)
{
private:
    class OverlayLeaderboardsPageViewModelHarness : public OverlayLeaderboardsPageViewModel
    {
    public:
        ra::api::mocks::MockServer mockServer;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockAchievementRuntime mockAchievementRuntime;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockImageRepository mockImageRepository;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        OverlayLeaderboardsPageViewModelHarness()
        {
            mockGameContext.SetGameTitle(L"Game Title");
        }

        ItemViewModel* GetItem(gsl::index nIndex) { return m_vItems.GetItemAt(nIndex); }

        void TestFetchItemDetail(gsl::index nIndex)
        {
            auto* pItem = GetItem(nIndex);
            Expects(pItem != nullptr);
            OverlayLeaderboardsPageViewModel::FetchItemDetail(*pItem);
        }

        ViewModelCollection<ItemViewModel>* GetRanks(ra::LeaderboardID nId)
        {
            const auto pIter = m_vLeaderboardRanks.find(nId);
            if (pIter != m_vLeaderboardRanks.end())
                return &pIter->second;

            return nullptr;
        }

        void SetAssetFilter(ra::ui::viewmodels::AssetListViewModel::FilterCategory nCategory)
        {
            auto& pAssetList = ra::services::ServiceLocator::GetMutable<ra::ui::viewmodels::WindowManager>().AssetList;
            pAssetList.SetAssetTypeFilter(ra::data::models::AssetType::Leaderboard);
            pAssetList.SetFilterCategory(nCategory);
        }

        void AssertHeader(gsl::index nIndex, const std::wstring& sExpectedLabel)
        {
            const auto* pItem = GetItem(nIndex);
            Expects(pItem != nullptr);
            Assert::IsTrue(pItem->IsHeader());
            Assert::AreEqual(sExpectedLabel, pItem->GetLabel());
            Assert::IsFalse(pItem->IsDisabled());
        }

        void AssertLeaderboard(gsl::index nIndex,
                               const rc_client_leaderboard_info_t* pLeaderboard,
                               const std::wstring& sTrackerValue = L"")
        {
            const auto* pItem = GetItem(nIndex);
            Expects(pItem != nullptr);
            Assert::IsFalse(pItem->IsHeader());
            Assert::AreEqual(pLeaderboard->public_.id, static_cast<uint32_t>(pItem->GetId()));

            Assert::AreEqual(ra::Widen(pLeaderboard->public_.title), pItem->GetLabel());
            Assert::AreEqual(ra::Widen(pLeaderboard->public_.description), pItem->GetDetail());
            Assert::AreEqual(sTrackerValue, pItem->GetProgressString());
            if (sTrackerValue.empty())
                Assert::AreEqual(0.0f, pItem->GetProgressPercentage());
            else
                Assert::AreEqual(-1.0f, pItem->GetProgressPercentage());
        }
    };

public:
    TEST_METHOD(TestRefreshNoLeaderboards)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        leaderboardsPage.SetCanCollapseHeaders(false);
        leaderboardsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), leaderboardsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"No leaderboards present"), leaderboardsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(), leaderboardsPage.GetTitleDetail());
        Assert::IsNull(leaderboardsPage.GetItem(0));
    }

    TEST_METHOD(TestRefreshLeaderboards)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        leaderboardsPage.mockAchievementRuntime.MockGame();

        auto* pLbd1 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(1);
        auto* pLbd2 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(2);

        leaderboardsPage.SetCanCollapseHeaders(false);
        leaderboardsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), leaderboardsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"2 leaderboards present"), leaderboardsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(), leaderboardsPage.GetTitleDetail());

        leaderboardsPage.AssertHeader(0, L"Inactive");
        leaderboardsPage.AssertLeaderboard(1, pLbd1);
        leaderboardsPage.AssertLeaderboard(2, pLbd2);
    }

    TEST_METHOD(TestRefreshCoreOnly)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        leaderboardsPage.mockAchievementRuntime.MockGame();

        // only 1 and 3 will be visible - 2 is not core, and filtered out by asset list
        auto* pLbd1 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(1);
                      leaderboardsPage.mockAchievementRuntime.MockLocalLeaderboard(2);
        auto* pLbd3 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(3);

        leaderboardsPage.SetCanCollapseHeaders(false);
        leaderboardsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), leaderboardsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"2 leaderboards present"), leaderboardsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(), leaderboardsPage.GetTitleDetail());

        leaderboardsPage.AssertHeader(0, L"Inactive");
        leaderboardsPage.AssertLeaderboard(1, pLbd1);
        leaderboardsPage.AssertLeaderboard(2, pLbd3);
        Assert::IsNull(leaderboardsPage.GetItem(3));
    }

    TEST_METHOD(TestRefreshCoreAndLocal)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        leaderboardsPage.mockAchievementRuntime.MockGame();

        auto* pLbd1 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(1);
        auto* pLbd2 = leaderboardsPage.mockAchievementRuntime.MockLocalLeaderboard(2);
        auto* pLbd3 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(3);

        leaderboardsPage.SetAssetFilter(ra::ui::viewmodels::AssetListViewModel::FilterCategory::Local);
        leaderboardsPage.SetCanCollapseHeaders(false);
        leaderboardsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), leaderboardsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"3 leaderboards present"), leaderboardsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(), leaderboardsPage.GetTitleDetail());

        leaderboardsPage.AssertHeader(0, L"Game Title - Inactive");
        leaderboardsPage.AssertLeaderboard(1, pLbd1);
        leaderboardsPage.AssertLeaderboard(2, pLbd3);
        leaderboardsPage.AssertHeader(3, L"Local - Inactive");
        leaderboardsPage.AssertLeaderboard(4, pLbd2);
        Assert::IsNull(leaderboardsPage.GetItem(5));
    }

    TEST_METHOD(TestRefreshWithHidden)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        leaderboardsPage.mockAchievementRuntime.MockGame();

        // only 1 and 3 will be visible - 2 is hidden
        auto* pLbd1 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(1);
        auto* pLbd2 = leaderboardsPage.mockAchievementRuntime.MockLocalLeaderboard(2);
        pLbd2->hidden = 1;
        auto* pLbd3 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(3);

        leaderboardsPage.SetCanCollapseHeaders(false);
        leaderboardsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), leaderboardsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"2 leaderboards present"), leaderboardsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(), leaderboardsPage.GetTitleDetail());

        leaderboardsPage.AssertHeader(0, L"Inactive");
        leaderboardsPage.AssertLeaderboard(1, pLbd1);
        leaderboardsPage.AssertLeaderboard(2, pLbd3);
        Assert::IsNull(leaderboardsPage.GetItem(3));
    }

    TEST_METHOD(TestRefreshWithActive)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        leaderboardsPage.mockAchievementRuntime.MockGame();

        auto* pLbd1 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(1);
        auto* pLbd2 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(2);
        pLbd2->public_.state = RC_CLIENT_LEADERBOARD_STATE_TRACKING;
        pLbd2->public_.tracker_value = "1:23.00";
        auto* pLbd3 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(3);

        leaderboardsPage.SetCanCollapseHeaders(false);
        leaderboardsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), leaderboardsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"3 leaderboards present"), leaderboardsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(), leaderboardsPage.GetTitleDetail());

        leaderboardsPage.AssertHeader(0, L"Active");
        leaderboardsPage.AssertLeaderboard(1, pLbd2, L"1:23.00");
        leaderboardsPage.AssertHeader(2, L"Inactive");
        leaderboardsPage.AssertLeaderboard(3, pLbd1);
        leaderboardsPage.AssertLeaderboard(4, pLbd3);
        Assert::IsNull(leaderboardsPage.GetItem(5));
    }

    TEST_METHOD(TestRefreshWithHiddenActive)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        leaderboardsPage.mockAchievementRuntime.MockGame();

        // only 1 and 3 will be visible - 2 is hidden (TODO: should it be?)
        auto* pLbd1 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(1);
        auto* pLbd2 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(2);
        pLbd2->hidden = 1;
        pLbd2->public_.state = RC_CLIENT_LEADERBOARD_STATE_TRACKING;
        pLbd2->public_.tracker_value = "1:23.00";
        auto* pLbd3 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(3);

        leaderboardsPage.SetCanCollapseHeaders(false);
        leaderboardsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), leaderboardsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"2 leaderboards present"), leaderboardsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(), leaderboardsPage.GetTitleDetail());

        leaderboardsPage.AssertHeader(0, L"Inactive");
        leaderboardsPage.AssertLeaderboard(1, pLbd1);
        leaderboardsPage.AssertLeaderboard(2, pLbd3);
        Assert::IsNull(leaderboardsPage.GetItem(3));
    }

    TEST_METHOD(TestRefreshWithDisabled)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        leaderboardsPage.mockAchievementRuntime.MockGame();

        auto* pLbd1 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(1);
        auto* pLbd2 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(2);
        pLbd2->public_.state = RC_CLIENT_LEADERBOARD_STATE_DISABLED;
        auto* pLbd3 = leaderboardsPage.mockAchievementRuntime.MockLeaderboard(3);

        leaderboardsPage.SetAssetFilter(ra::ui::viewmodels::AssetListViewModel::FilterCategory::Local);
        leaderboardsPage.SetCanCollapseHeaders(false);
        leaderboardsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), leaderboardsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"3 leaderboards present"), leaderboardsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(), leaderboardsPage.GetTitleDetail());

        leaderboardsPage.AssertHeader(0, L"Inactive");
        leaderboardsPage.AssertLeaderboard(1, pLbd1);
        leaderboardsPage.AssertLeaderboard(2, pLbd3);
        leaderboardsPage.AssertHeader(3, L"Unsupported");
        leaderboardsPage.AssertLeaderboard(4, pLbd2);
        Assert::IsNull(leaderboardsPage.GetItem(5));
    }

    TEST_METHOD(TestFetchItemDetail)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        leaderboardsPage.mockAchievementRuntime.MockGame();
        leaderboardsPage.mockAchievementRuntime.MockLeaderboard(1);

        leaderboardsPage.mockUserContext.Initialize("user2", "User2", "ApiToken");
        leaderboardsPage.mockServer.HandleRequest<ra::api::FetchLeaderboardInfo>([](const ra::api::FetchLeaderboardInfo::Request& request, ra::api::FetchLeaderboardInfo::Response& response)
        {
            Assert::AreEqual(1U, request.LeaderboardId);

            response.Result = ra::api::ApiResult::Success;
            response.Entries.emplace_back(ra::api::FetchLeaderboardInfo::Response::Entry{ 1U, "User1", 1234, 123456789 });
            response.Entries.emplace_back(ra::api::FetchLeaderboardInfo::Response::Entry{ 2U, "User2", 6789, 123456789 });
            response.Entries.emplace_back(ra::api::FetchLeaderboardInfo::Response::Entry{ 3U, "User3", 1, 123456789 });

            return true;
        });

        leaderboardsPage.Refresh();
        leaderboardsPage.TestFetchItemDetail(1);

        auto* pDetail = leaderboardsPage.GetRanks(1);
        Expects(pDetail != nullptr);
        Assert::AreEqual({ 0U }, pDetail->Count());

        leaderboardsPage.mockThreadPool.ExecuteNextTask();
        pDetail = leaderboardsPage.GetRanks(1);
        Expects(pDetail != nullptr);

        Assert::AreEqual({ 3U }, pDetail->Count());
        auto* pItem = pDetail->GetItemAt(0);
        Expects(pItem != nullptr);
        Assert::AreEqual(std::wstring(L"User1"), pItem->GetLabel());
        Assert::IsFalse(pItem->GetDetail().empty());
        Assert::IsFalse(pItem->IsDisabled());

        pItem = pDetail->GetItemAt(1);
        Expects(pItem != nullptr);
        Assert::AreEqual(std::wstring(L"User2"), pItem->GetLabel());
        Assert::IsFalse(pItem->GetDetail().empty());
        Assert::IsTrue(pItem->IsDisabled()); // current user should be highlighted

        pItem = pDetail->GetItemAt(2);
        Expects(pItem != nullptr);
        Assert::AreEqual(std::wstring(L"User3"), pItem->GetLabel());
        Assert::IsFalse(pItem->GetDetail().empty());
        Assert::IsFalse(pItem->IsDisabled());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
