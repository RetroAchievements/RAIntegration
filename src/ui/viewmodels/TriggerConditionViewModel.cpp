#include "TriggerConditionViewModel.hh"

#include "RA_Defs.h"
#include "RA_StringUtils.h"

#include "data\context\GameContext.hh"
#include "data\models\TriggerValidation.hh"

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
const BoolModelProperty TriggerConditionViewModel::HasTargetProperty("TriggerConditionViewModel", "HasTarget", true);
const BoolModelProperty TriggerConditionViewModel::HasTargetSizeProperty("TriggerConditionViewModel", "HasTargetSize", false);
const BoolModelProperty TriggerConditionViewModel::HasHitsProperty("TriggerConditionViewModel", "HasHits", true);
const BoolModelProperty TriggerConditionViewModel::CanEditHitsProperty("TriggerConditionViewModel", "CanEditHits", true);
const IntModelProperty TriggerConditionViewModel::RowColorProperty("TriggerConditionViewModel", "RowColor", 0);

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
        switch (nType)
        {
            case TriggerConditionType::ResetIf:     sBuffer.push_back('R'); break;
            case TriggerConditionType::PauseIf:     sBuffer.push_back('P'); break;
            case TriggerConditionType::AddSource:   sBuffer.push_back('A'); break;
            case TriggerConditionType::SubSource:   sBuffer.push_back('B'); break;
            case TriggerConditionType::AddHits:     sBuffer.push_back('C'); break;
            case TriggerConditionType::SubHits:     sBuffer.push_back('D'); break;
            case TriggerConditionType::AndNext:     sBuffer.push_back('N'); break;
            case TriggerConditionType::OrNext:      sBuffer.push_back('O'); break;
            case TriggerConditionType::MeasuredIf:  sBuffer.push_back('Q'); break;
            case TriggerConditionType::AddAddress:  sBuffer.push_back('I'); break;
            case TriggerConditionType::Trigger:     sBuffer.push_back('T'); break;
            case TriggerConditionType::ResetNextIf: sBuffer.push_back('Z'); break;
            case TriggerConditionType::Measured:
            {
                const auto* pTriggerViewModel = dynamic_cast<const TriggerViewModel*>(m_pTriggerViewModel);
                if (pTriggerViewModel != nullptr && pTriggerViewModel->IsMeasuredTrackedAsPercent())
                    sBuffer.push_back('G');
                else
                    sBuffer.push_back('M');
                break;
            }
            default:
                assert(!"Unknown condition type");
                break;
        }

        sBuffer.push_back(':');
    }

    SerializeAppendOperand(sBuffer, GetSourceType(), GetSourceSize(), GetSourceValue());

    const auto nOperator = GetOperator();
    if (nOperator != TriggerOperatorType::None)
    {
        switch (nOperator)
        {
            case TriggerOperatorType::Equals:
                sBuffer.push_back('=');
                break;

            case TriggerOperatorType::NotEquals:
                sBuffer.push_back('!');
                sBuffer.push_back('=');
                break;

            case TriggerOperatorType::LessThan:
                sBuffer.push_back('<');
                break;

            case TriggerOperatorType::LessThanOrEqual:
                sBuffer.push_back('<');
                sBuffer.push_back('=');
                break;

            case TriggerOperatorType::GreaterThan:
                sBuffer.push_back('>');
                break;

            case TriggerOperatorType::GreaterThanOrEqual:
                sBuffer.push_back('>');
                sBuffer.push_back('=');
                break;

            case TriggerOperatorType::Multiply:
                sBuffer.push_back('*');
                break;

            case TriggerOperatorType::Divide:
                sBuffer.push_back('/');
                break;

            case TriggerOperatorType::BitwiseAnd:
                sBuffer.push_back('&');
                break;

            default:
                assert(!"Unknown comparison");
                break;
        }

        SerializeAppendOperand(sBuffer, GetTargetType(), GetTargetSize(), GetTargetValue());
    }

    if (GetValue(HasHitsProperty) && GetValue(CanEditHitsProperty))
    {
        const auto nRequiredHits = GetRequiredHits();
        if (nRequiredHits > 0)
            sBuffer.append(ra::StringPrintf(".%zu.", nRequiredHits));
    }
}

