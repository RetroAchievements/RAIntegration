#include "TriggerConditionViewModel.hh"

#include "RA_Defs.h"
#include "RA_StringUtils.h"

#include "data\context\GameContext.hh"
#include "data\models\TriggerValidation.hh"

#include "services\AchievementLogicSerializer.hh"
#include "services\AchievementRuntime.hh"
#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\TriggerViewModel.hh"
#include "ui\EditorTheme.hh"

#include <rcheevos\src\rcheevos\rc_internal.h>

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty TriggerConditionViewModel::IndexProperty("TriggerConditionViewModel", "Index", 1);
const IntModelProperty TriggerConditionViewModel::TypeProperty("TriggerConditionViewModel", "Type", ra::etoi(TriggerConditionType::Standard));
const IntModelProperty TriggerConditionViewModel::SourceTypeProperty("TriggerConditionViewModel", "SourceType", ra::etoi(TriggerOperandType::Address));
const IntModelProperty TriggerConditionViewModel::SourceSizeProperty("TriggerConditionViewModel", "SourceSize", ra::etoi(MemSize::EightBit));
const StringModelProperty TriggerConditionViewModel::SourceValueProperty("TriggerConditionViewModel", "SourceValue", L"0");
const IntModelProperty TriggerConditionViewModel::OperatorProperty("TriggerConditionViewModel", "Operator", ra::etoi(TriggerOperatorType::Equals));
const IntModelProperty TriggerConditionViewModel::TargetTypeProperty("TriggerConditionViewModel", "TargetType", ra::etoi(TriggerOperandType::Value));
const IntModelProperty TriggerConditionViewModel::TargetSizeProperty("TriggerConditionViewModel", "TargetSize", ra::etoi(MemSize::ThirtyTwoBit));
const StringModelProperty TriggerConditionViewModel::TargetValueProperty("TriggerConditionViewModel", "TargetValue", L"0");
const IntModelProperty TriggerConditionViewModel::CurrentHitsProperty("TriggerConditionViewModel", "CurrentHits", 0);
const IntModelProperty TriggerConditionViewModel::RequiredHitsProperty("TriggerConditionViewModel", "RequiredHits", 0);
const IntModelProperty TriggerConditionViewModel::TotalHitsProperty("TriggerConditionViewModel", "TotalHits", 0);
const BoolModelProperty TriggerConditionViewModel::IsSelectedProperty("TriggerConditionViewModel", "IsSelected", false);
const BoolModelProperty TriggerConditionViewModel::IsIndirectProperty("TriggerConditionViewModel", "IsIndirect", false);
const BoolModelProperty TriggerConditionViewModel::HasSourceSizeProperty("TriggerConditionViewModel", "HasSourceSize", true);
const BoolModelProperty TriggerConditionViewModel::HasSourceValueProperty("TriggerConditionViewModel", "HasSourceValue", true);
const BoolModelProperty TriggerConditionViewModel::HasTargetProperty("TriggerConditionViewModel", "HasTarget", true);
const BoolModelProperty TriggerConditionViewModel::HasTargetSizeProperty("TriggerConditionViewModel", "HasTargetSize", false);
const BoolModelProperty TriggerConditionViewModel::HasTargetValueProperty("TriggerConditionViewModel", "HasTargetValue", true);
const BoolModelProperty TriggerConditionViewModel::HasHitsProperty("TriggerConditionViewModel", "HasHits", true);
const BoolModelProperty TriggerConditionViewModel::CanEditHitsProperty("TriggerConditionViewModel", "CanEditHits", true);
const IntModelProperty TriggerConditionViewModel::RowColorProperty("TriggerConditionViewModel", "RowColor", 0);

constexpr ra::ByteAddress UNKNOWN_ADDRESS = 0xFFFFFFFF;

std::string TriggerConditionViewModel::Serialize() const
{
    std::string buffer;
    SerializeAppend(buffer);
    return buffer;
}

void TriggerConditionViewModel::SerializeAppend(std::string& sBuffer) const
{
    const auto nType = GetType();
    if (nType != TriggerConditionType::Standard)
    {
        if (nType == TriggerConditionType::Measured)
        {
            const auto* pTriggerViewModel = dynamic_cast<const TriggerViewModel*>(m_pTriggerViewModel);
            if (pTriggerViewModel != nullptr && pTriggerViewModel->IsMeasuredTrackedAsPercent())
                ra::services::AchievementLogicSerializer::AppendConditionType(sBuffer, TriggerConditionType::MeasuredAsPercent);
            else
                ra::services::AchievementLogicSerializer::AppendConditionType(sBuffer, TriggerConditionType::Measured);
        }
        else
        {
            ra::services::AchievementLogicSerializer::AppendConditionType(sBuffer, nType);
        }
    }

    SerializeAppendOperand(sBuffer, GetSourceType(), GetSourceSize(), GetSourceValue());

    const auto nOperator = GetOperator();
    if (nOperator != TriggerOperatorType::None)
    {
        ra::services::AchievementLogicSerializer::AppendOperator(sBuffer, nOperator);
        SerializeAppendOperand(sBuffer, GetTargetType(), GetTargetSize(), GetTargetValue());
    }

    if (GetValue(HasHitsProperty) && GetValue(CanEditHitsProperty))
        ra::services::AchievementLogicSerializer::AppendHitTarget(sBuffer, GetRequiredHits());
}

