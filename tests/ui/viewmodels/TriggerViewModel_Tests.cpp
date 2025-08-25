#include "CppUnitTest.h"

#include "ui\viewmodels\TriggerViewModel.hh"

#include "tests\ui\UIAsserts.hh"
#include "tests\mocks\MockClipboard.hh"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockDesktop.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\mocks\MockWindowManager.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(TriggerViewModel_Tests)
{
private:
    class TriggerViewModelHarness : public TriggerViewModel
    {
    public:
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::services::mocks::MockClipboard mockClipboard;
        ra::services::mocks::MockConfiguration mockConfiguration;
        ra::ui::mocks::MockDesktop mockDesktop;
        ra::ui::viewmodels::mocks::MockWindowManager mockWindowManager;

        void InitializeMemory(uint8_t* pMemory, size_t nMemorySize)
        {
            // make sure to hook up the memory inspector first or we won't be able to set an address
            mockWindowManager.MemoryInspector.Viewer().InitializeNotifyTargets();
            mockEmulatorContext.MockMemory(pMemory, nMemorySize);
        }

        const std::set<unsigned int>& GetSelectedItems() const
        {
            return m_vSelectedConditions;
        }

        int GetScrollOffset() const
        {
            return GetValue(ScrollOffsetProperty);
        }

        int GetEnsureVisibleGroupIndex() const
        {
            return GetValue(EnsureVisibleGroupIndexProperty);
        }

        int GetVersion() const
        {
            return GetValue(VersionProperty);
        }

        std::wstring GetHitChainTooltip(gsl::index nIndex)
        {
            std::wstring sTooltip;
            BuildHitChainTooltip(sTooltip, Conditions(), nIndex);
            return sTooltip;
        }
    };

    void Parse(TriggerViewModel& vmTrigger, const std::string& sInput)
    {
        ra::data::models::CapturedTriggerHits pCapturedHits;
        vmTrigger.InitializeFrom(sInput, pCapturedHits);
    }

    void ParseAndRegenerate(const std::string& sInput)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, sInput);

        const std::string sOutput = vmTrigger.Serialize();
        Assert::AreEqual(sInput, sOutput);
        Assert::IsFalse(vmTrigger.IsValue());
    }

    void ParseAndRegenerateValue(const std::string& sInput)
    {
        TriggerViewModelHarness vmTrigger;
        vmTrigger.SetIsValue(true);
        Parse(vmTrigger, sInput);

        const std::string sOutput = vmTrigger.Serialize();
        Assert::AreEqual(sInput, sOutput);
        Assert::IsTrue(vmTrigger.IsValue());
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
        Assert::AreEqual({ 0U }, vmTrigger.Groups().Count());
        Assert::AreEqual(std::string(), vmTrigger.Serialize());

        Assert::AreEqual({ 15U }, vmTrigger.ConditionTypes().Count());
        Assert::AreEqual((int)TriggerConditionType::Standard, vmTrigger.ConditionTypes().GetItemAt(0)->GetId());
        Assert::AreEqual(std::wstring(L""), vmTrigger.ConditionTypes().GetItemAt(0)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::PauseIf, vmTrigger.ConditionTypes().GetItemAt(1)->GetId());
        Assert::AreEqual(std::wstring(L"Pause If"), vmTrigger.ConditionTypes().GetItemAt(1)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::ResetIf, vmTrigger.ConditionTypes().GetItemAt(2)->GetId());
        Assert::AreEqual(std::wstring(L"Reset If"), vmTrigger.ConditionTypes().GetItemAt(2)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::ResetNextIf, vmTrigger.ConditionTypes().GetItemAt(3)->GetId());
        Assert::AreEqual(std::wstring(L"Reset Next If"), vmTrigger.ConditionTypes().GetItemAt(3)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::AddSource, vmTrigger.ConditionTypes().GetItemAt(4)->GetId());
        Assert::AreEqual(std::wstring(L"Add Source"), vmTrigger.ConditionTypes().GetItemAt(4)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::SubSource, vmTrigger.ConditionTypes().GetItemAt(5)->GetId());
        Assert::AreEqual(std::wstring(L"Sub Source"), vmTrigger.ConditionTypes().GetItemAt(5)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::AddHits, vmTrigger.ConditionTypes().GetItemAt(6)->GetId());
        Assert::AreEqual(std::wstring(L"Add Hits"), vmTrigger.ConditionTypes().GetItemAt(6)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::SubHits, vmTrigger.ConditionTypes().GetItemAt(7)->GetId());
        Assert::AreEqual(std::wstring(L"Sub Hits"), vmTrigger.ConditionTypes().GetItemAt(7)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::AddAddress, vmTrigger.ConditionTypes().GetItemAt(8)->GetId());
        Assert::AreEqual(std::wstring(L"Add Address"), vmTrigger.ConditionTypes().GetItemAt(8)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::AndNext, vmTrigger.ConditionTypes().GetItemAt(9)->GetId());
        Assert::AreEqual(std::wstring(L"And Next"), vmTrigger.ConditionTypes().GetItemAt(9)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::OrNext, vmTrigger.ConditionTypes().GetItemAt(10)->GetId());
        Assert::AreEqual(std::wstring(L"Or Next"), vmTrigger.ConditionTypes().GetItemAt(10)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::Measured, vmTrigger.ConditionTypes().GetItemAt(11)->GetId());
        Assert::AreEqual(std::wstring(L"Measured"), vmTrigger.ConditionTypes().GetItemAt(11)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::MeasuredIf, vmTrigger.ConditionTypes().GetItemAt(12)->GetId());
        Assert::AreEqual(std::wstring(L"Measured If"), vmTrigger.ConditionTypes().GetItemAt(12)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::Trigger, vmTrigger.ConditionTypes().GetItemAt(13)->GetId());
        Assert::AreEqual(std::wstring(L"Trigger"), vmTrigger.ConditionTypes().GetItemAt(13)->GetLabel());
        Assert::AreEqual((int)TriggerConditionType::Remember, vmTrigger.ConditionTypes().GetItemAt(14)->GetId());
        Assert::AreEqual(std::wstring(L"Remember"), vmTrigger.ConditionTypes().GetItemAt(14)->GetLabel());

        Assert::AreEqual({ 8U }, vmTrigger.OperandTypes().Count());
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
        Assert::AreEqual((int)TriggerOperandType::Float, vmTrigger.OperandTypes().GetItemAt(5)->GetId());
        Assert::AreEqual(std::wstring(L"Float"), vmTrigger.OperandTypes().GetItemAt(5)->GetLabel());
        Assert::AreEqual((int)TriggerOperandType::Inverted, vmTrigger.OperandTypes().GetItemAt(6)->GetId());
        Assert::AreEqual(std::wstring(L"Invert"), vmTrigger.OperandTypes().GetItemAt(6)->GetLabel());
        Assert::AreEqual((int)TriggerOperandType::Recall, vmTrigger.OperandTypes().GetItemAt(7)->GetId());
        Assert::AreEqual(std::wstring(L"Recall"), vmTrigger.OperandTypes().GetItemAt(7)->GetLabel());

        Assert::AreEqual({ 24U }, vmTrigger.OperandSizes().Count());
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
        Assert::AreEqual((int)MemSize::SixteenBitBigEndian, vmTrigger.OperandSizes().GetItemAt(14)->GetId());
        Assert::AreEqual(std::wstring(L"16-bit BE"), vmTrigger.OperandSizes().GetItemAt(14)->GetLabel());
        Assert::AreEqual((int)MemSize::TwentyFourBitBigEndian, vmTrigger.OperandSizes().GetItemAt(15)->GetId());
        Assert::AreEqual(std::wstring(L"24-bit BE"), vmTrigger.OperandSizes().GetItemAt(15)->GetLabel());
        Assert::AreEqual((int)MemSize::ThirtyTwoBitBigEndian, vmTrigger.OperandSizes().GetItemAt(16)->GetId());
        Assert::AreEqual(std::wstring(L"32-bit BE"), vmTrigger.OperandSizes().GetItemAt(16)->GetLabel());
        Assert::AreEqual((int)MemSize::BitCount, vmTrigger.OperandSizes().GetItemAt(17)->GetId());
        Assert::AreEqual(std::wstring(L"BitCount"), vmTrigger.OperandSizes().GetItemAt(17)->GetLabel());
        Assert::AreEqual((int)MemSize::Float, vmTrigger.OperandSizes().GetItemAt(18)->GetId());
        Assert::AreEqual(std::wstring(L"Float"), vmTrigger.OperandSizes().GetItemAt(18)->GetLabel());
        Assert::AreEqual((int)MemSize::FloatBigEndian, vmTrigger.OperandSizes().GetItemAt(19)->GetId());
        Assert::AreEqual(std::wstring(L"Float BE"), vmTrigger.OperandSizes().GetItemAt(19)->GetLabel());
        Assert::AreEqual((int)MemSize::Double32, vmTrigger.OperandSizes().GetItemAt(20)->GetId());
        Assert::AreEqual(std::wstring(L"Double32"), vmTrigger.OperandSizes().GetItemAt(20)->GetLabel());
        Assert::AreEqual((int)MemSize::Double32BigEndian, vmTrigger.OperandSizes().GetItemAt(21)->GetId());
        Assert::AreEqual(std::wstring(L"Double32 BE"), vmTrigger.OperandSizes().GetItemAt(21)->GetLabel());
        Assert::AreEqual((int)MemSize::MBF32, vmTrigger.OperandSizes().GetItemAt(22)->GetId());
        Assert::AreEqual(std::wstring(L"MBF32"), vmTrigger.OperandSizes().GetItemAt(22)->GetLabel());
        Assert::AreEqual((int)MemSize::MBF32LE, vmTrigger.OperandSizes().GetItemAt(23)->GetId());
        Assert::AreEqual(std::wstring(L"MBF32 LE"), vmTrigger.OperandSizes().GetItemAt(23)->GetLabel());

        Assert::AreEqual({ 14U }, vmTrigger.OperatorTypes().Count());
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
        Assert::AreEqual((int)TriggerOperatorType::Modulus, vmTrigger.OperatorTypes().GetItemAt(9)->GetId());
        Assert::AreEqual(std::wstring(L"%"), vmTrigger.OperatorTypes().GetItemAt(9)->GetLabel());
        Assert::AreEqual((int)TriggerOperatorType::Add, vmTrigger.OperatorTypes().GetItemAt(10)->GetId());
        Assert::AreEqual(std::wstring(L"+"), vmTrigger.OperatorTypes().GetItemAt(10)->GetLabel());
        Assert::AreEqual((int)TriggerOperatorType::Subtract, vmTrigger.OperatorTypes().GetItemAt(11)->GetId());
        Assert::AreEqual(std::wstring(L"-"), vmTrigger.OperatorTypes().GetItemAt(11)->GetLabel());
        Assert::AreEqual((int)TriggerOperatorType::BitwiseAnd, vmTrigger.OperatorTypes().GetItemAt(12)->GetId());
        Assert::AreEqual(std::wstring(L"&"), vmTrigger.OperatorTypes().GetItemAt(12)->GetLabel());
        Assert::AreEqual((int)TriggerOperatorType::BitwiseXor, vmTrigger.OperatorTypes().GetItemAt(13)->GetId());
        Assert::AreEqual(std::wstring(L"^"), vmTrigger.OperatorTypes().GetItemAt(13)->GetLabel());
    }

    TEST_METHOD(TestParseAndRegenerateCoreOnly)
    {
        ParseAndRegenerate(""); // no conditions
        ParseAndRegenerate("0xH1234=0xH2345"); // one condition
        ParseAndRegenerate("0xH1234=0xH2345_0xX5555=1.3._R:0x face=678"); // several conditions
        ParseAndRegenerate("I:0x 1234_A:0xH2345_0xH7777=345"); // addsource/addaddress chain
        ParseAndRegenerate("M:0xH1234>3"); // measured (not as percent)
        ParseAndRegenerate("Q:0xH1234>3"); // measured (as percent)
    }

    TEST_METHOD(TestParseAndRegenerateWithAlts)
    {
        ParseAndRegenerate("0xH1234=0xH2345S0xX5555=1"); // one alt
        ParseAndRegenerate("0xH1234=0xH2345S0xX5555=1.3.SR:0x face=678S0xL3333!=d0xL3333"); // several alts
        ParseAndRegenerate("S0xX5555=1.3.SR:0x face=678S0xL3333!=d0xL3333"); // only alts
        ParseAndRegenerate("0xH1234=0xH2345SI:0x 1234_A:0xH2345_0xH7777=345"); // addsource/addaddress chain
    }

    TEST_METHOD(TestParseAndRegenerateComplex)
    {
        ParseAndRegenerate("R:0xH1234=5.100.");                               // flag and 100 hits
        ParseAndRegenerate("A:0xH1234_A:0xH1236_A:0xH1237_M:0xH1238=99");     // addsource chain
        ParseAndRegenerate("A:d0xH1234_A:d0xH1236_A:d0xH1237_M:d0xH1238=99"); // addsource delta chain
        ParseAndRegenerate("A:p0xH1234_A:p0xH1236_A:p0xH1237_M:p0xH1238=99"); // addsource prior chain
        ParseAndRegenerate("I:0xX1234_0xH0010=1");                            // addaddress chain
    }

    TEST_METHOD(TestParseAndRegenerateValue)
    {
        ParseAndRegenerateValue(""); // empty
        ParseAndRegenerateValue("A:0xH1234*10_M:0xH1235"); // multiplication
        ParseAndRegenerateValue("A:0xH1234*10_M:0xH1235*5"); // multiplication
        ParseAndRegenerateValue("M:0xH1234$M:0xX5555"); // one alt
        ParseAndRegenerateValue("M:0xH1234$M:0xX5555$M:b0xL3333"); // several alts
        ParseAndRegenerateValue("M:0xH1234$I:0x 1234_A:0xH2345_M:0xH7777"); // addsource/addaddress chain
        ParseAndRegenerateValue("M:0xS2345"); // bit6
        ParseAndRegenerateValue("M:0xS2345$M:0xS2346"); // bit6 in alts
        ParseAndRegenerateValue("A:0xH1234&31_M:0"); // bitwise and
        ParseAndRegenerateValue("A:0xH1234^129_M:0"); // bitwise or
    }

    TEST_METHOD(TestTriggerUpdateConditionModified)
    {
        TriggerViewModelHarness vmTrigger;
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
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=0xH2345S0xX5555=1");

        TriggerMonitor pMonitor(vmTrigger);
        vmTrigger.SetSelectedGroupIndex(0);
        vmTrigger.NewCondition();
        Assert::AreEqual(std::string("0xH1234=0xH2345_0xH0000=0S0xX5555=1"), pMonitor.sNewTrigger);

        vmTrigger.SelectRange(1, 1, false); // deselect new condition
        vmTrigger.SelectRange(0, 0, true); // select first condition
        vmTrigger.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [](ra::ui::viewmodels::MessageBoxViewModel&) { return DialogResult::Yes; });
        vmTrigger.RemoveSelectedConditions();
        Assert::AreEqual(std::string("0xH0000=0S0xX5555=1"), pMonitor.sNewTrigger);

        vmTrigger.SetSelectedGroupIndex(1);
        vmTrigger.NewCondition();
        Assert::AreEqual(std::string("0xH0000=0S0xX5555=1_0xH0000=0"), pMonitor.sNewTrigger);

        vmTrigger.SelectRange(1, 1, false); // deselect new condition
        vmTrigger.SelectRange(0, 0, true);  // select first condition
        vmTrigger.RemoveSelectedConditions();
        Assert::AreEqual(std::string("0xH0000=0S0xH0000=0"), pMonitor.sNewTrigger);
    }

    TEST_METHOD(TestTriggerUpdateConditionsAddedModified)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=0xH2345S0xX5555=1");

        TriggerMonitor pMonitor(vmTrigger);
        vmTrigger.SetSelectedGroupIndex(0);
        vmTrigger.NewCondition();
        Assert::AreEqual(std::string("0xH1234=0xH2345_0xH0000=0S0xX5555=1"), pMonitor.sNewTrigger);

        vmTrigger.Conditions().GetItemAt(1)->SetSourceValue(1234U);
        Assert::AreEqual(std::string("0xH1234=0xH2345_0xH04d2=0S0xX5555=1"), pMonitor.sNewTrigger);
    }

    TEST_METHOD(TestTriggerUpdateSelectedGroupRemoved)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=0xH2345S0xX5555=1");

        vmTrigger.SetSelectedGroupIndex(1);
        Assert::AreEqual({ 1 }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::wstring(L"0x5555"), vmTrigger.Conditions().GetItemAt(0)->GetSourceValue());

        vmTrigger.UpdateFrom("0xH1234=0xH2345");

        Assert::AreEqual(0, vmTrigger.GetSelectedGroupIndex());
        Assert::AreEqual({ 1 }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::wstring(L"0x1234"), vmTrigger.Conditions().GetItemAt(0)->GetSourceValue());

        // calling UpdateFrom does not change the version, so we can't use a monitor as it
        // won't get the event. but we can check the serialized value
        Assert::AreEqual(std::string("0xH1234=0xH2345"), vmTrigger.Serialize());
    }

    TEST_METHOD(TestTriggerUpdateConditionsResetAndReapplied)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=1");

        TriggerMonitor pMonitor(vmTrigger);
        vmTrigger.SetSelectedGroupIndex(0);
        vmTrigger.NewCondition();
        Assert::AreEqual(std::string("0xH1234=1_0xH0000=0"), pMonitor.sNewTrigger);
        Assert::AreEqual(std::string("0xH1234=1_0xH0000=0"), vmTrigger.Serialize());

        vmTrigger.UpdateFrom("0xH1234=1");
        Assert::AreEqual({ 1 }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xH1234=1"), vmTrigger.Serialize());

        vmTrigger.NewCondition();
        Assert::AreEqual(std::string("0xH1234=1_0xH0000=0"), pMonitor.sNewTrigger);
        Assert::AreEqual(std::string("0xH1234=1_0xH0000=0"), vmTrigger.Serialize());
    }

    TEST_METHOD(TestCopyToClipboard)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16_0xL65FF=11.1.SR:0xT3333=1S0xW5555=16");

        Assert::AreEqual({ 2U }, vmTrigger.Conditions().Count());
        Assert::AreEqual({ 3U }, vmTrigger.Groups().Count());
        vmTrigger.CopyToClipboard();
        Assert::AreEqual(std::wstring(L"0xH1234=16_0xL65ff=11.1.SR:0xT3333=1S0xW5555=16"), vmTrigger.mockClipboard.GetText());

        vmTrigger.Conditions().GetItemAt(1)->SetSelected(true);
        vmTrigger.CopyToClipboard();
        Assert::AreEqual(std::wstring(L"0xH1234=16_0xL65ff=11.1.SR:0xT3333=1S0xW5555=16"), vmTrigger.mockClipboard.GetText());

        vmTrigger.Groups().GetItemAt(2)->SetSelected(true);
        vmTrigger.CopyToClipboard();
        Assert::AreEqual(std::wstring(L"0xH1234=16_0xL65ff=11.1.SR:0xT3333=1S0xW5555=16"), vmTrigger.mockClipboard.GetText());
    }

    TEST_METHOD(TestCopySelectedConditionsToClipboard)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16_0xL65FF=11.1._R:0xT3333=1_0xW5555=16");

        Assert::AreEqual({ 4U }, vmTrigger.Conditions().Count());
        vmTrigger.SelectRange(1, 2, true);

        vmTrigger.CopySelectedConditionsToClipboard();
        Assert::AreEqual(std::wstring(L"0xL65ff=11.1._R:0xT3333=1"), vmTrigger.mockClipboard.GetText());

        vmTrigger.SelectRange(3, 3, true);
        vmTrigger.CopySelectedConditionsToClipboard();
        Assert::AreEqual(std::wstring(L"0xL65ff=11.1._R:0xT3333=1_0xW5555=16"), vmTrigger.mockClipboard.GetText());

        vmTrigger.SelectRange(2, 2, false);
        vmTrigger.CopySelectedConditionsToClipboard();
        Assert::AreEqual(std::wstring(L"0xL65ff=11.1._0xW5555=16"), vmTrigger.mockClipboard.GetText());
    }

    TEST_METHOD(TestCopySelectedConditionsToClipboardNoSelection)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16_0xL65FF=11.1._R:0xT3333=1_0xW5555=16");

        Assert::AreEqual({ 4U }, vmTrigger.Conditions().Count());

        vmTrigger.CopySelectedConditionsToClipboard();
        Assert::AreEqual(std::wstring(L"0xH1234=16_0xL65ff=11.1._R:0xT3333=1_0xW5555=16"), vmTrigger.mockClipboard.GetText());

        vmTrigger.SelectRange(2, 2, true);
        vmTrigger.CopySelectedConditionsToClipboard();
        Assert::AreEqual(std::wstring(L"R:0xT3333=1"), vmTrigger.mockClipboard.GetText());

        vmTrigger.SelectRange(2, 2, false);
        vmTrigger.CopySelectedConditionsToClipboard();
        Assert::AreEqual(std::wstring(L"0xH1234=16_0xL65ff=11.1._R:0xT3333=1_0xW5555=16"), vmTrigger.mockClipboard.GetText());
    }

    TEST_METHOD(TestPasteFromClipboardEmpty)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16");
        Assert::AreEqual({ 1U }, vmTrigger.Conditions().Count());

        bool bDialogShown = false;
        vmTrigger.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogShown = true;

            Assert::AreEqual(std::wstring(L"Paste failed."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Clipboard did not contain any text."), vmMessageBox.GetMessage());

            return DialogResult::OK;
        });

        vmTrigger.mockClipboard.SetText(L"");
        vmTrigger.PasteFromClipboard();

        Assert::IsTrue(bDialogShown);
    }

    TEST_METHOD(TestPasteFromClipboardTrigger)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "A:0xH1235*100");
        Assert::AreEqual({1U}, vmTrigger.Conditions().Count());

        bool bDialogShown = false;
        vmTrigger.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox) {
                bDialogShown = true;

                Assert::AreEqual(std::wstring(L"Paste failed."), vmMessageBox.GetHeader());
                Assert::AreEqual(std::wstring(L"Clipboard did not contain valid trigger conditions."),
                                 vmMessageBox.GetMessage());

                return DialogResult::OK;
            });

        // Triggers allow conditions without flags
        vmTrigger.mockClipboard.SetText(L"0xH1234=7");
        vmTrigger.PasteFromClipboard();

        Assert::IsFalse(bDialogShown);

        // Measured requires a comparison for triggers - but allow the paste so the user can add one
        vmTrigger.mockClipboard.SetText(L"M:0xH1234");
        vmTrigger.PasteFromClipboard();

        Assert::IsFalse(bDialogShown);

        // cannot paste unparseable value
        vmTrigger.mockClipboard.SetText(L"banana");
        vmTrigger.PasteFromClipboard();

        Assert::IsTrue(bDialogShown);
    }

    TEST_METHOD(TestPasteFromClipboardValue)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "A:0xH1235*100");
        Assert::AreEqual({1U}, vmTrigger.Conditions().Count());
        vmTrigger.SetIsValue(true);

        // Measured requires a comparison for triggers
        bool bDialogShown = false;
        vmTrigger.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox) {
                bDialogShown = true;

                Assert::AreEqual(std::wstring(L"Paste failed."), vmMessageBox.GetHeader());
                Assert::AreEqual(std::wstring(L"Clipboard did not contain valid value conditions."),
                                 vmMessageBox.GetMessage());

                return DialogResult::OK;
            });

        // Measured does not require a comparison for values
        vmTrigger.mockClipboard.SetText(L"M:0xH1234");
        vmTrigger.PasteFromClipboard();

        Assert::IsFalse(bDialogShown);

        // Values do not allow conditions without flags - but allow the paste so the user can add a flag
        vmTrigger.mockClipboard.SetText(L"0xH1234=7");
        vmTrigger.PasteFromClipboard();

        Assert::IsFalse(bDialogShown);

        // cannot paste unparseable value
        vmTrigger.mockClipboard.SetText(L"banana");
        vmTrigger.PasteFromClipboard();

        Assert::IsTrue(bDialogShown);
    }

    TEST_METHOD(TestPasteFromClipboardInvalidSyntax)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16");
        Assert::AreEqual({ 1U }, vmTrigger.Conditions().Count());

        bool bDialogShown = false;
        vmTrigger.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogShown = true;

            Assert::AreEqual(std::wstring(L"Paste failed."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Clipboard did not contain valid trigger conditions."), vmMessageBox.GetMessage());

            return DialogResult::OK;
        });

        vmTrigger.mockClipboard.SetText(L"Hello, world!");
        vmTrigger.PasteFromClipboard();

        Assert::IsTrue(bDialogShown);
    }

    TEST_METHOD(TestPasteFromClipboardMultipleGroups)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16");
        Assert::AreEqual({ 1U }, vmTrigger.Conditions().Count());

        bool bDialogShown = false;
        vmTrigger.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogShown = true;

            Assert::AreEqual(std::wstring(L"Paste failed."), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"Clipboard contained multiple groups."), vmMessageBox.GetMessage());

            return DialogResult::OK;
        });

        vmTrigger.mockClipboard.SetText(L"0xL65FF=11S0xT3333=1");
        vmTrigger.PasteFromClipboard();

        Assert::IsTrue(bDialogShown);
    }

    TEST_METHOD(TestPasteFromClipboardMultipleConditions)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16");
        Assert::AreEqual({ 1U }, vmTrigger.Conditions().Count());
        vmTrigger.SelectRange(0, 0, true);

        vmTrigger.mockClipboard.SetText(L"0xL65ff=11.1._R:0xT3333=1");
        vmTrigger.PasteFromClipboard();

        Assert::IsFalse(vmTrigger.mockDesktop.WasDialogShown());
        Assert::AreEqual({ 3U }, vmTrigger.Conditions().Count());

        // newly pasted conditions should be selected, any previously selected items should be deselected
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(1)->IsSelected());
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(2)->IsSelected());

        Assert::AreEqual(std::string("0xH1234=16_0xL65ff=11.1._R:0xT3333=1"), vmTrigger.Serialize());
    }

    TEST_METHOD(TestRemoveSelectedConditions)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16_0xL65FF=11.1._R:0xT3333=1_0xW5555=16");

        Assert::AreEqual({ 4U }, vmTrigger.Conditions().Count());
        vmTrigger.SelectRange(1, 2, true);

        // two items selected
        bool bDialogShown = false;
        vmTrigger.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogShown = true;

            Assert::AreEqual(std::wstring(L"Are you sure that you want to delete 2 conditions?"), vmMessageBox.GetMessage());

            return DialogResult::Yes;
        });

        vmTrigger.RemoveSelectedConditions();
        Assert::IsTrue(bDialogShown);
        Assert::AreEqual({ 2U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xH1234=16_0xW5555=16"), vmTrigger.Serialize());
        Assert::AreEqual(1, vmTrigger.Conditions().GetItemAt(0)->GetIndex());
        Assert::AreEqual(2, vmTrigger.Conditions().GetItemAt(1)->GetIndex());

        // one item selected
        bDialogShown = false;
        vmTrigger.mockDesktop.ResetExpectedWindows();
        vmTrigger.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogShown = true;

            Assert::AreEqual(std::wstring(L"Are you sure that you want to delete 1 condition?"), vmMessageBox.GetMessage());

            return DialogResult::Yes;
        });

        vmTrigger.SelectRange(0, 0, true);
        vmTrigger.RemoveSelectedConditions();
        Assert::IsTrue(bDialogShown);
        Assert::AreEqual({ 1U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xW5555=16"), vmTrigger.Serialize());
        Assert::AreEqual(1, vmTrigger.Conditions().GetItemAt(0)->GetIndex());

        // no selection
        bDialogShown = false;
        vmTrigger.mockDesktop.ResetExpectedWindows();
        vmTrigger.RemoveSelectedConditions();
        Assert::IsFalse(bDialogShown);
        Assert::IsFalse(vmTrigger.mockDesktop.WasDialogShown());
        Assert::AreEqual({ 1U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xW5555=16"), vmTrigger.Serialize());
        Assert::AreEqual(1, vmTrigger.Conditions().GetItemAt(0)->GetIndex());
    }

    TEST_METHOD(TestRemoveSelectedConditionsCancel)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16_0xL65FF=11.1._R:0xT3333=1_0xW5555=16");

        Assert::AreEqual({ 4U }, vmTrigger.Conditions().Count());
        vmTrigger.SelectRange(1, 2, true);

        // two items selected
        bool bDialogShown = false;
        vmTrigger.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogShown = true;

            Assert::AreEqual(std::wstring(L"Are you sure that you want to delete 2 conditions?"), vmMessageBox.GetMessage());

            return DialogResult::No;
        });

        vmTrigger.RemoveSelectedConditions();
        Assert::IsTrue(bDialogShown);

        Assert::AreEqual({ 4U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xH1234=16_0xL65ff=11.1._R:0xT3333=1_0xW5555=16"), vmTrigger.Serialize());
    }

    TEST_METHOD(TestRemoveSelectedConditionsAll)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16_0xL65FF=11");

        Assert::AreEqual({ 2U }, vmTrigger.Conditions().Count());
        vmTrigger.SelectRange(0, 1, true);
        // DO NOT test Serialize() response here. We need to validate that it can go TO empty from uninitialized
        Assert::AreEqual(0, vmTrigger.GetVersion());

        // two items selected
        bool bDialogShown = false;
        vmTrigger.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogShown = true;

            Assert::AreEqual(std::wstring(L"Are you sure that you want to delete 2 conditions?"), vmMessageBox.GetMessage());

            return DialogResult::Yes;
        });

        vmTrigger.RemoveSelectedConditions();
        Assert::IsTrue(bDialogShown);
        Assert::AreEqual({ 0U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string(""), vmTrigger.Serialize());
        Assert::AreEqual(1, vmTrigger.GetVersion());
    }

    TEST_METHOD(TestResetSelectionWhenChangingGroups)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16_0xL65FF=11.1._R:0xT3333=1S0xW5555=16S0=1");

        Assert::AreEqual({ 3U }, vmTrigger.Conditions().Count());
        Assert::AreEqual({ 3U }, vmTrigger.Groups().Count());

        vmTrigger.SetSelectedGroupIndex(1);
        Assert::AreEqual({ 1U }, vmTrigger.Conditions().Count());

        vmTrigger.SelectRange(0, 0, true);
        Assert::AreEqual({ 1U }, vmTrigger.GetSelectedItems().size());

        vmTrigger.SetSelectedGroupIndex(0);
        Assert::AreEqual({ 0U }, vmTrigger.GetSelectedItems().size());

        vmTrigger.SelectRange(1, 1, true);
        Assert::AreEqual({ 1U }, vmTrigger.GetSelectedItems().size());

        vmTrigger.SetSelectedGroupIndex(1);
        Assert::AreEqual({ 0U }, vmTrigger.GetSelectedItems().size());

        vmTrigger.SelectRange(0, 0, true);
        Assert::AreEqual({ 1U }, vmTrigger.GetSelectedItems().size());

        vmTrigger.SetSelectedGroupIndex(0);
        Assert::AreEqual({ 0U }, vmTrigger.GetSelectedItems().size());

        vmTrigger.SelectRange(0, 0, true);
        Assert::AreEqual({ 1U }, vmTrigger.GetSelectedItems().size());

        vmTrigger.SetSelectedGroupIndex(1);
        Assert::AreEqual({ 0U }, vmTrigger.GetSelectedItems().size());
    }

    TEST_METHOD(TestMoveSelectedConditionsUp)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16_0xL65FF=11.1._R:0xT3333=1_0xW5555=16");

        Assert::AreEqual({ 4U }, vmTrigger.Conditions().Count());
        vmTrigger.SelectRange(2, 2, true);

        // one item selected - move item 3 to position 2
        vmTrigger.MoveSelectedConditionsUp();

        Assert::AreEqual({ 4U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xH1234=16_R:0xT3333=1_0xL65ff=11.1._0xW5555=16"), vmTrigger.Serialize());

        // selection should be maintained
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(1)->IsSelected());
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(2)->IsSelected());
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(3)->IsSelected());

        // indices should be updated
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(0)->GetIndex(), 1);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(1)->GetIndex(), 2);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(2)->GetIndex(), 3);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(3)->GetIndex(), 4);

        // items 1 and 3 are selected, they should get moved into slots 0 and 1
        vmTrigger.SelectRange(3, 3, true);
        vmTrigger.MoveSelectedConditionsUp();

        Assert::AreEqual(std::string("R:0xT3333=1_0xW5555=16_0xH1234=16_0xL65ff=11.1."), vmTrigger.Serialize());

        // selection should be maintained
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(1)->IsSelected());
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(2)->IsSelected());
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(3)->IsSelected());

        // indices should be updated
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(0)->GetIndex(), 1);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(1)->GetIndex(), 2);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(2)->GetIndex(), 3);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(3)->GetIndex(), 4);

        // items 0 and 2 are selected, they should get moved into slots 0 and 1
        vmTrigger.SelectRange(1, 1, false);
        vmTrigger.SelectRange(2, 2, true);
        vmTrigger.MoveSelectedConditionsUp();

        Assert::AreEqual(std::string("R:0xT3333=1_0xH1234=16_0xW5555=16_0xL65ff=11.1."), vmTrigger.Serialize());

        // selection should be maintained
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(1)->IsSelected());
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(2)->IsSelected());
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(3)->IsSelected());

        // indices should be updated
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(0)->GetIndex(), 1);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(1)->GetIndex(), 2);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(2)->GetIndex(), 3);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(3)->GetIndex(), 4);

        // items 0 and 1 are selected and cannot be moved up any further
        vmTrigger.MoveSelectedConditionsUp();

        Assert::AreEqual(std::string("R:0xT3333=1_0xH1234=16_0xW5555=16_0xL65ff=11.1."), vmTrigger.Serialize());

        // selection should be maintained
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(1)->IsSelected());
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(2)->IsSelected());
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(3)->IsSelected());

        // indices should be updated
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(0)->GetIndex(), 1);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(1)->GetIndex(), 2);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(2)->GetIndex(), 3);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(3)->GetIndex(), 4);
    }

    TEST_METHOD(TestMoveSelectedConditionsDown)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16_0xL65FF=11.1._R:0xT3333=1_0xW5555=16");

        Assert::AreEqual({ 4U }, vmTrigger.Conditions().Count());
        vmTrigger.SelectRange(1, 1, true);

        // one item selected - move item 2 to position 3
        vmTrigger.MoveSelectedConditionsDown();

        Assert::AreEqual({ 4U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xH1234=16_R:0xT3333=1_0xL65ff=11.1._0xW5555=16"), vmTrigger.Serialize());

        // selection should be maintained
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(0)->IsSelected());
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(1)->IsSelected());
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(2)->IsSelected());
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(3)->IsSelected());

        // indices should be updated
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(0)->GetIndex(), 1);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(1)->GetIndex(), 2);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(2)->GetIndex(), 3);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(3)->GetIndex(), 4);

        // items 0 and 2 are selected, they should get moved into slots 2 and 3
        vmTrigger.SelectRange(0, 0, true);
        vmTrigger.MoveSelectedConditionsDown();

        Assert::AreEqual(std::string("R:0xT3333=1_0xW5555=16_0xH1234=16_0xL65ff=11.1."), vmTrigger.Serialize());

        // selection should be maintained
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(0)->IsSelected());
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(1)->IsSelected());
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(2)->IsSelected());
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(3)->IsSelected());

        // indices should be updated
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(0)->GetIndex(), 1);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(1)->GetIndex(), 2);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(2)->GetIndex(), 3);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(3)->GetIndex(), 4);

        // items 1 and 3 are selected, they should get moved into slots 2 and 3
        vmTrigger.SelectRange(2, 2, false);
        vmTrigger.SelectRange(1, 1, true);
        vmTrigger.MoveSelectedConditionsDown();

        Assert::AreEqual(std::string("R:0xT3333=1_0xH1234=16_0xW5555=16_0xL65ff=11.1."), vmTrigger.Serialize());

        // selection should be maintained
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(0)->IsSelected());
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(1)->IsSelected());
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(2)->IsSelected());
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(3)->IsSelected());

        // indices should be updated
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(0)->GetIndex(), 1);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(1)->GetIndex(), 2);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(2)->GetIndex(), 3);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(3)->GetIndex(), 4);

        // items 2 and 3 are selected and cannot be moved
        vmTrigger.MoveSelectedConditionsDown();

        Assert::AreEqual(std::string("R:0xT3333=1_0xH1234=16_0xW5555=16_0xL65ff=11.1."), vmTrigger.Serialize());

        // selection should be maintained
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(0)->IsSelected());
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(1)->IsSelected());
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(2)->IsSelected());
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(3)->IsSelected());

        // indices should be updated
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(0)->GetIndex(), 1);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(1)->GetIndex(), 2);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(2)->GetIndex(), 3);
        Assert::AreEqual(vmTrigger.Conditions().GetItemAt(3)->GetIndex(), 4);
    }

    TEST_METHOD(TestNewCondition)
    {
        std::array<uint8_t, 10> pMemory = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16");
        Assert::AreEqual({ 1U }, vmTrigger.Conditions().Count());
        vmTrigger.SelectRange(0, 0, true);

        vmTrigger.InitializeMemory(&pMemory.at(0), pMemory.size());
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetAddress(8);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetSize(MemSize::EightBit);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().DoFrame(); // load viewer memory data
        vmTrigger.NewCondition();

        Assert::AreEqual({ 2U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(1, vmTrigger.GetVersion());

        // new condition should be selected, any previously selected items should be deselected
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(1)->IsSelected());

        Assert::AreEqual(std::string("0xH1234=16_0xH0008=8"), vmTrigger.Serialize());
    }

    TEST_METHOD(TestNewConditionViewerSize)
    {
        std::array<uint8_t, 10> pMemory = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16");
        Assert::AreEqual({ 1U }, vmTrigger.Conditions().Count());

        vmTrigger.InitializeMemory(&pMemory.at(0), pMemory.size());
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetAddress(8);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetSize(MemSize::SixteenBit);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().DoFrame(); // load viewer memory data
        vmTrigger.NewCondition();

        Assert::AreEqual({ 2U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xH1234=16_0x 0008=2312"), vmTrigger.Serialize());

        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetAddress(4);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetSize(MemSize::ThirtyTwoBit);
        vmTrigger.NewCondition();

        Assert::AreEqual({ 3U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xH1234=16_0x 0008=2312_0xX0004=117835012"), vmTrigger.Serialize());
    }

    TEST_METHOD(TestNewConditionCodeNoteSize)
    {
        std::array<uint8_t, 10> pMemory = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16");
        Assert::AreEqual({ 1U }, vmTrigger.Conditions().Count());
        vmTrigger.mockGameContext.SetCodeNote({ 8U }, L"[16-bit] test");
        vmTrigger.mockGameContext.SetCodeNote({ 4U }, L"[4 byte] test");

        vmTrigger.InitializeMemory(&pMemory.at(0), pMemory.size());
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetAddress(8);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetSize(MemSize::EightBit);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().DoFrame(); // load viewer memory data
        vmTrigger.NewCondition();

        Assert::AreEqual({ 2U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xH1234=16_0x 0008=2312"), vmTrigger.Serialize());

        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetAddress(4);
        vmTrigger.NewCondition();

        Assert::AreEqual({ 3U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xH1234=16_0x 0008=2312_0xX0004=117835012"), vmTrigger.Serialize());
    }

    TEST_METHOD(TestNewConditionCodeNoteSizeFloat)
    {
        std::array<uint8_t, 12> pMemory = { 0, 1, 2, 3, 0xDB, 0x0F, 0x49, 0x40, 0x82, 0x80, 0x00, 0x00 };
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16");
        Assert::AreEqual({ 1U }, vmTrigger.Conditions().Count());
        vmTrigger.mockGameContext.SetCodeNote({ 4U }, L"[float] test");
        vmTrigger.mockGameContext.SetCodeNote({ 8U }, L"[MBF32] test");

        vmTrigger.InitializeMemory(&pMemory.at(0), pMemory.size());
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetAddress(4);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetSize(MemSize::EightBit);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().DoFrame(); // load viewer memory data
        vmTrigger.NewCondition();

        Assert::AreEqual({ 2U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xH1234=16_fF0004=f3.141593"), vmTrigger.Serialize());

        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetAddress(8);
        vmTrigger.NewCondition();

        Assert::AreEqual({ 3U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xH1234=16_fF0004=f3.141593_fM0008=f-2.0"), vmTrigger.Serialize());
    }

    TEST_METHOD(TestNewConditionCodeNoteSizeViewerSize)
    {
        std::array<uint8_t, 10> pMemory = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16");
        Assert::AreEqual({ 1U }, vmTrigger.Conditions().Count());
        vmTrigger.mockGameContext.SetCodeNote({ 8U }, L"[16-bit] test");
        vmTrigger.mockGameContext.SetCodeNote({ 2U }, L"[8-bit] test");
        vmTrigger.mockGameContext.SetCodeNote({ 4U }, L"test");

        vmTrigger.InitializeMemory(&pMemory.at(0), pMemory.size());
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetAddress(8);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetSize(MemSize::ThirtyTwoBit);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().DoFrame(); // load viewer memory data
        vmTrigger.NewCondition();

        Assert::AreEqual({ 2U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xH1234=16_0x 0008=2312"), vmTrigger.Serialize());

        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetAddress(4);
        vmTrigger.NewCondition();

        Assert::AreEqual({ 3U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xH1234=16_0x 0008=2312_0xX0004=117835012"), vmTrigger.Serialize());

        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetAddress(2);
        vmTrigger.NewCondition();

        Assert::AreEqual({ 4U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xH1234=16_0x 0008=2312_0xX0004=117835012_0xH0002=2"), vmTrigger.Serialize());
    }

    TEST_METHOD(TestNewConditionAddAddressChain)
    {
        std::array<uint8_t, 10> pMemory = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16");
        Assert::AreEqual({ 1U }, vmTrigger.Conditions().Count());
        vmTrigger.Conditions().GetItemAt(0)->SetSelected(true);

        vmTrigger.InitializeMemory(&pMemory.at(0), pMemory.size());
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetAddress(8);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetSize(MemSize::EightBit);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().DoFrame(); // load viewer memory data

        vmTrigger.NewCondition();
        Assert::AreEqual({ 2U }, vmTrigger.Conditions().Count());
        vmTrigger.Conditions().GetItemAt(1)->SetType(TriggerConditionType::AddAddress);
        vmTrigger.Conditions().GetItemAt(1)->SetSourceValue(2);
        vmTrigger.NewCondition();
        Assert::AreEqual({ 3U }, vmTrigger.Conditions().Count());
        vmTrigger.Conditions().GetItemAt(2)->SetSourceValue(5);
        vmTrigger.Conditions().GetItemAt(2)->SetTargetValue(6U);

        Assert::AreEqual({ 3U }, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xH1234=16_I:0xH0002_0xH0005=6"), vmTrigger.Serialize());
        vmTrigger.UpdateFrom("0xH1234=16_I:0xH0002_0xH0005=6"); // force refresh to rebuild indirect chain

        Assert::AreEqual(std::wstring(L"0x0007 (indirect $0x0002+0x05)\r\n[No code note]"),
            vmTrigger.Conditions().GetItemAt(2)->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestNewConditionFromIndirectNote)
    {
        std::array<uint8_t, 10> pMemory = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0xH1234=16");
        Assert::AreEqual({1U}, vmTrigger.Conditions().Count());
        vmTrigger.Conditions().GetItemAt(0)->SetSelected(true);

        vmTrigger.InitializeMemory(&pMemory.at(0), pMemory.size());
        vmTrigger.mockGameContext.SetCodeNote({4U}, L"[8-bit pointer] test\n+4=[16-bit] note");
        vmTrigger.mockGameContext.DoFrame();

        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetAddress(8);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetSize(MemSize::EightBit);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().DoFrame(); // load viewer memory data
        Assert::AreEqual(
            std::wstring(L"[Indirect from 0x0004]\r\n[16-bit] note"),
            vmTrigger.mockWindowManager.MemoryInspector.GetCurrentAddressNote());

        vmTrigger.NewCondition();
        Assert::AreEqual({3U}, vmTrigger.Conditions().Count());
        Assert::AreEqual(std::string("0xH1234=16_I:0xH0004_0x 0004=2312"), vmTrigger.Serialize());
    }

    TEST_METHOD(TestNewConditionValue)
    {
        std::array<uint8_t, 10> pMemory = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        TriggerViewModelHarness vmTrigger;
        vmTrigger.SetIsValue(true);
        vmTrigger.AddGroup();
        Assert::AreEqual({ 0U }, vmTrigger.Conditions().Count());

        vmTrigger.InitializeMemory(&pMemory.at(0), pMemory.size());
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetAddress(8);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetSize(MemSize::EightBit);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().DoFrame(); // load viewer memory data
        vmTrigger.NewCondition();

        // first condition added to a value should be Measured without an operator
        Assert::AreEqual({ 1U }, vmTrigger.Conditions().Count());
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(0)->IsSelected());
        Assert::AreEqual(std::string("M:0xH0008"), vmTrigger.Serialize());

        // second condition should be added normally (as a comparison)
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetAddress(4);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetSize(MemSize::SixteenBit);
        vmTrigger.NewCondition();

        Assert::AreEqual({ 2U }, vmTrigger.Conditions().Count());

        // new condition should be selected, any previously selected items should be deselected
        Assert::IsFalse(vmTrigger.Conditions().GetItemAt(0)->IsSelected());
        Assert::IsTrue(vmTrigger.Conditions().GetItemAt(1)->IsSelected());

        Assert::AreEqual(std::string("M:0xH0008_N:0x 0004=1284"), vmTrigger.Serialize());
    }

    TEST_METHOD(TestAddGroupNoAlts)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "");
        Assert::AreEqual({ 1U }, vmTrigger.Groups().Count());

        vmTrigger.AddGroup();
        Assert::AreEqual({ 3U }, vmTrigger.Groups().Count());

        Assert::AreEqual(std::wstring(L"Core"), vmTrigger.Groups().GetItemAt(0)->GetLabel());
        Assert::AreEqual(std::wstring(L"Alt 1"), vmTrigger.Groups().GetItemAt(1)->GetLabel());
        Assert::AreEqual(std::wstring(L"Alt 2"), vmTrigger.Groups().GetItemAt(2)->GetLabel());

        Assert::AreEqual(1, vmTrigger.GetSelectedGroupIndex());
        Assert::AreEqual(1, vmTrigger.GetEnsureVisibleGroupIndex());
        Assert::AreEqual(1, vmTrigger.GetVersion());
    }

    TEST_METHOD(TestAddGroupOneAlt)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "S");
        Assert::AreEqual({ 2U }, vmTrigger.Groups().Count());

        vmTrigger.AddGroup();
        Assert::AreEqual({ 3U }, vmTrigger.Groups().Count());

        Assert::AreEqual(std::wstring(L"Core"), vmTrigger.Groups().GetItemAt(0)->GetLabel());
        Assert::AreEqual(std::wstring(L"Alt 1"), vmTrigger.Groups().GetItemAt(1)->GetLabel());
        Assert::AreEqual(std::wstring(L"Alt 2"), vmTrigger.Groups().GetItemAt(2)->GetLabel());

        Assert::AreEqual(2, vmTrigger.GetSelectedGroupIndex());
        Assert::AreEqual(2, vmTrigger.GetEnsureVisibleGroupIndex());
        Assert::AreEqual(1, vmTrigger.GetVersion());
    }

    TEST_METHOD(TestAddGroupTwoAlts)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "SS");
        Assert::AreEqual({ 3U }, vmTrigger.Groups().Count());

        vmTrigger.AddGroup();
        Assert::AreEqual({ 4U }, vmTrigger.Groups().Count());

        Assert::AreEqual(std::wstring(L"Core"), vmTrigger.Groups().GetItemAt(0)->GetLabel());
        Assert::AreEqual(std::wstring(L"Alt 1"), vmTrigger.Groups().GetItemAt(1)->GetLabel());
        Assert::AreEqual(std::wstring(L"Alt 2"), vmTrigger.Groups().GetItemAt(2)->GetLabel());
        Assert::AreEqual(std::wstring(L"Alt 3"), vmTrigger.Groups().GetItemAt(3)->GetLabel());

        Assert::AreEqual(3, vmTrigger.GetSelectedGroupIndex());
        Assert::AreEqual(3, vmTrigger.GetEnsureVisibleGroupIndex());
        Assert::AreEqual(1, vmTrigger.GetVersion());
    }

    TEST_METHOD(TestAddGroupNoAltsValue)
    {
        TriggerViewModelHarness vmTrigger;
        vmTrigger.SetIsValue(true);
        Parse(vmTrigger, "");
        Assert::AreEqual({ 1U }, vmTrigger.Groups().Count());

        vmTrigger.AddGroup();
        Assert::AreEqual({ 2U }, vmTrigger.Groups().Count());

        Assert::AreEqual(std::wstring(L"Value"), vmTrigger.Groups().GetItemAt(0)->GetLabel());
        Assert::AreEqual(std::wstring(L"Alt 1"), vmTrigger.Groups().GetItemAt(1)->GetLabel());

        Assert::AreEqual(1, vmTrigger.GetSelectedGroupIndex());
        Assert::AreEqual(1, vmTrigger.GetEnsureVisibleGroupIndex());
        Assert::AreEqual(1, vmTrigger.GetVersion());
    }

    TEST_METHOD(TestRemoveGroupCore)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "SS");
        Assert::AreEqual({ 3U }, vmTrigger.Groups().Count());
        Assert::AreEqual(0, vmTrigger.GetSelectedGroupIndex());

        bool bDialogShown = false;
        vmTrigger.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogShown = true;

            Assert::AreEqual(std::wstring(L"The core group cannot be removed."), vmMessageBox.GetMessage());

            return DialogResult::OK;
        });

        vmTrigger.RemoveGroup();
        Assert::IsTrue(bDialogShown);
        Assert::AreEqual({ 3U }, vmTrigger.Groups().Count());

        Assert::AreEqual(0, vmTrigger.GetSelectedGroupIndex());
        Assert::AreEqual(0, vmTrigger.GetVersion());
    }

    TEST_METHOD(TestRemoveGroupNonEmptyAlt)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "S0xH1234=1S");
        Assert::AreEqual({ 3U }, vmTrigger.Groups().Count());
        vmTrigger.SetSelectedGroupIndex(1);
        Assert::AreEqual(1, vmTrigger.GetSelectedGroupIndex());

        bool bDialogShown = false;
        vmTrigger.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogShown = true;

            Assert::AreEqual(std::wstring(L"Are you sure that you want to delete Alt 1?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The group is not empty, and this operation cannot be reverted."), vmMessageBox.GetMessage());

            return DialogResult::Yes;
        });

        vmTrigger.RemoveGroup();
        Assert::IsTrue(bDialogShown);
        Assert::AreEqual({ 2U }, vmTrigger.Groups().Count());

        Assert::AreEqual(std::wstring(L"Core"), vmTrigger.Groups().GetItemAt(0)->GetLabel());
        Assert::AreEqual(std::wstring(L"Alt 1"), vmTrigger.Groups().GetItemAt(1)->GetLabel());

        Assert::AreEqual(1, vmTrigger.GetSelectedGroupIndex());
        Assert::AreEqual(1, vmTrigger.GetVersion());

        Assert::AreEqual(std::string("S"), vmTrigger.Serialize());
    }

    TEST_METHOD(TestRemoveGroupNonEmptyAltCanceled)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "S0xH1234=1S");
        Assert::AreEqual({ 3U }, vmTrigger.Groups().Count());
        vmTrigger.SetSelectedGroupIndex(1);
        Assert::AreEqual(1, vmTrigger.GetSelectedGroupIndex());

        bool bDialogShown = false;
        vmTrigger.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogShown = true;

            Assert::AreEqual(std::wstring(L"Are you sure that you want to delete Alt 1?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The group is not empty, and this operation cannot be reverted."), vmMessageBox.GetMessage());

            return DialogResult::No;
        });

        vmTrigger.RemoveGroup();
        Assert::IsTrue(bDialogShown);
        Assert::AreEqual({ 3U }, vmTrigger.Groups().Count());

        Assert::AreEqual(std::wstring(L"Core"), vmTrigger.Groups().GetItemAt(0)->GetLabel());
        Assert::AreEqual(std::wstring(L"Alt 1"), vmTrigger.Groups().GetItemAt(1)->GetLabel());
        Assert::AreEqual(std::wstring(L"Alt 2"), vmTrigger.Groups().GetItemAt(2)->GetLabel());

        Assert::AreEqual(1, vmTrigger.GetSelectedGroupIndex());
        Assert::AreEqual(0, vmTrigger.GetVersion());

        Assert::AreEqual(std::string("S0xH1234=1S"), vmTrigger.Serialize());
    }

    TEST_METHOD(TestRemoveGroupNonEmptyAltLast)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "S0xH1234=1S0xH2345=1");
        Assert::AreEqual({ 3U }, vmTrigger.Groups().Count());
        vmTrigger.SetSelectedGroupIndex(2);
        Assert::AreEqual(2, vmTrigger.GetSelectedGroupIndex());

        bool bDialogShown = false;
        vmTrigger.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel& vmMessageBox)
        {
            bDialogShown = true;

            Assert::AreEqual(std::wstring(L"Are you sure that you want to delete Alt 2?"), vmMessageBox.GetHeader());
            Assert::AreEqual(std::wstring(L"The group is not empty, and this operation cannot be reverted."), vmMessageBox.GetMessage());

            return DialogResult::Yes;
        });

        vmTrigger.RemoveGroup();
        Assert::IsTrue(bDialogShown);
        Assert::AreEqual({ 2U }, vmTrigger.Groups().Count());

        Assert::AreEqual(std::wstring(L"Core"), vmTrigger.Groups().GetItemAt(0)->GetLabel());
        Assert::AreEqual(std::wstring(L"Alt 1"), vmTrigger.Groups().GetItemAt(1)->GetLabel());

        Assert::AreEqual(1, vmTrigger.GetSelectedGroupIndex());
        Assert::AreEqual(1, vmTrigger.GetVersion());

        Assert::AreEqual(std::string("S0xH1234=1"), vmTrigger.Serialize());
    }

    TEST_METHOD(TestRemoveGroupEmptyAlt)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "SS");
        Assert::AreEqual({ 3U }, vmTrigger.Groups().Count());
        vmTrigger.SetSelectedGroupIndex(1);
        Assert::AreEqual(1, vmTrigger.GetSelectedGroupIndex());

        bool bDialogShown = false;
        vmTrigger.mockDesktop.ExpectWindow<ra::ui::viewmodels::MessageBoxViewModel>(
            [&bDialogShown](ra::ui::viewmodels::MessageBoxViewModel&)
        {
            bDialogShown = true;
            return DialogResult::Yes;
        });

        vmTrigger.RemoveGroup();
        Assert::IsFalse(bDialogShown);
        Assert::AreEqual({ 2U }, vmTrigger.Groups().Count());

        Assert::AreEqual(std::wstring(L"Core"), vmTrigger.Groups().GetItemAt(0)->GetLabel());
        Assert::AreEqual(std::wstring(L"Alt 1"), vmTrigger.Groups().GetItemAt(1)->GetLabel());

        Assert::AreEqual(1, vmTrigger.GetSelectedGroupIndex());
        Assert::AreEqual(1, vmTrigger.GetVersion());

        Assert::AreEqual(std::string("S"), vmTrigger.Serialize());
    }

    TEST_METHOD(TestRemoveGroupNoSelection)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "SS");
        Assert::AreEqual({ 3U }, vmTrigger.Groups().Count());

        vmTrigger.SetSelectedGroupIndex(-1);
        Assert::AreEqual(-1, vmTrigger.GetSelectedGroupIndex());

        vmTrigger.RemoveGroup();
        Assert::IsFalse(vmTrigger.mockDesktop.WasDialogShown());
        Assert::AreEqual({ 3U }, vmTrigger.Groups().Count());

        Assert::AreEqual(-1, vmTrigger.GetSelectedGroupIndex());
        Assert::AreEqual(0, vmTrigger.GetVersion());
    }

    TEST_METHOD(TestDoFrameUpdatesHits)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "1=1");
        Assert::AreEqual({ 1U }, vmTrigger.Groups().Count());

        vmTrigger.Groups().GetItemAt(0)->m_pConditionSet->conditions->current_hits = 2;
        vmTrigger.DoFrame();
        Assert::AreEqual(2U, vmTrigger.Conditions().GetItemAt(0)->GetCurrentHits());
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(0)->GetTotalHits()); // total hits only applies to AddHits groups

        vmTrigger.Groups().GetItemAt(0)->m_pConditionSet->conditions->current_hits++;
        vmTrigger.DoFrame();
        Assert::AreEqual(3U, vmTrigger.Conditions().GetItemAt(0)->GetCurrentHits());
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(0)->GetTotalHits()); // total hits only applies to AddHits groups

        vmTrigger.Groups().GetItemAt(0)->m_pConditionSet->conditions->current_hits++;
        vmTrigger.DoFrame();
        Assert::AreEqual(4U, vmTrigger.Conditions().GetItemAt(0)->GetCurrentHits());
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(0)->GetTotalHits()); // total hits only applies to AddHits groups

        vmTrigger.Groups().GetItemAt(0)->m_pConditionSet->conditions->current_hits = 0;
        vmTrigger.DoFrame();
        Assert::AreEqual(0U, vmTrigger.Conditions().GetItemAt(0)->GetCurrentHits());
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(0)->GetTotalHits()); // total hits only applies to AddHits groups
    }

    TEST_METHOD(TestDoFrameDoesNotUpdateTrigger)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0001=0001(10)");
        Assert::AreEqual({ 1U }, vmTrigger.Groups().Count());
        Assert::AreEqual(0, vmTrigger.GetVersion());

        vmTrigger.Groups().GetItemAt(0)->m_pConditionSet->conditions->current_hits = 2;
        vmTrigger.DoFrame();
        Assert::AreEqual(2U, vmTrigger.Conditions().GetItemAt(0)->GetCurrentHits());

        // version should not have been updated. trigger did not actually change.
        Assert::AreEqual(0, vmTrigger.GetVersion());

        // Serialize will regenerate the input string, which will differ, but the trigger version should not be updated
        Assert::AreEqual(std::string("1=1.10."), vmTrigger.Serialize());
        Assert::AreEqual(0, vmTrigger.GetVersion());
    }

    TEST_METHOD(TestDoFrameHitsChain)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0=0.5._C:0=0.10._0=0.20._0=0.30._C:0=0.50._D:0=0.60._0=0.70.");
        Assert::AreEqual({ 1U }, vmTrigger.Groups().Count());

        auto* cond = vmTrigger.Groups().GetItemAt(0)->m_pConditionSet->conditions;
        cond->current_hits = 2; cond = cond->next;  //         0=0 (5)
        cond->current_hits = 3; cond = cond->next;  // AddHits 0=0 (10)
        cond->current_hits = 6; cond = cond->next;  //         0=0 (20)
        cond->current_hits = 12; cond = cond->next; //         0=0 (30)
        cond->current_hits = 24; cond = cond->next; // AddHits 0=0 (50)
        cond->current_hits = 1; cond = cond->next;  // SubHits 0=0 (60)
        cond->current_hits = 48;                    //         0=0 (70)
        vmTrigger.DoFrame();
        Assert::AreEqual(2U, vmTrigger.Conditions().GetItemAt(0)->GetCurrentHits());
        Assert::AreEqual(3U, vmTrigger.Conditions().GetItemAt(1)->GetCurrentHits());
        Assert::AreEqual(6U, vmTrigger.Conditions().GetItemAt(2)->GetCurrentHits());
        Assert::AreEqual(12U, vmTrigger.Conditions().GetItemAt(3)->GetCurrentHits());
        Assert::AreEqual(24U, vmTrigger.Conditions().GetItemAt(4)->GetCurrentHits());
        Assert::AreEqual(1U, vmTrigger.Conditions().GetItemAt(5)->GetCurrentHits());
        Assert::AreEqual(48U, vmTrigger.Conditions().GetItemAt(6)->GetCurrentHits());
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(0)->GetTotalHits()); // non hit-chain
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(1)->GetTotalHits()); // middle of hit-chain
        Assert::AreEqual(9, vmTrigger.Conditions().GetItemAt(2)->GetTotalHits()); // end of hit-chain (3+6)
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(3)->GetTotalHits()); // new non hit-chain
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(4)->GetTotalHits()); // middle of new hit-chain
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(5)->GetTotalHits()); // middle of new hit-chain
        Assert::AreEqual(71, vmTrigger.Conditions().GetItemAt(6)->GetTotalHits()); // end of new hit-chain (24-1+48)
    }

    TEST_METHOD(TestDoFrameHitsChainWithModifiers)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "C:0=0.5._I:0_A:0_C:0=0.50._N:0=0_O:0=0_P:0=0.70.");
        Assert::AreEqual({ 1U }, vmTrigger.Groups().Count());

        auto* cond = vmTrigger.Groups().GetItemAt(0)->m_pConditionSet->conditions;
        cond->current_hits = 2; cond = cond->next;  // AddHits    0=0 (5)
        cond->current_hits = 3; cond = cond->next;  // AddAddress 0
        cond->current_hits = 6; cond = cond->next;  // AddSource  0
        cond->current_hits = 12; cond = cond->next; // AddHits    0=0 (50)
        cond->current_hits = 24; cond = cond->next; // AndNext    0=0
        cond->current_hits = 1; cond = cond->next;  // OrNext     0=0
        cond->current_hits = 48;                    // PauseIf    0=0 (70)
        vmTrigger.DoFrame();
        Assert::AreEqual(2U, vmTrigger.Conditions().GetItemAt(0)->GetCurrentHits());
        Assert::AreEqual(3U, vmTrigger.Conditions().GetItemAt(1)->GetCurrentHits());
        Assert::AreEqual(6U, vmTrigger.Conditions().GetItemAt(2)->GetCurrentHits());
        Assert::AreEqual(12U, vmTrigger.Conditions().GetItemAt(3)->GetCurrentHits());
        Assert::AreEqual(24U, vmTrigger.Conditions().GetItemAt(4)->GetCurrentHits());
        Assert::AreEqual(1U, vmTrigger.Conditions().GetItemAt(5)->GetCurrentHits());
        Assert::AreEqual(48U, vmTrigger.Conditions().GetItemAt(6)->GetCurrentHits());
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(0)->GetTotalHits()); // start of hit-chain
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(1)->GetTotalHits()); // non hit modifier
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(2)->GetTotalHits()); // non hit modifier
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(3)->GetTotalHits()); // middle of hits-chain
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(4)->GetTotalHits()); // non hit modifier
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(5)->GetTotalHits()); // non hit modifier
        Assert::AreEqual(62, vmTrigger.Conditions().GetItemAt(6)->GetTotalHits()); // end of hit-chain (2+12+48)
    }

    TEST_METHOD(TestDoFrameHitsChainReset)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0=0.5._C:0=0.10._0=0.20._R:1=1");
        Assert::AreEqual({ 1U }, vmTrigger.Groups().Count());

        auto* cond = vmTrigger.Groups().GetItemAt(0)->m_pConditionSet->conditions;
        cond->current_hits = 2; cond = cond->next;  //         0=0 (5)
        cond->current_hits = 3; cond = cond->next;  // AddHits 0=0 (10)
        cond->current_hits = 6; cond = cond->next;  //         0=0 (20)
        vmTrigger.DoFrame();
        Assert::AreEqual(2U, vmTrigger.Conditions().GetItemAt(0)->GetCurrentHits());
        Assert::AreEqual(3U, vmTrigger.Conditions().GetItemAt(1)->GetCurrentHits());
        Assert::AreEqual(6U, vmTrigger.Conditions().GetItemAt(2)->GetCurrentHits());
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(0)->GetTotalHits()); // non hit-chain
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(1)->GetTotalHits()); // middle of hit-chain
        Assert::AreEqual(9, vmTrigger.Conditions().GetItemAt(2)->GetTotalHits()); // end of hit-chain (3+6)

        // mimic a ResetIf clearing all hits
        cond = vmTrigger.Groups().GetItemAt(0)->m_pConditionSet->conditions;
        cond->current_hits = 0; cond = cond->next;  //         0=0 (5)
        cond->current_hits = 0; cond = cond->next;  // AddHits 0=0 (10)
        cond->current_hits = 0; cond = cond->next;  //         0=0 (20)
        vmTrigger.DoFrame();
        Assert::AreEqual(0U, vmTrigger.Conditions().GetItemAt(0)->GetCurrentHits());
        Assert::AreEqual(0U, vmTrigger.Conditions().GetItemAt(1)->GetCurrentHits());
        Assert::AreEqual(0U, vmTrigger.Conditions().GetItemAt(2)->GetCurrentHits());
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(0)->GetTotalHits()); // non hit-chain
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(1)->GetTotalHits()); // middle of hit-chain
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(2)->GetTotalHits()); // end of hit-chain (3+6)
    }

    TEST_METHOD(TestBuildHitChainTooltip)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "0=0.5._C:0=0.10._0=0.20._0=0.30._C:0=0.50._D:0=0.60._0=0.70.");
        Assert::AreEqual({ 1U }, vmTrigger.Groups().Count());

        auto* cond = vmTrigger.Groups().GetItemAt(0)->m_pConditionSet->conditions;
        cond->current_hits = 2; cond = cond->next;  //         0=0 (5)
        cond->current_hits = 3; cond = cond->next;  // AddHits 0=0 (10)
        cond->current_hits = 6; cond = cond->next;  //         0=0 (20)
        cond->current_hits = 12; cond = cond->next; //         0=0 (30)
        cond->current_hits = 24; cond = cond->next; // AddHits 0=0 (50)
        cond->current_hits = 1; cond = cond->next;  // SubHits 0=0 (60)
        cond->current_hits = 48;                    //         0=0 (70)
        vmTrigger.DoFrame();

        const std::wstring sTooltip0 = L"";
        const std::wstring sTooltip1 = L"+3 (condition 2)\n+6 (condition 3)\n9/20 total";
        const std::wstring sTooltip2 = L"+24 (condition 5)\n-1 (condition 6)\n+48 (condition 7)\n71/70 total";
        Assert::AreEqual(sTooltip0, vmTrigger.GetHitChainTooltip(0)); // non hit-chain
        Assert::AreEqual(sTooltip1, vmTrigger.GetHitChainTooltip(1)); // middle of hit-chain
        Assert::AreEqual(sTooltip1, vmTrigger.GetHitChainTooltip(2)); // end of hit-chain (3+6)
        Assert::AreEqual(sTooltip0, vmTrigger.GetHitChainTooltip(3)); // new non hit-chain
        Assert::AreEqual(sTooltip2, vmTrigger.GetHitChainTooltip(4)); // middle of new hit-chain
        Assert::AreEqual(sTooltip2, vmTrigger.GetHitChainTooltip(5)); // middle of new hit-chain
        Assert::AreEqual(sTooltip2, vmTrigger.GetHitChainTooltip(6)); // end of new hit-chain (24-1+48)
    }

    TEST_METHOD(TestBuildHitChainTooltipWithModifiers)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "C:0=0.5._I:0_A:0_C:0=0.50._N:0=0_O:0=0_P:0=0.70.");
        Assert::AreEqual({ 1U }, vmTrigger.Groups().Count());

        auto* cond = vmTrigger.Groups().GetItemAt(0)->m_pConditionSet->conditions;
        cond->current_hits = 2; cond = cond->next;  // AddHits    0=0 (5)
        cond->current_hits = 3; cond = cond->next;  // AddAddress 0
        cond->current_hits = 6; cond = cond->next;  // AddSource  0
        cond->current_hits = 12; cond = cond->next; // AddHits    0=0 (50)
        cond->current_hits = 24; cond = cond->next; // AndNext    0=0
        cond->current_hits = 1; cond = cond->next;  // OrNext     0=0
        cond->current_hits = 48;                    // PauseIf    0=0 (70)
        vmTrigger.DoFrame();

        const std::wstring sTooltip = L"+2 (condition 1)\n+12 (condition 4)\n+48 (condition 7)\n62/70 total";
        Assert::AreEqual(sTooltip, vmTrigger.GetHitChainTooltip(0)); // start of hit-chain
        Assert::AreEqual(sTooltip, vmTrigger.GetHitChainTooltip(1)); // non hit modifier
        Assert::AreEqual(sTooltip, vmTrigger.GetHitChainTooltip(2)); // non hit modifier
        Assert::AreEqual(sTooltip, vmTrigger.GetHitChainTooltip(3)); // middle of hits-chain
        Assert::AreEqual(sTooltip, vmTrigger.GetHitChainTooltip(4)); // non hit modifier
        Assert::AreEqual(sTooltip, vmTrigger.GetHitChainTooltip(5)); // non hit modifier
        Assert::AreEqual(sTooltip, vmTrigger.GetHitChainTooltip(6)); // end of hits-chain
    }

    TEST_METHOD(TestBuildHitChainTooltipNegativeTotal)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "C:0=0.5._D:0=0.10._0=0.20.");
        Assert::AreEqual({ 1U }, vmTrigger.Groups().Count());

        auto* cond = vmTrigger.Groups().GetItemAt(0)->m_pConditionSet->conditions;
        cond->current_hits = 2; cond = cond->next;  // AddHits 0=0 (5)
        cond->current_hits = 6; cond = cond->next;  // SubHits 0=0 (10)
        cond->current_hits = 1; cond = cond->next;  //         0=0 (20)
        vmTrigger.DoFrame();

        const std::wstring sTooltip = L"+2 (condition 1)\n-6 (condition 2)\n+1 (condition 3)\n-3/20 total";
        Assert::AreEqual(sTooltip, vmTrigger.GetHitChainTooltip(0));
        Assert::AreEqual(sTooltip, vmTrigger.GetHitChainTooltip(1));
        Assert::AreEqual(sTooltip, vmTrigger.GetHitChainTooltip(2));

        const auto* vmCondition = vmTrigger.Conditions().GetItemAt(2);
        Expects(vmCondition != nullptr);
        Assert::AreEqual(20U, vmCondition->GetRequiredHits());
        Assert::AreEqual(1U, vmCondition->GetCurrentHits());
        Assert::AreEqual(-3, vmCondition->GetTotalHits());
    }

    TEST_METHOD(TestAltHitChain)
    {
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "1=1SC:0xH1234=1_0=1.10.SC:0xH2345=1_0=1.10.");
        Assert::AreEqual({3U}, vmTrigger.Groups().Count());

        vmTrigger.Groups().GetItemAt(1)->m_pConditionSet->conditions->current_hits = 2;
        vmTrigger.Groups().GetItemAt(2)->m_pConditionSet->conditions->current_hits = 3;

        vmTrigger.SetSelectedGroupIndex(2);
        Assert::AreEqual(3U, vmTrigger.Conditions().GetItemAt(0)->GetCurrentHits());
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(0)->GetTotalHits());
        Assert::AreEqual(0U, vmTrigger.Conditions().GetItemAt(1)->GetCurrentHits());
        Assert::AreEqual(3, vmTrigger.Conditions().GetItemAt(1)->GetTotalHits());

        vmTrigger.SetSelectedGroupIndex(1);
        Assert::AreEqual(2U, vmTrigger.Conditions().GetItemAt(0)->GetCurrentHits());
        Assert::AreEqual(0, vmTrigger.Conditions().GetItemAt(0)->GetTotalHits());
        Assert::AreEqual(0U, vmTrigger.Conditions().GetItemAt(1)->GetCurrentHits());
        Assert::AreEqual(2, vmTrigger.Conditions().GetItemAt(1)->GetTotalHits());
    }

    TEST_METHOD(TestToggleMeasuredAsPercent)
    {
        std::array<uint8_t, 10> pMemory = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        TriggerViewModelHarness vmTrigger;
        Parse(vmTrigger, "M:0xH1234=16");
        Assert::AreEqual({ 1U }, vmTrigger.Conditions().Count());

        vmTrigger.InitializeMemory(&pMemory.at(0), pMemory.size());
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetAddress(8);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().SetSize(MemSize::EightBit);
        vmTrigger.mockWindowManager.MemoryInspector.Viewer().DoFrame(); // load viewer memory data

        vmTrigger.SetMeasuredTrackedAsPercent(true);
        Assert::AreEqual(std::string("G:0xH1234=16"), vmTrigger.Serialize());

        vmTrigger.SetMeasuredTrackedAsPercent(false);
        Assert::AreEqual(std::string("M:0xH1234=16"), vmTrigger.Serialize());
    }

    TEST_METHOD(TestToggleMeasuredAsPercentInNewAlt)
    {
        ra::data::models::AchievementModel pAchievement;
        TriggerViewModelHarness vmTrigger;
        vmTrigger.InitializeFrom(pAchievement.GetTrigger(), pAchievement.GetCapturedHits());
        vmTrigger.AddGroup();
        vmTrigger.SetSelectedGroupIndex(1);
        vmTrigger.NewCondition();
        auto* vmCondition = vmTrigger.Conditions().GetItemAt(0);
        Expects(vmCondition != nullptr);
        vmCondition->SetType(ra::services::TriggerConditionType::Measured);
        vmCondition->SetSourceType(ra::services::TriggerOperandType::Value);
        vmCondition->SetSourceValue(0);
        vmCondition->SetTargetType(ra::services::TriggerOperandType::Value);
        vmCondition->SetTargetValue(1U);

        vmTrigger.SetMeasuredTrackedAsPercent(true);
        Assert::AreEqual(std::string("SG:0=1S"), vmTrigger.Serialize());

        vmTrigger.SetMeasuredTrackedAsPercent(false);
        Assert::AreEqual(std::string("SM:0=1S"), vmTrigger.Serialize());
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
