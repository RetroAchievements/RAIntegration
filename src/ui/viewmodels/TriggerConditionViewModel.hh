#ifndef RA_UI_TRIGGERCONDITIONVIEWMODEL_H
#define RA_UI_TRIGGERCONDITIONVIEWMODEL_H
#pragma once

#include "ui\ViewModelBase.hh"

#include "data\Types.hh"

#include "ra_utility.h"

struct rc_condition_t;

namespace ra {
namespace ui {
namespace viewmodels {

enum class TriggerConditionType : uint8_t
{
    Standard = RC_CONDITION_STANDARD,
    PauseIf = RC_CONDITION_PAUSE_IF,
    ResetIf = RC_CONDITION_RESET_IF,
    AddSource = RC_CONDITION_ADD_SOURCE,
    SubSource = RC_CONDITION_SUB_SOURCE,
    AddHits = RC_CONDITION_ADD_HITS,
    AndNext = RC_CONDITION_AND_NEXT,
    Measured = RC_CONDITION_MEASURED,
    AddAddress = RC_CONDITION_ADD_ADDRESS,
    OrNext = RC_CONDITION_OR_NEXT,
    Trigger = RC_CONDITION_TRIGGER,
    MeasuredIf = RC_CONDITION_MEASURED_IF
};

enum class TriggerOperandType : uint8_t
{
    Address = RC_OPERAND_ADDRESS,       // compare to the value of a live address in RAM
    Delta = RC_OPERAND_DELTA,           // the value last known at this address.
    Value = RC_OPERAND_CONST,           // a 32 bit unsigned integer
    Prior = RC_OPERAND_PRIOR,           // the last differing value at this address.
    BCD = RC_OPERAND_BCD,               // Address, but decoded from binary-coded-decimal
    Inverted = RC_OPERAND_INVERTED      // Address, but two-compliment applied (~Address)
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
    BitwiseAnd = RC_OPERATOR_AND
};

class TriggerConditionViewModel : public ViewModelBase
{
public:
    static const IntModelProperty IndexProperty;
    int GetIndex() const { return GetValue(IndexProperty); }
    void SetIndex(int nValue) { SetValue(IndexProperty, nValue); }

    static const IntModelProperty TypeProperty;
    TriggerConditionType GetType() const { return ra::itoe<TriggerConditionType>(GetValue(TypeProperty)); }
    void SetType(TriggerConditionType nValue) { SetValue(TypeProperty, ra::etoi(nValue)); }

    static const IntModelProperty SourceTypeProperty;
    TriggerOperandType GetSourceType() const { return ra::itoe<TriggerOperandType>(GetValue(SourceTypeProperty)); }
    void SetSourceType(TriggerOperandType nValue) { SetValue(SourceTypeProperty, ra::etoi(nValue)); }

    static const IntModelProperty SourceSizeProperty;
    MemSize GetSourceSize() const { return ra::itoe<MemSize>(GetValue(SourceSizeProperty)); }
    void SetSourceSize(MemSize nValue) { SetValue(SourceSizeProperty, ra::etoi(nValue)); }

    static const IntModelProperty SourceValueProperty;
    unsigned int GetSourceValue() const { return ra::to_unsigned(GetValue(SourceValueProperty)); }
    void SetSourceValue(unsigned int nValue) { SetValue(SourceValueProperty, ra::to_signed(nValue)); }

    static const IntModelProperty OperatorProperty;
    TriggerOperatorType GetOperator() const { return ra::itoe<TriggerOperatorType>(GetValue(OperatorProperty)); }
    void SetOperator(TriggerOperatorType nValue) { SetValue(OperatorProperty, ra::etoi(nValue)); }

    static const IntModelProperty TargetTypeProperty;
    TriggerOperandType GetTargetType() const { return ra::itoe<TriggerOperandType>(GetValue(TargetTypeProperty)); }
    void SetTargetType(TriggerOperandType nValue) { SetValue(TargetTypeProperty, ra::etoi(nValue)); }

    static const IntModelProperty TargetSizeProperty;
    MemSize GetTargetSize() const { return ra::itoe<MemSize>(GetValue(TargetSizeProperty)); }
    void SetTargetSize(MemSize nValue) { SetValue(TargetSizeProperty, ra::etoi(nValue)); }

    static const IntModelProperty TargetValueProperty;
    unsigned int GetTargetValue() const { return ra::to_unsigned(GetValue(TargetValueProperty)); }
    void SetTargetValue(unsigned int nValue) { SetValue(TargetValueProperty, ra::to_signed(nValue)); }

    static const IntModelProperty CurrentHitsProperty;
    unsigned int GetCurrentHits() const { return ra::to_unsigned(GetValue(CurrentHitsProperty)); }
    void SetCurrentHits(unsigned int nValue) { SetValue(CurrentHitsProperty, ra::to_signed(nValue)); }

    static const IntModelProperty RequiredHitsProperty;
    unsigned int GetRequiredHits() const { return ra::to_unsigned(GetValue(RequiredHitsProperty)); }
    void SetRequiredHits(unsigned int nValue) { SetValue(RequiredHitsProperty, ra::to_signed(nValue)); }

    static const BoolModelProperty IsSelectedProperty;
    bool IsSelected() const { return GetValue(IsSelectedProperty); }
    void SetSelected(bool bValue) { SetValue(IsSelectedProperty, bValue); }

    std::string Serialize() const;
    void SerializeAppend(std::string& sBuffer) const;

    void InitializeFrom(const struct rc_condition_t& pCondition);

private:
    void SerializeAppendOperand(std::string& sBuffer, TriggerOperandType nType, MemSize nSize, unsigned int nValue) const;

    void SetOperand(const IntModelProperty& pTypeProperty, const IntModelProperty& pSizeProperty,
        const IntModelProperty& pValueProperty, const rc_operand_t& operand);
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_TRIGGERCONDITIONVIEWMODEL_H
