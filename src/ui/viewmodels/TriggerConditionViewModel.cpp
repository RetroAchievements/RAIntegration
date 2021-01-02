#include "TriggerConditionViewModel.hh"

#include "RA_Defs.h"
#include "RA_StringUtils.h"

#include "data\context\GameContext.hh"

#include "services\IConfiguration.hh"
#include "services\ServiceLocator.hh"

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

std::wstring TriggerConditionViewModel::GetTooltip(const IntModelProperty& nProperty) const
{
    if (nProperty == SourceValueProperty)
        return GetValueTooltip(GetSourceType(), GetSourceValue());

    if (nProperty == TargetValueProperty)
        return GetValueTooltip(GetTargetType(), GetTargetValue());

    return L"";
}

std::wstring TriggerConditionViewModel::GetValueTooltip(TriggerOperandType nType, unsigned int nValue) const
{
    if (nType != TriggerOperandType::Value)
        return GetAddressTooltip(nValue);

    const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
    if (pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal))
        return L"";

    return std::to_wstring(nValue);
}

std::wstring TriggerConditionViewModel::GetAddressTooltip(unsigned nAddress) const
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto* pNote = pGameContext.FindCodeNote(nAddress);
    if (!pNote)
        return ra::StringPrintf(L"%s\r\n[No code note]", ra::ByteAddressToString(nAddress));

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
        return ra::StringPrintf(L"%s\r\n%s...", ra::ByteAddressToString(nAddress), sSubString);
    }

    return ra::StringPrintf(L"%s\r\n%s", ra::ByteAddressToString(nAddress), *pNote);
}

} // namespace viewmodels
} // namespace ui
} // namespace ra
