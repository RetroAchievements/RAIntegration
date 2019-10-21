#include "CppUnitTest.h"

#include "ui\viewmodels\OverlayLeaderboardsPageViewModel.hh"

#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockImageRepository.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"
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
        ra::data::mocks::MockGameContext mockGameContext;
        ra::data::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockImageRepository mockImageRepository;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;

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
    };

public:
    TEST_METHOD(TestRefreshNoLeaderboards)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        leaderboardsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Leaderboards"), leaderboardsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"No leaderboards present"), leaderboardsPage.GetSummary());
    }

    TEST_METHOD(TestRefreshLeaderboards)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        auto& pLB1 = leaderboardsPage.mockGameContext.NewLeaderboard(1U);
        pLB1.SetTitle("LeaderboardTitle");
        pLB1.SetDescription("Trigger this");
        auto& pLB2 = leaderboardsPage.mockGameContext.NewLeaderboard(2U);
        pLB2.SetTitle("LeaderboardTitle2");
        pLB2.SetDescription("Trigger this too");
        leaderboardsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Leaderboards"), leaderboardsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"2 leaderboards present"), leaderboardsPage.GetSummary());

        auto* pItem = leaderboardsPage.GetItem(0);
        Expects(pItem != nullptr);
        Assert::AreEqual(1, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Trigger this"), pItem->GetDetail());

        pItem = leaderboardsPage.GetItem(1);
        Expects(pItem != nullptr);
        Assert::AreEqual(2, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle2"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Trigger this too"), pItem->GetDetail());
    }

    TEST_METHOD(TestFetchItemDetail)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        auto& pLB1 = leaderboardsPage.mockGameContext.NewLeaderboard(1U);
        pLB1.SetTitle("LeaderboardTitle");
        pLB1.SetDescription("Trigger this");

        leaderboardsPage.mockUserContext.Initialize("User2", "ApiToken");
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
        leaderboardsPage.TestFetchItemDetail(0);

        auto* pDetail = leaderboardsPage.GetRanks(1);
        Expects(pDetail != nullptr);
        Assert::AreEqual(0U, pDetail->Count());

        leaderboardsPage.mockThreadPool.ExecuteNextTask();
        pDetail = leaderboardsPage.GetRanks(1);
        Expects(pDetail != nullptr);

        Assert::AreEqual(3U, pDetail->Count());
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
