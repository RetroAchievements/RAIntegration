#include "CppUnitTest.h"

#include "ui\viewmodels\OverlayFriendsPageViewModel.hh"

#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockImageRepository.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(OverlayFriendsPageViewModel_Tests)
{
private:
    class OverlayFriendsPageViewModelHarness : public OverlayFriendsPageViewModel
    {
    public:
        ra::api::mocks::MockServer mockServer;
        ra::services::mocks::MockClock mockClock;
        ra::services::mocks::MockThreadPool mockTheadPool;
        ra::ui::mocks::MockImageRepository mockImageRepository;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;

        ItemViewModel* GetItem(gsl::index nIndex) { return m_vItems.GetItemAt(nIndex); }

        size_t GetItemCount() const noexcept { return m_vItems.Count(); }

        void TestFetchItemDetail(gsl::index nIndex)
        {
            auto* pItem = GetItem(nIndex);
            Expects(pItem != nullptr);
            OverlayFriendsPageViewModel::FetchItemDetail(*pItem);
        }
    };

public:
    TEST_METHOD(TestRefreshNoFriends)
    {
        OverlayFriendsPageViewModelHarness friendsPage;
        friendsPage.mockServer.HandleRequest<ra::api::FetchUserFriends>(
            [](const ra::api::FetchUserFriends::Request&, ra::api::FetchUserFriends::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        friendsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Friends"), friendsPage.GetTitle());
        Assert::AreEqual(std::wstring(), friendsPage.GetSummary());

        friendsPage.mockTheadPool.ExecuteNextTask(); // fetch friends list is async

        Assert::AreEqual(std::wstring(), friendsPage.GetSummary());
        Assert::AreEqual({ 0U }, friendsPage.GetItemCount());
    }

    TEST_METHOD(TestRefreshSeveralFriends)
    {
        OverlayFriendsPageViewModelHarness friendsPage;
        friendsPage.mockServer.HandleRequest<ra::api::FetchUserFriends>(
            [](const ra::api::FetchUserFriends::Request&, ra::api::FetchUserFriends::Response& response)
        {
            response.Friends.push_back(ra::api::FetchUserFriends::Response::Friend{ "User1", 20, L"Activity1" });
            response.Friends.push_back(ra::api::FetchUserFriends::Response::Friend{ "User2", 60, L"Activity2" });
            response.Friends.push_back(ra::api::FetchUserFriends::Response::Friend{ "User3", 40, L"Activity3" });
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        friendsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Friends"), friendsPage.GetTitle());
        Assert::AreEqual(std::wstring(), friendsPage.GetSummary());

        friendsPage.mockTheadPool.ExecuteNextTask(); // fetch friends list is async

        Assert::AreEqual(std::wstring(), friendsPage.GetSummary());

        const auto* pFriend1 = friendsPage.GetItem(0);
        Assert::IsNotNull(pFriend1);
        Ensures(pFriend1 != nullptr);
        Assert::AreEqual(std::wstring(L"User1 (20)"), pFriend1->GetLabel());
        Assert::AreEqual(std::wstring(L"Activity1"), pFriend1->GetDetail());

        const auto* pFriend2 = friendsPage.GetItem(1);
        Assert::IsNotNull(pFriend2);
        Ensures(pFriend2 != nullptr);
        Assert::AreEqual(std::wstring(L"User2 (60)"), pFriend2->GetLabel());
        Assert::AreEqual(std::wstring(L"Activity2"), pFriend2->GetDetail());

        const auto* pFriend3 = friendsPage.GetItem(2);
        Assert::IsNotNull(pFriend3);
        Ensures(pFriend3 != nullptr);
        Assert::AreEqual(std::wstring(L"User3 (40)"), pFriend3->GetLabel());
        Assert::AreEqual(std::wstring(L"Activity3"), pFriend3->GetDetail());
    }

    TEST_METHOD(TestRefreshError)
    {
        OverlayFriendsPageViewModelHarness friendsPage;
        friendsPage.mockServer.HandleRequest<ra::api::FetchUserFriends>(
            [](const ra::api::FetchUserFriends::Request&, ra::api::FetchUserFriends::Response& response)
        {
            response.Result = ra::api::ApiResult::Error;
            response.ErrorMessage = "Failed to find friends";
            return true;
        });

        friendsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Friends"), friendsPage.GetTitle());
        Assert::AreEqual(std::wstring(), friendsPage.GetSummary());

        friendsPage.mockTheadPool.ExecuteNextTask(); // fetch friends list is async

        Assert::AreEqual(std::wstring(L"Failed to find friends"), friendsPage.GetSummary());
        Assert::AreEqual({ 0U }, friendsPage.GetItemCount());
    }

    TEST_METHOD(TestRefreshTimer)
    {
        OverlayFriendsPageViewModelHarness friendsPage;
        friendsPage.mockServer.HandleRequest<ra::api::FetchUserFriends>(
            [](const ra::api::FetchUserFriends::Request&, ra::api::FetchUserFriends::Response& response)
        {
            response.Friends.push_back(ra::api::FetchUserFriends::Response::Friend{ "User1", 20, L"Activity1" });
            response.Friends.push_back(ra::api::FetchUserFriends::Response::Friend{ "User2", 60, L"Activity2" });
            response.Friends.push_back(ra::api::FetchUserFriends::Response::Friend{ "User3", 40, L"Activity3" });
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        friendsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Friends"), friendsPage.GetTitle());
        Assert::AreEqual(std::wstring(), friendsPage.GetSummary());

        friendsPage.mockTheadPool.ExecuteNextTask(); // fetch friends list is async
        Assert::AreEqual({ 3U }, friendsPage.GetItemCount());

        // two friends updated, one added
        friendsPage.mockServer.HandleRequest<ra::api::FetchUserFriends>(
            [](const ra::api::FetchUserFriends::Request&, ra::api::FetchUserFriends::Response& response)
        {
            response.Friends.push_back(ra::api::FetchUserFriends::Response::Friend{ "User1", 30, L"Activity1b" });
            response.Friends.push_back(ra::api::FetchUserFriends::Response::Friend{ "User2", 60, L"Activity2" });
            response.Friends.push_back(ra::api::FetchUserFriends::Response::Friend{ "User3", 45, L"Activity3b" });
            response.Friends.push_back(ra::api::FetchUserFriends::Response::Friend{ "User4", 90, L"Activity4" });
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        friendsPage.Refresh();
        friendsPage.mockTheadPool.ExecuteNextTask(); // fetch friends list is async
        Assert::AreEqual({ 3U }, friendsPage.GetItemCount()); // refresh should not have occured

        friendsPage.mockClock.AdvanceTime(std::chrono::minutes(1));
        friendsPage.Refresh();
        friendsPage.mockTheadPool.ExecuteNextTask(); // fetch friends list is async
        Assert::AreEqual({ 3U }, friendsPage.GetItemCount()); // refresh should not have occured


        friendsPage.mockClock.AdvanceTime(std::chrono::minutes(1));
        friendsPage.Refresh();
        friendsPage.mockTheadPool.ExecuteNextTask(); // fetch friends list is async
        Assert::AreEqual({ 4U }, friendsPage.GetItemCount()); // refresh should have occured

        const auto* pFriend1 = friendsPage.GetItem(0);
        Assert::IsNotNull(pFriend1);
        Ensures(pFriend1 != nullptr);
        Assert::AreEqual(std::wstring(L"User1 (30)"), pFriend1->GetLabel());
        Assert::AreEqual(std::wstring(L"Activity1b"), pFriend1->GetDetail());

        const auto* pFriend2 = friendsPage.GetItem(1);
        Assert::IsNotNull(pFriend2);
        Ensures(pFriend2 != nullptr);
        Assert::AreEqual(std::wstring(L"User2 (60)"), pFriend2->GetLabel());
        Assert::AreEqual(std::wstring(L"Activity2"), pFriend2->GetDetail());

        const auto* pFriend3 = friendsPage.GetItem(2);
        Assert::IsNotNull(pFriend3);
        Ensures(pFriend3 != nullptr);
        Assert::AreEqual(std::wstring(L"User3 (45)"), pFriend3->GetLabel());
        Assert::AreEqual(std::wstring(L"Activity3b"), pFriend3->GetDetail());

        const auto* pFriend4 = friendsPage.GetItem(3);
        Assert::IsNotNull(pFriend4);
        Ensures(pFriend4 != nullptr);
        Assert::AreEqual(std::wstring(L"User4 (90)"), pFriend4->GetLabel());
        Assert::AreEqual(std::wstring(L"Activity4"), pFriend4->GetDetail());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