void TriggerConditionViewModel::SerializeAppendOperand(std::string& sBuffer, TriggerOperandType nType, MemSize nSize, const std::wstring& sValue) const
{
    switch (nType)
    {
        case TriggerOperandType::Address:
            break;

        case TriggerOperandType::Value:
        {
            unsigned int nValue = 0;
            std::wstring sError;

            if (sValue.length() > 2 && sValue.at(1) == 'x' && ra::ParseHex(sValue, 0xFFFFFFFF, nValue, sError))
                sBuffer.append(std::to_string(nValue));
            else if (ra::ParseUnsignedInt(sValue, 0xFFFFFFFF, nValue, sError))
                sBuffer.append(std::to_string(nValue));
            else
                sBuffer.push_back('0');
            return;
        }

        case TriggerOperandType::Float:
        {
            float fValue = 0.0;
            std::wstring sError;

            if (ra::ParseFloat(sValue, fValue, sError))
            {
                std::string sFloat = std::to_string(fValue);
                if (sFloat.find('.') != std::string::npos)
                {
                    while (sFloat.back() == '0')
                        sFloat.pop_back();
                    if (sFloat.back() == '.')
                        sFloat.push_back('0');
                }

                sBuffer.push_back('f');
                sBuffer.append(sFloat);
            }
            else
            {
                sBuffer.push_back('0');
            }
            return;
        }

        case TriggerOperandType::Delta:
            sBuffer.push_back('d');
            break;

        case TriggerOperandType::Prior:
            sBuffer.push_back('p');
            break;

        case TriggerOperandType::BCD:
            sBuffer.push_back('b');
            break;

        default:
            assert(!"Unknown operand type");
            break;
    }

    sBuffer.push_back('0');
    sBuffer.push_back('x');

    switch (nSize)
    {
        case MemSize::BitCount:              sBuffer.push_back('K'); break;
        case MemSize::Bit_0:                 sBuffer.push_back('M'); break;
        case MemSize::Bit_1:                 sBuffer.push_back('N'); break;
        case MemSize::Bit_2:                 sBuffer.push_back('O'); break;
        case MemSize::Bit_3:                 sBuffer.push_back('P'); break;
        case MemSize::Bit_4:                 sBuffer.push_back('Q'); break;
        case MemSize::Bit_5:                 sBuffer.push_back('R'); break;
        case MemSize::Bit_6:                 sBuffer.push_back('S'); break;
        case MemSize::Bit_7:                 sBuffer.push_back('T'); break;
        case MemSize::Nibble_Lower:          sBuffer.push_back('L'); break;
        case MemSize::Nibble_Upper:          sBuffer.push_back('U'); break;
        case MemSize::EightBit:              sBuffer.push_back('H'); break;
        case MemSize::TwentyFourBit:         sBuffer.push_back('W'); break;
        case MemSize::ThirtyTwoBit:          sBuffer.push_back('X'); break;
        case MemSize::SixteenBit:            sBuffer.push_back(' '); break;
        case MemSize::ThirtyTwoBitBigEndian: sBuffer.push_back('G'); break;
        case MemSize::SixteenBitBigEndian:   sBuffer.push_back('I'); break;
        case MemSize::TwentyFourBitBigEndian:sBuffer.push_back('J'); break;

        case MemSize::Float:
            sBuffer.pop_back();
            sBuffer.pop_back();
            sBuffer.push_back('f');
            sBuffer.push_back('F');
            break;

        case MemSize::MBF32:
            sBuffer.pop_back();
            sBuffer.pop_back();
            sBuffer.push_back('f');
            sBuffer.push_back('M');
            break;

        default:
            assert(!"Unknown memory size");
            break;
    }

    {
        unsigned int nValue = 0;
        std::wstring sError;

        if (ra::ParseHex(sValue, 0xFFFFFFFF, nValue, sError))
            sBuffer.append(ra::ByteAddressToString(nValue), 2);
        else
            sBuffer.push_back('0');
    }
}

std::wstring FormatValue(rc_typed_value_t& pValue, TriggerOperandType nType)
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
            break;
    }
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

    SetValue(sValueProperty, FormatValue(pValue, nNewType));
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
        {
            const auto nSize = ra::data::models::TriggerValidation::MapRcheevosMemSize(operand.size);
            SetValue(pSizeProperty, ra::etoi(nSize));
            pValue.type = RC_VALUE_TYPE_UNSIGNED;
            pValue.value.u32 = operand.value.memref->address;
            break;
        }

        default:
            Expects(!"Unknown operand type");
            break;
    }

    SetValue(pValueProperty, FormatValue(pValue, nType));
}