void TriggerConditionViewModel::SerializeAppendOperand(std::string& sBuffer, TriggerOperandType nType, MemSize nSize, const std::wstring& sValue) const
{
    unsigned int nValue = 0;
    float fValue = 0.0;
    std::wstring sError;

    switch (nType)
    {
        case TriggerOperandType::Value:
            if (!ra::ParseNumeric(sValue, nValue, sError))
                nValue = 0;

            ra::services::AchievementLogicSerializer::AppendOperand(sBuffer, nType, nSize, nValue);
            break;

        case TriggerOperandType::Float:
            if (!ra::ParseFloat(sValue, fValue, sError))
                fValue = 0.0;

            ra::services::AchievementLogicSerializer::AppendOperand(sBuffer, nType, nSize, fValue);
            break;

        default:
            if (!ra::ParseHex(sValue, 0xFFFFFFFF, nValue, sError))
                nValue = 0;

            ra::services::AchievementLogicSerializer::AppendOperand(sBuffer, nType, nSize, nValue);
            break;
    }
}

static std::wstring FormatTypedValue(rc_typed_value_t& pValue, TriggerOperandType nType)
{
    switch (nType)
    {
        case TriggerOperandType::Value:
        {
            rc_typed_value_convert(&pValue, RC_VALUE_TYPE_UNSIGNED);
            const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
            if (pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal))
                return std::to_wstring(pValue.value.u32);

            return ra::StringPrintf(L"0x%02x", pValue.value.u32);
        }

        case TriggerOperandType::Float:
        {
            rc_typed_value_convert(&pValue, RC_VALUE_TYPE_FLOAT);
            auto sFloat = std::to_wstring(pValue.value.f32);
            if (sFloat.find('.') != std::string::npos)
            {
                while (sFloat.back() == '0')
                    sFloat.pop_back();
                if (sFloat.back() == '.')
                    sFloat.push_back('0');
            }
            return sFloat;
        }

        default:
            rc_typed_value_convert(&pValue, RC_VALUE_TYPE_UNSIGNED);
            return ra::Widen(ra::ByteAddressToString(pValue.value.u32));
    }
}

std::wstring TriggerConditionViewModel::FormatValue(unsigned nValue, TriggerOperandType nType)
{
    rc_typed_value_t pValue;
    pValue.type = RC_VALUE_TYPE_UNSIGNED;
    pValue.value.u32 = nValue;
    return FormatTypedValue(pValue, nType);
}

std::wstring TriggerConditionViewModel::FormatValue(float fValue, TriggerOperandType nType)
{
    rc_typed_value_t pValue;
    pValue.type = RC_VALUE_TYPE_FLOAT;
    pValue.value.f32 = fValue;
    return FormatTypedValue(pValue, nType);
}

void TriggerConditionViewModel::ChangeOperandType(const StringModelProperty& sValueProperty, TriggerOperandType nOldType, TriggerOperandType nNewType)
{
    const auto& sValue = GetValue(sValueProperty);
    std::wstring sError;

    rc_typed_value_t pValue{};
    if (nOldType == TriggerOperandType::Float)
    {
        pValue.type = RC_VALUE_TYPE_FLOAT;
        ra::ParseFloat(sValue, pValue.value.f32, sError);
    }
    else
    {
        pValue.type = RC_VALUE_TYPE_UNSIGNED;
        if (sValue.length() > 2 && sValue.at(1) == 'x')
            ra::ParseHex(sValue, 0xFFFFFFFF, pValue.value.u32, sError);
        else
            ra::ParseUnsignedInt(sValue, 0xFFFFFFFF, pValue.value.u32, sError);
    }

    SetValue(sValueProperty, FormatTypedValue(pValue, nNewType));
}

void TriggerConditionViewModel::SetOperand(const IntModelProperty& pTypeProperty,
    const IntModelProperty& pSizeProperty, const StringModelProperty& pValueProperty, const rc_operand_t& operand)
{
    rc_typed_value_t pValue{};

    const auto nType = static_cast<TriggerOperandType>(operand.type);
    SetValue(pTypeProperty, ra::etoi(nType));

    switch (nType)
    {
        case TriggerOperandType::Value:
            SetValue(pSizeProperty, ra::etoi(MemSize::ThirtyTwoBit));
            pValue.type = RC_VALUE_TYPE_UNSIGNED;
            pValue.value.u32 = operand.value.num;
            break;

        case TriggerOperandType::Float:
            SetValue(pSizeProperty, ra::etoi(MemSize::Float));
            pValue.type = RC_VALUE_TYPE_FLOAT;
            pValue.value.f32 = gsl::narrow_cast<float>(operand.value.dbl);
            break;

        case TriggerOperandType::Address:
        case TriggerOperandType::Delta:
        case TriggerOperandType::Prior:
        case TriggerOperandType::BCD:
        case TriggerOperandType::Inverted:
        {
            const auto nSize = ra::data::models::TriggerValidation::MapRcheevosMemSize(operand.size);
            SetValue(pSizeProperty, ra::etoi(nSize));
            pValue.type = RC_VALUE_TYPE_UNSIGNED;
            pValue.value.u32 = operand.value.memref->address;
            break;
        }

        case TriggerOperandType::Recall:
            SetValue(pSizeProperty, ra::etoi(MemSize::ThirtyTwoBit));
            pValue.type = RC_VALUE_TYPE_UNSIGNED;
            pValue.value.u32 = 1;
            break;

        default:
            Expects(!"Unknown operand type");
            break;
    }

    SetValue(pValueProperty, FormatTypedValue(pValue, nType));
}

