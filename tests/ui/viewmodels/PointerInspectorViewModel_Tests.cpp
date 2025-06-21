#include "CppUnitTest.h"

#include "api\DeleteCodeNote.hh"
#include "api\UpdateCodeNote.hh"

#include "ui\viewmodels\PointerInspectorViewModel.hh"

#include "tests\ui\UIAsserts.hh"
#include "tests\RA_UnitTestHelpers.h"

#include "tests\mocks\MockClipboard.hh"
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

TEST_CLASS(PointerInspectorViewModel_Tests)
{
private:
    class PointerInspectorViewModelHarness : public PointerInspectorViewModel
    {
    public:
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::services::mocks::MockClipboard mockClipboard;

        std::array<unsigned char, 32> memory{};

        GSL_SUPPRESS_F6 PointerInspectorViewModelHarness()
        {
            InitializeNotifyTargets();

            for (size_t i = 0; i < memory.size(); ++i)
                memory.at(i) = gsl::narrow_cast<unsigned char>(i);

            mockEmulatorContext.MockMemory(memory);

            mockUserContext.SetUsername("Author");

            mockGameContext.InitializeCodeNotes();
        }

        ~PointerInspectorViewModelHarness() = default;

        PointerInspectorViewModelHarness(const PointerInspectorViewModelHarness&) noexcept = delete;
        PointerInspectorViewModelHarness& operator=(const PointerInspectorViewModelHarness&) noexcept = delete;
        PointerInspectorViewModelHarness(PointerInspectorViewModelHarness&&) noexcept = delete;
        PointerInspectorViewModelHarness& operator=(PointerInspectorViewModelHarness&&) noexcept = delete;

        void DoFrame() override
        {
            mockGameContext.DoFrame(); // ensure note pointers get updated
            PointerInspectorViewModel::DoFrame();
        }

        const std::wstring* FindCodeNote(ra::ByteAddress nAddress) const
        {
            const auto* pCodeNotes = mockGameContext.Assets().FindCodeNotes();
            return (pCodeNotes != nullptr) ? pCodeNotes->FindCodeNote(nAddress) : nullptr;
        }

        void AssertField(gsl::index nIndex, int32_t nOffset, ra::ByteAddress nAddress,
            const std::wstring& sOffset, const std::wstring& sDescription,
            MemSize nSize, MemFormat nFormat, const std::wstring& sCurrentValue)
        {
            const auto* pField = Bookmarks().GetItemAt<StructFieldViewModel>(nIndex);
            Assert::IsNotNull(pField);
            Ensures(pField != nullptr);
            Assert::AreEqual(nOffset, pField->m_nOffset);
            Assert::AreEqual(nAddress, pField->GetAddress());
            Assert::AreEqual(sOffset, pField->GetOffset());
            Assert::AreEqual(sDescription, pField->GetRealNote());
            Assert::AreEqual(nSize, pField->GetSize());
            Assert::AreEqual(nFormat, pField->GetFormat());
            Assert::AreEqual(sCurrentValue, pField->GetCurrentValue());
        }

        void AssertFieldBody(gsl::index nIndex, const std::wstring& sBody)
        {
            const auto* pField = Bookmarks().GetItemAt<StructFieldViewModel>(nIndex);
            Assert::IsNotNull(pField);
            Ensures(pField != nullptr);
            Assert::AreEqual(sBody, pField->GetBody());
        }

        void AssertPointerChain(gsl::index nIndex, const std::wstring& sOffset,
                         ra::ByteAddress nAddress, const std::wstring& sDescription,
                         const std::wstring& sValue)
        {
            const auto* pPointer = PointerChain().GetItemAt(nIndex);
            Assert::IsNotNull(pPointer);
            Ensures(pPointer != nullptr);
            Assert::AreEqual(sOffset, pPointer->GetOffset());
            Assert::AreEqual(nAddress, pPointer->GetAddress());
            Assert::AreEqual(sDescription, pPointer->GetRealNote());
            Assert::AreEqual(sValue, pPointer->GetCurrentValue());
        }

        void AssertPointerChainAddressValid(gsl::index nIndex)
        {
            const auto* pPointer = PointerChain().GetItemAt(nIndex);
            Assert::IsNotNull(pPointer);
            Ensures(pPointer != nullptr);
            Assert::AreEqual(ra::ui::Color(0x00000000).ARGB, pPointer->GetRowColor().ARGB);
        }

        void AssertPointerChainAddressInvalid(gsl::index nIndex)
        {
            const auto* pPointer = PointerChain().GetItemAt(nIndex);
            Assert::IsNotNull(pPointer);
            Ensures(pPointer != nullptr);
            Assert::AreEqual(ra::ui::Color(0xFFFFFFC0).ARGB, pPointer->GetRowColor().ARGB);
        }

        void AssertPointerChainAddressNull(gsl::index nIndex)
        {
            const auto* pPointer = PointerChain().GetItemAt(nIndex);
            Assert::IsNotNull(pPointer);
            Ensures(pPointer != nullptr);
            Assert::AreEqual(ra::ui::Color(0xFFFFC0C0).ARGB, pPointer->GetRowColor().ARGB);
        }

        void AssertNote(ra::ByteAddress nAddress, const std::wstring& sExpectedNote)
        {
            const auto* pNote = mockGameContext.Assets().FindCodeNotes()->FindCodeNoteModel(nAddress);
            Assert::IsNotNull(pNote);
            Ensures(pNote != nullptr);
            Assert::AreEqual(sExpectedNote, pNote->GetNote());
        }

        bool HasSingleSelection() const { return GetValue(HasSingleSelectionProperty); }
    };


public:
    TEST_METHOD(TestInitialValues)
    {
        PointerInspectorViewModelHarness inspector;

        Assert::AreEqual({ 0U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0000"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(), inspector.GetCurrentAddressNote());
    }

    TEST_METHOD(TestSetCurrentAddress)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        inspector.SetCurrentAddress({ 3U });

        Assert::AreEqual({ 3U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0003"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(), inspector.GetCurrentAddressNote());
    }

    TEST_METHOD(TestSetCurrentAddressText)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        inspector.SetCurrentAddressText(L"3");

        Assert::AreEqual({ 3U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"3"), inspector.GetCurrentAddressText()); /* don't update text when user types in partial address */
        Assert::AreEqual(std::wstring(), inspector.GetCurrentAddressNote());
    }

    TEST_METHOD(TestSetCurrentAddressWithNote)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        inspector.SetCurrentAddress({ 3U });
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({3U}, L"Note on 3");

        Assert::AreEqual({ 3U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0003"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(L"Note on 3"), inspector.GetCurrentAddressNote());
        Assert::AreEqual({ 0U }, inspector.Bookmarks().Count());

        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({3U}, L"Modified Note on 3");

        Assert::AreEqual(std::wstring(L"Modified Note on 3"), inspector.GetCurrentAddressNote());
    }

    TEST_METHOD(TestSetCurrentAddressWithPointerNote)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 8; i < memory.size(); i += 4)
            memory.at(i) = i;
        inspector.mockEmulatorContext.MockMemory(memory);
        memory.at(4) = 12;

        inspector.SetCurrentAddress({4U});
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({4U},
            L"[32-bit pointer] Player data\r\n"
            L"+4: [32-bit] Current HP\r\n"
            L"+8: [32-bit] Max HP");

        Assert::AreEqual({ 4U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0004"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(L"[32-bit pointer] Player data"), inspector.GetCurrentAddressNote());

        Assert::AreEqual({ 2U }, inspector.Bookmarks().Count());

        inspector.AssertField(0, 4, 16U, L"+0004", L"Current HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000010");
        inspector.AssertField(1, 8, 20U, L"+0008", L"Max HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000014");
    }

    TEST_METHOD(TestSetCurrentAddressWithMultiLinePointerNote)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 8; i < memory.size(); i += 4)
            memory.at(i) = i;
        inspector.mockEmulatorContext.MockMemory(memory);
        memory.at(4) = 12;

        inspector.SetCurrentAddress({4U});
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({4U},
            L"[32-bit pointer] Player data\r\n"
            L"+4: [32-bit] Class\r\n"
            "1=Wizard\r\n"
            "2=Fighter\r\n"
            L"+8: [32-bit] Max HP");

        Assert::AreEqual({ 4U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0004"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(L"[32-bit pointer] Player data"), inspector.GetCurrentAddressNote());

        Assert::AreEqual({ 2U }, inspector.Bookmarks().Count());

        inspector.AssertField(0, 4, 16U, L"+0004", L"Class", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000010");
        inspector.AssertFieldBody(0, L"[32-bit] Class\r\n1=Wizard\r\n2=Fighter");
        inspector.AssertField(1, 8, 20U, L"+0008", L"Max HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000014");
        inspector.AssertFieldBody(1, L"[32-bit] Max HP");
    }

    TEST_METHOD(TestKnownPointers)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({1U}, L"Simple note");
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({4U},
            L"[32-bit pointer] Player data\r\n"
            L"+4: [32-bit] Current HP\r\n"
            L"+8: [32-bit] Max HP");
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({8U}, L"Something here");
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({16U},
            L"[32-bit pointer] Level data\r\n"
            L"+4: [32-bit] Current world\r\n"
            L"+8: [32-bit] Current stage");
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({20U}, L"Read this");

        inspector.mockGameContext.NotifyGameLoad();

        Assert::AreEqual({0U}, inspector.GetCurrentAddress());

        Assert::AreEqual({2U}, inspector.KnownPointers().Count());
        Assert::AreEqual(4, inspector.KnownPointers().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"0x0004 | Player data"), inspector.KnownPointers().GetItemAt(0)->GetLabel());
        Assert::AreEqual(16, inspector.KnownPointers().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"0x0010 | Level data"), inspector.KnownPointers().GetItemAt(1)->GetLabel());
    }

        TEST_METHOD(TestKnownPointersUpdateOnCodeNoteChange)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({1U}, L"Simple note");
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({4U},
            L"[32-bit pointer] Player data\r\n"
            L"+4: [32-bit] Current HP\r\n"
            L"+8: [32-bit] Max HP");
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({8U}, L"Something here");
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({16U},
            L"[32-bit pointer] Level data\r\n"
            L"+4: [32-bit] Current world\r\n"
            L"+8: [32-bit] Current stage");
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({20U}, L"Read this");

        inspector.mockGameContext.NotifyGameLoad();

        Assert::AreEqual({0U}, inspector.GetCurrentAddress());

        Assert::AreEqual({2U}, inspector.KnownPointers().Count());
        Assert::AreEqual(4, inspector.KnownPointers().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"0x0004 | Player data"), inspector.KnownPointers().GetItemAt(0)->GetLabel());
        Assert::AreEqual(16, inspector.KnownPointers().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"0x0010 | Level data"), inspector.KnownPointers().GetItemAt(1)->GetLabel());

        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({12U},
            L"[32-bit pointer] New pointer");

        Assert::AreEqual({3U}, inspector.KnownPointers().Count());
        Assert::AreEqual(4, inspector.KnownPointers().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"0x0004 | Player data"), inspector.KnownPointers().GetItemAt(0)->GetLabel());
        Assert::AreEqual(12, inspector.KnownPointers().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"0x000c | New pointer"), inspector.KnownPointers().GetItemAt(1)->GetLabel());
        Assert::AreEqual(16, inspector.KnownPointers().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"0x0010 | Level data"), inspector.KnownPointers().GetItemAt(2)->GetLabel());

        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({4U}, L"Normal note");
        Assert::AreEqual({2U}, inspector.KnownPointers().Count());
        Assert::AreEqual(12, inspector.KnownPointers().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"0x000c | New pointer"), inspector.KnownPointers().GetItemAt(0)->GetLabel());
        Assert::AreEqual(16, inspector.KnownPointers().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"0x0010 | Level data"), inspector.KnownPointers().GetItemAt(1)->GetLabel());

        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({16U}, L"Wrong again");
        Assert::AreEqual({1U}, inspector.KnownPointers().Count());
        Assert::AreEqual(12, inspector.KnownPointers().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"0x000c | New pointer"), inspector.KnownPointers().GetItemAt(0)->GetLabel());

        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({20U}, L"[32-bit pointer] Should be here instead");
        Assert::AreEqual({2U}, inspector.KnownPointers().Count());
        Assert::AreEqual(12, inspector.KnownPointers().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"0x000c | New pointer"), inspector.KnownPointers().GetItemAt(0)->GetLabel());
        Assert::AreEqual(20, inspector.KnownPointers().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"0x0014 | Should be here instead"), inspector.KnownPointers().GetItemAt(1)->GetLabel());
    }

    TEST_METHOD(TestSelectedNode)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 4; i < memory.size(); i += 4)
            memory.at(i) = i + 8;
        inspector.mockEmulatorContext.MockMemory(memory);

        inspector.SetCurrentAddress({4U});
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({4U},
            L"[8-bit pointer] Player data\r\n"
            L"+0x00: [8-bit pointer] Row 1\r\n"
            L".+0x00: [8-bit] Column 1a\r\n"
            L".+0x04: [8-bit pointer] Column 1b\r\n"
            L"..+0x00: [8-bit] Column 1a\r\n"
            L".+0x08: [8-bit] Column 1c\r\n"
            L"+0x08: [8-bit pointer] Row 2\r\n"
            L"+0x10: [8-bit pointer] Row 3\r\n"
            L"+0x18: Generic data"
        );

        Assert::AreEqual({4U}, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0004"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(L"[8-bit pointer] Player data"), inspector.GetCurrentAddressNote());

        Assert::AreEqual({5U}, inspector.Nodes().Count());
        Assert::AreEqual(PointerInspectorViewModel::PointerNodeViewModel::RootNodeId, inspector.Nodes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"[8-bit pointer] Player data"), inspector.Nodes().GetItemAt(0)->GetLabel());
        Assert::AreEqual(0x00000000, inspector.Nodes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"+0000 | [8-bit pointer] Row 1"), inspector.Nodes().GetItemAt(1)->GetLabel());
        Assert::AreEqual(0x01000004, inspector.Nodes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L" +0004 | [8-bit pointer] Column 1b"), inspector.Nodes().GetItemAt(2)->GetLabel());
        Assert::AreEqual(0x00000008, inspector.Nodes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"+0008 | [8-bit pointer] Row 2"), inspector.Nodes().GetItemAt(3)->GetLabel());
        Assert::AreEqual(0x00000010, inspector.Nodes().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L"+0010 | [8-bit pointer] Row 3"), inspector.Nodes().GetItemAt(4)->GetLabel());

        Assert::AreEqual(PointerInspectorViewModel::PointerNodeViewModel::RootNodeId, inspector.GetSelectedNode());
        Assert::AreEqual(std::wstring(L"[8-bit pointer] Player data"), inspector.GetCurrentAddressNote());
        Assert::AreEqual({4U}, inspector.Bookmarks().Count());
        inspector.AssertField(0, 0, 0x0CU, L"+0000", L"[pointer] Row 1", MemSize::EightBit, MemFormat::Hex, L"14");
        inspector.AssertField(1, 8, 0x14U, L"+0008", L"[pointer] Row 2", MemSize::EightBit, MemFormat::Hex, L"1c");
        inspector.AssertField(2, 16, 0x1CU, L"+0010", L"[pointer] Row 3", MemSize::EightBit, MemFormat::Hex, L"24");
        inspector.AssertField(3, 24, 0x24U, L"+0018", L"Generic data", MemSize::EightBit, MemFormat::Hex, L"2c");

        Assert::AreEqual({1U}, inspector.PointerChain().Count());
        inspector.AssertPointerChain(0, L"", 0x04, L"[8-bit pointer] Player data", L"0c");

        inspector.SetSelectedNode(0x00000008);
        Assert::AreEqual(0x00000008, inspector.GetSelectedNode());
        Assert::AreEqual(std::wstring(L"[8-bit pointer] Row 2"), inspector.GetCurrentAddressNote());
        Assert::AreEqual({0U}, inspector.Bookmarks().Count());

        Assert::AreEqual({2U}, inspector.PointerChain().Count());
        inspector.AssertPointerChain(0, L"", 0x04, L"[8-bit pointer] Player data", L"0c");
        inspector.AssertPointerChain(1, L"+0008", 0x14, L"[8-bit pointer] Row 2", L"1c");

        inspector.SetSelectedNode(0x00000000);
        Assert::AreEqual(0x00000000, inspector.GetSelectedNode());
        Assert::AreEqual(std::wstring(L"[8-bit pointer] Row 1"), inspector.GetCurrentAddressNote());
        Assert::AreEqual({3U}, inspector.Bookmarks().Count());
        inspector.AssertField(0, 0, 0x14U, L"+0000", L"Column 1a", MemSize::EightBit, MemFormat::Hex, L"1c");
        inspector.AssertField(1, 4, 0x18U, L"+0004", L"[pointer] Column 1b", MemSize::EightBit, MemFormat::Hex, L"20");
        inspector.AssertField(2, 8, 0x1CU, L"+0008", L"Column 1c", MemSize::EightBit, MemFormat::Hex, L"24");

        Assert::AreEqual({2U}, inspector.PointerChain().Count());
        inspector.AssertPointerChain(0, L"", 0x04, L"[8-bit pointer] Player data", L"0c");
        inspector.AssertPointerChain(1, L"+0000", 0x0C, L"[8-bit pointer] Row 1", L"14");
    }

    TEST_METHOD(TestSelectedField)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 8; i < memory.size(); i += 4)
            memory.at(i) = i;
        inspector.mockEmulatorContext.MockMemory(memory);
        memory.at(4) = 12;

        inspector.SetCurrentAddress({4U});
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({4U},
            L"[32-bit pointer] Player data\r\n"
            L"+4: [32-bit] Class\r\n"
            "1=Wizard\r\n"
            "2=Fighter\r\n"
            L"+8: [32-bit] Max HP\r\n"
            L"+12: [32-bit pointer] Inventory\r\n"
            L"++0: [16-bit] First item\r\n"
            L"++2: [16-bit] Second item");

        Assert::AreEqual({ 4U }, inspector.GetCurrentAddress());

        Assert::AreEqual({ 3U }, inspector.Bookmarks().Count());
        Assert::IsFalse(inspector.HasSelection());
        Assert::IsFalse(inspector.HasSingleSelection());
        Assert::AreEqual(std::wstring(), inspector.GetCurrentFieldNote());

        inspector.Bookmarks().GetItemAt(0)->SetSelected(true);
        Assert::IsTrue(inspector.HasSelection());
        Assert::IsTrue(inspector.HasSingleSelection());
        Assert::AreEqual(std::wstring(L"[32-bit] Class\r\n1=Wizard\r\n2=Fighter"), inspector.GetCurrentFieldNote());

        inspector.Bookmarks().GetItemAt(1)->SetSelected(true);
        Assert::IsTrue(inspector.HasSelection());
        Assert::IsFalse(inspector.HasSingleSelection());
        Assert::AreEqual(std::wstring(), inspector.GetCurrentFieldNote());

        inspector.Bookmarks().GetItemAt(0)->SetSelected(false);
        Assert::IsTrue(inspector.HasSelection());
        Assert::IsTrue(inspector.HasSingleSelection());
        Assert::AreEqual(std::wstring(L"[32-bit] Max HP"), inspector.GetCurrentFieldNote());

        inspector.Bookmarks().GetItemAt(1)->SetSelected(false);
        Assert::IsFalse(inspector.HasSelection());
        Assert::IsFalse(inspector.HasSingleSelection());
        Assert::AreEqual(std::wstring(), inspector.GetCurrentFieldNote());

        inspector.Bookmarks().GetItemAt(1)->SetSelected(true);
        Assert::IsTrue(inspector.HasSelection());
        Assert::IsTrue(inspector.HasSingleSelection());
        Assert::AreEqual(std::wstring(L"[32-bit] Max HP"), inspector.GetCurrentFieldNote());

        inspector.SetSelectedNode(0x0000000C);
        Assert::AreEqual({2U}, inspector.Bookmarks().Count());
        Assert::IsFalse(inspector.HasSelection());
        Assert::IsFalse(inspector.HasSingleSelection());
        Assert::AreEqual(std::wstring(), inspector.GetCurrentFieldNote());

        inspector.Bookmarks().GetItemAt(1)->SetSelected(true);
        Assert::IsTrue(inspector.HasSelection());
        Assert::IsTrue(inspector.HasSingleSelection());
        Assert::AreEqual(std::wstring(L"[16-bit] Second item"), inspector.GetCurrentFieldNote());
    }

    TEST_METHOD(TestUpdateCurrentFieldNoteSimple)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 8; i < memory.size(); i += 4)
            memory.at(i) = i;
        inspector.mockEmulatorContext.MockMemory(memory);
        memory.at(4) = 12;

        inspector.SetCurrentAddress({4U});
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({4U},
            L"[32-bit pointer] Player data\r\n"
            L"+8: [32-bit] Max HP");

        Assert::AreEqual({ 4U }, inspector.GetCurrentAddress());
        inspector.AssertField(0, 8, 20, L"+0008", L"Max HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000014");

        inspector.Bookmarks().GetItemAt(0)->SetSelected(true);
        Assert::AreEqual(std::wstring(L"[32-bit] Max HP"), inspector.GetCurrentFieldNote());

        inspector.SetCurrentFieldNote(L"[32-bit] Current HP");
        Assert::AreEqual(std::wstring(L"[32-bit] Current HP"), inspector.GetCurrentFieldNote());
        inspector.AssertField(0, 8, 20, L"+0008", L"Current HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000014");

        inspector.AssertNote({4U}, std::wstring(L"[32-bit pointer] Player data\r\n+0x08: [32-bit] Current HP"));
    }
    
    TEST_METHOD(TestUpdateCurrentFieldNoteMultiline)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 8; i < memory.size(); i += 4)
            memory.at(i) = i;
        inspector.mockEmulatorContext.MockMemory(memory);
        memory.at(4) = 12;

        inspector.SetCurrentAddress({4U});
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({4U},
            L"[32-bit pointer] Player data\r\n"
            L"+4: [32-bit] Class\r\n"
            "1=Wizard\r\n"
            "2=Fighter\r\n"
            L"+8: [32-bit] Max HP");

        Assert::AreEqual({ 4U }, inspector.GetCurrentAddress());
        inspector.AssertField(0, 4, 16, L"+0004", L"Class", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000010");

        inspector.Bookmarks().GetItemAt(0)->SetSelected(true);
        Assert::AreEqual(std::wstring(L"[32-bit] Class\r\n1=Wizard\r\n2=Fighter"), inspector.GetCurrentFieldNote());

        inspector.SetCurrentFieldNote(L"[32-bit] Class\r\n1=Mage\r\n2=Fighter");
        Assert::AreEqual(std::wstring(L"[32-bit] Class\r\n1=Mage\r\n2=Fighter"), inspector.GetCurrentFieldNote());
        inspector.AssertField(0, 4, 16, L"+0004", L"Class", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000010");

        inspector.AssertNote({4U}, std::wstring(L"[32-bit pointer] Player data\r\n+0x04: [32-bit] Class\r\n1=Mage\r\n2=Fighter\r\n+0x08: [32-bit] Max HP"));
    }

    TEST_METHOD(TestUpdateCurrentFieldNoteNested)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 8; i < memory.size(); i += 4)
            memory.at(i) = i;
        inspector.mockEmulatorContext.MockMemory(memory);
        memory.at(4) = 12;

        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({4U},
            L"[32-bit pointer] Player data\r\n"
            L"+8: [32-bit] Max HP\r\n"//\r\n"
            L"+12: [32-bit pointer] Inventory\r\n"
            L"++8: [16-bit] First item\r\n"
            L"++10: [16-bit] Second item");
        inspector.SetCurrentAddress({4U});
        inspector.SetSelectedNode(0x0000000C);

        Assert::AreEqual({ 4U }, inspector.GetCurrentAddress());
        inspector.AssertField(0, 8, 32, L"+0008", L"First item", MemSize::SixteenBit, MemFormat::Hex, L"0020");

        inspector.Bookmarks().GetItemAt(0)->SetSelected(true);
        Assert::AreEqual(std::wstring(L"[16-bit] First item"), inspector.GetCurrentFieldNote());

        inspector.SetCurrentFieldNote(L"[16-bit] 1st item");
        Assert::AreEqual(std::wstring(L"[16-bit] 1st item"), inspector.GetCurrentFieldNote());
        inspector.AssertField(0, 8, 32, L"+0008", L"1st item", MemSize::SixteenBit, MemFormat::Hex, L"0020");

        inspector.AssertNote({4U}, std::wstring(L"[32-bit pointer] Player data\r\n+0x08: [32-bit] Max HP\r\n+0x0C: [32-bit pointer] Inventory\r\n"
                                                L"++0x08: [16-bit] 1st item\r\n++0x0A: [16-bit] Second item"));
    }

    TEST_METHOD(TestCopyDefinition)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 4; i < memory.size(); i += 4)
            memory.at(i) = i + 8;
        inspector.mockEmulatorContext.MockMemory(memory);

        inspector.SetCurrentAddress({4U});
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({4U},
            L"[8-bit pointer] Player data\r\n"
            L"+0x00: [8-bit pointer] Row 1\r\n"
            L".+0x00: [8-bit] Column 1a\r\n"
            L".+0x04: [8-bit pointer] Column 1b\r\n"
            L"..+0x00: [8-bit] Column 1a\r\n"
            L".+0x08: [8-bit] Column 1c\r\n"
            L"+0x08: [8-bit pointer] Row 2\r\n"
            L"+0x10: [8-bit pointer] Row 3\r\n"
            L"+0x18: Generic data"
        );

        Assert::AreEqual({4U}, inspector.GetCurrentAddress());

        Assert::AreEqual(PointerInspectorViewModel::PointerNodeViewModel::RootNodeId, inspector.GetSelectedNode());
        Assert::AreEqual(std::wstring(L"[8-bit pointer] Player data"), inspector.GetCurrentAddressNote());
        inspector.AssertField(1, 8, 0x14U, L"+0008", L"[pointer] Row 2", MemSize::EightBit, MemFormat::Hex, L"1c");

        inspector.CopyDefinition();
        Assert::AreEqual(std::wstring(), inspector.mockClipboard.GetText());

        inspector.Bookmarks().GetItemAt(1)->SetSelected(true);
        inspector.CopyDefinition();
        Assert::AreEqual(std::wstring(L"I:0xH0004_0xH0008=28"), inspector.mockClipboard.GetText());

        inspector.SetSelectedNode(0x01000004);
        Assert::AreEqual(0x01000004, inspector.GetSelectedNode());
        Assert::AreEqual(std::wstring(L"[8-bit pointer] Column 1b"), inspector.GetCurrentAddressNote());
        inspector.AssertField(0, 0, 0x20U, L"+0000", L"Column 1a", MemSize::EightBit, MemFormat::Hex, L"28");
        inspector.CopyDefinition();
        Assert::AreEqual(std::wstring(), inspector.mockClipboard.GetText());

        inspector.Bookmarks().GetItemAt(0)->SetSelected(true);
        inspector.CopyDefinition();
        Assert::AreEqual(std::wstring(L"I:0xH0004_I:0xH0000_I:0xH0004_0xH0000=40"), inspector.mockClipboard.GetText());
    }

    TEST_METHOD(TestDoFrame)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        std::array<uint8_t, 64> memory = {};
        for (uint8_t i = 8; i < memory.size(); i += 4)
            memory.at(i) = i;
        inspector.mockEmulatorContext.MockMemory(memory);
        memory.at(4) = 12;

        inspector.SetCurrentAddress({4U});
        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({4U},
            L"[32-bit pointer] Player data\r\n"
            L"+4: [32-bit] Current HP\r\n"
            L"+8: [32-bit] Max HP");

        Assert::AreEqual({4U}, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0004"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(L"[32-bit pointer] Player data"), inspector.GetCurrentAddressNote());

        Assert::AreEqual({2U}, inspector.Bookmarks().Count());
        inspector.AssertField(0, 4, 16U, L"+0004", L"Current HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000010");
        inspector.AssertField(1, 8, 20U, L"+0008", L"Max HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000014");

        Assert::AreEqual({1U}, inspector.PointerChain().Count());
        inspector.AssertPointerChain(0, L"", 0x04, L"[32-bit pointer] Player data", L"0000000c");

        // data changes
        memory.at(17) = 1;
        memory.at(20) = 99;
        inspector.DoFrame();
        inspector.AssertField(0, 4, 16U, L"+0004", L"Current HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000110");
        inspector.AssertField(1, 8, 20U, L"+0008", L"Max HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000063");

        Assert::AreEqual({1U}, inspector.PointerChain().Count());
        inspector.AssertPointerChain(0, L"", 0x04, L"[32-bit pointer] Player data", L"0000000c");

        // pointer changes
        memory.at(4) = 13;
        inspector.DoFrame();
        inspector.AssertField(0, 4, 17U, L"+0004", L"Current HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"63000001");
        inspector.AssertField(1, 8, 21U, L"+0008", L"Max HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"18000000");

        Assert::AreEqual({1U}, inspector.PointerChain().Count());
        inspector.AssertPointerChain(0, L"", 0x04, L"[32-bit pointer] Player data", L"0000000d");
    }

    TEST_METHOD(TestDoFrameNested)
    {
        PointerInspectorViewModelHarness inspector;
        inspector.mockGameContext.SetGameId(1);
        inspector.mockGameContext.NotifyActiveGameChanged(); // enable note support

        std::array<uint8_t, 64> memory = {};
        inspector.mockConsoleContext.AddMemoryRegion(0x00, 0x3F, ra::data::context::ConsoleContext::AddressType::SystemRAM);
        inspector.mockEmulatorContext.MockMemory(memory);

        inspector.mockGameContext.Assets().FindCodeNotes()->SetCodeNote({4U},
            L"[32-bit pointer] Player data\r\n"
            L"+4: [32-bit pointer] 1st player\r\n"
            L"++8: [32-bit pointer] Health\r\n"
            L"+++0: [32-bit] Current HP\r\n"
            L"+++8: [32-bit] Max HP");

        memory.at(0x0004) = 0x0C;
        memory.at(0x0008) = 0x08;
        memory.at(0x000C) = 0x30;
        memory.at(0x0010) = 0x34;
        memory.at(0x0038) = 0x24;
        memory.at(0x003C) = 0x20;
        memory.at(0x0020) = 0x56;
        memory.at(0x0024) = 0x60;
        memory.at(0x0028) = 0x64;

        // update pointer values in note
        inspector.mockGameContext.Assets().FindCodeNotes()->DoFrame();

        inspector.SetCurrentAddress({4U});
        Assert::AreEqual({4U}, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0004"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(L"[32-bit pointer] Player data"), inspector.GetCurrentAddressNote());

        inspector.SetSelectedNode(0x01000008);
        Assert::AreEqual(std::wstring(L"[32-bit pointer] Health"), inspector.GetCurrentAddressNote());

        // all pointers are good
        Assert::AreEqual({2U}, inspector.Bookmarks().Count());
        inspector.AssertField(0, 0, 0x20U, L"+0000", L"Current HP", // 20+00=20 (56)
            MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000056");
        inspector.AssertField(1, 8, 0x28U, L"+0008", L"Max HP",     // 20+08=28 (64)
            MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000064");

        Assert::AreEqual({3U}, inspector.PointerChain().Count());
        inspector.AssertPointerChain(0, L"", 0x04, L"[32-bit pointer] Player data", L"0000000c");     //    04    (0C)
        inspector.AssertPointerChain(1, L"+0004", 0x10, L"[32-bit pointer] 1st player", L"00000034"); // 0C+04=10 (34)
        inspector.AssertPointerChain(2, L" +0008", 0x3C, L"[32-bit pointer] Health", L"00000020");    // 34+08=3C (20)
        inspector.AssertPointerChainAddressValid(0);
        inspector.AssertPointerChainAddressValid(1);
        inspector.AssertPointerChainAddressValid(2);

        // second level is null
        memory.at(0x0010) = 0x00;
        inspector.DoFrame();

        inspector.AssertPointerChain(0, L"", 0x04, L"[32-bit pointer] Player data", L"0000000c");     //    04    (0C)
        inspector.AssertPointerChain(1, L"+0004", 0x10, L"[32-bit pointer] 1st player", L"00000000"); // 0C+04=10 (00)
        inspector.AssertPointerChain(2, L" +0008", 0x08, L"[32-bit pointer] Health", L"00000008");    // 00+08=08 (08)
        inspector.AssertPointerChainAddressValid(0);
        inspector.AssertPointerChainAddressNull(1);
        inspector.AssertPointerChainAddressValid(2);
        inspector.AssertField(0, 0, 0x08U, L"+0000", L"Current HP", // 08+00=08 (08)
                              MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000008");
        inspector.AssertField(1, 8, 0x10U, L"+0008", L"Max HP",     // 08+08=10 (00)
                              MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000000");
        
        // third level is out of range
        memory.at(0x0004) = 0x20;
        inspector.DoFrame();

        inspector.AssertPointerChain(0, L"", 0x04, L"[32-bit pointer] Player data", L"00000020");     //    04    (20)
        inspector.AssertPointerChain(1, L"+0004", 0x24, L"[32-bit pointer] 1st player", L"00000060"); // 20+04=24 (60)
        inspector.AssertPointerChain(2, L" +0008", 0x68, L"[32-bit pointer] Health", L"00000000");    // 60+08=68 (00)
        inspector.AssertPointerChainAddressValid(0);
        inspector.AssertPointerChainAddressInvalid(1);
        inspector.AssertPointerChainAddressNull(2);
        inspector.AssertField(0, 0, 0x00U, L"+0000", L"Current HP", // 00+00=00 (00)
                              MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000000");
        inspector.AssertField(1, 8, 0x08U, L"+0008", L"Max HP",     // 00+08=08 (08)
                              MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000008");
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
