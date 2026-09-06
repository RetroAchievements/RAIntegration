#ifndef RA_DATA_REQUIREMENT_H
#define RA_DATA_REQUIREMENT_H
#pragma once

#include <stdint.h>
#include <string>

#include <rcheevos/include/rc_runtime_types.h>

namespace ra {
namespace data {

class Requirement {
public:
    enum class Type : uint8_t
    {
        /// <summary>
        /// No special behavior.
        /// </summary>
        Standard = RC_CONDITION_STANDARD,

        /// <summary>
        /// Pauses processing of the current requirement group if true.
        /// </summary>
        PauseIf = RC_CONDITION_PAUSE_IF,

        /// <summary>
        /// Resets any HitCounts in the current trigger if true.
        /// </summary>
        ResetIf = RC_CONDITION_RESET_IF,

        /// <summary>
        /// Calculates a value and adds it to the next requirement.
        /// </summary>
        AddSource = RC_CONDITION_ADD_SOURCE,

        /// <summary>
        /// Calculates a value and subtracts it from the next requirement.
        /// </summary>
        SubSource = RC_CONDITION_SUB_SOURCE,

        /// <summary>
        /// Adds the HitsCounts from this requirement to the next requirement.
        /// </summary>
        AddHits = RC_CONDITION_ADD_HITS,

        /// <summary>
        /// Subtracts the HitsCounts from this requirement to the next requirement.
        /// </summary>
        SubHits = RC_CONDITION_SUB_HITS,

        /// <summary>
        /// This requirement must also be true for the next requirement to be true.
        /// </summary>
        AndNext = RC_CONDITION_AND_NEXT,

        /// <summary>
        /// This requirement or the following requirement must be true for the next requirement to be true.
        /// </summary>
        OrNext = RC_CONDITION_OR_NEXT,

        /// <summary>
        /// Meta-flag indicating that this condition tracks progress as a raw value.
        /// </summary>
        Measured = RC_CONDITION_MEASURED,

        /// <summary>
        /// Meta-flag indicating that this condition must be true to track progress.
        /// </summary>
        MeasuredIf = RC_CONDITION_MEASURED_IF,

        /// <summary>
        /// Calculates a value to offset the operands of the next requirement by.
        /// </summary>
        AddAddress = RC_CONDITION_ADD_ADDRESS,

        /// <summary>
        /// Resets any HitCounts on the next requirement chain if true.
        /// </summary>
        ResetNextIf = RC_CONDITION_RESET_NEXT_IF,

        /// <summary>
        /// While all non-Trigger conditions are true, a challenge indicator will be displayed.
        /// </summary>
        Trigger = RC_CONDITION_TRIGGER,

        /// <summary>
        /// Calculates a value and captures it for later use.
        /// </summary>
        Remember = RC_CONDITION_REMEMBER,

        /// <summary>
        /// Meta-flag indicating that this condition tracks progress as a percentage.
        /// </summary>
        MeasuredAsPercent = 99
    };

    static constexpr bool IsCombining(Type nType) noexcept
    {
        switch (nType)
        {
            case Type::Standard:
            case Type::PauseIf:
            case Type::ResetIf:
            case Type::Measured:
            case Type::MeasuredAsPercent:
            case Type::MeasuredIf:
            case Type::Trigger:
                return false;

            default:
                return true;
        }
    }

    /// <summary>
    /// Returns <c>true</c> if the condition supports modifying operators.
    /// </summary>
    static constexpr bool SupportsModifyingOperators(Type nType) noexcept
    {
        switch (nType)
        {
            case Type::AddAddress:
            case Type::AddSource:
            case Type::SubSource:
            case Type::Remember:
                return true;

            default:
                return false;
        }
    }

    enum class OperandType : uint8_t
    {
        /// <summary>
        /// The value at a memory address.
        /// </summary>
        Address = RC_OPERAND_ADDRESS,

        /// <summary>
        /// The previous value at a memory address.
        /// </summary>
        Delta = RC_OPERAND_DELTA,

        /// <summary>
        /// The last differing value at a memory address.
        /// </summary>
        Prior = RC_OPERAND_PRIOR,

        /// <summary>
        /// The current value at a memory address decoded from BCD.
        /// </summary>
        BCD = RC_OPERAND_BCD,

