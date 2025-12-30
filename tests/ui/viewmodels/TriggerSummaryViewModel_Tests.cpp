#include "CppUnitTest.h"

#include "ui\viewmodels\TriggerSummaryViewModel.hh"

#include "tests\ui\UIAsserts.hh"
#include "tests\mocks\MockClipboard.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockImageRepository.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"

#include <rcheevos\src\rcheevos\rc_internal.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(TriggerSummaryViewModel_Tests)
{
private:
    class TriggerSummaryViewModelHarness : public TriggerSummaryViewModel
    {
    public:
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockUserContext mockUserContext;

        void InitializeFrom(const std::string& sTrigger)
        {
            const auto nSize = rc_trigger_size(sTrigger.c_str());
            if (nSize > 0)
            {
                m_sTriggerBuffer.resize(nSize);
                rc_trigger_t* pTrigger = rc_parse_trigger(m_sTriggerBuffer.data(), sTrigger.c_str(), nullptr, 0);
                TriggerSummaryViewModel::InitializeFrom(*pTrigger->requirement);
            }
        }

        void AssertClause(gsl::index nIndex, const std::wstring& sIndices,
            const std::wstring& sReference, const std::wstring& sOperation, const std::wstring& sTarget)
        {
            const auto* pClause = Clauses().GetItemAt(nIndex);
            Assert::IsNotNull(pClause);
            Assert::AreEqual(sIndices, pClause->GetIndices());
            Assert::AreEqual(sReference, pClause->GetReference());
            Assert::AreEqual(sOperation, pClause->GetOperation());
            Assert::AreEqual(sTarget, pClause->GetTarget());
        }

    private:
        std::string m_sTriggerBuffer;
    };

public:
    TEST_METHOD(TestSimpleNoNotes)
    {
        TriggerSummaryViewModelHarness summary;
        summary.InitializeFrom("0xH1234=5_0xH2345!=0");

        Assert::AreEqual({ 2U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"0x1234", L"is", L"5");
        summary.AssertClause(1, L"2", L"0x2345", L"is not", L"0");
    }

    TEST_METHOD(TestSimpleNote)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetCodeNote({ 0x1234U }, L"World");
        summary.mockGameContext.SetCodeNote({ 0x2345U }, L"[16-bit] Level");
        summary.InitializeFrom("0xH1234=5_0x 2345!=0");

        Assert::AreEqual({ 2U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"World", L"is", L"5");
        summary.AssertClause(1, L"2", L"Level", L"is not", L"0");
    }

    TEST_METHOD(TestEnumNote)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetCodeNote({ 0x1234U }, L"Character (1=Bill, 2=Bob, 3=Betty, 4=Bonnie, 5=Ben)");
        summary.mockGameContext.SetCodeNote({ 0x2345U }, L"Difficulty\r\n0=Easy\r\n1=Normal\r\n2=Hard\r\n");
        summary.InitializeFrom("0xH1234=5_0x 2345!=0");

        Assert::AreEqual({ 2U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"Character", L"is", L"Ben");
        summary.AssertClause(1, L"2", L"Difficulty", L"is not", L"Easy");
    }

    TEST_METHOD(TestEnumNoteGreaterThanZero)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetCodeNote({ 0x1234U }, L"Character (1=Bill, 2=Bob, 3=Betty, 4=Bonnie, 5=Ben)");
        summary.mockGameContext.SetCodeNote({ 0x2345U }, L"Difficulty\r\n0=Easy\r\n1=Normal\r\n2=Hard\r\n");
        summary.InitializeFrom("0xH1234>2_0x 2345>0");

        Assert::AreEqual({ 2U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"Character", L"is greater than", L"Bob");
        summary.AssertClause(1, L"2", L"Difficulty", L"is not", L"Easy"); // >0 converted to !=0
    }

    TEST_METHOD(TestEnumNoteLessThanOne)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetCodeNote({ 0x1234U }, L"Character (1=Bill, 2=Bob, 3=Betty, 4=Bonnie, 5=Ben)");
        summary.mockGameContext.SetCodeNote({ 0x2345U }, L"Difficulty\r\n0=Easy\r\n1=Normal\r\n2=Hard\r\n");
        summary.InitializeFrom("0xH1234<1_0x 2345<1");

        Assert::AreEqual({ 2U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"Character", L"is", L"0");
        summary.AssertClause(1, L"2", L"Difficulty", L"is", L"Easy"); // <1 converted to ==0
    }

    TEST_METHOD(TestMemoryReferenceDeltaSelf)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetCodeNote({ 0x2345U }, L"Difficulty\r\n0=Easy\r\n1=Normal\r\n2=Hard\r\n");
        summary.InitializeFrom("0xH1234!=d0xH1234_0x 2345>d0x 2345_0x 2345<d0x 2345_d0x 2345<0x 2345");

        Assert::AreEqual({ 4U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"0x1234", L"changed", L"");
        summary.AssertClause(1, L"2", L"Difficulty", L"increased", L"");
        summary.AssertClause(2, L"3", L"Difficulty", L"decreased", L"");
        summary.AssertClause(3, L"4", L"Difficulty", L"increased", L"");
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
