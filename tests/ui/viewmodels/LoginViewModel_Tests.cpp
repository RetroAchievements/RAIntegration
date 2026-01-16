#include "CppUnitTest.h"

#include "ui\viewmodels\LoginViewModel.hh"

#include "tests\ui\UIAsserts.hh"
#include "tests\devkit\context\mocks\MockRcClient.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockLoginService.hh"
#include "tests\mocks\MockUserContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

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

        ra::context::mocks::MockRcClient mockRcClient;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockLoginService mockLoginService;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::data::context::mocks::MockUserContext mockUserContext;
    };

public:
    TEST_METHOD(TestInitialValueFromConfiguration)
    {
        ra::services::mocks::MockConfiguration mockConfiguration;
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
        vmLogin.mockRcClient.AssertNoPendingRequests();
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
        vmLogin.mockRcClient.AssertNoPendingRequests();
    }

    TEST_METHOD(TestLoginSuccessful)
    {
        LoginViewModelHarness vmLogin;
        vmLogin.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Successfully logged in as User_"), vmMessageBox.GetMessage());
            Assert::AreEqual(MessageBoxViewModel::Icon::Info, vmMessageBox.GetIcon());
            return DialogResult::OK;
        });

        vmLogin.SetUsername(L"User");
        vmLogin.SetPassword(L"Pa$$w0rd");
        Assert::IsTrue(vmLogin.Login());
        Assert::IsTrue(vmLogin.mockDesktop.WasDialogShown());

        // API token should not be set unless "remember me" is checked
        Assert::AreEqual(std::string("User_"), vmLogin.mockConfiguration.GetUsername());
        Assert::AreEqual(std::string(""), vmLogin.mockConfiguration.GetApiToken());
    }

    TEST_METHOD(TestLoginSuccessfulRememberPassword)
    {
        LoginViewModelHarness vmLogin;
        vmLogin.mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Successfully logged in as User_"), vmMessageBox.GetMessage());
            Assert::AreEqual(MessageBoxViewModel::Icon::Info, vmMessageBox.GetIcon());
            return DialogResult::OK;
        });

        vmLogin.SetUsername(L"User");
        vmLogin.SetPassword(L"Pa$$w0rd");
        vmLogin.SetPasswordRemembered(true);
        Assert::IsTrue(vmLogin.Login());
        Assert::IsTrue(vmLogin.mockDesktop.WasDialogShown());

        Assert::AreEqual(std::string("User_"), vmLogin.mockConfiguration.GetUsername());
        Assert::AreEqual(std::string("APITOKEN"), vmLogin.mockConfiguration.GetApiToken());
    }

    TEST_METHOD(TestLoginFailure)
    {
        LoginViewModelHarness vmLogin;
        vmLogin.mockLoginService.MockLoginFailure(true);

        vmLogin.SetUsername(L"User");
        vmLogin.SetPassword(L"Pa$$w0rd");
        Assert::IsFalse(vmLogin.Login());

        Assert::AreEqual(std::string(""), vmLogin.mockConfiguration.GetUsername());
        Assert::AreEqual(std::string(""), vmLogin.mockConfiguration.GetApiToken());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
