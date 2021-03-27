#include "TriggerConditionViewModel.hh"

#include "RA_Defs.h"
#include "RA_StringUtils.h"

#include "data\context\GameContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

#include "ui\viewmodels\TriggerViewModel.hh"

#include <rcheevos\src\rcheevos\rc_internal.h>

namespace ra {
namespace ui {
namespace viewmodels {

const IntModelProperty TriggerConditionViewModel::IndexProperty("TriggerConditionViewModel", "Index", 1);
const IntModelProperty TriggerConditionViewModel::TypeProperty("TriggerConditionViewModel", "Type", ra::etoi(TriggerConditionType::Standard));
const IntModelProperty TriggerConditionViewModel::SourceTypeProperty("TriggerConditionViewModel", "SourceType", ra::etoi(TriggerOperandType::Address));
const IntModelProperty TriggerConditionViewModel::SourceSizeProperty("TriggerConditionViewModel", "SourceSize", ra::etoi(MemSize::EightBit));
const IntModelProperty TriggerConditionViewModel::SourceValueProperty("TriggerConditionViewModel", "SourceValue", 0);
const IntModelProperty TriggerConditionViewModel::OperatorProperty("TriggerConditionViewModel", "Operator", ra::etoi(TriggerOperatorType::Equals));
const IntModelProperty TriggerConditionViewModel::TargetTypeProperty("TriggerConditionViewModel", "TargetType", ra::etoi(TriggerOperandType::Value));
const IntModelProperty TriggerConditionViewModel::TargetSizeProperty("TriggerConditionViewModel", "TargetSize", ra::etoi(MemSize::ThirtyTwoBit));
const IntModelProperty TriggerConditionViewModel::TargetValueProperty("TriggerConditionViewModel", "TargetValue", 0);
const IntModelProperty TriggerConditionViewModel::CurrentHitsProperty("TriggerConditionViewModel", "CurrentHits", 0);
const IntModelProperty TriggerConditionViewModel::RequiredHitsProperty("TriggerConditionViewModel", "RequiredHits", 0);
const BoolModelProperty TriggerConditionViewModel::IsSelectedProperty("TriggerConditionViewModel", "IsSelected", false);
const BoolModelProperty TriggerConditionViewModel::IsIndirectProperty("TriggerConditionViewModel", "IsIndirect", false);
const BoolModelProperty TriggerConditionViewModel::HasSourceSizeProperty("TriggerConditionViewModel", "HasSourceSize", true);
const BoolModelProperty TriggerConditionViewModel::HasTargetProperty("TriggerConditionViewModel", "HasTarget", true);
const BoolModelProperty TriggerConditionViewModel::HasTargetSizeProperty("TriggerConditionViewModel", "HasTargetSize", false);
const BoolModelProperty TriggerConditionViewModel::HasHitsProperty("TriggerConditionViewModel", "HasHits", true);

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
            case TriggerConditionType::Measured:    sBuffer.push_back('M'); break;
            case TriggerConditionType::MeasuredIf:  sBuffer.push_back('Q'); break;
            case TriggerConditionType::AddAddress:  sBuffer.push_back('I'); break;
            case TriggerConditionType::Trigger:     sBuffer.push_back('T'); break;
            case TriggerConditionType::ResetNextIf: sBuffer.push_back('Z'); break;
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

