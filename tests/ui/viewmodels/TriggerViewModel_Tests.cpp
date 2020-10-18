#include "CppUnitTest.h"

#include "ui\viewmodels\TriggerViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(TriggerViewModel_Tests)
{
private:
    void Parse(TriggerViewModel& vmTrigger, const std::string& sInput)
    {
        vmTrigger.InitializeFrom(sInput);
    }

    void ParseAndRegenerate(const std::string& sInput)
    {
        TriggerViewModel vmTrigger;
        Parse(vmTrigger, sInput);

        const std::string sOutput = vmTrigger.Serialize();
        Assert::AreEqual(sInput, sOutput);
    }

    class TriggerMonitor : ViewModelBase::NotifyTarget
    {
    public:
        TriggerMonitor(TriggerViewModel& vmTrigger) noexcept
        {
            vmTrigger.AddNotifyTarget(*this);
            m_vmTrigger = &vmTrigger;
        }

        std::string sNewTrigger;

    protected:
        void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override
        {
            if (args.Property == TriggerViewModel::VersionProperty)
                sNewTrigger = m_vmTrigger->Serialize();
        }

        TriggerViewModel* m_vmTrigger;
    };

public:
    TEST_METHOD(TestInitialState)
    {
        TriggerViewModel vmTrigger;
        Assert::AreEqual(-1, vmTrigger.GetSelectedGroupIndex());
        Assert::IsFalse(vmTrigger.IsPauseOnReset());
        Assert::IsFalse(vmTrigger.IsPauseOnTrigger());
        Assert::AreEqual({ 0U }, vmTrigger.Groups().Count());
        Assert::AreEqual(std::string(), vmTrigger.Serialize());

        Assert::AreEqual({ 12U }, vmTrigger.ConditionTypes().Count());
        Assert::AreEqual((int)TriggerConditionType::Standard, vmTrigger.ConditionTypes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L""), vmTrigger.ConditionTypes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::PauseIf, vmTrigger.ConditionTypes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Pause If"), vmTrigger.ConditionTypes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::ResetIf, vmTrigger.ConditionTypes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Reset If"), vmTrigger.ConditionTypes().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::AddSource, vmTrigger.ConditionTypes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Add Source"), vmTrigger.ConditionTypes().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::SubSource, vmTrigger.ConditionTypes().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L"Sub Source"), vmTrigger.ConditionTypes().GetItemAt(4)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::AddHits, vmTrigger.ConditionTypes().GetItemAt(5)->GetId());
        Assert::AreEqual(std::wstring(L"Add Hits"), vmTrigger.ConditionTypes().GetItemAt(5)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::AddAddress, vmTrigger.ConditionTypes().GetItemAt(6)->GetId());
        Assert::AreEqual(std::wstring(L"Add Address"), vmTrigger.ConditionTypes().GetItemAt(6)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::AndNext, vmTrigger.ConditionTypes().GetItemAt(7)->GetId());
        Assert::AreEqual(std::wstring(L"And Next"), vmTrigger.ConditionTypes().GetItemAt(7)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::OrNext, vmTrigger.ConditionTypes().GetItemAt(8)->GetId());
        Assert::AreEqual(std::wstring(L"Or Next"), vmTrigger.ConditionTypes().GetItemAt(8)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::Measured, vmTrigger.ConditionTypes().GetItemAt(9)->GetId());
        Assert::AreEqual(std::wstring(L"Measured"), vmTrigger.ConditionTypes().GetItemAt(9)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::MeasuredIf, vmTrigger.ConditionTypes().GetItemAt(10)->GetId());
        Assert::AreEqual(std::wstring(L"Measured If"), vmTrigger.ConditionTypes().GetItemAt(10)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::Trigger, vmTrigger.ConditionTypes().GetItemAt(11)->GetId());
        Assert::AreEqual(std::wstring(L"Trigger"), vmTrigger.ConditionTypes().GetItemAt(11)->GetLabel());

        Assert::AreEqual({ 5U }, vmTrigger.OperandTypes().Count());
        Assert::AreEqual((int)TriggerOperandType::Address, vmTrigger.OperandTypes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"Mem"), vmTrigger.OperandTypes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)TriggerOperandType::Value, vmTrigger.OperandTypes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Value"), vmTrigger.OperandTypes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)TriggerOperandType::Delta, vmTrigger.OperandTypes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Delta"), vmTrigger.OperandTypes().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)TriggerOperandType::Prior, vmTrigger.OperandTypes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Prior"), vmTrigger.OperandTypes().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)TriggerOperandType::BCD, vmTrigger.OperandTypes().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L"BCD"), vmTrigger.OperandTypes().GetItemAt(4)->GetLabel());

        Assert::AreEqual({ 15U }, vmTrigger.OperandSizes().Count());
        Assert::AreEqual((int)MemSize::Bit_0, vmTrigger.OperandSizes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"Bit0"), vmTrigger.OperandSizes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)MemSize::Bit_1, vmTrigger.OperandSizes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Bit1"), vmTrigger.OperandSizes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)MemSize::Bit_2, vmTrigger.OperandSizes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Bit2"), vmTrigger.OperandSizes().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)MemSize::Bit_3, vmTrigger.OperandSizes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Bit3"), vmTrigger.OperandSizes().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)MemSize::Bit_4, vmTrigger.OperandSizes().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L"Bit4"), vmTrigger.OperandSizes().GetItemAt(4)->GetLabel());
        Assert::AreEqual((int)MemSize::Bit_5, vmTrigger.OperandSizes().GetItemAt(5)->GetId());
        Assert::AreEqual(std::wstring(L"Bit5"), vmTrigger.OperandSizes().GetItemAt(5)->GetLabel());
        Assert::AreEqual((int)MemSize::Bit_6, vmTrigger.OperandSizes().GetItemAt(6)->GetId());
        Assert::AreEqual(std::wstring(L"Bit6"), vmTrigger.OperandSizes().GetItemAt(6)->GetLabel());
        Assert::AreEqual((int)MemSize::Bit_7, vmTrigger.OperandSizes().GetItemAt(7)->GetId());
        Assert::AreEqual(std::wstring(L"Bit7"), vmTrigger.OperandSizes().GetItemAt(7)->GetLabel());
        Assert::AreEqual((int)MemSize::Nibble_Lower, vmTrigger.OperandSizes().GetItemAt(8)->GetId());
        Assert::AreEqual(std::wstring(L"Lower4"), vmTrigger.OperandSizes().GetItemAt(8)->GetLabel());
        Assert::AreEqual((int)MemSize::Nibble_Upper, vmTrigger.OperandSizes().GetItemAt(9)->GetId());
        Assert::AreEqual(std::wstring(L"Upper4"), vmTrigger.OperandSizes().GetItemAt(9)->GetLabel());
        Assert::AreEqual((int)MemSize::EightBit, vmTrigger.OperandSizes().GetItemAt(10)->GetId());
        Assert::AreEqual(std::wstring(L"8-bit"), vmTrigger.OperandSizes().GetItemAt(10)->GetLabel());
        Assert::AreEqual((int)MemSize::SixteenBit, vmTrigger.OperandSizes().GetItemAt(11)->GetId());
        Assert::AreEqual(std::wstring(L"16-bit"), vmTrigger.OperandSizes().GetItemAt(11)->GetLabel());
        Assert::AreEqual((int)MemSize::TwentyFourBit, vmTrigger.OperandSizes().GetItemAt(12)->GetId());
        Assert::AreEqual(std::wstring(L"24-bit"), vmTrigger.OperandSizes().GetItemAt(12)->GetLabel());
        Assert::AreEqual((int)MemSize::ThirtyTwoBit, vmTrigger.OperandSizes().GetItemAt(13)->GetId());
        Assert::AreEqual(std::wstring(L"32-bit"), vmTrigger.OperandSizes().GetItemAt(13)->GetLabel());
        Assert::AreEqual((int)MemSize::BitCount, vmTrigger.OperandSizes().GetItemAt(14)->GetId());
        Assert::AreEqual(std::wstring(L"BitCount"), vmTrigger.OperandSizes().GetItemAt(14)->GetLabel());

        Assert::AreEqual({ 10U }, vmTrigger.OperatorTypes().Count());
        Assert::AreEqual((int)TriggerOperatorType::Equals, vmTrigger.OperatorTypes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L"="), vmTrigger.OperatorTypes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)TriggerOperatorType::LessThan, vmTrigger.OperatorTypes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"<"), vmTrigger.OperatorTypes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)TriggerOperatorType::LessThanOrEqual, vmTrigger.OperatorTypes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"<="), vmTrigger.OperatorTypes().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)TriggerOperatorType::GreaterThan, vmTrigger.OperatorTypes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L">"), vmTrigger.OperatorTypes().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)TriggerOperatorType::GreaterThanOrEqual, vmTrigger.OperatorTypes().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L">="), vmTrigger.OperatorTypes().GetItemAt(4)->GetLabel());
        Assert::AreEqual((int)TriggerOperatorType::NotEquals, vmTrigger.OperatorTypes().GetItemAt(5)->GetId());
        Assert::AreEqual(std::wstring(L"!="), vmTrigger.OperatorTypes().GetItemAt(5)->GetLabel());
        Assert::AreEqual((int)TriggerOperatorType::None, vmTrigger.OperatorTypes().GetItemAt(6)->GetId());
        Assert::AreEqual(std::wstring(L""), vmTrigger.OperatorTypes().GetItemAt(6)->GetLabel());
        Assert::AreEqual((int)TriggerOperatorType::Multiply, vmTrigger.OperatorTypes().GetItemAt(7)->GetId());
        Assert::AreEqual(std::wstring(L"*"), vmTrigger.OperatorTypes().GetItemAt(7)->GetLabel());
        Assert::AreEqual((int)TriggerOperatorType::Divide, vmTrigger.OperatorTypes().GetItemAt(8)->GetId());
        Assert::AreEqual(std::wstring(L"/"), vmTrigger.OperatorTypes().GetItemAt(8)->GetLabel());
        Assert::AreEqual((int)TriggerOperatorType::BitwiseAnd, vmTrigger.OperatorTypes().GetItemAt(9)->GetId());
        Assert::AreEqual(std::wstring(L"&"), vmTrigger.OperatorTypes().GetItemAt(9)->GetLabel());
    }

    TEST_METHOD(TestParseAndRegenerateCoreOnly)
    {
        ParseAndRegenerate("0xH1234=0xH2345"); // one condition
        ParseAndRegenerate("0xH1234=0xH2345_0xX5555=1.3._R:0x face=678"); // several conditions
        ParseAndRegenerate("I:0x 1234_A:0xH2345_0xH7777=345"); // addsource/addaddress chain
    }

    TEST_METHOD(TestParseAndRegenerateWithAlts)
    {
        ParseAndRegenerate("0xH1234=0xH2345S0xX5555=1"); // one alt
        ParseAndRegenerate("0xH1234=0xH2345S0xX5555=1.3.SR:0x face=678S0xL3333!=d0xL3333"); // several alts
        ParseAndRegenerate("S0xX5555=1.3.SR:0x face=678S0xL3333!=d0xL3333"); // only alts
        ParseAndRegenerate("0xH1234=0xH2345SI:0x 1234_A:0xH2345_0xH7777=345"); // addsource/addaddress chain
    }

    TEST_METHOD(TestTriggerUpdateConditionModified)
    {
        TriggerViewModel vmTrigger;
        Parse(vmTrigger, "0xH1234=0xH2345S0xX5555=1");

        TriggerMonitor pMonitor(vmTrigger);
        vmTrigger.SetSelectedGroupIndex(0);
        vmTrigger.Conditions().GetItemAt(0)->SetOperator(ra::ui::viewmodels::TriggerOperatorType::LessThan);
        Assert::AreEqual(std::string("0xH1234<0xH2345S0xX5555=1"), pMonitor.sNewTrigger);

        vmTrigger.SetSelectedGroupIndex(1);
        vmTrigger.Conditions().GetItemAt(0)->SetTargetValue(9U);
        Assert::AreEqual(std::string("0xH1234<0xH2345S0xX5555=9"), pMonitor.sNewTrigger);
    }

    TEST_METHOD(TestTriggerUpdateConditionsAddedRemoved)
    {
        TriggerViewModel vmTrigger;
        Parse(vmTrigger, "0xH1234=0xH2345S0xX5555=1");

        TriggerMonitor pMonitor(vmTrigger);
        vmTrigger.SetSelectedGroupIndex(0);
        vmTrigger.Conditions().Add();
        Assert::AreEqual(std::string("0xH1234=0xH2345_0xH0000=0S0xX5555=1"), pMonitor.sNewTrigger);

        vmTrigger.Conditions().RemoveAt(0);
        Assert::AreEqual(std::string("0xH0000=0S0xX5555=1"), pMonitor.sNewTrigger);

        vmTrigger.SetSelectedGroupIndex(1);
        vmTrigger.Conditions().Add();
        Assert::AreEqual(std::string("0xH0000=0S0xX5555=1_0xH0000=0"), pMonitor.sNewTrigger);

        vmTrigger.Conditions().RemoveAt(0);
        Assert::AreEqual(std::string("0xH0000=0S0xH0000=0"), pMonitor.sNewTrigger);
    }

    TEST_METHOD(TestTriggerUpdateConditionsAddedModified)
    {
        TriggerViewModel vmTrigger;
        Parse(vmTrigger, "0xH1234=0xH2345S0xX5555=1");

        TriggerMonitor pMonitor(vmTrigger);
        vmTrigger.SetSelectedGroupIndex(0);
        vmTrigger.Conditions().Add();
        Assert::AreEqual(std::string("0xH1234=0xH2345_0xH0000=0S0xX5555=1"), pMonitor.sNewTrigger);

        vmTrigger.Conditions().GetItemAt(1)->SetSourceValue(1234U);
        Assert::AreEqual(std::string("0xH1234=0xH2345_0xH04d2=0S0xX5555=1"), pMonitor.sNewTrigger);
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
