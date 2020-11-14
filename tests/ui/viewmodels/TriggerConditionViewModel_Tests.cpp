#include "CppUnitTest.h"

#include "ui\viewmodels\TriggerConditionViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockGameContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;


namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

template<>
std::wstring ToString<ra::ui::viewmodels::TriggerConditionType>(
    const ra::ui::viewmodels::TriggerConditionType& nConditionType)
{
    switch (nConditionType)
    {
        case ra::ui::viewmodels::TriggerConditionType::Standard:
            return L"Standard";
        case ra::ui::viewmodels::TriggerConditionType::PauseIf:
            return L"PauseIf";
        case ra::ui::viewmodels::TriggerConditionType::ResetIf:
            return L"ResetIf";
        case ra::ui::viewmodels::TriggerConditionType::AddSource:
            return L"AddSource";
        case ra::ui::viewmodels::TriggerConditionType::SubSource:
            return L"SubSource";
        case ra::ui::viewmodels::TriggerConditionType::AddHits:
            return L"AddHits";
        case ra::ui::viewmodels::TriggerConditionType::AndNext:
            return L"AndNext";
        case ra::ui::viewmodels::TriggerConditionType::Measured:
            return L"Measured";
        case ra::ui::viewmodels::TriggerConditionType::AddAddress:
            return L"AddAddress";
        case ra::ui::viewmodels::TriggerConditionType::OrNext:
            return L"OrNext";
        case ra::ui::viewmodels::TriggerConditionType::Trigger:
            return L"Trigger";
        case ra::ui::viewmodels::TriggerConditionType::MeasuredIf:
            return L"MeasuredIf";
        default:
            return std::to_wstring(static_cast<int>(nConditionType));
    }
}

template<>
std::wstring ToString<ra::ui::viewmodels::TriggerOperandType>(
    const ra::ui::viewmodels::TriggerOperandType& nOperandType)
{
    switch (nOperandType)
    {
        case ra::ui::viewmodels::TriggerOperandType::Address:
            return L"Address";
        case ra::ui::viewmodels::TriggerOperandType::Delta:
            return L"Delta";
        case ra::ui::viewmodels::TriggerOperandType::Value:
            return L"Value";
        case ra::ui::viewmodels::TriggerOperandType::Prior:
            return L"Prior";
        case ra::ui::viewmodels::TriggerOperandType::BCD:
            return L"BCD";
        default:
            return std::to_wstring(static_cast<int>(nOperandType));
    }
}

template<>
std::wstring ToString<ra::ui::viewmodels::TriggerOperatorType>(
    const ra::ui::viewmodels::TriggerOperatorType& nConditionType)
{
    switch (nConditionType)
    {
        case ra::ui::viewmodels::TriggerOperatorType::Equals:
            return L"Equals";
        case ra::ui::viewmodels::TriggerOperatorType::LessThan:
            return L"LessThan";
        case ra::ui::viewmodels::TriggerOperatorType::LessThanOrEqual:
            return L"LessThanOrEqual";
        case ra::ui::viewmodels::TriggerOperatorType::GreaterThan:
            return L"GreaterThan";
        case ra::ui::viewmodels::TriggerOperatorType::GreaterThanOrEqual:
            return L"GreaterThanOrEqual";
        case ra::ui::viewmodels::TriggerOperatorType::NotEquals:
            return L"NotEquals";
        case ra::ui::viewmodels::TriggerOperatorType::None:
            return L"None";
        case ra::ui::viewmodels::TriggerOperatorType::Multiply:
            return L"Multiply";
        case ra::ui::viewmodels::TriggerOperatorType::Divide:
            return L"Divide";
        case ra::ui::viewmodels::TriggerOperatorType::BitwiseAnd:
            return L"BitwiseAnd";
        default:
            return std::to_wstring(static_cast<int>(nConditionType));
    }
}

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

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
        ra::data::mocks::MockGameContext mockGameContext;

        void Parse(const std::string& sInput)
        {
            const auto nSize = rc_trigger_size(sInput.c_str());
            sBuffer.resize(nSize);

            const rc_trigger_t* pTrigger = rc_parse_trigger(sBuffer.data(), sInput.c_str(), nullptr, 0);
            Assert::IsNotNull(pTrigger);
            Ensures(pTrigger != nullptr);

            TriggerConditionViewModel::InitializeFrom(*pTrigger->requirement->conditions);
        }

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
        TriggerConditionViewModel vmCondition;
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
        ParseAndRegenerate("0xH1234=0xH2345"); // bitcount
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
        ParseAndRegenerate("A:0xH1234*5"); // multiply
        ParseAndRegenerate("A:0xH1234/5"); // divide
        ParseAndRegenerate("A:0xH1234&5"); // bitwise and
    }

    TEST_METHOD(TestConditionTypes)
    {
        ParseAndRegenerate("0xH1234=5"); // none
        ParseAndRegenerate("R:0xH1234=5"); // reset if
        ParseAndRegenerate("P:0xH1234=5"); // pause if
        ParseAndRegenerate("A:0xH1234"); // add source
        ParseAndRegenerate("B:0xH1234"); // sub source
        ParseAndRegenerate("C:0xH1234=5"); // add hits
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
        condition.Parse("0xH0008=3");

        Assert::AreEqual(std::wstring(L"0x0008\r\nThis is a note."), condition.GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L""), condition.GetTooltip(TriggerConditionViewModel::TargetValueProperty));
    }

    TEST_METHOD(TestTooltipAddressNoCodeNote)
    {
        TriggerConditionViewModelHarness condition;
        condition.Parse("0xH0008=3");

        Assert::AreEqual(std::wstring(L"0x0008\r\n[No code note]"), condition.GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L""), condition.GetTooltip(TriggerConditionViewModel::TargetValueProperty));
    }

    TEST_METHOD(TestTooltipAddressLongCodeNote)
    {
        TriggerConditionViewModelHarness condition;
        condition.mockGameContext.SetCodeNote({ 8U }, L"A\nB\nC\nD\nE\nF\nG\nH\nI\nJ\nK\nL\nM\nO\nP\nQ\nR\nS\nT\nU\nV\nW\nX\nY\nZ");
        condition.Parse("0xH0008=3");

        Assert::AreEqual(std::wstring(L"0x0008\r\nA\nB\nC\nD\nE\nF\nG\nH\nI\nJ\nK\nL\nM\nO\nP\nQ\nR\nS\nT\nU\n..."), condition.GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L""), condition.GetTooltip(TriggerConditionViewModel::TargetValueProperty));
    }

    TEST_METHOD(TestTooltipAddressMultiNote)
    {
        TriggerConditionViewModelHarness condition;
        condition.mockGameContext.SetCodeNote({ 8U }, L"This is a note.");
        condition.mockGameContext.SetCodeNote({ 9U }, L"This is another note.");
        condition.Parse("0xH0008>d0xH0009");

        Assert::AreEqual(std::wstring(L"0x0008\r\nThis is a note."), condition.GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0009\r\nThis is another note."), condition.GetTooltip(TriggerConditionViewModel::TargetValueProperty));
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