    const auto nRequiredHits = GetRequiredHits();
    if (nRequiredHits > 0)
        sBuffer.append(ra::StringPrintf(".%zu.", nRequiredHits));
}

void TriggerConditionViewModel::SerializeAppendOperand(std::string& sBuffer, TriggerOperandType nType, MemSize nSize, unsigned int nValue) const
{
    switch (nType)
    {
        case TriggerOperandType::Address:
            break;

        case TriggerOperandType::Value:
            sBuffer.append(std::to_string(nValue));
            return;

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
        case MemSize::BitCount:     sBuffer.push_back('K'); break;
        case MemSize::Bit_0:        sBuffer.push_back('M'); break;
        case MemSize::Bit_1:        sBuffer.push_back('N'); break;
        case MemSize::Bit_2:        sBuffer.push_back('O'); break;
        case MemSize::Bit_3:        sBuffer.push_back('P'); break;
        case MemSize::Bit_4:        sBuffer.push_back('Q'); break;
        case MemSize::Bit_5:        sBuffer.push_back('R'); break;
        case MemSize::Bit_6:        sBuffer.push_back('S'); break;
        case MemSize::Bit_7:        sBuffer.push_back('T'); break;
        case MemSize::Nibble_Lower: sBuffer.push_back('L'); break;
        case MemSize::Nibble_Upper: sBuffer.push_back('U'); break;
        case MemSize::EightBit:     sBuffer.push_back('H'); break;
        case MemSize::TwentyFourBit:sBuffer.push_back('W'); break;
        case MemSize::ThirtyTwoBit: sBuffer.push_back('X'); break;
        case MemSize::SixteenBit:   sBuffer.push_back(' '); break;
        default:
            assert(!"Unknown memory size");
            break;
    }

    sBuffer.append(ra::ByteAddressToString(nValue), 2);
}

static constexpr MemSize MapMemSize(char nSize) noexcept
{
    switch (nSize)
    {
        case RC_MEMSIZE_BIT_0: return MemSize::Bit_0;
        case RC_MEMSIZE_BIT_1: return MemSize::Bit_1;
        case RC_MEMSIZE_BIT_2: return MemSize::Bit_2;
        case RC_MEMSIZE_BIT_3: return MemSize::Bit_3;
        case RC_MEMSIZE_BIT_4: return MemSize::Bit_4;
        case RC_MEMSIZE_BIT_5: return MemSize::Bit_5;
        case RC_MEMSIZE_BIT_6: return MemSize::Bit_6;
        case RC_MEMSIZE_BIT_7: return MemSize::Bit_7;
        case RC_MEMSIZE_LOW: return MemSize::Nibble_Lower;
        case RC_MEMSIZE_HIGH: return MemSize::Nibble_Upper;
        case RC_MEMSIZE_8_BITS: return MemSize::EightBit;
        case RC_MEMSIZE_16_BITS: return MemSize::SixteenBit;
        case RC_MEMSIZE_24_BITS: return MemSize::TwentyFourBit;
        case RC_MEMSIZE_32_BITS: return MemSize::ThirtyTwoBit;
        default:
            assert(!"Unsupported operand size");
            return MemSize::EightBit;
    }
}

void TriggerConditionViewModel::SetOperand(const IntModelProperty& pTypeProperty,
    const IntModelProperty& pSizeProperty, const IntModelProperty& pValueProperty, const rc_operand_t& operand)
{
    const auto nType = static_cast<TriggerOperandType>(operand.type);
    SetValue(pTypeProperty, ra::etoi(nType));

    switch (nType)
    {
        case TriggerOperandType::Value:
            SetValue(pSizeProperty, ra::etoi(MemSize::ThirtyTwoBit));
            SetValue(pValueProperty, operand.value.num);
            break;

        case TriggerOperandType::Address:
        case TriggerOperandType::Delta:
        case TriggerOperandType::Prior:
        case TriggerOperandType::BCD:
        {
            const auto nSize = MapMemSize(operand.size);
            SetValue(pSizeProperty, ra::etoi(nSize));
            SetValue(pValueProperty, operand.value.memref->address);
            break;
        }

        default:
            assert(!"Unknown operand type");
            break;
    }
}

void TriggerConditionViewModel::InitializeFrom(const rc_condition_t& pCondition)
{
    SetType(static_cast<TriggerConditionType>(pCondition.type));

    SetOperand(SourceTypeProperty, SourceSizeProperty, SourceValueProperty, pCondition.operand1);
    SetOperand(TargetTypeProperty, TargetSizeProperty, TargetValueProperty, pCondition.operand2);

    SetOperator(static_cast<TriggerOperatorType>(pCondition.oper));

    SetCurrentHits(pCondition.current_hits);
    SetRequiredHits(pCondition.required_hits);
}

void TriggerConditionViewModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    if (args.Property == SourceTypeProperty)
    {
        SetValue(HasSourceSizeProperty, ra::itoe<TriggerOperandType>(args.tNewValue) != TriggerOperandType::Value);
    }
    else if (args.Property == TargetTypeProperty)
    {
        SetValue(HasTargetSizeProperty, ra::itoe<TriggerOperandType>(args.tNewValue) != TriggerOperandType::Value && GetValue(HasTargetProperty));
    }
    else if (args.Property == OperatorProperty)
    {
        SetValue(HasTargetProperty, ra::itoe<TriggerOperatorType>(args.tNewValue) != TriggerOperatorType::None);
    }
    else if (args.Property == TypeProperty)
    {
        const auto nOldType = ra::itoe<TriggerConditionType>(args.tOldValue);
        const auto nNewType = ra::itoe<TriggerConditionType>(args.tNewValue);
        const auto bIsModifying = IsModifying(nNewType);
        if (bIsModifying != IsModifying(nOldType))
        {
            if (bIsModifying)
            {
                SetValue(HasHitsProperty, false);
                SetOperator(TriggerOperatorType::None);
            }
            else
            {
                SetValue(HasHitsProperty, true);
                SetOperator(TriggerOperatorType::Equals);
            }
        }
    }