void TriggerConditionViewModel::InitializeFrom(const rc_condition_t& pCondition)
{
    SetType(static_cast<TriggerConditionType>(pCondition.type));

    const auto* pOperand = rc_condition_get_real_operand1(&pCondition);
    Expects(pOperand != nullptr);
    SetOperand(SourceTypeProperty, SourceSizeProperty, SourceValueProperty, *pOperand);

    // if the runtime has optimized a delta into a delta read of a non-delta chain, we
    // have to select the delta value manually
    if (pOperand != &pCondition.operand1 &&
        (pCondition.operand1.type == RC_OPERAND_DELTA || pCondition.operand1.type == RC_OPERAND_PRIOR))
    {
        const auto nType = static_cast<TriggerOperandType>(pCondition.operand1.type);
        SetSourceType(nType);
    }

    SetOperand(TargetTypeProperty, TargetSizeProperty, TargetValueProperty, pCondition.operand2);

    SetOperator(static_cast<TriggerOperatorType>(pCondition.oper));

    SetCurrentHits(pCondition.current_hits);

    // Values have an "infinite" hit target, display 0
    SetRequiredHits(pCondition.required_hits == 0xFFFFFFFF ? 0 : pCondition.required_hits);
}

bool TriggerConditionViewModel::IsForValue() const noexcept
{
    const auto* pTriggerViewModel = dynamic_cast<const TriggerViewModel*>(m_pTriggerViewModel);
    return (pTriggerViewModel && pTriggerViewModel->IsValue());
}

void TriggerConditionViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    // suspend the condition monitor so it only handles one change event regardless of how many properties we change
    const auto* pTriggerViewModel = dynamic_cast<const TriggerViewModel*>(m_pTriggerViewModel);
    if (pTriggerViewModel != nullptr)
        pTriggerViewModel->SuspendConditionMonitor();

    if (args.Property == SourceTypeProperty)
    {
        const auto nOldType = ra::itoe<TriggerOperandType>(args.tOldValue);
        const auto nNewType = ra::itoe<TriggerOperandType>(args.tNewValue);
        ChangeOperandType(SourceValueProperty, nOldType, nNewType);

        if (!IsAddressType(nNewType))
        {
            SetValue(HasSourceSizeProperty, false);
            SetSourceSize(nNewType == TriggerOperandType::Value || IsParameterlessType(nNewType) ? MemSize::ThirtyTwoBit : MemSize::Float);
        }
        else if (!IsAddressType(nOldType))
        {
            SetSourceSize(GetTargetSize());
            SetValue(HasSourceSizeProperty, true);
        }

        SetValue(HasSourceValueProperty, IsParameterlessType(nNewType) ? false : true);
    }
    else if (args.Property == TargetTypeProperty)
    {
        const auto nOldType = ra::itoe<TriggerOperandType>(args.tOldValue);
        const auto nNewType = ra::itoe<TriggerOperandType>(args.tNewValue);
        ChangeOperandType(TargetValueProperty, nOldType, nNewType);

        if (!IsAddressType(nNewType))
        {
            SetValue(HasTargetSizeProperty, false);
            SetTargetSize(nNewType == TriggerOperandType::Value || IsParameterlessType(nNewType) ? MemSize::ThirtyTwoBit : MemSize::Float);
        }
        else if (!IsAddressType(ra::itoe<TriggerOperandType>(args.tOldValue)))
        {
            SetTargetSize(GetSourceSize());
            SetValue(HasTargetSizeProperty, GetValue(HasTargetProperty));
        }

        SetValue(HasTargetValueProperty, IsParameterlessType(nNewType) || !GetValue(HasTargetProperty) ? false : true);
    }
    else if (args.Property == OperatorProperty)
    {
        SetValue(HasTargetProperty, ra::itoe<TriggerOperatorType>(args.tNewValue) != TriggerOperatorType::None);
        UpdateHasHits();
    }
    else if (args.Property == TypeProperty)
    {
        const auto nOldType = ra::itoe<TriggerConditionType>(args.tOldValue);
        const auto nNewType = ra::itoe<TriggerConditionType>(args.tNewValue);
        const auto bIsModifying = IsModifying(nNewType);
        if (bIsModifying != IsModifying(nOldType))
            SetOperator(bIsModifying ? TriggerOperatorType::None : TriggerOperatorType::Equals);
        else if (!bIsModifying && GetOperator() == TriggerOperatorType::None)
            SetOperator(TriggerOperatorType::Equals);

        UpdateHasHits();
    }

    ViewModelBase::OnValueChanged(args);

    // resume after calling base, as that's where the ConditionMonitor will handle the current event
    if (pTriggerViewModel != nullptr)
        pTriggerViewModel->ResumeConditionMonitor();
}

