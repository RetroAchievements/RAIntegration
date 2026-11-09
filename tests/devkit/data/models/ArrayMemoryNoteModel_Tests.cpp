#include "data/models/ArrayMemoryNoteModel.hh"

#include "data/models/PointerMemoryNoteModel.hh"

#include "tests/devkit/context/mocks/MockEmulatorMemoryContext.hh"

#include "testutil/MemoryAsserts.hh"

#include "util/Strings.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

TEST_CLASS(ArrayMemoryNoteModel_Tests)
{
private:
    static std::unique_ptr<ArrayMemoryNoteModel> Parse(const std::wstring& sNote)
    {
        auto pNote = MemoryNoteModel::Parse(sNote);
        auto* pArrayNoteRaw = dynamic_cast<ArrayMemoryNoteModel*>(pNote.get());
        if (!pArrayNoteRaw)
            Assert::Fail(L"Parse failed.");

        pNote.release();
        return std::unique_ptr<ArrayMemoryNoteModel>(pArrayNoteRaw);
    }

    static std::unique_ptr<PointerMemoryNoteModel> ParsePointer(const std::wstring& sNote)
    {
        auto pNote = MemoryNoteModel::Parse(sNote);
        auto* pPointerNoteRaw = dynamic_cast<PointerMemoryNoteModel*>(pNote.get());
        if (!pPointerNoteRaw)
            Assert::Fail(L"Parse failed.");

        pNote.release();
        return std::unique_ptr<PointerMemoryNoteModel>(pPointerNoteRaw);
    }

    const MemoryNoteModel& AssertIndirectNote(const ArrayMemoryNoteModel& pNote, unsigned int nOffset,
        const std::wstring& sExpectedNote, Memory::Size nExpectedSize, unsigned int nExpectedBytes)
    {
        const auto* offsetNote = pNote.GetNoteAtOffset(nOffset);
        Assert::IsNotNull(offsetNote, ra::util::String::Printf(L"No note found at offset 0x%04x", nOffset).c_str());
        Ensures(offsetNote != nullptr);

        const auto sMessage = ra::util::String::Printf(L"Offset 0x%04x", nOffset);
        Assert::AreEqual(nExpectedSize, offsetNote->GetMemSize(), sMessage.c_str());
        Assert::AreEqual(nExpectedBytes, offsetNote->GetBytes(), sMessage.c_str());
        Assert::AreEqual(sExpectedNote, offsetNote->GetNote(), sMessage.c_str());

        return *offsetNote;
    }

    const MemoryNoteModel& AssertIndirectNote(const MemoryNoteModel& pNote, unsigned int nOffset,
        const std::wstring& sExpectedNote, Memory::Size nExpectedSize, unsigned int nExpectedBytes)
    {
        const auto* pArrayNote = dynamic_cast<const ArrayMemoryNoteModel*>(&pNote);
        Assert::IsNotNull(pArrayNote, L"Provided note is not an array note");
        Ensures(pArrayNote != nullptr);
        return AssertIndirectNote(*pArrayNote, nOffset, sExpectedNote, nExpectedSize, nExpectedBytes);
    }

public:

    TEST_METHOD(TestGetNoteAtOffsetStruct)
    {
        const std::wstring sNote =
            L"[16-byte struct] Item Data\r\n"
            L"|0x00: [32-bit] Item Type\r\n"
            L"|0x04: [16-bit] Item Quantity";
        const auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::Array, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address
        Assert::AreEqual(16U, pNote->GetBytes());
        Assert::AreEqual(std::wstring(L"Item Data"), pNote->GetSummary());

        // extracted notes for offset fields
        AssertIndirectNote(*pNote, 0x00U, L"[32-bit] Item Type", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*pNote, 0x04U, L"[16-bit] Item Quantity", Memory::Size::SixteenBit, 2);
        Assert::IsNull(pNote->GetNoteAtOffset(0x10U));
    }

    TEST_METHOD(TestGetNoteAtOffsetUnannotated)
    {
        const std::wstring sNote =
            L"[16-bytes] Item Data\r\n"
            L"|0x00: [32-bit] Item Type\r\n"
            L"|0x04: [16-bit] Item Quantity";
        const auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::Array, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address
        Assert::AreEqual(16U, pNote->GetBytes());
        Assert::AreEqual(std::wstring(L"Item Data"), pNote->GetSummary());

        // extracted notes for offset fields
        AssertIndirectNote(*pNote, 0x00U, L"[32-bit] Item Type", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*pNote, 0x04U, L"[16-bit] Item Quantity", Memory::Size::SixteenBit, 2);
        Assert::IsNull(pNote->GetNoteAtOffset(0x10U));
    }

    TEST_METHOD(TestGetNoteAtOffsetArray)
    {
        const std::wstring sNote =
            L"[4x16 bytes] Item Data\r\n"
            L"|0x00: [32-bit] Item Type\r\n"
            L"|0x04: [16-bit] Item Quantity";
        const auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::Array, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address
        Assert::AreEqual(64U, pNote->GetBytes());
        Assert::AreEqual(std::wstring(L"Item Data"), pNote->GetSummary());

        // extracted notes for offset fields
        AssertIndirectNote(*pNote, 0x00U, L"[32-bit] Item Type", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*pNote, 0x04U, L"[16-bit] Item Quantity", Memory::Size::SixteenBit, 2);
        AssertIndirectNote(*pNote, 0x10U, L"[32-bit] Item Type", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*pNote, 0x14U, L"[16-bit] Item Quantity", Memory::Size::SixteenBit, 2);
        AssertIndirectNote(*pNote, 0x20U, L"[32-bit] Item Type", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*pNote, 0x24U, L"[16-bit] Item Quantity", Memory::Size::SixteenBit, 2);
        AssertIndirectNote(*pNote, 0x30U, L"[32-bit] Item Type", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*pNote, 0x34U, L"[16-bit] Item Quantity", Memory::Size::SixteenBit, 2);
        Assert::IsNull(pNote->GetNoteAtOffset(0x40U));
    }

    TEST_METHOD(TestGetNoteAtOffsetNested)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        const std::wstring sNote =
            L"[32-bit pointer] Root\r\n"
            L"+0x08: [4x16 bytes] Item Data\r\n"
            L"+|0x00: [32-bit] Item Type\r\n"
            L"+|0x04: [16-bit] Item Quantity";
        const auto pNote = ParsePointer(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, pNote->GetMemSize());
        const auto* pOffsetNote = dynamic_cast<const ArrayMemoryNoteModel*>(pNote->GetNoteAtOffset(0x08));
        Assert::IsNotNull(pOffsetNote);
        Ensures(pOffsetNote != nullptr);

        Assert::AreEqual(Memory::Size::Array, pOffsetNote->GetMemSize());
        Assert::AreEqual(std::wstring(L"Item Data"), pOffsetNote->GetSummary());

        // extracted notes for offset fields
        AssertIndirectNote(*pOffsetNote, 0x00U, L"[32-bit] Item Type", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*pOffsetNote, 0x04U, L"[16-bit] Item Quantity", Memory::Size::SixteenBit, 2);
        AssertIndirectNote(*pOffsetNote, 0x10U, L"[32-bit] Item Type", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*pOffsetNote, 0x14U, L"[16-bit] Item Quantity", Memory::Size::SixteenBit, 2);
        AssertIndirectNote(*pOffsetNote, 0x20U, L"[32-bit] Item Type", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*pOffsetNote, 0x24U, L"[16-bit] Item Quantity", Memory::Size::SixteenBit, 2);
        AssertIndirectNote(*pOffsetNote, 0x30U, L"[32-bit] Item Type", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*pOffsetNote, 0x34U, L"[16-bit] Item Quantity", Memory::Size::SixteenBit, 2);
        Assert::IsNull(pOffsetNote->GetNoteAtOffset(0x40U));
    }

    TEST_METHOD(TestGetNoteAtOffsetNestedPointer)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        const std::wstring sNote =
            L"[64 bytes] Array of NPC\r\n"
            L"|0x00=[32-bit] Pointer to NPC #1\r\n"
            L"|+0x210=[Float] NPC Health\r\n"
            L"..1.0=Max";
        const auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::Array, pNote->GetMemSize());
        const auto* pOffsetNote = dynamic_cast<const PointerMemoryNoteModel*>(pNote->GetNoteAtOffset(0x00));
        Assert::IsNotNull(pOffsetNote);
        Ensures(pOffsetNote != nullptr);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, pOffsetNote->GetMemSize());
        Assert::AreEqual(std::wstring(L"Pointer to NPC #1"), pOffsetNote->GetSummary());

        // extracted notes for offset fields
        const auto* pChildNote = pOffsetNote->GetNoteAtOffset(0x210);
        Assert::IsNotNull(pChildNote, ra::util::String::Printf(L"No note found at offset 0x%04x", 0x210).c_str());
        Ensures(pChildNote != nullptr);
        Assert::AreEqual(Memory::Size::Float, pChildNote->GetMemSize());
        Assert::AreEqual(std::wstring(L"[Float] NPC Health\r\n..1.0=Max"), pChildNote->GetNote());
    }

    TEST_METHOD(TestPlusInSummary)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        const std::wstring sNote =
            L"[63-byte] First line of text log (R+B)\r\n"
            L"\r\n"
            L"+0: in use\r\n"
            L"+1: inserts a separator above it\r\n"
            L"+3: start of the actual text\r\n"
            L"\r\n"
            L"Used to prevent false triggers for recruits(gets set to 0 on floor changes)";
        const auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::Array, pNote->GetMemSize());
        Assert::AreEqual({ 63U }, pNote->GetBytes());

        const auto* pOffsetNote = pNote->GetNoteAtOffset(0x00);
        Assert::IsNotNull(pOffsetNote);
        Ensures(pOffsetNote != nullptr);

        Assert::AreEqual(Memory::Size::Unknown, pOffsetNote->GetMemSize());
        Assert::AreEqual(std::wstring(L"in use"), pOffsetNote->GetSummary());

        pOffsetNote = pNote->GetNoteAtOffset(0x01);
        Assert::IsNotNull(pOffsetNote);
        Ensures(pOffsetNote != nullptr);

        Assert::AreEqual(Memory::Size::Unknown, pOffsetNote->GetMemSize());
        Assert::AreEqual(std::wstring(L"inserts a separator above it"), pOffsetNote->GetSummary());

        pOffsetNote = pNote->GetNoteAtOffset(0x03);
        Assert::IsNotNull(pOffsetNote);
        Ensures(pOffsetNote != nullptr);

        Assert::AreEqual(Memory::Size::Unknown, pOffsetNote->GetMemSize());
        Assert::AreEqual(std::wstring(L"start of the actual text"), pOffsetNote->GetSummary());
    }
};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
