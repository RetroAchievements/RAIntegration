#include "CppUnitTest.h"

#include "ui\viewmodels\OverlayAchievementsPageViewModel.hh"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockImageRepository.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockRcheevosClient.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockSessionTracker.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"
#include "tests\data\DataAsserts.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::data::models::AssetCategory;
using ra::data::models::AssetState;

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
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockSessionTracker mockSessionTracker;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockAchievementRuntime mockAchievementRuntime;
        ra::services::mocks::MockClock mockClock;
        ra::services::mocks::MockRcheevosClient mockRcheevosClient;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockImageRepository mockImageRepository;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        OverlayAchievementsPageViewModelHarness() noexcept
        {
            GSL_SUPPRESS_F6 mockWindowManager.AssetList.InitializeNotifyTargets();
        }

        ItemViewModel* GetItem(gsl::index nIndex) { return m_vItems.GetItemAt(nIndex); }

        void TestFetchItemDetail(gsl::index nIndex)
        {
            auto* pItem = GetItem(nIndex);
            Expects(pItem != nullptr);
            OverlayAchievementsPageViewModel::FetchItemDetail(*pItem);
        }

        AchievementViewModel* GetItemDetail(ra::AchievementID nId)
        {
            const auto pIter = m_vAchievementDetails.find(nId);
            if (pIter != m_vAchievementDetails.end())
                return &pIter->second;

            return nullptr;
        }

        ra::data::models::AchievementModel& NewAchievement(AssetCategory nCategory)
        {
            auto& pAchievement = mockGameContext.Assets().NewAchievement();
            pAchievement.SetCategory(nCategory);
            return pAchievement;
        }

        void SetProgress(ra::AchievementID nId, int nValue, int nMax)
        {
            auto* pTrigger = mockAchievementRuntime.GetAchievementTrigger(nId);
            if (pTrigger == nullptr)
            {
                mockAchievementRuntime.ActivateAchievement(nId, "0=1");
                pTrigger = mockAchievementRuntime.GetAchievementTrigger(nId);
            }

            pTrigger->measured_value = nValue;
            pTrigger->measured_target = nMax;
        }

        void AssertHeader(gsl::index nIndex, const std::wstring& sExpectedLabel)
        {
            const auto* pItem = GetItem(nIndex);
            Expects(pItem != nullptr);
            Assert::IsTrue(pItem->IsHeader());
            Assert::AreEqual(sExpectedLabel, pItem->GetLabel());
            Assert::IsFalse(pItem->IsDisabled());
        }

    private:
        void AssertAchievement(gsl::index nIndex, const rc_client_achievement_info_t* pAchievement, bool bLocked)
        {
            const auto* pItem = GetItem(nIndex);
            Expects(pItem != nullptr);
            Assert::IsFalse(pItem->IsHeader());
            Assert::AreEqual(pAchievement->public_.id, static_cast<uint32_t>(pItem->GetId()));

            const std::wstring sTitle =
                (pAchievement->public_.points == 1)
                    ? ra::StringPrintf(L"%s (1 point)", pAchievement->public_.title)
                    : ra::StringPrintf(L"%s (%u points)", pAchievement->public_.title, pAchievement->public_.points);
            Assert::AreEqual(sTitle, pItem->GetLabel());
            Assert::AreEqual(ra::Widen(pAchievement->public_.description), pItem->GetDetail());

            if (bLocked)
            {
                Assert::IsTrue(pItem->IsDisabled(), ra::StringPrintf(L"Item %d not disabled", nIndex).c_str());
                Assert::AreEqual(ra::StringPrintf("%s_lock", pAchievement->public_.badge_name), pItem->Image.Name());
            }
            else
            {
                Assert::IsFalse(pItem->IsDisabled(), ra::StringPrintf(L"Item %d disabled", nIndex).c_str());
                Assert::AreEqual(std::string(pAchievement->public_.badge_name), pItem->Image.Name());
            }
        }

    public:
        void AssertLockedAchievement(gsl::index nIndex, const rc_client_achievement_info_t* pAchievement)
        {
            AssertAchievement(nIndex, pAchievement, true);
        }

        void AssertUnlockedAchievement(gsl::index nIndex, const rc_client_achievement_info_t* pAchievement)
        {
            AssertAchievement(nIndex, pAchievement, false);
        }

        void AssertProgressAchievement(gsl::index nIndex, const rc_client_achievement_info_t* pAchievement, float fProgress, const std::wstring& sProgress)
        {
            AssertAchievement(nIndex, pAchievement, true);

            const auto* pItem = GetItem(nIndex);
            Expects(pItem != nullptr);

            Assert::AreEqual(fProgress, pItem->GetProgressPercentage());
            Assert::AreEqual(sProgress, pItem->GetProgressString());
        }
    };