static constexpr bool IsModifyingOperator(TriggerOperatorType nType)
{
    switch (nType)
    {
        case TriggerOperatorType::None:
        case TriggerOperatorType::Multiply:
        case TriggerOperatorType::Divide:
        case TriggerOperatorType::BitwiseAnd:
        case TriggerOperatorType::BitwiseXor:
        case TriggerOperatorType::Modulus:
        case TriggerOperatorType::Add:
        case TriggerOperatorType::Subtract:
            return true;

        default:
            return false;
    }
}

void TriggerConditionViewModel::UpdateHasHits()
{
    const auto nType = GetType();

    if (IsModifying(nType))
    {
        SetValue(HasHitsProperty, false);
        SetValue(CanEditHitsProperty, false);
    }
    else if (nType == TriggerConditionType::Measured && IsForValue())
    {
        // if Measured value is generated from hit count, show the hit count, but don't allow the target to be edited
        SetValue(HasHitsProperty, !IsModifyingOperator(GetOperator()));
        SetValue(CanEditHitsProperty, false);
        SetValue(RequiredHitsProperty, 0);
    }
    else
    {
        SetValue(HasHitsProperty, true);
        SetValue(CanEditHitsProperty, true);
    }
}

void TriggerConditionViewModel::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == HasTargetProperty)
    {
        const auto nTargetType = GetTargetType();
        SetValue(HasTargetSizeProperty, args.tNewValue && IsAddressType(nTargetType));
        SetValue(HasTargetValueProperty, args.tNewValue && !IsParameterlessType(nTargetType));
    }

    ViewModelBase::OnValueChanged(args);
}

void TriggerConditionViewModel::SetSourceValue(unsigned int nValue)
{
    SetValue(SourceValueProperty, FormatValue(nValue, GetSourceType()));
}

ra::ByteAddress TriggerConditionViewModel::GetSourceAddress() const
{
    unsigned int nValue = 0;
    std::wstring sError;
    ra::ParseHex(GetSourceValue(), 0xFFFFFFFF, nValue, sError);
    return nValue;
}

void TriggerConditionViewModel::SetTargetValue(unsigned int nValue)
{
    SetValue(TargetValueProperty, FormatValue(nValue, GetTargetType()));
}

void TriggerConditionViewModel::SetTargetValue(float fValue)
{
    SetValue(TargetValueProperty, FormatValue(fValue, GetTargetType()));
}

ra::ByteAddress TriggerConditionViewModel::GetTargetAddress() const
{
    unsigned int nValue = 0;
    std::wstring sError;
    ra::ParseHex(GetTargetValue(), 0xFFFFFFFF, nValue, sError);
    return nValue;
}

std::wstring TriggerConditionViewModel::GetTooltip(const StringModelProperty& nProperty) const
{
    if (nProperty == SourceValueProperty)
    {
        const auto nType = GetSourceType();
        if (nType == TriggerOperandType::Value)
            return GetValueTooltip(GetSourceAddress());

        if (nType == TriggerOperandType::Float)
            return L"";

        if (nType == TriggerOperandType::Recall)
            return L"";

        if (IsIndirect())
        {
            const ra::data::models::CodeNoteModel* pNote = nullptr;
            std::wstring sPointerChain;
            const auto nOffset = GetSourceAddress();
            const auto nIndirectAddress = GetIndirectAddress(nOffset, sPointerChain, &pNote);
            return GetAddressTooltip(nIndirectAddress, sPointerChain, pNote);
        }

        return GetAddressTooltip(GetSourceAddress(), L"", nullptr);
    }

    if (nProperty == TargetValueProperty && GetValue(HasTargetValueProperty))
    {
        const auto nType = GetTargetType();
        if (nType == TriggerOperandType::Value)
            return GetValueTooltip(GetTargetAddress());

        if (nType == TriggerOperandType::Float)
            return L"";

        if (nType == TriggerOperandType::Recall)
            return L"";

        if (IsIndirect())
        {
            const ra::data::models::CodeNoteModel* pNote = nullptr;
            std::wstring sPointerChain;
            const auto nOffset = GetTargetAddress();
            const auto nIndirectAddress = GetIndirectAddress(nOffset, sPointerChain, &pNote);
            return GetAddressTooltip(nIndirectAddress, sPointerChain, pNote);
        }

        return GetAddressTooltip(GetTargetAddress(), L"", nullptr);
    }

    return L"";
}

std::wstring TriggerConditionViewModel::GetTooltip(const IntModelProperty& nProperty) const
{
    if (nProperty == SourceTypeProperty)
    {
        const auto nType = GetSourceType();
        if (nType == TriggerOperandType::Recall)
            return GetRecallTooltip(false);
    }

    if (nProperty == TargetTypeProperty)
    {
        const auto nType = GetTargetType();
        if (nType == TriggerOperandType::Recall)
            return GetRecallTooltip(true);
    }

    return L"";
}

std::wstring TriggerConditionViewModel::GetValueTooltip(unsigned int nValue)
{
    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal))
        return L"";

    return std::to_wstring(nValue);
}