    ViewModelBase::OnValueChanged(args);
}

void TriggerConditionViewModel::OnValueChanged(const BoolModelProperty::ChangeArgs& args)
{
    if (args.Property == HasTargetProperty)
        SetValue(HasTargetSizeProperty, args.tNewValue && GetTargetType() != TriggerOperandType::Value);

    ViewModelBase::OnValueChanged(args);
}

std::wstring TriggerConditionViewModel::GetTooltip(const IntModelProperty& nProperty) const
{
    if (nProperty == SourceValueProperty)
    {
        const auto nType = GetSourceType();
        if (nType == TriggerOperandType::Value)
            return GetValueTooltip(GetSourceValue());

        if (IsIndirect())
            return GetAddressTooltip(GetIndirectAddress(GetSourceValue()), true);

        return GetAddressTooltip(GetSourceValue(), false);
    }

    if (nProperty == TargetValueProperty)
    {
        const auto nType = GetTargetType();
        if (nType == TriggerOperandType::Value)
            return GetValueTooltip(GetTargetValue());

        if (IsIndirect())
            return GetAddressTooltip(GetIndirectAddress(GetTargetValue()), true);

        return GetAddressTooltip(GetTargetValue(), false);
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

static bool IsIndirectMemref(rc_operand_t& operand) noexcept
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
    oEvalState.peek = rc_peek_callback;

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

                oEvalState.add_address = rc_evaluate_condition_value(&oCondition, &oEvalState);
            }
            else
            {
                oEvalState.add_address = rc_evaluate_condition_value(pCondition, &oEvalState);
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
        sAddress = ra::StringPrintf(L"%s [%d/%d]", ra::ByteAddressToString(nStartAddress),
            nAddress - nStartAddress + 1, pGameContext.FindCodeNoteSize(nStartAddress));
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

bool TriggerConditionViewModel::IsComparisonVisible(const ViewModelBase& vmItem, int nValue)
{
    const auto* vmCondition = dynamic_cast<const TriggerConditionViewModel*>(&vmItem);
    if (vmCondition == nullptr)
        return false;

    const auto nComparison = ra::itoe<TriggerOperatorType>(nValue);
    switch (nComparison)
    {
        case TriggerOperatorType::None:
        case TriggerOperatorType::Multiply:
        case TriggerOperatorType::Divide:
        case TriggerOperatorType::BitwiseAnd:
            return vmCondition->IsModifying();

        default:
            return !vmCondition->IsModifying();
    }
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
