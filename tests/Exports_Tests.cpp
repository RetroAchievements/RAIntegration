#include "CppUnitTest.h"

#include "Exports.hh"

#include "RA_BuildVer.h"

#include "tests\mocks\\MockAudioSystem.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockOverlayManager.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockSessionTracker.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\RA_UnitTestHelpers.h"

#include "ui\viewmodels\LoginViewModel.hh"
#include "ui\viewmodels\MessageBoxViewModel.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::api::mocks::MockServer;
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
};

} // namespace tests
} // namespace ra