static void BuildOperandTooltip(std::wstring& sTooltip, const rc_operand_t& pOperand)
{
    rc_typed_value_t pValue;
    rc_evaluate_operand(&pValue, &pOperand, nullptr);
    switch (pValue.type)
    {
        case RC_VALUE_TYPE_FLOAT:
            sTooltip += FormatTypedValue(pValue, ra::services::TriggerOperandType::Float);
            break;
        case RC_VALUE_TYPE_SIGNED:
            sTooltip += ra::StringPrintf(L"%d", pValue.value.i32);
            break;
        default:
            sTooltip += ra::StringPrintf(L"0x%08x", pValue.value.u32);
            break;
    }
}

static void BuildOperatorTooltip(std::wstring& sTooltip, uint8_t nOperatorType)
{
    switch (nOperatorType)
    {
        case RC_OPERATOR_MULT:
            sTooltip.append(L" * ");
            break;
        case RC_OPERATOR_DIV:
            sTooltip.append(L" / ");
            break;
        case RC_OPERATOR_AND:
            sTooltip.append(L" & ");
            break;
        case RC_OPERATOR_XOR:
            sTooltip.append(L" ^ ");
            break;
        case RC_OPERATOR_MOD:
            sTooltip.append(L" % ");
            break;
        case RC_OPERATOR_ADD:
            sTooltip.append(L" + ");
            break;
        case RC_OPERATOR_SUB:
            sTooltip.append(L" - ");
            break;
        default:
            break;
    }
}

static ra::ByteAddress GetIndirectAddressFromOperand(const rc_operand_t* pOperand, std::wstring& sPointerChain,
    const ra::data::models::CodeNoteModel** pParentNote)
{
    rc_typed_value_t pValue;
    rc_evaluate_operand(&pValue, pOperand, nullptr);
    rc_typed_value_convert(&pValue, RC_VALUE_TYPE_UNSIGNED);

    // check for {recall}
    if (pOperand->type == RC_OPERAND_RECALL)
    {
        *pParentNote = nullptr;

        sPointerChain += ra::StringPrintf(L"{recall:0x%02x}", pValue.value.u32);
        return pValue.value.u32;
    }

    // check for constant offset
    if (!rc_operand_is_memref(pOperand))
    {
        if (*pParentNote)
            *pParentNote = (*pParentNote)->GetPointerNoteAtOffset(pValue.value.u32);

        sPointerChain += ra::StringPrintf(L"0x%02x", pValue.value.u32);
        return pValue.value.u32;
    }

    // check for root pointer
    if (pOperand->value.memref->value.memref_type != RC_MEMREF_TYPE_MODIFIED_MEMREF)
    {
        const auto nAddress = pOperand->value.memref->address;

        // find the code note associated to the parent
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
        const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
        *pParentNote = pCodeNotes ? pCodeNotes->FindCodeNoteModel(nAddress, false) : nullptr;

        sPointerChain.push_back('$');
        sPointerChain += ra::Widen(ra::ByteAddressToString(nAddress));
        return pValue.value.u32;
    }

    // process offset
    GSL_SUPPRESS_TYPE1 const auto* pModifiedMemref =
        reinterpret_cast<const rc_modified_memref_t*>(pOperand->value.memref);

    GetIndirectAddressFromOperand(&pModifiedMemref->parent, sPointerChain, pParentNote);

    // calculate current values of both operands
    rc_evaluate_operand(&pValue, &pModifiedMemref->parent, nullptr);
    rc_typed_value_convert(&pValue, RC_VALUE_TYPE_UNSIGNED);

    rc_typed_value_t pModifier{};
    rc_evaluate_operand(&pModifier, &pModifiedMemref->modifier, nullptr);
    rc_typed_value_convert(&pModifier, RC_VALUE_TYPE_UNSIGNED);

    // if it's an indirect read, determine if it's a pointer or index offset
    if (pModifiedMemref->modifier_type == RC_OPERATOR_INDIRECT_READ)
    {
        rc_typed_value_combine(&pValue, &pModifier, RC_OPERATOR_ADD);

        if (*pParentNote && !(*pParentNote)->IsPointer())
        {
            // if the parent note is not a pointer, assume it's an index
            Expects(pModifiedMemref->modifier.type == RC_OPERAND_CONST);
            const auto nAddress = pModifier.value.u32;
            std::wstring sPrefix = ra::StringPrintf(L"%s[", ra::ByteAddressToString(nAddress));
            sPointerChain.insert(0, sPrefix);
            sPointerChain.push_back(']');

            // find the code note associated to the start of the array
            const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
            const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
            *pParentNote = (*pParentNote) ? pCodeNotes->FindCodeNoteModel(nAddress, false) : nullptr;

            // return the address offset into the array
            return pValue.value.u32;
        }

        // process the pointer
        sPointerChain.push_back('+');
        GetIndirectAddressFromOperand(&pModifiedMemref->modifier, sPointerChain, pParentNote);

        // value is the parent pointer, modifier is the offset. combine them to get the new address
        return pValue.value.u32;
    }

    // not an indirect read. must be a scalar.
    if (pModifiedMemref->modifier_type != RC_OPERATOR_AND)
    {
        // don't report mask on every pointer evaluation
        std::wstring sTemp;
        BuildOperatorTooltip(sTemp, pModifiedMemref->modifier_type);
        sPointerChain.push_back(sTemp.empty() ? '?' : sTemp.at(1));

        if (pModifiedMemref->modifier.type == RC_OPERAND_RECALL)
        {
            sPointerChain += ra::StringPrintf(L"{recall:0x%02x}", pModifier.value.u32);
        }
        else if (rc_operand_is_memref(&pModifiedMemref->modifier))
        {
            sPointerChain.push_back('$');
            sPointerChain += ra::Widen(ra::ByteAddressToString(pModifiedMemref->modifier.value.memref->address));
        }
        else
        {
            const auto nModifier = pModifiedMemref->modifier.value.num;

            switch (pModifiedMemref->modifier_type)
            {
                case RC_OPERATOR_AND:
                case RC_OPERATOR_XOR: // use hex for bitwise combines
                    sPointerChain += ra::StringPrintf(L"0x%02x", nModifier);
                    break;

                default:
                    if (nModifier >= 0x1000 && (nModifier & 0xFF) == 0) // large multiple of 256, use hex
                        sPointerChain += ra::StringPrintf(L"0x%04x", nModifier);
                    else // otherwise use decimal
                        sPointerChain += std::to_wstring(nModifier);
                    break;
            }
        }
    }

    // return the result of combining the value and the modifier
    rc_typed_value_combine(&pValue, &pModifier, pModifiedMemref->modifier_type);
    return pValue.value.u32;
}

