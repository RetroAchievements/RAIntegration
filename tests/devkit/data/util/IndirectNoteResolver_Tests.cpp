#include "data/util/IndirectNoteResolver.hh"

#include "context/mocks/MockConsoleContext.hh"
#include "context/mocks/MockEmulatorMemoryContext.hh"
#include "context/mocks/MockGameContext.hh"
#include "context/mocks/MockUserContext.hh"

#include "testutil/CppUnitTest.hh"

#include <rcheevos\src\rcheevos\rc_internal.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace util {
namespace tests {

TEST_CLASS(IndirectNoteResolver_Tests)
{
private:
    class IndirectNoteResolverHarness : public IndirectNoteResolver
    {
    public:
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        ra::context::mocks::MockGameContext mockGameContext;
        ra::context::mocks::MockUserContext mockUserContext;

        IndirectNoteResolverHarness()
            : IndirectNoteResolver()
        {
            m_pMemoryNotes = &mockGameContext.MemoryNotes();
        }

        const rc_trigger_t* Parse(const std::string& sInput)
        {
            mockEmulatorMemoryContext.MockMemory(m_pMemory);

            const auto nSize = rc_trigger_size(sInput.c_str());
            Assert::IsTrue(nSize > 0);

            m_sTriggerBuffer.resize(nSize);
            m_pTrigger = rc_parse_trigger(m_sTriggerBuffer.data(), sInput.c_str(), nullptr, 0);

            UpdateMemrefs();
            return m_pTrigger;
        }

        void UpdateMemrefs()
        {
            rc_memrefs_t* memrefs = rc_trigger_get_memrefs(m_pTrigger);
            rc_update_memref_values(memrefs, mockEmulatorMemoryContext.Peek, nullptr);
        }

        void SetMemory(ra::data::ByteAddress nAddress, uint8_t nValue)
        {
            m_pMemory.at(static_cast<size_t>(nAddress)) = nValue;
        }

        const rc_condition_t& GetCondition(size_t nIndex) const
        {
            const rc_condition_t* pCondition = m_pTrigger->requirement->conditions;
            for (; nIndex > 0; --nIndex)
                pCondition = pCondition->next;

            return *pCondition;
        }

    private:
        std::array<unsigned char, 64> m_pMemory = {
            0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
            16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
            32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
            48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
        };

        std::string m_sTriggerBuffer;
        rc_trigger_t* m_pTrigger = nullptr;
    };

public:
    TEST_METHOD(TestSimpleAddress)
    {
        IndirectNoteResolverHarness resolver;
        resolver.mockGameContext.SetNote({ 8U }, L"This is a note.");
        resolver.Parse("0xH0008=3");

        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(8U, resolver.ResolveOperand(resolver.GetCondition(0), true, vParentChain));
        Assert::AreEqual({ 1U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"This is a note."), vParentChain.front().pNote->GetNote());

        Assert::AreEqual(std::wstring(L"0x0008"), resolver.BuildPath(vParentChain));

        vParentChain.clear();
        Assert::AreEqual(0U, resolver.ResolveOperand(resolver.GetCondition(0), false, vParentChain));
        Assert::AreEqual({ 0U }, vParentChain.size());
    }

    TEST_METHOD(TestSimpleAddressNoMemoryNote)
    {
        IndirectNoteResolverHarness resolver;
        resolver.Parse("0xH0008=3");

        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(8U, resolver.ResolveOperand(resolver.GetCondition(0), true, vParentChain));
        Assert::AreEqual({ 1U }, vParentChain.size());
        Assert::IsNull(vParentChain.front().pNote);
    }

    TEST_METHOD(TestSimpleAddressMultiNote)
    {
        IndirectNoteResolverHarness resolver;
        resolver.mockGameContext.SetNote({ 8U }, L"This is a note.");
        resolver.mockGameContext.SetNote({ 9U }, L"This is another note.");
        resolver.Parse("0xH0008>0xH0009");

        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(8U, resolver.ResolveOperand(resolver.GetCondition(0), true, vParentChain));
        Assert::AreEqual({ 1U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"This is a note."), vParentChain.front().pNote->GetNote());

        Assert::AreEqual(std::wstring(L"0x0008"), resolver.BuildPath(vParentChain));

        vParentChain.clear();
        Assert::AreEqual(9U, resolver.ResolveOperand(resolver.GetCondition(0), false, vParentChain));
        Assert::AreEqual({ 1U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"This is another note."), vParentChain.front().pNote->GetNote());

        Assert::AreEqual(std::wstring(L"0x0009"), resolver.BuildPath(vParentChain));
    }

    TEST_METHOD(TestSimpleAddressInMiddleOfNote)
    {
        IndirectNoteResolverHarness resolver;
        resolver.mockGameContext.SetNote({ 8U }, L"[8 bytes] This is a note.");
        resolver.Parse("0xH0008=0xH000C");

        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(8U, resolver.ResolveOperand(resolver.GetCondition(0), true, vParentChain));
        Assert::AreEqual({ 1U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"[8 bytes] This is a note."), vParentChain.front().pNote->GetNote());

        Assert::AreEqual(std::wstring(L"0x0008"), resolver.BuildPath(vParentChain));

        vParentChain.clear();
        Assert::AreEqual(0x0CU, resolver.ResolveOperand(resolver.GetCondition(0), false, vParentChain));
        Assert::AreEqual({ 2U }, vParentChain.size());
        Assert::IsNotNull(vParentChain.front().pNote);
        Assert::AreEqual(std::wstring(L"[8 bytes] This is a note."), vParentChain.front().pNote->GetNote());
        Assert::IsNotNull(vParentChain.back().pNote);
        Assert::AreEqual(std::wstring(L"[8 bytes] This is a note."), vParentChain.front().pNote->GetNote());

        Assert::AreEqual(std::wstring(L"0x0008+4"), resolver.BuildPath(vParentChain));
    }

    TEST_METHOD(TestAddressLargeArrayNote)
    {
        IndirectNoteResolverHarness resolver;
        resolver.mockGameContext.SetNote({ 8U }, L"[100 bytes] This is a note.");

        std::vector<IndirectNoteResolver::Node> vParentChain;
        resolver.Parse("0xH0008=3");
        Assert::AreEqual(8U, resolver.ResolveOperand(resolver.GetCondition(0), true, vParentChain));
        Assert::AreEqual(std::wstring(L"0x0008"), resolver.BuildPath(vParentChain));

        vParentChain.clear();
        resolver.Parse("0xH000C=3");
        Assert::AreEqual(0x0CU, resolver.ResolveOperand(resolver.GetCondition(0), true, vParentChain));
        Assert::AreEqual(std::wstring(L"0x0008+4"), resolver.BuildPath(vParentChain));

        vParentChain.clear();
        resolver.Parse("0xH0016=3");
        Assert::AreEqual(0x16U, resolver.ResolveOperand(resolver.GetCondition(0), true, vParentChain));
        Assert::AreEqual(std::wstring(L"0x0008+0x0e"), resolver.BuildPath(vParentChain));

        vParentChain.clear();
        resolver.Parse("0xH003E=3");
        Assert::AreEqual(0x3EU, resolver.ResolveOperand(resolver.GetCondition(0), true, vParentChain));
        Assert::AreEqual(std::wstring(L"0x0008+0x36"), resolver.BuildPath(vParentChain));
    }

    TEST_METHOD(TestAddressBitNote)
    {
        IndirectNoteResolverHarness resolver;
        resolver.mockGameContext.SetNote({ 8U }, L"b1=Left\r\nb2=Right\r\nb3=Upper\r\n");
        resolver.Parse("0xO0008=1");

        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(0U, resolver.ResolveOperand(resolver.GetCondition(0), true, vParentChain));
        Assert::AreEqual({ 1U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"b1=Left\r\nb2=Right\r\nb3=Upper\r\n"), vParentChain.front().pNote->GetNote());

        Assert::AreEqual(std::wstring(L"0x0008"), resolver.BuildPath(vParentChain));
    }

    TEST_METHOD(TestIndirectAddressNoMemoryNote)
    {
        IndirectNoteResolverHarness resolver;
        resolver.Parse("I:0xH0001_0xH0002=3");

        // $0001 = 1, 1+2 = $0003
        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(3U, resolver.ResolveOperand(resolver.GetCondition(1), true, vParentChain));
        Assert::AreEqual({ 2U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"$0x0001+0x02"), resolver.BuildPath(vParentChain));
        Assert::IsNull(vParentChain.front().pNote);
        Assert::IsNull(vParentChain.back().pNote);
    }

    TEST_METHOD(TestIndirectAddress)
    {
        IndirectNoteResolverHarness resolver;
        resolver.mockGameContext.SetNote({ 1U }, L"[8-bit pointer]\n+2=This is a note.");
        resolver.Parse("I:0xH0001_0xH0002=3");

        // $0001 = 1, 1+2 = $0003
        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(3U, resolver.ResolveOperand(resolver.GetCondition(1), true, vParentChain));
        Assert::AreEqual({ 2U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"$0x0001+0x02"), resolver.BuildPath(vParentChain));
        Assert::AreEqual(std::wstring(L"[8-bit pointer]\n+2=This is a note."), vParentChain.front().pNote->GetNote());
        Assert::AreEqual(std::wstring(L"This is a note."), vParentChain.back().pNote->GetNote());
    }

    TEST_METHOD(TestIndirectAddressMultiply)
    {
        IndirectNoteResolverHarness resolver;
        resolver.Parse("I:0xH0001*3_0xH0002=3");

        // $0001 = 1, 1*3+2 = $0005
        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(5U, resolver.ResolveOperand(resolver.GetCondition(1), true, vParentChain));
        Assert::AreEqual({ 3U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"$0x0001*3+0x02"), resolver.BuildPath(vParentChain));
    }

    TEST_METHOD(TestIndirectAddressMasked)
    {
        IndirectNoteResolverHarness resolver;
        ra::context::mocks::MockConsoleContext mockConsoleContext(ConsoleID::PSP, L"PSP");
        resolver.mockGameContext.SetNote({0U}, L"[32-bit pointer]\n+2=This is a note.");
        resolver.Parse("I:0xX0000&33554431_0xH0002=6"); // 33554431 = 0x01FFFFFF

        // $0001 = 0x08000003, 0x80000003&0x01FFFFFF=0x00000003+2 = 0x00000005
        resolver.SetMemory({ 0 }, 0x03);
        resolver.SetMemory({ 1 }, 0x00);
        resolver.SetMemory({ 2 }, 0x00);
        resolver.SetMemory({ 3 }, 0x08);
        resolver.UpdateMemrefs();

        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(5U, resolver.ResolveOperand(resolver.GetCondition(1), true, vParentChain));
        Assert::AreEqual({ 2U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"$0x0000+0x02"), resolver.BuildPath(vParentChain));
    }

    TEST_METHOD(TestDoubleIndirectAddress)
    {
        IndirectNoteResolverHarness resolver;
        resolver.mockGameContext.SetNote({ 1U }, L"[8-bit pointer]\n+2=First Level [8-bit pointer]\n  +3=Second Level.");
        resolver.Parse("I:0xH0001_I:0xH0002_0xH0003=4");

        // $0001 = 1, 1+2 = 0003, $0003 = 3, 3+3 = 0006, $0006 = 6
        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(6U, resolver.ResolveOperand(resolver.GetCondition(2), true, vParentChain));
        Assert::AreEqual({ 3U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"$0x0001+0x02+0x03"), resolver.BuildPath(vParentChain));

        Assert::AreEqual(std::wstring(L"[8-bit pointer]\n+2=First Level [8-bit pointer]\n  +3=Second Level."), vParentChain.front().pNote->GetNote());
        Assert::AreEqual(std::wstring(L"First Level [8-bit pointer]\n+3=Second Level."), vParentChain.at(1).pNote->GetNote());
        Assert::AreEqual(std::wstring(L"Second Level."), vParentChain.back().pNote->GetNote());
    }

    TEST_METHOD(TestIndirectAddressOffset)
    {
        IndirectNoteResolverHarness resolver;
        resolver.mockGameContext.SetNote({ 1U }, L"Region differentiator");
        resolver.mockGameContext.SetNote({ 2U }, L"[US] Note for NA");
        resolver.Parse("I:0xH0001_0xH0002=3");

        // $0001 = 1, 1+2 = $0003
        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(3U, resolver.ResolveOperand(resolver.GetCondition(1), true, vParentChain));
        Assert::AreEqual({ 2U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"0x0002[$0x0001]"), resolver.BuildPath(vParentChain));

        Assert::AreEqual(std::wstring(L"Region differentiator"), vParentChain.front().pNote->GetNote());
        Assert::AreEqual(std::wstring(L"[US] Note for NA"), vParentChain.back().pNote->GetNote());
    }

    TEST_METHOD(TestIndirectAddressScaledOffset)
    {
        IndirectNoteResolverHarness resolver;
        resolver.mockGameContext.SetNote({ 1U }, L"Region differentiator");
        resolver.mockGameContext.SetNote({ 2U }, L"[US] Note for NA");
        resolver.Parse("I:0xH0001*2_0xH0002=3");

        // $0001 = 1, 1*2+2 = $0004
        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(4U, resolver.ResolveOperand(resolver.GetCondition(1), true, vParentChain));
        Assert::AreEqual({ 3U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"0x0002[$0x0001*2]"), resolver.BuildPath(vParentChain));

        Assert::AreEqual(std::wstring(L"Region differentiator"), vParentChain.front().pNote->GetNote());
        Assert::AreEqual(std::wstring(L"[US] Note for NA"), vParentChain.back().pNote->GetNote());
    }

    TEST_METHOD(TestRecallBasic)
    {
        IndirectNoteResolverHarness resolver;
        resolver.mockGameContext.SetNote({ 1U }, L"[8-bit pointer]\n+2=First Level (8-bit pointer)\n  +3=Second Level.");
        resolver.Parse("K:0xH0001_I:{recall}_I:0xH0002_0xH0003=4");

        // $0001 = 1, 1+2 = $0003, $0003 = 3, 3+3 = $0006
        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(6U, resolver.ResolveOperand(resolver.GetCondition(3), true, vParentChain));
        Assert::AreEqual({ 3U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"{recall:0x01}+0x02+0x03"), resolver.BuildPath(vParentChain));

        Assert::AreEqual(std::wstring(L"[8-bit pointer]\n+2=First Level (8-bit pointer)\n  +3=Second Level."), vParentChain.front().pNote->GetNote());
        Assert::AreEqual(std::wstring(L"First Level (8-bit pointer)\n+3=Second Level."), vParentChain.at(1).pNote->GetNote());
        Assert::AreEqual(std::wstring(L"Second Level."), vParentChain.back().pNote->GetNote());
    }

    TEST_METHOD(TestRecallAddSource)
    {
        IndirectNoteResolverHarness resolver;
        resolver.Parse("A:1_K:0xH0001_I:{recall}_I:0xH0002_0xH0003=4");

        // $0001 = 1, 1+1+2 = $0004, $0004 = 4, 4+3 = $0007
        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(7U, resolver.ResolveOperand(resolver.GetCondition(4), true, vParentChain));
        Assert::AreEqual({ 3U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"{recall:0x02}+0x02+0x03"), resolver.BuildPath(vParentChain));
    }

    TEST_METHOD(TestRecallSubSource)
    {
        IndirectNoteResolverHarness resolver;
        resolver.Parse("B:2_K:0xH0003_I:{recall}_I:0xH0002_0xH0003=4");

        // $0003 = 3, 3-2+2 = $0003, $0003 = 3, 3+3 = $0006
        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(6U, resolver.ResolveOperand(resolver.GetCondition(4), true, vParentChain));
        Assert::AreEqual({ 3U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"{recall:0x01}+0x02+0x03"), resolver.BuildPath(vParentChain));
    }

    TEST_METHOD(TestRecallChain)
    {
        IndirectNoteResolverHarness resolver;
        resolver.SetMemory({ 0x11 }, 0);
        resolver.Parse("0xH1234=6_K:0x 0002&511_K:{recall}*2_K:{recall}+4_I:0x 0010+{recall}_K:0x 0004_I:{recall}_M:0xH0002=20");
        // 1          Byte 0x1234 = 6
        // 2 Remember Word 0x0002 & 0x01ff
        // 3 Remember Recall      * 2
        // 4 Remember Recall      + 4
        // 5 AddAddr  Word 0x0010 + Recall
        // 6 Remember Word 0x0004
        // 7 AddAddr  Recall
        // 8 Measured Byte 0x0002

        // 2: $0002 = 0x0302 & 0x01ff = 0x0102
        // 3: 0x0102 * 2 = 0x0204
        // 4: 0x0204 + 4 = 0x0208
        // 6: $0010 = 0x0010 + 0x0208 + 4 = 0x021c, $021c=0x0000
        // 8: 0x0000 + 0x0002 = 0x0002, $0002 = 2

        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(2U, resolver.ResolveOperand(resolver.GetCondition(7), true, vParentChain));
        Assert::AreEqual({ 2U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"{recall:0x00}+0x02"), resolver.BuildPath(vParentChain));
    }

    TEST_METHOD(TestRecallInvalid)
    {
        IndirectNoteResolverHarness resolver;
        resolver.Parse("A:{recall}_0xH0000=1");

        std::vector<IndirectNoteResolver::Node> vParentChain;
        Assert::AreEqual(0U, resolver.ResolveOperand(resolver.GetCondition(1), true, vParentChain));
        Assert::AreEqual({ 1U }, vParentChain.size());
        Assert::AreEqual(std::wstring(L"0x0000"), resolver.BuildPath(vParentChain));
    }
};

} // namespace tests
} // namespace util
} // namespace data
} // namespace ra
