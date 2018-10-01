#include "CppUnitTest.h"

#include "ui\WindowViewModelBase.hh"

#include "services\ServiceLocator.hh"
#include "ui\IDesktop.hh"

#include "RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace tests {

TEST_CLASS(UI_WindowViewModelBase_Tests)
{
    class WindowViewModelHarness : public WindowViewModelBase
    {
    public:
        WindowViewModelHarness() = default;
    };

    class MockDesktop : public IDesktop, ra::services::ServiceLocator::ServiceOverride<IDesktop>
    {
    public:
        MockDesktop() : ra::services::ServiceLocator::ServiceOverride<IDesktop>(this)
        {
        }

        void ShowWindow(WindowViewModelBase& vmViewModel) const override
        {
            vmShownWindow = &vmViewModel;
        }

        ra::ui::DialogResult ShowModal(WindowViewModelBase& vmViewModel) const override
        {
            vmModalWindow = &vmViewModel;
            return DialogResult::OK;
        }

        void Shutdown() override {}

        mutable WindowViewModelBase* vmShownWindow = nullptr;
        mutable WindowViewModelBase* vmModalWindow = nullptr;
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
    TEST_METHOD(TestWindowTitle)
    {
        WindowViewModelHarness vmViewModel;
        Assert::AreEqual(std::wstring(L"Window"), vmViewModel.GetWindowTitle());

        vmViewModel.SetWindowTitle(L"Test");
        Assert::AreEqual(std::wstring(L"Test"), vmViewModel.GetWindowTitle());
    }

    TEST_METHOD(TestDialogResult)
    {
        WindowViewModelHarness vmViewModel;
        Assert::AreEqual(DialogResult::None, vmViewModel.GetDialogResult());

        vmViewModel.SetDialogResult(DialogResult::OK);
        Assert::AreEqual(DialogResult::OK, vmViewModel.GetDialogResult());
    }

    TEST_METHOD(TestShow)
    {
        MockDesktop mockDesktop;
        WindowViewModelHarness vmViewModel;
        vmViewModel.Show();

        AssertPointer(&vmViewModel, mockDesktop.vmShownWindow);
    }

    TEST_METHOD(TestShowModal)
    {
        MockDesktop mockDesktop;
        WindowViewModelHarness vmViewModel;
        vmViewModel.ShowModal();

        AssertPointer(&vmViewModel, mockDesktop.vmModalWindow);
    }
};

} // namespace tests
} // namespace ui
} // namespace ra
