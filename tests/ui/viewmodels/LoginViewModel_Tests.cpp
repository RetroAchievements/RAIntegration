#include "CppUnitTest.h"

#include "ui\viewmodels\LoginViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockUserContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using ra::api::mocks::MockServer;
using ra::data::mocks::MockUserContext;
using ra::services::mocks::MockConfiguration;
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
        MockDesktop mockDesktop;
        MockServer mockServer;
        MockUserContext mockUserContext;
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
        vmLogin.mockServer.HandleRequest<ra::api::Login>([](_UNUSED const ra::api::Login::Request&, _UNUSED ra::api::Login::Response&)
        {
            Assert::IsFalse(true, L"API called with invalid inputs");
            return false;
        });
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
    }

    TEST_METHOD(TestLoginNoPassword)
    {
        LoginViewModelHarness vmLogin;
        vmLogin.mockServer.HandleRequest<ra::api::Login>([](_UNUSED const ra::api::Login::Request&, _UNUSED ra::api::Login::Response&)
        {
            Assert::IsFalse(true, L"API called with invalid inputs");
            return false;
        });
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
    }

    TEST_METHOD(TestLoginSuccessful)
    {
        LoginViewModelHarness vmLogin;
        vmLogin.mockServer.HandleRequest<ra::api::Login>([](_UNUSED const ra::api::Login::Request&, ra::api::Login::Response& response)
        {
            response.Username = "User";
            response.ApiToken = "ApiToken";
            response.Score = 12345;
            response.Result = ra::api::ApiResult::Success;
            return true;
        });
        vmLogin.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Successfully logged in as User"), vmMessageBox.GetMessage());
            Assert::AreEqual(MessageBoxViewModel::Icon::Info, vmMessageBox.GetIcon());
            return DialogResult::OK;
        });

        vmLogin.SetUsername(L"user");
        vmLogin.SetPassword(L"Pa$$w0rd");
        Assert::IsTrue(vmLogin.Login());
        Assert::IsTrue(vmLogin.mockDesktop.WasDialogShown());

        // expect case of username to be corrected by API response
        Assert::AreEqual(std::string("User"), vmLogin.mockConfiguration.GetUsername());

        // API token should not be set unless "remember me" is checked
        Assert::AreEqual(std::string(""), vmLogin.mockConfiguration.GetApiToken());

        // values should also be updated in UserContext, including API token
        Assert::AreEqual(std::string("User"), vmLogin.mockUserContext.GetUsername());
        Assert::AreEqual(std::string("ApiToken"), vmLogin.mockUserContext.GetApiToken());
    }

    TEST_METHOD(TestLoginInvalidPassword)
    {
        LoginViewModelHarness vmLogin;
        vmLogin.mockServer.HandleRequest<ra::api::Login>([](_UNUSED const ra::api::Login::Request&, ra::api::Login::Response& response)
        {
            response.ErrorMessage = "Invalid User/Password combination. Please try again\n";
            response.Result = ra::api::ApiResult::Error;
            return true;
        });
        vmLogin.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Failed to login"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Invalid User/Password combination. Please try again\n"), vmMessageBox.GetMessage());
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
        vmLogin.mockServer.HandleRequest<ra::api::Login>([](_UNUSED const ra::api::Login::Request&, ra::api::Login::Response& response)
        {
            response.Username = "User";
            response.ApiToken = "ApiToken";
            response.Score = 12345;
            response.Result = ra::api::ApiResult::Success;
            return true;
        });
        vmLogin.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Successfully logged in as User"), vmMessageBox.GetMessage());
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
