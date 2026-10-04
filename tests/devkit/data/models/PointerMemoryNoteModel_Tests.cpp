#include "data/models/PointerMemoryNoteModel.hh"

#include "tests/devkit/context/mocks/MockConsoleContext.hh"
#include "tests/devkit/context/mocks/MockEmulatorMemoryContext.hh"

#include "testutil/MemoryAsserts.hh"

#include "util/Strings.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

TEST_CLASS(PointerMemoryNoteModel_Tests)
{
private:
    static std::unique_ptr<PointerMemoryNoteModel> Parse(const std::wstring& sNote)
    {
        auto pNote = MemoryNoteModel::Parse(sNote);
        auto* pPointerNoteRaw = dynamic_cast<PointerMemoryNoteModel*>(pNote.get());
        if (!pPointerNoteRaw)
            Assert::Fail(L"Parse failed.");

        pNote.release();
        return std::unique_ptr<PointerMemoryNoteModel>(pPointerNoteRaw);
    }

    const MemoryNoteModel& AssertIndirectNote(const PointerMemoryNoteModel& pNote, unsigned int nOffset,
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
        const auto* pPointerNote = dynamic_cast<const PointerMemoryNoteModel*>(&pNote);
        Assert::IsNotNull(pPointerNote, L"Provided note is not a pointer note");
        Ensures(pPointerNote != nullptr);
        return AssertIndirectNote(*pPointerNote, nOffset, sExpectedNote, nExpectedSize, nExpectedBytes);
    }

public:

    TEST_METHOD(TestGetPointerNoteAtOffset)
    {
        const std::wstring sNote =
            L"Bomb Timer Pointer (24-bit)\r\n"
            L"+03 - Bombs Defused\r\n"
            L"+04 - Bomb Timer";
        auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::TwentyFourBit, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address

        // extracted notes for offset fields
        AssertIndirectNote(*pNote, 3U, L"Bombs Defused", Memory::Size::Unknown, 1);
        AssertIndirectNote(*pNote, 4U, L"Bomb Timer", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestGetPointerNoteAtOffsetMultiline)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0x1BC | Equipment - Head - String[24 Bytes]\r\n"
            L"---DEFAULT_HEAD = Barry's Head\r\n"
            L"---FRAGGER_HEAD = Fragger Helmet";
        auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address

        // extracted notes for offset fields
        AssertIndirectNote(*pNote, 0x1BCU,
                           L"Equipment - Head - String[24 Bytes]\r\n"
                           L"---DEFAULT_HEAD = Barry's Head\r\n"
                           L"---FRAGGER_HEAD = Fragger Helmet",
                           Memory::Size::Array, 24);
    }

    TEST_METHOD(TestGetPointerNoteAtOffsetMultilineWithHeader)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        const std::wstring sNote =
            L"[PAL]\r\n"
            L"Pointer [32bit]\r\n"
            L"+0x1BC | Equipment - Head - String[24 Bytes]\r\n"
            L"---DEFAULT_HEAD = Barry's Head\r\n"
            L"---FRAGGER_HEAD = Fragger Helmet";
        auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address

        // extracted notes for offset fields
        AssertIndirectNote(*pNote, 0x1BCU,
                           L"Equipment - Head - String[24 Bytes]\r\n"
                           L"---DEFAULT_HEAD = Barry's Head\r\n"
                           L"---FRAGGER_HEAD = Fragger Helmet",
                           Memory::Size::Array, 24);
    }

    TEST_METHOD(TestGetPointerNoteAtNegativeOffset)
    {
        const std::wstring sNote =
            L"Bomb Timer Pointer (24-bit)\r\n"
            L"+0xFFFFFFFE - Bombs Defused\r\n" // -2
            L"+0xFFFFFFFC - Bomb Timer";       // -4
        auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::TwentyFourBit, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address

        // extracted notes for offset fields
        AssertIndirectNote(*pNote, 0xFFFFFFFEU, L"Bombs Defused", Memory::Size::Unknown, 1);
        AssertIndirectNote(*pNote, 0xFFFFFFFCU, L"Bomb Timer", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestExtractFormatIndirect)
    {
        const std::wstring sNote =
            L"Bomb Timer Pointer (24-bit)\r\n"
            L"+03 - Bombs Defused\r\n"
            L"+04 - Bomb Timer (BCD)\r\n"
            L"+08 - [24-bit pointer] Bomb Info\r\n"
            L"++00 - [8-bit] Type\r\n"
            L"++04 - [8-bit] Color (hex)\r\n"
            L"+10 - Bombs Remaining\r\n";
        auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Format::Hex, pNote->GetDefaultMemFormat()); // pointer
        Assert::AreEqual(Memory::Format::Dec, pNote->GetNoteAtOffset(3)->GetDefaultMemFormat()); // bombs defused
        Assert::AreEqual(Memory::Format::Hex, pNote->GetNoteAtOffset(4)->GetDefaultMemFormat()); // bomb timer
        Assert::AreEqual(Memory::Format::Dec, pNote->GetNoteAtOffset(10)->GetDefaultMemFormat()); // bombs remaining

        const auto* nestedNote = dynamic_cast<const PointerMemoryNoteModel*>(pNote->GetNoteAtOffset(8));
        Expects(nestedNote != nullptr);
        Assert::AreEqual(Memory::Format::Hex, nestedNote->GetDefaultMemFormat()); // bomb info pointer
        Assert::AreEqual(Memory::Format::Dec, nestedNote->GetNoteAtOffset(0)->GetDefaultMemFormat()); // type
        Assert::AreEqual(Memory::Format::Hex, nestedNote->GetNoteAtOffset(4)->GetDefaultMemFormat());  // color
    }

    TEST_METHOD(TestHeaderedPointer)
    {
        const std::wstring sNote =
            L"Pointer (16bit because negative)\r\n\r\n"
            L"Circuit:\r\n"
            L"+0x1B56E = Current Position\r\n"
            L"+0x1B57E = Total Racers\r\n\r\n"
            L"Free Run:\r\n"
            L"+0x1B5BE = Seconds 0x\r\n"
            L"+0x1B5CE = Lap";
        auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::SixteenBit, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address

        // extracted notes for offset fields (note: pointer base default is $0000)
        AssertIndirectNote(*pNote, 0x1B56EU, L"Current Position", Memory::Size::Unknown, 1);
        AssertIndirectNote(*pNote, 0x1B57EU, L"Total Racers\r\n\r\nFree Run:", Memory::Size::Unknown, 1);
        AssertIndirectNote(*pNote, 0x1B5BEU, L"Seconds 0x", Memory::Size::Unknown, 1);
        AssertIndirectNote(*pNote, 0x1B5CEU, L"Lap", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestPointerOverlap)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        const std::wstring sNote =
            L"Pointer\r\r\n"
            L"[OFFSETS]\r\r\n"
            L"+2 = EXP (32-bit)\r\r\n"
            L"+5 = Base Level (8-bit)\r\r\n" // 32-bit value at 2 continues into 5
            L"+6 = Job Level (8-bit)";
        auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address

        // extracted notes for offset fields (note: pointer base default is $0000)
        AssertIndirectNote(*pNote, 2, L"EXP (32-bit)", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(*pNote, 5, L"Base Level (8-bit)", Memory::Size::EightBit, 1);
        AssertIndirectNote(*pNote, 6, L"Job Level (8-bit)", Memory::Size::EightBit, 1);
    }

    TEST_METHOD(TestNestedPointer)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0x428 | Pointer - Award - Tee Hee Two (32bit)\r\n"
            L"--- +0x24C | Flag\r\n"
            L"+0x438 | Pointer - Award - Pretty Woman (32bit)\r\n"
            L"--- +0x24C | Flag";
        auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address

        const auto& offsetNote = AssertIndirectNote(*pNote, 0x428,
            L"Pointer - Award - Tee Hee Two (32bit)\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(offsetNote, 0x24C, L"Flag", Memory::Size::Unknown, 1);

        const auto& offsetNote2 = AssertIndirectNote(*pNote, 0x438,
            L"Pointer - Award - Pretty Woman (32bit)\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(offsetNote2, 0x24C, L"Flag", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestUnannotatedPointerChain)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0x428 | Award - Tee Hee Two\r\n"
            L"++0x24C | Flag\r\n"
            L"+0x438 | Award - Pretty Woman\r\n"
            L"++0x24C | Flag";
        auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address

        const auto& offsetNote = AssertIndirectNote(
            *pNote, 0x428, L"Award - Tee Hee Two\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        Assert::AreEqual(std::wstring(L"Award - Tee Hee Two"), offsetNote.GetSummary());
        AssertIndirectNote(offsetNote, 0x24C, L"Flag", Memory::Size::Unknown, 1);

        const auto& offsetNote2 = AssertIndirectNote(*pNote, 0x438, L"Award - Pretty Woman\r\n+0x24C | Flag",
                                                     Memory::Size::ThirtyTwoBit, 4);
        Assert::AreEqual(std::wstring(L"Award - Pretty Woman"), offsetNote2.GetSummary());
        AssertIndirectNote(offsetNote2, 0x24C, L"Flag", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestNestedPointerAlternateFormat)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0x428 | (32-bit pointer) Award - Tee Hee Two\r\n"
            L"++0x24C | Flag\r\n"
            L"+0x438 | (32-bit pointer) Award - Pretty Woman\r\n"
            L"++0x24C | Flag";
        auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address

        const auto& offsetNote = AssertIndirectNote(*pNote, 0x428,
            L"(32-bit pointer) Award - Tee Hee Two\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(offsetNote, 0x24C, L"Flag", Memory::Size::Unknown, 1);

        const auto& offsetNote2 = AssertIndirectNote(*pNote, 0x438,
            L"(32-bit pointer) Award - Pretty Woman\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(offsetNote2, 0x24C, L"Flag", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestNestedPointerBracketNotSeparator)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0x428 [32bit] Pointer - Award - Tee Hee Two\r\n"
            L"--- +0x24C [8bit] Flag\r\n"
            L"+0x438 [32bit] Pointer - Award - Pretty Woman\r\n"
            L"--- +0x24C [8bit] Flag";
        auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address

        const auto& offsetNote = AssertIndirectNote(*pNote, 0x428,
            L"[32bit] Pointer - Award - Tee Hee Two\r\n+0x24C [8bit] Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(offsetNote, 0x24C, L"[8bit] Flag", Memory::Size::EightBit, 1);

        const auto& offsetNote2 = AssertIndirectNote(*pNote, 0x438,
            L"[32bit] Pointer - Award - Pretty Woman\r\n+0x24C [8bit] Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(offsetNote2, 0x24C, L"[8bit] Flag", Memory::Size::EightBit, 1);
    }

    TEST_METHOD(TestNestedPointerMultiLine)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0x428 | Obj1 pointer\r\n"
            L"++0x24C | [16-bit] State\r\n"
            L"-- Increments\r\n"
            L"+0x438 | Obj2 pointer\r\n"
            L"++0x08 | Flag\r\n"
            L"-- b0=quest1 complete\r\n"
            L"-- b1=quest2 complete\r\n"
            L"+0x448 | [32-bit BE] Not-nested number";
        auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address

        const auto& offsetNote = AssertIndirectNote(*pNote, 0x428, L"Obj1 pointer\r\n+0x24C | [16-bit] State\r\n-- Increments",
                                                    Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(offsetNote, 0x24C, L"[16-bit] State\r\n-- Increments", Memory::Size::SixteenBit, 2);

        const auto& offsetNote2 = AssertIndirectNote(*pNote, 0x438, L"Obj2 pointer\r\n+0x08 | Flag\r\n-- b0=quest1 complete\r\n-- b1=quest2 complete",
                                                     Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(offsetNote2, 0x08, L"Flag\r\n-- b0=quest1 complete\r\n-- b1=quest2 complete", Memory::Size::Unknown, 1);

        AssertIndirectNote(*pNote, 0x448, L"[32-bit BE] Not-nested number", Memory::Size::ThirtyTwoBitBigEndian, 4);
    }

    TEST_METHOD(TestNestedPointerRepeatedNodes)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0x308\r\n"
            L"++0x428 | (32-bit pointer) Award - Tee Hee Two\r\n"
            L"+++0x24C | Flag\r\n"
            L"+0x308\r\n"
            L"++0x438 | (32-bit pointer) Award - Pretty Woman\r\n"
            L"+++0x24C | Flag";
        auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address

        // expect individual nodes to be merged together
        const auto& sharedNote = AssertIndirectNote(*pNote, 0x308,
            L"+0x428 | (32-bit pointer) Award - Tee Hee Two\r\n"
            L"++0x24C | Flag\r\n"
            L"+0x438 | (32-bit pointer) Award - Pretty Woman\r\n"
            L"++0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);

        const auto& offsetNote = AssertIndirectNote(sharedNote, 0x428,
            L"(32-bit pointer) Award - Tee Hee Two\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(offsetNote, 0x24C, L"Flag", Memory::Size::Unknown, 1);

        const auto& offsetNote2 = AssertIndirectNote(sharedNote, 0x438,
            L"(32-bit pointer) Award - Pretty Woman\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(offsetNote2, 0x24C, L"Flag", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestImpliedPointerChain)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        const std::wstring sNote =
            L"Root Pointer [24 bits]\r\n"
            L"\r\n"
            L"+0x8318\r\n"
            L"++0x8014\r\n"
            L"+++0x8004 = First [32-bits]\r\n"
            L"+++0x8008 = Second [32-bits]";
        auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::TwentyFourBit, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address

        const auto& nestedNote = AssertIndirectNote(*pNote, 0x8318,
            L"+0x8014\r\n++0x8004 = First [32-bits]\r\n++0x8008 = Second [32-bits]", Memory::Size::ThirtyTwoBit, 4);
        Assert::AreEqual(std::wstring(L""), nestedNote.GetSummary());
        const auto& nestedNote2 = AssertIndirectNote(nestedNote, 0x8014,
            L"+0x8004 = First [32-bits]\r\n+0x8008 = Second [32-bits]", Memory::Size::ThirtyTwoBit, 4);
        Assert::AreEqual(std::wstring(L""), nestedNote2.GetSummary());
        AssertIndirectNote(nestedNote2, 0x8004, L"First [32-bits]", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(nestedNote2, 0x8008, L"Second [32-bits]", Memory::Size::ThirtyTwoBit, 4);
    }

    TEST_METHOD(TestArrayPointer)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;

        // The 80-bytes subnote reflects the size of the structure that is pointed-to.
        const std::wstring sNote =
            L"Pointer [32bit] (80 bytes)\r\n"
            L"+0x428 | Pointer - Award - Tee Hee Two (32bit)\r\n"
            L"--- +0x24C | Flag\r\n"
            L"+0x438 | Pointer - Award - Pretty Woman (32bit)\r\n"
            L"--- +0x24C | Flag";
        const auto pNote = Parse(sNote);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, pNote->GetMemSize());
        Assert::AreEqual(sNote, pNote->GetNote()); // full note for pointer address
        Assert::AreEqual(std::wstring(L"Pointer [32bit] (80 bytes)"), pNote->GetFullSummary());
        Assert::AreEqual(std::wstring(L"Pointer (80 bytes)"), pNote->GetSummary());

        const auto& offsetNote = AssertIndirectNote(*pNote, 0x428,
            L"Pointer - Award - Tee Hee Two (32bit)\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(offsetNote, 0x24C, L"Flag", Memory::Size::Unknown, 1);

        const auto& offsetNote2 = AssertIndirectNote(*pNote, 0x438,
            L"Pointer - Award - Pretty Woman (32bit)\r\n+0x24C | Flag", Memory::Size::ThirtyTwoBit, 4);
        AssertIndirectNote(offsetNote2, 0x24C, L"Flag", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestPointerOverflow)
    {
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        mockConsoleContext.AddMemoryRegion(0, 0x7F, ra::data::MemoryRegion::Type::SystemRAM, 0x80000000);

        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        std::array<unsigned char, 256> memory{};
        memory.at(0x04) = 0x10; // pointer@0x04 = 0x80000010
        memory.at(0x07) = 0x80;
        memory.at(0x14) = 0x20; // pointer@0x14 = 0x80000020
        memory.at(0x17) = 0x80;
        mockEmulatorMemoryContext.MockMemory(memory);

        const std::wstring sNote =
            L"[32-bit pointer] root\r\n"
            L"+0x80000004 = [32-bit pointer] nested\r\n"
            L"++0x80000006 = data (8-bit)";
        const auto pNote = Parse(sNote);
        pNote->SetAddress(0x04);
        pNote->UpdateBaseAddress(0x04, mockEmulatorMemoryContext, nullptr);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, pNote->GetMemSize());

        bool bSeen = false;
        pNote->EnumerateOffsetNotes([&bSeen](const MemoryNoteModel::Reference& pNote)
            {
                Assert::AreEqual(0x14U, pNote.nAddress);
                Assert::AreEqual(0x80000004U, pNote.pMemoryNote->GetAddress());
                Assert::AreEqual(std::wstring(L"nested"), pNote.pMemoryNote->GetSummary());

                bSeen = true;
                return true;
            });

        Assert::IsTrue(bSeen);
    }

    TEST_METHOD(TestPointerNegative)
    {
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        mockConsoleContext.AddMemoryRegion(0, 0x7F, ra::data::MemoryRegion::Type::SystemRAM, 0x80000000);

        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        std::array<unsigned char, 256> memory{};
        memory.at(0x04) = 0x10; // pointer@0x04 = 0x00000010
        memory.at(0x08) = 0x20; // pointer@0x08 = 0x00000020
        mockEmulatorMemoryContext.MockMemory(memory);

        const std::wstring sNote =
            L"[32-bit pointer] root\r\n"
            L"+0xFFFFFFF8 = [32-bit pointer] nested\r\n"
            L"++0x80000006 = data (8-bit)";
        const auto pNote = Parse(sNote);
        pNote->SetAddress(0x04);
        pNote->UpdateBaseAddress(0x04, mockEmulatorMemoryContext, nullptr);

        Assert::AreEqual(Memory::Size::ThirtyTwoBit, pNote->GetMemSize());
        Assert::AreEqual(std::wstring(L"root"), pNote->GetSummary());

        bool bSeen = false;
        pNote->EnumerateOffsetNotes([&bSeen](const MemoryNoteModel::Reference& pNote)
            {
                Assert::AreEqual(0x08U, pNote.nAddress);
                Assert::AreEqual(0xFFFFFFF8U, pNote.pMemoryNote->GetAddress());
                Assert::AreEqual(std::wstring(L"nested"), pNote.pMemoryNote->GetSummary());

                bSeen = true;
                return true;
            });

        Assert::IsTrue(bSeen);
    }

    TEST_METHOD(TestUpdateRawPointerValue)
    {
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0 | Obj1 pointer\r\n"
            L"++0 | [16-bit] State\r\n"
            L"++2 | [16-bit] Visible\r\n"
            L"+4 | Obj2 pointer\r\n"
            L"++0 | [32-bit] ID\r\n"
            L"+8 | Count";
        auto pNote = Parse(sNote);
        pNote->SetAddress(4U);
        Assert::AreEqual(std::wstring(L"Pointer"), pNote->GetSummary());

        std::array<unsigned char, 32> memory{};
        mockEmulatorMemoryContext.MockMemory(memory);

        memory.at(4) = 8; // pointer = 8
        memory.at(8) = 20; // obj1 pointer = 20
        memory.at(12) = 28; // obj2 pointer = 28

        pNote->UpdateBaseAddress(4U, mockEmulatorMemoryContext, nullptr);
        Assert::AreEqual(8U, pNote->GetBaseAddress());

        const auto* pObj1Note = dynamic_cast<const PointerMemoryNoteModel*>(pNote->GetNoteAtOffset(0));
        Expects(pObj1Note != nullptr);
        Assert::IsNotNull(pObj1Note);
        Assert::AreEqual(20U, pObj1Note->GetBaseAddress());

        const auto* pObj2Note = dynamic_cast<const PointerMemoryNoteModel*>(pNote->GetNoteAtOffset(4));
        Expects(pObj2Note != nullptr);
        Assert::IsNotNull(pObj2Note);
        Assert::AreEqual(28U, pObj2Note->GetBaseAddress());
    }
};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
