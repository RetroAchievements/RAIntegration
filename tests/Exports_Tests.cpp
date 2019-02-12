#include "CppUnitTest.h"

#include "Exports.hh"

#include "RA_BuildVer.h"

#include "tests\mocks\MockAudioSystem.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockSessionTracker.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\RA_UnitTestHelpers.h"

#include "services\AchievementRuntime.hh"
#include "services\ILeaderboardManager.hh"

#include "ui\viewmodels\LoginViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::api::mocks::MockServer;
using ra::data::mocks::MockGameContext;
using ra::data::mocks::MockSessionTracker;
using ra::data::mocks::MockUserContext;
using ra::services::mocks::MockAudioSystem;
using ra::services::mocks::MockConfiguration;
using ra::services::mocks::MockThreadPool;
using ra::ui::mocks::MockDesktop;
using ra::ui::viewmodels::MessageBoxViewModel;
using ra::ui::viewmodels::mocks::MockOverlayManager;

namespace ra {
namespace tests {

TEST_CLASS(Exports_Tests)
{
public:
    TEST_METHOD(TestIntegrationVersion)
    {
        Assert::AreEqual(RA_INTEGRATION_VERSION, _RA_IntegrationVersion());
    }

    TEST_METHOD(TestHostName)
    {
        MockConfiguration mockConfiguration;

        mockConfiguration.SetHostName("retroachievements.org");
        Assert::AreEqual("retroachievements.org", _RA_HostName());

        mockConfiguration.SetHostName("stage.retroachievements.org");
        Assert::AreEqual("stage.retroachievements.org", _RA_HostName());
    }

    TEST_METHOD(TestHardcoreModeIsActive)
    {
        MockConfiguration mockConfiguration;

        mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, true);
        Assert::AreEqual(1, _RA_HardcoreModeIsActive());

        mockConfiguration.SetFeatureEnabled(ra::services::Feature::Hardcore, false);
        Assert::AreEqual(0, _RA_HardcoreModeIsActive());
    }

    TEST_METHOD(TestAttemptLoginNewUser)
    {
        MockUserContext mockUserContext;
        MockSessionTracker mockSessionTracker;
        MockConfiguration mockConfiguration;
        MockServer mockServer;

        MockDesktop mockDesktop;

        bool bLoginDialogShown = false;
        mockDesktop.ExpectWindow<ra::ui::viewmodels::LoginViewModel>([&bLoginDialogShown](_UNUSED ra::ui::viewmodels::LoginViewModel&)
        {
            bLoginDialogShown = true;
            return ra::ui::DialogResult::OK;
        });

        mockServer.HandleRequest<api::Login>([](_UNUSED const ra::api::Login::Request&, _UNUSED ra::api::Login::Response&)
        {
            Assert::IsFalse(true, L"API called without user info");
            return false;
        });

        Assert::IsFalse(mockUserContext.IsLoggedIn());

        _RA_AttemptLogin(true);

        Assert::IsFalse(mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string(""), mockUserContext.GetUsername());
        Assert::AreEqual(std::wstring(L""), mockSessionTracker.GetUsername());
        Assert::IsTrue(bLoginDialogShown);
    }

