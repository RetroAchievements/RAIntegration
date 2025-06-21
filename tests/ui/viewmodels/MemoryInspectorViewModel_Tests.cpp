#include "CppUnitTest.h"

#include "api\DeleteCodeNote.hh"
#include "api\UpdateCodeNote.hh"

#include "ui\viewmodels\MemoryInspectorViewModel.hh"

#include "tests\ui\UIAsserts.hh"
#include "tests\RA_UnitTestHelpers.h"

#include "tests\mocks\MockAchievementRuntime.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockLocalStorage.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockThreadPool.hh"
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
        ra::api::mocks::MockServer mockServer;
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::services::mocks::MockLocalStorage mockLocalStorage;
        ra::services::mocks::MockThreadPool mockThreadPool;
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

            mockUserContext.SetUsername("Author");

            mockGameContext.InitializeCodeNotes();

            Viewer().DoFrame(); // load memory into viewer so CurrentAddress value can be read
        }

        ~MemoryInspectorViewModelHarness()
        {
            // ensure we stop monitoring the MemoryBookmarksViewModel before the MockWindowManager is destroyed
            Viewer().DetachNotifyTargets();
        }

        MemoryInspectorViewModelHarness(const MemoryInspectorViewModelHarness&) noexcept = delete;
        MemoryInspectorViewModelHarness& operator=(const MemoryInspectorViewModelHarness&) noexcept = delete;
        MemoryInspectorViewModelHarness(MemoryInspectorViewModelHarness&&) noexcept = delete;
        MemoryInspectorViewModelHarness& operator=(MemoryInspectorViewModelHarness&&) noexcept = delete;

        bool CurrentBitsVisible() const { return GetValue(CurrentBitsVisibleProperty); }

        const std::wstring* FindCodeNote(ra::ByteAddress nAddress) const
        {
            const auto* pCodeNotes = mockGameContext.Assets().FindCodeNotes();
            return (pCodeNotes != nullptr) ? pCodeNotes->FindCodeNote(nAddress) : nullptr;
        }

        void PreparePublish(ra::ByteAddress nAddress, std::wstring sNote)
        {
            mockServer.HandleRequest<ra::api::UpdateCodeNote>([this]
                (const ra::api::UpdateCodeNote::Request& pRequest, ra::api::UpdateCodeNote::Response& pResponse)
            {
                m_nPublishedAddress = pRequest.Address;

                pResponse.Result = ra::api::ApiResult::Success;
                return true;
            });

            mockServer.HandleRequest<ra::api::DeleteCodeNote>([this]
                (const ra::api::DeleteCodeNote::Request& pRequest, ra::api::DeleteCodeNote::Response& pResponse)
            {
                m_nPublishedAddress = pRequest.Address;

                pResponse.Result = ra::api::ApiResult::Success;
                return true;
            });

            mockThreadPool.SetSynchronous(true);

            SetValue(CanModifyNotesProperty, true);

            mockGameContext.Assets().FindCodeNotes()->SetCodeNote(nAddress, sNote);
            SetCurrentAddress(nAddress);

            Assert::AreEqual(sNote, GetCurrentAddressNote());
        }

        void PreparePublishFailure(ra::ByteAddress nAddress, std::wstring sNote)
        {
            mockServer.HandleRequest<ra::api::UpdateCodeNote>(
                [this](const ra::api::UpdateCodeNote::Request& pRequest, ra::api::UpdateCodeNote::Response& pResponse) {
                    m_nPublishedAddress = pRequest.Address;

                    pResponse.Result = ra::api::ApiResult::Failed;
                    return true;
                });

            mockServer.HandleRequest<ra::api::DeleteCodeNote>(
                [this](const ra::api::DeleteCodeNote::Request& pRequest, ra::api::DeleteCodeNote::Response& pResponse) {
                    m_nPublishedAddress = pRequest.Address;

                    pResponse.Result = ra::api::ApiResult::Failed;
                    return true;
                });

            mockThreadPool.SetSynchronous(true);

            SetValue(CanModifyNotesProperty, true);

            mockGameContext.Assets().FindCodeNotes()->SetCodeNote(nAddress, sNote);
            SetCurrentAddress(nAddress);

            Assert::AreEqual(sNote, GetCurrentAddressNote());
        }

        ra::ByteAddress GetPublishedAddress() const noexcept { return m_nPublishedAddress; }

    private:
        ra::ByteAddress m_nPublishedAddress = 0U;
    };


