#include "ui\WindowViewModelBase.hh"

#include "services\ServiceLocator.hh"
#include "ui\IDesktop.hh"
#include "tests\mocks\MockDesktop.hh"

#include "tests\RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::ui::mocks::MockDesktop;

namespace ra {
namespace ui {
namespace tests {

TEST_CLASS(WindowViewModelBase_Tests)
{
    class WindowViewModelHarness : public WindowViewModelBase
    {
    public:
        WindowViewModelHarness()
            noexcept(std::is_nothrow_default_constructible_v<WindowViewModelBase>) = default;
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
        WindowViewModelBase* vmShownWindow = nullptr;
        MockDesktop mockDesktop;
        mockDesktop.ExpectWindow<WindowViewModelHarness>([&vmShownWindow](WindowViewModelHarness& vmWindow)
        {
            vmShownWindow = &vmWindow;
            return DialogResult::OK;
        });

        WindowViewModelHarness vmViewModel;
        vmViewModel.Show();

        AssertPointer(&vmViewModel, vmShownWindow);
    }

    TEST_METHOD(TestShowModal)
    {
        WindowViewModelBase* vmShownWindow = nullptr;
        MockDesktop mockDesktop;
        mockDesktop.ExpectWindow<WindowViewModelHarness>([&vmShownWindow](WindowViewModelHarness& vmWindow)
        {
            vmShownWindow = &vmWindow;
            return DialogResult::OK;
        });

        WindowViewModelHarness vmViewModel;
        vmViewModel.ShowModal();

        AssertPointer(&vmViewModel, vmShownWindow);
    }
};

} // namespace tests
} // namespace ui
} // namespace ra
