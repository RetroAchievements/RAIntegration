#include "data/models/MemoryNotesModel.hh"

#include "services/impl/StringTextWriter.hh"

#include "tests/devkit/context/mocks/MockConsoleContext.hh"
#include "tests/devkit/context/mocks/MockEmulatorMemoryContext.hh"
#include "tests/devkit/context/mocks/MockRcClient.hh"
#include "tests/devkit/context/mocks/MockUserContext.hh"
#include "tests/devkit/testutil/AssetAsserts.hh"
#include "tests/devkit/testutil/MemoryAsserts.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

TEST_CLASS(MemoryNotesModel_Tests)
{
private:
    class MemoryNotesModelHarness : public MemoryNotesModel
    {
    public:
        ra::context::mocks::MockConsoleContext mockConsoleContext;
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        ra::context::mocks::MockRcClient mockRcClient;
        ra::context::mocks::MockUserContext mockUserContext;

        std::map<unsigned, std::wstring> mNewNotes;

        void InitializeNotes(unsigned nGameId)
        {
            mNewNotes.clear();

            if (!mockRcClient.HasMockResponse("r=codenotes2&g=1"))
                mockRcClient.MockResponse("r=codenotes2&g=1", "{\"Success\":true,\"CodeNotes\":[]}");

            MemoryNotesModel::Refresh(nGameId,
                [this](ra::data::ByteAddress nAddress, const std::wstring& sNewNote) {
                    mNewNotes[nAddress] = sNewNote;
                },
                [this](ra::data::ByteAddress nOldAddress, ra::data::ByteAddress nNewAddress, const std::wstring& sNote) {
                    if (nOldAddress != 0xFFFFFFFF)
                        mNewNotes[nOldAddress] = L"";
                    mNewNotes[nNewAddress] = sNote;
                },
                []() {});
        }

        void MonitorNoteChanges()
        {
            m_fMemoryNoteChanged = [this](ra::data::ByteAddress nAddress, const std::wstring& sNewNote) {
                mNewNotes[nAddress] = sNewNote;
            };
            m_fMemoryNoteMoved = [this](ra::data::ByteAddress nOldAddress, ra::data::ByteAddress nNewAddress, const std::wstring& sNote) {
                if (nOldAddress != 0xFFFFFFFF)
                    mNewNotes[nOldAddress] = L"";
                mNewNotes[nNewAddress] = sNote;
            };
        }

        using MemoryNotesModel::AddMemoryNote;

        void AssertNoNote(ra::data::ByteAddress nAddress)
        {
            const auto* pNote = FindNote(nAddress);
            if (pNote != nullptr)
                Assert::Fail(ra::util::String::Printf(L"Note found for address %04X: %s", nAddress, *pNote).c_str());
        }

        void AssertNote(ra::data::ByteAddress nAddress, const std::wstring& sExpected)
        {
            const auto* pNote = FindNote(nAddress);
            Assert::IsNotNull(pNote, ra::util::String::Printf(L"Note not found for address %04X", nAddress).c_str());
            Ensures(pNote != nullptr);

            Assert::AreEqual(sExpected, *pNote);
        }

        void AssertNote(ra::data::ByteAddress nAddress, const std::wstring& sExpected, Memory::Size nExpectedSize, unsigned nExpectedBytes = 0)
        {
            const auto* pNote = FindMemoryNoteModel(nAddress);
            Assert::IsNotNull(pNote, ra::util::String::Printf(L"Note not found for address %04X", nAddress).c_str());
            Ensures(pNote != nullptr);

            Assert::AreEqual(sExpected, pNote->GetNote());

            Assert::AreEqual(nExpectedSize, pNote->GetMemSize(),
                ra::util::String::Printf(L"Size for address %04X", nAddress).c_str());

            if (nExpectedBytes)
            {
                Assert::AreEqual(nExpectedBytes, pNote->GetBytes(),
                    ra::util::String::Printf(L"Bytes for address %04X", nAddress).c_str());
            }
        }

        void AssertIndirectNote(ra::data::ByteAddress nAddress, unsigned nOffset, const std::wstring& sExpected)
        {
            const auto* pNote = FindMemoryNoteModel(nAddress);
            Assert::IsNotNull(pNote, ra::util::String::Printf(L"Note not found for address %04X", nAddress).c_str());
            Ensures(pNote != nullptr);
            pNote = pNote->GetPointerNoteAtOffset(nOffset);
            Assert::IsNotNull(pNote, ra::util::String::Printf(L"Note not found for address %04X + %u", nAddress, nOffset).c_str());
            Ensures(pNote != nullptr);
            Assert::AreEqual(sExpected, pNote->GetNote());
        }

        void AssertNoteDescription(ra::data::ByteAddress nAddress, const wchar_t* sExpected)
        {
            const auto* pNote = FindMemoryNoteModel(nAddress);
            if (!sExpected)
            {
                if (pNote != nullptr)
                    Assert::IsNull(pNote, ra::util::String::Printf(L"Note found for address %04X: %s", nAddress, pNote->GetNote()).c_str());

                return;
            }

            Assert::IsNotNull(pNote, ra::util::String::Printf(L"Note not found for address %04X", nAddress).c_str());
            Ensures(pNote != nullptr);
            if (pNote->IsPointer())
                Assert::AreEqual(std::wstring(sExpected), pNote->GetPointerDescription());
            else
                Assert::AreEqual(std::wstring(sExpected), pNote->GetNote());
        }

        void AssertSerialize(const std::string& sExpected)
        {
            std::string sSerialized = "N0";
            ra::services::impl::StringTextWriter pTextWriter(sSerialized);
            Serialize(pTextWriter);

            Assert::AreEqual(sExpected, sSerialized);
        }

        struct MockNote
        {
            uint32_t nAddress;
            std::string sNote;
            std::string sAuthor;
        };

        static std::string MockNotesResponse(std::initializer_list<MockNote> vNotes)
        {
            std::string sResponse = "{\"Success\":true,\"CodeNotes\":[";

            std::string sAddress;
            sAddress.resize(8);

            for (MockNote pNote : vNotes) {
                snprintf(sAddress.data(), sAddress.capacity(), "0x%06x", pNote.nAddress);
                sResponse.append("{\"User\":\"");
                sResponse.append(pNote.sAuthor);
                sResponse.append("\",\"Address\":\"");
                sResponse.append(sAddress);
                sResponse.append("\",\"Note\":\"");
                for (const char c : pNote.sNote) {
                    switch (c)
                    {
                        case '\n':
                            sResponse.append("\\n");
                            break;

                        case '\r':
                            sResponse.append("\\r");
                            break;

                        case '"':
                            sResponse.append("\\\"");
                            break;

                        default:
                            sResponse.push_back(c);
                            break;
                    }
                }
                sResponse.append("\"},");
            }

            if (sResponse.back() == ',')
                sResponse.pop_back();

            sResponse.append("]}");
            return sResponse;
        }
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        MemoryNotesModelHarness notes;

        Assert::AreEqual(AssetType::MemoryNotes, notes.GetType());
        Assert::AreEqual(0U, notes.GetID());
        Assert::AreEqual(std::wstring(L"Memory Notes"), notes.GetName());
        Assert::AreEqual(std::wstring(L""), notes.GetDescription());
        Assert::AreEqual(AssetCategory::Core, notes.GetCategory());
        Assert::AreEqual(AssetState::Inactive, notes.GetState());
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());
    }

    
    TEST_METHOD(TestLoadMemoryNotes)
    {
        MemoryNotesModelHarness notes;
        notes.mockRcClient.MockResponse("r=codenotes2&g=1", MemoryNotesModelHarness::MockNotesResponse({
            { 1234, "Note1", "Author" },
            { 2345, "Note2", "Author" },
            { 3456, "Note3\nSubNote3", "Author" },
        }));

        notes.InitializeNotes(1U);
        Assert::AreEqual({3U}, notes.mNewNotes.size());

        notes.AssertNote(1234U, L"Note1");
        Assert::AreEqual(std::wstring(L"Note1"), notes.mNewNotes[1234U]);

        notes.AssertNote(2345U, L"Note2");
        Assert::AreEqual(std::wstring(L"Note2"), notes.mNewNotes[2345U]);

        // newlines should be normalized when loaded into the note.
        notes.AssertNote(3456U, L"Note3\r\nSubNote3");
        Assert::AreEqual(std::wstring(L"Note3\r\nSubNote3"), notes.mNewNotes[3456U]);

        const auto* pNote4 = notes.FindNote(4567U);
        Assert::IsNull(pNote4);

        notes.InitializeNotes(0U);
        const auto* pNote5 = notes.FindNote(1234U);
        Assert::IsNull(pNote5);
        Assert::AreEqual({0U}, notes.mNewNotes.size());
    }

    TEST_METHOD(TestFindNoteSized)
    {
        MemoryNotesModelHarness notes;
        notes.mockRcClient.MockResponse("r=codenotes2&g=1", MemoryNotesModelHarness::MockNotesResponse({
            { 1000, "[32-bit] Location", "Author" },
            { 1100, "Level", "Author" },
            { 1110, "[16-bit] Strength", "Author" },
            { 1120, "[8 byte] Exp", "Author" },
            { 1200, "[20 bytes] Items\r\nMultiline ignored", "Author" },
        }));

        notes.InitializeNotes(1U);

        Assert::AreEqual(std::wstring(L""), notes.FindNote(100, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L""), notes.FindNote(999, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [1/4]"), notes.FindNote(1000, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [2/4]"), notes.FindNote(1001, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [3/4]"), notes.FindNote(1002, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [4/4]"), notes.FindNote(1003, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L""), notes.FindNote(1004, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"Level"), notes.FindNote(1100, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [1/2]"), notes.FindNote(1110, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [2/2]"), notes.FindNote(1111, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [1/8]"), notes.FindNote(1120, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [2/8]"), notes.FindNote(1121, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [3/8]"), notes.FindNote(1122, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [4/8]"), notes.FindNote(1123, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [5/8]"), notes.FindNote(1124, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [6/8]"), notes.FindNote(1125, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [7/8]"), notes.FindNote(1126, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [8/8]"), notes.FindNote(1127, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L""), notes.FindNote(1128, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[20 bytes] Items [1/20]"), notes.FindNote(1200, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[20 bytes] Items [10/20]"), notes.FindNote(1209, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"[20 bytes] Items [20/20]"), notes.FindNote(1219, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L""), notes.FindNote(1300, Memory::Size::EightBit));

        Assert::AreEqual(std::wstring(L""), notes.FindNote(100, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L""), notes.FindNote(998, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindNote(999, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindNote(1000, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindNote(1001, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindNote(1002, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindNote(1003, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L""), notes.FindNote(1004, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Level [partial]"), notes.FindNote(1099, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Level [partial]"), notes.FindNote(1100, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), notes.FindNote(1109, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength"), notes.FindNote(1110, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), notes.FindNote(1111, Memory::Size::SixteenBit));

        Assert::AreEqual(std::wstring(L""), notes.FindNote(100, Memory::Size::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L""), notes.FindNote(996, Memory::Size::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindNote(997, Memory::Size::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location"), notes.FindNote(1000, Memory::Size::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindNote(1001, Memory::Size::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindNote(1002, Memory::Size::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindNote(1003, Memory::Size::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L""), notes.FindNote(1004, Memory::Size::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"Level [partial]"), notes.FindNote(1097, Memory::Size::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"Level [partial]"), notes.FindNote(1100, Memory::Size::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), notes.FindNote(1107, Memory::Size::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), notes.FindNote(1110, Memory::Size::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), notes.FindNote(1111, Memory::Size::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L""), notes.FindNote(1112, Memory::Size::ThirtyTwoBit));
    }

    TEST_METHOD(TestFindNoteStart)
    {
        MemoryNotesModelHarness notes;
        notes.mockRcClient.MockResponse("r=codenotes2&g=1", MemoryNotesModelHarness::MockNotesResponse({
            { 1000, "[32-bit] Location", "Author" },
            { 1100, "Level", "Author" },
            { 1110, "[16-bit] Strength", "Author" },
            { 1120, "[8 byte] Exp", "Author" },
            { 1200, "[20 bytes] Items", "Author" },
        }));

        notes.InitializeNotes(1U);

        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(100));

        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(999));
        Assert::AreEqual(1000U, notes.FindNoteStart(1000));
        Assert::AreEqual(1000U, notes.FindNoteStart(1001));
        Assert::AreEqual(1000U, notes.FindNoteStart(1002));
        Assert::AreEqual(1000U, notes.FindNoteStart(1003));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(1004));

        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(1099));
        Assert::AreEqual(1100U, notes.FindNoteStart(1100));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(1101));

        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(1109));
        Assert::AreEqual(1110U, notes.FindNoteStart(1110));
        Assert::AreEqual(1110U, notes.FindNoteStart(1111));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(1112));

        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(1119));
        Assert::AreEqual(1120U, notes.FindNoteStart(1120));
        Assert::AreEqual(1120U, notes.FindNoteStart(1127));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(1128));

        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(1199));
        Assert::AreEqual(1200U, notes.FindNoteStart(1200));
        Assert::AreEqual(1200U, notes.FindNoteStart(1219));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(1220));
    }

    TEST_METHOD(TestFindNoteStartOverlap)
    {
        MemoryNotesModelHarness notes;
        notes.mockRcClient.MockResponse("r=codenotes2&g=1", MemoryNotesModelHarness::MockNotesResponse({
            { 1000, "[100 bytes] Outer", "Author" },
            { 1010, "[10 bytes] Inner", "Author" },
            { 1015, "Individual", "Author" },
            { 1120, "[10 bytes] Secondary", "Author" },
            { 1125, "[10 bytes] Overlap", "Author" },
            { 1200, "Extra", "Author" },
        }));

        notes.InitializeNotes(1U);

        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(999));
        Assert::AreEqual(1000U, notes.FindNoteStart(1000));
        Assert::AreEqual(1000U, notes.FindNoteStart(1009));
        Assert::AreEqual(1010U, notes.FindNoteStart(1010));
        Assert::AreEqual(1010U, notes.FindNoteStart(1014));
        Assert::AreEqual(1015U, notes.FindNoteStart(1015));
        Assert::AreEqual(1010U, notes.FindNoteStart(1016));
        Assert::AreEqual(1010U, notes.FindNoteStart(1019));
        Assert::AreEqual(1000U, notes.FindNoteStart(1020));
        Assert::AreEqual(1000U, notes.FindNoteStart(1099));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(1100));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(1119));
        Assert::AreEqual(1120U, notes.FindNoteStart(1120));
        Assert::AreEqual(1120U, notes.FindNoteStart(1124));
        Assert::AreEqual(1125U, notes.FindNoteStart(1125));
        Assert::AreEqual(1125U, notes.FindNoteStart(1126));
        Assert::AreEqual(1125U, notes.FindNoteStart(1130));
        Assert::AreEqual(1125U, notes.FindNoteStart(1134));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(1135));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(1199));
        Assert::AreEqual(1200U, notes.FindNoteStart(1200));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(1201));
    }

    TEST_METHOD(TestSetNote)
    {
        MemoryNotesModelHarness notes;
        notes.InitializeNotes(1U);
        notes.mNewNotes.clear();
        Assert::AreEqual({ 0U }, notes.NoteCount());

        notes.SetNote(1234, L"Note1");

        notes.AssertNote(1234U, L"Note1");
        Assert::AreEqual(std::wstring(L"Note1"), notes.mNewNotes[1234U]);
        Assert::AreEqual({ 1U }, notes.NoteCount());

        notes.SetNote(1234, L"");
        notes.AssertNoNote(1234U);
        Assert::AreEqual(std::wstring(L""), notes.mNewNotes[1234U]);
        Assert::AreEqual({ 0U }, notes.NoteCount());
    }

    TEST_METHOD(TestFindCodeNotePointer1)
    {
        MemoryNotesModelHarness notes;
        const std::wstring sNote =
            L"Bomb Timer Pointer (24-bit)\n"
            L"+03 - Bombs Defused\n"
            L"+04 - Bomb Timer";
        notes.AddMemoryNote(12, "Author", sNote);
        std::array<unsigned char, 32> memory{};
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(12) = 0x04;
        notes.DoFrame();

        notes.AssertNote(12U, sNote, Memory::Size::TwentyFourBit); // full note for pointer address

        // extracted notes for offset fields (note: pointer base default is $0000)
        notes.AssertNote(4+3U, L"Bombs Defused", Memory::Size::Unknown, 1);
        notes.AssertNote(4+4U, L"Bomb Timer", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestFindNotePointer2)
    {
        MemoryNotesModelHarness notes;
        const std::wstring sNote =
            L"Pointer [32bit]\n"
            L"+0x1B | Equipment - Head - String[24 Bytes]\n"
            L"---DEFAULT_HEAD = Barry's Head\n"
            L"---FRAGGER_HEAD = Fragger Helmet";
        notes.AddMemoryNote(12, "Author", sNote);
        std::array<unsigned char, 64> memory{};
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(12) = 0x04;
        notes.DoFrame();

        notes.AssertNote(12U, sNote, Memory::Size::ThirtyTwoBit); // full note for pointer address

        notes.AssertNote(4+0x1BU, L"Equipment - Head - String[24 Bytes]\n"
            L"---DEFAULT_HEAD = Barry's Head\n"
            L"---FRAGGER_HEAD = Fragger Helmet", Memory::Size::Array, 24);
    }

    TEST_METHOD(TestFindNotePointer3)
    {
        MemoryNotesModelHarness notes;
        const std::wstring sNote =
            L"[32-bit] Pointer for Races (only exists during Race Game)\n"
            L"+0x7F47 = [8-bit] Acorns collected in current track\n"
            L"+0x8000 = [8-bit] Current lap\n"
            L"+0x8033 = [16-bit] Total race time";
        notes.AddMemoryNote(12, "Author", sNote);
        std::array<unsigned char, 32> memory{};
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(12) = 0x04;
        notes.DoFrame();

        notes.AssertNote(12U, sNote, Memory::Size::ThirtyTwoBit); // full note for pointer address

        notes.AssertNote(4+0x7F47U, L"[8-bit] Acorns collected in current track", Memory::Size::EightBit);
        notes.AssertNote(4+0x8000U, L"[8-bit] Current lap", Memory::Size::EightBit);
        notes.AssertNote(4+0x8033U, L"[16-bit] Total race time", Memory::Size::SixteenBit);
    }

    TEST_METHOD(TestFindNotePointer4)
    {
        MemoryNotesModelHarness notes;
        const std::wstring sNote =
            L"Pointer (16bit because negative)\n\n"
            L"Circuit:\n"
            L"+0x1B56E = Current Position\n"
            L"+0x1B57E = Total Racers\n\n"
            L"Free Run:\n"
            L"+0x1B5BE = Seconds 0x\n"
            L"+0x1B5CE = Lap";
        notes.AddMemoryNote(12, "Author", sNote);
        std::array<unsigned char, 32> memory{};
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(12) = 0x04;
        notes.DoFrame();

        notes.AssertNote(12U, sNote, Memory::Size::SixteenBit); // full note for pointer address

        notes.AssertNote(4+0x1B56EU, L"Current Position", Memory::Size::Unknown, 1);
        notes.AssertNote(4+0x1B57EU, L"Total Racers\n\nFree Run:", Memory::Size::Unknown, 1);
        notes.AssertNote(4+0x1B5BEU, L"Seconds 0x", Memory::Size::Unknown, 1);
        notes.AssertNote(4+0x1B5CEU, L"Lap", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestFindNotePointer5)
    {
        MemoryNotesModelHarness notes;
        const std::wstring sNote =
            L"Pointer\r\n"
            L"[OFFSETS]\r\n"
            L"+2 = EXP (32-bit)\r\n"
            L"+5 = Base Level (8-bit)\r\n"
            L"+6 = Job Level (8-bit)\r\n"
            L"+20 = Stat Points (16-bit)\r\n"
            L"+22 = Skill Points (8-bit)";
        notes.AddMemoryNote(12, "Author", sNote);
        std::array<unsigned char, 32> memory{};
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(12) = 0x04;
        notes.DoFrame();

        notes.AssertNote(12U, sNote, Memory::Size::ThirtyTwoBit, 4); // full note for pointer address (assume 32-bit if not specified)

        // extracted notes for offset fields (note: pointer base default is $0000)
        notes.AssertNote(4+2, L"EXP (32-bit)", Memory::Size::ThirtyTwoBit);
        notes.AssertNote(4+5, L"Base Level (8-bit)", Memory::Size::EightBit);
        notes.AssertNote(4+6, L"Job Level (8-bit)", Memory::Size::EightBit);
        notes.AssertNote(4+20, L"Stat Points (16-bit)", Memory::Size::SixteenBit);
        notes.AssertNote(4+22, L"Skill Points (8-bit)", Memory::Size::EightBit);
    }

    TEST_METHOD(TestFindNotePointerNested)
    {
        MemoryNotesModelHarness notes;
        const std::wstring sNote =
            L"Pointer [32bit]\n"
            L"+4 | Pointer - Award - Tee Hee Two (32bit)\n"
            L"-- +2 | Flag\n"
            L"+8 | Pointer - Award - Pretty Woman (32bit)\n"
            L"-- +2 | Flag";
        notes.AddMemoryNote(12, "Author", sNote);
        std::array<unsigned char, 32> memory{};
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(12) = 0x10;
        memory.at(0x10+4) = 0x20;
        memory.at(0x10+8) = 0x28;
        notes.DoFrame();

        notes.AssertNote(12U, sNote, Memory::Size::ThirtyTwoBit); // full note for pointer address

        notes.AssertNote(0x10+4U, L"Pointer - Award - Tee Hee Two (32bit)\n+2 | Flag", Memory::Size::ThirtyTwoBit);
        notes.AssertNote(0x10+8U, L"Pointer - Award - Pretty Woman (32bit)\n+2 | Flag", Memory::Size::ThirtyTwoBit);

        notes.AssertNote(0x20+2U, L"Flag", Memory::Size::Unknown);
        notes.AssertNote(0x28+2U, L"Flag", Memory::Size::Unknown);
    }

    TEST_METHOD(TestFindNotePointerNonPrefixedOffsets)
    {
        MemoryNotesModelHarness notes;
        const std::wstring sNote =
            L"[24-bit] Mech stats pointer (health...)\n"
            L"+07 = Jump Jet {8-bit}\n"
            L"+5C = Right Leg Health {16-bit}\n"
            L"+5E = Left Leg Health {16-bit}";
        notes.AddMemoryNote(12, "Author", sNote);
        std::array<unsigned char, 32> memory{};
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(12) = 4;
        notes.DoFrame();

        // offset parse failure results in a non-pointer note
        notes.AssertNote(12U, sNote, Memory::Size::TwentyFourBit);
        notes.AssertNoNote(4+0x07U);
        notes.AssertNoNote(4+0x5CU);
        notes.AssertNoNote(4+0x5EU);
    }

    TEST_METHOD(TestFindNotePointerOverwrite)
    {
        MemoryNotesModelHarness notes;
        const std::wstring sNote =
            L"Pointer (16bit because negative)\n"
            L"+0x1B56E = Current Position\n"
            L"+0x1B57E = Total Racers\n"
            L"+0x1B56E = Seconds 0x\n"
            L"+0x1B5CE = Lap";
        notes.AddMemoryNote(12, "Author", sNote);
        std::array<unsigned char, 32> memory{};
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(12) = 4;
        notes.DoFrame();

        notes.AssertNote(12U, sNote, Memory::Size::SixteenBit); // full note for pointer address

        // both note for 0x1B56E exist in the array, but only the first is returned
        notes.AssertNote(4+0x1B56EU, L"Current Position", Memory::Size::Unknown, 1);
        notes.AssertNote(4+0x1B57EU, L"Total Racers", Memory::Size::Unknown, 1);
        notes.AssertNote(4+0x1B5CEU, L"Lap", Memory::Size::Unknown, 1);
    }

    TEST_METHOD(TestFindNoteSizedPointer)
    {
        MemoryNotesModelHarness notes;
        const std::wstring sNote =
            L"Pointer\n"
            L"+1 = Unknown\n"
            L"+2 = Small (8-bit)\n"
            L"+4 = Medium (16-bit)\n"
            L"+6 = Large (32-bit)\n"
            L"+10 = Very Large (8 bytes)";
        notes.AddMemoryNote(12, "Author", sNote);
        std::array<unsigned char, 32> memory{};
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(12) = 0x10;
        notes.DoFrame();

        Assert::AreEqual(std::wstring(), notes.FindNote(0, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"Unknown [indirect]"), notes.FindNote(0x10+1, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"Small (8-bit) [indirect]"), notes.FindNote(0x10+2, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"Medium (16-bit) [1/2] [indirect]"), notes.FindNote(0x10+4, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"Medium (16-bit) [2/2] [indirect]"), notes.FindNote(0x10+5, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"Large (32-bit) [1/4] [indirect]"), notes.FindNote(0x10+6, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"Large (32-bit) [4/4] [indirect]"), notes.FindNote(0x10+9, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"Very Large (8 bytes) [1/8] [indirect]"), notes.FindNote(0x10+10, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"Very Large (8 bytes) [8/8] [indirect]"), notes.FindNote(0x10+17, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(), notes.FindNote(0x10+18, Memory::Size::EightBit));

        Assert::AreEqual(std::wstring(L"Unknown [partial] [indirect]"), notes.FindNote(0x10+0, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Unknown [partial] [indirect]"), notes.FindNote(0x10+1, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Small (8-bit) [partial] [indirect]"), notes.FindNote(0x10+2, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Medium (16-bit) [indirect]"), notes.FindNote(0x10+4, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Medium (16-bit) [partial] [indirect]"), notes.FindNote(0x10+5, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Large (32-bit) [partial] [indirect]"), notes.FindNote(0x10+6, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Large (32-bit) [partial] [indirect]"), notes.FindNote(0x10+9, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Very Large (8 bytes) [partial] [indirect]"), notes.FindNote(0x10+10, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Very Large (8 bytes) [partial] [indirect]"), notes.FindNote(0x10+17, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(), notes.FindNote(0x10+18, Memory::Size::SixteenBit));
    }

    
    TEST_METHOD(TestFindNoteSizedPointerOverflow)
    {
        MemoryNotesModelHarness notes;
        notes.mockConsoleContext.AddMemoryRegion(0, 31, ra::data::MemoryRegion::Type::SystemRAM, 0x80);
        std::array<unsigned char, 32> memory{};
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(4) = 0x88; // start with initial value for pointer (real address = 0x88, RA address = 0x08)

        const std::wstring sPointerNote =
            L"Pointer (32-bit)\n"              // only 32-bit pointers are eligible for real address conversion
            L"+0xFFFFFF88 = Small (8-bit)\n"   // 8+8=16
            L"+0xFFFFFF90 = Medium (16-bit)\n" // 16+8=24
            L"+0xFFFFFF98 = Large (32-bit)";   // 24+8=32
        notes.AddMemoryNote(4, "Author", sPointerNote);
        notes.AddMemoryNote(40, "Author", L"After [32-bit]");
        notes.AddMemoryNote(1, "Author", L"Before");
        notes.AddMemoryNote(20, "Author", L"In the middle");
        notes.DoFrame();

        Assert::AreEqual(std::wstring(), notes.FindNote(0, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"Before"), notes.FindNote(1, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"Pointer (32-bit) [1/4]"), notes.FindNote(4, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"Small (8-bit) [indirect]"), notes.FindNote(16, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"Medium (16-bit) [1/2] [indirect]"), notes.FindNote(24, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"Medium (16-bit) [2/2] [indirect]"), notes.FindNote(25, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"Large (32-bit) [1/4] [indirect]"), notes.FindNote(32, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(L"Large (32-bit) [4/4] [indirect]"), notes.FindNote(35, Memory::Size::EightBit));
        Assert::AreEqual(std::wstring(), notes.FindNote(36, Memory::Size::EightBit));

        Assert::AreEqual(std::wstring(L"Before [partial]"), notes.FindNote(0, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Before [partial]"), notes.FindNote(1, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Pointer (32-bit) [partial]"), notes.FindNote(4, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Small (8-bit) [partial] [indirect]"), notes.FindNote(16, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Medium (16-bit) [indirect]"), notes.FindNote(24, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Medium (16-bit) [partial] [indirect]"), notes.FindNote(25, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Large (32-bit) [partial] [indirect]"), notes.FindNote(32, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(L"Large (32-bit) [partial] [indirect]"), notes.FindNote(35, Memory::Size::SixteenBit));
        Assert::AreEqual(std::wstring(), notes.FindNote(36, Memory::Size::SixteenBit));
    }

    TEST_METHOD(TestFindNoteBrokenPointerChain)
    {
        MemoryNotesModelHarness notes;
        const std::wstring sNote =
            L"Pointer [32bit]\r\n"
            L"+0 | Obj1 pointer\r\n"
            L"++0 | [16-bit] State\r\n"
            L"++2 | [16-bit] Visible\r\n"
            L"+4 | Obj2 pointer\r\n"
            L"++0 | [32-bit] ID\r\n"
            L"+8 | Count";
        notes.AddMemoryNote(4U, "Author", sNote);

        notes.mockConsoleContext.AddMemoryRegion(0, 31, ra::data::MemoryRegion::Type::SystemRAM, 0x80);
        std::array<unsigned char, 32> memory{};
        memory.at(4) = 8; // pointer = 8
        memory.at(8) = 20; // obj1 pointer = 20
        memory.at(12) = 28; // obj2 pointer = 28
        notes.mockEmulatorMemoryContext.MockMemory(memory);

        notes.DoFrame();

        // expected locations
        notes.AssertNoteDescription(4U, L"Pointer [32bit]");
        notes.AssertNoteDescription(8U, L"Obj1 pointer");
        notes.AssertNoteDescription(12U, L"Obj2 pointer");
        notes.AssertNoteDescription(16U, L"Count");
        notes.AssertNoteDescription(20U, L"[16-bit] State");
        notes.AssertNoteDescription(22U, L"[16-bit] Visible");
        notes.AssertNoteDescription(28U, L"[32-bit] ID");

        // unexpected locations
        notes.AssertNoteDescription(0U, nullptr);
        notes.AssertNoteDescription(2U, nullptr);

        // make pointer2 null
        memory.at(12) = 0;
        notes.DoFrame();
        notes.AssertNoteDescription(4U, L"Pointer [32bit]");
        notes.AssertNoteDescription(8U, L"Obj1 pointer");
        notes.AssertNoteDescription(12U, L"Obj2 pointer");
        notes.AssertNoteDescription(16U, L"Count");
        notes.AssertNoteDescription(20U, L"[16-bit] State");
        notes.AssertNoteDescription(22U, L"[16-bit] Visible");
        notes.AssertNoteDescription(28U, nullptr);
        notes.AssertNoteDescription(0U, nullptr);
        notes.AssertNoteDescription(2U, nullptr);

        // make root pointer null
        memory.at(4) = 0;
        notes.DoFrame();
        notes.AssertNoteDescription(4U, L"Pointer [32bit]");
        notes.AssertNoteDescription(8U, nullptr);
        notes.AssertNoteDescription(12U, nullptr);
        notes.AssertNoteDescription(16U, nullptr);
        notes.AssertNoteDescription(20U, nullptr);
        notes.AssertNoteDescription(22U, nullptr);
        notes.AssertNoteDescription(28U, nullptr);
        notes.AssertNoteDescription(0U, nullptr);
        notes.AssertNoteDescription(2U, nullptr);

        // set root pointer
        memory.at(4) = 12;
        notes.DoFrame();
        notes.AssertNoteDescription(4U, L"Pointer [32bit]");
        notes.AssertNoteDescription(8U, nullptr);
        notes.AssertNoteDescription(12U, L"Obj1 pointer");
        notes.AssertNoteDescription(16U, L"Obj2 pointer");
        notes.AssertNoteDescription(20U, L"Count");
        notes.AssertNoteDescription(28U, nullptr);
        notes.AssertNoteDescription(0U, nullptr);
        notes.AssertNoteDescription(2U, nullptr);

        // make pointer1 not null
        memory.at(12) = 28;
        notes.DoFrame();
        notes.AssertNoteDescription(4U, L"Pointer [32bit]");
        notes.AssertNoteDescription(8U, nullptr);
        notes.AssertNoteDescription(12U, L"Obj1 pointer");
        notes.AssertNoteDescription(16U, L"Obj2 pointer");
        notes.AssertNoteDescription(20U, L"Count");
        notes.AssertNoteDescription(28U, L"[16-bit] State");
        notes.AssertNoteDescription(30U, L"[16-bit] Visible");
        notes.AssertNoteDescription(0U, nullptr);
        notes.AssertNoteDescription(2U, nullptr);
    }

    TEST_METHOD(TestEnumerateMemoryNotes)
    {
        MemoryNotesModelHarness notes;
        const std::wstring sPointerNote =
            L"Pointer\n"
            L"+8 = Unknown\n"
            L"+16 = Small (8-bit)";
        notes.AddMemoryNote(1234, "Author", sPointerNote);
        notes.AddMemoryNote(20, "Author", L"After [32-bit]");
        notes.AddMemoryNote(4, "Author", L"Before");
        notes.AddMemoryNote(12, "Author", L"In the middle");
        notes.DoFrame();

        int i = 0;
        notes.EnumerateMemoryNotes([&i, &sPointerNote](ra::data::ByteAddress nAddress, const MemoryNoteModel& pMemoryNote) {
            const auto nBytes = pMemoryNote.GetBytes();
            const auto& sNote = pMemoryNote.GetNote();

            switch (i++)
            {
                case 0:
                    Assert::AreEqual({4U}, nAddress);
                    Assert::AreEqual(1U, nBytes);
                    Assert::AreEqual(std::wstring(L"Before"), sNote);
                    break;
                case 1:
                    Assert::AreEqual({12U}, nAddress);
                    Assert::AreEqual(1U, nBytes);
                    Assert::AreEqual(std::wstring(L"In the middle"), sNote);
                    break;
                case 2:
                    Assert::AreEqual({20U}, nAddress);
                    Assert::AreEqual(4U, nBytes);
                    Assert::AreEqual(std::wstring(L"After [32-bit]"), sNote);
                    break;
                case 3:
                    Assert::AreEqual({1234U}, nAddress);
                    Assert::AreEqual(4U, nBytes); // unspecified size on pointer assumed to be 32-bit
                    Assert::AreEqual(sPointerNote, sNote);
                    break;
                case 4:
                    Assert::Fail(L"Too many notes");
                    break;
            }
            return true;
        }, false);
    }

    TEST_METHOD(TestEnumerateMemoryNotesWithIndirect)
    {
        MemoryNotesModelHarness notes;
        std::array<unsigned char, 128> memory{};
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(120) = 0x02; // start with initial value for pointer

        const std::wstring sPointerNote =
            L"Pointer\n"
            L"+8 = Unknown\n"
            L"+16 = Small (16-bit)";
        notes.AddMemoryNote(120, "Author", sPointerNote);
        notes.AddMemoryNote(20, "Author", L"After [32-bit]");
        notes.AddMemoryNote(4, "Author", L"Before");
        notes.AddMemoryNote(12, "Author", L"In the middle");
        notes.DoFrame();

        int i = 0;
        notes.EnumerateMemoryNotes([&i, &sPointerNote](ra::data::ByteAddress nAddress, const MemoryNoteModel& pMemoryNote) {
            const auto nBytes = pMemoryNote.GetBytes();
            const auto& sNote = pMemoryNote.GetNote();

            switch (i++)
            {
                case 0:
                    Assert::AreEqual({4U}, nAddress);
                    Assert::AreEqual(1U, nBytes);
                    Assert::AreEqual(std::wstring(L"Before"), sNote);
                    break;
                case 1:
                    Assert::AreEqual({10U}, nAddress);
                    Assert::AreEqual(1U, nBytes);
                    Assert::AreEqual(std::wstring(L"Unknown"), sNote);
                    break;
                case 2:
                    Assert::AreEqual({12U}, nAddress);
                    Assert::AreEqual(1U, nBytes);
                    Assert::AreEqual(std::wstring(L"In the middle"), sNote);
                    break;
                case 3:
                    Assert::AreEqual({18U}, nAddress);
                    Assert::AreEqual(2U, nBytes);
                    Assert::AreEqual(std::wstring(L"Small (16-bit)"), sNote);
                    break;
                case 4:
                    Assert::AreEqual({20U}, nAddress);
                    Assert::AreEqual(4U, nBytes);
                    Assert::AreEqual(std::wstring(L"After [32-bit]"), sNote);
                    break;
                case 5:
                    Assert::AreEqual({120U}, nAddress);
                    Assert::AreEqual(4U, nBytes); // unspecified size on pointer assumed to be 32-bit
                    Assert::AreEqual(sPointerNote, sNote);
                    break;
                case 6:
                    Assert::Fail(L"Too many notes");
                    break;
            }
            return true;
        }, true);
    }
    
    TEST_METHOD(TestEnumerateMemoryNotesWithIndirectOverflow)
    {
        MemoryNotesModelHarness notes;
        notes.mockConsoleContext.AddMemoryRegion(0, 31, ra::data::MemoryRegion::Type::SystemRAM, 0x80);
        std::array<unsigned char, 32> memory{};
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(4) = 0x88; // start with initial value for pointer (real address = 0x88, RA address = 0x08)

        const std::wstring sPointerNote =
            L"Pointer (32-bit)\n"              // only 32-bit pointers are eligible for real address conversion
            L"+0xFFFFFF88 = Small (8-bit)\n"   // 8+8=16
            L"+0xFFFFFF90 = Medium (16-bit)\n" // 16+8=24
            L"+0xFFFFFF98 = Large (32-bit)";   // 24+8=32
        notes.AddMemoryNote(4, "Author", sPointerNote);
        notes.AddMemoryNote(40, "Author", L"After [32-bit]");
        notes.AddMemoryNote(1, "Author", L"Before");
        notes.AddMemoryNote(20, "Author", L"In the middle");
        notes.DoFrame();

        int i = 0;
        notes.EnumerateMemoryNotes([&i, &sPointerNote](ra::data::ByteAddress nAddress, const MemoryNoteModel& pMemoryNote) {
            const auto nBytes = pMemoryNote.GetBytes();
            const auto& sNote = pMemoryNote.GetNote();

            switch (i++)
            {
                case 0:
                    Assert::AreEqual({1U}, nAddress);
                    Assert::AreEqual(1U, nBytes);
                    Assert::AreEqual(std::wstring(L"Before"), sNote);
                    break;
                case 1:
                    Assert::AreEqual({4U}, nAddress);
                    Assert::AreEqual(4U, nBytes);
                    Assert::AreEqual(sPointerNote, sNote);
                    break;
                case 2:
                    Assert::AreEqual({16U}, nAddress);
                    Assert::AreEqual(1U, nBytes);
                    Assert::AreEqual(std::wstring(L"Small (8-bit)"), sNote);
                    break;
                case 3:
                    Assert::AreEqual({20U}, nAddress);
                    Assert::AreEqual(1U, nBytes);
                    Assert::AreEqual(std::wstring(L"In the middle"), sNote);
                    break;
                case 4:
                    Assert::AreEqual({24U}, nAddress);
                    Assert::AreEqual(2U, nBytes);
                    Assert::AreEqual(std::wstring(L"Medium (16-bit)"), sNote);
                    break;
                case 5:
                    Assert::AreEqual({32}, nAddress);
                    Assert::AreEqual(4U, nBytes);
                    Assert::AreEqual(std::wstring(L"Large (32-bit)"), sNote);
                    break;
                case 6:
                    Assert::AreEqual({40}, nAddress);
                    Assert::AreEqual(4U, nBytes);
                    Assert::AreEqual(std::wstring(L"After [32-bit]"), sNote);
                    break;
                case 7:
                    Assert::Fail(L"Too many notes");
                    break;
            }
            return true;
        }, true);
    }

    TEST_METHOD(TestDoFrame)
    {
        MemoryNotesModelHarness notes;
        notes.MonitorNoteChanges();

        std::array<unsigned char, 32> memory{};
        for (uint8_t i = 0; i < memory.size(); i++)
            memory.at(i) = i;
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(0) = 16; // start with initial value for pointer

        const std::wstring sNote =
            L"Pointer (8-bit)\n"
            L"+1 = Small (8-bit)\n"
            L"+2 = Medium (16-bit)\n"
            L"+4 = Large (32-bit)";
        notes.AddMemoryNote(0x0000, "Author", sNote);
        notes.DoFrame();

        // should receive notifications for the pointer note, and for each subnote
        Assert::AreEqual({4U}, notes.mNewNotes.size());
        Assert::AreEqual(sNote, notes.mNewNotes[0x00]);
        Assert::AreEqual(std::wstring(L"Small (8-bit)"), notes.mNewNotes[0x11]);
        Assert::AreEqual(std::wstring(L"Medium (16-bit)"), notes.mNewNotes[0x12]);
        Assert::AreEqual(std::wstring(L"Large (32-bit)"), notes.mNewNotes[0x14]);

        notes.AssertNoNote(0x02U);
        notes.AssertNote(0x12U, L"Medium (16-bit)", Memory::Size::SixteenBit, 2);

        // calling DoFrame after updating the pointer should nofity about all the affected subnotes
        notes.mNewNotes.clear();
        memory.at(0) = 8;
        notes.DoFrame();

        Assert::AreEqual({6U}, notes.mNewNotes.size());
        Assert::AreEqual(std::wstring(L""), notes.mNewNotes[0x11]);
        Assert::AreEqual(std::wstring(L""), notes.mNewNotes[0x12]);
        Assert::AreEqual(std::wstring(L""), notes.mNewNotes[0x14]);
        Assert::AreEqual(std::wstring(L"Small (8-bit)"), notes.mNewNotes[0x09]);
        Assert::AreEqual(std::wstring(L"Medium (16-bit)"), notes.mNewNotes[0x0A]);
        Assert::AreEqual(std::wstring(L"Large (32-bit)"), notes.mNewNotes[0x0C]);

        notes.AssertNoNote(0x02U);
        notes.AssertNoNote(0x12U);
        notes.AssertNote(0x0AU, L"Medium (16-bit)", Memory::Size::SixteenBit, 2);

        // small change to pointer causes notification addresses to overlap. Make sure the final event
        // for the overlapping address is the new note value.
        notes.mNewNotes.clear();
        memory.at(0) = 6;
        notes.DoFrame();

        // Note on 0x0A changes from "Medium (16-bit)" to "Large (32-bit)". The change event is actually
        // raised twice - the first to clear it out, and the second with the updated value. All we care
        // about is that the updated value is seen later (will be the final value captured in mNewNotes)
        Assert::AreEqual({5U}, notes.mNewNotes.size());
        Assert::AreEqual(std::wstring(L""), notes.mNewNotes[0x09]);
        Assert::AreEqual(std::wstring(L""), notes.mNewNotes[0x0C]);
        Assert::AreEqual(std::wstring(L"Small (8-bit)"), notes.mNewNotes[0x07]);
        Assert::AreEqual(std::wstring(L"Medium (16-bit)"), notes.mNewNotes[0x08]);
        Assert::AreEqual(std::wstring(L"Large (32-bit)"), notes.mNewNotes[0x0A]);

        notes.AssertNoNote(0x02U);
        notes.AssertNoNote(0x12U);
        notes.AssertNote(0x0AU, L"Large (32-bit)", Memory::Size::ThirtyTwoBit, 4);

        // no change to pointer should not raise any events
        notes.mNewNotes.clear();
        notes.DoFrame();
        Assert::AreEqual({0U}, notes.mNewNotes.size());
    }

    TEST_METHOD(TestDoFrameRealAddressConversion)
    {
        MemoryNotesModelHarness notes;
        notes.MonitorNoteChanges();
        notes.mockConsoleContext.AddMemoryRegion(0, 31, ra::data::MemoryRegion::Type::SystemRAM, 0x80);

        std::array<unsigned char, 32> memory{};
        for (uint8_t i = 4; i < memory.size(); i++)
            memory.at(i) = i;
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(0) = 0x90; // start with initial value for pointer (real address = 0x90, RA address = 0x10)

        const std::wstring sNote =
            L"Pointer (32-bit)\n" // only 32-bit pointers are eligible for real address conversion
            L"+1 = Small (8-bit)\n"
            L"+2 = Medium (16-bit)\n"
            L"+4 = Large (32-bit)";
        notes.AddMemoryNote(0x0000, "Author", sNote);
        notes.DoFrame();

        // should receive notifications for the pointer note, and for each subnote
        Assert::AreEqual({4U}, notes.mNewNotes.size());
        Assert::AreEqual(sNote, notes.mNewNotes[0x00]);
        Assert::AreEqual(std::wstring(L"Small (8-bit)"), notes.mNewNotes[0x11]);
        Assert::AreEqual(std::wstring(L"Medium (16-bit)"), notes.mNewNotes[0x12]);
        Assert::AreEqual(std::wstring(L"Large (32-bit)"), notes.mNewNotes[0x14]);

        notes.AssertNoNote(0x02U);
        notes.AssertNote(0x12U, L"Medium (16-bit)", Memory::Size::SixteenBit, 2);

        // calling DoFrame after updating the pointer should notify about all the affected subnotes
        notes.mNewNotes.clear();
        memory.at(0) = 0x88;
        notes.DoFrame();

        Assert::AreEqual({6U}, notes.mNewNotes.size());
        Assert::AreEqual(std::wstring(L""), notes.mNewNotes[0x11]);
        Assert::AreEqual(std::wstring(L""), notes.mNewNotes[0x12]);
        Assert::AreEqual(std::wstring(L""), notes.mNewNotes[0x14]);
        Assert::AreEqual(std::wstring(L"Small (8-bit)"), notes.mNewNotes[0x09]);
        Assert::AreEqual(std::wstring(L"Medium (16-bit)"), notes.mNewNotes[0x0A]);
        Assert::AreEqual(std::wstring(L"Large (32-bit)"), notes.mNewNotes[0x0C]);

        notes.AssertNoNote(0x02U);
        notes.AssertNoNote(0x12U);
        notes.AssertNote(0x0AU, L"Medium (16-bit)", Memory::Size::SixteenBit, 2);
    }

    TEST_METHOD(TestDoFrameRealAddressConversionBigEndian)
    {
        MemoryNotesModelHarness notes;
        notes.MonitorNoteChanges();
        notes.mockConsoleContext.AddMemoryRegion(0, 31, ra::data::MemoryRegion::Type::SystemRAM, 0x80);

        std::array<unsigned char, 32> memory{};
        for (uint8_t i = 4; i < memory.size(); i++)
            memory.at(i) = i;
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(3) = 0x90; // start with initial value for pointer (real address = 0x90, RA address = 0x10)

        const std::wstring sNote =
            L"Pointer (32-bit BE)\n" // only 32-bit pointers are eligible for real address conversion
            L"+1 = Small (8-bit)\n"
            L"+2 = Medium (16-bit)\n"
            L"+4 = Large (32-bit)";
        notes.AddMemoryNote(0x0000, "Author", sNote);
        notes.DoFrame();

        // should receive notifications for the pointer note, and for each subnote
        Assert::AreEqual({4U}, notes.mNewNotes.size());
        Assert::AreEqual(sNote, notes.mNewNotes[0x00]);
        Assert::AreEqual(std::wstring(L"Small (8-bit)"), notes.mNewNotes[0x11]);
        Assert::AreEqual(std::wstring(L"Medium (16-bit)"), notes.mNewNotes[0x12]);
        Assert::AreEqual(std::wstring(L"Large (32-bit)"), notes.mNewNotes[0x14]);

        notes.AssertNoNote(0x02U);
        notes.AssertNote(0x12U, L"Medium (16-bit)", Memory::Size::SixteenBit, 2);

        // calling DoFrame after updating the pointer should notify about all the affected subnotes
        notes.mNewNotes.clear();
        memory.at(3) = 0x88;
        notes.DoFrame();

        Assert::AreEqual({6U}, notes.mNewNotes.size());
        Assert::AreEqual(std::wstring(L""), notes.mNewNotes[0x11]);
        Assert::AreEqual(std::wstring(L""), notes.mNewNotes[0x12]);
        Assert::AreEqual(std::wstring(L""), notes.mNewNotes[0x14]);
        Assert::AreEqual(std::wstring(L"Small (8-bit)"), notes.mNewNotes[0x09]);
        Assert::AreEqual(std::wstring(L"Medium (16-bit)"), notes.mNewNotes[0x0A]);
        Assert::AreEqual(std::wstring(L"Large (32-bit)"), notes.mNewNotes[0x0C]);

        notes.AssertNoNote(0x02U);
        notes.AssertNoNote(0x12U);
        notes.AssertNote(0x0AU, L"Medium (16-bit)", Memory::Size::SixteenBit, 2);
    }
    
    TEST_METHOD(TestDoFrameRealAddressConversionAvoidedByOverflow)
    {
        MemoryNotesModelHarness notes;
        notes.MonitorNoteChanges();
        notes.mockConsoleContext.AddMemoryRegion(0, 31, ra::data::MemoryRegion::Type::SystemRAM, 0x80);

        std::array<unsigned char, 32> memory{};
        for (uint8_t i = 4; i < memory.size(); i++)
            memory.at(i) = i;
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(0) = 0x90; // start with initial value for pointer (real address = 0x90, RA address = 0x10)

        const std::wstring sNote =
            L"Pointer (32-bit)\n" // only 32-bit pointers are eligible for real address conversion
            L"+0xFFFFFF81 = Small (8-bit)\n"
            L"+0xFFFFFF82 = Medium (16-bit)\n"
            L"+0xFFFFFF84 = Large (32-bit)";
        notes.AddMemoryNote(0x0000, "Author", sNote);
        notes.DoFrame();

        // should receive notifications for the pointer note, and for each subnote
        Assert::AreEqual({4U}, notes.mNewNotes.size());
        Assert::AreEqual(sNote, notes.mNewNotes[0x00]);
        Assert::AreEqual(std::wstring(L"Small (8-bit)"), notes.mNewNotes[0x11]);
        Assert::AreEqual(std::wstring(L"Medium (16-bit)"), notes.mNewNotes[0x12]);
        Assert::AreEqual(std::wstring(L"Large (32-bit)"), notes.mNewNotes[0x14]);

        notes.AssertNoNote(0x02U);
        notes.AssertNote(0x12U, L"Medium (16-bit)", Memory::Size::SixteenBit, 2);

        // calling DoFrame after updating the pointer should notify about all the affected subnotes
        notes.mNewNotes.clear();
        memory.at(0) = 0x88;
        notes.DoFrame();

        Assert::AreEqual({6U}, notes.mNewNotes.size());
        Assert::AreEqual(std::wstring(L""), notes.mNewNotes[0x11]);
        Assert::AreEqual(std::wstring(L""), notes.mNewNotes[0x12]);
        Assert::AreEqual(std::wstring(L""), notes.mNewNotes[0x14]);
        Assert::AreEqual(std::wstring(L"Small (8-bit)"), notes.mNewNotes[0x09]);
        Assert::AreEqual(std::wstring(L"Medium (16-bit)"), notes.mNewNotes[0x0A]);
        Assert::AreEqual(std::wstring(L"Large (32-bit)"), notes.mNewNotes[0x0C]);

        notes.AssertNoNote(0x02U);
        notes.AssertNoNote(0x12U);
        notes.AssertNote(0x0AU, L"Medium (16-bit)", Memory::Size::SixteenBit, 2);
    }

    TEST_METHOD(TestFindCodeNoteStartPointer)
    {
        MemoryNotesModelHarness notes;
        notes.MonitorNoteChanges();

        std::array<unsigned char, 32> memory{};
        for (uint8_t i = 0; i < memory.size(); i++)
            memory.at(i) = i;
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(0) = 16; // start with initial value for pointer

        const std::wstring sNote =
            L"Pointer (8-bit)\n"
            L"+1 = Small (8-bit)\n"
            L"+2 = Medium (16-bit)\n"
            L"+4 = Large (32-bit)";
        notes.AddMemoryNote(0x0000, "Author", sNote);
        notes.DoFrame();

        // indirect notes are at 0x11 (byte), 0x12 (word), and 0x14 (dword)
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(0x10));
        Assert::AreEqual(0x11U, notes.FindNoteStart(0x11));
        Assert::AreEqual(0x12U, notes.FindNoteStart(0x12));
        Assert::AreEqual(0x12U, notes.FindNoteStart(0x13));
        Assert::AreEqual(0x14U, notes.FindNoteStart(0x14));
        Assert::AreEqual(0x14U, notes.FindNoteStart(0x15));
        Assert::AreEqual(0x14U, notes.FindNoteStart(0x16));
        Assert::AreEqual(0x14U, notes.FindNoteStart(0x17));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(0x18));
    }

    TEST_METHOD(TestFindCodeNoteStartPointerOverflow)
    {
        MemoryNotesModelHarness notes;
        notes.mockConsoleContext.AddMemoryRegion(0, 31, ra::data::MemoryRegion::Type::SystemRAM, 0x80);
        std::array<unsigned char, 32> memory{};
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(4) = 0x88; // start with initial value for pointer (real address = 0x88, RA address = 0x08)

        const std::wstring sPointerNote =
            L"Pointer (32-bit)\n"              // only 32-bit pointers are eligible for real address conversion
            L"+0xFFFFFF88 = Small (8-bit)\n"   // 8+8=16
            L"+0xFFFFFF90 = Medium (16-bit)\n" // 16+8=24
            L"+0xFFFFFF98 = Large (32-bit)";   // 24+8=32
        notes.AddMemoryNote(4, "Author", sPointerNote);
        notes.DoFrame();

        // indirect notes are at 16 (byte), 24 (word), and 32 (dword)
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(15));
        Assert::AreEqual(16U, notes.FindNoteStart(16));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(17));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(23));
        Assert::AreEqual(24U, notes.FindNoteStart(24));
        Assert::AreEqual(24U, notes.FindNoteStart(25));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(31));
        Assert::AreEqual(32U, notes.FindNoteStart(32));
        Assert::AreEqual(32U, notes.FindNoteStart(33));
        Assert::AreEqual(32U, notes.FindNoteStart(34));
        Assert::AreEqual(32U, notes.FindNoteStart(35));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(36));
    }

    TEST_METHOD(TestGetIndirectSource)
    {
        MemoryNotesModelHarness notes;
        notes.MonitorNoteChanges();

        std::array<unsigned char, 32> memory{};
        for (uint8_t i = 0; i < memory.size(); i++)
            memory.at(i) = i;
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(0) = 16; // start with initial value for pointer

        const std::wstring sNote =
            L"Pointer (8-bit)\n"
            L"+1 = Small (8-bit)\n"
            L"+2 = Medium (16-bit)\n"
            L"+4 = Large (32-bit)";
        notes.AddMemoryNote(0x0000, "Author", sNote);
        notes.AddMemoryNote(0x0008, "Author", L"Not indirect");
        notes.DoFrame();

        // indirect notes are at 0x11 (byte), 0x12 (word), and 0x14 (dword)
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x10));
        Assert::AreEqual(0x0U, notes.GetIndirectSource(0x11));
        Assert::AreEqual(0x0U, notes.GetIndirectSource(0x12));
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x13));
        Assert::AreEqual(0x0U, notes.GetIndirectSource(0x14));
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x15));
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x16));
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x17));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(0x18));

        // non-indirect
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x08));
    }

    TEST_METHOD(TestGetIndirectSourceOverflow)
    {
        MemoryNotesModelHarness notes;
        notes.mockConsoleContext.AddMemoryRegion(0, 31, ra::data::MemoryRegion::Type::SystemRAM, 0x80);
        std::array<unsigned char, 32> memory{};
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(4) = 0x90; // start with initial value for pointer (real address = 0x90, RA address = 0x10)

        const std::wstring sNote =
            L"Pointer (32-bit)\n" // only 32-bit pointers are eligible for real address conversion
            L"+0xFFFFFF81 = Small (8-bit)\n"
            L"+0xFFFFFF82 = Medium (16-bit)\n"
            L"+0xFFFFFF84 = Large (32-bit)";
        notes.AddMemoryNote(0x0004, "Author", sNote);
        notes.AddMemoryNote(0x0008, "Author", L"Not indirect");
        notes.DoFrame();

        // indirect notes are at 0x11 (byte), 0x12 (word), and 0x14 (dword)
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x10));
        Assert::AreEqual(0x4U, notes.GetIndirectSource(0x11));
        Assert::AreEqual(0x4U, notes.GetIndirectSource(0x12));
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x13));
        Assert::AreEqual(0x4U, notes.GetIndirectSource(0x14));
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x15));
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x16));
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x17));
        Assert::AreEqual(0xFFFFFFFF, notes.FindNoteStart(0x18));

        // non-indirect
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x08));
    }
    
    TEST_METHOD(TestSetServerCodeNote)
    {
        MemoryNotesModelHarness notes;
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());

        notes.SetNote(0x1234, L"This is a note.");
        Assert::AreEqual(AssetChanges::Unpublished, notes.GetChanges());
        notes.AssertNote(0x1234, std::wstring(L"This is a note."));

        notes.SetServerNote(0x1234, L"This is a note.");
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());
        notes.AssertNote(0x1234, std::wstring(L"This is a note."));

        notes.SetNote(0x1234, L"This is a new note.");
        Assert::AreEqual(AssetChanges::Unpublished, notes.GetChanges());
        notes.AssertNote(0x1234, std::wstring(L"This is a new note."));

        notes.SetServerNote(0x1234, L"This is a newer note.");
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());
        notes.AssertNote(0x1234, std::wstring(L"This is a newer note."));
    }

    TEST_METHOD(TestSerialize)
    {
        MemoryNotesModelHarness notes;
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());

        notes.SetNote(0x1234, L"This is a note.");
        Assert::AreEqual(AssetChanges::Unpublished, notes.GetChanges());
        notes.AssertSerialize("N0:0x1234:\"This is a note.\"");

        notes.SetNote(0x1234, L"");
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());
    }

    TEST_METHOD(TestSerializeEscaped)
    {
        MemoryNotesModelHarness notes;
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());

        notes.SetNote(0x1234, L"16-bit pointer\n+2:\ta\n+4:\tb\n");
        notes.SetNote(0x0099, L"This string is \"quoted\".");
        Assert::AreEqual(AssetChanges::Unpublished, notes.GetChanges());
        notes.AssertSerialize("N0:0x0099:\"This string is \\\"quoted\\\".\"\n"
                              "N0:0x1234:\"16-bit pointer\\n+2:\\ta\\n+4:\\tb\\n\"");
    }

    TEST_METHOD(TestSerializeDeleted)
    {
        MemoryNotesModelHarness notes;
        notes.SetServerNote(0x1234, L"This is a note.");
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());

        notes.SetNote(0x1234, L"");
        Assert::AreEqual(AssetChanges::Unpublished, notes.GetChanges());
        notes.AssertSerialize("N0:0x1234:\"\"");

        notes.SetNote(0x1234, L"This is a note.");
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());
    }

    TEST_METHOD(TestDeserialize)
    {
        MemoryNotesModelHarness notes;
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());

        const std::string sSerialized = ":0x1234:\"This is a note.\"";
        ra::util::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Consume(':');

        Assert::IsTrue(notes.Deserialize(pTokenizer));
        Assert::AreEqual(AssetChanges::Unpublished, notes.GetChanges());
        notes.AssertNote(0x1234, std::wstring(L"This is a note."));
    }

    TEST_METHOD(TestDeserializeEscaped)
    {
        MemoryNotesModelHarness notes;
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());

        const std::string sSerialized = ":0x1234:\"16-bit pointer\\n+2:\\t\\\"a\\\"\\n+4:\\tb\\n\"";
        ra::util::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Consume(':');

        Assert::IsTrue(notes.Deserialize(pTokenizer));
        Assert::AreEqual(AssetChanges::Unpublished, notes.GetChanges());
        notes.AssertNote(0x1234, std::wstring(L"16-bit pointer\r\n+2:\t\"a\"\r\n+4:\tb\r\n"));
    }

    TEST_METHOD(TestDeserializeUnchanged)
    {
        MemoryNotesModelHarness notes;
        notes.SetServerNote(0x1234, L"This is a note.");
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());

        const std::string sSerialized = ":0x1234:\"This is a note.\"";
        ra::util::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Consume(':');

        Assert::IsTrue(notes.Deserialize(pTokenizer));
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());
        notes.AssertNote(0x1234, std::wstring(L"This is a note."));
    }

    TEST_METHOD(TestDeserializeDeleted)
    {
        MemoryNotesModelHarness notes;
        notes.SetServerNote(0x1234, L"This is a note.");
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());

        const std::string sSerialized = ":0x1234:\"\"";
        ra::util::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Consume(':');

        Assert::IsTrue(notes.Deserialize(pTokenizer));
        Assert::AreEqual(AssetChanges::Unpublished, notes.GetChanges());

        // uncommitted deleted note still exists
        notes.AssertNote(0x1234U, L"");
        Assert::IsTrue(notes.IsNoteModified(0x1234U));

        // committed deleted note does not exist
        notes.SetServerNote(0x1234U, L"");
        notes.AssertNoNote(0x1234);
    }

    TEST_METHOD(TestGetNextNoteAddress)
    {
        MemoryNotesModelHarness notes;
        const std::wstring sPointerNote =
            L"Pointer\n"
            L"+8 = Unknown\n"
            L"+16 = Small (16-bit)";
        notes.AddMemoryNote(1234, "Author", sPointerNote);
        notes.AddMemoryNote(20, "Author", L"After [32-bit]");
        notes.AddMemoryNote(4, "Author", L"Before");
        notes.AddMemoryNote(12, "Author", L"In the middle");
        notes.DoFrame();

        Assert::AreEqual({4U}, notes.GetNextNoteAddress({0U}));
        Assert::AreEqual({12U}, notes.GetNextNoteAddress({4U}));
        Assert::AreEqual({20U}, notes.GetNextNoteAddress({12U}));
        Assert::AreEqual({1234U}, notes.GetNextNoteAddress({20U}));
        Assert::AreEqual({0xFFFFFFFFU}, notes.GetNextNoteAddress({1234U}));

        Assert::AreEqual({4U}, notes.GetNextNoteAddress({0U}, true));
        Assert::AreEqual({8U}, notes.GetNextNoteAddress({4U}, true));
        Assert::AreEqual({12U}, notes.GetNextNoteAddress({8U}, true));
        Assert::AreEqual({16U}, notes.GetNextNoteAddress({12U}, true));
        Assert::AreEqual({20U}, notes.GetNextNoteAddress({16U}, true));
        Assert::AreEqual({1234U}, notes.GetNextNoteAddress({20U}, true));
        Assert::AreEqual({0xFFFFFFFFU}, notes.GetNextNoteAddress({1234U}, true));
    }

    TEST_METHOD(TestGetNextNoteAddressOverflow)
    {
        MemoryNotesModelHarness notes;
        notes.mockConsoleContext.AddMemoryRegion(0, 31, ra::data::MemoryRegion::Type::SystemRAM, 0x80);
        std::array<unsigned char, 32> memory{};
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(4) = 0x88; // start with initial value for pointer (real address = 0x88, RA address = 0x08)

        const std::wstring sPointerNote =
            L"Pointer (32-bit)\n"              // only 32-bit pointers are eligible for real address conversion
            L"+0xFFFFFF88 = Small (8-bit)\n"   // 8+8=16
            L"+0xFFFFFF90 = Medium (16-bit)\n" // 16+8=24
            L"+0xFFFFFF98 = Large (32-bit)";   // 24+8=32
        notes.AddMemoryNote(4, "Author", sPointerNote);
        notes.AddMemoryNote(40, "Author", L"After [32-bit]");
        notes.AddMemoryNote(1, "Author", L"Before");
        notes.AddMemoryNote(20, "Author", L"In the middle");
        notes.DoFrame();

        Assert::AreEqual({1U}, notes.GetNextNoteAddress({0U}));
        Assert::AreEqual({4U}, notes.GetNextNoteAddress({1U}));
        Assert::AreEqual({20U}, notes.GetNextNoteAddress({4U}));
        Assert::AreEqual({40U}, notes.GetNextNoteAddress({20U}));
        Assert::AreEqual({0xFFFFFFFFU}, notes.GetNextNoteAddress({40U}));

        Assert::AreEqual({1U}, notes.GetNextNoteAddress({0U}, true));
        Assert::AreEqual({4U}, notes.GetNextNoteAddress({1U}, true));
        Assert::AreEqual({16U}, notes.GetNextNoteAddress({4U}, true));
        Assert::AreEqual({20U}, notes.GetNextNoteAddress({16U}, true));
        Assert::AreEqual({24U}, notes.GetNextNoteAddress({20U}, true));
        Assert::AreEqual({32U}, notes.GetNextNoteAddress({24U}, true));
        Assert::AreEqual({40U}, notes.GetNextNoteAddress({32U}, true));
        Assert::AreEqual({0xFFFFFFFFU}, notes.GetNextNoteAddress({40U}, true));
    }

    TEST_METHOD(TestGetPreviousNoteAddress)
    {
        MemoryNotesModelHarness notes;
        const std::wstring sPointerNote =
            L"Pointer\n"
            L"+8 = Unknown\n"
            L"+16 = Small (16-bit)";
        notes.AddMemoryNote(1234, "Author", sPointerNote);
        notes.AddMemoryNote(20, "Author", L"After [32-bit]");
        notes.AddMemoryNote(4, "Author", L"Before");
        notes.AddMemoryNote(12, "Author", L"In the middle");
        notes.DoFrame();

        Assert::AreEqual({1234U}, notes.GetPreviousNoteAddress({0xFFFFFFFFU}));
        Assert::AreEqual({20U}, notes.GetPreviousNoteAddress({1234U}));
        Assert::AreEqual({12U}, notes.GetPreviousNoteAddress({20U}));
        Assert::AreEqual({4U}, notes.GetPreviousNoteAddress({12U}));
        Assert::AreEqual({0xFFFFFFFFU}, notes.GetPreviousNoteAddress({4U}));

        Assert::AreEqual({1234U}, notes.GetPreviousNoteAddress({0xFFFFFFFFU}, true));
        Assert::AreEqual({20U}, notes.GetPreviousNoteAddress({1234U}, true));
        Assert::AreEqual({16U}, notes.GetPreviousNoteAddress({20U}, true));
        Assert::AreEqual({12U}, notes.GetPreviousNoteAddress({16U}, true));
        Assert::AreEqual({8U}, notes.GetPreviousNoteAddress({12U}, true));
        Assert::AreEqual({4U}, notes.GetPreviousNoteAddress({8U}, true));
        Assert::AreEqual({0xFFFFFFFFU}, notes.GetPreviousNoteAddress({4U}, true));
    }
    
    TEST_METHOD(TestGetPreviousNoteAddressOverflow)
    {
        MemoryNotesModelHarness notes;
        notes.mockConsoleContext.AddMemoryRegion(0, 31, ra::data::MemoryRegion::Type::SystemRAM, 0x80);
        std::array<unsigned char, 32> memory{};
        notes.mockEmulatorMemoryContext.MockMemory(memory);
        memory.at(4) = 0x88; // start with initial value for pointer (real address = 0x88, RA address = 0x08)

        const std::wstring sPointerNote =
            L"Pointer (32-bit)\n"              // only 32-bit pointers are eligible for real address conversion
            L"+0xFFFFFF88 = Small (8-bit)\n"   // 8+8=16
            L"+0xFFFFFF90 = Medium (16-bit)\n" // 16+8=24
            L"+0xFFFFFF98 = Large (32-bit)";   // 24+8=32
        notes.AddMemoryNote(4, "Author", sPointerNote);
        notes.AddMemoryNote(40, "Author", L"After [32-bit]");
        notes.AddMemoryNote(1, "Author", L"Before");
        notes.AddMemoryNote(20, "Author", L"In the middle");
        notes.DoFrame();

        Assert::AreEqual({40U}, notes.GetPreviousNoteAddress({0xFFFFFFFFU}));
        Assert::AreEqual({20U}, notes.GetPreviousNoteAddress({40U}));
        Assert::AreEqual({4U}, notes.GetPreviousNoteAddress({20U}));
        Assert::AreEqual({1U}, notes.GetPreviousNoteAddress({4U}));
        Assert::AreEqual({0xFFFFFFFFU}, notes.GetPreviousNoteAddress({1U}));

        Assert::AreEqual({40U}, notes.GetPreviousNoteAddress({0xFFFFFFFFU}, true));
        Assert::AreEqual({32U}, notes.GetPreviousNoteAddress({40U}, true));
        Assert::AreEqual({24U}, notes.GetPreviousNoteAddress({32U}, true));
        Assert::AreEqual({20U}, notes.GetPreviousNoteAddress({24U}, true));
        Assert::AreEqual({16U}, notes.GetPreviousNoteAddress({20U}, true));
        Assert::AreEqual({4U}, notes.GetPreviousNoteAddress({16U}, true));
        Assert::AreEqual({1U}, notes.GetPreviousNoteAddress({4U}, true));
        Assert::AreEqual({0xFFFFFFFFU}, notes.GetPreviousNoteAddress({1U}, true));
    }
};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
