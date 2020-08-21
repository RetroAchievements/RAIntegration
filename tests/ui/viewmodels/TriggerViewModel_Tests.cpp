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
        unsigned char pBuffer[768] = { 0 };
        const auto nSize = rc_trigger_size(sInput.c_str());
        Assert::IsTrue(nSize > 0 && nSize < sizeof(pBuffer));

        const rc_trigger_t* pTrigger = rc_parse_trigger(pBuffer, sInput.c_str(), nullptr, 0);
        Assert::IsNotNull(pTrigger);
        Ensures(pTrigger != nullptr);

        vmTrigger.InitializeFrom(*pTrigger);
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
        vmTrigger.Groups().GetItemAt(0)->Conditions().GetItemAt(0)->SetOperator(ra::ui::viewmodels::TriggerOperatorType::LessThan);
        Assert::AreEqual(std::string("0xH1234<0xH2345S0xX5555=1"), pMonitor.sNewTrigger);

        vmTrigger.Groups().GetItemAt(1)->Conditions().GetItemAt(0)->SetTargetValue(9U);
        Assert::AreEqual(std::string("0xH1234<0xH2345S0xX5555=9"), pMonitor.sNewTrigger);
    }

    TEST_METHOD(TestTriggerUpdateConditionsAddedRemoved)
    {
        TriggerViewModel vmTrigger;
        Parse(vmTrigger, "0xH1234=0xH2345S0xX5555=1");

        TriggerMonitor pMonitor(vmTrigger);
        vmTrigger.Groups().GetItemAt(0)->Conditions().Add();
        Assert::AreEqual(std::string("0xH1234=0xH2345_0xH0000=0S0xX5555=1"), pMonitor.sNewTrigger);

        vmTrigger.Groups().GetItemAt(0)->Conditions().RemoveAt(0);
        Assert::AreEqual(std::string("0xH0000=0S0xX5555=1"), pMonitor.sNewTrigger);

        vmTrigger.Groups().GetItemAt(1)->Conditions().Add();
        Assert::AreEqual(std::string("0xH0000=0S0xX5555=1_0xH0000=0"), pMonitor.sNewTrigger);

        vmTrigger.Groups().GetItemAt(1)->Conditions().RemoveAt(0);
        Assert::AreEqual(std::string("0xH0000=0S0xH0000=0"), pMonitor.sNewTrigger);
    }

    TEST_METHOD(TestTriggerUpdateConditionsAddedModified)
    {
        TriggerViewModel vmTrigger;
        Parse(vmTrigger, "0xH1234=0xH2345S0xX5555=1");

        TriggerMonitor pMonitor(vmTrigger);
        vmTrigger.Groups().GetItemAt(0)->Conditions().Add();
        Assert::AreEqual(std::string("0xH1234=0xH2345_0xH0000=0S0xX5555=1"), pMonitor.sNewTrigger);

        vmTrigger.Groups().GetItemAt(0)->Conditions().GetItemAt(1)->SetSourceValue(1234U);
        Assert::AreEqual(std::string("0xH1234=0xH2345_0xH04d2=0S0xX5555=1"), pMonitor.sNewTrigger);
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
