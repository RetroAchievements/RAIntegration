#include "CppUnitTest.h"

#include "ui\viewmodels\TriggerConditionViewModel.hh"
#include "ui\viewmodels\TriggerViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\ui\viewmodels\TriggerConditionAsserts.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(TriggerConditionViewModel_Tests)
{
private:
    class TriggerConditionViewModelHarness : public TriggerConditionViewModel
    {
    public:
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockConfiguration mockConfiguration;

        void Parse(const std::string& sInput)
        {
            const auto nSize = rc_trigger_size(sInput.c_str());
            sBuffer.resize(nSize);

            const rc_trigger_t* pTrigger = rc_parse_trigger(sBuffer.data(), sInput.c_str(), nullptr, 0);
            Assert::IsNotNull(pTrigger);
            Ensures(pTrigger != nullptr);

            TriggerConditionViewModel::InitializeFrom(*pTrigger->requirement->conditions);
        }

        bool HasSourceSize() const { return GetValue(HasSourceSizeProperty); }
        bool HasTargetSize() const { return GetValue(HasTargetSizeProperty); }
        bool HasTarget() const { return GetValue(HasTargetProperty); }
        bool HasHits() const { return GetValue(HasHitsProperty); }

    private:
        std::string sBuffer;
    };

    void ParseAndRegenerate(const std::string& sInput)
    {
        unsigned char pBuffer[512] = {0};
        const auto nSize = rc_trigger_size(sInput.c_str());
        Assert::IsTrue(nSize > 0 && nSize < sizeof(pBuffer));

        const rc_trigger_t* pTrigger = rc_parse_trigger(pBuffer, sInput.c_str(), nullptr, 0);
        Assert::IsNotNull(pTrigger);
        Ensures(pTrigger != nullptr);

        TriggerConditionViewModel vmCondition;
        vmCondition.InitializeFrom(*pTrigger->requirement->conditions);

        const std::string sOutput = vmCondition.Serialize();
        Assert::AreEqual(sInput, sOutput);
    }

public:
    TEST_METHOD(TestInitialState)
    {
        TriggerConditionViewModelHarness vmCondition;
        Assert::AreEqual(1, vmCondition.GetIndex());
        Assert::AreEqual(TriggerConditionType::Standard, vmCondition.GetType());
        Assert::AreEqual(TriggerOperandType::Address, vmCondition.GetSourceType());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetSourceSize());
        Assert::AreEqual({ 0U }, vmCondition.GetSourceValue());
        Assert::AreEqual(TriggerOperatorType::Equals, vmCondition.GetOperator());
        Assert::AreEqual(TriggerOperandType::Value, vmCondition.GetTargetType());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetTargetSize());
        Assert::AreEqual({ 0U }, vmCondition.GetTargetValue());
        Assert::AreEqual(0U, vmCondition.GetCurrentHits());
        Assert::AreEqual(0U, vmCondition.GetRequiredHits());

        Assert::IsTrue(vmCondition.HasSourceSize());
        Assert::IsFalse(vmCondition.HasTargetSize());
        Assert::IsTrue(vmCondition.HasTarget());
        Assert::IsTrue(vmCondition.HasHits());
    }

    TEST_METHOD(TestHasSourceSize)
    {
        TriggerConditionViewModelHarness vmCondition;
        vmCondition.SetTargetSize(MemSize::SixteenBit);

        Assert::AreEqual(TriggerOperandType::Address, vmCondition.GetSourceType());
        Assert::IsTrue(vmCondition.HasSourceSize());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetSourceSize()); // initial size is 8-bit

        vmCondition.SetSourceType(TriggerOperandType::BCD);
        Assert::AreEqual(TriggerOperandType::BCD, vmCondition.GetSourceType());
        Assert::IsTrue(vmCondition.HasSourceSize());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetSourceSize());

        vmCondition.SetSourceType(TriggerOperandType::Delta);
        Assert::AreEqual(TriggerOperandType::Delta, vmCondition.GetSourceType());
        Assert::IsTrue(vmCondition.HasSourceSize());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetSourceSize());

        vmCondition.SetSourceType(TriggerOperandType::Prior);
        Assert::AreEqual(TriggerOperandType::Prior, vmCondition.GetSourceType());
        Assert::IsTrue(vmCondition.HasSourceSize());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetSourceSize());

        vmCondition.SetSourceType(TriggerOperandType::Value);
        Assert::AreEqual(TriggerOperandType::Value, vmCondition.GetSourceType());
        Assert::IsFalse(vmCondition.HasSourceSize());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetSourceSize()); // value always 32-bit

        vmCondition.SetSourceType(TriggerOperandType::Address);
        Assert::AreEqual(TriggerOperandType::Address, vmCondition.GetSourceType());
        Assert::IsTrue(vmCondition.HasSourceSize());
        Assert::AreEqual(MemSize::SixteenBit, vmCondition.GetSourceSize()); // copied from target size
    }

    TEST_METHOD(TestHasTarget)
    {
        TriggerConditionViewModelHarness vmCondition;
        Assert::AreEqual(TriggerOperatorType::Equals, vmCondition.GetOperator());
        Assert::IsTrue(vmCondition.HasTarget());

        vmCondition.SetOperator(TriggerOperatorType::NotEquals);
        Assert::AreEqual(TriggerOperatorType::NotEquals, vmCondition.GetOperator());
        Assert::IsTrue(vmCondition.HasTarget());

        vmCondition.SetOperator(TriggerOperatorType::LessThan);
        Assert::AreEqual(TriggerOperatorType::LessThan, vmCondition.GetOperator());
        Assert::IsTrue(vmCondition.HasTarget());

        vmCondition.SetOperator(TriggerOperatorType::LessThanOrEqual);
        Assert::AreEqual(TriggerOperatorType::LessThanOrEqual, vmCondition.GetOperator());
        Assert::IsTrue(vmCondition.HasTarget());

        vmCondition.SetOperator(TriggerOperatorType::GreaterThan);
        Assert::AreEqual(TriggerOperatorType::GreaterThan, vmCondition.GetOperator());
        Assert::IsTrue(vmCondition.HasTarget());

        vmCondition.SetOperator(TriggerOperatorType::GreaterThanOrEqual);
        Assert::AreEqual(TriggerOperatorType::GreaterThanOrEqual, vmCondition.GetOperator());
        Assert::IsTrue(vmCondition.HasTarget());

        vmCondition.SetOperator(TriggerOperatorType::None);
        Assert::AreEqual(TriggerOperatorType::None, vmCondition.GetOperator());
        Assert::IsFalse(vmCondition.HasTarget());

        vmCondition.SetOperator(TriggerOperatorType::Multiply);
        Assert::AreEqual(TriggerOperatorType::Multiply, vmCondition.GetOperator());
        Assert::IsTrue(vmCondition.HasTarget());

        vmCondition.SetOperator(TriggerOperatorType::Divide);
        Assert::AreEqual(TriggerOperatorType::Divide, vmCondition.GetOperator());
        Assert::IsTrue(vmCondition.HasTarget());

        vmCondition.SetOperator(TriggerOperatorType::BitwiseAnd);
        Assert::AreEqual(TriggerOperatorType::BitwiseAnd, vmCondition.GetOperator());
        Assert::IsTrue(vmCondition.HasTarget());
    }

    TEST_METHOD(TestHasTargetSize)
    {
        TriggerConditionViewModelHarness vmCondition;
        Assert::AreEqual(TriggerOperandType::Value, vmCondition.GetTargetType());
        Assert::AreEqual(TriggerOperatorType::Equals, vmCondition.GetOperator());
        Assert::IsFalse(vmCondition.HasTargetSize());
        Assert::IsTrue(vmCondition.HasTarget());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetTargetSize()); // value always 32-bit

        vmCondition.SetTargetType(TriggerOperandType::Address);
        Assert::IsTrue(vmCondition.HasTargetSize());
        Assert::IsTrue(vmCondition.HasTarget());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetTargetSize()); // size copied from source

        vmCondition.SetOperator(TriggerOperatorType::None);
        Assert::IsFalse(vmCondition.HasTargetSize());
        Assert::IsFalse(vmCondition.HasTarget());

        vmCondition.SetTargetType(TriggerOperandType::Value);
        Assert::IsFalse(vmCondition.HasTargetSize());
        Assert::IsFalse(vmCondition.HasTarget());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetTargetSize()); // value always 32-bit

        vmCondition.SetTargetType(TriggerOperandType::Delta);
        Assert::IsFalse(vmCondition.HasTargetSize());
        Assert::IsFalse(vmCondition.HasTarget());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetTargetSize()); // size copied from source

        vmCondition.SetOperator(TriggerOperatorType::Equals);
        Assert::IsTrue(vmCondition.HasTargetSize());
        Assert::IsTrue(vmCondition.HasTarget());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetTargetSize()); // size copied from source

        vmCondition.SetTargetType(TriggerOperandType::Value);
        Assert::IsFalse(vmCondition.HasTargetSize());
        Assert::IsTrue(vmCondition.HasTarget());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetTargetSize()); // value always 32-bit

        vmCondition.SetOperator(TriggerOperatorType::None);
        Assert::IsFalse(vmCondition.HasTargetSize());
        Assert::IsFalse(vmCondition.HasTarget());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetTargetSize()); // value still 32-bit

        vmCondition.SetTargetType(TriggerOperandType::Prior);
        Assert::IsFalse(vmCondition.HasTargetSize());
        Assert::IsFalse(vmCondition.HasTarget());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetTargetSize()); // size copied from source

        vmCondition.SetOperator(TriggerOperatorType::NotEquals);
        Assert::IsTrue(vmCondition.HasTargetSize());
        Assert::IsTrue(vmCondition.HasTarget());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetTargetSize());
    }

    TEST_METHOD(TestHasTargetSizeHasTarget)
    {
        TriggerConditionViewModelHarness vmCondition;
        Assert::AreEqual(TriggerOperandType::Value, vmCondition.GetTargetType());
        Assert::IsFalse(vmCondition.HasTargetSize());

        vmCondition.SetTargetType(TriggerOperandType::Address);
        Assert::AreEqual(TriggerOperandType::Address, vmCondition.GetTargetType());
        Assert::IsTrue(vmCondition.HasTargetSize());

        vmCondition.SetTargetType(TriggerOperandType::BCD);
        Assert::AreEqual(TriggerOperandType::BCD, vmCondition.GetTargetType());
        Assert::IsTrue(vmCondition.HasTargetSize());

        vmCondition.SetTargetType(TriggerOperandType::Delta);
        Assert::AreEqual(TriggerOperandType::Delta, vmCondition.GetTargetType());
        Assert::IsTrue(vmCondition.HasTargetSize());

        vmCondition.SetTargetType(TriggerOperandType::Prior);
        Assert::AreEqual(TriggerOperandType::Prior, vmCondition.GetTargetType());
        Assert::IsTrue(vmCondition.HasTargetSize());

        vmCondition.SetTargetType(TriggerOperandType::Value);
        Assert::AreEqual(TriggerOperandType::Value, vmCondition.GetTargetType());
        Assert::IsFalse(vmCondition.HasTargetSize());
    }

    TEST_METHOD(TestHasHits)
    {
        TriggerConditionViewModelHarness vmCondition;
        Assert::AreEqual(TriggerConditionType::Standard, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());

        vmCondition.SetType(TriggerConditionType::AddAddress);
        Assert::AreEqual(TriggerConditionType::AddAddress, vmCondition.GetType());
        Assert::IsFalse(vmCondition.HasHits());

        vmCondition.SetType(TriggerConditionType::AddHits);
        Assert::AreEqual(TriggerConditionType::AddHits, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());

        vmCondition.SetType(TriggerConditionType::AddSource);
        Assert::AreEqual(TriggerConditionType::AddSource, vmCondition.GetType());
        Assert::IsFalse(vmCondition.HasHits());

        vmCondition.SetType(TriggerConditionType::AndNext);
        Assert::AreEqual(TriggerConditionType::AndNext, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());

        vmCondition.SetType(TriggerConditionType::Measured);
        Assert::AreEqual(TriggerConditionType::Measured, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());

        vmCondition.SetType(TriggerConditionType::MeasuredIf);
        Assert::AreEqual(TriggerConditionType::MeasuredIf, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());

        vmCondition.SetType(TriggerConditionType::OrNext);
        Assert::AreEqual(TriggerConditionType::OrNext, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());

        vmCondition.SetType(TriggerConditionType::PauseIf);
        Assert::AreEqual(TriggerConditionType::PauseIf, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());

        vmCondition.SetType(TriggerConditionType::ResetIf);
        Assert::AreEqual(TriggerConditionType::ResetIf, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());

        vmCondition.SetType(TriggerConditionType::SubHits);
        Assert::AreEqual(TriggerConditionType::SubHits, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());

        vmCondition.SetType(TriggerConditionType::SubSource);
        Assert::AreEqual(TriggerConditionType::SubSource, vmCondition.GetType());
        Assert::IsFalse(vmCondition.HasHits());

        vmCondition.SetType(TriggerConditionType::Trigger);
        Assert::AreEqual(TriggerConditionType::Trigger, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());
    }

    TEST_METHOD(TestSizes)
    {
        ParseAndRegenerate("0xH1234=0xH2345"); // 8-bit
        ParseAndRegenerate("0x 1234=0x 2345"); // 16-bit
        ParseAndRegenerate("0xW1234=0xW2345"); // 24-bit
        ParseAndRegenerate("0xX1234=0xX2345"); // 32-bit
        ParseAndRegenerate("0xU1234=0xU2345"); // upper4
        ParseAndRegenerate("0xL1234=0xL2345"); // lower4
        ParseAndRegenerate("0xM1234=0xM2345"); // bit0
        ParseAndRegenerate("0xN1234=0xN2345"); // bit1
        ParseAndRegenerate("0xO1234=0xO2345"); // bit2
        ParseAndRegenerate("0xP1234=0xP2345"); // bit3
        ParseAndRegenerate("0xQ1234=0xQ2345"); // bit4
        ParseAndRegenerate("0xR1234=0xR2345"); // bit5
        ParseAndRegenerate("0xS1234=0xS2345"); // bit6
        ParseAndRegenerate("0xT1234=0xT2345"); // bit7
        ParseAndRegenerate("0xK1234=0xK2345"); // bitcount
    }

    TEST_METHOD(TestOperandTypes)
    {
        ParseAndRegenerate("0xH1234=0xH1234"); // address
        ParseAndRegenerate("0xH1234=d0xH1234"); // delta
        ParseAndRegenerate("0xH1234=p0xH1234"); // prior
        ParseAndRegenerate("0xH1234=b0xH1234"); // bcd
        ParseAndRegenerate("0xH1234=1234"); // value
    }

    TEST_METHOD(TestOperators)
    {
        ParseAndRegenerate("0xH1234=5"); // equals
        ParseAndRegenerate("0xH1234!=5"); // not equals
        ParseAndRegenerate("0xH1234<5"); // less than
        ParseAndRegenerate("0xH1234<=5"); // less than or equal
        ParseAndRegenerate("0xH1234>5"); // greater than
        ParseAndRegenerate("0xH1234>=5"); // greater than or equal
        ParseAndRegenerate("A:0xH1234"); // none
        ParseAndRegenerate("A:0xH1234*5"); // multiply
        ParseAndRegenerate("A:0xH1234/5"); // divide
        ParseAndRegenerate("A:0xH1234&5"); // bitwise and
    }

    TEST_METHOD(TestConditionTypes)
    {
        ParseAndRegenerate("0xH1234=5"); // none
        ParseAndRegenerate("R:0xH1234=5"); // reset if
        ParseAndRegenerate("Z:0xH1234=5"); // reset next if
        ParseAndRegenerate("P:0xH1234=5"); // pause if
        ParseAndRegenerate("A:0xH1234"); // add source
        ParseAndRegenerate("B:0xH1234"); // sub source
        ParseAndRegenerate("C:0xH1234=5"); // add hits
        ParseAndRegenerate("D:0xH1234=5"); // sub hits
        ParseAndRegenerate("N:0xH1234=5"); // and next
        ParseAndRegenerate("O:0xH1234=5"); // or next
        ParseAndRegenerate("M:0xH1234=5"); // measured
        ParseAndRegenerate("Q:0xH1234=5"); // measured if
        ParseAndRegenerate("I:0xH1234"); // add address
        ParseAndRegenerate("T:0xH1234=5"); // trigger
    }

    TEST_METHOD(TestRequiredHits)
    {
        ParseAndRegenerate("0xH1234=5"); // none
        ParseAndRegenerate("R:0xH1234=5.100."); // 100
    }

    TEST_METHOD(TestTooltipAddress)
    {
        TriggerConditionViewModelHarness condition;
        condition.mockGameContext.SetCodeNote({ 8U }, L"This is a note.");
        condition.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        condition.Parse("0xH0008=3");

        Assert::AreEqual(std::wstring(L"0x0008\r\nThis is a note."), condition.GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L""), condition.GetTooltip(TriggerConditionViewModel::TargetValueProperty));
    }

    TEST_METHOD(TestTooltipAddressNoCodeNote)
    {
        TriggerConditionViewModelHarness condition;
        condition.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        condition.Parse("0xH0008=3");

        Assert::AreEqual(std::wstring(L"0x0008\r\n[No code note]"), condition.GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L""), condition.GetTooltip(TriggerConditionViewModel::TargetValueProperty));
    }

    TEST_METHOD(TestTooltipAddressLongCodeNote)
    {
        TriggerConditionViewModelHarness condition;
        condition.mockGameContext.SetCodeNote({ 8U }, L"A\nB\nC\nD\nE\nF\nG\nH\nI\nJ\nK\nL\nM\nO\nP\nQ\nR\nS\nT\nU\nV\nW\nX\nY\nZ");
        condition.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        condition.Parse("0xH0008=3");

        Assert::AreEqual(std::wstring(L"0x0008\r\nA\nB\nC\nD\nE\nF\nG\nH\nI\nJ\nK\nL\nM\nO\nP\nQ\nR\nS\nT\nU\n..."), condition.GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L""), condition.GetTooltip(TriggerConditionViewModel::TargetValueProperty));
    }

    TEST_METHOD(TestTooltipAddressMultiNote)
    {
        TriggerConditionViewModelHarness condition;
        condition.mockGameContext.SetCodeNote({ 8U }, L"This is a note.");
        condition.mockGameContext.SetCodeNote({ 9U }, L"This is another note.");
        condition.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        condition.Parse("0xH0008>d0xH0009");

        Assert::AreEqual(std::wstring(L"0x0008\r\nThis is a note."), condition.GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0009\r\nThis is another note."), condition.GetTooltip(TriggerConditionViewModel::TargetValueProperty));
    }

    TEST_METHOD(TestTooltipAddressMultiByteNote)
    {
        TriggerConditionViewModelHarness condition;
        condition.mockGameContext.SetCodeNote({ 8U }, L"[8 bytes] This is a note.");

        condition.Parse("0xH0008=3");
        Assert::AreEqual(std::wstring(L"0x0008\r\n[8 bytes] This is a note."), condition.GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        condition.Parse("0xH000C=3");
        Assert::AreEqual(std::wstring(L"0x000c [0x0008+4]\r\n[8 bytes] This is a note."), condition.GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipValueDecimal)
    {
        TriggerConditionViewModelHarness condition;
        condition.mockGameContext.SetCodeNote({ 99U }, L"This is a note.");
        condition.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        condition.Parse("255=99");

        Assert::AreEqual(std::wstring(L""), condition.GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L""), condition.GetTooltip(TriggerConditionViewModel::TargetValueProperty));
    }

    TEST_METHOD(TestTooltipValueHex)
    {
        TriggerConditionViewModelHarness condition;
        condition.mockGameContext.SetCodeNote({ 99U }, L"This is a note.");
        condition.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, false);
        condition.Parse("255=99");

        // in hex mode, the fields will show "0xFF" and "0x63", so display the decimal value in the tooltip
        Assert::AreEqual(std::wstring(L"255"), condition.GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"99"), condition.GetTooltip(TriggerConditionViewModel::TargetValueProperty));
    }