    TEST_METHOD(TestAttemptLoginSuccess)
    {
        MockUserContext mockUserContext;
        MockSessionTracker mockSessionTracker;
        MockConfiguration mockConfiguration;
        MockAudioSystem mockAudioSystem;
        MockServer mockServer;
        MockOverlayManager mockOverlayManager;

        bool bLoggedIn = false;
        mockServer.HandleRequest<api::Login>([&bLoggedIn](const ra::api::Login::Request& request, ra::api::Login::Response& response)
        {
            Assert::AreEqual(std::string("User"), request.Username);
            Assert::AreEqual(std::string("ApiToken"), request.ApiToken);
            bLoggedIn = true;

            response.Username = "User";
            response.ApiToken = "ApiToken";
            response.Score = 12345U;
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        mockConfiguration.SetUsername("User");
        mockConfiguration.SetApiToken("ApiToken");

        Assert::IsFalse(mockUserContext.IsLoggedIn());

        _RA_AttemptLogin(true);

        // user context
        Assert::IsTrue(mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string("User"), mockUserContext.GetUsername());
        Assert::AreEqual(std::string("ApiToken"), mockUserContext.GetApiToken());
        Assert::AreEqual(12345U, mockUserContext.GetScore());

        // session context
        Assert::AreEqual(std::wstring(L"User"), mockSessionTracker.GetUsername());

        // popup notification and sound
        Assert::IsTrue(mockAudioSystem.WasAudioFilePlayed(L"Overlay\\login.wav"));
        const auto* pPopup = mockOverlayManager.GetMessage(1);
        Assert::IsNotNull(pPopup);
        Ensures(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Welcome User (12345)"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"You have 0 new messages"), pPopup->GetDescription());
        Assert::AreEqual(ra::ui::ImageType::UserPic, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("User"), pPopup->GetImage().Name());
    }

    TEST_METHOD(TestAttemptLoginSuccessWithMessages)
    {
        MockUserContext mockUserContext;
        MockSessionTracker mockSessionTracker;
        MockConfiguration mockConfiguration;
        MockAudioSystem mockAudioSystem;
        MockServer mockServer;
        MockOverlayManager mockOverlayManager;

        mockServer.HandleRequest<api::Login>([](const ra::api::Login::Request&, ra::api::Login::Response& response)
        {
            response.Username = "User";
            response.ApiToken = "ApiToken";
            response.NumUnreadMessages = 3;
            response.Score = 0U;
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        mockConfiguration.SetUsername("User");
        mockConfiguration.SetApiToken("ApiToken");

        _RA_AttemptLogin(true);

        const auto* pPopup = mockOverlayManager.GetMessage(1);
        Assert::IsNotNull(pPopup);
        Ensures(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Welcome User (0)"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"You have 3 new messages"), pPopup->GetDescription());
        Assert::AreEqual(ra::ui::ImageType::UserPic, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("User"), pPopup->GetImage().Name());
    }

    TEST_METHOD(TestAttemptLoginSuccessWithPreviousSessionData)
    {
        MockUserContext mockUserContext;
        MockSessionTracker mockSessionTracker;
        MockConfiguration mockConfiguration;
        MockAudioSystem mockAudioSystem;
        MockServer mockServer;
        MockOverlayManager mockOverlayManager;

        mockServer.HandleRequest<api::Login>([](const ra::api::Login::Request&, ra::api::Login::Response& response)
        {
            response.Username = "User";
            response.ApiToken = "ApiToken";
            response.Score = 12345U;
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        mockConfiguration.SetUsername("User");
        mockConfiguration.SetApiToken("ApiToken");

        mockSessionTracker.MockSession(6U, 123456789, std::chrono::hours(2));

        _RA_AttemptLogin(true);

        const auto* pPopup = mockOverlayManager.GetMessage(1);
        Assert::IsNotNull(pPopup);
        Ensures(pPopup != nullptr);
        Assert::AreEqual(std::wstring(L"Welcome back User (12345)"), pPopup->GetTitle());
        Assert::AreEqual(std::wstring(L"You have 0 new messages"), pPopup->GetDescription());
        Assert::AreEqual(ra::ui::ImageType::UserPic, pPopup->GetImage().Type());
        Assert::AreEqual(std::string("User"), pPopup->GetImage().Name());
    }

    TEST_METHOD(TestAttemptLoginInvalid)
    {
        MockUserContext mockUserContext;
        MockSessionTracker mockSessionTracker;
        MockConfiguration mockConfiguration;
        MockServer mockServer;
        MockDesktop mockDesktop;

        mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Login Failed"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Invalid user/password combination. Please try again."), vmMessageBox.GetMessage());
            Assert::AreEqual(MessageBoxViewModel::Icon::Error, vmMessageBox.GetIcon());

            return ra::ui::DialogResult::OK;
        });

        mockServer.HandleRequest<api::Login>([](_UNUSED const ra::api::Login::Request&, ra::api::Login::Response& response)
        {
            response.ErrorMessage = "Invalid user/password combination. Please try again.";
            response.Result = ra::api::ApiResult::Error;
            return true;
        });

        mockConfiguration.SetUsername("User");
        mockConfiguration.SetApiToken("ApiToken");

        _RA_AttemptLogin(true);

        Assert::IsTrue(mockDesktop.WasDialogShown());
        Assert::IsFalse(mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string(""), mockUserContext.GetUsername());
        Assert::AreEqual(std::string(""), mockUserContext.GetApiToken());
        Assert::AreEqual(0U, mockUserContext.GetScore());

        Assert::AreEqual(std::wstring(L""), mockSessionTracker.GetUsername());
    }

    TEST_METHOD(TestAttemptLoginDisabled)
    {
        MockUserContext mockUserContext;
        MockSessionTracker mockSessionTracker;
        MockConfiguration mockConfiguration;
        MockServer mockServer;
        MockDesktop mockDesktop;

        mockConfiguration.SetUsername("User");
        mockConfiguration.SetApiToken("ApiToken");

        mockUserContext.DisableLogin();
        _RA_AttemptLogin(true);

        Assert::IsFalse(mockDesktop.WasDialogShown());
        Assert::IsFalse(mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string(""), mockUserContext.GetUsername());
        Assert::AreEqual(std::string(""), mockUserContext.GetApiToken());
        Assert::AreEqual(0U, mockUserContext.GetScore());

        Assert::AreEqual(std::wstring(L""), mockSessionTracker.GetUsername());
    }

    TEST_METHOD(TestAttemptLoginDisabledDuringRequest)
    {
        MockUserContext mockUserContext;
        MockSessionTracker mockSessionTracker;
        MockConfiguration mockConfiguration;
        MockServer mockServer;
        MockDesktop mockDesktop;
        MockThreadPool mockThreadPool;

        mockConfiguration.SetUsername("User");
        mockConfiguration.SetApiToken("ApiToken");

        _RA_AttemptLogin(false);

        mockUserContext.DisableLogin();
        mockThreadPool.ExecuteNextTask();

        Assert::IsFalse(mockDesktop.WasDialogShown());
        Assert::IsFalse(mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string(""), mockUserContext.GetUsername());
        Assert::AreEqual(std::string(""), mockUserContext.GetApiToken());
        Assert::AreEqual(0U, mockUserContext.GetScore());

        Assert::AreEqual(std::wstring(L""), mockSessionTracker.GetUsername());
    }

private:
    class MockAchievementRuntime : public ra::services::AchievementRuntime
    {
    public:
        MockAchievementRuntime() noexcept : m_Override(this) {}

        void QueueChange(ChangeType nType, unsigned int nId, unsigned int nValue = 0)
        {
            m_vChanges.emplace_back(Change{ nType, nId, nValue });
        }

        void Process(_Inout_ std::vector<Change>& changes) override
        {
            changes.insert(changes.begin(), m_vChanges.begin(), m_vChanges.end());
            m_vChanges.clear();
        }

    private:
        std::vector<ra::services::AchievementRuntime::Change> m_vChanges;
        ra::services::ServiceLocator::ServiceOverride<ra::services::AchievementRuntime> m_Override;
    };

    class MockLeaderboardManager : public ra::services::ILeaderboardManager
    {
    public:
        MockLeaderboardManager() : m_Override(this) {}

        void ActivateLeaderboard(const RA_Leaderboard& lb) const override
        {
            m_vActiveLeaderboards.insert(lb.ID());
        }

        void DeactivateLeaderboard(const RA_Leaderboard& lb) const override
        {
            m_vActiveLeaderboards.erase(lb.ID());
        }

        bool IsLeaderboardActive(ra::LeaderboardID nId) const
        {
            return m_vActiveLeaderboards.find(nId) != m_vActiveLeaderboards.end();
        }

        void SubmitLeaderboardEntry(const RA_Leaderboard& lb, unsigned int nValue) const override
        {
            m_mSubmittedScores.insert_or_assign(lb.ID(), nValue);
        }

        unsigned int GetSubmittedValue(ra::LeaderboardID nId) const
        {
            const auto pIter = m_mSubmittedScores.find(nId);
            return (pIter != m_mSubmittedScores.end()) ? pIter->second : 0U;
        }

        void DeactivateLeaderboards() const override {}

        void AddLeaderboard(RA_Leaderboard&& lb) override
        {
            m_vLeaderboards.emplace_back(std::move(lb));
        }

        size_t Count() const override { return m_vLeaderboards.size(); }

        const RA_Leaderboard& GetLB(size_t index) const override
        {
            return m_vLeaderboards.at(index);
        }

        RA_Leaderboard* FindLB(LeaderboardID nID) noexcept override
        {
            for (auto& pLeaderboard : m_vLeaderboards)
            {
                if (pLeaderboard.ID() == nID)
                    return &pLeaderboard;
            }

            return nullptr;
        }

        const RA_Leaderboard* FindLB(LeaderboardID nID) const noexcept override
        {
            for (auto& pLeaderboard : m_vLeaderboards)
            {
                if (pLeaderboard.ID() == nID)
                    return &pLeaderboard;
            }

            return nullptr;
        }

        void Clear() { m_vLeaderboards.clear(); }

    private:
        ra::services::ServiceLocator::ServiceOverride<ra::services::ILeaderboardManager> m_Override;
        std::vector<RA_Leaderboard> m_vLeaderboards;
        mutable std::set<ra::LeaderboardID> m_vActiveLeaderboards;
        mutable std::map<ra::LeaderboardID, unsigned int> m_mSubmittedScores;
    };

    class DoAchievementsFrameHarness
    {
    public:
        MockAchievementRuntime mockRuntime;
        MockGameContext mockGameContext;
        MockDesktop mockDesktop;
        MockLeaderboardManager mockLeaderboardManager;

        DoAchievementsFrameHarness()
        {
            mockServer.HandleRequest<ra::api::AwardAchievement>([this](const ra::api::AwardAchievement::Request& request, ra::api::AwardAchievement::Response& response)
            {
                m_vUnlockedAchievements.insert(request.AchievementId);
                response.Result = ra::api::ApiResult::Success;

                return true;
            });
        }

        void MockAchievement(unsigned int nId)
        {
            auto& pAch = mockGameContext.NewAchievement(AchievementSet::Type::Core);
            pAch.SetID(nId);
        }

        bool WasUnlocked(unsigned int nId)
        {
            mockThreadPool.ExecuteNextTask();
            return m_vUnlockedAchievements.find(nId) != m_vUnlockedAchievements.end();
        }

    private:
        MockThreadPool mockThreadPool;
        MockServer mockServer;
        MockAudioSystem mockAudioSystem;
        MockUserContext mockUserContext;
        MockOverlayManager mockOverlayManager;
        MockConfiguration mockConfiguration;

        std::set<unsigned int> m_vUnlockedAchievements;
    };

public:
    TEST_METHOD(TestDoAchievementsFrameAchievementTriggered)
    {
        DoAchievementsFrameHarness harness;
        harness.MockAchievement(1U);
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementTriggered, 1U);

        _RA_DoAchievementsFrame();

        Assert::IsTrue(harness.WasUnlocked(1U));
        Assert::IsFalse(harness.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestDoAchievementsFramePaused)
    {
        DoAchievementsFrameHarness harness;
        harness.MockAchievement(1U);
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementTriggered, 1U);
        harness.mockRuntime.SetPaused(true);

        _RA_DoAchievementsFrame();
        Assert::IsFalse(harness.WasUnlocked(1U));

        harness.mockRuntime.SetPaused(false);
        _RA_DoAchievementsFrame();
        Assert::IsTrue(harness.WasUnlocked(1U));
    }

    TEST_METHOD(TestDoAchievementsFramePauseOnTrigger)
    {
        DoAchievementsFrameHarness harness;
        harness.MockAchievement(1U);
        harness.mockGameContext.FindAchievement(1U)->SetPauseOnTrigger(true);
        harness.mockGameContext.FindAchievement(1U)->SetTitle("AchievementTitle");
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementTriggered, 1U);

        bool bMessageSeen = false;
        harness.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bMessageSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bMessageSeen = true;
            Assert::AreEqual(std::wstring(L"Pause on Trigger: AchievementTitle"), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        _RA_DoAchievementsFrame();

        Assert::IsTrue(harness.WasUnlocked(1U));
        Assert::IsTrue(bMessageSeen);
    }

    TEST_METHOD(TestDoAchievementsFramePauseOnReset)
    {
        DoAchievementsFrameHarness harness;
        harness.MockAchievement(1U);
        harness.mockGameContext.FindAchievement(1U)->SetPauseOnReset(true);
        harness.mockGameContext.FindAchievement(1U)->SetTitle("AchievementTitle");
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementReset, 1U);

        bool bMessageSeen = false;
        harness.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bMessageSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bMessageSeen = true;
            Assert::AreEqual(std::wstring(L"Pause on Reset: AchievementTitle"), vmMessageBox.GetMessage());
            return ra::ui::DialogResult::OK;
        });

        _RA_DoAchievementsFrame();

        Assert::IsTrue(bMessageSeen);
    }