public:
    TEST_METHOD(TestRefreshNoAchievements)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"No achievements present"), achievementsPage.GetSummary());
        Assert::IsNull(achievementsPage.GetItem(0));
    }

    TEST_METHOD(TestRefreshUnlockedAchievement)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockRcheevosClient.MockGame();

        auto* pAch1 = achievementsPage.mockRcheevosClient.MockAchievement(1, "AchievementTitle");
        pAch1->public_.description = "Trigger this";
        pAch1->public_.points = 5;
        snprintf(pAch1->public_.badge_name, sizeof(pAch1->public_.badge_name), "BADGE");
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch1->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 1 won (5/5)"), achievementsPage.GetSummary());

        achievementsPage.AssertHeader(0, L"Unlocked");
        achievementsPage.AssertUnlockedAchievement(1, pAch1);
        Assert::IsNull(achievementsPage.GetItem(2));
    }

    TEST_METHOD(TestRefreshActiveAchievement)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockRcheevosClient.MockGame();

        auto* pAch1 = achievementsPage.mockRcheevosClient.MockAchievement(1, "AchievementTitle");
        pAch1->public_.description = "Trigger this";
        pAch1->public_.points = 5;
        snprintf(pAch1->public_.badge_name, sizeof(pAch1->public_.badge_name), "BADGE");

        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"0 of 1 won (0/5)"), achievementsPage.GetSummary());

        achievementsPage.AssertHeader(0, L"Locked");
        achievementsPage.AssertLockedAchievement(1, pAch1);
        Assert::IsNull(achievementsPage.GetItem(2));
    }

    TEST_METHOD(TestRefreshOnePointAchievement)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockRcheevosClient.MockGame();

        auto* pAch1 = achievementsPage.mockRcheevosClient.MockAchievement(1, "AchievementTitle");
        pAch1->public_.description = "Trigger this";
        pAch1->public_.points = 1; // should say "1 point" instead of "1 points"
        snprintf(pAch1->public_.badge_name, sizeof(pAch1->public_.badge_name), "BADGE");
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"0 of 1 won (0/1)"), achievementsPage.GetSummary());

        achievementsPage.AssertHeader(0, L"Locked");
        achievementsPage.AssertLockedAchievement(1, pAch1);

        const auto* pItem = achievementsPage.GetItem(1);
        Expects(pItem != nullptr);
        Assert::AreEqual(std::wstring(L"AchievementTitle (1 point)"), pItem->GetLabel());

        Assert::IsNull(achievementsPage.GetItem(2));
    }

    TEST_METHOD(TestRefreshActiveAndInactiveAchievements)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockRcheevosClient.MockGame();

        auto* pAch1 = achievementsPage.mockRcheevosClient.MockAchievement(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE; // Waiting appears as active at this level

        auto* pAch2 = achievementsPage.mockRcheevosClient.MockAchievement(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_INACTIVE;

        auto* pAch3 = achievementsPage.mockRcheevosClient.MockAchievement(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch4 = achievementsPage.mockRcheevosClient.MockAchievement(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch4->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 4 won (4/10)"), achievementsPage.GetSummary());

        achievementsPage.AssertHeader(0, L"Locked");
        achievementsPage.AssertLockedAchievement(1, pAch1);
        achievementsPage.AssertLockedAchievement(2, pAch2); // Inactive returned in Locked bucket
        achievementsPage.AssertLockedAchievement(3, pAch3);
        achievementsPage.AssertHeader(4, L"Unlocked");
        achievementsPage.AssertUnlockedAchievement(5, pAch4);
        Assert::IsNull(achievementsPage.GetItem(6));
    }

    TEST_METHOD(TestRefreshActiveAndDisabledAchievements)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockRcheevosClient.MockGame();

        auto* pAch1 = achievementsPage.mockRcheevosClient.MockAchievement(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE; // Waiting appears as active at this level

        auto* pAch2 = achievementsPage.mockRcheevosClient.MockAchievement(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_DISABLED;
        pAch2->public_.bucket = RC_CLIENT_ACHIEVEMENT_BUCKET_UNSUPPORTED;

        auto* pAch3 = achievementsPage.mockRcheevosClient.MockAchievement(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch4 = achievementsPage.mockRcheevosClient.MockAchievement(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_DISABLED; // will appear as locked, even though it was unlocked
        pAch4->public_.bucket = RC_CLIENT_ACHIEVEMENT_BUCKET_UNSUPPORTED;
        pAch4->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH; // still counts in the overall unlock rate

        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 4 won (4/10)"), achievementsPage.GetSummary());

        auto* pItem = achievementsPage.GetItem(0);
        Expects(pItem != nullptr);
        Assert::IsTrue(pItem->IsHeader());
        Assert::AreEqual(std::wstring(L"Locked"), pItem->GetLabel());
        Assert::IsFalse(pItem->IsDisabled());

        pItem = achievementsPage.GetItem(1);
        Expects(pItem != nullptr);
        Assert::AreEqual(1, pItem->GetId());
        Assert::IsTrue(pItem->IsDisabled());

        pItem = achievementsPage.GetItem(2);
        Expects(pItem != nullptr);
        Assert::AreEqual(3, pItem->GetId());
        Assert::IsTrue(pItem->IsDisabled());

        pItem = achievementsPage.GetItem(3);
        Expects(pItem != nullptr);
        Assert::IsTrue(pItem->IsHeader());
        Assert::AreEqual(std::wstring(L"Unsupported"), pItem->GetLabel());
        Assert::IsFalse(pItem->IsDisabled());

        pItem = achievementsPage.GetItem(4);
        Expects(pItem != nullptr);
        Assert::AreEqual(2, pItem->GetId());
        Assert::IsTrue(pItem->IsDisabled());

        pItem = achievementsPage.GetItem(5);
        Expects(pItem != nullptr);
        Assert::AreEqual(4, pItem->GetId());
        Assert::IsTrue(pItem->IsDisabled());

        achievementsPage.AssertHeader(0, L"Locked");
        achievementsPage.AssertLockedAchievement(1, pAch1);
        achievementsPage.AssertLockedAchievement(2, pAch3);
        achievementsPage.AssertHeader(3, L"Unsupported");
        achievementsPage.AssertLockedAchievement(4, pAch2);
        achievementsPage.AssertLockedAchievement(5, pAch4);
        Assert::IsNull(achievementsPage.GetItem(6));
    }

    TEST_METHOD(TestRefreshInactiveAndDisabledAchievements)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockRcheevosClient.MockGame();

        auto* pAch1 = achievementsPage.mockRcheevosClient.MockAchievement(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_INACTIVE;

        auto* pAch2 = achievementsPage.mockRcheevosClient.MockAchievement(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_DISABLED;
        pAch2->public_.bucket = RC_CLIENT_ACHIEVEMENT_BUCKET_UNSUPPORTED;

        auto* pAch3 = achievementsPage.mockRcheevosClient.MockAchievement(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch3->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

        auto* pAch4 = achievementsPage.mockRcheevosClient.MockAchievement(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_DISABLED; // will appear as locked, even though it was unlocked
        pAch4->public_.bucket = RC_CLIENT_ACHIEVEMENT_BUCKET_UNSUPPORTED;
        pAch4->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH; // still counts in the overall unlock rate

        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"2 of 4 won (7/10)"), achievementsPage.GetSummary());

        achievementsPage.AssertHeader(0, L"Locked");
        achievementsPage.AssertLockedAchievement(1, pAch1);
        achievementsPage.AssertHeader(2, L"Unsupported");
        achievementsPage.AssertLockedAchievement(3, pAch2);
        achievementsPage.AssertLockedAchievement(4, pAch4);
        achievementsPage.AssertHeader(5, L"Unlocked");
        achievementsPage.AssertUnlockedAchievement(6, pAch3);
        Assert::IsNull(achievementsPage.GetItem(7));
    }

    TEST_METHOD(TestRefreshActiveAndPrimedAchievements)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockRcheevosClient.MockGame();

        auto* pAch1 = achievementsPage.mockRcheevosClient.MockAchievement(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch2 = achievementsPage.mockRcheevosClient.MockAchievementWithTrigger(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAch2->trigger->state = RC_TRIGGER_STATE_PRIMED;

        auto* pAch3 = achievementsPage.mockRcheevosClient.MockAchievement(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"0 of 3 won (0/6)"), achievementsPage.GetSummary());

        achievementsPage.AssertHeader(0, L"Active Challenges");
        achievementsPage.AssertLockedAchievement(1, pAch2);
        achievementsPage.AssertHeader(2, L"Locked");
        achievementsPage.AssertLockedAchievement(3, pAch1);
        achievementsPage.AssertLockedAchievement(4, pAch3);
        Assert::IsNull(achievementsPage.GetItem(5));
    }

    TEST_METHOD(TestRefreshCategoriesCoreOnly)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockRcheevosClient.MockGame();

        auto* pAch1 = achievementsPage.mockRcheevosClient.MockAchievement(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch2 = achievementsPage.mockRcheevosClient.MockUnofficialAchievement(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch3 = achievementsPage.mockRcheevosClient.MockLocalAchievement(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch4 = achievementsPage.mockRcheevosClient.MockAchievement(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch4->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

        achievementsPage.mockWindowManager.AssetList.SetFilterCategory(
            ra::ui::viewmodels::AssetListViewModel::FilterCategory::Core);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 2 won (4/5)"), achievementsPage.GetSummary());

        // only 1 and 4 will be visible - 2 and 3 are not core, and will be filtered out
        achievementsPage.AssertHeader(0, L"Locked");
        achievementsPage.AssertLockedAchievement(1, pAch1);
        achievementsPage.AssertHeader(2, L"Unlocked");
        achievementsPage.AssertUnlockedAchievement(3, pAch4);
        Assert::IsNull(achievementsPage.GetItem(4));
    }

    TEST_METHOD(TestRefreshCategoriesWithLocal)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockRcheevosClient.MockGame();

        auto* pAch1 = achievementsPage.mockRcheevosClient.MockAchievement(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch2 = achievementsPage.mockRcheevosClient.MockUnofficialAchievement(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch3 = achievementsPage.mockRcheevosClient.MockLocalAchievement(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch4 = achievementsPage.mockRcheevosClient.MockAchievement(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch4->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

        achievementsPage.mockWindowManager.AssetList.SetFilterCategory(
            ra::ui::viewmodels::AssetListViewModel::FilterCategory::Local);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 2 won (4/5)"), achievementsPage.GetSummary());

        achievementsPage.AssertHeader(0, L"Game Title - Locked");
        achievementsPage.AssertLockedAchievement(1, pAch1);
        achievementsPage.AssertHeader(2, L"Game Title - Unofficial");
        achievementsPage.AssertLockedAchievement(3, pAch2);
        achievementsPage.AssertHeader(4, L"Game Title - Unlocked");
        achievementsPage.AssertUnlockedAchievement(5, pAch4);
        achievementsPage.AssertHeader(6, L"Local Achievements - Locked");
        achievementsPage.AssertUnlockedAchievement(7, pAch3); // local achievements appear unlocked (no greyscale image)
        Assert::IsNull(achievementsPage.GetItem(8));
    }

    TEST_METHOD(TestRefreshProgressAchievements)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockRcheevosClient.MockGame();

        auto* pAch1 = achievementsPage.mockRcheevosClient.MockAchievement(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch2 = achievementsPage.mockRcheevosClient.MockAchievementWithTrigger(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAch2->trigger->measured_target = 10;
        pAch2->trigger->measured_value = 5;

        auto* pAch3 = achievementsPage.mockRcheevosClient.MockAchievement(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch4 = achievementsPage.mockRcheevosClient.MockAchievementWithTrigger(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch4->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;
        pAch4->trigger->measured_target = 10;
        pAch4->trigger->measured_value = 5;

        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 4 won (4/10)"), achievementsPage.GetSummary());

        achievementsPage.AssertHeader(0, L"Locked");
        achievementsPage.AssertProgressAchievement(1, pAch1, 0.0, L"");
        achievementsPage.AssertProgressAchievement(2, pAch2, 50.0, L"5/10");
        achievementsPage.AssertProgressAchievement(3, pAch3, 0.0, L"");
        achievementsPage.AssertHeader(4, L"Unlocked");
        achievementsPage.AssertUnlockedAchievement(5, pAch4);
        Assert::IsNull(achievementsPage.GetItem(6));

         // unlocked achievement should not report progress
        const auto* pItem = achievementsPage.GetItem(5);
        Expects(pItem != nullptr);
        Assert::AreEqual(0.0f, pItem->GetProgressPercentage());
        Assert::AreEqual(std::wstring(), pItem->GetProgressString());
    }

    TEST_METHOD(TestRefreshAlmostThereAchievements)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockRcheevosClient.MockGame();

        auto* pAch1 = achievementsPage.mockRcheevosClient.MockAchievementWithTrigger(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAch1->trigger->measured_target = 10;
        pAch1->trigger->measured_value = 1;

        auto* pAch2 = achievementsPage.mockRcheevosClient.MockAchievementWithTrigger(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch2->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;
        pAch2->trigger->measured_target = 10;
        pAch2->trigger->measured_value = 9;

        auto* pAch3 = achievementsPage.mockRcheevosClient.MockAchievementWithTrigger(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAch3->trigger->measured_target = 10;
        pAch3->trigger->measured_value = 9;

        auto* pAch4 = achievementsPage.mockRcheevosClient.MockAchievement(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 4 won (2/10)"), achievementsPage.GetSummary());

        achievementsPage.AssertHeader(0, L"Almost There");
        achievementsPage.AssertProgressAchievement(1, pAch3, 90.0, L"9/10");
        achievementsPage.AssertHeader(2, L"Locked");
        achievementsPage.AssertProgressAchievement(3, pAch1, 10.0, L"1/10");
        achievementsPage.AssertProgressAchievement(4, pAch4, 0.0, L"");
        achievementsPage.AssertHeader(5, L"Unlocked");
        achievementsPage.AssertUnlockedAchievement(6, pAch2);
        Assert::IsNull(achievementsPage.GetItem(7));

        // unlocked achievement should not report progress
        const auto* pItem = achievementsPage.GetItem(6);
        Expects(pItem != nullptr);
        Assert::AreEqual(0.0f, pItem->GetProgressPercentage());
        Assert::AreEqual(std::wstring(), pItem->GetProgressString());
    }

    TEST_METHOD(TestRefreshAlmostThereAchievementsSorting)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockRcheevosClient.MockGame();

        auto* pAch1 = achievementsPage.mockRcheevosClient.MockAchievementWithTrigger(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAch1->trigger->measured_target = 100;
        pAch1->trigger->measured_value = 90;

        auto* pAch2 = achievementsPage.mockRcheevosClient.MockAchievementWithTrigger(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAch2->trigger->measured_target = 100;
        pAch2->trigger->measured_value = 85;

        auto* pAch3 = achievementsPage.mockRcheevosClient.MockAchievementWithTrigger(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAch3->trigger->measured_target = 100;
        pAch3->trigger->measured_value = 95;

        auto* pAch4 = achievementsPage.mockRcheevosClient.MockAchievementWithTrigger(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAch4->trigger->measured_target = 100;
        pAch4->trigger->measured_value = 85;

        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"0 of 4 won (0/10)"), achievementsPage.GetSummary());

        achievementsPage.AssertHeader(0, L"Almost There");
        achievementsPage.AssertProgressAchievement(1, pAch3, 95.0, L"95/100");
        achievementsPage.AssertProgressAchievement(2, pAch1, 90.0, L"90/100");
        achievementsPage.AssertProgressAchievement(3, pAch2, 85.0, L"85/100");
        achievementsPage.AssertProgressAchievement(4, pAch4, 85.0, L"85/100");
        Assert::IsNull(achievementsPage.GetItem(5));
    }

    TEST_METHOD(TestRefreshSession)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockGameContext.SetGameId(1U);
        achievementsPage.mockRcheevosClient.MockGame();
        achievementsPage.mockSessionTracker.MockSession(1U, 1234567879, std::chrono::minutes(347));

        auto* pAch1 = achievementsPage.mockRcheevosClient.MockAchievement(1);
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch1->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Achievements"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 1 won (5/5) - 5h47m"), achievementsPage.GetSummary());
    }

    TEST_METHOD(TestUpdateSession)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockGameContext.SetGameId(1U);
        achievementsPage.mockRcheevosClient.MockGame();
        achievementsPage.mockSessionTracker.MockSession(1U, 1234567879, std::chrono::seconds(17 * 60 + 12));

        auto* pAch1 = achievementsPage.mockRcheevosClient.MockAchievement(1);
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch1->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

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
        achievementsPage.mockWindowManager.AssetList.SetFilterCategory(AssetListViewModel::FilterCategory::Local);
        achievementsPage.mockRcheevosClient.MockGame();
        const auto* pAch1 = achievementsPage.mockRcheevosClient.MockAchievement(ra::data::context::GameAssets::FirstLocalId);

        achievementsPage.mockServer.ExpectUncalled<ra::api::FetchAchievementInfo>();

        achievementsPage.Refresh();
        achievementsPage.TestFetchItemDetail(1);

        auto const* pDetail = achievementsPage.GetItemDetail(pAch1->public_.id);
        Expects(pDetail != nullptr);

        Assert::AreEqual(std::wstring(L"Local Achievement"), pDetail->GetWonBy());
        Assert::AreEqual({ 0U }, pDetail->RecentWinners.Count());
    }

    TEST_METHOD(TestFetchItemDetail)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockRcheevosClient.MockGame();
        auto* pAch1 = achievementsPage.mockRcheevosClient.MockAchievement(1234);
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch1->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

        achievementsPage.mockUserContext.Initialize("user2", "User2", "ApiToken");
        achievementsPage.mockServer.HandleRequest<ra::api::FetchAchievementInfo>([](const ra::api::FetchAchievementInfo::Request& request, ra::api::FetchAchievementInfo::Response& response)
        {
            Assert::AreEqual(1234U, request.AchievementId);

            response.Result = ra::api::ApiResult::Success;
            response.EarnedBy = 6;
            response.NumPlayers = 12;
            response.Entries.emplace_back(ra::api::FetchAchievementInfo::Response::Entry{ "User1", 1234, 123456789 });
            response.Entries.emplace_back(ra::api::FetchAchievementInfo::Response::Entry{ "User2", 6789, 123456789 });
            response.Entries.emplace_back(ra::api::FetchAchievementInfo::Response::Entry{ "User3", 1, 123456789 });

            return true;
        });

        achievementsPage.Refresh();
        achievementsPage.TestFetchItemDetail(1);

        auto* pDetail = achievementsPage.GetItemDetail(pAch1->public_.id);
        Expects(pDetail != nullptr);
        Assert::AreEqual(std::wstring(), pDetail->GetWonBy());

        achievementsPage.mockThreadPool.ExecuteNextTask();
        pDetail = achievementsPage.GetItemDetail(pAch1->public_.id);
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