private:
    class IndirectAddressTriggerViewModelHarness : public TriggerViewModel
    {
    public:
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockConfiguration mockConfiguration;

        void Parse(const std::string& sInput)
        {
            mockEmulatorContext.MockMemory(m_pMemory);

            ra::data::models::CapturedTriggerHits pCapturedHits;
            InitializeFrom(sInput, pCapturedHits);
        }

        void SetMemory(size_t nAddress, unsigned char nValue)
        {
            m_pMemory.at(nAddress) = nValue;
        }

    private:
        std::array<unsigned char, 10> m_pMemory = { 0,1,2,3,4,5,6,7,8,9 };
    };

public:
    TEST_METHOD(TestTooltipIndirectAddressNoCodeNote)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.Parse("I:0xH0001_0xH0002=3");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);

        const auto* pCondition = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::IsTrue(pCondition->IsIndirect());

        // $0001 = 1, 1+2 = $0003
        Assert::AreEqual(std::wstring(L"0x0003 (indirect)\r\n[No code note]"), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 3, 3+2 = $0005
        vmTrigger.SetMemory({ 1 }, 3);
        Assert::AreEqual(std::wstring(L"0x0005 (indirect)\r\n[No code note]"), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipIndirectAddressCodeNote)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.Parse("I:0xH0001_0xH0002=3");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        vmTrigger.mockGameContext.SetCodeNote({ 3U }, L"This is a note.");

        const auto* pCondition = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::IsTrue(pCondition->IsIndirect());

        // $0001 = 1, 1+2 = $0003
        Assert::AreEqual(std::wstring(L"0x0003 (indirect)\r\nThis is a note."), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 3, 3+2 = $0005
        vmTrigger.SetMemory({ 1 }, 3);
        Assert::AreEqual(std::wstring(L"0x0005 (indirect)\r\n[No code note]"), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipIndirectAddressMultiNote)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.Parse("I:0xH0001_0xH0002=0xH0004");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        vmTrigger.mockGameContext.SetCodeNote({ 3U }, L"This is a note.");
        vmTrigger.mockGameContext.SetCodeNote({ 5U }, L"This is another note.");

        const auto* pCondition = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::IsTrue(pCondition->IsIndirect());

        // $0001 = 1, 1+2 = $0003, 1+4 = $0005
        Assert::AreEqual(std::wstring(L"0x0003 (indirect)\r\nThis is a note."), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0005 (indirect)\r\nThis is another note."), pCondition->GetTooltip(TriggerConditionViewModel::TargetValueProperty));

        // $0001 = 3, 3+2 = $0005, 3+4 = $0007
        vmTrigger.SetMemory({ 1 }, 3);
        Assert::AreEqual(std::wstring(L"0x0005 (indirect)\r\nThis is another note."), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0007 (indirect)\r\n[No code note]"), pCondition->GetTooltip(TriggerConditionViewModel::TargetValueProperty));
    }

    TEST_METHOD(TestTooltipIndirectAddressMultiByteNote)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.Parse("I:0xH0001_0xH0002=0xH0004");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        vmTrigger.mockGameContext.SetCodeNote({ 4U }, L"[8 bytes] This is a note.");

        const auto* pCondition = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::IsTrue(pCondition->IsIndirect());

        // $0001 = 1, 1+2 = $0003, 1+4 = $0005
        Assert::AreEqual(std::wstring(L"0x0003 (indirect)\r\n[No code note]"), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0005 [0x0004+1] (indirect)\r\n[8 bytes] This is a note."), pCondition->GetTooltip(TriggerConditionViewModel::TargetValueProperty));
    }

    TEST_METHOD(TestTooltipIndirectAddressMultiplyNoCodeNote)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.Parse("I:0xH0001*3_0xH0002=3");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);

        const auto* pCondition = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::IsTrue(pCondition->IsIndirect());

        // $0001 = 1, 1*3+2 = $0005
        Assert::AreEqual(std::wstring(L"0x0005 (indirect)\r\n[No code note]"), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 3, 3*3+2 = $000B
        vmTrigger.SetMemory({ 1 }, 3);
        Assert::AreEqual(std::wstring(L"0x000b (indirect)\r\n[No code note]"), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipDoubleIndirectAddress)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.Parse("I:0xH0001_I:0xH0002_0xH0003=4");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);

        const auto* pCondition1 = vmTrigger.Conditions().GetItemAt(0);
        Expects(pCondition1 != nullptr);
        const auto* pCondition2 = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition2 != nullptr);
        const auto* pCondition3 = vmTrigger.Conditions().GetItemAt(2);
        Expects(pCondition3 != nullptr);

        Assert::IsFalse(pCondition1->IsIndirect());
        Assert::IsTrue(pCondition2->IsIndirect());
        Assert::IsTrue(pCondition3->IsIndirect());

        // $0001 = 1, 1+2 = $0003, $0003 = 3, 3+3 = $0006
        Assert::AreEqual(std::wstring(L"0x0001\r\n[No code note]"), pCondition1->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0003 (indirect)\r\n[No code note]"), pCondition2->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0006 (indirect)\r\n[No code note]"), pCondition3->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 3, 3+2 = $0005, $0005 = 5, 5+3 = $0008
        vmTrigger.SetMemory({ 1 }, 3);
        Assert::AreEqual(std::wstring(L"0x0001\r\n[No code note]"), pCondition1->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0005 (indirect)\r\n[No code note]"), pCondition2->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0008 (indirect)\r\n[No code note]"), pCondition3->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipDoubleIndirectAddressExternal)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.Parse("I:0xH0001_I:0xH0002_0xH0003=4");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);

        // Neat trick! Get the trigger managed by the viewmodel, and reinitialize the viewmodel using it.
        // It'll rebuild itself as if the trigger was external (i.e. from the runtime), but without destroying it.
        // This leverages the fact that InitializeFrom only clears out the pointer, not the buffer.
        auto* pTrigger = vmTrigger.GetTriggerFromString();
        Expects(pTrigger != nullptr);
        vmTrigger.InitializeFrom(*pTrigger);

        // manually update the memrefs - GetIndirectAddress won't if the trigger is not internal to the viewmodel.
        pTrigger->memrefs->value.value = 1;

        const auto* pCondition1 = vmTrigger.Conditions().GetItemAt(0);
        Expects(pCondition1 != nullptr);
        const auto* pCondition2 = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition2 != nullptr);
        const auto* pCondition3 = vmTrigger.Conditions().GetItemAt(2);
        Expects(pCondition3 != nullptr);

        Assert::IsFalse(pCondition1->IsIndirect());
        Assert::IsTrue(pCondition2->IsIndirect());
        Assert::IsTrue(pCondition3->IsIndirect());

        // $0001 = 1, 1+2 = $0003, $0003 = 3, 3+3 = $0006
        Assert::AreEqual(std::wstring(L"0x0001\r\n[No code note]"), pCondition1->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0003 (indirect)\r\n[No code note]"), pCondition2->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0006 (indirect)\r\n[No code note]"), pCondition3->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // Calling GetTooltip on a viewmodel attached to an external trigger should not evaluate the memrefs.
        // External memrefs should only be updated by the runtime to ensure delta values are correct.
        vmTrigger.SetMemory({ 1 }, 3);
        Assert::AreEqual(std::wstring(L"0x0001\r\n[No code note]"), pCondition1->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0003 (indirect)\r\n[No code note]"), pCondition2->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0006 (indirect)\r\n[No code note]"), pCondition3->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // memref should not have been updated
        Assert::AreEqual(1U, pTrigger->memrefs->value.value);

        // Manually update the memref as if rc_runtime_do_frame were called, then the tooltips will update
        pTrigger->memrefs->value.value = 3;

        // $0001 = 3, 3+2 = $0005, $0005 = 5, 5+3 = $0008
        Assert::AreEqual(std::wstring(L"0x0001\r\n[No code note]"), pCondition1->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0005 (indirect)\r\n[No code note]"), pCondition2->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0008 (indirect)\r\n[No code note]"), pCondition3->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestIsModifying)
    {
        TriggerConditionViewModelHarness condition;

        condition.SetType(TriggerConditionType::AddAddress);
        Assert::IsTrue(condition.IsModifying());

        condition.SetType(TriggerConditionType::AddHits);
        Assert::IsFalse(condition.IsModifying());

        condition.SetType(TriggerConditionType::AddSource);
        Assert::IsTrue(condition.IsModifying());

        condition.SetType(TriggerConditionType::AndNext);
        Assert::IsFalse(condition.IsModifying());

        condition.SetType(TriggerConditionType::Measured);
        Assert::IsFalse(condition.IsModifying());

        condition.SetType(TriggerConditionType::MeasuredIf);
        Assert::IsFalse(condition.IsModifying());

        condition.SetType(TriggerConditionType::OrNext);
        Assert::IsFalse(condition.IsModifying());

        condition.SetType(TriggerConditionType::PauseIf);
        Assert::IsFalse(condition.IsModifying());

        condition.SetType(TriggerConditionType::ResetIf);
        Assert::IsFalse(condition.IsModifying());

        condition.SetType(TriggerConditionType::Standard);
        Assert::IsFalse(condition.IsModifying());

        condition.SetType(TriggerConditionType::SubHits);
        Assert::IsFalse(condition.IsModifying());

        condition.SetType(TriggerConditionType::SubSource);
        Assert::IsTrue(condition.IsModifying());

        condition.SetType(TriggerConditionType::Trigger);
        Assert::IsFalse(condition.IsModifying());
    }

    TEST_METHOD(TestSwitchBetweenModifyingAndNonModifying)
    {
        TriggerConditionViewModelHarness condition;
        condition.SetSourceValue(0x1234U);
        condition.SetOperator(TriggerOperatorType::NotEquals);
        condition.SetTargetValue(8);
        Assert::AreEqual(std::string("0xH1234!=8"), condition.Serialize());
        Assert::IsFalse(condition.IsModifying());

        // change to modifying switches operator to None. the 8 is unmodified, but ignored
        condition.SetType(TriggerConditionType::AddSource);
        Assert::AreEqual(TriggerOperatorType::None, condition.GetOperator());
        Assert::AreEqual(std::string("A:0xH1234"), condition.Serialize());
        Assert::IsTrue(condition.IsModifying());

        // changing operator "restores" the 8
        condition.SetOperator(TriggerOperatorType::Multiply);
        Assert::AreEqual(std::string("A:0xH1234*8"), condition.Serialize());

        // change to non-modifying switches operator to Equals
        condition.SetType(TriggerConditionType::ResetIf);
        Assert::AreEqual(TriggerOperatorType::Equals, condition.GetOperator());
        Assert::AreEqual(std::string("R:0xH1234=8"), condition.Serialize());
        Assert::IsFalse(condition.IsModifying());

        // change to modifying switches operator to None. the 8 is unmodified, but ignored
        condition.SetType(TriggerConditionType::AddAddress);
        Assert::AreEqual(TriggerOperatorType::None, condition.GetOperator());
        Assert::AreEqual(std::string("I:0xH1234"), condition.Serialize());
        Assert::IsTrue(condition.IsModifying());

        // change to non-modifying switches operator to Equals
        condition.SetType(TriggerConditionType::Standard);
        Assert::AreEqual(TriggerOperatorType::Equals, condition.GetOperator());
        Assert::AreEqual(std::string("0xH1234=8"), condition.Serialize());
        Assert::IsFalse(condition.IsModifying());
    }

    TEST_METHOD(TestIsComparisonVisible)
    {
        TriggerConditionViewModelHarness condition;

        condition.SetType(TriggerConditionType::Standard);
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::Equals)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::NotEquals)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::LessThan)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::LessThanOrEqual)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::GreaterThan)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::GreaterThanOrEqual)));
        Assert::IsFalse(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::None)));
        Assert::IsFalse(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::Multiply)));
        Assert::IsFalse(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::Divide)));
        Assert::IsFalse(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::BitwiseAnd)));

        condition.SetType(TriggerConditionType::AddAddress);
        Assert::IsFalse(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::Equals)));
        Assert::IsFalse(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::NotEquals)));
        Assert::IsFalse(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::LessThan)));
        Assert::IsFalse(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::LessThanOrEqual)));
        Assert::IsFalse(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::GreaterThan)));
        Assert::IsFalse(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::GreaterThanOrEqual)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::None)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::Multiply)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::Divide)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::BitwiseAnd)));
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