const rc_condition_t* TriggerConditionViewModel::GetFirstCondition() const
{
    const auto* pTriggerViewModel = dynamic_cast<const TriggerViewModel*>(m_pTriggerViewModel);
    if (pTriggerViewModel == nullptr)
        return nullptr;

    const auto nIndex = pTriggerViewModel->GetSelectedGroupIndex();
    const auto* pGroup = pTriggerViewModel->Groups().GetItemAt(nIndex);
    if (pGroup == nullptr)
        return nullptr;

    rc_condition_t* pFirstCondition = nullptr;

    auto* pTrigger = pTriggerViewModel->GetTriggerFromString();
    if (pTrigger != nullptr)
    {
        // if the trigger is managed by the viewmodel (not the runtime) then we need to update the memrefs
        auto* memrefs = rc_trigger_get_memrefs(pTrigger);
        if (memrefs)
            rc_update_memref_values(memrefs, rc_peek_callback, nullptr);

        // find the condset associated to the selected group
        if (nIndex == 0)
        {
            pFirstCondition = pTrigger->requirement->conditions;
        }
        else
        {
            rc_condset_t* pAlt = pTrigger->alternative;
            for (int i = 1; pAlt && i < nIndex; ++i)
                pAlt = pAlt->next;

            if (!pAlt)
                return nullptr;

            pFirstCondition = pAlt->conditions;
        }
    }
    else
    {
        Expects(pGroup->m_pConditionSet != nullptr);
        pFirstCondition = pGroup->m_pConditionSet->conditions;
    }

    return pFirstCondition;
}

const rc_condition_t* TriggerConditionViewModel::GetCondition() const
{
    const rc_condition_t* pCondition = GetFirstCondition();
    if (!pCondition)
        return nullptr;

    const auto* pTriggerViewModel = dynamic_cast<const TriggerViewModel*>(m_pTriggerViewModel);
    if (pTriggerViewModel != nullptr)
    {
        auto nScrollOffset = pTriggerViewModel->GetScrollOffset();
        while (nScrollOffset)
        {
            pCondition = pCondition->next;
            if (!pCondition)
                break;

            --nScrollOffset;
        }

        gsl::index nConditionIndex = 0;
        for (; pCondition != nullptr; pCondition = pCondition->next)
        {

            auto* vmCondition = pTriggerViewModel->Conditions().GetItemAt(nConditionIndex++);
            if (!vmCondition)
                break;

            if (vmCondition == this)
                return pCondition;
        }
    }

    return nullptr;
}

ra::ByteAddress TriggerConditionViewModel::GetIndirectAddress(ra::ByteAddress nAddress, std::wstring& sPointerChain,
    const ra::data::models::CodeNoteModel** pLeafNote) const
{
    const auto* pCondition = GetCondition();
    if (pCondition)
    {
        *pLeafNote = nullptr;

        const auto* pOperand1 = rc_condition_get_real_operand1(pCondition);
        if (rc_operand_is_memref(pOperand1) && nAddress == pOperand1->value.memref->address)
            return GetIndirectAddressFromOperand(pOperand1, sPointerChain, pLeafNote);

        if (rc_operand_is_memref(&pCondition->operand2) && nAddress == pCondition->operand2.value.memref->address)
            return GetIndirectAddressFromOperand(&pCondition->operand2, sPointerChain, pLeafNote);
    }

    return nAddress;
}

