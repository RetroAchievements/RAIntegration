#include "CppUnitTest.h"

#include "ui\viewmodels\OverlayAchievementsPageViewModel.hh"

#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockImageRepository.hh"
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

TEST_CLASS(OverlayAchievementsPageViewModel_Tests)
{
private:
    class OverlayAchievementsPageViewModelHarness : public OverlayAchievementsPageViewModel
    {
    public:
        ra::api::mocks::MockServer mockServer;
        ra::data::mocks::MockGameContext mockGameContext;
        ra::data::mocks::MockSessionTracker mockSessionTracker;
        ra::data::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockImageRepository mockImageRepository;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;

        ItemViewModel* GetItem(gsl::index nIndex) { return m_vItems.GetItemAt(nIndex); }

        void TestFetchItemDetail(gsl::index nIndex)
        {
            auto* pItem = GetItem(nIndex);
            Expects(pItem != nullptr);
            OverlayAchievementsPageViewModel::FetchItemDetail(*pItem);
        }

        AchievementViewModel* GetItemDetail(ra::AchievementID nId)
        {
            auto pIter = m_vAchievementDetails.find(nId);
            if (pIter != m_vAchievementDetails.end())
                return &pIter->second;

            return nullptr;
        }
    };

public:
    TEST_METHOD(TestRefreshNoAchievements)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"No achievements present"), achievementsPage.GetSummary());
    }

    TEST_METHOD(TestRefreshInactiveAchievement)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        auto& pAch1 = achievementsPage.mockGameContext.NewAchievement(Achievement::Category::Core);
        pAch1.SetID(1);
        pAch1.SetTitle("AchievementTitle");
        pAch1.SetDescription("Trigger this");
        pAch1.SetPoints(5U);
        pAch1.SetBadgeImage("BADGE_URI");
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 1 won (5/5)"), achievementsPage.GetSummary());

        auto const* pItem = achievementsPage.GetItem(0);
        Expects(pItem != nullptr);
        Assert::AreEqual(1, pItem->GetId());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5 points)"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Trigger this"), pItem->GetDetail());
        Assert::IsFalse(pItem->IsDisabled());
        Assert::AreEqual(std::string("BADGE_URI"), pItem->Image.Name());
    }

    TEST_METHOD(TestRefreshActiveAchievement)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        auto& pAch1 = achievementsPage.mockGameContext.NewAchievement(Achievement::Category::Core);
        pAch1.SetID(1);
        pAch1.SetTitle("AchievementTitle");
        pAch1.SetDescription("Trigger this");
        pAch1.SetPoints(5U);
        pAch1.SetBadgeImage("BADGE_URI");
        pAch1.SetActive(true);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"0 of 1 won (0/5)"), achievementsPage.GetSummary());

        auto const* pItem = achievementsPage.GetItem(0);
        Expects(pItem != nullptr);
        Assert::AreEqual(1, pItem->GetId());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5 points)"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Trigger this"), pItem->GetDetail());
        Assert::IsTrue(pItem->IsDisabled());
        Assert::AreEqual(std::string("BADGE_URI_lock"), pItem->Image.Name());
    }

    TEST_METHOD(TestRefreshActiveAndInactiveAchievements)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        auto& pAch1 = achievementsPage.mockGameContext.NewAchievement(Achievement::Category::Core);
        pAch1.SetID(1);
        pAch1.SetPoints(1U);
        pAch1.SetActive(true);
        auto& pAch2 = achievementsPage.mockGameContext.NewAchievement(Achievement::Category::Core);
        pAch2.SetID(2);
        pAch2.SetPoints(2U);
        pAch2.SetActive(false);
        auto& pAch3 = achievementsPage.mockGameContext.NewAchievement(Achievement::Category::Core);
        pAch3.SetID(3);
        pAch3.SetPoints(3U);
        pAch3.SetActive(true);
        auto& pAch4 = achievementsPage.mockGameContext.NewAchievement(Achievement::Category::Core);
        pAch4.SetID(4);
        pAch4.SetPoints(4U);
        pAch4.SetActive(false);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"2 of 4 won (6/10)"), achievementsPage.GetSummary());

        auto const* pItem = achievementsPage.GetItem(0);
        Expects(pItem != nullptr);
        Assert::AreEqual(1, pItem->GetId());
        Assert::IsTrue(pItem->IsDisabled());

        pItem = achievementsPage.GetItem(1);
        Expects(pItem != nullptr);
        Assert::AreEqual(2, pItem->GetId());
        Assert::IsFalse(pItem->IsDisabled());

        pItem = achievementsPage.GetItem(2);
        Expects(pItem != nullptr);
        Assert::AreEqual(3, pItem->GetId());
        Assert::IsTrue(pItem->IsDisabled());

        pItem = achievementsPage.GetItem(3);
        Expects(pItem != nullptr);
        Assert::AreEqual(4, pItem->GetId());
        Assert::IsFalse(pItem->IsDisabled());
    }

    TEST_METHOD(TestRefreshCategoryFilter)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        auto& pAch1 = achievementsPage.mockGameContext.NewAchievement(Achievement::Category::Core);
        pAch1.SetID(1);
        pAch1.SetPoints(1U);
        pAch1.SetActive(true);
        auto& pAch2 = achievementsPage.mockGameContext.NewAchievement(Achievement::Category::Unofficial);
        pAch2.SetID(2);
        pAch2.SetPoints(2U);
        pAch2.SetActive(false);
        auto& pAch3 = achievementsPage.mockGameContext.NewAchievement(Achievement::Category::Local);
        pAch3.SetID(3);
        pAch3.SetPoints(3U);
        pAch3.SetActive(true);
        auto& pAch4 = achievementsPage.mockGameContext.NewAchievement(Achievement::Category::Core);
        pAch4.SetID(4);
        pAch4.SetPoints(4U);
        pAch4.SetActive(false);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 2 won (4/5)"), achievementsPage.GetSummary());

        auto* pItem = achievementsPage.GetItem(0);
        Expects(pItem != nullptr);
        Assert::AreEqual(1, pItem->GetId());
        Assert::IsTrue(pItem->IsDisabled());

        pItem = achievementsPage.GetItem(1);
        Expects(pItem != nullptr);
        Assert::AreEqual(2, pItem->GetId());
        Assert::IsFalse(pItem->IsDisabled());

        pItem = achievementsPage.GetItem(2);
        Expects(pItem != nullptr);
        Assert::AreEqual(3, pItem->GetId());
        Assert::IsFalse(pItem->IsDisabled());

        pItem = achievementsPage.GetItem(3);
        Expects(pItem != nullptr);
        Assert::AreEqual(4, pItem->GetId());
        Assert::IsFalse(pItem->IsDisabled());

        Assert::IsNull(achievementsPage.GetItem(5));
    }

    TEST_METHOD(TestRefreshLocalAchievement)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        auto& pAch1 = achievementsPage.mockGameContext.NewAchievement(Achievement::Category::Local);
        pAch1.SetID(1);
        pAch1.SetTitle("AchievementTitle");
        pAch1.SetDescription("Trigger this");
        pAch1.SetPoints(5U);
        pAch1.SetBadgeImage("BADGE_URI");
        pAch1.SetActive(true);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 achievements present"), achievementsPage.GetSummary());

        auto const* pItem = achievementsPage.GetItem(0);
        Expects(pItem != nullptr);
        Assert::AreEqual(1, pItem->GetId());
        Assert::AreEqual(std::wstring(L"AchievementTitle (5 points)"), pItem->GetLabel());
        Assert::AreEqual(std::wstring(L"Trigger this"), pItem->GetDetail());
        Assert::IsFalse(pItem->IsDisabled()); // local achievement never disabled
        Assert::AreEqual(std::string("BADGE_URI"), pItem->Image.Name());
    }

    TEST_METHOD(TestRefreshSession)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockGameContext.SetGameId(1U);
        achievementsPage.mockSessionTracker.MockSession(1U, 1234567879, std::chrono::minutes(347));

        auto& pAch1 = achievementsPage.mockGameContext.NewAchievement(Achievement::Category::Core);
        pAch1.SetID(1);
        pAch1.SetPoints(5U);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 1 won (5/5) - 5h47m"), achievementsPage.GetSummary());
    }

    TEST_METHOD(TestUpdateSession)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockGameContext.SetGameId(1U);
        achievementsPage.mockSessionTracker.MockSession(1U, 1234567879, std::chrono::seconds(17 * 60 + 12));

        auto& pAch1 = achievementsPage.mockGameContext.NewAchievement(Achievement::Category::Core);
        pAch1.SetID(1);
        pAch1.SetPoints(5U);
        achievementsPage.Refresh();

        achievementsPage.mockSessionTracker.MockSession(1U, 1234567879, std::chrono::minutes(1));
        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 1 won (5/5) - 0h17m"), achievementsPage.GetSummary());

        // Refresh() called at 12 seconds into session, text shouldn't update until 60 seconds in
        achievementsPage.Update(40.0); // 12 -> 52
        Assert::AreEqual(std::wstring(L"1 of 1 won (5/5) - 0h17m"), achievementsPage.GetSummary());

        Assert::IsFalse(achievementsPage.Update(7.9)); // 52 -> 59.9
        Assert::AreEqual(std::wstring(L"1 of 1 won (5/5) - 0h17m"), achievementsPage.GetSummary());

        Assert::IsTrue(achievementsPage.Update(0.2)); // 59.9 -> 60.1
        Assert::AreEqual(std::wstring(L"1 of 1 won (5/5) - 0h18m"), achievementsPage.GetSummary());
    }

    TEST_METHOD(TestFetchItemDetailLocal)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        auto& pAch1 = achievementsPage.mockGameContext.NewAchievement(Achievement::Category::Local);
        pAch1.SetID(1);
        pAch1.SetTitle("AchievementTitle");
        pAch1.SetDescription("Trigger this");
        pAch1.SetPoints(5U);
        pAch1.SetBadgeImage("BADGE_URI");
        pAch1.SetActive(true);

        achievementsPage.mockServer.ExpectUncalled<ra::api::FetchAchievementInfo>();

        achievementsPage.Refresh();
        achievementsPage.TestFetchItemDetail(0);

        auto const* pDetail = achievementsPage.GetItemDetail(1);
        Expects(pDetail != nullptr);

        Assert::AreEqual(std::wstring(L"Local Achievement"), pDetail->GetWonBy());
        Assert::AreEqual({ 0U }, pDetail->RecentWinners.Count());
    }

    TEST_METHOD(TestFetchItemDetail)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        auto& pAch1 = achievementsPage.mockGameContext.NewAchievement(Achievement::Category::Core);
        pAch1.SetID(1);
        pAch1.SetTitle("AchievementTitle");
        pAch1.SetDescription("Trigger this");
        pAch1.SetPoints(5U);
        pAch1.SetBadgeImage("BADGE_URI");
        pAch1.SetActive(true);

        achievementsPage.mockUserContext.Initialize("User2", "ApiToken");
        achievementsPage.mockServer.HandleRequest<ra::api::FetchAchievementInfo>([](const ra::api::FetchAchievementInfo::Request& request, ra::api::FetchAchievementInfo::Response& response)
        {
            Assert::AreEqual(1U, request.AchievementId);

            response.Result = ra::api::ApiResult::Success;
            response.EarnedBy = 6;
            response.NumPlayers = 12;
            response.Entries.emplace_back(ra::api::FetchAchievementInfo::Response::Entry{ "User1", 1234, 123456789 });
            response.Entries.emplace_back(ra::api::FetchAchievementInfo::Response::Entry{ "User2", 6789, 123456789 });
            response.Entries.emplace_back(ra::api::FetchAchievementInfo::Response::Entry{ "User3", 1, 123456789 });

            return true;
        });

        achievementsPage.Refresh();
        achievementsPage.TestFetchItemDetail(0);

        auto* pDetail = achievementsPage.GetItemDetail(1);
        Expects(pDetail != nullptr);
        Assert::AreEqual(std::wstring(), pDetail->GetWonBy());

        achievementsPage.mockThreadPool.ExecuteNextTask();
        pDetail = achievementsPage.GetItemDetail(1);
        Expects(pDetail != nullptr);

        Assert::AreEqual(std::wstring(L"Won by 6 of 12 (50%)"), pDetail->GetWonBy());
        Assert::AreEqual({ 3U }, pDetail->RecentWinners.Count());
        auto* pItem = pDetail->RecentWinners.GetItemAt(0);
        Expects(pItem != nullptr);
        Assert::AreEqual(std::wstring(L"User1"), pItem->GetLabel());
        Assert::IsFalse(pItem->GetDetail().empty());
        Assert::IsFalse(pItem->IsDisabled());

        pItem = pDetail->RecentWinners.GetItemAt(1);
        Expects(pItem != nullptr);
        Assert::AreEqual(std::wstring(L"User2"), pItem->GetLabel());
        Assert::IsFalse(pItem->GetDetail().empty());
        Assert::IsTrue(pItem->IsDisabled()); // current user should be highlighted

        pItem = pDetail->RecentWinners.GetItemAt(2);
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