void TriggerConditionViewModel::InitializeFrom(const rc_condition_t& pCondition)
{
    SetType(static_cast<TriggerConditionType>(pCondition.type));

    SetOperand(SourceTypeProperty, SourceSizeProperty, SourceValueProperty, pCondition.operand1);
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
            SetSourceSize(nNewType == TriggerOperandType::Value ? MemSize::ThirtyTwoBit : MemSize::Float);
        }
        else if (!IsAddressType(nOldType))
        {
            SetSourceSize(GetTargetSize());
            SetValue(HasSourceSizeProperty, true);
        }
    }
    else if (args.Property == TargetTypeProperty)
    {
        const auto nOldType = ra::itoe<TriggerOperandType>(args.tOldValue);
        const auto nNewType = ra::itoe<TriggerOperandType>(args.tNewValue);
        ChangeOperandType(TargetValueProperty, nOldType, nNewType);

        if (!IsAddressType(nNewType))
        {
            SetValue(HasTargetSizeProperty, false);
            SetTargetSize(nNewType == TriggerOperandType::Value ? MemSize::ThirtyTwoBit : MemSize::Float);
        }
        else if (!IsAddressType(ra::itoe<TriggerOperandType>(args.tOldValue)))
        {
            SetTargetSize(GetSourceSize());
            SetValue(HasTargetSizeProperty, GetValue(HasTargetProperty));
        }
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
        SetValue(HasHitsProperty, GetOperator() != TriggerOperatorType::None);
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
        SetValue(HasTargetSizeProperty, args.tNewValue && IsAddressType(GetTargetType()));

    ViewModelBase::OnValueChanged(args);
}

void TriggerConditionViewModel::SetSourceValue(unsigned int nValue)
{
    rc_typed_value_t pValue;
    pValue.type = RC_VALUE_TYPE_UNSIGNED;
    pValue.value.u32 = nValue;
    SetValue(SourceValueProperty, FormatValue(pValue, GetSourceType()));
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
    rc_typed_value_t pValue;
    pValue.type = RC_VALUE_TYPE_UNSIGNED;
    pValue.value.u32 = nValue;
    SetValue(TargetValueProperty, FormatValue(pValue, GetTargetType()));
}