std::wstring TriggerConditionViewModel::GetAddressTooltip(ra::ByteAddress nAddress,
    const std::wstring& sPointerChain, const ra::data::models::CodeNoteModel* pNote) const
{
    std::wstring sAddress;
    if (sPointerChain.empty())
        sAddress = ra::Widen(ra::ByteAddressToString(nAddress));
    else
        sAddress = ra::StringPrintf(L"%s (indirect %s)", ra::ByteAddressToString(nAddress), sPointerChain);

    if (!pNote)
    {
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
        const auto* pCodeNotes = pGameContext.Assets().FindCodeNotes();
        if (pCodeNotes)
        {
            pNote = pCodeNotes->FindCodeNoteModel(nAddress);
            if (pNote == nullptr)
            {
                const auto nNoteStart = pCodeNotes->FindCodeNoteStart(nAddress);
                if (nNoteStart != 0xFFFFFFFF)
                {
                    pNote = pCodeNotes->FindCodeNoteModel(nNoteStart);

                    if (sPointerChain.empty())
                        sAddress = ra::StringPrintf(L"%s (%s+%u)", ra::ByteAddressToString(nAddress), ra::ByteAddressToString(nNoteStart), nAddress - nNoteStart);
                }
            }
        }
        if (!pNote)
            return ra::StringPrintf(L"%s\r\n[No code note]", sAddress);
    }

    if (pNote->IsPointer() && GetType() == TriggerConditionType::AddAddress)
        return ra::StringPrintf(L"%s\r\n%s", sAddress, pNote->GetPointerDescription());

    // limit the tooltip to the first 20 lines of the code note
    const auto& sNote = pNote->GetNote();
    size_t nLines = 0;
    size_t nIndex = 0;
    do
    {
        nIndex = sNote.find('\n', nIndex);
        if (nIndex == std::string::npos)
            break;

        ++nIndex;
        ++nLines;
    } while (nLines < 20);

    if (nIndex != std::string::npos && sNote.find('\n', nIndex) != std::string::npos)
    {
        std::wstring sSubString(sNote, 0, nIndex);
        return ra::StringPrintf(L"%s\r\n%s...", sAddress, sSubString);
    }

    return ra::StringPrintf(L"%s\r\n%s", sAddress, sNote);
}

static void BuildRecallTooltip(std::wstring& sTooltip,
    const std::map<gsl::index, std::pair<gsl::index, const rc_condition_t*>>& mRememberRef,
    gsl::index nConditionIndex, const rc_condition_t& pCondition, const rc_operand_t& pOperand)
{
    const auto* pOperand1 = rc_condition_get_real_operand1(&pCondition);
    const rc_operand_t* pRecallOperand =
        (pOperand1 && pOperand1->type == RC_OPERAND_RECALL) ? pOperand1 :
        (pCondition.operand2.type == RC_OPERAND_RECALL) ? &pCondition.operand2 : nullptr;
    if (pRecallOperand)
    {
        const auto pIter = mRememberRef.find(nConditionIndex);
        if (pIter != mRememberRef.end())
            BuildRecallTooltip(sTooltip, mRememberRef, pIter->second.first, *pIter->second.second, *pRecallOperand);
    }

    sTooltip += ra::StringPrintf(L"\r\n%u: ", gsl::narrow_cast<uint32_t>(nConditionIndex + 1));

    if (pOperand.value.memref->value.memref_type == RC_MEMREF_TYPE_MODIFIED_MEMREF)
    {
        GSL_SUPPRESS_TYPE1 const rc_modified_memref_t* combining_memref =
            reinterpret_cast<rc_modified_memref_t*>(pOperand.value.memref);
        if (combining_memref->modifier_type == RC_OPERATOR_INDIRECT_READ)
        {
            rc_typed_value_t pValue{}, pModifier{};
            rc_evaluate_operand(&pValue, &combining_memref->parent, nullptr);
            rc_typed_value_convert(&pValue, RC_VALUE_TYPE_UNSIGNED);

            rc_evaluate_operand(&pModifier, &combining_memref->modifier, nullptr);
            rc_typed_value_convert(&pModifier, RC_VALUE_TYPE_UNSIGNED);

            rc_typed_value_combine(&pValue, &pModifier, RC_OPERATOR_ADD);
            sTooltip.push_back('$');
            sTooltip.append(ra::Widen(ra::ByteAddressToString(pValue.value.u32)));
        }
        else
        {
            BuildOperandTooltip(sTooltip, combining_memref->parent);
            if (combining_memref->modifier_type == RC_OPERATOR_NONE)
                return;

            BuildOperatorTooltip(sTooltip, combining_memref->modifier_type);
            BuildOperandTooltip(sTooltip, combining_memref->modifier);
        }

        sTooltip.append(L" -> ");
        BuildOperandTooltip(sTooltip, pOperand);
    }
}

