#include "CppUnitTest.h"

#include "ui\viewmodels\TriggerSummaryViewModel.hh"

#include "tests\devkit\context\mocks\MockConsoleContext.hh"
#include "tests\devkit\context\mocks\MockEmulatorMemoryContext.hh"
#include "tests\devkit\context\mocks\MockUserContext.hh"
#include "tests\mocks\MockGameContext.hh"

#include "ui\EditorTheme.hh"

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
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        ra::context::mocks::MockUserContext mockUserContext;
        ra::data::context::mocks::MockGameContext mockGameContext;

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
            const std::wstring& sReference, const std::wstring& sOperation,
            const std::wstring& sTarget, const std::wstring& sTally = L"")
        {
            const auto* pClause = Clauses().GetItemAt(nIndex);
            Assert::IsNotNull(pClause);
            Assert::AreEqual(sIndices, pClause->GetIndices(), ra::util::String::Printf(L"Indices on clause %u differ", nIndex).c_str());
            Assert::AreEqual(sReference, pClause->GetReference(), ra::util::String::Printf(L"reference on clause %u differ", nIndex).c_str());
            Assert::AreEqual(sOperation, pClause->GetOperation(), ra::util::String::Printf(L"Operation on clause %u differ", nIndex).c_str());
            Assert::AreEqual(sTarget, pClause->GetTarget(), ra::util::String::Printf(L"Target on clause %u differ", nIndex).c_str());
            Assert::AreEqual(sTally, pClause->GetTally(), ra::util::String::Printf(L"Tally on clause %u differ", nIndex).c_str());
        }

        void AssertHeader(gsl::index nIndex, const std::wstring& sHeader)
        {
            AssertClause(nIndex, L"", sHeader, L"", L"", L"");
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

    TEST_METHOD(TestSimpleDeltaOnly)
    {
        TriggerSummaryViewModelHarness summary;
        summary.InitializeFrom("d0xH1234=5_d0xH2345!=0");

        Assert::AreEqual({ 2U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"0x1234", L"last frame was", L"5");
        summary.AssertClause(1, L"2", L"0x2345", L"last frame was not", L"0");
    }

    TEST_METHOD(TestSimplePriorOnly)
    {
        TriggerSummaryViewModelHarness summary;
        summary.InitializeFrom("p0xH1234=5_p0xH2345!=0");

        Assert::AreEqual({ 2U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"0x1234", L"was", L"5");
        summary.AssertClause(1, L"2", L"0x2345", L"was not", L"0");
    }

    TEST_METHOD(TestSimpleHitTarget)
    {
        TriggerSummaryViewModelHarness summary;
        summary.InitializeFrom("0xH1234=5.8._0xH2345>d0xH2345.4.");

        Assert::AreEqual({ 2U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"0x1234", L"is", L"5", L"for 8 frames");
        summary.AssertClause(1, L"2", L"0x2345", L"increased", L"", L"4 times");
    }

    TEST_METHOD(TestSimpleOnce)
    {
        TriggerSummaryViewModelHarness summary;
        summary.InitializeFrom("0xH1234=5.1._R:0xH2345=6.1.");

        Assert::AreEqual({ 2U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"0x1234", L"is", L"5", L""); // don't report once for "start" conditions
        summary.AssertClause(1, L"2", L"0x2345", L"is", L"6", L"once"); // do report once for non-"start" conditions
    }

    TEST_METHOD(TestSimpleNote)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetNote({ 0x1234U }, L"World");
        summary.mockGameContext.SetNote({ 0x2345U }, L"[16-bit] Level");
        summary.InitializeFrom("0xH1234=5_0x 2345!=0");

        Assert::AreEqual({ 2U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"World", L"is", L"5");
        summary.AssertClause(1, L"2", L"Level", L"is not", L"0");
    }

    TEST_METHOD(TestEnumNote)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetNote({ 0x1234U }, L"Character (1=Bill, 2=Bob, 3=Betty, 4=Bonnie, 5=Ben)");
        summary.mockGameContext.SetNote({ 0x2345U }, L"Difficulty\r\n0=Easy\r\n1=Normal\r\n2=Hard\r\n");
        summary.InitializeFrom("0xH1234=5_0x 2345!=0");

        Assert::AreEqual({ 2U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"Character", L"is", L"Ben");
        summary.AssertClause(1, L"2", L"Difficulty", L"is not", L"Easy");
    }

    TEST_METHOD(TestEnumNoteGreaterThanZero)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetNote({ 0x1234U }, L"Character (1=Bill, 2=Bob, 3=Betty, 4=Bonnie, 5=Ben)");
        summary.mockGameContext.SetNote({ 0x2345U }, L"Difficulty\r\n0=Easy\r\n1=Normal\r\n2=Hard\r\n");
        summary.InitializeFrom("0xH1234>2_0x 2345>0");

        Assert::AreEqual({ 2U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"Character", L"is greater than", L"Bob");
        summary.AssertClause(1, L"2", L"Difficulty", L"is not", L"Easy"); // >0 converted to !=0
    }

    TEST_METHOD(TestEnumNoteLessThanOne)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetNote({ 0x1234U }, L"Character (1=Bill, 2=Bob, 3=Betty, 4=Bonnie, 5=Ben)");
        summary.mockGameContext.SetNote({ 0x2345U }, L"Difficulty\r\n0=Easy\r\n1=Normal\r\n2=Hard\r\n");
        summary.InitializeFrom("0xH1234<1_0x 2345<1");

        Assert::AreEqual({ 2U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"Character", L"is", L"0");
        summary.AssertClause(1, L"2", L"Difficulty", L"is", L"Easy"); // <1 converted to ==0
    }

    TEST_METHOD(TestMemoryReferenceDeltaSelf)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetNote({ 0x2345U }, L"Difficulty\r\n0=Easy\r\n1=Normal\r\n2=Hard\r\n");
        summary.InitializeFrom("0xH1234!=d0xH1234_0x 2345>d0x 2345_0x 2345<d0x 2345_d0x 2345<0x 2345");

        Assert::AreEqual({ 4U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"0x1234", L"changed", L"");
        summary.AssertClause(1, L"2", L"Difficulty", L"increased", L"");
        summary.AssertClause(2, L"3", L"Difficulty", L"decreased", L"");
        summary.AssertClause(3, L"4", L"Difficulty", L"increased", L"");
    }

    TEST_METHOD(TestChangedTo)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetNote({ 0x1234U }, L"World");
        summary.InitializeFrom("0xH1234=5_d0xH1234!=5");

        Assert::AreEqual({ 1U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1-2", L"World", L"changed to", L"5");
    }

    TEST_METHOD(TestChangedFrom)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetNote({ 0x1234U }, L"World");
        summary.InitializeFrom("0xH1234!=5_d0xH1234=5");

        Assert::AreEqual({ 1U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1-2", L"World", L"changed from", L"5");
    }

    TEST_METHOD(TestBitChangedTo)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetNote({ 0x1234U }, L"b1:Town\r\nb2:World");
        summary.InitializeFrom("0xO1234=1_d0xO1234=0");

        Assert::AreEqual({ 1U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1-2", L"World", L"changed to", L"1");
    }

    TEST_METHOD(TestBitChangedToDeltaFirst)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetNote({ 0x1234U }, L"World");
        summary.InitializeFrom("d0xO1234=1_0xO1234=0");

        Assert::AreEqual({ 1U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1-2", L"World", L"changed to", L"0");
    }

    TEST_METHOD(TestBitChangedToFromExplicit)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetNote({ 0x1234U }, L"World");
        summary.InitializeFrom("0xH1234=1_d0xH1234=0");

        Assert::AreEqual({ 2U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"World", L"changed to", L"1");
        summary.AssertClause(1, L"2", L"World", L"changed from", L"0");
    }

    TEST_METHOD(TestBitChangedToFromExplicitDeltaFirst)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetNote({ 0x1234U }, L"World");
        summary.InitializeFrom("d0xH1234=1_0xH1234=0");

        Assert::AreEqual({ 2U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1", L"World", L"changed from", L"1");
        summary.AssertClause(1, L"2", L"World", L"changed to", L"0");
    }

    TEST_METHOD(TestIncreasedTo)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetNote({ 0x1234U }, L"World");
        summary.InitializeFrom("0xH1234=5_d0xH1234<5");

        Assert::AreEqual({ 1U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1-2", L"World", L"increased to", L"5");
    }

    TEST_METHOD(TestDecreasedTo)
    {
        TriggerSummaryViewModelHarness summary;
        summary.mockGameContext.SetNote({ 0x1234U }, L"World");
        summary.InitializeFrom("0xH1234=5_d0xH1234>5");

        Assert::AreEqual({ 1U }, summary.Clauses().Count());
        summary.AssertClause(0, L"1-2", L"World", L"decreased to", L"5");
    }

    TEST_METHOD(TestAddHeadersSimple)
    {
        ra::ui::EditorTheme pTheme;
        ra::services::ServiceLocator::ServiceOverride<ra::ui::EditorTheme> pThemeOverride(&pTheme);

        TriggerSummaryViewModelHarness summary;
        summary.InitializeFrom("0xH1234=5_0xH2345!=0");
        summary.AddHeaders();

        Assert::AreEqual({ 3U }, summary.Clauses().Count());
        summary.AssertHeader(0, L"--- TRIGGER WHEN ---");
        summary.AssertClause(1, L"1", L"0x1234", L"is", L"5");
        summary.AssertClause(2, L"2", L"0x2345", L"is not", L"0");
    }

    TEST_METHOD(TestAddHeadersSimpleWithDelta)
    {
        ra::ui::EditorTheme pTheme;
        ra::services::ServiceLocator::ServiceOverride<ra::ui::EditorTheme> pThemeOverride(&pTheme);

        TriggerSummaryViewModelHarness summary;
        summary.InitializeFrom("0xH1234=5_0xH2345>d0xH2345");
        summary.AddHeaders();

        Assert::AreEqual({ 4U }, summary.Clauses().Count());
        summary.AssertHeader(0, L"--- TRIGGER WHEN ---");
        summary.AssertClause(1, L"2", L"0x2345", L"increased", L"");
        summary.AssertHeader(2, L"--- WHILE ---");
        summary.AssertClause(3, L"1", L"0x1234", L"is", L"5");
    }

    TEST_METHOD(TestAddHeadersHitTargetWithReset)
    {
        ra::ui::EditorTheme pTheme;
        ra::services::ServiceLocator::ServiceOverride<ra::ui::EditorTheme> pThemeOverride(&pTheme);

        TriggerSummaryViewModelHarness summary;
        summary.InitializeFrom("0xH1234=5_0xH2345=6.1._R:0x3456=8.3.");
        summary.AddHeaders();

        Assert::AreEqual({ 6U }, summary.Clauses().Count());
        summary.AssertHeader(0, L"--- TRIGGER WHEN ---");
        summary.AssertClause(1, L"1", L"0x1234", L"is", L"5");
        summary.AssertHeader(2, L"--- STARTING WHEN ---");
        summary.AssertClause(3, L"2", L"0x2345", L"is", L"6");
        summary.AssertHeader(4, L"--- FAILING WHEN ---");
        summary.AssertClause(5, L"3", L"0x3456", L"is", L"8", L"for 3 frames");
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
