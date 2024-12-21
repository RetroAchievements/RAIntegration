#include "CppUnitTest.h"

#include "services\AchievementRuntime.hh"

#include "ui\EditorTheme.hh"
#include "ui\viewmodels\TriggerConditionViewModel.hh"
#include "ui\viewmodels\TriggerViewModel.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockConfiguration.hh"
#include "tests\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockUserContext.hh"
#include "tests\ui\viewmodels\TriggerConditionAsserts.hh"

#include <rcheevos\src\rcheevos\rc_internal.h>

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
        ra::data::context::mocks::MockUserContext mockUserContext;
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
        bool HasSourceValue() const { return GetValue(HasSourceValueProperty); }
        bool HasTargetSize() const { return GetValue(HasTargetSizeProperty); }
        bool HasTargetValue() const { return GetValue(HasTargetValueProperty); }
        bool HasTarget() const { return GetValue(HasTargetProperty); }
        bool HasHits() const { return GetValue(HasHitsProperty); }
        bool CanEditHits() const { return GetValue(CanEditHitsProperty); }

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

        TriggerConditionViewModelHarness vmCondition;
        vmCondition.InitializeFrom(*pTrigger->requirement->conditions);

        const std::string sOutput = vmCondition.Serialize();
        Assert::AreEqual(sInput, sOutput);
    }

    void ParseAndRegenerateValue(const std::string& sInput)
    {
        unsigned char pBuffer[512] = { 0 };
        const auto nSize = rc_value_size(sInput.c_str());
        Assert::IsTrue(nSize > 0 && nSize < sizeof(pBuffer));

        const rc_value_t* pValue = rc_parse_value(pBuffer, sInput.c_str(), nullptr, 0);
        Assert::IsNotNull(pValue);
        Ensures(pValue != nullptr);

        TriggerConditionViewModelHarness vmCondition;
        vmCondition.InitializeFrom(*pValue->conditions->conditions);

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
        Assert::AreEqual(std::wstring(L"0"), vmCondition.GetSourceValue());
        Assert::AreEqual(TriggerOperatorType::Equals, vmCondition.GetOperator());
        Assert::AreEqual(TriggerOperandType::Value, vmCondition.GetTargetType());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetTargetSize());
        Assert::AreEqual(std::wstring(L"0"), vmCondition.GetTargetValue());
        Assert::AreEqual(0U, vmCondition.GetCurrentHits());
        Assert::AreEqual(0U, vmCondition.GetRequiredHits());

        Assert::IsTrue(vmCondition.HasSourceSize());
        Assert::IsFalse(vmCondition.HasTargetSize());
        Assert::IsTrue(vmCondition.HasTarget());
        Assert::IsTrue(vmCondition.HasHits());
        Assert::IsTrue(vmCondition.CanEditHits());
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

        vmCondition.SetSourceType(TriggerOperandType::Inverted);
        Assert::AreEqual(TriggerOperandType::Inverted, vmCondition.GetSourceType());
        Assert::IsTrue(vmCondition.HasSourceSize());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetSourceSize());

        vmCondition.SetSourceType(TriggerOperandType::Recall);
        Assert::AreEqual(TriggerOperandType::Recall, vmCondition.GetSourceType());
        Assert::IsFalse(vmCondition.HasSourceSize());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetSourceSize());

        vmCondition.SetSourceType(TriggerOperandType::Value);
        Assert::AreEqual(TriggerOperandType::Value, vmCondition.GetSourceType());
        Assert::IsFalse(vmCondition.HasSourceSize());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetSourceSize()); // value always 32-bit

        vmCondition.SetSourceType(TriggerOperandType::Address);
        Assert::AreEqual(TriggerOperandType::Address, vmCondition.GetSourceType());
        Assert::IsTrue(vmCondition.HasSourceSize());
        Assert::AreEqual(MemSize::SixteenBit, vmCondition.GetSourceSize()); // copied from target size
    }

    TEST_METHOD(TestHasSourceValue)
    {
        TriggerConditionViewModelHarness vmCondition;
        vmCondition.SetTargetSize(MemSize::SixteenBit);

        Assert::AreEqual(TriggerOperandType::Address, vmCondition.GetSourceType());
        Assert::IsTrue(vmCondition.HasSourceValue());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetSourceSize()); // initial size is 8-bit

        vmCondition.SetSourceType(TriggerOperandType::BCD);
        Assert::AreEqual(TriggerOperandType::BCD, vmCondition.GetSourceType());
        Assert::IsTrue(vmCondition.HasSourceValue());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetSourceSize());

        vmCondition.SetSourceType(TriggerOperandType::Delta);
        Assert::AreEqual(TriggerOperandType::Delta, vmCondition.GetSourceType());
        Assert::IsTrue(vmCondition.HasSourceValue());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetSourceSize());

        vmCondition.SetSourceType(TriggerOperandType::Prior);
        Assert::AreEqual(TriggerOperandType::Prior, vmCondition.GetSourceType());
        Assert::IsTrue(vmCondition.HasSourceValue());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetSourceSize());

        vmCondition.SetSourceType(TriggerOperandType::Inverted);
        Assert::AreEqual(TriggerOperandType::Inverted, vmCondition.GetSourceType());
        Assert::IsTrue(vmCondition.HasSourceValue());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetSourceSize());

        vmCondition.SetSourceType(TriggerOperandType::Recall);
        Assert::AreEqual(TriggerOperandType::Recall, vmCondition.GetSourceType());
        Assert::IsFalse(vmCondition.HasSourceValue());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetSourceSize());

        vmCondition.SetSourceType(TriggerOperandType::Value);
        Assert::AreEqual(TriggerOperandType::Value, vmCondition.GetSourceType());
        Assert::IsTrue(vmCondition.HasSourceValue());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetSourceSize()); // value always 32-bit

        vmCondition.SetSourceType(TriggerOperandType::Address);
        Assert::AreEqual(TriggerOperandType::Address, vmCondition.GetSourceType());
        Assert::IsTrue(vmCondition.HasSourceValue());
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

        vmCondition.SetOperator(TriggerOperatorType::Modulus);
        Assert::AreEqual(TriggerOperatorType::Modulus, vmCondition.GetOperator());
        Assert::IsTrue(vmCondition.HasTarget());

        vmCondition.SetOperator(TriggerOperatorType::Add);
        Assert::AreEqual(TriggerOperatorType::Add, vmCondition.GetOperator());
        Assert::IsTrue(vmCondition.HasTarget());

        vmCondition.SetOperator(TriggerOperatorType::Subtract);
        Assert::AreEqual(TriggerOperatorType::Subtract, vmCondition.GetOperator());
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

        vmCondition.SetTargetType(TriggerOperandType::Recall);
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

        vmCondition.SetTargetType(TriggerOperandType::Value);
        Assert::IsFalse(vmCondition.HasTargetSize());
        Assert::IsFalse(vmCondition.HasTarget());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetTargetSize()); // value always 32-bit

        vmCondition.SetTargetType(TriggerOperandType::Inverted);
        Assert::IsFalse(vmCondition.HasTargetSize());
        Assert::IsFalse(vmCondition.HasTarget());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetTargetSize()); // size copied from source

        vmCondition.SetOperator(TriggerOperatorType::NotEquals);
        Assert::IsTrue(vmCondition.HasTargetSize());
        Assert::IsTrue(vmCondition.HasTarget());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetTargetSize());
    }

    TEST_METHOD(TestHasTargetValue)
    {
        TriggerConditionViewModelHarness vmCondition;
        Assert::AreEqual(TriggerOperandType::Value, vmCondition.GetTargetType());
        Assert::AreEqual(TriggerOperatorType::Equals, vmCondition.GetOperator());
        Assert::IsTrue(vmCondition.HasTargetValue());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetTargetSize()); // value always 32-bit

        vmCondition.SetTargetType(TriggerOperandType::Address);
        Assert::IsTrue(vmCondition.HasTargetValue());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetTargetSize()); // size copied from source

        vmCondition.SetTargetType(TriggerOperandType::Value);
        Assert::IsTrue(vmCondition.HasTargetValue());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetTargetSize()); // value always 32-bit

        vmCondition.SetTargetType(TriggerOperandType::Delta);
        Assert::IsTrue(vmCondition.HasTargetValue());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetTargetSize()); // size copied from source

        vmCondition.SetTargetType(TriggerOperandType::Inverted);
        Assert::IsTrue(vmCondition.HasTargetValue());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetTargetSize());

        vmCondition.SetTargetType(TriggerOperandType::BCD);
        Assert::IsTrue(vmCondition.HasTargetValue());
        Assert::AreEqual(MemSize::EightBit, vmCondition.GetTargetSize());

        vmCondition.SetTargetType(TriggerOperandType::Recall);
        Assert::IsFalse(vmCondition.HasTargetValue());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetTargetSize()); // value always 32-bit

        vmCondition.SetOperator(TriggerOperatorType::None);
        Assert::IsFalse(vmCondition.HasTargetValue());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetTargetSize()); // value still 32-bit

        vmCondition.SetOperator(TriggerOperatorType::NotEquals);
        Assert::IsFalse(vmCondition.HasTargetValue());
        Assert::AreEqual(MemSize::ThirtyTwoBit, vmCondition.GetTargetSize());
    }

    TEST_METHOD(TestModifyingConditionUpdatesHasTargetValue)
    {
        TriggerConditionViewModelHarness vmCondition;
        vmCondition.SetType(TriggerConditionType::AddSource);
        Assert::IsFalse(vmCondition.HasTargetValue());
        vmCondition.SetOperator(TriggerOperatorType::Divide);
        Assert::IsTrue(vmCondition.HasTargetValue());
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

        vmCondition.SetTargetType(TriggerOperandType::Inverted);
        Assert::AreEqual(TriggerOperandType::Inverted, vmCondition.GetTargetType());
        Assert::IsTrue(vmCondition.HasTargetSize());

        vmCondition.SetTargetType(TriggerOperandType::Recall);
        Assert::AreEqual(TriggerOperandType::Recall, vmCondition.GetTargetType());
        Assert::IsFalse(vmCondition.HasTargetSize());

        vmCondition.SetTargetType(TriggerOperandType::Value);
        Assert::AreEqual(TriggerOperandType::Value, vmCondition.GetTargetType());
        Assert::IsFalse(vmCondition.HasTargetSize());
    }

    TEST_METHOD(TestHasHits)
    {
        TriggerConditionViewModelHarness vmCondition;
        Assert::AreEqual(TriggerConditionType::Standard, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());
        Assert::IsTrue(vmCondition.CanEditHits());

        vmCondition.SetType(TriggerConditionType::AddAddress);
        Assert::AreEqual(TriggerConditionType::AddAddress, vmCondition.GetType());
        Assert::IsFalse(vmCondition.HasHits());
        Assert::IsFalse(vmCondition.CanEditHits());

        vmCondition.SetType(TriggerConditionType::AddHits);
        Assert::AreEqual(TriggerConditionType::AddHits, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());
        Assert::IsTrue(vmCondition.CanEditHits());

        vmCondition.SetType(TriggerConditionType::AddSource);
        Assert::AreEqual(TriggerConditionType::AddSource, vmCondition.GetType());
        Assert::IsFalse(vmCondition.HasHits());
        Assert::IsFalse(vmCondition.CanEditHits());

        vmCondition.SetType(TriggerConditionType::AndNext);
        Assert::AreEqual(TriggerConditionType::AndNext, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());
        Assert::IsTrue(vmCondition.CanEditHits());

        vmCondition.SetType(TriggerConditionType::Measured);
        Assert::AreEqual(TriggerConditionType::Measured, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());
        Assert::IsTrue(vmCondition.CanEditHits());

        vmCondition.SetType(TriggerConditionType::MeasuredIf);
        Assert::AreEqual(TriggerConditionType::MeasuredIf, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());
        Assert::IsTrue(vmCondition.CanEditHits());

        vmCondition.SetType(TriggerConditionType::OrNext);
        Assert::AreEqual(TriggerConditionType::OrNext, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());
        Assert::IsTrue(vmCondition.CanEditHits());

        vmCondition.SetType(TriggerConditionType::PauseIf);
        Assert::AreEqual(TriggerConditionType::PauseIf, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());
        Assert::IsTrue(vmCondition.CanEditHits());

        vmCondition.SetType(TriggerConditionType::ResetIf);
        Assert::AreEqual(TriggerConditionType::ResetIf, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());
        Assert::IsTrue(vmCondition.CanEditHits());

        vmCondition.SetType(TriggerConditionType::SubHits);
        Assert::AreEqual(TriggerConditionType::SubHits, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());
        Assert::IsTrue(vmCondition.CanEditHits());

        vmCondition.SetType(TriggerConditionType::SubSource);
        Assert::AreEqual(TriggerConditionType::SubSource, vmCondition.GetType());
        Assert::IsFalse(vmCondition.HasHits());
        Assert::IsFalse(vmCondition.CanEditHits());

        vmCondition.SetType(TriggerConditionType::Trigger);
        Assert::AreEqual(TriggerConditionType::Trigger, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());
        Assert::IsTrue(vmCondition.CanEditHits());

        vmCondition.SetType(TriggerConditionType::Remember);
        Assert::AreEqual(TriggerConditionType::Remember, vmCondition.GetType());
        Assert::IsFalse(vmCondition.HasHits());
        Assert::IsFalse(vmCondition.CanEditHits());
    }

    TEST_METHOD(TestHasHitsForValue)
    {
        TriggerConditionViewModelHarness vmCondition;
        TriggerViewModel vmTrigger;
        vmTrigger.SetIsValue(true);
        vmCondition.SetTriggerViewModel(&vmTrigger);

        Assert::AreEqual(TriggerConditionType::Standard, vmCondition.GetType());
        Assert::IsTrue(vmCondition.HasHits());
        Assert::IsTrue(vmCondition.CanEditHits());

        vmCondition.SetType(TriggerConditionType::Measured);
        Assert::AreEqual(TriggerConditionType::Measured, vmCondition.GetType());
        Assert::AreEqual(TriggerOperatorType::Equals, vmCondition.GetOperator());
        Assert::IsTrue(vmCondition.HasHits());
        Assert::IsFalse(vmCondition.CanEditHits());

        vmCondition.SetOperator(TriggerOperatorType::None);
        Assert::AreEqual(TriggerConditionType::Measured, vmCondition.GetType());
        Assert::AreEqual(TriggerOperatorType::None, vmCondition.GetOperator());
        Assert::IsFalse(vmCondition.HasHits());
        Assert::IsFalse(vmCondition.CanEditHits());

        vmCondition.SetType(TriggerConditionType::Standard);
        Assert::AreEqual(TriggerConditionType::Standard, vmCondition.GetType());
        Assert::AreEqual(TriggerOperatorType::Equals, vmCondition.GetOperator());
        Assert::IsTrue(vmCondition.HasHits());
        Assert::IsTrue(vmCondition.CanEditHits());

        vmCondition.SetRequiredHits(5U);
        Assert::AreEqual(5U, vmCondition.GetRequiredHits());

        vmCondition.SetType(TriggerConditionType::Measured);
        Assert::AreEqual(TriggerConditionType::Measured, vmCondition.GetType());
        Assert::AreEqual(TriggerOperatorType::Equals, vmCondition.GetOperator());
        Assert::IsTrue(vmCondition.HasHits());
        Assert::IsFalse(vmCondition.CanEditHits());
        Assert::AreEqual(0U, vmCondition.GetRequiredHits());

        vmCondition.SetOperator(TriggerOperatorType::Multiply);
        Assert::AreEqual(TriggerConditionType::Measured, vmCondition.GetType());
        Assert::AreEqual(TriggerOperatorType::Multiply, vmCondition.GetOperator());
        Assert::IsFalse(vmCondition.HasHits());
        Assert::IsFalse(vmCondition.CanEditHits());
        Assert::AreEqual(0U, vmCondition.GetRequiredHits());
    }

    TEST_METHOD(TestSerializeHasHits)
    {
        TriggerConditionViewModelHarness vmCondition;
        vmCondition.Parse("0xH1234=0xH2345.10.");
        std::string sSerialized = vmCondition.Serialize();
        Assert::AreEqual(std::string("0xH1234=0xH2345.10."), sSerialized);

        // changing to a type that supports hits does not change the hit count
        vmCondition.SetType(TriggerConditionType::AddHits);
        sSerialized = vmCondition.Serialize();
        Assert::AreEqual(std::string("C:0xH1234=0xH2345.10."), sSerialized);

        // changing to a type that does not support hits should remove the hit count (and comparison)
        vmCondition.SetType(TriggerConditionType::AddSource);
        sSerialized = vmCondition.Serialize();
        Assert::AreEqual(std::string("A:0xH1234"), sSerialized);

        // changing back to a type that does support hits should restore the hit count and comparison
        // NOTE: editor does not behave this way as the condition is reconstructed from the serialized trigger
        vmCondition.SetType(TriggerConditionType::ResetIf);
        sSerialized = vmCondition.Serialize();
        Assert::AreEqual(std::string("R:0xH1234=0xH2345.10."), sSerialized);

        // changing to a type that does not support hits should remove the hit count (and comparison)
        vmCondition.SetType(TriggerConditionType::AddAddress);
        sSerialized = vmCondition.Serialize();
        Assert::AreEqual(std::string("I:0xH1234"), sSerialized);

        // changing to a type that does not support hits should remove the hit count (and comparison)
        vmCondition.SetType(TriggerConditionType::SubSource);
        sSerialized = vmCondition.Serialize();
        Assert::AreEqual(std::string("B:0xH1234"), sSerialized);

        // changing back to a type that does support hits should restore the hit count and comparison
        vmCondition.SetType(TriggerConditionType::Standard);
        sSerialized = vmCondition.Serialize();
        Assert::AreEqual(std::string("0xH1234=0xH2345.10."), sSerialized);

        // changing to a type that does not support hits should remove the hit count (and comparison)
        vmCondition.SetType(TriggerConditionType::Remember);
        sSerialized = vmCondition.Serialize();
        Assert::AreEqual(std::string("K:0xH1234"), sSerialized);
    }

    TEST_METHOD(TestSizes)
    {
        ParseAndRegenerate("0xH1234=0xH2345"); // 8-bit
        ParseAndRegenerate("0x 1234=0x 2345"); // 16-bit
        ParseAndRegenerate("0xW1234=0xW2345"); // 24-bit
        ParseAndRegenerate("0xX1234=0xX2345"); // 32-bit
        ParseAndRegenerate("0xI1234=0xI2345"); // 16-bit BE
        ParseAndRegenerate("0xJ1234=0xJ2345"); // 24-bit BE
        ParseAndRegenerate("0xG1234=0xG2345"); // 32-bit BE
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
        ParseAndRegenerate("fF1234=fF2345"); // float
        ParseAndRegenerate("fM1234=fM2345"); // mbf32
        ParseAndRegenerate("fL1234=fL2345"); // mbf32le
    }

    TEST_METHOD(TestOperandTypes)
    {
        ParseAndRegenerate("0xH1234=0xH1234"); // address
        ParseAndRegenerate("0xH1234=d0xH1234"); // delta
        ParseAndRegenerate("0xH1234=p0xH1234"); // prior
        ParseAndRegenerate("0xH1234=b0xH1234"); // bcd
        ParseAndRegenerate("0xH1234=~0xH1234"); // inverted
        ParseAndRegenerate("{recall}={recall}"); // recall
        ParseAndRegenerate("0xH1234=1234"); // value
        ParseAndRegenerate("0xH1234=f12.34"); // float
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
        ParseAndRegenerate("A:0xH1234%5"); // modulus
        ParseAndRegenerate("A:0xH1234+5"); // addition
        ParseAndRegenerate("A:0xH1234-5"); // subtraction
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
        ParseAndRegenerate("K:0xH1234"); // remember
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
        ParseAndRegenerateValue("M:0xH1234=5"); // measured without hit target
        ParseAndRegenerateValue("M:0xH1234"); // measured without comparison
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
        Assert::AreEqual(std::wstring(L"0x000c (0x0008+4)\r\n[8 bytes] This is a note."), condition.GetTooltip(TriggerConditionViewModel::SourceValueProperty));
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
        ra::data::context::mocks::MockUserContext mockUserContext;
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

        void CodeNotesDoFrame()
        {
            auto* pCodeNotes = mockGameContext.Assets().FindCodeNotes();
            if (pCodeNotes != nullptr)
                pCodeNotes->DoFrame();
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
        Assert::AreEqual(std::wstring(L"0x0003 (indirect 0x0001+0x02)\r\n[No code note]"), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 3, 3+2 = $0005
        vmTrigger.SetMemory({ 1 }, 3);
        Assert::AreEqual(std::wstring(L"0x0005 (indirect 0x0001+0x02)\r\n[No code note]"), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipIndirectAddressCodeNote)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.Parse("I:0xH0001_0xH0002=3");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        vmTrigger.mockGameContext.SetCodeNote({ 1U }, L"[8-bit pointer]\n+2=This is a note.");

        const auto* pCondition = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::IsTrue(pCondition->IsIndirect());

        // $0001 = 1, 1+2 = $0003
        Assert::AreEqual(std::wstring(L"0x0003 (indirect 0x0001+0x02)\r\nThis is a note."), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 3, 3+2 = $0005
        vmTrigger.SetMemory({ 1 }, 3);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x0005 (indirect 0x0001+0x02)\r\nThis is a note."), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipIndirectAddressMultiNote)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.Parse("I:0xH0001_0xH0002=0xH0004");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        vmTrigger.mockGameContext.SetCodeNote({ 1U }, L"[8-bit pointer]\n+2=This is a note.\n+4=This is another note.");

        const auto* pCondition = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::IsTrue(pCondition->IsIndirect());

        // $0001 = 1, 1+2 = $0003, 1+4 = $0005
        Assert::AreEqual(std::wstring(L"0x0003 (indirect 0x0001+0x02)\r\nThis is a note."), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0005 (indirect 0x0001+0x04)\r\nThis is another note."), pCondition->GetTooltip(TriggerConditionViewModel::TargetValueProperty));

        // $0001 = 3, 3+2 = $0005, 3+4 = $0007
        vmTrigger.SetMemory({ 1 }, 3);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x0005 (indirect 0x0001+0x02)\r\nThis is a note."), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0007 (indirect 0x0001+0x04)\r\nThis is another note."), pCondition->GetTooltip(TriggerConditionViewModel::TargetValueProperty));
    }

    TEST_METHOD(TestTooltipIndirectAddressMultiByteNote)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.Parse("I:0xH0001_0xH0002=0xH0005");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        vmTrigger.mockGameContext.SetCodeNote({ 1U }, L"[8-bit pointer]\n+4=[8 bytes] This is a note.");

        const auto* pCondition = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::IsTrue(pCondition->IsIndirect());

        // $0001 = 1, 1+2 = $0003, 1+5 = $0006
        Assert::AreEqual(std::wstring(L"0x0003 (indirect 0x0001+0x02)\r\n[No code note]"), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0006 (indirect 0x0001+0x05)\r\n[8 bytes] This is a note."), pCondition->GetTooltip(TriggerConditionViewModel::TargetValueProperty));
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
        Assert::AreEqual(std::wstring(L"0x0005 (indirect 0x0001+0x02)\r\n[No code note]"), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 3, 3*3+2 = $000B
        vmTrigger.SetMemory({ 1 }, 3);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x000b (indirect 0x0001+0x02)\r\n[No code note]"), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipIndirectAddressWithAltCodeNote)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.Parse("I:0xH0001_0xH0002=3S0=0S1=1");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        vmTrigger.mockGameContext.SetCodeNote({ 1U }, L"[8-bit pointer]\n+2=This is a note.");

        const auto* pCondition = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::IsTrue(pCondition->IsIndirect());

        // $0001 = 1, 1+2 = $0003
        Assert::AreEqual(std::wstring(L"0x0003 (indirect 0x0001+0x02)\r\nThis is a note."), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 3, 3+2 = $0005
        vmTrigger.SetMemory({ 1 }, 3);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x0005 (indirect 0x0001+0x02)\r\nThis is a note."),
                         pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipIndirectAddressInAlt1CodeNote)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.Parse("0=0SI:0xH0001_0xH0002=3S1=1");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        vmTrigger.mockGameContext.SetCodeNote({ 1U }, L"[8-bit pointer]\n+2=This is a note.");
        vmTrigger.SetSelectedGroupIndex(1);

        const auto* pCondition = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::IsTrue(pCondition->IsIndirect());

        // $0001 = 1, 1+2 = $0003
        Assert::AreEqual(std::wstring(L"0x0003 (indirect 0x0001+0x02)\r\nThis is a note."), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 3, 3+2 = $0005
        vmTrigger.SetMemory({ 1 }, 3);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x0005 (indirect 0x0001+0x02)\r\nThis is a note."), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipIndirectAddressInAlt2CodeNote)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.Parse("0=0S1=1SI:0xH0001_0xH0002=3");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        vmTrigger.mockGameContext.SetCodeNote({ 1U }, L"[8-bit pointer]\n+2=This is a note.");
        vmTrigger.SetSelectedGroupIndex(2);

        const auto* pCondition = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::IsTrue(pCondition->IsIndirect());

        // $0001 = 1, 1+2 = $0003
        Assert::AreEqual(std::wstring(L"0x0003 (indirect 0x0001+0x02)\r\nThis is a note."), pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 3, 3+2 = $0005
        vmTrigger.SetMemory({ 1 }, 3);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x0005 (indirect 0x0001+0x02)\r\nThis is a note."),
                         pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipIndirectAddressCodeNoteOverflow)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        vmTrigger.Parse("I:0xX0000_0xH80000002=6");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        vmTrigger.mockGameContext.SetCodeNote({0U}, L"[32-bit pointer]\n+0x80000002=This is a note.");

        const auto* pCondition = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::IsTrue(pCondition->IsIndirect());

        // $0001 = 0x80000003, 0x80000003+0x80000002 = 0x100000005 (too big for uint32_t) -> truncated to $0005
        vmTrigger.SetMemory({0}, 0x03);
        vmTrigger.SetMemory({1}, 0x00);
        vmTrigger.SetMemory({2}, 0x00);
        vmTrigger.SetMemory({3}, 0x80);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x0005 (indirect 0x0000+0x80000002)\r\nThis is a note."),
                         pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 0x80000004, 0x8000004+0x80000002 = 0x100000006 (too big for uint32_t) -> truncated to $0006
        vmTrigger.SetMemory({0}, 4);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x0006 (indirect 0x0000+0x80000002)\r\nThis is a note."),
                         pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipIndirectAddressCodeNoteMasked)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        ra::data::context::mocks::MockConsoleContext mockConsoleContext(ConsoleID::PSP, L"PSP");
        vmTrigger.Parse("I:0xX0000&33554431_0xH0002=6"); // 33554431 = 0x01FFFFFF
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        vmTrigger.mockGameContext.SetCodeNote({0U}, L"[32-bit pointer]\n+2=This is a note.");

        const auto* pCondition = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition != nullptr);
        Assert::IsTrue(pCondition->IsIndirect());

        // $0001 = 0x08000003, 0x80000003&0x01FFFFFF=0x00000003+2 = 0x00000005
        vmTrigger.SetMemory({0}, 0x03);
        vmTrigger.SetMemory({1}, 0x00);
        vmTrigger.SetMemory({2}, 0x00);
        vmTrigger.SetMemory({3}, 0x08);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x0005 (indirect 0x0000+0x02)\r\nThis is a note."),
                         pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 0x80000004, 0x8000004+0x80000002 = 0x100000006 (too big for uint32_t) -> truncated to $0006
        vmTrigger.SetMemory({0}, 4);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x0006 (indirect 0x0000+0x02)\r\nThis is a note."),
                         pCondition->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipDoubleIndirectAddress)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        vmTrigger.mockGameContext.InitializeCodeNotes();
        vmTrigger.Parse("I:0xH0001_I:0xH0002_0xH0003=4");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        vmTrigger.mockGameContext.SetCodeNote({1U}, L"[8-bit pointer]\n+2=First Level [8-bit pointer]\n  +3=Second Level.");

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
        Assert::AreEqual(std::wstring(L"0x0001\r\n[8-bit pointer]"), pCondition1->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0003 (indirect 0x0001+0x02)\r\nFirst Level [8-bit pointer]"), pCondition2->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0006 (indirect 0x0001+0x02+0x03)\r\nSecond Level."), pCondition3->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 3, 3+2 = $0005, $0005 = 5, 5+3 = $0008
        vmTrigger.SetMemory({ 1 }, 3);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x0001\r\n[8-bit pointer]"), pCondition1->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0005 (indirect 0x0001+0x02)\r\nFirst Level [8-bit pointer]"), pCondition2->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0008 (indirect 0x0001+0x02+0x03)\r\nSecond Level."), pCondition3->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipDoubleIndirectAddressExternal)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        vmTrigger.Parse("I:0xH0001_I:0xH0002_0xH0003=4");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);

        // Neat trick! Get the trigger managed by the viewmodel, and reinitialize the viewmodel using it.
        // It'll rebuild itself as if the trigger was external (i.e. from the runtime), but without destroying it.
        // This leverages the fact that InitializeFrom only clears out the pointer, not the buffer.
        auto* pTrigger = vmTrigger.GetTriggerFromString();
        Expects(pTrigger != nullptr);
        vmTrigger.InitializeFrom(*pTrigger);

        // manually update the memrefs as if rc_runtime_do_frame where called
        auto* memrefs = rc_trigger_get_memrefs(pTrigger);
        rc_update_memref_values(memrefs, rc_peek_callback, nullptr);

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
        Assert::AreEqual(std::wstring(L"0x0003 (indirect 0x0001+0x02)\r\n[No code note]"), pCondition2->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0006 (indirect 0x0001+0x02+0x03)\r\n[No code note]"), pCondition3->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // Calling GetTooltip on a viewmodel attached to an external trigger should not evaluate the memrefs.
        // External memrefs should only be updated by the runtime to ensure delta values are correct.
        vmTrigger.SetMemory({ 1 }, 3);

        // Manually update the memrefs as if rc_runtime_do_frame where called
        rc_update_memref_values(memrefs, rc_peek_callback, nullptr);

        // $0001 = 3, 3+2 = $0005, $0005 = 5, 5+3 = $0008
        Assert::AreEqual(std::wstring(L"0x0001\r\n[No code note]"), pCondition1->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0005 (indirect 0x0001+0x02)\r\n[No code note]"), pCondition2->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0008 (indirect 0x0001+0x02+0x03)\r\n[No code note]"), pCondition3->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipDoubleIndirectAddressValue)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        ra::data::context::mocks::MockConsoleContext mockConsoleContext;
        vmTrigger.SetIsValue(true);
        vmTrigger.Parse("I:0xH0001_I:0xH0002_M:0xH0003=4");
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
        Assert::AreEqual(std::wstring(L"0x0003 (indirect 0x0001+0x02)\r\n[No code note]"), pCondition2->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0006 (indirect 0x0001+0x02+0x03)\r\n[No code note]"), pCondition3->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 3, 3+2 = $0005, $0005 = 5, 5+3 = $0008
        vmTrigger.SetMemory({ 1 }, 3);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x0001\r\n[No code note]"),
                         pCondition1->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0005 (indirect 0x0001+0x02)\r\n[No code note]"), pCondition2->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0008 (indirect 0x0001+0x02+0x03)\r\n[No code note]"), pCondition3->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipRecallBasic)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.mockGameContext.InitializeCodeNotes();
        vmTrigger.Parse("K:0xH0001_I:{recall}_I:0xH0002_0xH0003=4");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        vmTrigger.mockGameContext.SetCodeNote({1U}, L"[8-bit pointer]\n+2=First Level (8-bit pointer)\n  +3=Second Level.");

        const auto* pCondition1 = vmTrigger.Conditions().GetItemAt(0);
        Expects(pCondition1 != nullptr);
        const auto* pCondition2 = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition2 != nullptr);
        const auto* pCondition3 = vmTrigger.Conditions().GetItemAt(2);
        Expects(pCondition3 != nullptr);
        const auto* pCondition4 = vmTrigger.Conditions().GetItemAt(3);
        Expects(pCondition4 != nullptr);

        Assert::IsFalse(pCondition1->IsIndirect());
        Assert::IsFalse(pCondition2->IsIndirect());
        Assert::IsTrue(pCondition3->IsIndirect());
        Assert::IsTrue(pCondition4->IsIndirect());

        // $0001 = 1, 1+2 = $0003, $0003 = 3, 3+3 = $0006
        Assert::AreEqual(std::wstring(L""), pCondition2->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0003 (indirect {recall}+0x02)\r\nFirst Level (8-bit pointer)"), pCondition3->GetTooltip(TriggerConditionViewModel::SourceValueProperty)); //Kind of a quirk because recall value is a code-noted address.
        Assert::AreEqual(std::wstring(L"0x0006 (indirect {recall}+0x02+0x03)\r\nSecond Level."), pCondition4->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 3, 3+2 = $0005, $0005 = 5, 5+3 = $0008
        vmTrigger.SetMemory({ 1 }, 3);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x0005 (indirect {recall}+0x02)\r\nFirst Level (8-bit pointer)"), pCondition3->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0008 (indirect {recall}+0x02+0x03)\r\nSecond Level."), pCondition4->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipRecallAddSource)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.mockGameContext.InitializeCodeNotes();
        vmTrigger.Parse("A:1_K:0xH0001_I:{recall}_I:0xH0002_0xH0003=4");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);

        const auto* pCondition1 = vmTrigger.Conditions().GetItemAt(0);
        Expects(pCondition1 != nullptr);
        const auto* pCondition2 = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition2 != nullptr);
        const auto* pCondition3 = vmTrigger.Conditions().GetItemAt(2);
        Expects(pCondition3 != nullptr);
        const auto* pCondition4 = vmTrigger.Conditions().GetItemAt(3);
        Expects(pCondition4 != nullptr);
        const auto* pCondition5 = vmTrigger.Conditions().GetItemAt(4);
        Expects(pCondition5 != nullptr);

        Assert::IsFalse(pCondition1->IsIndirect());
        Assert::IsFalse(pCondition2->IsIndirect());
        Assert::IsFalse(pCondition3->IsIndirect());
        Assert::IsTrue(pCondition4->IsIndirect());
        Assert::IsTrue(pCondition5->IsIndirect());

        // $0001 = 1, 1+1+2 = $0004, $0004 = 4, 4+3 = $0007
        Assert::AreEqual(std::wstring(L"0x0004 (indirect {recall}+0x02)\r\n[No code note]"),
                         pCondition4->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0007 (indirect {recall}+0x02+0x03)\r\n[No code note]"), pCondition5->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 3, 1+3+2 = $0006, $0006 = 6, 6+3 = $0009
        vmTrigger.SetMemory({ 1 }, 3);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x0006 (indirect {recall}+0x02)\r\n[No code note]"), pCondition4->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0009 (indirect {recall}+0x02+0x03)\r\n[No code note]"), pCondition5->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipRecallSubSource)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.mockGameContext.InitializeCodeNotes();
        vmTrigger.Parse("B:2_K:0xH0003_I:{recall}_I:0xH0002_0xH0003=4");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);

        const auto* pCondition1 = vmTrigger.Conditions().GetItemAt(0);
        Expects(pCondition1 != nullptr);
        const auto* pCondition2 = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition2 != nullptr);
        const auto* pCondition3 = vmTrigger.Conditions().GetItemAt(2);
        Expects(pCondition3 != nullptr);
        const auto* pCondition4 = vmTrigger.Conditions().GetItemAt(3);
        Expects(pCondition4 != nullptr);
        const auto* pCondition5 = vmTrigger.Conditions().GetItemAt(4);
        Expects(pCondition5 != nullptr);

        Assert::IsFalse(pCondition1->IsIndirect());
        Assert::IsFalse(pCondition2->IsIndirect());
        Assert::IsFalse(pCondition3->IsIndirect());
        Assert::IsTrue(pCondition4->IsIndirect());
        Assert::IsTrue(pCondition5->IsIndirect());

        // $0003 = 3, 3-2+2 = $0003, $0003 = 3, 3+3 = $0006
        Assert::AreEqual(std::wstring(L"0x0003 (indirect {recall}+0x02)\r\n[No code note]"), pCondition4->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0006 (indirect {recall}+0x02+0x03)\r\n[No code note]"), pCondition5->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0003 = 5, 5-2 +2 = $0005, 5+4 = $0008
        vmTrigger.SetMemory({ 3 }, 5);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x0005 (indirect {recall}+0x02)\r\n[No code note]"), pCondition4->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0008 (indirect {recall}+0x02+0x03)\r\n[No code note]"), pCondition5->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipRecallPauseIfEarly)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.mockGameContext.InitializeCodeNotes();
        vmTrigger.Parse("K:0xH0001_P:0=1_I:{recall}_I:0xH0002_0xH0003=4");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        vmTrigger.mockGameContext.SetCodeNote({ 1U }, L"[8-bit pointer]\n+2=First Level (8-bit pointer)\n  +3=Second Level.");

        const auto* pCondition1 = vmTrigger.Conditions().GetItemAt(0);
        Expects(pCondition1 != nullptr);
        const auto* pCondition2 = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition2 != nullptr);
        const auto* pCondition3 = vmTrigger.Conditions().GetItemAt(2);
        Expects(pCondition3 != nullptr);
        const auto* pCondition4 = vmTrigger.Conditions().GetItemAt(3);
        Expects(pCondition4 != nullptr);
        const auto* pCondition5 = vmTrigger.Conditions().GetItemAt(4);
        Expects(pCondition5 != nullptr);

        Assert::IsFalse(pCondition1->IsIndirect());
        Assert::IsFalse(pCondition2->IsIndirect());
        Assert::IsFalse(pCondition3->IsIndirect());
        Assert::IsTrue(pCondition4->IsIndirect());
        Assert::IsTrue(pCondition5->IsIndirect());

        // $0001 = 1, 1+2 = $0003, $0003 = 3, 3+3 = $0006
        Assert::AreEqual(std::wstring(L""), pCondition3->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0003 (indirect {recall}+0x02)\r\nFirst Level (8-bit pointer)"), pCondition4->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0006 (indirect {recall}+0x02+0x03)\r\nSecond Level."), pCondition5->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 3, 3+2 = $0005, $0005 = 5, 5+3 = $0008
        vmTrigger.SetMemory({ 1 }, 3);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x0005 (indirect {recall}+0x02)\r\nFirst Level (8-bit pointer)"), pCondition4->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0008 (indirect {recall}+0x02+0x03)\r\nSecond Level."), pCondition5->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
    }

    TEST_METHOD(TestTooltipRecallPauseIfLate)
    {
        IndirectAddressTriggerViewModelHarness vmTrigger;
        vmTrigger.mockGameContext.InitializeCodeNotes();
        vmTrigger.Parse("I:{recall}_I:0xH0002_0xH0003=4_K:0xH0001_P:0=1");
        vmTrigger.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);
        vmTrigger.mockGameContext.SetCodeNote({ 1U }, L"[8-bit pointer]\n+2=First Level (8-bit pointer)\n  +3=Second Level.");

        const auto* pCondition1 = vmTrigger.Conditions().GetItemAt(0);
        Expects(pCondition1 != nullptr);
        const auto* pCondition2 = vmTrigger.Conditions().GetItemAt(1);
        Expects(pCondition2 != nullptr);
        const auto* pCondition3 = vmTrigger.Conditions().GetItemAt(2);
        Expects(pCondition3 != nullptr);
        const auto* pCondition4 = vmTrigger.Conditions().GetItemAt(3);
        Expects(pCondition4 != nullptr);
        const auto* pCondition5 = vmTrigger.Conditions().GetItemAt(4);
        Expects(pCondition5 != nullptr);

        Assert::IsFalse(pCondition1->IsIndirect());
        Assert::IsTrue(pCondition2->IsIndirect());
        Assert::IsTrue(pCondition3->IsIndirect());
        Assert::IsFalse(pCondition4->IsIndirect());
        Assert::IsFalse(pCondition5->IsIndirect());

        // $0001 = 1, 1+2 = $0003, $0003 = 3, 3+3 = $0006
        Assert::AreEqual(std::wstring(L""), pCondition1->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0003 (indirect {recall}+0x02)\r\nFirst Level (8-bit pointer)"), pCondition2->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0006 (indirect {recall}+0x02+0x03)\r\nSecond Level."), pCondition3->GetTooltip(TriggerConditionViewModel::SourceValueProperty));

        // $0001 = 3, 3+2 = $0005, $0005 = 5, 5+3 = $0008
        vmTrigger.SetMemory({ 1 }, 3);
        vmTrigger.CodeNotesDoFrame();
        Assert::AreEqual(std::wstring(L"0x0005 (indirect {recall}+0x02)\r\nFirst Level (8-bit pointer)"), pCondition2->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
        Assert::AreEqual(std::wstring(L"0x0008 (indirect {recall}+0x02+0x03)\r\nSecond Level."), pCondition3->GetTooltip(TriggerConditionViewModel::SourceValueProperty));
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

        condition.SetType(TriggerConditionType::Remember);
        Assert::IsTrue(condition.IsModifying());
    }

    TEST_METHOD(TestSwitchBetweenModifyingAndNonModifying)
    {
        TriggerConditionViewModelHarness condition;
        condition.SetSourceValue(0x1234U);
        condition.SetOperator(TriggerOperatorType::NotEquals);
        condition.SetTargetValue(8U);
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

        // change to modifying switches operator to None. the 8 is unmodified, but ignored
        condition.SetType(TriggerConditionType::Remember);
        Assert::AreEqual(TriggerOperatorType::None, condition.GetOperator());
        Assert::AreEqual(std::string("K:0xH1234"), condition.Serialize());
        Assert::IsTrue(condition.IsModifying());

        // changing operator "restores" the 8
        condition.SetOperator(TriggerOperatorType::Multiply);
        Assert::AreEqual(std::string("K:0xH1234*8"), condition.Serialize());

        // change to non-modifying switches operator to Equals
        condition.SetType(TriggerConditionType::ResetIf);
        Assert::AreEqual(TriggerOperatorType::Equals, condition.GetOperator());
        Assert::AreEqual(std::string("R:0xH1234=8"), condition.Serialize());
        Assert::IsFalse(condition.IsModifying());
    }

    TEST_METHOD(TestSwitchBetweenValueAndAddress)
    {
        TriggerConditionViewModelHarness condition;
        condition.SetSourceValue(0x1234U);
        condition.SetOperator(TriggerOperatorType::NotEquals);
        condition.SetTargetValue(8U);
        Assert::AreEqual(std::string("0xH1234!=8"), condition.Serialize());
        Assert::IsFalse(condition.HasTargetSize());

        // change to address switches size to match source
        condition.SetTargetType(TriggerOperandType::Delta);
        Assert::IsTrue(condition.HasTargetSize());
        Assert::AreEqual(MemSize::EightBit, condition.GetTargetSize());
        Assert::AreEqual(std::string("0xH1234!=d0xH0008"), condition.Serialize());

        // change to value sets size back to 32-bit
        condition.SetTargetType(TriggerOperandType::Value);
        Assert::IsFalse(condition.HasTargetSize());
        Assert::AreEqual(MemSize::ThirtyTwoBit, condition.GetTargetSize());
        Assert::AreEqual(std::string("0xH1234!=8"), condition.Serialize());
    }

    TEST_METHOD(TestChangeOperand)
    {
        TriggerConditionViewModelHarness condition;
        condition.SetSourceValue(0x1234U);
        condition.SetOperator(TriggerOperatorType::NotEquals);
        condition.SetTargetValue(8U);
        Assert::AreEqual(std::string("0xH1234!=8"), condition.Serialize());

        condition.SetTargetType(TriggerOperandType::Float);
        Assert::AreEqual(TriggerOperandType::Float, condition.GetTargetType());
        Assert::AreEqual(std::wstring(L"8.0"), condition.GetTargetValue());
        Assert::AreEqual(std::string("0xH1234!=f8.0"), condition.Serialize());

        condition.SetTargetValue(L"3.14");
        Assert::AreEqual(std::string("0xH1234!=f3.14"), condition.Serialize());

        condition.SetTargetType(TriggerOperandType::Value);
        Assert::AreEqual(TriggerOperandType::Value, condition.GetTargetType());
        Assert::AreEqual(std::wstring(L"0x03"), condition.GetTargetValue());
        Assert::AreEqual(std::string("0xH1234!=3"), condition.Serialize());

        condition.SetTargetType(TriggerOperandType::Address);
        Assert::AreEqual(TriggerOperandType::Address, condition.GetTargetType());
        Assert::AreEqual(std::wstring(L"0x0003"), condition.GetTargetValue());
        Assert::AreEqual(std::string("0xH1234!=0xH0003"), condition.Serialize());

        condition.SetTargetType(TriggerOperandType::Float);
        Assert::AreEqual(TriggerOperandType::Float, condition.GetTargetType());
        Assert::AreEqual(std::wstring(L"3.0"), condition.GetTargetValue());
        Assert::AreEqual(std::string("0xH1234!=f3.0"), condition.Serialize());

        condition.SetTargetType(TriggerOperandType::Address);
        Assert::AreEqual(TriggerOperandType::Address, condition.GetTargetType());
        Assert::AreEqual(std::wstring(L"0x0003"), condition.GetTargetValue());
        Assert::AreEqual(std::string("0xH1234!=0xH0003"), condition.Serialize());
    }

    TEST_METHOD(TestSetOperandMismatch)
    {
        TriggerConditionViewModelHarness condition;
        condition.SetSourceValue(0x1234U);
        condition.SetOperator(TriggerOperatorType::NotEquals);
        condition.SetTargetValue(8U);
        Assert::AreEqual(std::string("0xH1234!=8"), condition.Serialize());

        condition.SetTargetValue(L"3.14");
        Assert::AreEqual(TriggerOperandType::Value, condition.GetTargetType());
        Assert::AreEqual(std::wstring(L"3.14"), condition.GetTargetValue()); // value is not updated until trigger is reconstructed
        Assert::AreEqual(std::string("0xH1234!=0"), condition.Serialize()); // serializer will choke and convert to 0

        condition.SetTargetType(TriggerOperandType::Float);
        condition.SetTargetValue(L"3.14");
        Assert::AreEqual(TriggerOperandType::Float, condition.GetTargetType());
        Assert::AreEqual(std::wstring(L"3.14"), condition.GetTargetValue());
        Assert::AreEqual(std::string("0xH1234!=f3.14"), condition.Serialize());

        condition.SetTargetValue(L"0x0F");
        Assert::AreEqual(TriggerOperandType::Float, condition.GetTargetType());
        Assert::AreEqual(std::wstring(L"0x0F"), condition.GetTargetValue()); // value is not updated until trigger is reconstructed
        Assert::AreEqual(std::string("0xH1234!=f15.0"), condition.Serialize()); // apparently wcstof supports 0x prefix

        condition.SetTargetValue(L"6");
        Assert::AreEqual(TriggerOperandType::Float, condition.GetTargetType());
        Assert::AreEqual(std::wstring(L"6"), condition.GetTargetValue()); // value is not updated until trigger is reconstructed
        Assert::AreEqual(std::string("0xH1234!=f6.0"), condition.Serialize()); // serializer can handle this conversion
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
        Assert::IsFalse(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::BitwiseXor)));
        Assert::IsFalse(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::Modulus)));

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
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::BitwiseXor)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::Modulus)));
    }

    TEST_METHOD(TestIsComparisonVisibleMeasured)
    {
        TriggerConditionViewModelHarness condition;

        condition.SetType(TriggerConditionType::Measured);
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
        Assert::IsFalse(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::BitwiseXor)));
        Assert::IsFalse(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::Modulus)));

        TriggerViewModel vmTrigger;
        vmTrigger.SetIsValue(true);
        condition.SetTriggerViewModel(&vmTrigger);

        // None operator is visible when associated to a Value trigger
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::Equals)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::NotEquals)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::LessThan)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::LessThanOrEqual)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::GreaterThan)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::GreaterThanOrEqual)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::None)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::Multiply)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::Divide)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::BitwiseAnd)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::BitwiseXor)));
        Assert::IsTrue(TriggerConditionViewModel::IsComparisonVisible(condition, ra::etoi(TriggerOperatorType::Modulus)));
    }

    TEST_METHOD(TestFormatValueNumber)
    {
        TriggerConditionViewModelHarness condition;
        condition.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);

        Assert::AreEqual(std::wstring(L"0"), TriggerConditionViewModel::FormatValue(0U, TriggerOperandType::Value));
        Assert::AreEqual(std::wstring(L"1"), TriggerConditionViewModel::FormatValue(1U, TriggerOperandType::Value));
        Assert::AreEqual(std::wstring(L"10"), TriggerConditionViewModel::FormatValue(10U, TriggerOperandType::Value));
        Assert::AreEqual(std::wstring(L"100"), TriggerConditionViewModel::FormatValue(100U, TriggerOperandType::Value));
        Assert::AreEqual(std::wstring(L"1000000000"), TriggerConditionViewModel::FormatValue(1000000000U, TriggerOperandType::Value));
        Assert::AreEqual(std::wstring(L"4294967295"), TriggerConditionViewModel::FormatValue(0xFFFFFFFFU, TriggerOperandType::Value));
        Assert::AreEqual(std::wstring(L"12"), TriggerConditionViewModel::FormatValue(12.34f, TriggerOperandType::Value));

        condition.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, false);

        Assert::AreEqual(std::wstring(L"0x00"), TriggerConditionViewModel::FormatValue(0U, TriggerOperandType::Value));
        Assert::AreEqual(std::wstring(L"0x01"), TriggerConditionViewModel::FormatValue(1U, TriggerOperandType::Value));
        Assert::AreEqual(std::wstring(L"0x0a"), TriggerConditionViewModel::FormatValue(10U, TriggerOperandType::Value));
        Assert::AreEqual(std::wstring(L"0x64"), TriggerConditionViewModel::FormatValue(100U, TriggerOperandType::Value));
        Assert::AreEqual(std::wstring(L"0x3b9aca00"), TriggerConditionViewModel::FormatValue(1000000000U, TriggerOperandType::Value));
        Assert::AreEqual(std::wstring(L"0xffffffff"), TriggerConditionViewModel::FormatValue(0xFFFFFFFFU, TriggerOperandType::Value));
        Assert::AreEqual(std::wstring(L"0x0c"), TriggerConditionViewModel::FormatValue(12.34f, TriggerOperandType::Value));
    }

    TEST_METHOD(TestFormatValueFloat)
    {
        TriggerConditionViewModelHarness condition;
        condition.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);

        Assert::AreEqual(std::wstring(L"0.0"), TriggerConditionViewModel::FormatValue(0.0f, TriggerOperandType::Float));
        Assert::AreEqual(std::wstring(L"1.0"), TriggerConditionViewModel::FormatValue(1.0f, TriggerOperandType::Float));
        Assert::AreEqual(std::wstring(L"1.23"), TriggerConditionViewModel::FormatValue(1.23f, TriggerOperandType::Float));
        Assert::AreEqual(std::wstring(L"-3.14159"), TriggerConditionViewModel::FormatValue(-3.14159f, TriggerOperandType::Float));
        Assert::AreEqual(std::wstring(L"960.75"), TriggerConditionViewModel::FormatValue(960.75f, TriggerOperandType::Float));

        condition.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, false);

        Assert::AreEqual(std::wstring(L"0.0"), TriggerConditionViewModel::FormatValue(0.0f, TriggerOperandType::Float));
        Assert::AreEqual(std::wstring(L"1.0"), TriggerConditionViewModel::FormatValue(1.0f, TriggerOperandType::Float));
        Assert::AreEqual(std::wstring(L"1.23"), TriggerConditionViewModel::FormatValue(1.23f, TriggerOperandType::Float));
        Assert::AreEqual(std::wstring(L"-3.14159"), TriggerConditionViewModel::FormatValue(-3.14159f, TriggerOperandType::Float));
        Assert::AreEqual(std::wstring(L"960.75"), TriggerConditionViewModel::FormatValue(960.75f, TriggerOperandType::Float));
    }

    TEST_METHOD(TestFormatValueAddress)
    {
        TriggerConditionViewModelHarness condition;
        condition.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);

        Assert::AreEqual(std::wstring(L"0x0000"), TriggerConditionViewModel::FormatValue(0U, TriggerOperandType::Address));
        Assert::AreEqual(std::wstring(L"0x1234"), TriggerConditionViewModel::FormatValue(4660U, TriggerOperandType::Address));
        Assert::AreEqual(std::wstring(L"0xc3500"), TriggerConditionViewModel::FormatValue(800000U, TriggerOperandType::Address));
        Assert::AreEqual(std::wstring(L"0x1234"), TriggerConditionViewModel::FormatValue(4660U, TriggerOperandType::Delta));
        Assert::AreEqual(std::wstring(L"0x1234"), TriggerConditionViewModel::FormatValue(4660U, TriggerOperandType::Prior));
        Assert::AreEqual(std::wstring(L"0x1234"), TriggerConditionViewModel::FormatValue(4660U, TriggerOperandType::BCD));
        Assert::AreEqual(std::wstring(L"0x1234"), TriggerConditionViewModel::FormatValue(4660U, TriggerOperandType::Inverted));
        Assert::AreEqual(std::wstring(L"0x1234"), TriggerConditionViewModel::FormatValue(4660.25f, TriggerOperandType::Address));

        condition.mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, false);

        Assert::AreEqual(std::wstring(L"0x0000"), TriggerConditionViewModel::FormatValue(0U, TriggerOperandType::Address));
        Assert::AreEqual(std::wstring(L"0x1234"), TriggerConditionViewModel::FormatValue(4660U, TriggerOperandType::Address));
        Assert::AreEqual(std::wstring(L"0xc3500"), TriggerConditionViewModel::FormatValue(800000U, TriggerOperandType::Address));
        Assert::AreEqual(std::wstring(L"0x1234"), TriggerConditionViewModel::FormatValue(4660U, TriggerOperandType::Delta));
        Assert::AreEqual(std::wstring(L"0x1234"), TriggerConditionViewModel::FormatValue(4660U, TriggerOperandType::Prior));
        Assert::AreEqual(std::wstring(L"0x1234"), TriggerConditionViewModel::FormatValue(4660U, TriggerOperandType::Inverted));
        Assert::AreEqual(std::wstring(L"0x1234"), TriggerConditionViewModel::FormatValue(4660U, TriggerOperandType::BCD));
        Assert::AreEqual(std::wstring(L"0x1234"), TriggerConditionViewModel::FormatValue(4660.25f, TriggerOperandType::Address));
    }

    TEST_METHOD(TestUpdateRowColor)
    {
        TriggerConditionViewModelHarness condition;
        ra::ui::EditorTheme pTheme;
        ra::services::ServiceLocator::ServiceOverride<ra::ui::EditorTheme> pThemeOverride(&pTheme);
        const auto nDefaultColor = ra::to_unsigned(TriggerConditionViewModel::RowColorProperty.GetDefaultValue());

        const std::string sInput = "R:0xH0000=1_P:0xH0001=1_0xH0002=1(2)_0xH0003=1";
        const auto nSize = rc_trigger_size(sInput.c_str());
        std::string sBuffer;
        sBuffer.resize(nSize);

        const rc_trigger_t* pTrigger = rc_parse_trigger(sBuffer.data(), sInput.c_str(), nullptr, 0);
        Assert::IsNotNull(pTrigger);
        Ensures(pTrigger != nullptr);

        // ResetIf
        rc_condition_t* pCondition = pTrigger->requirement->conditions;
        Expects(pCondition != nullptr);
        condition.InitializeFrom(*pCondition);

        pCondition->is_true = 0;
        condition.UpdateRowColor(pCondition);
        Assert::AreEqual(nDefaultColor, condition.GetRowColor().ARGB);

        pCondition->is_true = 1;
        condition.UpdateRowColor(pCondition);
        Assert::AreEqual(pTheme.ColorTriggerResetTrue().ARGB, condition.GetRowColor().ARGB);

        pCondition->is_true = 0;
        condition.UpdateRowColor(pCondition);
        Assert::AreEqual(nDefaultColor, condition.GetRowColor().ARGB);

        // PauseIf
        pCondition = pCondition->next;
        Expects(pCondition != nullptr);
        condition.InitializeFrom(*pCondition);

        pCondition->is_true = 0;
        condition.UpdateRowColor(pCondition);
        Assert::AreEqual(nDefaultColor, condition.GetRowColor().ARGB);

        pCondition->is_true = 1;
        condition.UpdateRowColor(pCondition);
        Assert::AreEqual(pTheme.ColorTriggerPauseTrue().ARGB, condition.GetRowColor().ARGB);

        pCondition->is_true = 0;
        condition.UpdateRowColor(pCondition);
        Assert::AreEqual(nDefaultColor, condition.GetRowColor().ARGB);

        // Condition with hit target
        pCondition = pCondition->next;
        Expects(pCondition != nullptr);
        condition.InitializeFrom(*pCondition);

        pCondition->is_true = 0;
        pCondition->current_hits = 0;
        condition.UpdateRowColor(pCondition);
        Assert::AreEqual(nDefaultColor, condition.GetRowColor().ARGB);

        pCondition->is_true = 1;
        pCondition->current_hits = 1;
        condition.UpdateRowColor(pCondition);
        Assert::AreEqual(pTheme.ColorTriggerBecomingTrue().ARGB, condition.GetRowColor().ARGB);

        pCondition->is_true = 0;
        pCondition->current_hits = 1;
        condition.UpdateRowColor(pCondition);
        Assert::AreEqual(nDefaultColor, condition.GetRowColor().ARGB);

        pCondition->is_true = 1;
        pCondition->current_hits = 2;
        condition.UpdateRowColor(pCondition);
        Assert::AreEqual(pTheme.ColorTriggerIsTrue().ARGB, condition.GetRowColor().ARGB);

        pCondition->is_true = 0;
        pCondition->current_hits = 2;
        condition.UpdateRowColor(pCondition);
        Assert::AreEqual(pTheme.ColorTriggerWasTrue().ARGB, condition.GetRowColor().ARGB);

        // Simple condition
        pCondition = pCondition->next;
        Expects(pCondition != nullptr);
        condition.InitializeFrom(*pCondition);

        pCondition->is_true = 0;
        condition.UpdateRowColor(pCondition);
        Assert::AreEqual(nDefaultColor, condition.GetRowColor().ARGB);

        pCondition->is_true = 1;
        condition.UpdateRowColor(pCondition);
        Assert::AreEqual(pTheme.ColorTriggerIsTrue().ARGB, condition.GetRowColor().ARGB);

        pCondition->is_true = 0;
        condition.UpdateRowColor(pCondition);
        Assert::AreEqual(nDefaultColor, condition.GetRowColor().ARGB);
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
