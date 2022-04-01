#include "CppUnitTest.h"

#include "ui\viewmodels\OverlayLeaderboardsPageViewModel.hh"

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
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockImageRepository mockImageRepository;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

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
        leaderboardsPage.SetAssetFilter(ra::ui::viewmodels::AssetListViewModel::FilterCategory::All);
        auto& pLB1 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB1.SetID(1U);
        pLB1.SetName(L"LeaderboardTitle");
        pLB1.SetDescription(L"Trigger this");
        auto& pLB2 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB2.SetID(2U);
        pLB2.SetName(L"LeaderboardTitle2");
        pLB2.SetDescription(L"Trigger this too");
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

    TEST_METHOD(TestRefreshCoreOnly)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        auto& pLB1 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB1.SetID(1U);
        pLB1.SetName(L"LeaderboardTitle");
        pLB1.SetDescription(L"Capture this");
        auto& pLB2 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB2.SetID(2U);
        pLB2.SetName(L"LeaderboardTitle2");
        pLB2.SetDescription(L"Local Leaderboard");
        pLB2.SetCategory(ra::data::models::AssetCategory::Local);
        auto& pLB3 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB3.SetID(3U);
        pLB3.SetName(L"LeaderboardTitle3");
        pLB3.SetDescription(L"Capture this too");
        leaderboardsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Leaderboards"), leaderboardsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"2 leaderboards present"), leaderboardsPage.GetSummary());

        // only 1 and 3 will be visible - 2 is not core, and filtered out by asset list
        auto* pItem = leaderboardsPage.GetItem(0);
        Expects(pItem != nullptr);
        Assert::AreEqual(1, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Capture this"), pItem->GetDetail());

        pItem = leaderboardsPage.GetItem(1);
        Expects(pItem != nullptr);
        Assert::AreEqual(3, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle3"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Capture this too"), pItem->GetDetail());

        Assert::IsNull(leaderboardsPage.GetItem(3));
    }

    TEST_METHOD(TestRefreshCoreAndLocal)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        auto& pLB1 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB1.SetID(1U);
        pLB1.SetName(L"LeaderboardTitle");
        pLB1.SetDescription(L"Capture this");
        auto& pLB2 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB2.SetID(2U);
        pLB2.SetName(L"LeaderboardTitle2");
        pLB2.SetDescription(L"Local Leaderboard");
        pLB2.SetCategory(ra::data::models::AssetCategory::Local);
        auto& pLB3 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB3.SetID(3U);
        pLB3.SetName(L"LeaderboardTitle3");
        pLB3.SetDescription(L"Capture this too");
        leaderboardsPage.mockWindowManager.AssetList.SetAssetTypeFilter(ra::data::models::AssetType::Leaderboard);
        leaderboardsPage.mockWindowManager.AssetList.SetFilterCategory(ra::ui::viewmodels::AssetListViewModel::FilterCategory::All);
        leaderboardsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Leaderboards"), leaderboardsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"3 leaderboards present"), leaderboardsPage.GetSummary());

        auto* pItem = leaderboardsPage.GetItem(0);
        Expects(pItem != nullptr);
        Assert::IsTrue(pItem->IsHeader());
        Assert::AreEqual(std::wstring(L"Inactive Local Leaderboards"), pItem->GetLabel());
        Assert::IsFalse(pItem->IsDisabled());

        pItem = leaderboardsPage.GetItem(1);
        Expects(pItem != nullptr);
        Assert::AreEqual(2, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle2"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Local Leaderboard"), pItem->GetDetail());

        pItem = leaderboardsPage.GetItem(2);
        Expects(pItem != nullptr);
        Assert::IsTrue(pItem->IsHeader());
        Assert::AreEqual(std::wstring(L"Inactive Leaderboards"), pItem->GetLabel());
        Assert::IsFalse(pItem->IsDisabled());

        pItem = leaderboardsPage.GetItem(3);
        Expects(pItem != nullptr);
        Assert::AreEqual(1, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Capture this"), pItem->GetDetail());

        pItem = leaderboardsPage.GetItem(4);
        Expects(pItem != nullptr);
        Assert::AreEqual(3, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle3"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Capture this too"), pItem->GetDetail());

        Assert::IsNull(leaderboardsPage.GetItem(5));
    }

    TEST_METHOD(TestRefreshWithHidden)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        auto& pLB1 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB1.SetID(1U);
        pLB1.SetName(L"LeaderboardTitle");
        pLB1.SetDescription(L"Capture this");
        auto& pLB2 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB2.SetID(2U);
        pLB2.SetName(L"LeaderboardTitle2");
        pLB2.SetDescription(L"Hidden Leaderboard");
        pLB2.SetHidden(true);
        auto& pLB3 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB3.SetID(3U);
        pLB3.SetName(L"LeaderboardTitle3");
        pLB3.SetDescription(L"Capture this too");
        leaderboardsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Leaderboards"), leaderboardsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"2 leaderboards present"), leaderboardsPage.GetSummary());

        // only 1 and 3 will be visible - 2 is not core, and filtered out by asset list
        auto* pItem = leaderboardsPage.GetItem(0);
        Expects(pItem != nullptr);
        Assert::AreEqual(1, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Capture this"), pItem->GetDetail());

        pItem = leaderboardsPage.GetItem(1);
        Expects(pItem != nullptr);
        Assert::AreEqual(3, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle3"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Capture this too"), pItem->GetDetail());

        Assert::IsNull(leaderboardsPage.GetItem(3));
    }

    TEST_METHOD(TestRefreshWithActive)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        auto& pLB1 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB1.SetID(1U);
        pLB1.SetName(L"LeaderboardTitle");
        pLB1.SetDescription(L"Capture this");
        auto& pLB2 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB2.SetID(2U);
        pLB2.SetName(L"LeaderboardTitle2");
        pLB2.SetDescription(L"Another Capture");
        pLB2.SetState(ra::data::models::AssetState::Primed);
        auto& pLB3 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB3.SetID(3U);
        pLB3.SetName(L"LeaderboardTitle3");
        pLB3.SetDescription(L"Capture this too");
        pLB3.SetState(ra::data::models::AssetState::Primed);
        leaderboardsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Leaderboards"), leaderboardsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"3 leaderboards present"), leaderboardsPage.GetSummary());

        auto* pItem = leaderboardsPage.GetItem(0);
        Expects(pItem != nullptr);
        Assert::IsTrue(pItem->IsHeader());
        Assert::AreEqual(std::wstring(L"Active Leaderboards"), pItem->GetLabel());
        Assert::IsFalse(pItem->IsDisabled());

        pItem = leaderboardsPage.GetItem(1);
        Expects(pItem != nullptr);
        Assert::AreEqual(2, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle2"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Another Capture"), pItem->GetDetail());

        pItem = leaderboardsPage.GetItem(2);
        Expects(pItem != nullptr);
        Assert::AreEqual(3, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle3"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Capture this too"), pItem->GetDetail());

        pItem = leaderboardsPage.GetItem(3);
        Expects(pItem != nullptr);
        Assert::IsTrue(pItem->IsHeader());
        Assert::AreEqual(std::wstring(L"Inactive Leaderboards"), pItem->GetLabel());
        Assert::IsFalse(pItem->IsDisabled());

        pItem = leaderboardsPage.GetItem(4);
        Expects(pItem != nullptr);
        Assert::AreEqual(1, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Capture this"), pItem->GetDetail());

        Assert::IsNull(leaderboardsPage.GetItem(5));
    }

    TEST_METHOD(TestRefreshWithHiddenActive)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        auto& pLB1 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB1.SetID(1U);
        pLB1.SetName(L"LeaderboardTitle");
        pLB1.SetDescription(L"Capture this");
        auto& pLB2 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB2.SetID(2U);
        pLB2.SetName(L"LeaderboardTitle2");
        pLB2.SetDescription(L"Another Capture");
        pLB2.SetState(ra::data::models::AssetState::Primed);
        pLB2.SetHidden(true);
        auto& pLB3 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB3.SetID(3U);
        pLB3.SetName(L"LeaderboardTitle3");
        pLB3.SetDescription(L"Capture this too");
        pLB3.SetState(ra::data::models::AssetState::Primed);
        leaderboardsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Leaderboards"), leaderboardsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"2 leaderboards present"), leaderboardsPage.GetSummary());

        auto* pItem = leaderboardsPage.GetItem(0);
        Expects(pItem != nullptr);
        Assert::IsTrue(pItem->IsHeader());
        Assert::AreEqual(std::wstring(L"Active Leaderboards"), pItem->GetLabel());
        Assert::IsFalse(pItem->IsDisabled());

        pItem = leaderboardsPage.GetItem(1);
        Expects(pItem != nullptr);
        Assert::AreEqual(3, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle3"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Capture this too"), pItem->GetDetail());

        pItem = leaderboardsPage.GetItem(2);
        Expects(pItem != nullptr);
        Assert::IsTrue(pItem->IsHeader());
        Assert::AreEqual(std::wstring(L"Inactive Leaderboards"), pItem->GetLabel());
        Assert::IsFalse(pItem->IsDisabled());

        pItem = leaderboardsPage.GetItem(3);
        Expects(pItem != nullptr);
        Assert::AreEqual(1, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Capture this"), pItem->GetDetail());

        Assert::IsNull(leaderboardsPage.GetItem(4));
    }

    TEST_METHOD(TestRefreshWithDisabled)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        auto& pLB1 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB1.SetID(1U);
        pLB1.SetName(L"LeaderboardTitle");
        pLB1.SetDescription(L"Capture this");
        auto& pLB2 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB2.SetID(2U);
        pLB2.SetName(L"LeaderboardTitle2");
        pLB2.SetDescription(L"Disabled Leaderboard");
        pLB2.SetState(ra::data::models::AssetState::Disabled);
        auto& pLB3 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB3.SetID(3U);
        pLB3.SetName(L"LeaderboardTitle3");
        pLB3.SetDescription(L"Capture this too");
        leaderboardsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Leaderboards"), leaderboardsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"3 leaderboards present"), leaderboardsPage.GetSummary());

        auto* pItem = leaderboardsPage.GetItem(0);
        Expects(pItem != nullptr);
        Assert::IsTrue(pItem->IsHeader());
        Assert::AreEqual(std::wstring(L"Inactive Leaderboards"), pItem->GetLabel());
        Assert::IsFalse(pItem->IsDisabled());

        pItem = leaderboardsPage.GetItem(1);
        Expects(pItem != nullptr);
        Assert::AreEqual(1, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Capture this"), pItem->GetDetail());

        pItem = leaderboardsPage.GetItem(2);
        Expects(pItem != nullptr);
        Assert::AreEqual(3, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle3"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Capture this too"), pItem->GetDetail());

        pItem = leaderboardsPage.GetItem(3);
        Expects(pItem != nullptr);
        Assert::IsTrue(pItem->IsHeader());
        Assert::AreEqual(std::wstring(L"Unsupported Leaderboards"), pItem->GetLabel());
        Assert::IsFalse(pItem->IsDisabled());

        pItem = leaderboardsPage.GetItem(4);
        Expects(pItem != nullptr);
        Assert::AreEqual(2, pItem->GetId());
        Assert::AreEqual(std::wstring(L"LeaderboardTitle2"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Disabled Leaderboard"), pItem->GetDetail());

        Assert::IsNull(leaderboardsPage.GetItem(5));
    }

    TEST_METHOD(TestFetchItemDetail)
    {
        OverlayLeaderboardsPageViewModelHarness leaderboardsPage;
        auto& pLB1 = leaderboardsPage.mockGameContext.Assets().NewLeaderboard();
        pLB1.SetID(1U);
        pLB1.SetName(L"LeaderboardTitle");
        pLB1.SetDescription(L"Trigger this");

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
        leaderboardsPage.TestFetchItemDetail(0);

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
