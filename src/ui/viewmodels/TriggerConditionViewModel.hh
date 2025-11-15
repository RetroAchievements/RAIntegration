#ifndef RA_UI_TRIGGERCONDITIONVIEWMODEL_H
#define RA_UI_TRIGGERCONDITIONVIEWMODEL_H
#pragma once

#include "data\Types.hh"

#include "services\AchievementLogicSerializer.hh"

#include "ui\ViewModelBase.hh"
#include "ui\Types.hh"

#include "util\TypeCasts.hh"

struct rc_condition_t;

namespace ra {
namespace ui {
namespace viewmodels {

using ra::services::TriggerConditionType;
using ra::services::TriggerOperandType;
using ra::services::TriggerOperatorType;

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

    static const BoolModelProperty HasSourceValueProperty;
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

    static const BoolModelProperty HasTargetValueProperty;
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
    int GetTotalHits() const { return GetValue(TotalHitsProperty); }
    void SetTotalHits(int nValue) { SetValue(TotalHitsProperty, nValue); }

    static const BoolModelProperty IsIndirectProperty;
    bool IsIndirect() const { return GetValue(IsIndirectProperty); }
    void SetIndirect(bool bValue) { SetValue(IsIndirectProperty, bValue); }
    ra::ByteAddress GetIndirectAddress(ra::ByteAddress nAddress, std::wstring& sPointerChain, const ra::data::models::CodeNoteModel** pLeafNote) const;

    static const BoolModelProperty IsSelectedProperty;
    bool IsSelected() const { return GetValue(IsSelectedProperty); }
    void SetSelected(bool bValue) { SetValue(IsSelectedProperty, bValue); }

    std::string Serialize() const;
    void SerializeAppend(std::string& sBuffer) const;

    void InitializeFrom(const struct rc_condition_t& pCondition);
    void SetTriggerViewModel(const ViewModelBase* pTriggerViewModel) noexcept { m_pTriggerViewModel = pTriggerViewModel; }

    std::wstring GetTooltip(const StringModelProperty& nProperty) const;
    std::wstring GetTooltip(const IntModelProperty& nProperty) const;

    bool IsModifying() const { return IsModifying(GetType()); }

    static bool IsComparisonVisible(const ViewModelBase& vmItem, int nValue);
    static std::wstring FormatValue(unsigned nValue, TriggerOperandType nType);
    static std::wstring FormatValue(float fValue, TriggerOperandType nType);

    static const IntModelProperty RowColorProperty;
    Color GetRowColor() const { return Color(ra::to_unsigned(GetValue(RowColorProperty))); }
    void SetRowColor(Color value) { SetValue(RowColorProperty, ra::to_signed(value.ARGB)); }
    void UpdateRowColor(const rc_condition_t* pCondition);

    void PushValue(const ra::data::IntModelProperty& pProperty, int nNewValue);

private:
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const BoolModelProperty::ChangeArgs& args) override;

    void SerializeAppendOperand(std::string& sBuffer, TriggerOperandType nType, MemSize nSize, const std::wstring& nValue) const;

    static std::wstring GetValueTooltip(unsigned int nValue);
    std::wstring GetAddressTooltip(ra::ByteAddress nAddress, const std::wstring& sPointerChain, const ra::data::models::CodeNoteModel* pNote) const;
    std::wstring GetRecallTooltip(bool bOperand2) const;
    ra::ByteAddress GetSourceAddress() const;
    ra::ByteAddress GetTargetAddress() const;
    const rc_condition_t* GetFirstCondition() const;
    const rc_condition_t* GetCondition() const;
    void SetOperand(const IntModelProperty& pTypeProperty, const IntModelProperty& pSizeProperty,
        const StringModelProperty& pValueProperty, const rc_operand_t& operand);
    void ChangeOperandType(const StringModelProperty& sValueProperty, TriggerOperandType nOldType, TriggerOperandType nNewType);

    static bool IsModifying(TriggerConditionType nType) noexcept;
    static bool IsAddressType(TriggerOperandType nType) noexcept;
    static bool IsParameterlessType(TriggerOperandType nType) noexcept;
    void UpdateHasHits();
    bool IsForValue() const noexcept;

    const ViewModelBase* m_pTriggerViewModel = nullptr;
};

} // namespace viewmodels
} // namespace ui
} // namespace ra

#endif !RA_UI_TRIGGERCONDITIONVIEWMODEL_H
