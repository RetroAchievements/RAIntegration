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
        const auto* pPointerNote = dynamic_cast<const ArrayMemoryNoteModel*>(&pNote);
        Assert::IsNotNull(pPointerNote, L"Provided note is not an array note");
        Ensures(pPointerNote != nullptr);
        return AssertIndirectNote(*pPointerNote, nOffset, sExpectedNote, nExpectedSize, nExpectedBytes);
    }

public:

    TEST_METHOD(TestGetPointerNoteAtOffsetStruct)
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

    TEST_METHOD(TestGetPointerNoteAtOffsetUnannotated)
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

    TEST_METHOD(TestPointerNoteAtOffsetArray)
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

    TEST_METHOD(TestPointerNoteAtOffsetNested)
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
};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
