#include "CppUnitTest.h"

#include "api\DeleteCodeNote.hh"
#include "api\UpdateCodeNote.hh"

#include "ui\viewmodels\PointerInspectorViewModel.hh"

#include "tests\ui\UIAsserts.hh"
#include "tests\RA_UnitTestHelpers.h"

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

constexpr int RootNodeID = -1;

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
            const auto *pField = Fields().GetItemAt(nIndex);
            Assert::IsNotNull(pField);
            Ensures(pField != nullptr);
            Assert::AreEqual(nOffset, pField->m_nOffset);
            Assert::AreEqual(nAddress, pField->GetAddress());
            Assert::AreEqual(sOffset, pField->GetOffset());
            Assert::AreEqual(sDescription, pField->GetDescription());
            Assert::AreEqual(nSize, pField->GetSize());
            Assert::AreEqual(nFormat, pField->GetFormat());
            Assert::AreEqual(sCurrentValue, pField->GetCurrentValue());
        }
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
            L"[32-bit pointer] Player data\n"
            L"+4: [32-bit] Current HP\n"
            L"+8: [32-bit] Max HP");

        Assert::AreEqual({ 4U }, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0004"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(L"[32-bit pointer] Player data"), inspector.GetCurrentAddressNote());

        Assert::AreEqual({ 2U }, inspector.Fields().Count());

        inspector.AssertField(0, 4, 16U, L"+0004", L"[32-bit] Current HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000010");
        inspector.AssertField(1, 8, 20U, L"+0008", L"[32-bit] Max HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000014");
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
            L"[8-bit pointer] Player data\n"
            L"+0x00: [8-bit pointer] Row 1\n"
            L".+0x00: [8-bit] Column 1a\n"
            L".+0x04: [8-bit pointer] Column 1b\n"
            L"..+0x00: [8-bit] Column 1a\n"
            L".+0x08: [8-bit] Column 1c\n"
            L"+0x08: [8-bit pointer] Row 2\n"
            L"+0x10: [8-bit pointer] Row 3\n"
            L"+0x18: Generic data"
        );

        Assert::AreEqual({4U}, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0004"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(L"[8-bit pointer] Player data"), inspector.GetCurrentAddressNote());

        Assert::AreEqual({5U}, inspector.Nodes().Count());
        Assert::AreEqual(RootNodeID, inspector.Nodes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"[8-bit pointer] Player data"), inspector.Nodes().GetItemAt(0)->GetLabel());
        Assert::AreEqual(0x00000000, inspector.Nodes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"+0000 | [8-bit pointer] Row 1"), inspector.Nodes().GetItemAt(1)->GetLabel());
        Assert::AreEqual(0x01000004, inspector.Nodes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L" +0004 | [8-bit pointer] Column 1b"), inspector.Nodes().GetItemAt(2)->GetLabel());
        Assert::AreEqual(0x00000008, inspector.Nodes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"+0008 | [8-bit pointer] Row 2"), inspector.Nodes().GetItemAt(3)->GetLabel());
        Assert::AreEqual(0x00000010, inspector.Nodes().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L"+0010 | [8-bit pointer] Row 3"), inspector.Nodes().GetItemAt(4)->GetLabel());

        Assert::AreEqual(RootNodeID, inspector.GetSelectedNode());
        Assert::AreEqual(std::wstring(L"[8-bit pointer] Player data"), inspector.GetCurrentAddressNote());
        Assert::AreEqual({4U}, inspector.Fields().Count());
        inspector.AssertField(0, 0, 0x0CU, L"+0000", L"[8-bit pointer] Row 1", MemSize::EightBit, MemFormat::Hex, L"14");
        inspector.AssertField(1, 8, 0x14U, L"+0008", L"[8-bit pointer] Row 2", MemSize::EightBit, MemFormat::Hex, L"1c");
        inspector.AssertField(2, 16, 0x1CU, L"+0010", L"[8-bit pointer] Row 3", MemSize::EightBit, MemFormat::Hex, L"24");
        inspector.AssertField(3, 24, 0x24U, L"+0018", L"Generic data", MemSize::EightBit, MemFormat::Hex, L"2c");

        inspector.SetSelectedNode(0x00000008);
        Assert::AreEqual(0x00000008, inspector.GetSelectedNode());
        Assert::AreEqual(std::wstring(L"[8-bit pointer] Row 2"), inspector.GetCurrentAddressNote());
        Assert::AreEqual({0U}, inspector.Fields().Count());

        inspector.SetSelectedNode(0x00000000);
        Assert::AreEqual(0x00000000, inspector.GetSelectedNode());
        Assert::AreEqual(std::wstring(L"[8-bit pointer] Row 1"), inspector.GetCurrentAddressNote());
        Assert::AreEqual({3U}, inspector.Fields().Count());
        inspector.AssertField(0, 0, 0x14U, L"+0000", L"[8-bit] Column 1a", MemSize::EightBit, MemFormat::Hex, L"1c");
        inspector.AssertField(1, 4, 0x18U, L"+0004", L"[8-bit pointer] Column 1b", MemSize::EightBit, MemFormat::Hex, L"20");
        inspector.AssertField(2, 8, 0x1CU, L"+0008", L"[8-bit] Column 1c", MemSize::EightBit, MemFormat::Hex, L"24");
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
            L"[32-bit pointer] Player data\n"
            L"+4: [32-bit] Current HP\n"
            L"+8: [32-bit] Max HP");

        Assert::AreEqual({4U}, inspector.GetCurrentAddress());
        Assert::AreEqual(std::wstring(L"0x0004"), inspector.GetCurrentAddressText());
        Assert::AreEqual(std::wstring(L"[32-bit pointer] Player data"), inspector.GetCurrentAddressNote());

        Assert::AreEqual({2U}, inspector.Fields().Count());

        inspector.AssertField(0, 4, 16U, L"+0004", L"[32-bit] Current HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000010");
        inspector.AssertField(1, 8, 20U, L"+0008", L"[32-bit] Max HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000014");

        // data changes
        memory.at(17) = 1;
        memory.at(20) = 99;
        inspector.DoFrame();
        inspector.AssertField(0, 4, 16U, L"+0004", L"[32-bit] Current HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000110");
        inspector.AssertField(1, 8, 20U, L"+0008", L"[32-bit] Max HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"00000063");

        // pointer changes
        memory.at(4) = 13;
        inspector.DoFrame();
        inspector.AssertField(0, 4, 17U, L"+0004", L"[32-bit] Current HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"63000001");
        inspector.AssertField(1, 8, 21U, L"+0008", L"[32-bit] Max HP", MemSize::ThirtyTwoBit, MemFormat::Hex, L"18000000");
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
