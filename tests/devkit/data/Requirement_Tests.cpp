#include "data/Requirement.hh"

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
