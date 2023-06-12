#include "CppUnitTest.h"

#include "data\models\CodeNotesModel.hh"

#include "services\impl\StringTextWriter.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"

#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockServer.hh"
#include "tests\mocks\MockThreadPool.hh"
#include "tests\mocks\MockUserContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

TEST_CLASS(CodeNotesModel_Tests)
{
private:
    class CodeNotesModelHarness : public CodeNotesModel
    {
    public:
        ra::api::mocks::MockServer mockServer;
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockThreadPool mockThreadPool;
        ra::ui::mocks::MockDesktop mockDesktop;

        std::map<unsigned, std::wstring> mNewNotes;

        void InitializeCodeNotes(unsigned nGameId)
        {
            mNewNotes.clear();

            CodeNotesModel::Refresh(nGameId,
                [this](ra::ByteAddress nAddress, const std::wstring& sNewNote) {
                    mNewNotes[nAddress] = sNewNote;
                },
                []() {});

            mockThreadPool.ExecuteNextTask(); // FetchCodeNotes is async
        }

        void MonitorCodeNoteChanges()
        {
            m_fCodeNoteChanged = [this](ra::ByteAddress nAddress, const std::wstring& sNewNote) {
                mNewNotes[nAddress] = sNewNote;
            };
        }

        void SetGameId(unsigned nGameId) noexcept
        {
            m_nGameId = nGameId;
        }

        using CodeNotesModel::AddCodeNote;

        void AssertNoNote(ra::ByteAddress nAddress)
        {
            const auto* pNote = FindCodeNote(nAddress);
            if (pNote != nullptr)
                Assert::Fail(ra::StringPrintf(L"Note found for address %04X: %s", nAddress, *pNote).c_str());
        }

        void AssertNote(ra::ByteAddress nAddress, const std::wstring& sExpected)
        {
            const auto* pNote = FindCodeNote(nAddress);
            Assert::IsNotNull(pNote, ra::StringPrintf(L"Note not found for address %04X", nAddress).c_str());
            Ensures(pNote != nullptr);
            Assert::AreEqual(sExpected, *pNote);
        }

        void AssertNote(ra::ByteAddress nAddress, const std::wstring& sExpected, MemSize nExpectedSize, unsigned nExpectedBytes = 0)
        {
            AssertNote(nAddress, sExpected);

            Assert::AreEqual(nExpectedSize, GetCodeNoteMemSize(nAddress),
                ra::StringPrintf(L"Size for address %04X", nAddress).c_str());

            if (nExpectedBytes)
            {
                Assert::AreEqual(nExpectedBytes, GetCodeNoteBytes(nAddress),
                    ra::StringPrintf(L"Bytes for address %04X", nAddress).c_str());
            }
        }

        void AssertIndirectNote(ra::ByteAddress nAddress, unsigned nOffset, const std::wstring& sExpected)
        {
            const auto* pNote = FindIndirectCodeNote(nAddress, nOffset);
            Assert::IsNotNull(pNote, ra::StringPrintf(L"Note not found for address %04X + %u", nAddress, nOffset).c_str());
            Ensures(pNote != nullptr);
            Assert::AreEqual(sExpected, *pNote);
        }

        void AssertSerialize(const std::string& sExpected)
        {
            std::string sSerialized = "N0";
            ra::services::impl::StringTextWriter pTextWriter(sSerialized);
            Serialize(pTextWriter);

            Assert::AreEqual(sExpected, sSerialized);
        }
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        CodeNotesModelHarness notes;

        Assert::AreEqual(AssetType::CodeNotes, notes.GetType());
        Assert::AreEqual(0U, notes.GetID());
        Assert::AreEqual(std::wstring(L"Code Notes"), notes.GetName());
        Assert::AreEqual(std::wstring(L""), notes.GetDescription());
        Assert::AreEqual(AssetCategory::Core, notes.GetCategory());
        Assert::AreEqual(AssetState::Inactive, notes.GetState());
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());
    }

    
    TEST_METHOD(TestLoadCodeNotes)
    {
        CodeNotesModelHarness notes;
        notes.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request& request, ra::api::FetchCodeNotes::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1234, L"Note1", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 2345, L"Note2", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 3456, L"Note3", "Author" });
            return true;
        });

        notes.InitializeCodeNotes(1U);
        Assert::AreEqual({3U}, notes.mNewNotes.size());

        notes.AssertNote(1234U, L"Note1");
        Assert::AreEqual(std::wstring(L"Note1"), notes.mNewNotes[1234U]);

        notes.AssertNote(2345U, L"Note2");
        Assert::AreEqual(std::wstring(L"Note2"), notes.mNewNotes[2345U]);

        notes.AssertNote(3456U, L"Note3");
        Assert::AreEqual(std::wstring(L"Note3"), notes.mNewNotes[3456U]);

        const auto* pNote4 = notes.FindCodeNote(4567U);
        Assert::IsNull(pNote4);

        notes.InitializeCodeNotes(0U);
        const auto* pNote5 = notes.FindCodeNote(1234U);
        Assert::IsNull(pNote5);
        Assert::AreEqual({0U}, notes.mNewNotes.size());
    }

    void TestCodeNoteSize(const std::wstring& sNote, unsigned int nExpectedBytes, MemSize nExpectedSize)
    {
        CodeNotesModelHarness notes;
        notes.mockServer.HandleRequest<ra::api::FetchCodeNotes>([&sNote](const ra::api::FetchCodeNotes::Request& request, ra::api::FetchCodeNotes::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1234, sNote, "Author" });
            return true;
        });

        notes.InitializeCodeNotes(1U);

        Assert::AreEqual(nExpectedBytes, notes.GetCodeNoteBytes(1234U), sNote.c_str());
        Assert::AreEqual(nExpectedSize, notes.GetCodeNoteMemSize(1234U), sNote.c_str());
    }

    TEST_METHOD(TestLoadCodeNotesSized)
    {
        TestCodeNoteSize(L"", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"16-bit Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Test 16-bit", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Test 16-bi", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[16-bit] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[16 bit] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[16 Bit] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[24-bit] Test", 3U, MemSize::TwentyFourBit);
        TestCodeNoteSize(L"[32-bit] Test", 4U, MemSize::ThirtyTwoBit);
        TestCodeNoteSize(L"[32 bit] Test", 4U, MemSize::ThirtyTwoBit);
        TestCodeNoteSize(L"[32bit] Test", 4U, MemSize::ThirtyTwoBit);
        TestCodeNoteSize(L"Test [16-bit]", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Test (16-bit)", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Test (16 bits)", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[64-bit] Test", 8U, MemSize::Array);
        TestCodeNoteSize(L"[128-bit] Test", 16U, MemSize::Array);
        TestCodeNoteSize(L"[17-bit] Test", 3U, MemSize::TwentyFourBit);
        TestCodeNoteSize(L"[100-bit] Test", 13U, MemSize::Array);
        TestCodeNoteSize(L"[0-bit] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[1-bit] Test", 1U, MemSize::EightBit);
        TestCodeNoteSize(L"[4-bit] Test", 1U, MemSize::EightBit);
        TestCodeNoteSize(L"[8-bit] Test", 1U, MemSize::EightBit);
        TestCodeNoteSize(L"[9-bit] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"bit", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"9bit", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"-bit", 1U, MemSize::Unknown);

        TestCodeNoteSize(L"[16-bit BE] Test", 2U, MemSize::SixteenBitBigEndian);
        TestCodeNoteSize(L"[24-bit BE] Test", 3U, MemSize::TwentyFourBitBigEndian);
        TestCodeNoteSize(L"[32-bit BE] Test", 4U, MemSize::ThirtyTwoBitBigEndian);
        TestCodeNoteSize(L"Test [32-bit BE]", 4U, MemSize::ThirtyTwoBitBigEndian);
        TestCodeNoteSize(L"Test (32-bit BE)", 4U, MemSize::ThirtyTwoBitBigEndian);
        TestCodeNoteSize(L"Test 32-bit BE", 4U, MemSize::ThirtyTwoBitBigEndian);
        TestCodeNoteSize(L"[16-bit BigEndian] Test", 2U, MemSize::SixteenBitBigEndian);
        TestCodeNoteSize(L"[16-bit-BE] Test", 2U, MemSize::SixteenBitBigEndian);
        TestCodeNoteSize(L"[4-bit BE] Test", 1U, MemSize::EightBit);

        TestCodeNoteSize(L"8 BYTE Test", 8U, MemSize::Array);
        TestCodeNoteSize(L"Test 8 BYTE", 8U, MemSize::Array);
        TestCodeNoteSize(L"Test 8 BYT", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[2 Byte] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[4 Byte] Test", 4U, MemSize::ThirtyTwoBit);
        TestCodeNoteSize(L"[4 Byte - Float] Test", 4U, MemSize::Float);
        TestCodeNoteSize(L"[8 Byte] Test", 8U, MemSize::Array);
        TestCodeNoteSize(L"[100 Bytes] Test", 100U, MemSize::Array);
        TestCodeNoteSize(L"[2 byte] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"[2-byte] Test", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Test (6 bytes)", 6U, MemSize::Array);
        TestCodeNoteSize(L"[2byte] Test", 2U, MemSize::SixteenBit);

        TestCodeNoteSize(L"[float] Test", 4U, MemSize::Float);
        TestCodeNoteSize(L"[float32] Test", 4U, MemSize::Float);
        TestCodeNoteSize(L"Test float", 4U, MemSize::Float);
        TestCodeNoteSize(L"Test floa", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"is floating", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"has floated", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"16-afloat", 1U, MemSize::Unknown);

        TestCodeNoteSize(L"[MBF32] Test", 4U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF40] Test", 5U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF32 float] Test", 4U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF80] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[MBF320] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[MBF-32] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"[32-bit MBF] Test", 4U, MemSize::MBF32);
        TestCodeNoteSize(L"[40-bit MBF] Test", 5U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF] Test", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"Test MBF32", 4U, MemSize::MBF32);
        TestCodeNoteSize(L"[MBF32 LE] Test", 4U, MemSize::MBF32LE);
        TestCodeNoteSize(L"[MBF40-LE] Test", 5U, MemSize::MBF32LE);

        TestCodeNoteSize(L"42=bitten", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"42-bitten", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"bit by bit", 1U, MemSize::Unknown);
        TestCodeNoteSize(L"bit1=chest", 1U, MemSize::Unknown);

        TestCodeNoteSize(L"Bite count (16-bit)", 2U, MemSize::SixteenBit);
        TestCodeNoteSize(L"Number of bits collected (32 bits)", 4U, MemSize::ThirtyTwoBit);

        TestCodeNoteSize(L"100 32-bit pointers [400 bytes]", 400U, MemSize::Array);
        TestCodeNoteSize(L"[400 bytes] 100 32-bit pointers", 400U, MemSize::Array);
    }

    TEST_METHOD(TestFindCodeNoteSized)
    {
        CodeNotesModelHarness notes;
        notes.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request& request, ra::api::FetchCodeNotes::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1000, L"[32-bit] Location", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1100, L"Level", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1110, L"[16-bit] Strength", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1120, L"[8 byte] Exp", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1200, L"[20 bytes] Items\r\nMultiline ignored", "Author" });
            return true;
        });

        notes.InitializeCodeNotes(1U);

        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(100, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(999, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [1/4]"), notes.FindCodeNote(1000, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [2/4]"), notes.FindCodeNote(1001, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [3/4]"), notes.FindCodeNote(1002, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [4/4]"), notes.FindCodeNote(1003, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(1004, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"Level"), notes.FindCodeNote(1100, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [1/2]"), notes.FindCodeNote(1110, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [2/2]"), notes.FindCodeNote(1111, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [1/8]"), notes.FindCodeNote(1120, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [2/8]"), notes.FindCodeNote(1121, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [3/8]"), notes.FindCodeNote(1122, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [4/8]"), notes.FindCodeNote(1123, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [5/8]"), notes.FindCodeNote(1124, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [6/8]"), notes.FindCodeNote(1125, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [7/8]"), notes.FindCodeNote(1126, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[8 byte] Exp [8/8]"), notes.FindCodeNote(1127, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(1128, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[20 bytes] Items [1/20]"), notes.FindCodeNote(1200, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[20 bytes] Items [10/20]"), notes.FindCodeNote(1209, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"[20 bytes] Items [20/20]"), notes.FindCodeNote(1219, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(1300, MemSize::EightBit));

        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(100, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(998, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(999, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(1000, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(1001, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(1002, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(1003, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(1004, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"Level [partial]"), notes.FindCodeNote(1099, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"Level [partial]"), notes.FindCodeNote(1100, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), notes.FindCodeNote(1109, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength"), notes.FindCodeNote(1110, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), notes.FindCodeNote(1111, MemSize::SixteenBit));

        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(100, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(996, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(997, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location"), notes.FindCodeNote(1000, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(1001, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(1002, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[32-bit] Location [partial]"), notes.FindCodeNote(1003, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(1004, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"Level [partial]"), notes.FindCodeNote(1097, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"Level [partial]"), notes.FindCodeNote(1100, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), notes.FindCodeNote(1107, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), notes.FindCodeNote(1110, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L"[16-bit] Strength [partial]"), notes.FindCodeNote(1111, MemSize::ThirtyTwoBit));
        Assert::AreEqual(std::wstring(L""), notes.FindCodeNote(1112, MemSize::ThirtyTwoBit));
    }

    TEST_METHOD(TestFindCodeNoteStart)
    {
        CodeNotesModelHarness notes;
        notes.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request& request, ra::api::FetchCodeNotes::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1000, L"[32-bit] Location", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1100, L"Level", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1110, L"[16-bit] Strength", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1120, L"[8 byte] Exp", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1200, L"[20 bytes] Items", "Author" });
            return true;
        });

        notes.InitializeCodeNotes(1U);

        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(100));

        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(999));
        Assert::AreEqual(1000U, notes.FindCodeNoteStart(1000));
        Assert::AreEqual(1000U, notes.FindCodeNoteStart(1001));
        Assert::AreEqual(1000U, notes.FindCodeNoteStart(1002));
        Assert::AreEqual(1000U, notes.FindCodeNoteStart(1003));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1004));

        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1099));
        Assert::AreEqual(1100U, notes.FindCodeNoteStart(1100));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1101));

        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1109));
        Assert::AreEqual(1110U, notes.FindCodeNoteStart(1110));
        Assert::AreEqual(1110U, notes.FindCodeNoteStart(1111));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1112));

        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1119));
        Assert::AreEqual(1120U, notes.FindCodeNoteStart(1120));
        Assert::AreEqual(1120U, notes.FindCodeNoteStart(1127));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1128));

        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1199));
        Assert::AreEqual(1200U, notes.FindCodeNoteStart(1200));
        Assert::AreEqual(1200U, notes.FindCodeNoteStart(1219));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1220));
    }

    TEST_METHOD(TestFindCodeNoteStartOverlap)
    {
        CodeNotesModelHarness notes;
        notes.mockServer.HandleRequest<ra::api::FetchCodeNotes>([](const ra::api::FetchCodeNotes::Request& request, ra::api::FetchCodeNotes::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);

            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1000, L"[100 bytes] Outer", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1010, L"[10 bytes] Inner", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1015, L"Individual", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1120, L"[10 bytes] Secondary", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1125, L"[10 bytes] Overlap", "Author" });
            response.Notes.emplace_back(ra::api::FetchCodeNotes::Response::CodeNote{ 1200, L"Extra", "Author" });
            return true;
        });

        notes.InitializeCodeNotes(1U);

        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(999));
        Assert::AreEqual(1000U, notes.FindCodeNoteStart(1000));
        Assert::AreEqual(1000U, notes.FindCodeNoteStart(1009));
        Assert::AreEqual(1010U, notes.FindCodeNoteStart(1010));
        Assert::AreEqual(1010U, notes.FindCodeNoteStart(1014));
        Assert::AreEqual(1015U, notes.FindCodeNoteStart(1015));
        Assert::AreEqual(1010U, notes.FindCodeNoteStart(1016));
        Assert::AreEqual(1010U, notes.FindCodeNoteStart(1019));
        Assert::AreEqual(1000U, notes.FindCodeNoteStart(1020));
        Assert::AreEqual(1000U, notes.FindCodeNoteStart(1099));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1100));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1119));
        Assert::AreEqual(1120U, notes.FindCodeNoteStart(1120));
        Assert::AreEqual(1120U, notes.FindCodeNoteStart(1124));
        Assert::AreEqual(1125U, notes.FindCodeNoteStart(1125));
        Assert::AreEqual(1125U, notes.FindCodeNoteStart(1126));
        Assert::AreEqual(1125U, notes.FindCodeNoteStart(1130));
        Assert::AreEqual(1125U, notes.FindCodeNoteStart(1134));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1135));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1199));
        Assert::AreEqual(1200U, notes.FindCodeNoteStart(1200));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(1201));
    }

    TEST_METHOD(TestSetCodeNote)
    {
        CodeNotesModelHarness notes;
        notes.InitializeCodeNotes(1U);
        notes.mNewNotes.clear();
        Assert::AreEqual({ 0U }, notes.CodeNoteCount());

        notes.SetCodeNote(1234, L"Note1");

        notes.AssertNote(1234U, L"Note1");
        Assert::AreEqual(std::wstring(L"Note1"), notes.mNewNotes[1234U]);
        Assert::AreEqual({ 1U }, notes.CodeNoteCount());

        notes.SetCodeNote(1234, L"");
        notes.AssertNoNote(1234U);
        Assert::AreEqual(std::wstring(L""), notes.mNewNotes[1234U]);
        Assert::AreEqual({ 0U }, notes.CodeNoteCount());
    }

    TEST_METHOD(TestDeleteCodeNote)
    {
        CodeNotesModelHarness notes;
        notes.mockServer.HandleRequest<ra::api::UpdateCodeNote>([](const ra::api::UpdateCodeNote::Request&, ra::api::UpdateCodeNote::Response& response)
        {
            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        notes.mockServer.HandleRequest<ra::api::DeleteCodeNote>([](const ra::api::DeleteCodeNote::Request& request, ra::api::DeleteCodeNote::Response& response)
        {
            Assert::AreEqual(1U, request.GameId);
            Assert::AreEqual(1234U, request.Address);

            response.Result = ra::api::ApiResult::Success;
            return true;
        });

        notes.SetGameId(1U);
        notes.SetCodeNote(1234U, L"Note1");
        notes.SetServerCodeNote(1234U, L"Note1");

        notes.AssertNote(1234U, L"Note1");

        // setting a note to blank does not actually delete it until it's committed
        notes.SetCodeNote(1234U, L"");
        notes.AssertNote(1234U, L"");
        Assert::IsTrue(notes.IsNoteModified(1234U));

        // committed deleted note should no longer exist
        notes.SetServerCodeNote(1234U, L"");
        notes.AssertNoNote(1234U);
    }

    TEST_METHOD(TestDeleteCodeNoteNonExistant)
    {
        CodeNotesModelHarness notes;
        notes.mockServer.ExpectUncalled<ra::api::DeleteCodeNote>();

        notes.SetGameId(1U);

        notes.SetCodeNote(1234, L"");
        const auto* pNote1 = notes.FindCodeNote(1234U);
        Assert::IsNull(pNote1);
    }

    TEST_METHOD(TestCodeNotePointer1)
    {
        CodeNotesModelHarness notes;
        const std::wstring sNote =
            L"Bomb Timer Pointer (24-bit)\n"
            L"+03 - Bombs Defused\n"
            L"+04 - Bomb Timer";
        notes.AddCodeNote(1234, "Author", sNote);

        notes.AssertNote(1234U, sNote, MemSize::TwentyFourBit); // full note for pointer address

        // extracted notes for offset fields (note: pointer base default is $0000)
        notes.AssertNote(3U, L"Bombs Defused", MemSize::Unknown, 1);
        notes.AssertNote(4U, L"Bomb Timer", MemSize::Unknown, 1);
    }

    TEST_METHOD(TestCodeNotePointer2)
    {
        CodeNotesModelHarness notes;
        const std::wstring sNote =
            L"Pointer [32bit]\n"
            L"+0x1BC | Equipment - Head - String[24 Bytes]\n"
            L"---DEFAULT_HEAD = Barry's Head\n"
            L"---FRAGGER_HEAD = Fragger Helmet";
        notes.AddCodeNote(1234, "Author", sNote);

        notes.AssertNote(1234U, sNote, MemSize::ThirtyTwoBit); // full note for pointer address

        // extracted notes for offset fields (note: pointer base default is $0000)
        notes.AssertNote(0x1BCU, L"Equipment - Head - String[24 Bytes]\n"
            L"---DEFAULT_HEAD = Barry's Head\n"
            L"---FRAGGER_HEAD = Fragger Helmet", MemSize::Array, 24);
    }

    TEST_METHOD(TestCodeNotePointer3)
    {
        CodeNotesModelHarness notes;
        const std::wstring sNote =
            L"[32-bit] Pointer for Races (only exists during Race Game)\n"
            L"+0x7F47 = [8-bit] Acorns collected in current track\n"
            L"+0x8000 = [8-bit] Current lap\n"
            L"+0x8033 = [16-bit] Total race time";
        notes.AddCodeNote(1234, "Author", sNote);

        notes.AssertNote(1234U, sNote, MemSize::ThirtyTwoBit); // full note for pointer address

        // extracted notes for offset fields (note: pointer base default is $0000)
        notes.AssertNote(0x7F47U, L"[8-bit] Acorns collected in current track", MemSize::EightBit);
        notes.AssertNote(0x8000U, L"[8-bit] Current lap", MemSize::EightBit);
        notes.AssertNote(0x8033U, L"[16-bit] Total race time", MemSize::SixteenBit);
    }

    TEST_METHOD(TestCodeNotePointer4)
    {
        CodeNotesModelHarness notes;
        const std::wstring sNote =
            L"Pointer (16bit because negative)\n\n"
            L"Circuit:\n"
            L"+0x1B56E = Current Position\n"
            L"+0x1B57E = Total Racers\n\n"
            L"Free Run:\n"
            L"+0x1B5BE = Seconds 0x\n"
            L"+0x1B5CE = Lap";
        notes.AddCodeNote(1234, "Author", sNote);

        notes.AssertNote(1234U, sNote, MemSize::SixteenBit); // full note for pointer address

        // extracted notes for offset fields (note: pointer base default is $0000)
        notes.AssertNote(0x1B56EU, L"Current Position", MemSize::Unknown, 1);
        notes.AssertNote(0x1B57EU, L"Total Racers\n\nFree Run:", MemSize::Unknown, 1);
        notes.AssertNote(0x1B5BEU, L"Seconds 0x", MemSize::Unknown, 1);
        notes.AssertNote(0x1B5CEU, L"Lap", MemSize::Unknown, 1);
    }

    TEST_METHOD(TestCodeNotePointer5)
    {
        CodeNotesModelHarness notes;
        const std::wstring sNote =
            L"Pointer\r\n"
            L"[OFFSETS]\r\n"
            L"+2 = EXP (32-bit)\r\n"
            L"+5 = Base Level (8-bit)\r\n"
            L"+6 = Job Level (8-bit)\r\n"
            L"+20 = Stat Points (16-bit)\r\n"
            L"+22 = Skill Points (8-bit)";
        notes.AddCodeNote(1234, "Author", sNote);

        notes.AssertNote(1234U, sNote, MemSize::ThirtyTwoBit, 4); // full note for pointer address (assume 32-bit if not specified)

        // extracted notes for offset fields (note: pointer base default is $0000)
        notes.AssertNote(2, L"EXP (32-bit)", MemSize::ThirtyTwoBit);
        notes.AssertNote(5, L"Base Level (8-bit)", MemSize::EightBit);
        notes.AssertNote(6, L"Job Level (8-bit)", MemSize::EightBit);
        notes.AssertNote(20, L"Stat Points (16-bit)", MemSize::SixteenBit);
        notes.AssertNote(22, L"Skill Points (8-bit)", MemSize::EightBit);
    }

    TEST_METHOD(TestCodeNotePointerNested)
    {
        CodeNotesModelHarness notes;
        const std::wstring sNote =
            L"Pointer [32bit]\n"
            L"+0x428 | Pointer - Award - Tee Hee Two (32bit)\n"
            L"--- +0x24C | Flag\n"
            L"+0x438 | Pointer - Award - Pretty Woman (32bit)\n"
            L"--- +0x24C | Flag";
        notes.AddCodeNote(1234, "Author", sNote);

        notes.AssertNote(1234U, sNote, MemSize::ThirtyTwoBit); // full note for pointer address

        // extracted notes for offset fields (note: pointer base default is $0000)
        notes.AssertNote(0x428U, L"Pointer - Award - Tee Hee Two (32bit)\n--- +0x24C | Flag", MemSize::ThirtyTwoBit);
        notes.AssertNote(0x438U, L"Pointer - Award - Pretty Woman (32bit)\n--- +0x24C | Flag", MemSize::ThirtyTwoBit);
    }

    TEST_METHOD(TestCodeNotePointerNonPrefixedOffsets)
    {
        CodeNotesModelHarness notes;
        const std::wstring sNote =
            L"[24-bit] Mech stats pointer (health...)\n"
            L"+07 = Jump Jet {8-bit}\n"
            L"+5C = Right Leg Health {16-bit}\n"
            L"+5E = Left Leg Health {16-bit}";
        notes.AddCodeNote(1234, "Author", sNote);

        // offset parse failure results in a non-pointer note
        notes.AssertNote(1234U, sNote, MemSize::TwentyFourBit);
        notes.AssertNoNote(0x07U);
        notes.AssertNoNote(0x5CU);
        notes.AssertNoNote(0x5EU);
    }

    TEST_METHOD(TestCodeNotePointerOverwrite)
    {
        CodeNotesModelHarness notes;
        const std::wstring sNote =
            L"Pointer (16bit because negative)\n"
            L"+0x1B56E = Current Position\n"
            L"+0x1B57E = Total Racers\n"
            L"+0x1B56E = Seconds 0x\n"
            L"+0x1B5CE = Lap";
        notes.AddCodeNote(1234, "Author", sNote);

        notes.AssertNote(1234U, sNote, MemSize::SixteenBit); // full note for pointer address

        // extracted notes for offset fields (note: pointer base default is $0000)
        // both note for 0x1B56E exist in the array, but only the first is returned
        notes.AssertNote(0x1B56EU, L"Current Position", MemSize::Unknown, 1);
        notes.AssertNote(0x1B57EU, L"Total Racers", MemSize::Unknown, 1);
        notes.AssertNote(0x1B5CEU, L"Lap", MemSize::Unknown, 1);
    }

    TEST_METHOD(TestFindCodeNoteSizedPointer)
    {
        CodeNotesModelHarness notes;
        const std::wstring sNote =
            L"Pointer\n"
            L"+1 = Unknown\n"
            L"+2 = Small (8-bit)\n"
            L"+4 = Medium (16-bit)\n"
            L"+6 = Large (32-bit)\n"
            L"+10 = Very Large (8 bytes)";
        notes.AddCodeNote(1234, "Author", sNote);

        Assert::AreEqual(std::wstring(), notes.FindCodeNote(0, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"Unknown [indirect]"), notes.FindCodeNote(1, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"Small (8-bit) [indirect]"), notes.FindCodeNote(2, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"Medium (16-bit) [1/2] [indirect]"), notes.FindCodeNote(4, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"Medium (16-bit) [2/2] [indirect]"), notes.FindCodeNote(5, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"Large (32-bit) [1/4] [indirect]"), notes.FindCodeNote(6, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"Large (32-bit) [4/4] [indirect]"), notes.FindCodeNote(9, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"Very Large (8 bytes) [1/8] [indirect]"), notes.FindCodeNote(10, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"Very Large (8 bytes) [8/8] [indirect]"), notes.FindCodeNote(17, MemSize::EightBit));
        Assert::AreEqual(std::wstring(), notes.FindCodeNote(18, MemSize::EightBit));

        Assert::AreEqual(std::wstring(L"Unknown [partial] [indirect]"), notes.FindCodeNote(0, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"Unknown [partial] [indirect]"), notes.FindCodeNote(1, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"Small (8-bit) [partial] [indirect]"), notes.FindCodeNote(2, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"Medium (16-bit) [indirect]"), notes.FindCodeNote(4, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"Medium (16-bit) [partial] [indirect]"), notes.FindCodeNote(5, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"Large (32-bit) [partial] [indirect]"), notes.FindCodeNote(6, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"Large (32-bit) [partial] [indirect]"), notes.FindCodeNote(9, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"Very Large (8 bytes) [partial] [indirect]"), notes.FindCodeNote(10, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"Very Large (8 bytes) [partial] [indirect]"), notes.FindCodeNote(17, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(), notes.FindCodeNote(18, MemSize::SixteenBit));
    }

    TEST_METHOD(TestFindIndirectCodeNote)
    {
        CodeNotesModelHarness notes;
        const std::wstring sNote =
            L"Pointer\r\n"
            L"[OFFSETS]\r\n"
            L"+2 = EXP (32-bit)\r\n"
            L"+5 = Base Level (8-bit)\r\n"
            L"+6 = Job Level (8-bit)\r\n"
            L"+20 = Stat Points (16-bit)\r\n"
            L"+22 = Skill Points (8-bit)";
        notes.AddCodeNote(1234, "Author", sNote);

        notes.AssertIndirectNote(1234U, 5, L"Base Level (8-bit)");
        notes.AssertIndirectNote(1234U, 22, L"Skill Points (8-bit)");
        Assert::IsNull(notes.FindIndirectCodeNote(1234U, 0)); // no offset
        Assert::IsNull(notes.FindIndirectCodeNote(1234U, 21)); // unknown offset
        Assert::IsNull(notes.FindIndirectCodeNote(1235U, 5)); // wrong base address
    }

    TEST_METHOD(TestEnumerateCodeNotes)
    {
        CodeNotesModelHarness notes;
        const std::wstring sPointerNote =
            L"Pointer\n"
            L"+8 = Unknown\n"
            L"+16 = Small (8-bit)";
        notes.AddCodeNote(1234, "Author", sPointerNote);
        notes.AddCodeNote(20, "Author", L"After [32-bit]");
        notes.AddCodeNote(4, "Author", L"Before");
        notes.AddCodeNote(12, "Author", L"In the middle");

        int i = 0;
        notes.EnumerateCodeNotes([&i, &sPointerNote](ra::ByteAddress nAddress, unsigned nBytes, const std::wstring& sNote) {
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

    TEST_METHOD(TestEnumerateCodeNotesWithIndirect)
    {
        CodeNotesModelHarness notes;
        const std::wstring sPointerNote =
            L"Pointer\n"
            L"+8 = Unknown\n"
            L"+16 = Small (16-bit)";
        notes.AddCodeNote(1234, "Author", sPointerNote);
        notes.AddCodeNote(20, "Author", L"After [32-bit]");
        notes.AddCodeNote(4, "Author", L"Before");
        notes.AddCodeNote(12, "Author", L"In the middle");

        int i = 0;
        notes.EnumerateCodeNotes([&i, &sPointerNote](ra::ByteAddress nAddress, unsigned nBytes, const std::wstring& sNote) {
            switch (i++)
            {
                case 0:
                    Assert::AreEqual({4U}, nAddress);
                    Assert::AreEqual(1U, nBytes);
                    Assert::AreEqual(std::wstring(L"Before"), sNote);
                    break;
                case 1:
                    Assert::AreEqual({8U}, nAddress);
                    Assert::AreEqual(1U, nBytes);
                    Assert::AreEqual(std::wstring(L"Unknown"), sNote);
                    break;
                case 2:
                    Assert::AreEqual({12U}, nAddress);
                    Assert::AreEqual(1U, nBytes);
                    Assert::AreEqual(std::wstring(L"In the middle"), sNote);
                    break;
                case 3:
                    Assert::AreEqual({16U}, nAddress);
                    Assert::AreEqual(2U, nBytes);
                    Assert::AreEqual(std::wstring(L"Small (16-bit)"), sNote);
                    break;
                case 4:
                    Assert::AreEqual({20U}, nAddress);
                    Assert::AreEqual(4U, nBytes);
                    Assert::AreEqual(std::wstring(L"After [32-bit]"), sNote);
                    break;
                case 5:
                    Assert::AreEqual({1234U}, nAddress);
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

    TEST_METHOD(TestDoFrame)
    {
        CodeNotesModelHarness notes;
        notes.MonitorCodeNoteChanges();

        std::array<unsigned char, 32> memory{};
        for (uint8_t i = 0; i < memory.size(); i++)
            memory.at(i) = i;
        notes.mockEmulatorContext.MockMemory(memory);
        memory.at(0) = 16; // start with initial value for pointer

        const std::wstring sNote =
            L"Pointer (8-bit)\n"
            L"+1 = Small (8-bit)\n"
            L"+2 = Medium (16-bit)\n"
            L"+4 = Large (32-bit)";
        notes.AddCodeNote(0x0000, "Author", sNote);

        // should receive notifications for the pointer note, and for each subnote
        Assert::AreEqual({4U}, notes.mNewNotes.size());
        Assert::AreEqual(sNote, notes.mNewNotes[0x00]);
        Assert::AreEqual(std::wstring(L"Small (8-bit)"), notes.mNewNotes[0x11]);
        Assert::AreEqual(std::wstring(L"Medium (16-bit)"), notes.mNewNotes[0x12]);
        Assert::AreEqual(std::wstring(L"Large (32-bit)"), notes.mNewNotes[0x14]);

        notes.AssertNoNote(0x02U);
        notes.AssertNote(0x12U, L"Medium (16-bit)", MemSize::SixteenBit, 2);

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
        notes.AssertNote(0x0AU, L"Medium (16-bit)", MemSize::SixteenBit, 2);

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
        notes.AssertNote(0x0AU, L"Large (32-bit)", MemSize::ThirtyTwoBit, 4);

        // no change to pointer should not raise any events
        notes.mNewNotes.clear();
        notes.DoFrame();
        Assert::AreEqual({0U}, notes.mNewNotes.size());
    }

    TEST_METHOD(TestDoFrameRealAddressConversion)
    {
        CodeNotesModelHarness notes;
        notes.MonitorCodeNoteChanges();
        notes.mockConsoleContext.AddMemoryRegion(0, 31, ra::data::context::ConsoleContext::AddressType::SystemRAM, 0x80);

        std::array<unsigned char, 32> memory{};
        for (uint8_t i = 4; i < memory.size(); i++)
            memory.at(i) = i;
        notes.mockEmulatorContext.MockMemory(memory);
        memory.at(0) = 0x90; // start with initial value for pointer (real address = 0x90, RA address = 0x10)

        const std::wstring sNote =
            L"Pointer (32-bit)\n" // only 32-bit pointers are eligible for real address conversion
            L"+1 = Small (8-bit)\n"
            L"+2 = Medium (16-bit)\n"
            L"+4 = Large (32-bit)";
        notes.AddCodeNote(0x0000, "Author", sNote);

        // should receive notifications for the pointer note, and for each subnote
        Assert::AreEqual({4U}, notes.mNewNotes.size());
        Assert::AreEqual(sNote, notes.mNewNotes[0x00]);
        Assert::AreEqual(std::wstring(L"Small (8-bit)"), notes.mNewNotes[0x11]);
        Assert::AreEqual(std::wstring(L"Medium (16-bit)"), notes.mNewNotes[0x12]);
        Assert::AreEqual(std::wstring(L"Large (32-bit)"), notes.mNewNotes[0x14]);

        notes.AssertNoNote(0x02U);
        notes.AssertNote(0x12U, L"Medium (16-bit)", MemSize::SixteenBit, 2);

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
        notes.AssertNote(0x0AU, L"Medium (16-bit)", MemSize::SixteenBit, 2);
    }

    TEST_METHOD(TestFindCodeNoteStartPointer)
    {
        CodeNotesModelHarness notes;
        notes.MonitorCodeNoteChanges();

        std::array<unsigned char, 32> memory{};
        for (uint8_t i = 0; i < memory.size(); i++)
            memory.at(i) = i;
        notes.mockEmulatorContext.MockMemory(memory);
        memory.at(0) = 16; // start with initial value for pointer

        const std::wstring sNote =
            L"Pointer (8-bit)\n"
            L"+1 = Small (8-bit)\n"
            L"+2 = Medium (16-bit)\n"
            L"+4 = Large (32-bit)";
        notes.AddCodeNote(0x0000, "Author", sNote);

        // indirect notes are at 0x11 (byte), 0x12 (word), and 0x14 (dword)
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(0x10));
        Assert::AreEqual(0x11U, notes.FindCodeNoteStart(0x11));
        Assert::AreEqual(0x12U, notes.FindCodeNoteStart(0x12));
        Assert::AreEqual(0x12U, notes.FindCodeNoteStart(0x13));
        Assert::AreEqual(0x14U, notes.FindCodeNoteStart(0x14));
        Assert::AreEqual(0x14U, notes.FindCodeNoteStart(0x15));
        Assert::AreEqual(0x14U, notes.FindCodeNoteStart(0x16));
        Assert::AreEqual(0x14U, notes.FindCodeNoteStart(0x17));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(0x18));
    }

    TEST_METHOD(TestGetIndirectSource)
    {
        CodeNotesModelHarness notes;
        notes.MonitorCodeNoteChanges();

        std::array<unsigned char, 32> memory{};
        for (uint8_t i = 0; i < memory.size(); i++)
            memory.at(i) = i;
        notes.mockEmulatorContext.MockMemory(memory);
        memory.at(0) = 16; // start with initial value for pointer

        const std::wstring sNote =
            L"Pointer (8-bit)\n"
            L"+1 = Small (8-bit)\n"
            L"+2 = Medium (16-bit)\n"
            L"+4 = Large (32-bit)";
        notes.AddCodeNote(0x0000, "Author", sNote);
        notes.AddCodeNote(0x0008, "Author", L"Not indirect");

        // indirect notes are at 0x11 (byte), 0x12 (word), and 0x14 (dword)
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x10));
        Assert::AreEqual(0x0U, notes.GetIndirectSource(0x11));
        Assert::AreEqual(0x0U, notes.GetIndirectSource(0x12));
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x13));
        Assert::AreEqual(0x0U, notes.GetIndirectSource(0x14));
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x15));
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x16));
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x17));
        Assert::AreEqual(0xFFFFFFFF, notes.FindCodeNoteStart(0x18));

        // non-indirect
        Assert::AreEqual(0xFFFFFFFF, notes.GetIndirectSource(0x08));
    }
    
    TEST_METHOD(TestSetServerCodeNote)
    {
        CodeNotesModelHarness notes;
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());

        notes.SetCodeNote(0x1234, L"This is a note.");
        Assert::AreEqual(AssetChanges::Unpublished, notes.GetChanges());
        notes.AssertNote(0x1234, std::wstring(L"This is a note."));

        notes.SetServerCodeNote(0x1234, L"This is a note.");
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());
        notes.AssertNote(0x1234, std::wstring(L"This is a note."));

        notes.SetCodeNote(0x1234, L"This is a new note.");
        Assert::AreEqual(AssetChanges::Unpublished, notes.GetChanges());
        notes.AssertNote(0x1234, std::wstring(L"This is a new note."));

        notes.SetServerCodeNote(0x1234, L"This is a newer note.");
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());
        notes.AssertNote(0x1234, std::wstring(L"This is a newer note."));
    }

    TEST_METHOD(TestSerialize)
    {
        CodeNotesModelHarness notes;
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());

        notes.SetCodeNote(0x1234, L"This is a note.");
        Assert::AreEqual(AssetChanges::Unpublished, notes.GetChanges());
        notes.AssertSerialize("N0:0x1234:\"This is a note.\"");

        notes.SetCodeNote(0x1234, L"");
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());
    }

    TEST_METHOD(TestSerializeEscaped)
    {
        CodeNotesModelHarness notes;
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());

        notes.SetCodeNote(0x1234, L"16-bit pointer\n+2:\ta\n+4:\tb\n");
        notes.SetCodeNote(0x0099, L"This string is \"quoted\".");
        Assert::AreEqual(AssetChanges::Unpublished, notes.GetChanges());
        notes.AssertSerialize("N0:0x0099:\"This string is \\\"quoted\\\".\"\n"
                              "N0:0x1234:\"16-bit pointer\\n+2:\\ta\\n+4:\\tb\\n\"");
    }

    TEST_METHOD(TestSerializeDeleted)
    {
        CodeNotesModelHarness notes;
        notes.SetServerCodeNote(0x1234, L"This is a note.");
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());

        notes.SetCodeNote(0x1234, L"");
        Assert::AreEqual(AssetChanges::Unpublished, notes.GetChanges());
        notes.AssertSerialize("N0:0x1234:\"\"");

        notes.SetCodeNote(0x1234, L"This is a note.");
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());
    }

    TEST_METHOD(TestDeserialize)
    {
        CodeNotesModelHarness notes;
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());

        const std::string sSerialized = ":0x1234:\"This is a note.\"";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Consume(':');

        Assert::IsTrue(notes.Deserialize(pTokenizer));
        Assert::AreEqual(AssetChanges::Unpublished, notes.GetChanges());
        notes.AssertNote(0x1234, std::wstring(L"This is a note."));
    }

    TEST_METHOD(TestDeserializeEscaped)
    {
        CodeNotesModelHarness notes;
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());

        const std::string sSerialized = ":0x1234:\"16-bit pointer\\n+2:\\t\\\"a\\\"\\n+4:\\tb\\n\"";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Consume(':');

        Assert::IsTrue(notes.Deserialize(pTokenizer));
        Assert::AreEqual(AssetChanges::Unpublished, notes.GetChanges());
        notes.AssertNote(0x1234, std::wstring(L"16-bit pointer\n+2:\t\"a\"\n+4:\tb\n"));
    }

    TEST_METHOD(TestDeserializeUnchanged)
    {
        CodeNotesModelHarness notes;
        notes.SetServerCodeNote(0x1234, L"This is a note.");
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());

        const std::string sSerialized = ":0x1234:\"This is a note.\"";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Consume(':');

        Assert::IsTrue(notes.Deserialize(pTokenizer));
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());
        notes.AssertNote(0x1234, std::wstring(L"This is a note."));
    }

    TEST_METHOD(TestDeserializeDeleted)
    {
        CodeNotesModelHarness notes;
        notes.SetServerCodeNote(0x1234, L"This is a note.");
        Assert::AreEqual(AssetChanges::None, notes.GetChanges());

        const std::string sSerialized = ":0x1234:\"\"";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Consume(':');

        Assert::IsTrue(notes.Deserialize(pTokenizer));
        Assert::AreEqual(AssetChanges::Unpublished, notes.GetChanges());

        // uncommitted deleted note still exists
        notes.AssertNote(0x1234U, L"");
        Assert::IsTrue(notes.IsNoteModified(0x1234U));

        // committed deleted note does not exist
        notes.SetServerCodeNote(0x1234U, L"");
        notes.AssertNoNote(0x1234);
    }

    TEST_METHOD(TestGetNextNoteAddress)
    {
        CodeNotesModelHarness notes;
        const std::wstring sPointerNote =
            L"Pointer\n"
            L"+8 = Unknown\n"
            L"+16 = Small (16-bit)";
        notes.AddCodeNote(1234, "Author", sPointerNote);
        notes.AddCodeNote(20, "Author", L"After [32-bit]");
        notes.AddCodeNote(4, "Author", L"Before");
        notes.AddCodeNote(12, "Author", L"In the middle");

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

    TEST_METHOD(TestGetPreviousNoteAddress)
    {
        CodeNotesModelHarness notes;
        const std::wstring sPointerNote =
            L"Pointer\n"
            L"+8 = Unknown\n"
            L"+16 = Small (16-bit)";
        notes.AddCodeNote(1234, "Author", sPointerNote);
        notes.AddCodeNote(20, "Author", L"After [32-bit]");
        notes.AddCodeNote(4, "Author", L"Before");
        notes.AddCodeNote(12, "Author", L"In the middle");

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
};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
