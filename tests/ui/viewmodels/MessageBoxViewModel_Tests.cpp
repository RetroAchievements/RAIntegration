#include "CppUnitTest.h"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "services\ServiceLocator.hh"
#include "ui\IDesktop.hh"
#include "tests\mocks\MockDesktop.hh"

#include "tests\RA_UnitTestHelpers.h"

#undef GetMessage

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::ui::mocks::MockDesktop;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(MessageBoxViewModel_Tests)
{
    typedef ra::ui::DialogResult(*DialogHandler)(MessageBoxViewModel&);

    void AssertPointer(const WindowViewModelBase* pExpected, const WindowViewModelBase* pActual)
    {
        if (pExpected != pActual)
        {
            wchar_t buffer[128];
            swprintf_s(buffer, 128, L"%p != %p", pExpected, pActual);
            Assert::Fail(buffer);
        }
    }

public:
    TEST_METHOD(TestMessage)
    {
        MessageBoxViewModel vmMessageBox;
        Assert::AreEqual(std::wstring(L""), vmMessageBox.GetMessage());

        vmMessageBox.SetMessage(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), vmMessageBox.GetMessage());
    }

    TEST_METHOD(TestHeader)
    {
        MessageBoxViewModel vmMessageBox;
        Assert::AreEqual(std::wstring(L""), vmMessageBox.GetHeader());

        vmMessageBox.SetHeader(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), vmMessageBox.GetHeader());
    }

    TEST_METHOD(TestIcon)
    {
        MessageBoxViewModel vmMessageBox;
        Assert::AreEqual(MessageBoxViewModel::Icon::None, vmMessageBox.GetIcon());

        vmMessageBox.SetIcon(MessageBoxViewModel::Icon::Warning);
        Assert::AreEqual(MessageBoxViewModel::Icon::Warning, vmMessageBox.GetIcon());
    }

    TEST_METHOD(TestButtons)
    {
        MessageBoxViewModel vmMessageBox;
        Assert::AreEqual(MessageBoxViewModel::Buttons::OK, vmMessageBox.GetButtons());

        vmMessageBox.SetButtons(MessageBoxViewModel::Buttons::YesNo);
        Assert::AreEqual(MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());
    }

    TEST_METHOD(TestShowMessage)
    {
        MockDesktop mockDesktop;
        mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Hello, world!"), vmMessageBox.GetMessage());
            Assert::AreEqual(std::wstring(), vmMessageBox.GetHeader());
            Assert::AreEqual(MessageBoxViewModel::Icon::None, vmMessageBox.GetIcon());
            Assert::AreEqual(MessageBoxViewModel::Buttons::OK, vmMessageBox.GetButtons());
            return DialogResult::OK;
        });

        MessageBoxViewModel::ShowMessage(L"Hello, world!");

        Assert::IsTrue(mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestShowInfoMessage)
    {
        MockDesktop mockDesktop;
        mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Something happened."), vmMessageBox.GetMessage());
            Assert::AreEqual(std::wstring(), vmMessageBox.GetHeader());
            Assert::AreEqual(MessageBoxViewModel::Icon::Info, vmMessageBox.GetIcon());
            Assert::AreEqual(MessageBoxViewModel::Buttons::OK, vmMessageBox.GetButtons());
            return DialogResult::OK;
        });

        MessageBoxViewModel::ShowInfoMessage(L"Something happened.");

        Assert::IsTrue(mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestShowWarningMessage)
    {
        MockDesktop mockDesktop;
        mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Something happened."), vmMessageBox.GetMessage());
            Assert::AreEqual(std::wstring(), vmMessageBox.GetHeader());
            Assert::AreEqual(MessageBoxViewModel::Icon::Warning, vmMessageBox.GetIcon());
            Assert::AreEqual(MessageBoxViewModel::Buttons::OK, vmMessageBox.GetButtons());
            return DialogResult::OK;
        });

        MessageBoxViewModel::ShowWarningMessage(L"Something happened.");

        Assert::IsTrue(mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestShowHeaderedWarningMessage)
    {
        MockDesktop mockDesktop;
        mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Something happened."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Please try again."), vmMessageBox.GetMessage());
            Assert::AreEqual(MessageBoxViewModel::Icon::Warning, vmMessageBox.GetIcon());
            Assert::AreEqual(MessageBoxViewModel::Buttons::OK, vmMessageBox.GetButtons());
            return DialogResult::OK;
        });

        MessageBoxViewModel::ShowWarningMessage(L"Something happened.", L"Please try again.");

        Assert::IsTrue(mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestShowErrorMessage)
    {
        MockDesktop mockDesktop;
        mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Something happened."), vmMessageBox.GetMessage());
            Assert::AreEqual(std::wstring(), vmMessageBox.GetHeader());
            Assert::AreEqual(MessageBoxViewModel::Icon::Error, vmMessageBox.GetIcon());
            Assert::AreEqual(MessageBoxViewModel::Buttons::OK, vmMessageBox.GetButtons());
            return DialogResult::OK;
        });

        MessageBoxViewModel::ShowErrorMessage(L"Something happened.");

        Assert::IsTrue(mockDesktop.WasDialogShown());
    }

    TEST_METHOD(TestShowHeaderedErrorMessage)
    {
        MockDesktop mockDesktop;
        mockDesktop.ExpectWindow<MessageBoxViewModel>([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Something happened."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Please try again."), vmMessageBox.GetMessage());
            Assert::AreEqual(MessageBoxViewModel::Icon::Error, vmMessageBox.GetIcon());
            Assert::AreEqual(MessageBoxViewModel::Buttons::OK, vmMessageBox.GetButtons());
            return DialogResult::OK;
        });

        MessageBoxViewModel::ShowErrorMessage(L"Something happened.", L"Please try again.");

        Assert::IsTrue(mockDesktop.WasDialogShown());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
