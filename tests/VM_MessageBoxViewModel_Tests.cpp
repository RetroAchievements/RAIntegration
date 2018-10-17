#include "CppUnitTest.h"

#include "ui\viewmodels\MessageBoxViewModel.hh"

#include "services\ServiceLocator.hh"
#include "ui\IDesktop.hh"

#include "RA_UnitTestHelpers.h"

#undef GetMessage

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

template<> static std::wstring ToString<ra::ui::viewmodels::MessageBoxViewModel::Icon>(const ra::ui::viewmodels::MessageBoxViewModel::Icon& icon)
{
    switch (icon)
    {
        case ra::ui::viewmodels::MessageBoxViewModel::Icon::None: return L"None";
        case ra::ui::viewmodels::MessageBoxViewModel::Icon::Info: return L"Info";
        case ra::ui::viewmodels::MessageBoxViewModel::Icon::Warning: return L"Warning";
        case ra::ui::viewmodels::MessageBoxViewModel::Icon::Error: return L"Error";
        default: return std::to_wstring(static_cast<int>(icon));
    }
}

template<> static std::wstring ToString<ra::ui::viewmodels::MessageBoxViewModel::Buttons>(const ra::ui::viewmodels::MessageBoxViewModel::Buttons& buttons)
{
    switch (buttons)
    {
        case ra::ui::viewmodels::MessageBoxViewModel::Buttons::OK: return L"OK";
        case ra::ui::viewmodels::MessageBoxViewModel::Buttons::OKCancel: return L"OKCancel";
        case ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo: return L"YesNo";
        case ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNoCancel: return L"YesNoCancel";
        case ra::ui::viewmodels::MessageBoxViewModel::Buttons::RetryCancel: return L"RetryCancel";
        default: return std::to_wstring(static_cast<int>(buttons));
    }
}

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(VM_MessageBoxViewModel_Tests)
{
    typedef ra::ui::DialogResult(*DialogHandler)(MessageBoxViewModel&);

    class MockDesktop : public IDesktop, ra::services::ServiceLocator::ServiceOverride<IDesktop>
    {
    public:
        MockDesktop(DialogHandler pHandler) : ra::services::ServiceLocator::ServiceOverride<IDesktop>(this), m_pHandler(pHandler)
        {
        }

        ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel) const override
        {
            m_bDialogShown = true;

            auto* vmMessageBox = dynamic_cast<MessageBoxViewModel*>(&vmViewModel);
            if (vmMessageBox == nullptr)
                return ra::ui::DialogResult::None;

            return m_pHandler(*vmMessageBox);
        }

        void ShowWindow(WindowViewModelBase& vmViewModel) const override {}

        void Shutdown() override {}

        void AssertDialogShown()
        {
            Assert::IsTrue(m_bDialogShown);
        }

    private:
        DialogHandler m_pHandler = nullptr;
        mutable bool m_bDialogShown = false;
    };

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
        MockDesktop mockDesktop([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Hello, world!"), vmMessageBox.GetMessage());
            Assert::AreEqual(std::wstring(), vmMessageBox.GetHeader());
            Assert::AreEqual(MessageBoxViewModel::Icon::None, vmMessageBox.GetIcon());
            Assert::AreEqual(MessageBoxViewModel::Buttons::OK, vmMessageBox.GetButtons());
            return DialogResult::OK;
        });

        MessageBoxViewModel::ShowMessage(L"Hello, world!");

        mockDesktop.AssertDialogShown();
    }

    TEST_METHOD(TestShowInfoMessage)
    {
        MockDesktop mockDesktop([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Something happened."), vmMessageBox.GetMessage());
            Assert::AreEqual(std::wstring(), vmMessageBox.GetHeader());
            Assert::AreEqual(MessageBoxViewModel::Icon::Info, vmMessageBox.GetIcon());
            Assert::AreEqual(MessageBoxViewModel::Buttons::OK, vmMessageBox.GetButtons());
            return DialogResult::OK;
        });

        MessageBoxViewModel::ShowInfoMessage(L"Something happened.");

        mockDesktop.AssertDialogShown();
    }

    TEST_METHOD(TestShowWarningMessage)
    {
        MockDesktop mockDesktop([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Something happened."), vmMessageBox.GetMessage());
            Assert::AreEqual(std::wstring(), vmMessageBox.GetHeader());
            Assert::AreEqual(MessageBoxViewModel::Icon::Warning, vmMessageBox.GetIcon());
            Assert::AreEqual(MessageBoxViewModel::Buttons::OK, vmMessageBox.GetButtons());
            return DialogResult::OK;
        });

        MessageBoxViewModel::ShowWarningMessage(L"Something happened.");

        mockDesktop.AssertDialogShown();
    }

    TEST_METHOD(TestShowHeaderedWarningMessage)
    {
        MockDesktop mockDesktop([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Something happened."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Please try again."), vmMessageBox.GetMessage());
            Assert::AreEqual(MessageBoxViewModel::Icon::Warning, vmMessageBox.GetIcon());
            Assert::AreEqual(MessageBoxViewModel::Buttons::OK, vmMessageBox.GetButtons());
            return DialogResult::OK;
        });

        MessageBoxViewModel::ShowWarningMessage(L"Something happened.", L"Please try again.");

        mockDesktop.AssertDialogShown();
    }

    TEST_METHOD(TestShowErrorMessage)
    {
        MockDesktop mockDesktop([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Something happened."), vmMessageBox.GetMessage());
            Assert::AreEqual(std::wstring(), vmMessageBox.GetHeader());
            Assert::AreEqual(MessageBoxViewModel::Icon::Error, vmMessageBox.GetIcon());
            Assert::AreEqual(MessageBoxViewModel::Buttons::OK, vmMessageBox.GetButtons());
            return DialogResult::OK;
        });

        MessageBoxViewModel::ShowErrorMessage(L"Something happened.");

        mockDesktop.AssertDialogShown();
    }

    TEST_METHOD(TestShowHeaderedErrorMessage)
    {
        MockDesktop mockDesktop([](MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Something happened."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Please try again."), vmMessageBox.GetMessage());
            Assert::AreEqual(MessageBoxViewModel::Icon::Error, vmMessageBox.GetIcon());
            Assert::AreEqual(MessageBoxViewModel::Buttons::OK, vmMessageBox.GetButtons());
            return DialogResult::OK;
        });

        MessageBoxViewModel::ShowErrorMessage(L"Something happened.", L"Please try again.");

        mockDesktop.AssertDialogShown();
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