    TEST_METHOD(TestDoAchievementsFrameLeaderboardStart)
    {
        DoAchievementsFrameHarness harness;
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::LeaderboardStarted, 1U, 1234U);
        harness.mockLeaderboardManager.AddLeaderboard(RA_Leaderboard(1U));

        _RA_DoAchievementsFrame();

        Assert::AreEqual(1234U, harness.mockLeaderboardManager.FindLB(1U)->GetCurrentValue());
        Assert::IsTrue(harness.mockLeaderboardManager.IsLeaderboardActive(1U));
    }

    TEST_METHOD(TestDoAchievementsFrameLeaderboardCanceled)
    {
        DoAchievementsFrameHarness harness;
        harness.mockLeaderboardManager.AddLeaderboard(RA_Leaderboard(1U));
        harness.mockLeaderboardManager.ActivateLeaderboard(*harness.mockLeaderboardManager.FindLB(1U));
        Assert::IsTrue(harness.mockLeaderboardManager.IsLeaderboardActive(1U));

        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::LeaderboardCanceled, 1U);
        _RA_DoAchievementsFrame();

        Assert::IsFalse(harness.mockLeaderboardManager.IsLeaderboardActive(1U));
    }

    TEST_METHOD(TestDoAchievementsFrameLeaderboardUpdated)
    {
        DoAchievementsFrameHarness harness;
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::LeaderboardUpdated, 1U, 1235U);
        harness.mockLeaderboardManager.AddLeaderboard(RA_Leaderboard(1U));

        _RA_DoAchievementsFrame();

        Assert::AreEqual(1235U, harness.mockLeaderboardManager.FindLB(1U)->GetCurrentValue());
    }

    TEST_METHOD(TestDoAchievementsFrameLeaderboardTriggered)
    {
        DoAchievementsFrameHarness harness;
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::LeaderboardTriggered, 1U, 1236U);
        harness.mockLeaderboardManager.AddLeaderboard(RA_Leaderboard(1U));

        _RA_DoAchievementsFrame();

        Assert::AreEqual(1236U, harness.mockLeaderboardManager.GetSubmittedValue(1U));
    }

    TEST_METHOD(TestDoAchievementsFrameMultiple)
    {
        DoAchievementsFrameHarness harness;
        harness.MockAchievement(1U);
        harness.MockAchievement(2U);
        harness.mockLeaderboardManager.AddLeaderboard(RA_Leaderboard(3U));
        harness.mockLeaderboardManager.AddLeaderboard(RA_Leaderboard(4U));
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::LeaderboardStarted, 3U, 1234U);
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementTriggered, 1U);
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::AchievementTriggered, 2U);
        harness.mockRuntime.QueueChange(ra::services::AchievementRuntime::ChangeType::LeaderboardStarted, 4U, 5678U);

        _RA_DoAchievementsFrame();

        Assert::IsTrue(harness.WasUnlocked(1U));
        Assert::IsTrue(harness.WasUnlocked(2U));
        Assert::AreEqual(1234U, harness.mockLeaderboardManager.FindLB(3U)->GetCurrentValue());
        Assert::IsTrue(harness.mockLeaderboardManager.IsLeaderboardActive(3U));
        Assert::AreEqual(5678U, harness.mockLeaderboardManager.FindLB(4U)->GetCurrentValue());
        Assert::IsTrue(harness.mockLeaderboardManager.IsLeaderboardActive(4U));
    }
};

} // namespace tests
} // namespace ra
