#include "services\FrameEventQueue.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace services {
namespace tests {

TEST_CLASS(FrameEventQueue_Tests)
{
private:
    class FrameEventQueueHarness : public FrameEventQueue
    {
    public:
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::ui::mocks::MockDesktop mockDesktop;

        size_t NumTriggeredTriggers() const noexcept { return m_vTriggeredTriggers.size(); }
        size_t NumResetTriggers() const noexcept { return m_vResetTriggers.size(); }
        size_t NumMemoryChanges() const noexcept { return m_vMemChanges.size(); }
    };

public:
    TEST_METHOD(TestInitialization)
    {
        FrameEventQueueHarness eventQueue;
        Assert::AreEqual({ 0U }, eventQueue.NumMemoryChanges());
        Assert::AreEqual({ 0U }, eventQueue.NumResetTriggers());
        Assert::AreEqual({ 0U }, eventQueue.NumMemoryChanges());

        eventQueue.DoFrame();

        Assert::IsFalse(eventQueue.mockDesktop.WasDialogShown());
        Assert::AreEqual({ 0U }, eventQueue.NumMemoryChanges());
        Assert::AreEqual({ 0U }, eventQueue.NumResetTriggers());
        Assert::AreEqual({ 0U }, eventQueue.NumMemoryChanges());
    }

    TEST_METHOD(TestPauseOnReset)
    {
        FrameEventQueueHarness eventQueue;
        bool bPaused = false;
        eventQueue.mockEmulatorContext.SetPauseFunction([&bPaused]() { bPaused = true; });

        bool bSawDialog = false;
        eventQueue.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bSawDialog](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bSawDialog = true;

            Assert::AreEqual(std::wstring(L"The emulator has been paused."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The following triggers have been reset:\n* You did it!"), vmMessageBox.GetMessage());

            return ra::ui::DialogResult::OK;
        });

        eventQueue.QueuePauseOnReset(L"You did it!");
        Assert::AreEqual({ 0U }, eventQueue.NumMemoryChanges());
        Assert::AreEqual({ 1U }, eventQueue.NumResetTriggers());
        Assert::AreEqual({ 0U }, eventQueue.NumTriggeredTriggers());
        Assert::IsFalse(bSawDialog);
        Assert::IsFalse(bPaused);

        eventQueue.DoFrame();
        Assert::IsTrue(bSawDialog);
        Assert::IsTrue(bPaused);
        Assert::AreEqual({ 0U }, eventQueue.NumMemoryChanges());
        Assert::AreEqual({ 0U }, eventQueue.NumResetTriggers());
        Assert::AreEqual({ 0U }, eventQueue.NumTriggeredTriggers());

        bSawDialog = false;
        bPaused = false;
        eventQueue.DoFrame();
        Assert::IsFalse(bSawDialog);
        Assert::IsFalse(bPaused);

        eventQueue.mockDesktop.ResetExpectedWindows();
        eventQueue.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bSawDialog](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bSawDialog = true;

            Assert::AreEqual(std::wstring(L"The emulator has been paused."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The following triggers have been reset:\n* You did it again!\n* And again!\n* And again!"), vmMessageBox.GetMessage());

            return ra::ui::DialogResult::OK;
        });

        eventQueue.QueuePauseOnReset(L"You did it again!");
        eventQueue.QueuePauseOnReset(L"And again!");
        eventQueue.QueuePauseOnReset(L"And again!"); // duplicate titles allowed
        Assert::AreEqual({ 0U }, eventQueue.NumMemoryChanges());
        Assert::AreEqual({ 3U }, eventQueue.NumResetTriggers());
        Assert::AreEqual({ 0U }, eventQueue.NumTriggeredTriggers());

        eventQueue.DoFrame();
        Assert::IsTrue(bSawDialog);
        Assert::IsTrue(bPaused);
        Assert::AreEqual({ 0U }, eventQueue.NumMemoryChanges());
        Assert::AreEqual({ 0U }, eventQueue.NumResetTriggers());
        Assert::AreEqual({ 0U }, eventQueue.NumTriggeredTriggers());
    }

    TEST_METHOD(TestPauseOnTrigger)
    {
        FrameEventQueueHarness eventQueue;
        bool bPaused = false;
        eventQueue.mockEmulatorContext.SetPauseFunction([&bPaused]() { bPaused = true; });

        bool bSawDialog = false;
        eventQueue.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bSawDialog](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bSawDialog = true;

            Assert::AreEqual(std::wstring(L"The emulator has been paused."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The following triggers have triggered:\n* You did it!"), vmMessageBox.GetMessage());

            return ra::ui::DialogResult::OK;
        });

        eventQueue.QueuePauseOnTrigger(L"You did it!");
        Assert::AreEqual({ 0U }, eventQueue.NumMemoryChanges());
        Assert::AreEqual({ 0U }, eventQueue.NumResetTriggers());
        Assert::AreEqual({ 1U }, eventQueue.NumTriggeredTriggers());
        Assert::IsFalse(bSawDialog);
        Assert::IsFalse(bPaused);

        eventQueue.DoFrame();
        Assert::IsTrue(bSawDialog);
        Assert::IsTrue(bPaused);
        Assert::AreEqual({ 0U }, eventQueue.NumMemoryChanges());
        Assert::AreEqual({ 0U }, eventQueue.NumResetTriggers());
        Assert::AreEqual({ 0U }, eventQueue.NumTriggeredTriggers());

        bSawDialog = false;
        bPaused = false;
        eventQueue.DoFrame();
        Assert::IsFalse(bSawDialog);
        Assert::IsFalse(bPaused);

        eventQueue.mockDesktop.ResetExpectedWindows();
        eventQueue.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bSawDialog](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bSawDialog = true;

            Assert::AreEqual(std::wstring(L"The emulator has been paused."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The following triggers have triggered:\n* You did it again!\n* And again!\n* And again!"), vmMessageBox.GetMessage());

            return ra::ui::DialogResult::OK;
        });

        eventQueue.QueuePauseOnTrigger(L"You did it again!");
        eventQueue.QueuePauseOnTrigger(L"And again!");
        eventQueue.QueuePauseOnTrigger(L"And again!"); // duplicate titles allowed
        Assert::AreEqual({ 0U }, eventQueue.NumMemoryChanges());
        Assert::AreEqual({ 0U }, eventQueue.NumResetTriggers());
        Assert::AreEqual({ 3U }, eventQueue.NumTriggeredTriggers());

        eventQueue.DoFrame();
        Assert::IsTrue(bSawDialog);
        Assert::IsTrue(bPaused);
        Assert::AreEqual({ 0U }, eventQueue.NumMemoryChanges());
        Assert::AreEqual({ 0U }, eventQueue.NumResetTriggers());
        Assert::AreEqual({ 0U }, eventQueue.NumTriggeredTriggers());
    }

    TEST_METHOD(TestPauseOnChange)
    {
        FrameEventQueueHarness eventQueue;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager; // for MemSize->string mapping
        bool bPaused = false;
        eventQueue.mockEmulatorContext.SetPauseFunction([&bPaused]() { bPaused = true; });

        bool bSawDialog = false;
        eventQueue.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bSawDialog](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bSawDialog = true;

            Assert::AreEqual(std::wstring(L"The emulator has been paused."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The following bookmarks have changed:\n* 8-bit 0x1234"), vmMessageBox.GetMessage());

            return ra::ui::DialogResult::OK;
        });

        eventQueue.QueuePauseOnChange(MemSize::EightBit, { 0x1234U });
        Assert::AreEqual({ 1U }, eventQueue.NumMemoryChanges());
        Assert::AreEqual({ 0U }, eventQueue.NumResetTriggers());
        Assert::AreEqual({ 0U }, eventQueue.NumTriggeredTriggers());
        Assert::IsFalse(bSawDialog);
        Assert::IsFalse(bPaused);

        eventQueue.DoFrame();
        Assert::IsTrue(bSawDialog);
        Assert::IsTrue(bPaused);
        Assert::AreEqual({ 0U }, eventQueue.NumMemoryChanges());
        Assert::AreEqual({ 0U }, eventQueue.NumResetTriggers());
        Assert::AreEqual({ 0U }, eventQueue.NumTriggeredTriggers());

        bSawDialog = false;
        bPaused = false;
        eventQueue.DoFrame();
        Assert::IsFalse(bSawDialog);
        Assert::IsFalse(bPaused);

        eventQueue.mockDesktop.ResetExpectedWindows();
        eventQueue.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bSawDialog](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bSawDialog = true;

            Assert::AreEqual(std::wstring(L"The emulator has been paused."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The following bookmarks have changed:\n* 16-bit 0x2345\n* 32-bit 0x3456\n* 32-bit 0x2345"), vmMessageBox.GetMessage());

            return ra::ui::DialogResult::OK;
        });

        eventQueue.QueuePauseOnChange(MemSize::SixteenBit, { 0x2345U });
        eventQueue.QueuePauseOnChange(MemSize::ThirtyTwoBit, { 0x3456U });
        eventQueue.QueuePauseOnChange(MemSize::SixteenBit, { 0x2345U }); // duplicate ignored
        eventQueue.QueuePauseOnChange(MemSize::ThirtyTwoBit, { 0x2345U }); // same address, different size allowed
        eventQueue.QueuePauseOnChange(MemSize::ThirtyTwoBit, { 0x2345U }); // duplicate ignored

        Assert::AreEqual({ 3U }, eventQueue.NumMemoryChanges());
        Assert::AreEqual({ 0U }, eventQueue.NumResetTriggers());
        Assert::AreEqual({ 0U }, eventQueue.NumTriggeredTriggers());

        eventQueue.DoFrame();
        Assert::IsTrue(bSawDialog);
        Assert::IsTrue(bPaused);
        Assert::AreEqual({ 0U }, eventQueue.NumMemoryChanges());
        Assert::AreEqual({ 0U }, eventQueue.NumResetTriggers());
        Assert::AreEqual({ 0U }, eventQueue.NumTriggeredTriggers());
    }

    TEST_METHOD(TestComplexDialog)
    {
        FrameEventQueueHarness eventQueue;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager; // for MemSize->string mapping
        int nPaused = 0;
        eventQueue.mockEmulatorContext.SetPauseFunction([&nPaused]() { ++nPaused; });

        bool bSawDialog = false;
        eventQueue.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bSawDialog](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bSawDialog = true;

            Assert::AreEqual(std::wstring(L"The emulator has been paused."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(
                L"The following triggers have triggered:\n* You did it!"
                L"\nThe following triggers have been reset:\n* You did it again!"
                L"\nThe following bookmarks have changed:\n* 8-bit 0x1234"), vmMessageBox.GetMessage());

            return ra::ui::DialogResult::OK;
        });

        eventQueue.QueuePauseOnChange(MemSize::EightBit, { 0x1234U });
        eventQueue.QueuePauseOnTrigger(L"You did it!");
        eventQueue.QueuePauseOnReset(L"You did it again!");
        Assert::AreEqual({ 1U }, eventQueue.NumMemoryChanges());
        Assert::AreEqual({ 1U }, eventQueue.NumResetTriggers());
        Assert::AreEqual({ 1U }, eventQueue.NumTriggeredTriggers());
        Assert::IsFalse(bSawDialog);
        Assert::AreEqual(0, nPaused);

        eventQueue.DoFrame();
        Assert::IsTrue(bSawDialog);
        Assert::AreEqual(1, nPaused); // should only pause once
        Assert::AreEqual({ 0U }, eventQueue.NumMemoryChanges());
        Assert::AreEqual({ 0U }, eventQueue.NumResetTriggers());
        Assert::AreEqual({ 0U }, eventQueue.NumTriggeredTriggers());
    }
};

} // namespace tests
} // namespace services
} // namespace ra
