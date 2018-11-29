#include "CppUnitTest.h"

#include "Exports.hh"

#include "RA_BuildVer.h"

#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockSessionTracker.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\RA_UnitTestHelpers.h"

#include "ui\viewmodels\MessageBoxViewModel.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::api::mocks::MockServer;
using ra::data::mocks::MockSessionTracker;
using ra::data::mocks::MockUserContext;
using ra::services::mocks::MockConfiguration;
using ra::ui::mocks::MockDesktop;
using ra::ui::viewmodels::MessageBoxViewModel;

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

    TEST_METHOD(TestAttemptLoginSuccess)
    {
        MockUserContext mockUserContext;
        MockSessionTracker mockSessionTracker;
        MockConfiguration mockConfiguration;
        MockServer mockServer;

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

        Assert::IsTrue(mockUserContext.IsLoggedIn());
        Assert::AreEqual(std::string("User"), mockUserContext.GetUsername());
        Assert::AreEqual(std::string("ApiToken"), mockUserContext.GetApiToken());
        Assert::AreEqual(12345U, mockUserContext.GetScore());

        Assert::AreEqual(std::wstring(L"User"), mockSessionTracker.GetUsername());
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

        mockServer.HandleRequest<api::Login>([](_UNUSED const ra::api::Login::Request& request, ra::api::Login::Response& response)
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
};

} // namespace tests
} // namespace ra