        /// <summary>
        /// The bitwise inversion of the value at a memory address.
        /// </summary>
        Inverted = RC_OPERAND_INVERTED,

        /// <summary>
        /// An unsigned integer constant.
        /// </summary>
        Value = RC_OPERAND_CONST,

        /// <summary>
        /// A floating point constant.
        /// </summary>
        Float = RC_OPERAND_FP,

        /// <summary>
        /// The accumulator captured by a Remember condition.
        /// </summary>
        Recall = RC_OPERAND_RECALL
    };

    /// <summary>
    /// Returns <c>true</c> if the operand is an address.
    /// </summary>
    static constexpr bool IsOperandTypeForAddress(OperandType nType) noexcept
    {
        switch (nType)
        {
            case OperandType::Value:
            case OperandType::Float:
            case OperandType::Recall:
                return false;

            default:
                return true;
        }
    }

    /// <summary>
    /// Returns <c>true</c> if the operand has no address or value portion.
    /// </summary>
    static constexpr bool IsOperandTypeParameterless(ra::data::Requirement::OperandType nType) noexcept
    {
        switch (nType)
        {
            case ra::data::Requirement::OperandType::Recall:
                return true;
            default:
                return false;
        }
    }

    enum class OperatorType : uint8_t
    {
        /// <summary>
        /// Unspecified.
        /// </summary>
        None = RC_OPERATOR_NONE,

        /// <summary>
        /// The left and right values are equivalent.
        /// </summary>
        Equals = RC_OPERATOR_EQ,

        /// <summary>
        /// The left and right values are not equivalent.
        /// </summary>
        NotEquals = RC_OPERATOR_NE,

        /// <summary>
        /// The left value is less than the right value.
        /// </summary>
        LessThan = RC_OPERATOR_LT,

        /// <summary>
        /// The left value is less than or equal to the right value.
        /// </summary>
        LessThanOrEqual = RC_OPERATOR_LE,

        /// <summary>
        /// The left value is greater than the right value.
        /// </summary>
        GreaterThan = RC_OPERATOR_GT,

        /// <summary>
        /// The left value is greater than or equal to the right value.
        /// </summary>
        GreaterThanOrEqual = RC_OPERATOR_GE,

        /// <summary>
        /// The right value is added to the left value. (combining conditions only)
        /// </summary>
        Add = RC_OPERATOR_ADD,

        /// <summary>
        /// The right value is subtracted from the left value. (combining conditions only)
        /// </summary>
        Subtract = RC_OPERATOR_SUB,

        /// <summary>
        /// The left value is multiplied by the right value. (combining conditions only)
        /// </summary>
        Multiply = RC_OPERATOR_MULT,

        /// <summary>
        /// The left value is divided by the right value. (combining conditions only)
        /// </summary>
        Divide = RC_OPERATOR_DIV,

        /// <summary>
        /// The left value is divided by the right value and the remainder is returned. (combining conditions only)
        /// </summary>
        Modulus = RC_OPERATOR_MOD,

        /// <summary>
        /// The left value is masked by the right value. (combining conditions only)
        /// </summary>
        BitwiseAnd = RC_OPERATOR_AND,

        /// <summary>
        /// The bits in the left value are toggled by the bits in the right value. (combining conditions only)
        /// </summary>
        BitwiseXor = RC_OPERATOR_XOR
    };

    /// <summary>
    /// Returns <c>true</c> if the operator is used to combine the left and right operands.
    /// </summary>
    static constexpr bool IsModifyingOperator(ra::data::Requirement::OperatorType nType)
    {
        switch (nType)
        {
            case ra::data::Requirement::OperatorType::None:
            case ra::data::Requirement::OperatorType::Multiply:
            case ra::data::Requirement::OperatorType::Divide:
            case ra::data::Requirement::OperatorType::BitwiseAnd:
            case ra::data::Requirement::OperatorType::BitwiseXor:
            case ra::data::Requirement::OperatorType::Modulus:
            case ra::data::Requirement::OperatorType::Add:
            case ra::data::Requirement::OperatorType::Subtract:
                return true;

            default:
                return false;
        }
    }
};

} // namespace data
} // namespace ra

#endif RA_DATA_MEMORY_H
