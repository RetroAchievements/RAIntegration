#include "CppUnitTest.h"

#include "ui\viewmodels\LoginViewModel.hh"

#include "tests\ui\UIAsserts.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockRcheevosClient.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockSessionTracker.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using ra::api::mocks::MockServer;
using ra::data::context::mocks::MockEmulatorContext;
using ra::data::context::mocks::MockSessionTracker;
using ra::data::context::mocks::MockUserContext;
using ra::services::mocks::MockConfiguration;
using ra::services::mocks::MockRcheevosClient;

using ra::ui::mocks::MockDesktop;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(LoginViewModel_Tests)
{
private:
    class LoginViewModelHarness : public LoginViewModel
    {
    public:
        GSL_SUPPRESS_F6 LoginViewModelHarness() : LoginViewModel(L"User") {}

        MockConfiguration mockConfiguration;
        MockRcheevosClient mockRcheevosClient;
        MockDesktop mockDesktop;
        MockServer mockServer;
        MockUserContext mockUserContext;
        MockEmulatorContext mockEmulatorContext;
        MockSessionTracker mockSessionTracker;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;
    };

public:
    TEST_METHOD(TestInitialValueFromConfiguration)
    {
        MockConfiguration mockConfiguration;
        mockConfiguration.SetUsername("Flower");
        LoginViewModel vmLogin;
        Assert::AreEqual(std::wstring(L"Flower"), vmLogin.GetUsername());
        Assert::AreEqual(std::wstring(L""), vmLogin.GetPassword());
        Assert::IsFalse(vmLogin.IsPasswordRemembered());
    }

    TEST_METHOD(TestLoginNoUsername)
    {
        LoginViewModelHarness vmLogin;
        vmLogin.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Username is required."), vmMessageBox.GetMessage());
            Assert::AreEqual(MessageBoxViewModel::Icon::Error, vmMessageBox.GetIcon());
            return DialogResult::OK;
        });

        vmLogin.SetUsername(L"");
        vmLogin.SetPassword(L"Pa$$w0rd");
        Assert::IsFalse(vmLogin.Login());
        Assert::IsTrue(vmLogin.mockDesktop.WasDialogShown());
        vmLogin.mockRcheevosClient.AssertNoPendingRequests();
    }

    TEST_METHOD(TestLoginNoPassword)
    {
        LoginViewModelHarness vmLogin;
        vmLogin.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Password is required."), vmMessageBox.GetMessage());
            Assert::AreEqual(MessageBoxViewModel::Icon::Error, vmMessageBox.GetIcon());
            return DialogResult::OK;
        });

        vmLogin.SetUsername(L"User");
        vmLogin.SetPassword(L"");
        Assert::IsFalse(vmLogin.Login());
        Assert::IsTrue(vmLogin.mockDesktop.WasDialogShown());
        vmLogin.mockRcheevosClient.AssertNoPendingRequests();
    }

    TEST_METHOD(TestLoginSuccessful)
    {
        LoginViewModelHarness vmLogin;
        vmLogin.mockRcheevosClient.MockResponse("r=login&u=user&p=Pa%24%24w0rd",
            "{\"Success\":true,\"User\":\"User\",\"DisplayName\":\"UserDisplay\","
            "\"Token\":\"ApiToken\",\"Score\":12345,\"SoftcoreScore\":123,"
            "\"Messages\":0,\"Permissions\":1,\"AccountType\":\"Registered\"}");

        vmLogin.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Successfully logged in as UserDisplay"), vmMessageBox.GetMessage());
            Assert::AreEqual(MessageBoxViewModel::Icon::Info, vmMessageBox.GetIcon());
            return DialogResult::OK;
        });
        bool bWasMenuRebuilt = false;
        vmLogin.mockEmulatorContext.SetRebuildMenuFunction([&bWasMenuRebuilt] { bWasMenuRebuilt = true; });

        vmLogin.SetUsername(L"user");
        vmLogin.SetPassword(L"Pa$$w0rd");
        Assert::IsTrue(vmLogin.Login());
        Assert::IsTrue(vmLogin.mockDesktop.WasDialogShown());

        // expect case of username to be corrected by API response
        Assert::AreEqual(std::string("User"), vmLogin.mockConfiguration.GetUsername());

        // API token should not be set unless "remember me" is checked
        Assert::AreEqual(std::string(""), vmLogin.mockConfiguration.GetApiToken());

        // values should also be updated in UserContext, including API token and score
        Assert::AreEqual(std::string("User"), vmLogin.mockUserContext.GetUsername());
        Assert::AreEqual(std::string("ApiToken"), vmLogin.mockUserContext.GetApiToken());
        Assert::AreEqual(12345U, vmLogin.mockUserContext.GetScore());

        // session tracker should know user name
        Assert::AreEqual(std::wstring(L"User"), vmLogin.mockSessionTracker.GetUsername());

        // emulator should have been notified to rebuild the RetroAchievements menu
        Assert::IsTrue(bWasMenuRebuilt);

        // app title should not be updated as _RA_UpdateAppTitle was not previously called
        Assert::AreEqual(std::wstring(L"Window"), vmLogin.mockWindowManager.Emulator.GetWindowTitle());
    }

    TEST_METHOD(TestLoginSuccessfulAppTitle)
    {
        LoginViewModelHarness vmLogin;
        vmLogin.mockRcheevosClient.MockResponse("r=login&u=user&p=Pa%24%24w0rd",
            "{\"Success\":true,\"User\":\"User\",\"DisplayName\":\"UserDisplay\","
            "\"Token\":\"ApiToken\",\"Score\":12345,\"SoftcoreScore\":123,"
            "\"Messages\":0,\"Permissions\":1,\"AccountType\":\"Registered\"}");

        vmLogin.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Successfully logged in as UserDisplay"), vmMessageBox.GetMessage());
            Assert::AreEqual(MessageBoxViewModel::Icon::Info, vmMessageBox.GetIcon());
            return DialogResult::OK;
        });
        bool bWasMenuRebuilt = false;
        vmLogin.mockEmulatorContext.SetRebuildMenuFunction([&bWasMenuRebuilt] { bWasMenuRebuilt = true; });
        vmLogin.mockEmulatorContext.MockClient("RATests", "0.1.2.0");
        vmLogin.mockWindowManager.Emulator.SetAppTitleMessage("Test");

        vmLogin.SetUsername(L"user");
        vmLogin.SetPassword(L"Pa$$w0rd");
        Assert::IsTrue(vmLogin.Login());
        Assert::IsTrue(vmLogin.mockDesktop.WasDialogShown());

        // expect case of username to be corrected by API response
        Assert::AreEqual(std::string("User"), vmLogin.mockConfiguration.GetUsername());

        // API token should not be set unless "remember me" is checked
        Assert::AreEqual(std::string(""), vmLogin.mockConfiguration.GetApiToken());

        // values should also be updated in UserContext, including API token and score
        Assert::AreEqual(std::string("User"), vmLogin.mockUserContext.GetUsername());
        Assert::AreEqual(std::string("ApiToken"), vmLogin.mockUserContext.GetApiToken());
        Assert::AreEqual(12345U, vmLogin.mockUserContext.GetScore());

        // session tracker should know user name
        Assert::AreEqual(std::wstring(L"User"), vmLogin.mockSessionTracker.GetUsername());

        // emulator should have been notified to rebuild the RetroAchievements menu
        Assert::IsTrue(bWasMenuRebuilt);

        // app title should be updated as _RA_UpdateAppTitle was previously called
        Assert::AreEqual(std::wstring(L"RATests - 0.1 - Test - UserDisplay []"), vmLogin.mockWindowManager.Emulator.GetWindowTitle());
    }

    TEST_METHOD(TestLoginInvalidPassword)
    {
        LoginViewModelHarness vmLogin;
        vmLogin.mockRcheevosClient.MockResponse("r=login&u=User&p=Pa%24%24w0rd",
            "{\"Success\":false,\"Error\":\"Invalid User/Password combination. Please try again\"}");

        vmLogin.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Failed to login"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Invalid User/Password combination. Please try again"), vmMessageBox.GetMessage());
            Assert::AreEqual(MessageBoxViewModel::Icon::Error, vmMessageBox.GetIcon());
            return DialogResult::OK;
        });

        vmLogin.SetUsername(L"User");
        vmLogin.SetPassword(L"Pa$$w0rd");
        Assert::IsFalse(vmLogin.Login());
        Assert::IsTrue(vmLogin.mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestLoginSuccessfulRememberPassword)
    {
        LoginViewModelHarness vmLogin;
        vmLogin.mockRcheevosClient.MockResponse("r=login&u=user&p=Pa%24%24w0rd",
            "{\"Success\":true,\"User\":\"User\",\"DisplayName\":\"UserDisplay\","
            "\"Token\":\"ApiToken\",\"Score\":12345,\"SoftcoreScore\":123,"
            "\"Messages\":0,\"Permissions\":1,\"AccountType\":\"Registered\"}");

        vmLogin.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Successfully logged in as UserDisplay"), vmMessageBox.GetMessage());
            Assert::AreEqual(MessageBoxViewModel::Icon::Info, vmMessageBox.GetIcon());
            return DialogResult::OK;
        });

        vmLogin.SetUsername(L"user");
        vmLogin.SetPassword(L"Pa$$w0rd");
        vmLogin.SetPasswordRemembered(true);
        Assert::IsTrue(vmLogin.Login());
        Assert::IsTrue(vmLogin.mockDesktop.WasDialogShown());

        Assert::AreEqual(std::string("User"), vmLogin.mockConfiguration.GetUsername());
        Assert::AreEqual(std::string("ApiToken"), vmLogin.mockConfiguration.GetApiToken());
        Assert::AreEqual(std::string("User"), vmLogin.mockUserContext.GetUsername());
        Assert::AreEqual(std::string("ApiToken"), vmLogin.mockUserContext.GetApiToken());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
