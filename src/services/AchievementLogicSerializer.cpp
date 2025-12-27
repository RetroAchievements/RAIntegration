#include "AchievementLogicSerializer.hh"

#include "RA_Defs.h"
#include "util\Strings.hh"

#include "data\context\ConsoleContext.hh"

#include "services\ServiceLocator.hh"

namespace ra {
namespace services {

void AchievementLogicSerializer::AppendConditionType(std::string& sBuffer, TriggerConditionType nType)
{
    switch (nType)
    {
        case TriggerConditionType::Standard:
            return;
        case TriggerConditionType::PauseIf:
            sBuffer.push_back('P');
            break;
        case TriggerConditionType::ResetIf:
            sBuffer.push_back('R');
            break;
        case TriggerConditionType::AddSource:
            sBuffer.push_back('A');
            break;
        case TriggerConditionType::SubSource:
            sBuffer.push_back('B');
            break;
        case TriggerConditionType::AddHits:
            sBuffer.push_back('C');
            break;
        case TriggerConditionType::SubHits:
            sBuffer.push_back('D');
            break;
        case TriggerConditionType::Remember:
            sBuffer.push_back('K');
            break;
        case TriggerConditionType::AndNext:
            sBuffer.push_back('N');
            break;
        case TriggerConditionType::OrNext:
            sBuffer.push_back('O');
            break;
        case TriggerConditionType::Measured:
            sBuffer.push_back('M');
            break;
        case TriggerConditionType::MeasuredAsPercent:
            sBuffer.push_back('G');
            break;
        case TriggerConditionType::MeasuredIf:
            sBuffer.push_back('Q');
            break;
        case TriggerConditionType::AddAddress:
            sBuffer.push_back('I');
            break;
        case TriggerConditionType::Trigger:
            sBuffer.push_back('T');
            break;
        case TriggerConditionType::ResetNextIf:
            sBuffer.push_back('Z');
            break;
        default:
            assert(!"Unknown condition type");
            break;
    }

    sBuffer.push_back(':');
}

void AchievementLogicSerializer::AppendOperand(std::string& sBuffer, TriggerOperandType nType, ra::data::Memory::Size nSize, uint32_t nValue)
{
    switch (nType)
    {
        case TriggerOperandType::Address:
            break;

        case TriggerOperandType::Value:
            sBuffer.append(std::to_string(nValue));
            return;

        case TriggerOperandType::Float:
            sBuffer.push_back('f');
            sBuffer.append(std::to_string(nValue));
            sBuffer.push_back('.');
            sBuffer.push_back('0');
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

        case TriggerOperandType::Inverted:
            sBuffer.push_back('~');
            break;

        case TriggerOperandType::Recall:
            sBuffer.append("{recall}");
            return;

        default:
            assert(!"Unknown operand type");
            break;
    }

    sBuffer.push_back('0');
    sBuffer.push_back('x');

    switch (nSize)
    {
        case ra::data::Memory::Size::BitCount:              sBuffer.push_back('K'); break;
        case ra::data::Memory::Size::Bit0:                 sBuffer.push_back('M'); break;
        case ra::data::Memory::Size::Bit1:                 sBuffer.push_back('N'); break;
        case ra::data::Memory::Size::Bit2:                 sBuffer.push_back('O'); break;
        case ra::data::Memory::Size::Bit3:                 sBuffer.push_back('P'); break;
        case ra::data::Memory::Size::Bit4:                 sBuffer.push_back('Q'); break;
        case ra::data::Memory::Size::Bit5:                 sBuffer.push_back('R'); break;
        case ra::data::Memory::Size::Bit6:                 sBuffer.push_back('S'); break;
        case ra::data::Memory::Size::Bit7:                 sBuffer.push_back('T'); break;
        case ra::data::Memory::Size::NibbleLower:          sBuffer.push_back('L'); break;
        case ra::data::Memory::Size::NibbleUpper:          sBuffer.push_back('U'); break;
        case ra::data::Memory::Size::EightBit:              sBuffer.push_back('H'); break;
        case ra::data::Memory::Size::TwentyFourBit:         sBuffer.push_back('W'); break;
        case ra::data::Memory::Size::ThirtyTwoBit:          sBuffer.push_back('X'); break;
        case ra::data::Memory::Size::SixteenBit:            sBuffer.push_back(' '); break;
        case ra::data::Memory::Size::ThirtyTwoBitBigEndian: sBuffer.push_back('G'); break;
        case ra::data::Memory::Size::SixteenBitBigEndian:   sBuffer.push_back('I'); break;
        case ra::data::Memory::Size::TwentyFourBitBigEndian:sBuffer.push_back('J'); break;

        case ra::data::Memory::Size::Float:
            sBuffer.pop_back();
            sBuffer.pop_back();
            sBuffer.push_back('f');
            sBuffer.push_back('F');
            break;

        case ra::data::Memory::Size::FloatBigEndian:
            sBuffer.pop_back();
            sBuffer.pop_back();
            sBuffer.push_back('f');
            sBuffer.push_back('B');
            break;

        case ra::data::Memory::Size::Double32:
            sBuffer.pop_back();
            sBuffer.pop_back();
            sBuffer.push_back('f');
            sBuffer.push_back('H');
            break;

        case ra::data::Memory::Size::Double32BigEndian:
            sBuffer.pop_back();
            sBuffer.pop_back();
            sBuffer.push_back('f');
            sBuffer.push_back('I');
            break;

        case ra::data::Memory::Size::MBF32:
            sBuffer.pop_back();
            sBuffer.pop_back();
            sBuffer.push_back('f');
            sBuffer.push_back('M');
            break;

        case ra::data::Memory::Size::MBF32LE:
            sBuffer.pop_back();
            sBuffer.pop_back();
            sBuffer.push_back('f');
            sBuffer.push_back('L');
            break;

        case ra::data::Memory::Size::Array:
        case ra::data::Memory::Size::Text:
            /* not a real size, use 32-bit BE as best approximation */
            sBuffer.push_back('G');
            break;

        default:
            assert(!"Unknown memory size");
            break;
    }

    sBuffer.append(ra::ByteAddressToString(nValue), 2);
}

void AchievementLogicSerializer::AppendOperand(std::string& sBuffer, TriggerOperandType nType, ra::data::Memory::Size, float fValue)
{
    switch (nType)
    {
        case TriggerOperandType::Value:
            sBuffer.append(std::to_string(ra::to_unsigned(gsl::narrow_cast<int>(fValue))));
            break;

        case TriggerOperandType::Float: {
            std::string sFloat = std::to_string(fValue);
            if (sFloat.find('.') != std::string::npos)
            {
                while (sFloat.back() == '0') // remove insignificant zeros
                    sFloat.pop_back();
                if (sFloat.back() == '.') // if everything after the decimal was removed, add back a zero
                    sFloat.push_back('0');
            }

            sBuffer.push_back('f');
            sBuffer.append(sFloat);
            break;
        }

        default:
            assert(!"Operand does not support float value");
            break;
    }
}

void AchievementLogicSerializer::AppendOperator(std::string& sBuffer, TriggerOperatorType nType)
{
    switch (nType)
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

        case TriggerOperatorType::BitwiseXor:
            sBuffer.push_back('^');
            break;

        case TriggerOperatorType::Modulus:
            sBuffer.push_back('%');
            break;

        case TriggerOperatorType::Add:
            sBuffer.push_back('+');
            break;

        case TriggerOperatorType::Subtract:
            sBuffer.push_back('-');
            break;

        default:
            assert(!"Unknown comparison");
            break;
    }
}

void AchievementLogicSerializer::AppendHitTarget(std::string& sBuffer, uint32_t nTarget)
{
    if (nTarget > 0)
    {
        sBuffer.push_back('.');
        sBuffer.append(std::to_string(nTarget));
        sBuffer.push_back('.');
    }
}

std::string AchievementLogicSerializer::BuildMemRefChain(const ra::data::models::CodeNoteModel& pRootNote,
    const ra::data::models::CodeNoteModel& pLeafNote)
{
    std::vector<const ra::data::models::CodeNoteModel*> vChain;
    if (!pLeafNote.GetPointerChain(vChain, pRootNote))
        return std::string();

    auto nSize = ra::data::Memory::Size::ThirtyTwoBit;
    uint32_t nMask = 0xFFFFFFFF;
    uint32_t nOffset = 0;

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::data::context::ConsoleContext>();
    if (!pConsoleContext.GetRealAddressConversion(&nSize, &nMask, &nOffset))
    {
        nSize = pRootNote.GetMemSize();
        nMask = 0xFFFFFFFF;
        nOffset = pConsoleContext.RealAddressFromByteAddress(0);
        if (nOffset == 0xFFFFFFFF)
            nOffset = 0;
    }

    std::string sBuffer;
    size_t nBitmaskOffset = std::string::npos;
    ra::data::ByteAddress nPointerBase = 0, nAddress = 0;
    for (size_t i = 0; i < vChain.size() - 1; ++i)
    {
        const auto* pNote = vChain.at(i);
        Expects(pNote != nullptr);

        nAddress = pNote->GetAddress();
        if (nBitmaskOffset != std::string::npos)
        {
            if (nAddress > nMask && nAddress < (0xFFFFFFFF - nMask))
            {
                sBuffer.erase(nBitmaskOffset, sBuffer.length() - nBitmaskOffset);
                AppendConditionSeparator(sBuffer);
            }

            nBitmaskOffset = std::string::npos;
        }

        AppendConditionType(sBuffer, ra::services::TriggerConditionType::AddAddress);
        AppendOperand(sBuffer, ra::services::TriggerOperandType::Address, nSize, nAddress);
        nPointerBase = pNote->GetPointerAddress();

        if (nOffset != 0)
        {
            AppendOperator(sBuffer, ra::services::TriggerOperatorType::Subtract);
            AppendOperand(sBuffer, ra::services::TriggerOperandType::Value, ra::data::Memory::Size::ThirtyTwoBit, nOffset);
        }
        else if (nMask != 0xFFFFFFFF)
        {
            const auto nBitsMask = ra::to_unsigned((1 << ra::data::Memory::SizeBits(nSize)) - 1);
            if (nMask != nBitsMask)
            {
                nBitmaskOffset = sBuffer.length();
                AppendOperator(sBuffer, ra::services::TriggerOperatorType::BitwiseAnd);
                AppendOperand(sBuffer, ra::services::TriggerOperandType::Value, ra::data::Memory::Size::ThirtyTwoBit, nMask);
            }
        }

        AppendConditionSeparator(sBuffer);
    }

    nAddress = pLeafNote.GetAddress();
    if (nAddress > nMask && nBitmaskOffset != std::string::npos)
    {
        sBuffer.erase(nBitmaskOffset, sBuffer.length() - nBitmaskOffset);
        AppendConditionSeparator(sBuffer);
    }

    AppendConditionType(sBuffer, ra::services::TriggerConditionType::Measured);

    nSize = pLeafNote.GetMemSize();
    if (nSize == ra::data::Memory::Size::Unknown)
        nSize = ra::data::Memory::Size::EightBit;

    AppendOperand(sBuffer, ra::services::TriggerOperandType::Address, nSize, nAddress);

    return sBuffer;
}

} // namespace services
} // namespace ra
