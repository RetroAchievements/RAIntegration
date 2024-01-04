#include "CppUnitTest.h"

#include "ui\viewmodels\OverlayAchievementsPageViewModel.hh"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockImageRepository.hh"
#include "tests\mocks\MockOverlayManager.hh"
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
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockImageRepository mockImageRepository;
        ra::ui::viewmodels::mocks::MockOverlayManager mockOverlayManager;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        OverlayAchievementsPageViewModelHarness() noexcept
        {
            GSL_SUPPRESS_F6 mockWindowManager.AssetList.InitializeNotifyTargets();

            mockGameContext.SetGameTitle(L"Game Title");
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
        achievementsPage.SetCanCollapseHeaders(false);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"No achievements present"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L""), achievementsPage.GetTitleDetail());
        Assert::IsNull(achievementsPage.GetItem(0));
    }

    TEST_METHOD(TestRefreshNoGameLoaded)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockGameContext.SetGameTitle(L"");
        achievementsPage.SetCanCollapseHeaders(false);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"No Game Loaded"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"No achievements present"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L""), achievementsPage.GetTitleDetail());
        Assert::IsNull(achievementsPage.GetItem(0));
    }

    TEST_METHOD(TestRefreshUnlockedAchievement)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockAchievementRuntime.MockGame();

        auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievement(1, "AchievementTitle");
        pAch1->public_.description = "Trigger this";
        pAch1->public_.points = 5;
        snprintf(pAch1->public_.badge_name, sizeof(pAch1->public_.badge_name), "BADGE");
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch1->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

        achievementsPage.SetCanCollapseHeaders(false);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 1 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"5 of 5 points"), achievementsPage.GetTitleDetail());

        achievementsPage.AssertHeader(0, L"Unlocked");
        achievementsPage.AssertUnlockedAchievement(1, pAch1);
        Assert::IsNull(achievementsPage.GetItem(2));
    }

    TEST_METHOD(TestRefreshActiveAchievement)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockAchievementRuntime.MockGame();

        auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievement(1, "AchievementTitle");
        pAch1->public_.description = "Trigger this";
        pAch1->public_.points = 5;
        snprintf(pAch1->public_.badge_name, sizeof(pAch1->public_.badge_name), "BADGE");

        achievementsPage.SetCanCollapseHeaders(false);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"0 of 1 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"0 of 5 points"), achievementsPage.GetTitleDetail());

        achievementsPage.AssertHeader(0, L"Locked");
        achievementsPage.AssertLockedAchievement(1, pAch1);
        Assert::IsNull(achievementsPage.GetItem(2));
    }

    TEST_METHOD(TestRefreshOnePointAchievement)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockAchievementRuntime.MockGame();

        auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievement(1, "AchievementTitle");
        pAch1->public_.description = "Trigger this";
        pAch1->public_.points = 1; // should say "1 point" instead of "1 points"
        snprintf(pAch1->public_.badge_name, sizeof(pAch1->public_.badge_name), "BADGE");
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        achievementsPage.SetCanCollapseHeaders(false);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"0 of 1 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"0 of 1 points"), achievementsPage.GetTitleDetail());

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
        achievementsPage.mockAchievementRuntime.MockGame();

        auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievement(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE; // Waiting appears as active at this level

        auto* pAch2 = achievementsPage.mockAchievementRuntime.MockAchievement(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_INACTIVE;

        auto* pAch3 = achievementsPage.mockAchievementRuntime.MockAchievement(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch4 = achievementsPage.mockAchievementRuntime.MockAchievement(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch4->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

        achievementsPage.SetCanCollapseHeaders(false);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 4 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"4 of 10 points"), achievementsPage.GetTitleDetail());

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
        achievementsPage.mockAchievementRuntime.MockGame();

        auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievement(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE; // Waiting appears as active at this level

        auto* pAch2 = achievementsPage.mockAchievementRuntime.MockAchievement(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_DISABLED;
        pAch2->public_.bucket = RC_CLIENT_ACHIEVEMENT_BUCKET_UNSUPPORTED;

        auto* pAch3 = achievementsPage.mockAchievementRuntime.MockAchievement(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch4 = achievementsPage.mockAchievementRuntime.MockAchievement(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_DISABLED; // will appear as locked, even though it was unlocked
        pAch4->public_.bucket = RC_CLIENT_ACHIEVEMENT_BUCKET_UNSUPPORTED;
        pAch4->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH; // still counts in the overall unlock rate

        achievementsPage.SetCanCollapseHeaders(false);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 4 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"4 of 10 points"), achievementsPage.GetTitleDetail());

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
        achievementsPage.mockAchievementRuntime.MockGame();

        auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievement(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_INACTIVE;

        auto* pAch2 = achievementsPage.mockAchievementRuntime.MockAchievement(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_DISABLED;
        pAch2->public_.bucket = RC_CLIENT_ACHIEVEMENT_BUCKET_UNSUPPORTED;

        auto* pAch3 = achievementsPage.mockAchievementRuntime.MockAchievement(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch3->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

        auto* pAch4 = achievementsPage.mockAchievementRuntime.MockAchievement(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_DISABLED; // will appear as locked, even though it was unlocked
        pAch4->public_.bucket = RC_CLIENT_ACHIEVEMENT_BUCKET_UNSUPPORTED;
        pAch4->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH; // still counts in the overall unlock rate

        achievementsPage.SetCanCollapseHeaders(false);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"2 of 4 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"7 of 10 points"), achievementsPage.GetTitleDetail());

        achievementsPage.AssertHeader(0, L"Locked");
        achievementsPage.AssertLockedAchievement(1, pAch1);
        achievementsPage.AssertHeader(2, L"Unsupported");
        achievementsPage.AssertLockedAchievement(3, pAch2);
        achievementsPage.AssertLockedAchievement(4, pAch4);
        achievementsPage.AssertHeader(5, L"Unlocked");
        achievementsPage.AssertUnlockedAchievement(6, pAch3);
        Assert::IsNull(achievementsPage.GetItem(7));
    }

    TEST_METHOD(TestRefreshInactiveAndDisabledAchievementsInitialCollapseState)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockAchievementRuntime.MockGame();

        auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievement(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_INACTIVE;

        auto* pAch2 = achievementsPage.mockAchievementRuntime.MockAchievement(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_DISABLED;
        pAch2->public_.bucket = RC_CLIENT_ACHIEVEMENT_BUCKET_UNSUPPORTED;

        auto* pAch3 = achievementsPage.mockAchievementRuntime.MockAchievement(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch3->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

        auto* pAch4 = achievementsPage.mockAchievementRuntime.MockAchievement(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_DISABLED;
        pAch4->public_.bucket = RC_CLIENT_ACHIEVEMENT_BUCKET_UNSUPPORTED;
        pAch4->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

        achievementsPage.SetCanCollapseHeaders(true);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"2 of 4 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"7 of 10 points"), achievementsPage.GetTitleDetail());

        achievementsPage.AssertHeader(0, L"Locked");
        achievementsPage.AssertLockedAchievement(1, pAch1);
        achievementsPage.AssertHeader(2, L"Unsupported");
        achievementsPage.AssertLockedAchievement(3, pAch2);
        achievementsPage.AssertLockedAchievement(4, pAch4);
        achievementsPage.AssertHeader(5, L"Unlocked"); // unlocked should be collapsed
        Assert::IsNull(achievementsPage.GetItem(6));
    }

    TEST_METHOD(TestRefreshActiveAndPrimedAchievements)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockAchievementRuntime.MockGame();

        auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievement(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch2 = achievementsPage.mockAchievementRuntime.MockAchievementWithTrigger(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAch2->trigger->state = RC_TRIGGER_STATE_PRIMED;

        auto* pAch3 = achievementsPage.mockAchievementRuntime.MockAchievement(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        achievementsPage.SetCanCollapseHeaders(false);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"0 of 3 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"0 of 6 points"), achievementsPage.GetTitleDetail());

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
        achievementsPage.mockAchievementRuntime.MockGame();

        auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievement(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch2 = achievementsPage.mockAchievementRuntime.MockUnofficialAchievement(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch3 = achievementsPage.mockAchievementRuntime.MockLocalAchievement(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch4 = achievementsPage.mockAchievementRuntime.MockAchievement(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch4->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

        achievementsPage.mockWindowManager.AssetList.SetFilterCategory(
            ra::ui::viewmodels::AssetListViewModel::FilterCategory::Core);
        achievementsPage.SetCanCollapseHeaders(false);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 2 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"4 of 5 points"), achievementsPage.GetTitleDetail());

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
        achievementsPage.mockAchievementRuntime.MockGame();

        auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievement(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch2 = achievementsPage.mockAchievementRuntime.MockUnofficialAchievement(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch3 = achievementsPage.mockAchievementRuntime.MockLocalAchievement(3);
        pAch3->public_.points = 3;
        snprintf(pAch3->public_.badge_name, sizeof(pAch3->public_.badge_name), "L000003");
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        // L000003 badge points at VM for full local badge path
        auto& vmAch3 = achievementsPage.mockGameContext.Assets().NewAchievement();
        vmAch3.SetID(3);
        vmAch3.SetBadge(L"local\\1-abcdef0123456789.png");

        auto* pAch4 = achievementsPage.mockAchievementRuntime.MockAchievement(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch4->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

        achievementsPage.mockWindowManager.AssetList.SetFilterCategory(
            ra::ui::viewmodels::AssetListViewModel::FilterCategory::Local);
        achievementsPage.SetCanCollapseHeaders(false);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 2 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"4 of 5 points"), achievementsPage.GetTitleDetail());

        achievementsPage.AssertHeader(0, L"Game Title - Locked");
        achievementsPage.AssertLockedAchievement(1, pAch1);
        achievementsPage.AssertHeader(2, L"Game Title - Unofficial");
        achievementsPage.AssertLockedAchievement(3, pAch2);
        achievementsPage.AssertHeader(4, L"Game Title - Unlocked");
        achievementsPage.AssertUnlockedAchievement(5, pAch4);
        achievementsPage.AssertHeader(6, L"Local - Locked");

        // local achievements appear unlocked (no greyscale image available)
        const auto* pItem = achievementsPage.GetItem(7);
        Expects(pItem != nullptr);
        Assert::IsFalse(pItem->IsHeader());
        Assert::AreEqual(pAch3->public_.id, static_cast<uint32_t>(pItem->GetId()));
        const std::wstring sTitle = ra::StringPrintf(L"%s (%u points)", pAch3->public_.title, pAch3->public_.points);
        Assert::AreEqual(sTitle, pItem->GetLabel());
        Assert::AreEqual(ra::Widen(pAch3->public_.description), pItem->GetDetail());
        Assert::IsFalse(pItem->IsDisabled(), ra::StringPrintf(L"Item %d disabled", 7).c_str());
        Assert::AreEqual(ra::Narrow(vmAch3.GetBadge()), pItem->Image.Name());

        Assert::IsNull(achievementsPage.GetItem(8));
    }

    TEST_METHOD(TestRefreshProgressAchievements)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockAchievementRuntime.MockGame();

        auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievement(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch2 = achievementsPage.mockAchievementRuntime.MockAchievementWithTrigger(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAch2->trigger->measured_target = 10;
        pAch2->trigger->measured_value = 5;

        auto* pAch3 = achievementsPage.mockAchievementRuntime.MockAchievement(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        auto* pAch4 = achievementsPage.mockAchievementRuntime.MockAchievementWithTrigger(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch4->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;
        pAch4->trigger->measured_target = 10;
        pAch4->trigger->measured_value = 5;

        achievementsPage.SetCanCollapseHeaders(false);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 4 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"4 of 10 points"), achievementsPage.GetTitleDetail());

        achievementsPage.AssertHeader(0, L"Locked");
        achievementsPage.AssertProgressAchievement(1, pAch1, 0.0, L"");
        achievementsPage.AssertProgressAchievement(2, pAch2, 0.5, L"5/10");
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
        achievementsPage.mockAchievementRuntime.MockGame();

        auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievementWithTrigger(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAch1->trigger->measured_target = 10;
        pAch1->trigger->measured_value = 1;

        auto* pAch2 = achievementsPage.mockAchievementRuntime.MockAchievementWithTrigger(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch2->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;
        pAch2->trigger->measured_target = 10;
        pAch2->trigger->measured_value = 9;

        auto* pAch3 = achievementsPage.mockAchievementRuntime.MockAchievementWithTrigger(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAch3->trigger->measured_target = 10;
        pAch3->trigger->measured_value = 9;

        auto* pAch4 = achievementsPage.mockAchievementRuntime.MockAchievement(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;

        achievementsPage.SetCanCollapseHeaders(false);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 4 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"2 of 10 points"), achievementsPage.GetTitleDetail());

        achievementsPage.AssertHeader(0, L"Almost There");
        achievementsPage.AssertProgressAchievement(1, pAch3, 0.9, L"9/10");
        achievementsPage.AssertHeader(2, L"Locked");
        achievementsPage.AssertProgressAchievement(3, pAch1, 0.1, L"1/10");
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
        achievementsPage.mockAchievementRuntime.MockGame();

        auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievementWithTrigger(1);
        pAch1->public_.points = 1;
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAch1->trigger->measured_target = 100;
        pAch1->trigger->measured_value = 90;

        auto* pAch2 = achievementsPage.mockAchievementRuntime.MockAchievementWithTrigger(2);
        pAch2->public_.points = 2;
        pAch2->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAch2->trigger->measured_target = 100;
        pAch2->trigger->measured_value = 85;

        auto* pAch3 = achievementsPage.mockAchievementRuntime.MockAchievementWithTrigger(3);
        pAch3->public_.points = 3;
        pAch3->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAch3->trigger->measured_target = 100;
        pAch3->trigger->measured_value = 95;

        auto* pAch4 = achievementsPage.mockAchievementRuntime.MockAchievementWithTrigger(4);
        pAch4->public_.points = 4;
        pAch4->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
        pAch4->trigger->measured_target = 100;
        pAch4->trigger->measured_value = 85;

        achievementsPage.SetCanCollapseHeaders(false);
        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"0 of 4 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"0 of 10 points"), achievementsPage.GetTitleDetail());

        achievementsPage.AssertHeader(0, L"Almost There");
        achievementsPage.AssertProgressAchievement(1, pAch3, 0.95, L"95/100");
        achievementsPage.AssertProgressAchievement(2, pAch1, 0.90, L"90/100");
        achievementsPage.AssertProgressAchievement(3, pAch2, 0.85, L"85/100");
        achievementsPage.AssertProgressAchievement(4, pAch4, 0.85, L"85/100");
        Assert::IsNull(achievementsPage.GetItem(5));
    }

    TEST_METHOD(TestRefreshSession)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockGameContext.SetGameId(1U);
        achievementsPage.mockAchievementRuntime.MockGame();
        achievementsPage.mockSessionTracker.MockSession(1U, 1234567879, std::chrono::minutes(347));

        auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievement(1);
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch1->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

        achievementsPage.Refresh();

        Assert::AreEqual(std::wstring(L"Game Title"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 1 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"5 of 5 points - 5h47m"), achievementsPage.GetTitleDetail());
    }

    TEST_METHOD(TestUpdateSession)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockGameContext.SetGameId(1U);
        achievementsPage.mockAchievementRuntime.MockGame();
        achievementsPage.mockSessionTracker.MockSession(1U, 1234567879, std::chrono::seconds(17 * 60 + 12));

        auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievement(1);
        pAch1->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
        pAch1->public_.unlocked = RC_CLIENT_ACHIEVEMENT_UNLOCKED_BOTH;

        achievementsPage.Refresh();

        achievementsPage.mockSessionTracker.MockSession(1U, 1234567879, std::chrono::minutes(1));
        Assert::AreEqual(std::wstring(L"Game Title"), achievementsPage.GetTitle());
        Assert::AreEqual(std::wstring(L"1 of 1 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"5 of 5 points - 0h17m"), achievementsPage.GetTitleDetail());

        // Refresh() called at 12 seconds into session, text shouldn't update until 60 seconds in
        achievementsPage.Update(40.0); // 12 -> 52
        Assert::AreEqual(std::wstring(L"1 of 1 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"5 of 5 points - 0h17m"), achievementsPage.GetTitleDetail());

        Assert::IsFalse(achievementsPage.Update(7.9)); // 52 -> 59.9
        Assert::AreEqual(std::wstring(L"1 of 1 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"5 of 5 points - 0h17m"), achievementsPage.GetTitleDetail());

        Assert::IsTrue(achievementsPage.Update(0.2)); // 59.9 -> 60.1
        Assert::AreEqual(std::wstring(L"1 of 1 achievements"), achievementsPage.GetSubTitle());
        Assert::AreEqual(std::wstring(L"5 of 5 points - 0h18m"), achievementsPage.GetTitleDetail());
    }

    TEST_METHOD(TestFetchItemDetailLocal)
    {
        OverlayAchievementsPageViewModelHarness achievementsPage;
        achievementsPage.mockWindowManager.AssetList.SetFilterCategory(AssetListViewModel::FilterCategory::Local);
        achievementsPage.mockAchievementRuntime.MockGame();
        const auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievement(ra::data::context::GameAssets::FirstLocalId);

        achievementsPage.mockServer.ExpectUncalled<ra::api::FetchAchievementInfo>();

        achievementsPage.SetCanCollapseHeaders(false);
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
        achievementsPage.mockAchievementRuntime.MockGame();
        auto* pAch1 = achievementsPage.mockAchievementRuntime.MockAchievement(1234);
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

        achievementsPage.SetCanCollapseHeaders(false);
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