void TriggerConditionViewModel::SetTargetValue(float fValue)
{
    rc_typed_value_t pValue;
    pValue.type = RC_VALUE_TYPE_FLOAT;
    pValue.value.f32 = fValue;
    SetValue(TargetValueProperty, FormatValue(pValue, GetTargetType()));
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

        if (IsIndirect())
            return GetAddressTooltip(GetIndirectAddress(GetSourceAddress()), true);

        return GetAddressTooltip(GetSourceAddress(), false);
    }

    if (nProperty == TargetValueProperty)
    {
        const auto nType = GetTargetType();
        if (nType == TriggerOperandType::Value)
            return GetValueTooltip(GetTargetAddress());

        if (nType == TriggerOperandType::Float)
            return L"";

        if (IsIndirect())
            return GetAddressTooltip(GetIndirectAddress(GetTargetAddress()), true);

        return GetAddressTooltip(GetTargetAddress(), false);
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

static bool IsIndirectMemref(const rc_operand_t& operand) noexcept
{
    return rc_operand_is_memref(&operand) && operand.value.memref->value.is_indirect;
}

unsigned int TriggerConditionViewModel::GetIndirectAddress(unsigned int nAddress) const
{
    const auto* pTriggerViewModel = dynamic_cast<const TriggerViewModel*>(m_pTriggerViewModel);
    if (pTriggerViewModel == nullptr)
        return nAddress;

    const auto* pGroup = pTriggerViewModel->Groups().GetItemAt(pTriggerViewModel->GetSelectedGroupIndex());
    if (pGroup == nullptr)
        return nAddress;

    const auto* pTrigger = pTriggerViewModel->GetTriggerFromString();
    if (pTrigger != nullptr)
    {
        // if the trigger is managed by the viewmodel (not the runtime) then we need to update the memrefs
        rc_update_memref_values(pTrigger->memrefs, rc_peek_callback, nullptr);
    }

    bool bIsIndirect = false;
    rc_eval_state_t oEvalState;
    memset(&oEvalState, 0, sizeof(oEvalState));
    rc_typed_value_t value = {};
    oEvalState.peek = rc_peek_callback;

    Expects(pGroup->m_pConditionSet != nullptr);

    gsl::index nConditionIndex = 0;
    rc_condition_t* pCondition = pGroup->m_pConditionSet->conditions;
    for (; pCondition != nullptr; pCondition = pCondition->next)
    {
        auto* vmCondition = pTriggerViewModel->Conditions().GetItemAt(nConditionIndex++);
        if (!vmCondition)
            break;

        if (vmCondition == this)
            return nAddress + oEvalState.add_address;

        if (pCondition->type == RC_CONDITION_ADD_ADDRESS)
        {
            if (bIsIndirect && pTrigger == nullptr)
            {
                // if this is part of a chain, we have to create a copy of the condition so we can point
                // at copies of the indirect memrefs so the real delta values aren't modified.
                rc_condition_t oCondition;
                memcpy(&oCondition, pCondition, sizeof(oCondition));
                rc_memref_t oSource{}, oTarget{};

                if (IsIndirectMemref(pCondition->operand1))
                {
                    memcpy(&oSource, pCondition->operand1.value.memref, sizeof(oSource));
                    oCondition.operand1.value.memref = &oSource;
                }

                if (IsIndirectMemref(pCondition->operand2))
                {
                    memcpy(&oTarget, pCondition->operand2.value.memref, sizeof(oTarget));
                    oCondition.operand2.value.memref = &oTarget;
                }

                rc_evaluate_condition_value(&value, &oCondition, &oEvalState);
                rc_typed_value_convert(&value, RC_VALUE_TYPE_UNSIGNED);
                oEvalState.add_address = value.value.u32;
            }
            else
            {
                rc_evaluate_condition_value(&value, pCondition, &oEvalState);
                rc_typed_value_convert(&value, RC_VALUE_TYPE_UNSIGNED);
                oEvalState.add_address = value.value.u32;
                bIsIndirect = true;
            }
        }
        else
        {
            bIsIndirect = false;
            oEvalState.add_address = 0;
        }
    }

    return nAddress;
}

std::wstring TriggerConditionViewModel::GetAddressTooltip(unsigned int nAddress, bool bIsIndirect)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    std::wstring sAddress;

    const auto nStartAddress = pGameContext.FindCodeNoteStart(nAddress);
    if (nStartAddress != nAddress && nStartAddress != 0xFFFFFFFF)
    {
        sAddress = ra::StringPrintf(L"%s [%s+%d]", ra::ByteAddressToString(nAddress),
            ra::ByteAddressToString(nStartAddress), nAddress - nStartAddress);
    }
    else
    {
        sAddress = ra::Widen(ra::ByteAddressToString(nAddress));
    }

    if (bIsIndirect)
        sAddress.append(L" (indirect)");

    const auto* pNote = (nStartAddress != 0xFFFFFFFF) ? pGameContext.FindCodeNote(nStartAddress) : nullptr;
    if (!pNote)
        return ra::StringPrintf(L"%s\r\n[No code note]", sAddress);

    // limit the tooltip to the first 20 lines of the code note
    size_t nLines = 0;
    size_t nIndex = 0;
    do
    {
        nIndex = pNote->find('\n', nIndex);
        if (nIndex == std::string::npos)
            break;

        ++nIndex;
        ++nLines;
    } while (nLines < 20);

    if (nIndex != std::string::npos && pNote->find('\n', nIndex) != std::string::npos)
    {
        std::wstring sSubString(*pNote, 0, nIndex);
        return ra::StringPrintf(L"%s\r\n%s...", sAddress, sSubString);
    }

    return ra::StringPrintf(L"%s\r\n%s", sAddress, *pNote);
}

bool TriggerConditionViewModel::IsModifying(TriggerConditionType nType) noexcept
{
    switch (nType)
    {
        case TriggerConditionType::AddAddress:
        case TriggerConditionType::AddSource:
        case TriggerConditionType::SubSource:
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
            return false;

        default:
            return true;
    }
}

bool TriggerConditionViewModel::IsComparisonVisible(const ViewModelBase& vmItem, int nValue)
{
    const auto* vmCondition = dynamic_cast<const TriggerConditionViewModel*>(&vmItem);
    if (vmCondition == nullptr)
        return false;

    const auto nComparison = ra::itoe<TriggerOperatorType>(nValue);
    switch (nComparison)
    {
        case TriggerOperatorType::None:
            if (vmCondition->IsModifying())
                return true;

            // "None" can only be selected for the Measured flag when editing a value
            if (vmCondition->GetType() == TriggerConditionType::Measured && vmCondition->IsForValue())
                return true;

            return false;

        case TriggerOperatorType::Multiply:
        case TriggerOperatorType::Divide:
        case TriggerOperatorType::BitwiseAnd:
            return vmCondition->IsModifying();

        default:
            return !vmCondition->IsModifying();
    }
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
        // true this frame, but target hitcount not met
        SetRowColor(pTheme.ColorTriggerBecomingTrue());
    }
    else
    {
        // true this frame, and target hitcount met
        switch (pCondition->type)
        {
            case RC_CONDITION_RESET_IF:
                SetRowColor(pTheme.ColorTriggerResetTrue());
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

} // namespace viewmodels
} // namespace ui
} // namespace ra
