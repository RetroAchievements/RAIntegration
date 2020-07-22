#include "CppUnitTest.h"

#include "ui\viewmodels\MemoryInspectorViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockAudioSystem.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(MemoryInspectorViewModel_Tests)
{
private:
    class MemoryInspectorViewModelHarness : public MemoryInspectorViewModel
    {
    public:
        ra::data::mocks::MockConsoleContext mockConsoleContext;
        ra::data::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockAudioSystem mockAudioSystem;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        void SetCurrentAddressText(const std::wstring& sValue) { SetValue(CurrentAddressTextProperty, sValue); }

        std::array<unsigned char, 32> memory{};

        GSL_SUPPRESS_F6 MemoryInspectorViewModelHarness()
        {
            InitializeNotifyTargets();

            for (size_t i = 0; i < memory.size(); ++i)
                memory.at(i) = gsl::narrow_cast<unsigned char>(i);

            mockEmulatorContext.MockMemory(memory);
        }

        ~MemoryInspectorViewModelHarness()
        {
            // ensure we stop monitoring the MemoryBookmarksViewModel before the MockWindowManager is destroyed
            Viewer().DetachNotifyTargets();
        }

        bool CanModifyCodeNotes() const { return GetValue(CanModifyNotesProperty); }
    };


public:
    TEST_METHOD(TestInitialValues)
    {
        MemoryInspectorViewModelHarness inspector;

        Assert::AreEqual({ 0U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0000"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(), inspector.GetCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"0 0 0 0 0 0 0 0"), inspector.GetCurrentAddressBits());
        Assert::IsFalse(inspector.CanModifyCodeNotes());

        Assert::AreEqual({ 0U }, inspector.Viewer().GetAddress());
    }

    TEST_METHOD(TestSetCurrentAddress)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.SetCurrentAddress({ 3U });

        Assert::AreEqual({ 3U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0003"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(), inspector.GetCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"0 0 0 0 0 0 1 1"), inspector.GetCurrentAddressBits());

        Assert::AreEqual({ 3U }, inspector.Viewer().GetAddress());
    }

    TEST_METHOD(TestSetCurrentAddressText)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.SetCurrentAddressText(L"3");

        Assert::AreEqual({ 3U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"3"), inspector.GetCurrentAddressText()); /* don't update text when user types in partial address */
        Assert::AreEqual(std::wstring(), inspector.GetCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"0 0 0 0 0 0 1 1"), inspector.GetCurrentAddressBits());

        Assert::AreEqual({ 3U }, inspector.Viewer().GetAddress());
    }

    TEST_METHOD(TestSetViewerAddress)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.Viewer().SetAddress({ 3U });

        Assert::AreEqual({ 3U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0003"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(), inspector.GetCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"0 0 0 0 0 0 1 1"), inspector.GetCurrentAddressBits());

        Assert::AreEqual({ 3U }, inspector.Viewer().GetAddress());
    }

    TEST_METHOD(TestToggleBit)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.Viewer().SetAddress({ 3U });

        Assert::AreEqual({ 0x03 }, inspector.memory.at(3));
        Assert::AreEqual(std::wstring(L"0 0 0 0 0 0 1 1"), inspector.GetCurrentAddressBits());

        inspector.ToggleBit(6);
        Assert::AreEqual({ 0x43 }, inspector.memory.at(3));
        Assert::AreEqual(std::wstring(L"0 1 0 0 0 0 1 1"), inspector.GetCurrentAddressBits());

        inspector.ToggleBit(1);
        Assert::AreEqual({ 0x41 }, inspector.memory.at(3));
        Assert::AreEqual(std::wstring(L"0 1 0 0 0 0 0 1"), inspector.GetCurrentAddressBits());
    }

    TEST_METHOD(TestToggleBitWhileFrozen)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.Viewer().SetAddress({ 3U });

        inspector.mockWindowManager.MemoryBookmarks.AddBookmark(3, MemSize::EightBit);
        auto* pNote = inspector.mockWindowManager.MemoryBookmarks.Bookmarks().GetItemAt(0);
        Expects(pNote != nullptr);
        Assert::AreEqual(3U, pNote->GetCurrentValue());
        Assert::AreEqual((int)MemoryBookmarksViewModel::BookmarkBehavior::None, (int)pNote->GetBehavior());

        Assert::AreEqual({ 0x03 }, inspector.memory.at(3));
        Assert::AreEqual(std::wstring(L"0 0 0 0 0 0 1 1"), inspector.GetCurrentAddressBits());

        inspector.ToggleBit(6);
        Assert::AreEqual({ 0x43 }, inspector.memory.at(3));
        Assert::AreEqual(std::wstring(L"0 1 0 0 0 0 1 1"), inspector.GetCurrentAddressBits());
        Assert::AreEqual(0x43U, pNote->GetCurrentValue());

        pNote->SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::Frozen);
        Assert::AreEqual((int)MemoryBookmarksViewModel::BookmarkBehavior::Frozen, (int)pNote->GetBehavior());

        inspector.ToggleBit(1);
        Assert::AreEqual({ 0x41 }, inspector.memory.at(3));
        Assert::AreEqual(std::wstring(L"0 1 0 0 0 0 0 1"), inspector.GetCurrentAddressBits());
        Assert::AreEqual(0x41U, pNote->GetCurrentValue());
    }

    TEST_METHOD(TestOpenNotesList)
    {
        MemoryInspectorViewModelHarness inspector;

        bool bWindowSeen = false;
        inspector.mockDesktop.ExpectWindow<ra::ui::viewmodels::CodeNotesViewModel>([&bWindowSeen](ra::ui::viewmodels::CodeNotesViewModel&)
        {
            bWindowSeen = true;
            return ra::ui::DialogResult::OK;
        });

        inspector.OpenNotesList();
        Assert::IsTrue(bWindowSeen);
    }

    TEST_METHOD(TestNextNote)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetCodeNote({ 8 }, L"Eight");
        inspector.mockGameContext.SetCodeNote({ 12 }, L"Twelve");
        inspector.mockGameContext.SetCodeNote({ 16 }, L"Sixteen");
        inspector.mockGameContext.SetCodeNote({ 20 }, L"Twenty");

        inspector.SetCurrentAddress(14);
        Assert::AreEqual(std::wstring(), inspector.GetCurrentAddressNote());

        Assert::IsTrue(inspector.NextNote());
        Assert::AreEqual({ 16 }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"Sixteen"), inspector.GetCurrentAddressNote());

        Assert::IsTrue(inspector.NextNote());
        Assert::AreEqual({ 20 }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"Twenty"), inspector.GetCurrentAddressNote());

        Assert::IsFalse(inspector.NextNote());
        Assert::AreEqual({ 20 }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"Twenty"), inspector.GetCurrentAddressNote());
    }

    TEST_METHOD(TestPreviousNote)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetCodeNote({ 8 }, L"Eight");
        inspector.mockGameContext.SetCodeNote({ 12 }, L"Twelve");
        inspector.mockGameContext.SetCodeNote({ 16 }, L"Sixteen");
        inspector.mockGameContext.SetCodeNote({ 20 }, L"Twenty");

        inspector.SetCurrentAddress(14);
        Assert::AreEqual(std::wstring(), inspector.GetCurrentAddressNote());

        Assert::IsTrue(inspector.PreviousNote());
        Assert::AreEqual({ 12 }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"Twelve"), inspector.GetCurrentAddressNote());

        Assert::IsTrue(inspector.PreviousNote());
        Assert::AreEqual({ 8 }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"Eight"), inspector.GetCurrentAddressNote());

        Assert::IsFalse(inspector.PreviousNote());
        Assert::AreEqual({ 8 }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"Eight"), inspector.GetCurrentAddressNote());
    }

    TEST_METHOD(TestSaveCurrentAddressNoteNew)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.SetCurrentAddressNote(L"Test");

        inspector.SaveCurrentAddressNote();
        Assert::IsFalse(inspector.mockDesktop.WasDialogShown());

        const auto* pNote = inspector.mockGameContext.FindCodeNote({ 0 });
        Assert::IsNotNull(pNote);
        Ensures(pNote != nullptr);
        Assert::AreEqual(std::wstring(L"Test"), *pNote);
        Assert::IsTrue(inspector.mockAudioSystem.WasAudioFilePlayed(ra::services::mocks::MockAudioSystem::BEEP));
    }

    TEST_METHOD(TestSaveCurrentAddressNoteNewBlank)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.SetCurrentAddressNote(L"");

        inspector.SaveCurrentAddressNote();
        Assert::IsFalse(inspector.mockDesktop.WasDialogShown());

        const auto* pNote = inspector.mockGameContext.FindCodeNote({ 0 });
        Assert::IsNull(pNote);
        Assert::IsFalse(inspector.mockAudioSystem.WasAudioFilePlayed(ra::services::mocks::MockAudioSystem::BEEP));
    }

    TEST_METHOD(TestSaveCurrentAddressNoteUnchanged)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetCodeNote({ 0 }, L"Test");
        inspector.SetCurrentAddressNote(L"Test");

        inspector.SaveCurrentAddressNote();
        Assert::IsFalse(inspector.mockDesktop.WasDialogShown());

        const auto* pNote = inspector.mockGameContext.FindCodeNote({ 0 });
        Assert::IsNotNull(pNote);
        Ensures(pNote != nullptr);
        Assert::AreEqual(std::wstring(L"Test"), *pNote);
        Assert::IsFalse(inspector.mockAudioSystem.WasAudioFilePlayed(ra::services::mocks::MockAudioSystem::BEEP));
    }

    TEST_METHOD(TestSaveCurrentAddressNoteOverwrite)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetCodeNote({ 0 }, L"Test");
        inspector.SetCurrentAddressNote(L"Test2");

        bool bWindowSeen = false;
        inspector.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Overwrite note for address 0x0000?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Are you sure you want to replace Author's note:\n\nTest\n\nWith your note:\n\nTest2"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());

            bWindowSeen = true;
            return ra::ui::DialogResult::Yes;
        });

        inspector.SaveCurrentAddressNote();
        Assert::IsTrue(bWindowSeen);

        const auto* pNote = inspector.mockGameContext.FindCodeNote({ 0 });
        Assert::IsNotNull(pNote);
        Ensures(pNote != nullptr);
        Assert::AreEqual(std::wstring(L"Test2"), *pNote);
        Assert::AreEqual(std::wstring(L"Test2"), inspector.GetCurrentAddressNote()); // text should not be reset
        Assert::IsTrue(inspector.mockAudioSystem.WasAudioFilePlayed(ra::services::mocks::MockAudioSystem::BEEP));
    }

    TEST_METHOD(TestSaveCurrentAddressNoteOverwriteCancel)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetCodeNote({ 0 }, L"Test");
        inspector.SetCurrentAddressNote(L"Test2");

        bool bWindowSeen = false;
        inspector.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Overwrite note for address 0x0000?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Are you sure you want to replace Author's note:\n\nTest\n\nWith your note:\n\nTest2"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());

            bWindowSeen = true;
            return ra::ui::DialogResult::No;
        });

        inspector.SaveCurrentAddressNote();
        Assert::IsTrue(bWindowSeen);

        const auto* pNote = inspector.mockGameContext.FindCodeNote({ 0 });
        Assert::IsNotNull(pNote);
        Ensures(pNote != nullptr);
        Assert::AreEqual(std::wstring(L"Test"), *pNote);
        Assert::AreEqual(std::wstring(L"Test"), inspector.GetCurrentAddressNote()); // text should be reset
        Assert::IsFalse(inspector.mockAudioSystem.WasAudioFilePlayed(ra::services::mocks::MockAudioSystem::BEEP));
    }

    TEST_METHOD(TestSaveCurrentAddressNoteOverwriteLong)
    {
        std::wstring sLongNote;
        for (int i = 0; i < 48; ++i)
            sLongNote += L"Test" + std::to_wstring(i) + L" ";

        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetCodeNote({ 0 }, sLongNote);
        inspector.SetCurrentAddressNote(L"Test");

        bool bWindowSeen = false;
        inspector.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Overwrite note for address 0x0000?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Are you sure you want to replace Author's note:\n\n" \
                "Test0 Test1 Test2 Test3 Test4 Test5 Test6 Test7 Test8 Test9 Test10 Test11 Test12 Test13 " \
                "Test14 Test15 Test16 Test17 Test18 Test19 Test20 Test21 Test22 Test23 Test24 Test25 " \
                "Test26 Test27 Test28 Test29 Test30 Test31 Test32 Test33 Test34 Test35 Test36 Test..." \
                "\n\nWith your note:\n\nTest"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());

            bWindowSeen = true;
            return ra::ui::DialogResult::Yes;
        });

        inspector.SaveCurrentAddressNote();
        Assert::IsTrue(bWindowSeen);
    }

    TEST_METHOD(TestDeleteCurrentAddressNoteBlank)
    {
        MemoryInspectorViewModelHarness inspector;

        inspector.DeleteCurrentAddressNote();
        Assert::IsFalse(inspector.mockDesktop.WasDialogShown());

        const auto* pNote = inspector.mockGameContext.FindCodeNote({ 0 });
        Assert::IsNull(pNote);
        Assert::IsFalse(inspector.mockAudioSystem.WasAudioFilePlayed(ra::services::mocks::MockAudioSystem::BEEP));
    }

    TEST_METHOD(TestDeleteCurrentAddressNoteConfirm)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetCodeNote({ 0 }, L"Test");
        inspector.SetCurrentAddressNote(L"Test");

        bool bWindowSeen = false;
        inspector.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Delete note for address 0x0000?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Are you sure you want to delete Author's note:\n\nTest"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());

            bWindowSeen = true;
            return ra::ui::DialogResult::Yes;
        });

        inspector.DeleteCurrentAddressNote();
        Assert::IsTrue(bWindowSeen);

        const auto* pNote = inspector.mockGameContext.FindCodeNote({ 0 });
        Assert::IsNull(pNote);
        Assert::AreEqual(std::wstring(L""), inspector.GetCurrentAddressNote()); // text should be updated
        Assert::IsTrue(inspector.mockAudioSystem.WasAudioFilePlayed(ra::services::mocks::MockAudioSystem::BEEP));
    }

    TEST_METHOD(TestDeleteCurrentAddressNoteDecline)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetCodeNote({ 0 }, L"Test");
        inspector.SetCurrentAddressNote(L"Test");

        bool bWindowSeen = false;
        inspector.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Delete note for address 0x0000?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Are you sure you want to delete Author's note:\n\nTest"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());

            bWindowSeen = true;
            return ra::ui::DialogResult::No;
        });

        inspector.DeleteCurrentAddressNote();
        Assert::IsTrue(bWindowSeen);

        const auto* pNote = inspector.mockGameContext.FindCodeNote({ 0 });
        Assert::IsNotNull(pNote);
        Ensures(pNote != nullptr);
        Assert::AreEqual(std::wstring(L"Test"), *pNote);
        Assert::AreEqual(std::wstring(L"Test"), inspector.GetCurrentAddressNote()); // text should not be updated
        Assert::IsFalse(inspector.mockAudioSystem.WasAudioFilePlayed(ra::services::mocks::MockAudioSystem::BEEP));
    }

    TEST_METHOD(TestActiveGameChanged)
    {
        MemoryInspectorViewModelHarness inspector;
        Assert::AreEqual(std::wstring(L"Memory Inspector [no game loaded]"), inspector.GetWindowTitle());
        Assert::IsFalse(inspector.CanModifyCodeNotes());

        inspector.mockGameContext.SetGameId({ 3 });
        inspector.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual(std::wstring(L"Memory Inspector"), inspector.GetWindowTitle());
        Assert::IsTrue(inspector.CanModifyCodeNotes());

        inspector.mockGameContext.SetGameId({ 0 });
        inspector.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual(std::wstring(L"Memory Inspector [no game loaded]"), inspector.GetWindowTitle());
        Assert::IsFalse(inspector.CanModifyCodeNotes());

        inspector.mockGameContext.SetGameId({ 5 });
        inspector.mockGameContext.SetMode(ra::data::GameContext::Mode::CompatibilityTest);
        inspector.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual(std::wstring(L"Memory Inspector [compatibility mode]"), inspector.GetWindowTitle());
        Assert::IsTrue(inspector.CanModifyCodeNotes());
    }

    TEST_METHOD(TestEndGameLoad)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetCodeNote({ 3U }, L"Test");
        inspector.mockGameContext.SetCodeNote({ 5U }, L"Test2");

        inspector.mockGameContext.NotifyGameLoad();

        Assert::AreEqual({ 3U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0003"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(L"Test"), inspector.GetCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"0 0 0 0 0 0 1 1"), inspector.GetCurrentAddressBits());

        Assert::AreEqual({ 3U }, inspector.Viewer().GetAddress());
    }

    TEST_METHOD(TestBookmarkCurrentAddress)
    {
        MemoryInspectorViewModelHarness inspector;

        inspector.SetCurrentAddress(2U);
        inspector.BookmarkCurrentAddress();

        const auto& pBookmarks = inspector.mockWindowManager.MemoryBookmarks.Bookmarks();
        Assert::AreEqual({ 1U }, pBookmarks.Count());
        Assert::AreEqual({ 2U }, pBookmarks.GetItemAt(0)->GetAddress());
        Assert::AreEqual(MemSize::EightBit, pBookmarks.GetItemAt(0)->GetSize());

        inspector.Viewer().SetSize(MemSize::ThirtyTwoBit);
        inspector.SetCurrentAddress(5U);
        inspector.BookmarkCurrentAddress();

        Assert::AreEqual({ 2U }, pBookmarks.Count());
        Assert::AreEqual({ 2U }, pBookmarks.GetItemAt(0)->GetAddress());
        Assert::AreEqual(MemSize::EightBit, pBookmarks.GetItemAt(0)->GetSize());
        Assert::AreEqual({ 5U }, pBookmarks.GetItemAt(1)->GetAddress());
        Assert::AreEqual(MemSize::ThirtyTwoBit, pBookmarks.GetItemAt(1)->GetSize());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