public:
    TEST_METHOD(TestInitialValues)
    {
        MemoryInspectorViewModelHarness inspector;

        Assert::AreEqual({ 0U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0000"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(), inspector.GetCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"0 0 0 0 0 0 0 0"), inspector.GetCurrentAddressBits());
        Assert::IsFalse(inspector.CanModifyNotes());

        Assert::AreEqual({ 0U }, inspector.Viewer().GetAddress());
    }

    TEST_METHOD(TestSetCurrentAddress)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        inspector.SetCurrentAddress({ 3U });

        Assert::AreEqual({ 3U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0003"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(), inspector.GetCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"0 0 0 0 0 0 1 1"), inspector.GetCurrentAddressBits());
        Assert::IsFalse(inspector.IsCurrentAddressNoteReadOnly());
        Assert::IsTrue(inspector.CanEditCurrentAddressNote());
        Assert::IsFalse(inspector.CanPublishCurrentAddressNote());
        Assert::IsFalse(inspector.CanRevertCurrentAddressNote());

        Assert::AreEqual({ 3U }, inspector.Viewer().GetAddress());
    }

    TEST_METHOD(TestSetCurrentAddressText)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        inspector.SetCurrentAddressText(L"3");

        Assert::AreEqual({ 3U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"3"), inspector.GetCurrentAddressText()); /* don't update text when user types in partial address */
        Assert::AreEqual(std::wstring(), inspector.GetCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"0 0 0 0 0 0 1 1"), inspector.GetCurrentAddressBits());
        Assert::IsFalse(inspector.IsCurrentAddressNoteReadOnly());
        Assert::IsTrue(inspector.CanEditCurrentAddressNote());
        Assert::IsFalse(inspector.CanPublishCurrentAddressNote());
        Assert::IsFalse(inspector.CanRevertCurrentAddressNote());

        Assert::AreEqual({ 3U }, inspector.Viewer().GetAddress());
    }

    TEST_METHOD(TestSetCurrentAddressWithNote)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        inspector.SetCurrentAddress({ 3U });
        inspector.mockGameContext.Assets().FindCodeNotes()->SetServerCodeNote({3U}, L"Note on 3");
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({3U}, L"Note on 3");

        Assert::AreEqual({ 3U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0003"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(L"Note on 3"), inspector.GetCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"0 0 0 0 0 0 1 1"), inspector.GetCurrentAddressBits());
        Assert::IsFalse(inspector.IsCurrentAddressNoteReadOnly());
        Assert::IsTrue(inspector.CanEditCurrentAddressNote());
        Assert::IsFalse(inspector.CanPublishCurrentAddressNote());
        Assert::IsFalse(inspector.CanRevertCurrentAddressNote());

        // update note directly as SetCurrentNoteAddress doesn't cause UpdateNoteButtons to be called
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({3U}, L"Modified Note on 3");

        Assert::IsFalse(inspector.IsCurrentAddressNoteReadOnly());
        Assert::IsTrue(inspector.CanEditCurrentAddressNote());
        Assert::IsTrue(inspector.CanPublishCurrentAddressNote());
        Assert::IsTrue(inspector.CanRevertCurrentAddressNote());
    }

    TEST_METHOD(TestSetCurrentAddressWithNoteIndirect)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support
        inspector.mockGameContext.Assets().FindCodeNotes()->SetServerCodeNote({3U}, L"[8-bit Pointer]\n+1 Test");

        inspector.SetCurrentAddress({ 3U });

        Assert::AreEqual({ 3U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0003"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(L"[8-bit Pointer]\n+1 Test"), inspector.GetCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"0 0 0 0 0 0 1 1"), inspector.GetCurrentAddressBits());
        Assert::IsFalse(inspector.IsCurrentAddressNoteReadOnly());
        Assert::IsTrue(inspector.CanEditCurrentAddressNote());
        Assert::IsFalse(inspector.CanPublishCurrentAddressNote());
        Assert::IsFalse(inspector.CanRevertCurrentAddressNote());

        inspector.SetCurrentAddress({ 4U });

        Assert::AreEqual({ 4U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0004"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(L"[Indirect from 0x0003]\r\nTest"), inspector.GetCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"0 0 0 0 0 1 0 0"), inspector.GetCurrentAddressBits());
        Assert::IsTrue(inspector.IsCurrentAddressNoteReadOnly());
        Assert::IsTrue(inspector.CanEditCurrentAddressNote());
        Assert::IsFalse(inspector.CanPublishCurrentAddressNote());
        Assert::IsFalse(inspector.CanRevertCurrentAddressNote());

        inspector.mockEmulatorContext.WriteMemoryByte({3U}, 2);
        inspector.mockGameContext.Assets().FindCodeNotes()->DoFrame();
        
        Assert::AreEqual({ 4U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0004"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(), inspector.GetCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"0 0 0 0 0 1 0 0"), inspector.GetCurrentAddressBits());
        Assert::IsFalse(inspector.IsCurrentAddressNoteReadOnly());
        Assert::IsTrue(inspector.CanEditCurrentAddressNote());
        Assert::IsFalse(inspector.CanPublishCurrentAddressNote());
        Assert::IsFalse(inspector.CanRevertCurrentAddressNote());

        // $3 = 2 + 1 = 3, so 3 is currently indirectly pointing it itself
        // expect to find the original note, not the indirect one
        inspector.SetCurrentAddress({ 3U });

        Assert::AreEqual({ 3U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0003"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(L"[8-bit Pointer]\n+1 Test"), inspector.GetCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"0 0 0 0 0 0 1 0"), inspector.GetCurrentAddressBits());
        Assert::IsFalse(inspector.IsCurrentAddressNoteReadOnly());
        Assert::IsTrue(inspector.CanEditCurrentAddressNote());
        Assert::IsFalse(inspector.CanPublishCurrentAddressNote());
        Assert::IsFalse(inspector.CanRevertCurrentAddressNote());
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

        inspector.mockWindowManager.MemoryBookmarks.InitializeNotifyTargets();
        inspector.mockWindowManager.MemoryBookmarks.AddBookmark(3, MemSize::EightBit);
        auto* pNote = inspector.mockWindowManager.MemoryBookmarks.Bookmarks().GetItemAt(0);
        Expects(pNote != nullptr);
        Assert::AreEqual(std::wstring(L"03"), pNote->GetCurrentValue());
        Assert::AreEqual((int)MemoryBookmarksViewModel::BookmarkBehavior::None, (int)pNote->GetBehavior());

        Assert::AreEqual({ 0x03 }, inspector.memory.at(3));
        Assert::AreEqual(std::wstring(L"0 0 0 0 0 0 1 1"), inspector.GetCurrentAddressBits());

        inspector.ToggleBit(6);
        Assert::AreEqual({ 0x43 }, inspector.memory.at(3));
        Assert::AreEqual(std::wstring(L"0 1 0 0 0 0 1 1"), inspector.GetCurrentAddressBits());
        Assert::AreEqual(std::wstring(L"43"), pNote->GetCurrentValue());

        pNote->SetBehavior(MemoryBookmarksViewModel::BookmarkBehavior::Frozen);
        Assert::AreEqual((int)MemoryBookmarksViewModel::BookmarkBehavior::Frozen, (int)pNote->GetBehavior());

        inspector.ToggleBit(1);
        Assert::AreEqual({ 0x41 }, inspector.memory.at(3));
        Assert::AreEqual(std::wstring(L"0 1 0 0 0 0 0 1"), inspector.GetCurrentAddressBits());
        Assert::AreEqual(std::wstring(L"41"), pNote->GetCurrentValue());
    }

    TEST_METHOD(TestCurrentBitsVisible)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.Viewer().SetAddress({ 3U });
        Assert::AreEqual(MemSize::EightBit, inspector.Viewer().GetSize());
        Assert::IsTrue(inspector.CurrentBitsVisible());

        inspector.Viewer().SetSize(MemSize::SixteenBit);
        Assert::IsFalse(inspector.CurrentBitsVisible());

        inspector.Viewer().SetSize(MemSize::ThirtyTwoBit);
        Assert::IsFalse(inspector.CurrentBitsVisible());

        inspector.Viewer().SetSize(MemSize::ThirtyTwoBitBigEndian);
        Assert::IsFalse(inspector.CurrentBitsVisible());

        inspector.Viewer().SetSize(MemSize::EightBit);
        Assert::IsTrue(inspector.CurrentBitsVisible());
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
        inspector.mockGameContext.SetGameId({ 3 });
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
        inspector.mockGameContext.SetGameId({ 3 });
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

    TEST_METHOD(TestPublishCurrentAddressNoteNew)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.PreparePublish(0x12, L"Test");

        bool bWindowSeen = false;
        inspector.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            bWindowSeen = true;
            return ra::ui::DialogResult::No;
        });

        Assert::IsTrue(inspector.CanPublishCurrentAddressNote());
        inspector.PublishCurrentAddressNote();

        Assert::IsFalse(bWindowSeen);
        Assert::AreEqual(0x12U, inspector.GetPublishedAddress());
        Assert::IsFalse(inspector.CanPublishCurrentAddressNote());
        Assert::IsFalse(inspector.CanRevertCurrentAddressNote());
    }

    TEST_METHOD(TestPublishCurrentAddressNoteDelete)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.Assets().FindCodeNotes()->SetServerCodeNote(0x12, L"Test");
        inspector.PreparePublish(0x12, L"");

        Assert::IsTrue(inspector.CanPublishCurrentAddressNote());
        inspector.PublishCurrentAddressNote();

        Assert::AreEqual(0x12U, inspector.GetPublishedAddress());
        Assert::IsFalse(inspector.CanPublishCurrentAddressNote());
        Assert::IsFalse(inspector.CanRevertCurrentAddressNote());
    }

    TEST_METHOD(TestPublishCurrentAddressNoteCancel)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.Assets().FindCodeNotes()->SetServerCodeNote(0x12, L"Test");
        inspector.mockUserContext.SetUsername("Me");
        inspector.PreparePublish(0x12, L"Test2");

        bool bWindowSeen = false;
        inspector.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Overwrite note for address 0x0012?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Are you sure you want to replace Author's note:\n\nTest\n\nWith your note:\n\nTest2"), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());

            bWindowSeen = true;
            return ra::ui::DialogResult::No;
        });

        Assert::IsTrue(inspector.CanPublishCurrentAddressNote());
        inspector.PublishCurrentAddressNote();

        Assert::AreEqual(0U, inspector.GetPublishedAddress());
        Assert::IsTrue(inspector.CanPublishCurrentAddressNote());
        Assert::IsTrue(inspector.CanRevertCurrentAddressNote());
        Assert::IsTrue(bWindowSeen);
    }
    
    TEST_METHOD(TestRevertCurrentAddressNoteApprove)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.Assets().FindCodeNotes()->SetServerCodeNote(0x12, L"Test");
        inspector.mockUserContext.SetUsername("Me");
        inspector.PreparePublish(0x12, L"Test2");

        bool bWindowSeen = false;
        inspector.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Revert note for address 0x0012?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"This will discard all local work and revert the note to the last state retrieved from the server."), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());

            bWindowSeen = true;
            return ra::ui::DialogResult::Yes;
        });

        Assert::IsTrue(inspector.CanRevertCurrentAddressNote());
        inspector.RevertCurrentAddressNote();

        Assert::IsTrue(bWindowSeen);
        Assert::IsFalse(inspector.CanRevertCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"Test"), inspector.GetCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"Test"), *inspector.mockGameContext.Assets().FindCodeNotes()->FindCodeNote(0x12));
        Assert::IsFalse(inspector.mockGameContext.Assets().FindCodeNotes()->IsNoteModified(0x12));
    }

    TEST_METHOD(TestRevertCurrentAddressNoteCancel)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.Assets().FindCodeNotes()->SetServerCodeNote(0x12, L"Test");
        inspector.mockUserContext.SetUsername("Me");
        inspector.PreparePublish(0x12, L"Test2");

        bool bWindowSeen = false;
        inspector.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            Assert::AreEqual(std::wstring(L"Revert note for address 0x0012?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"This will discard all local work and revert the note to the last state retrieved from the server."), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::YesNo, vmMessageBox.GetButtons());

            bWindowSeen = true;
            return ra::ui::DialogResult::No;
        });

        Assert::IsTrue(inspector.CanRevertCurrentAddressNote());
        inspector.RevertCurrentAddressNote();

        Assert::IsTrue(bWindowSeen);
        Assert::IsTrue(inspector.CanRevertCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"Test2"), inspector.GetCurrentAddressNote());
        Assert::AreEqual(std::wstring(L"Test2"), *inspector.mockGameContext.Assets().FindCodeNotes()->FindCodeNote(0x12));
        Assert::IsTrue(inspector.mockGameContext.Assets().FindCodeNotes()->IsNoteModified(0x12));
    }

    TEST_METHOD(TestPublishCurrentAddressNoteFailed)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.PreparePublishFailure(0x12, L"Test");

        bool bWindowSeen = false;
        inspector.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>([&bWindowSeen](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox) {
            Assert::AreEqual(std::wstring(L"Publish failed."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"0 items successfully uploaded.\n\n1 items failed:\n* Code Notes: "), vmMessageBox.GetMessage());
            Assert::AreEqual(ra::ui::viewmodels::MessageBoxViewModel::Buttons::OK, vmMessageBox.GetButtons());

            bWindowSeen = true;
            return ra::ui::DialogResult::OK;
        });

        Assert::IsTrue(inspector.CanPublishCurrentAddressNote());
        inspector.PublishCurrentAddressNote();

        Assert::IsTrue(bWindowSeen);
        Assert::IsTrue(inspector.CanPublishCurrentAddressNote());
        Assert::IsTrue(inspector.CanRevertCurrentAddressNote());
    }

    TEST_METHOD(TestActiveGameChanged)
    {
        MemoryInspectorViewModelHarness inspector;
        Assert::AreEqual(std::wstring(L"Memory Inspector [no game loaded]"), inspector.GetWindowTitle());
        Assert::IsFalse(inspector.CanModifyNotes());

        inspector.mockGameContext.SetGameId({ 3 });
        inspector.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual(std::wstring(L"Memory Inspector"), inspector.GetWindowTitle());
        Assert::IsTrue(inspector.CanModifyNotes());

        inspector.mockGameContext.SetGameId({ 0 });
        inspector.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual(std::wstring(L"Memory Inspector [no game loaded]"), inspector.GetWindowTitle());
        Assert::IsFalse(inspector.CanModifyNotes());

        inspector.mockGameContext.SetGameId({ 5 });
        inspector.mockGameContext.SetMode(ra::data::context::GameContext::Mode::CompatibilityTest);
        inspector.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual(std::wstring(L"Memory Inspector [compatibility mode]"), inspector.GetWindowTitle());
        Assert::IsTrue(inspector.CanModifyNotes());
    }

    TEST_METHOD(TestActiveGameChangedOffline)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockConfiguration.SetFeatureEnabled(ra::services::Feature::Offline, true);
        Assert::AreEqual(std::wstring(L"Memory Inspector [no game loaded]"), inspector.GetWindowTitle());
        Assert::IsFalse(inspector.CanModifyNotes());

        inspector.mockGameContext.SetGameId({3});
        inspector.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual(std::wstring(L"Memory Inspector"), inspector.GetWindowTitle());
        Assert::IsTrue(inspector.CanModifyNotes());

        inspector.mockGameContext.SetGameId({0});
        inspector.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual(std::wstring(L"Memory Inspector [no game loaded]"), inspector.GetWindowTitle());
        Assert::IsFalse(inspector.CanModifyNotes());

        inspector.mockGameContext.SetGameId({5});
        inspector.mockGameContext.SetMode(ra::data::context::GameContext::Mode::CompatibilityTest);
        inspector.mockGameContext.NotifyActiveGameChanged();
        Assert::AreEqual(std::wstring(L"Memory Inspector [compatibility mode]"), inspector.GetWindowTitle());
        Assert::IsTrue(inspector.CanModifyNotes());
    }

    TEST_METHOD(TestActiveGameChangedClearsSearchResults)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId({ 3 });

        std::array<uint8_t, 32> memory{};
        for (size_t i = 0; i < memory.size(); ++i)
            memory.at(i) = gsl::narrow_cast<unsigned char>(i);

        inspector.mockEmulatorContext.MockMemory(memory);
        inspector.Search().BeginNewSearch();

        inspector.Search().SetComparisonType(ComparisonType::GreaterThan);
        inspector.Search().SetValueType(ra::services::SearchFilterType::InitialValue);

        memory.at(10) = 20;
        memory.at(20) = 30;
        inspector.Search().ApplyFilter();
        Assert::AreEqual({ 2U }, inspector.Search().GetResultCount());
        Assert::AreEqual({ 2U }, inspector.Search().Results().Count());

        inspector.mockGameContext.SetGameId({ 4 });
        inspector.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual({ 0U }, inspector.Search().GetResultCount());
        Assert::AreEqual({ 0U }, inspector.Search().Results().Count());
    }

    TEST_METHOD(TestCompatibilityModeChangedDoesNotClearSearchResults)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId({ 3 });
        inspector.mockGameContext.NotifyActiveGameChanged();

        std::array<uint8_t, 32> memory{};
        for (size_t i = 0; i < memory.size(); ++i)
            memory.at(i) = gsl::narrow_cast<unsigned char>(i);

        inspector.mockEmulatorContext.MockMemory(memory);
        inspector.Search().BeginNewSearch();

        inspector.Search().SetComparisonType(ComparisonType::GreaterThan);
        inspector.Search().SetValueType(ra::services::SearchFilterType::InitialValue);

        memory.at(10) = 20;
        memory.at(20) = 30;
        inspector.Search().ApplyFilter();
        Assert::AreEqual({ 2U }, inspector.Search().GetResultCount());
        Assert::AreEqual({ 2U }, inspector.Search().Results().Count());

        inspector.mockGameContext.SetMode(ra::data::context::GameContext::Mode::CompatibilityTest);
        inspector.mockGameContext.NotifyActiveGameChanged();

        Assert::AreEqual({ 2U }, inspector.Search().GetResultCount());
        Assert::AreEqual({ 2U }, inspector.Search().Results().Count());
    }

    TEST_METHOD(TestEndGameLoad)
    {
        MemoryInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId({ 3 });
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

        inspector.Viewer().SetSize(MemSize::ThirtyTwoBitBigEndian);
        inspector.SetCurrentAddress(12U);
        inspector.BookmarkCurrentAddress();

        Assert::AreEqual({3U}, pBookmarks.Count());
        Assert::AreEqual({2U}, pBookmarks.GetItemAt(0)->GetAddress());
        Assert::AreEqual(MemSize::EightBit, pBookmarks.GetItemAt(0)->GetSize());
        Assert::AreEqual({5U}, pBookmarks.GetItemAt(1)->GetAddress());
        Assert::AreEqual(MemSize::ThirtyTwoBit, pBookmarks.GetItemAt(1)->GetSize());
        Assert::AreEqual({12U}, pBookmarks.GetItemAt(2)->GetAddress());
        Assert::AreEqual(MemSize::ThirtyTwoBitBigEndian, pBookmarks.GetItemAt(2)->GetSize());
    }

    TEST_METHOD(TestBookmarkCurrentAddressIndirect)
    {
        MemoryInspectorViewModelHarness inspector;
        ra::services::mocks::MockAchievementRuntime mockAchievementRuntime;

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 8; i < memory.size(); i += 4)
            memory.at(i) = i;
        inspector.mockEmulatorContext.MockMemory(memory);
        memory.at(4) = 12;

        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({4U},
            L"[32-bit pointer] Player data\n"
            L"+4: [32-bit] Current HP\n"
            L"+8: [32-bit] Max HP");
        inspector.mockGameContext.DoFrame(); // force indirect code note initialization

        inspector.SetCurrentAddress(16U);
        inspector.BookmarkCurrentAddress();

        const auto& pBookmarks = inspector.mockWindowManager.MemoryBookmarks.Bookmarks();
        Assert::AreEqual({1U}, pBookmarks.Count());
        Assert::AreEqual({16U}, pBookmarks.GetItemAt(0)->GetAddress());
        Assert::AreEqual(MemSize::ThirtyTwoBit, pBookmarks.GetItemAt(0)->GetSize());
        Assert::AreEqual(std::string("I:0xX0004_M:0xX0004"), pBookmarks.GetItemAt(0)->GetIndirectAddress());
        Assert::AreEqual(std::wstring(L"[32-bit] Current HP"), pBookmarks.GetItemAt(0)->GetRealNote());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
