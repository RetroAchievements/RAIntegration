#include "data/Requirement.hh"

#include "context/mocks/MockEmulatorMemoryContext.hh"

#include "services/mocks/MockConfiguration.hh"

#include "testutil/CppUnitTest.hh"

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(Requirement_Tests)
{
public:
    TEST_METHOD(TestIsCombining)
    {
        Assert::IsTrue(Requirement::IsCombining(Requirement::Type::AddAddress));
        Assert::IsTrue(Requirement::IsCombining(Requirement::Type::AddHits));
        Assert::IsTrue(Requirement::IsCombining(Requirement::Type::AddSource));
        Assert::IsTrue(Requirement::IsCombining(Requirement::Type::AndNext));
        Assert::IsFalse(Requirement::IsCombining(Requirement::Type::Measured));
        Assert::IsFalse(Requirement::IsCombining(Requirement::Type::MeasuredAsPercent));
        Assert::IsFalse(Requirement::IsCombining(Requirement::Type::MeasuredIf));
        Assert::IsTrue(Requirement::IsCombining(Requirement::Type::OrNext));
        Assert::IsFalse(Requirement::IsCombining(Requirement::Type::PauseIf));
        Assert::IsFalse(Requirement::IsCombining(Requirement::Type::ResetIf));
        Assert::IsTrue(Requirement::IsCombining(Requirement::Type::ResetNextIf));
        Assert::IsFalse(Requirement::IsCombining(Requirement::Type::Standard));
        Assert::IsTrue(Requirement::IsCombining(Requirement::Type::SubHits));
        Assert::IsTrue(Requirement::IsCombining(Requirement::Type::SubSource));
        Assert::IsFalse(Requirement::IsCombining(Requirement::Type::Trigger));
        Assert::IsTrue(Requirement::IsCombining(Requirement::Type::Remember));
    }

    TEST_METHOD(TestSupportsModifyingOperators)
    {
        Assert::IsTrue(Requirement::SupportsModifyingOperators(Requirement::Type::AddAddress));
        Assert::IsFalse(Requirement::SupportsModifyingOperators(Requirement::Type::AddHits));
        Assert::IsTrue(Requirement::SupportsModifyingOperators(Requirement::Type::AddSource));
        Assert::IsFalse(Requirement::SupportsModifyingOperators(Requirement::Type::AndNext));
        Assert::IsFalse(Requirement::SupportsModifyingOperators(Requirement::Type::Measured));
        Assert::IsFalse(Requirement::SupportsModifyingOperators(Requirement::Type::MeasuredAsPercent));
        Assert::IsFalse(Requirement::SupportsModifyingOperators(Requirement::Type::MeasuredIf));
        Assert::IsFalse(Requirement::SupportsModifyingOperators(Requirement::Type::OrNext));
        Assert::IsFalse(Requirement::SupportsModifyingOperators(Requirement::Type::PauseIf));
        Assert::IsFalse(Requirement::SupportsModifyingOperators(Requirement::Type::ResetIf));
        Assert::IsFalse(Requirement::SupportsModifyingOperators(Requirement::Type::ResetNextIf));
        Assert::IsFalse(Requirement::SupportsModifyingOperators(Requirement::Type::Standard));
        Assert::IsFalse(Requirement::SupportsModifyingOperators(Requirement::Type::SubHits));
        Assert::IsTrue(Requirement::SupportsModifyingOperators(Requirement::Type::SubSource));
        Assert::IsFalse(Requirement::SupportsModifyingOperators(Requirement::Type::Trigger));
        Assert::IsTrue(Requirement::SupportsModifyingOperators(Requirement::Type::Remember));
    }

    TEST_METHOD(TestIsOperandTypeForAddress)
    {
        Assert::IsTrue(Requirement::IsOperandTypeForAddress(Requirement::OperandType::Address));
        Assert::IsTrue(Requirement::IsOperandTypeForAddress(Requirement::OperandType::Delta));
        Assert::IsTrue(Requirement::IsOperandTypeForAddress(Requirement::OperandType::Prior));
        Assert::IsTrue(Requirement::IsOperandTypeForAddress(Requirement::OperandType::BCD));
        Assert::IsTrue(Requirement::IsOperandTypeForAddress(Requirement::OperandType::Inverted));
        Assert::IsFalse(Requirement::IsOperandTypeForAddress(Requirement::OperandType::Value));
        Assert::IsFalse(Requirement::IsOperandTypeForAddress(Requirement::OperandType::Float));
        Assert::IsFalse(Requirement::IsOperandTypeForAddress(Requirement::OperandType::Recall));
    }

    TEST_METHOD(TestIsOperandTypeParameterless)
    {
        Assert::IsFalse(Requirement::IsOperandTypeParameterless(Requirement::OperandType::Address));
        Assert::IsFalse(Requirement::IsOperandTypeParameterless(Requirement::OperandType::Delta));
        Assert::IsFalse(Requirement::IsOperandTypeParameterless(Requirement::OperandType::Prior));
        Assert::IsFalse(Requirement::IsOperandTypeParameterless(Requirement::OperandType::BCD));
        Assert::IsFalse(Requirement::IsOperandTypeParameterless(Requirement::OperandType::Inverted));
        Assert::IsFalse(Requirement::IsOperandTypeParameterless(Requirement::OperandType::Value));
        Assert::IsFalse(Requirement::IsOperandTypeParameterless(Requirement::OperandType::Float));
        Assert::IsTrue(Requirement::IsOperandTypeParameterless(Requirement::OperandType::Recall));
    }

    TEST_METHOD(TestFormatOperandValueNumber)
    {
        ra::services::mocks::MockConfiguration mockConfiguration;
        mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);

        Assert::AreEqual(std::wstring(L"0"), Requirement::FormatOperandValue(0U, ra::data::Requirement::OperandType::Value));
        Assert::AreEqual(std::wstring(L"1"), Requirement::FormatOperandValue(1U, ra::data::Requirement::OperandType::Value));
        Assert::AreEqual(std::wstring(L"10"), Requirement::FormatOperandValue(10U, ra::data::Requirement::OperandType::Value));
        Assert::AreEqual(std::wstring(L"100"), Requirement::FormatOperandValue(100U, ra::data::Requirement::OperandType::Value));
        Assert::AreEqual(std::wstring(L"1000000000"), Requirement::FormatOperandValue(1000000000U, ra::data::Requirement::OperandType::Value));
        Assert::AreEqual(std::wstring(L"4294967295"), Requirement::FormatOperandValue(0xFFFFFFFFU, ra::data::Requirement::OperandType::Value));
        Assert::AreEqual(std::wstring(L"12"), Requirement::FormatOperandValue(12.34f, ra::data::Requirement::OperandType::Value));

        mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, false);

        Assert::AreEqual(std::wstring(L"0x00"), Requirement::FormatOperandValue(0U, ra::data::Requirement::OperandType::Value));
        Assert::AreEqual(std::wstring(L"0x01"), Requirement::FormatOperandValue(1U, ra::data::Requirement::OperandType::Value));
        Assert::AreEqual(std::wstring(L"0x0a"), Requirement::FormatOperandValue(10U, ra::data::Requirement::OperandType::Value));
        Assert::AreEqual(std::wstring(L"0x64"), Requirement::FormatOperandValue(100U, ra::data::Requirement::OperandType::Value));
        Assert::AreEqual(std::wstring(L"0x3b9aca00"), Requirement::FormatOperandValue(1000000000U, ra::data::Requirement::OperandType::Value));
        Assert::AreEqual(std::wstring(L"0xffffffff"), Requirement::FormatOperandValue(0xFFFFFFFFU, ra::data::Requirement::OperandType::Value));
        Assert::AreEqual(std::wstring(L"0x0c"), Requirement::FormatOperandValue(12.34f, ra::data::Requirement::OperandType::Value));
    }

    TEST_METHOD(TestFormatOperandValueFloat)
    {
        ra::services::mocks::MockConfiguration mockConfiguration;
        mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);

        Assert::AreEqual(std::wstring(L"0.0"), Requirement::FormatOperandValue(0.0f, ra::data::Requirement::OperandType::Float));
        Assert::AreEqual(std::wstring(L"1.0"), Requirement::FormatOperandValue(1.0f, ra::data::Requirement::OperandType::Float));
        Assert::AreEqual(std::wstring(L"1.23"), Requirement::FormatOperandValue(1.23f, ra::data::Requirement::OperandType::Float));
        Assert::AreEqual(std::wstring(L"-3.14159"), Requirement::FormatOperandValue(-3.14159f, ra::data::Requirement::OperandType::Float));
        Assert::AreEqual(std::wstring(L"960.75"), Requirement::FormatOperandValue(960.75f, ra::data::Requirement::OperandType::Float));

        mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, false);

        Assert::AreEqual(std::wstring(L"0.0"), Requirement::FormatOperandValue(0.0f, ra::data::Requirement::OperandType::Float));
        Assert::AreEqual(std::wstring(L"1.0"), Requirement::FormatOperandValue(1.0f, ra::data::Requirement::OperandType::Float));
        Assert::AreEqual(std::wstring(L"1.23"), Requirement::FormatOperandValue(1.23f, ra::data::Requirement::OperandType::Float));
        Assert::AreEqual(std::wstring(L"-3.14159"), Requirement::FormatOperandValue(-3.14159f, ra::data::Requirement::OperandType::Float));
        Assert::AreEqual(std::wstring(L"960.75"), Requirement::FormatOperandValue(960.75f, ra::data::Requirement::OperandType::Float));
    }

    TEST_METHOD(TestFormatOperandValueAddress)
    {
        ra::context::mocks::MockEmulatorMemoryContext mockEmulatorMemoryContext;
        ra::services::mocks::MockConfiguration mockConfiguration;
        mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, true);

        Assert::AreEqual(std::wstring(L"0x0000"), Requirement::FormatOperandValue(0U, ra::data::Requirement::OperandType::Address));
        Assert::AreEqual(std::wstring(L"0x1234"), Requirement::FormatOperandValue(4660U, ra::data::Requirement::OperandType::Address));
        Assert::AreEqual(std::wstring(L"0x0c3500"), Requirement::FormatOperandValue(800000U, ra::data::Requirement::OperandType::Address));
        Assert::AreEqual(std::wstring(L"0x1234"), Requirement::FormatOperandValue(4660U, ra::data::Requirement::OperandType::Delta));
        Assert::AreEqual(std::wstring(L"0x1234"), Requirement::FormatOperandValue(4660U, ra::data::Requirement::OperandType::Prior));
        Assert::AreEqual(std::wstring(L"0x1234"), Requirement::FormatOperandValue(4660U, ra::data::Requirement::OperandType::BCD));
        Assert::AreEqual(std::wstring(L"0x1234"), Requirement::FormatOperandValue(4660U, ra::data::Requirement::OperandType::Inverted));
        Assert::AreEqual(std::wstring(L"0x1234"), Requirement::FormatOperandValue(4660.25f, ra::data::Requirement::OperandType::Address));

        mockConfiguration.SetFeatureEnabled(ra::services::Feature::PreferDecimal, false);

        Assert::AreEqual(std::wstring(L"0x0000"), Requirement::FormatOperandValue(0U, ra::data::Requirement::OperandType::Address));
        Assert::AreEqual(std::wstring(L"0x1234"), Requirement::FormatOperandValue(4660U, ra::data::Requirement::OperandType::Address));
        Assert::AreEqual(std::wstring(L"0x0c3500"), Requirement::FormatOperandValue(800000U, ra::data::Requirement::OperandType::Address));
        Assert::AreEqual(std::wstring(L"0x1234"), Requirement::FormatOperandValue(4660U, ra::data::Requirement::OperandType::Delta));
        Assert::AreEqual(std::wstring(L"0x1234"), Requirement::FormatOperandValue(4660U, ra::data::Requirement::OperandType::Prior));
        Assert::AreEqual(std::wstring(L"0x1234"), Requirement::FormatOperandValue(4660U, ra::data::Requirement::OperandType::Inverted));
        Assert::AreEqual(std::wstring(L"0x1234"), Requirement::FormatOperandValue(4660U, ra::data::Requirement::OperandType::BCD));
        Assert::AreEqual(std::wstring(L"0x1234"), Requirement::FormatOperandValue(4660.25f, ra::data::Requirement::OperandType::Address));
    }

    TEST_METHOD(TestIsModifyingOperator)
    {
        Assert::IsTrue(Requirement::IsModifyingOperator(Requirement::OperatorType::None));
        Assert::IsFalse(Requirement::IsModifyingOperator(Requirement::OperatorType::Equals));
        Assert::IsFalse(Requirement::IsModifyingOperator(Requirement::OperatorType::NotEquals));
        Assert::IsFalse(Requirement::IsModifyingOperator(Requirement::OperatorType::LessThan));
        Assert::IsFalse(Requirement::IsModifyingOperator(Requirement::OperatorType::LessThanOrEqual));
        Assert::IsFalse(Requirement::IsModifyingOperator(Requirement::OperatorType::GreaterThan));
        Assert::IsFalse(Requirement::IsModifyingOperator(Requirement::OperatorType::GreaterThanOrEqual));
        Assert::IsTrue(Requirement::IsModifyingOperator(Requirement::OperatorType::Add));
        Assert::IsTrue(Requirement::IsModifyingOperator(Requirement::OperatorType::Subtract));
        Assert::IsTrue(Requirement::IsModifyingOperator(Requirement::OperatorType::Multiply));
        Assert::IsTrue(Requirement::IsModifyingOperator(Requirement::OperatorType::Divide));
        Assert::IsTrue(Requirement::IsModifyingOperator(Requirement::OperatorType::Modulus));
        Assert::IsTrue(Requirement::IsModifyingOperator(Requirement::OperatorType::BitwiseAnd));
        Assert::IsTrue(Requirement::IsModifyingOperator(Requirement::OperatorType::BitwiseXor));
    }
};

} // namespace tests
} // namespace data
} // namespace ra
