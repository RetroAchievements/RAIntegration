#ifndef RA_UI_TRIGGERCONDITIONVIEWMODEL_H
#define RA_UI_TRIGGERCONDITIONVIEWMODEL_H
#pragma once

#include "ui\ViewModelBase.hh"
#include "ui\Types.hh"

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
    SubHits = RC_CONDITION_SUB_HITS,
    AndNext = RC_CONDITION_AND_NEXT,
    Measured = RC_CONDITION_MEASURED,
    AddAddress = RC_CONDITION_ADD_ADDRESS,
    OrNext = RC_CONDITION_OR_NEXT,
    Trigger = RC_CONDITION_TRIGGER,
    MeasuredIf = RC_CONDITION_MEASURED_IF,
    ResetNextIf = RC_CONDITION_RESET_NEXT_IF
};

enum class TriggerOperandType : uint8_t
{
    Address = RC_OPERAND_ADDRESS,       // compare to the value of a live address in RAM
    Delta = RC_OPERAND_DELTA,           // the value last known at this address.
    Value = RC_OPERAND_CONST,           // a 32 bit unsigned integer
    Prior = RC_OPERAND_PRIOR,           // the last differing value at this address.
    BCD = RC_OPERAND_BCD,               // Address, but decoded from binary-coded-decimal
    Float = RC_OPERAND_FP,              // a 32-bit floating point value
    Inverted = RC_OPERAND_INVERTED      // the bitwise compliment of the current value at the address
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

    static const BoolModelProperty HasSourceSizeProperty;
    static const IntModelProperty SourceSizeProperty;
    MemSize GetSourceSize() const { return ra::itoe<MemSize>(GetValue(SourceSizeProperty)); }
    void SetSourceSize(MemSize nValue) { SetValue(SourceSizeProperty, ra::etoi(nValue)); }

    static const StringModelProperty SourceValueProperty;
    const std::wstring& GetSourceValue() const { return GetValue(SourceValueProperty); }
    void SetSourceValue(const std::wstring& sValue) { SetValue(SourceValueProperty, sValue); }
    void SetSourceValue(unsigned int sValue);

    static const IntModelProperty OperatorProperty;
    TriggerOperatorType GetOperator() const { return ra::itoe<TriggerOperatorType>(GetValue(OperatorProperty)); }
    void SetOperator(TriggerOperatorType nValue) { SetValue(OperatorProperty, ra::etoi(nValue)); }

    static const BoolModelProperty HasTargetProperty;
    static const IntModelProperty TargetTypeProperty;
    TriggerOperandType GetTargetType() const { return ra::itoe<TriggerOperandType>(GetValue(TargetTypeProperty)); }
    void SetTargetType(TriggerOperandType nValue) { SetValue(TargetTypeProperty, ra::etoi(nValue)); }

    static const BoolModelProperty HasTargetSizeProperty;
    static const IntModelProperty TargetSizeProperty;
    MemSize GetTargetSize() const { return ra::itoe<MemSize>(GetValue(TargetSizeProperty)); }
    void SetTargetSize(MemSize nValue) { SetValue(TargetSizeProperty, ra::etoi(nValue)); }

    static const StringModelProperty TargetValueProperty;
    const std::wstring& GetTargetValue() const { return GetValue(TargetValueProperty); }
    void SetTargetValue(const std::wstring& sValue) { SetValue(TargetValueProperty, sValue); }
    void SetTargetValue(unsigned int nValue);
    void SetTargetValue(float fValue);

    static const BoolModelProperty HasHitsProperty;
    static const BoolModelProperty CanEditHitsProperty;
    static const IntModelProperty CurrentHitsProperty;
    unsigned int GetCurrentHits() const { return ra::to_unsigned(GetValue(CurrentHitsProperty)); }
    void SetCurrentHits(unsigned int nValue) { SetValue(CurrentHitsProperty, ra::to_signed(nValue)); }

    static const IntModelProperty RequiredHitsProperty;
    unsigned int GetRequiredHits() const { return ra::to_unsigned(GetValue(RequiredHitsProperty)); }
    void SetRequiredHits(unsigned int nValue) { SetValue(RequiredHitsProperty, ra::to_signed(nValue)); }

    static const IntModelProperty TotalHitsProperty;
    unsigned int GetTotalHits() const { return ra::to_unsigned(GetValue(TotalHitsProperty)); }
    void SetTotalHits(unsigned int nValue) { SetValue(TotalHitsProperty, ra::to_signed(nValue)); }

    static const BoolModelProperty IsIndirectProperty;
    bool IsIndirect() const { return GetValue(IsIndirectProperty); }
    void SetIndirect(bool bValue) { SetValue(IsIndirectProperty, bValue); }
    ra::ByteAddress GetIndirectAddress(ra::ByteAddress nAddress, ra::ByteAddress* pPointerAddress) const;

    static const BoolModelProperty IsSelectedProperty;
    bool IsSelected() const { return GetValue(IsSelectedProperty); }
    void SetSelected(bool bValue) { SetValue(IsSelectedProperty, bValue); }

    std::string Serialize() const;
    void SerializeAppend(std::string& sBuffer) const;

    void InitializeFrom(const struct rc_condition_t& pCondition);
    void SetTriggerViewModel(const ViewModelBase* pTriggerViewModel) noexcept { m_pTriggerViewModel = pTriggerViewModel; }

    std::wstring GetTooltip(const StringModelProperty& nProperty) const;

    bool IsModifying() const { return IsModifying(GetType()); }

    static bool IsComparisonVisible(const ViewModelBase& vmItem, int nValue);
    static std::wstring FormatValue(unsigned nValue, TriggerOperandType nType);
    static std::wstring FormatValue(float fValue, TriggerOperandType nType);

    static const IntModelProperty RowColorProperty;
    Color GetRowColor() const { return Color(ra::to_unsigned(GetValue(RowColorProperty))); }
    void SetRowColor(Color value) { SetValue(RowColorProperty, ra::to_signed(value.ARGB)); }
    void UpdateRowColor(const rc_condition_t* pCondition);

private:
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;

    void SerializeAppendOperand(std::string& sBuffer, TriggerOperandType nType, MemSize nSize, const std::wstring& nValue) const;

    static std::wstring GetValueTooltip(unsigned int nValue);
    static std::wstring GetAddressTooltip(ra::ByteAddress nAddress, ra::ByteAddress nPointerAddress, ra::ByteAddress nOffset);
    ra::ByteAddress GetSourceAddress() const;
    ra::ByteAddress GetTargetAddress() const;
    void SetOperand(const IntModelProperty& pTypeProperty, const IntModelProperty& pSizeProperty,
        const StringModelProperty& pValueProperty, const rc_operand_t& operand);
    void ChangeOperandType(const StringModelProperty& sValueProperty, TriggerOperandType nOldType, TriggerOperandType nNewType);

    static bool IsModifying(TriggerConditionType nType) noexcept;
    static bool IsAddressType(TriggerOperandType nType) noexcept;
    void UpdateHasHits();
    bool IsForValue() const noexcept;

    const ViewModelBase* m_pTriggerViewModel = nullptr;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_TRIGGERCONDITIONVIEWMODEL_H