std::wstring TriggerConditionViewModel::GetRecallTooltip(bool bOperand2) const
{
    const auto* pCondition = GetFirstCondition();
    if (!pCondition)
        return L"";

    const auto* pTriggerViewModel = dynamic_cast<const TriggerViewModel*>(m_pTriggerViewModel);
    if (!pTriggerViewModel)
        return L"";

    std::map<gsl::index, std::pair<gsl::index, const rc_condition_t*>> mRememberRef;
    const rc_condition_t* pLastRememberCondition = nullptr;
    gsl::index nLastRememberIndex = -1;

    const auto nFirstVisibleIndex = pTriggerViewModel->GetScrollOffset();
    const auto nLastVisibleIndex = nFirstVisibleIndex + pTriggerViewModel->GetVisibleItemCount() - 1;
    gsl::index nConditionIndex = 0;
    for (; pCondition != nullptr; pCondition = pCondition->next, ++nConditionIndex)
    {
        if (nConditionIndex >= nFirstVisibleIndex && nConditionIndex <= nLastVisibleIndex)
        {
            auto* vmCondition = pTriggerViewModel->Conditions().GetItemAt(nConditionIndex - nFirstVisibleIndex);
            if (vmCondition == this)
                break;
        }

        const auto* pOperand1 = rc_condition_get_real_operand1(pCondition);
        if ((pOperand1 && pOperand1->type == RC_OPERAND_RECALL) || pCondition->operand2.type == RC_OPERAND_RECALL)
            mRememberRef[nConditionIndex] = { nLastRememberIndex, pLastRememberCondition };

        if (pCondition->type == RC_CONDITION_REMEMBER)
        {
            nLastRememberIndex = nConditionIndex;
            pLastRememberCondition = pCondition;
        }
    }

    if (!pCondition)
        return L"";

    std::wstring sTooltip;

    const rc_operand_t* pOperand = nullptr;
    if (bOperand2)
        pOperand = &pCondition->operand2;
    else
        pOperand = rc_condition_get_real_operand1(pCondition);
    Expects(pOperand != nullptr);

    BuildOperandTooltip(sTooltip, *pOperand);
    sTooltip.append(L" (recall)");

    if (pLastRememberCondition)
        BuildRecallTooltip(sTooltip, mRememberRef, nLastRememberIndex, *pLastRememberCondition, *pOperand);
    else
        sTooltip += L"???";

    return sTooltip;
}

bool TriggerConditionViewModel::IsModifying(TriggerConditionType nType) noexcept
{
    switch (nType)
    {
        case TriggerConditionType::AddAddress:
        case TriggerConditionType::AddSource:
        case TriggerConditionType::SubSource:
        case TriggerConditionType::Remember:
            return true;

        default:
            return false;
    }
}

bool TriggerConditionViewModel::IsAddressType(TriggerOperandType nType) noexcept
{
    switch (nType)
    {
        case TriggerOperandType::Value:
        case TriggerOperandType::Float:
        case TriggerOperandType::Recall:
            return false;

        default:
            return true;
    }
}

/* Has no address or value */
bool TriggerConditionViewModel::IsParameterlessType(TriggerOperandType nType) noexcept
{
    switch (nType)
    {
        case TriggerOperandType::Recall:
            return true;
        default:
            return false;
    }
}

bool TriggerConditionViewModel::IsComparisonVisible(const ViewModelBase& vmItem, int nValue)
{
    const auto* vmCondition = dynamic_cast<const TriggerConditionViewModel*>(&vmItem);
    if (vmCondition == nullptr)
        return false;

    // comparison operators should only be visible for non-modifying operations
    const auto nOperator = ra::itoe<TriggerOperatorType>(nValue);
    if (!IsModifyingOperator(nOperator))
        return !vmCondition->IsModifying();

    // modifying operators should be visible for modifying operations
    if (vmCondition->IsModifying())
        return true;

    // modifying operators can be selected for the Measured flag when editing a value
    if (vmCondition->GetType() == TriggerConditionType::Measured && vmCondition->IsForValue())
        return true;

    // modifying operator is not valid at this time
    return false;
}

void TriggerConditionViewModel::UpdateRowColor(const rc_condition_t* pCondition)
{
    if (!pCondition)
    {
        SetValue(RowColorProperty, RowColorProperty.GetDefaultValue());
        return;
    }

    const auto& pTheme = ra::services::ServiceLocator::Get<ra::ui::EditorTheme>();

    if (!pCondition->is_true)
    {
        if (pCondition->required_hits != 0 && pCondition->current_hits == pCondition->required_hits)
        {
            // not true this frame, but target hitcount met
            SetRowColor(pTheme.ColorTriggerWasTrue());
            return;
        }

        // not true this frame, and target hitcount not met
        SetValue(RowColorProperty, RowColorProperty.GetDefaultValue());
    }
    else if (pCondition->required_hits != 0 && pCondition->current_hits != pCondition->required_hits)
    {
        if (pCondition->current_hits == 0 && pCondition->type == RC_CONDITION_RESET_IF && (pCondition->is_true & 0x02))
        {
            // a ResetIf condition with a hit target may have met the hit target before
            // being reset. if the second is_true bit is non-zero, the condition was
            // responsible for resetting the trigger.
            SetRowColor(pTheme.ColorTriggerResetTrue());
        }
        else
        {
            // true this frame, but target hitcount not met
            SetRowColor(pTheme.ColorTriggerBecomingTrue());
        }
    }
    else
    {
        // true this frame, and target hitcount met
        switch (pCondition->type)
        {
            case RC_CONDITION_RESET_IF:
                if (pCondition->is_true & 0x02)
                    SetRowColor(pTheme.ColorTriggerResetTrue());
                else
                    SetRowColor(pTheme.ColorTriggerIsTrue());
                return;

            case RC_CONDITION_PAUSE_IF:
                SetRowColor(pTheme.ColorTriggerPauseTrue());
                return;

            default:
                SetRowColor(pTheme.ColorTriggerIsTrue());
                return;
        }
    }
}

void TriggerConditionViewModel::PushValue(const ra::data::IntModelProperty& pProperty, int nNewValue)
{
    SetValue(pProperty, nNewValue);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
