#ifndef RA_ACHIEVEMENT_LOGIC_SERIALIZER_HH
#define RA_ACHIEVEMENT_LOGIC_SERIALIZER_HH
#pragma once

#include "ra_fwd.h"

#include "data\Types.hh"

#include "data\models\CodeNoteModel.hh"

namespace ra {
namespace services {

enum class TriggerConditionType : uint8_t
{
    Standard = RC_CONDITION_STANDARD,
    PauseIf = RC_CONDITION_PAUSE_IF,
    ResetIf = RC_CONDITION_RESET_IF,
    AddSource = RC_CONDITION_ADD_SOURCE,
    SubSource = RC_CONDITION_SUB_SOURCE,
    AddHits = RC_CONDITION_ADD_HITS,
    SubHits = RC_CONDITION_SUB_HITS,
    Remember = RC_CONDITION_REMEMBER,
    AndNext = RC_CONDITION_AND_NEXT,
    Measured = RC_CONDITION_MEASURED,
    AddAddress = RC_CONDITION_ADD_ADDRESS,
    OrNext = RC_CONDITION_OR_NEXT,
    Trigger = RC_CONDITION_TRIGGER,
    MeasuredIf = RC_CONDITION_MEASURED_IF,
    ResetNextIf = RC_CONDITION_RESET_NEXT_IF,
    MeasuredAsPercent = 99
};

enum class TriggerOperandType : uint8_t
{
    Address = RC_OPERAND_ADDRESS,   // compare to the value of a live address in RAM
    Delta = RC_OPERAND_DELTA,       // the value last known at this address.
    Value = RC_OPERAND_CONST,       // a 32 bit unsigned integer
    Prior = RC_OPERAND_PRIOR,       // the last differing value at this address.
    BCD = RC_OPERAND_BCD,           // Address, but decoded from binary-coded-decimal
    Float = RC_OPERAND_FP,          // a 32-bit floating point value
    Inverted = RC_OPERAND_INVERTED, // the bitwise compliment of the current value at the address
    Recall = RC_OPERAND_RECALL      // the last value stored by RC_CONDITION_REMEMBER
};

enum class TriggerOperatorType : uint8_t
{
    Equals = RC_OPERATOR_EQ,
    LessThan = RC_OPERATOR_LT,
    LessThanOrEqual = RC_OPERATOR_LE,
    GreaterThan = RC_OPERATOR_GT,
    GreaterThanOrEqual = RC_OPERATOR_GE,
    NotEquals = RC_OPERATOR_NE,
    None = RC_OPERATOR_NONE,
    Multiply = RC_OPERATOR_MULT,
    Divide = RC_OPERATOR_DIV,
    BitwiseAnd = RC_OPERATOR_AND,
    BitwiseXor = RC_OPERATOR_XOR,
    Modulus = RC_OPERATOR_MOD,
    Add = RC_OPERATOR_ADD,
    Subtract = RC_OPERATOR_SUB
};

class AchievementLogicSerializer
{
public:
    GSL_SUPPRESS_F6 AchievementLogicSerializer() = default;
    virtual ~AchievementLogicSerializer() = default;

    AchievementLogicSerializer(const AchievementLogicSerializer&) noexcept = delete;
    AchievementLogicSerializer& operator=(const AchievementLogicSerializer&) noexcept = delete;
    AchievementLogicSerializer(AchievementLogicSerializer&&) noexcept = delete;
    AchievementLogicSerializer& operator=(AchievementLogicSerializer&&) noexcept = delete;

    static void AppendConditionSeparator(std::string& sBuffer) { sBuffer.push_back('_'); }

    static void AppendConditionType(std::string& sBuffer, TriggerConditionType nType);

    static void AppendOperand(std::string& sBuffer, TriggerOperandType nType, MemSize nSize, uint32_t nValue);
    static void AppendOperand(std::string& sBuffer, TriggerOperandType nType, MemSize nSize, float fValue);

    static void AppendOperator(std::string& sBuffer, TriggerOperatorType nType);

    static void AppendHitTarget(std::string& sBuffer, uint32_t nTarget);

    static std::string BuildMemRefChain(const ra::data::models::CodeNoteModel& pRootNote,
                                        const ra::data::models::CodeNoteModel& pLeafNote);
};

} // namespace services
} // namespace ra

#endif // !RA_ACHIEVEMENT_LOGIC_SERIALIZER_HH
