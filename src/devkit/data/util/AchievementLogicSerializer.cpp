#include "AchievementLogicSerializer.hh"

#include "context/IConsoleContext.hh"

#include "services/ServiceLocator.hh"

#include "util/Strings.hh"

namespace ra {
namespace data {
namespace util {

void AchievementLogicSerializer::AppendConditionType(std::string& sBuffer, Requirement::Type nType)
{
    switch (nType)
    {
        case Requirement::Type::Standard:
            return;
        case Requirement::Type::PauseIf:
            sBuffer.push_back('P');
            break;
        case Requirement::Type::ResetIf:
            sBuffer.push_back('R');
            break;
        case Requirement::Type::AddSource:
            sBuffer.push_back('A');
            break;
        case Requirement::Type::SubSource:
            sBuffer.push_back('B');
            break;
        case Requirement::Type::AddHits:
            sBuffer.push_back('C');
            break;
        case Requirement::Type::SubHits:
            sBuffer.push_back('D');
            break;
        case Requirement::Type::Remember:
            sBuffer.push_back('K');
            break;
        case Requirement::Type::AndNext:
            sBuffer.push_back('N');
            break;
        case Requirement::Type::OrNext:
            sBuffer.push_back('O');
            break;
        case Requirement::Type::Measured:
            sBuffer.push_back('M');
            break;
        case Requirement::Type::MeasuredAsPercent:
            sBuffer.push_back('G');
            break;
        case Requirement::Type::MeasuredIf:
            sBuffer.push_back('Q');
            break;
        case Requirement::Type::AddAddress:
            sBuffer.push_back('I');
            break;
        case Requirement::Type::Trigger:
            sBuffer.push_back('T');
            break;
        case Requirement::Type::ResetNextIf:
            sBuffer.push_back('Z');
            break;
        default:
            assert(!"Unknown condition type");
            break;
    }

    sBuffer.push_back(':');
}

void AchievementLogicSerializer::AppendOperand(std::string& sBuffer, Requirement::OperandType nType, ra::data::Memory::Size nSize, uint32_t nValue)
{
    switch (nType)
    {
        case Requirement::OperandType::Address:
            break;

        case Requirement::OperandType::Value:
            sBuffer.append(std::to_string(nValue));
            return;

        case Requirement::OperandType::Float:
            sBuffer.push_back('f');
            sBuffer.append(std::to_string(nValue));
            sBuffer.push_back('.');
            sBuffer.push_back('0');
            return;

        case Requirement::OperandType::Delta:
            sBuffer.push_back('d');
            break;

        case Requirement::OperandType::Prior:
            sBuffer.push_back('p');
            break;

        case Requirement::OperandType::BCD:
            sBuffer.push_back('b');
            break;

        case Requirement::OperandType::Inverted:
            sBuffer.push_back('~');
            break;

        case Requirement::OperandType::Recall:
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
        case ra::data::Memory::Size::Bit0:                  sBuffer.push_back('M'); break;
        case ra::data::Memory::Size::Bit1:                  sBuffer.push_back('N'); break;
        case ra::data::Memory::Size::Bit2:                  sBuffer.push_back('O'); break;
        case ra::data::Memory::Size::Bit3:                  sBuffer.push_back('P'); break;
        case ra::data::Memory::Size::Bit4:                  sBuffer.push_back('Q'); break;
        case ra::data::Memory::Size::Bit5:                  sBuffer.push_back('R'); break;
        case ra::data::Memory::Size::Bit6:                  sBuffer.push_back('S'); break;
        case ra::data::Memory::Size::Bit7:                  sBuffer.push_back('T'); break;
        case ra::data::Memory::Size::NibbleLower:           sBuffer.push_back('L'); break;
        case ra::data::Memory::Size::NibbleUpper:           sBuffer.push_back('U'); break;
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

    const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
    sBuffer.append(ra::util::String::Narrow(pMemoryContext.FormatAddress(nValue)), 2);
}

void AchievementLogicSerializer::AppendOperand(std::string& sBuffer, Requirement::OperandType nType, ra::data::Memory::Size, float fValue)
{
    switch (nType)
    {
        case Requirement::OperandType::Value:
            sBuffer.append(std::to_string(ra::to_unsigned(gsl::narrow_cast<int>(fValue))));
            break;

        case Requirement::OperandType::Float: {
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

void AchievementLogicSerializer::AppendOperator(std::string& sBuffer, Requirement::OperatorType nType)
{
    switch (nType)
    {
        case Requirement::OperatorType::Equals:
            sBuffer.push_back('=');
            break;

        case Requirement::OperatorType::NotEquals:
            sBuffer.push_back('!');
            sBuffer.push_back('=');
            break;

        case Requirement::OperatorType::LessThan:
            sBuffer.push_back('<');
            break;

        case Requirement::OperatorType::LessThanOrEqual:
            sBuffer.push_back('<');
            sBuffer.push_back('=');
            break;

        case Requirement::OperatorType::GreaterThan:
            sBuffer.push_back('>');
            break;

        case Requirement::OperatorType::GreaterThanOrEqual:
            sBuffer.push_back('>');
            sBuffer.push_back('=');
            break;

        case Requirement::OperatorType::Multiply:
            sBuffer.push_back('*');
            break;

        case Requirement::OperatorType::Divide:
            sBuffer.push_back('/');
            break;

        case Requirement::OperatorType::BitwiseAnd:
            sBuffer.push_back('&');
            break;

        case Requirement::OperatorType::BitwiseXor:
            sBuffer.push_back('^');
            break;

        case Requirement::OperatorType::Modulus:
            sBuffer.push_back('%');
            break;

        case Requirement::OperatorType::Add:
            sBuffer.push_back('+');
            break;

        case Requirement::OperatorType::Subtract:
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

std::string AchievementLogicSerializer::BuildMemRefChain(const ra::data::models::MemoryNoteModel& pRootNote,
    const ra::data::models::MemoryNoteModel& pLeafNote)
{
    std::vector<const ra::data::models::MemoryNoteModel*> vChain;
    if (!pLeafNote.GetPointerChain(vChain, pRootNote))
        return std::string();

    auto nSize = ra::data::Memory::Size::ThirtyTwoBit;
    uint32_t nMask = 0xFFFFFFFF;
    uint32_t nOffset = 0;

    const auto& pConsoleContext = ra::services::ServiceLocator::Get<ra::context::IConsoleContext>();
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

        AppendConditionType(sBuffer, Requirement::Type::AddAddress);
        AppendOperand(sBuffer, Requirement::OperandType::Address, nSize, nAddress);
        nPointerBase = pNote->GetPointerAddress();

        if (nOffset != 0)
        {
            AppendOperator(sBuffer, Requirement::OperatorType::Subtract);
            AppendOperand(sBuffer, Requirement::OperandType::Value, ra::data::Memory::Size::ThirtyTwoBit, nOffset);
        }
        else if (nMask != 0xFFFFFFFF)
        {
            const auto nBitsMask = ra::to_unsigned((1 << ra::data::Memory::SizeBits(nSize)) - 1);
            if (nMask != nBitsMask)
            {
                nBitmaskOffset = sBuffer.length();
                AppendOperator(sBuffer, Requirement::OperatorType::BitwiseAnd);
                AppendOperand(sBuffer, Requirement::OperandType::Value, ra::data::Memory::Size::ThirtyTwoBit, nMask);
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

    AppendConditionType(sBuffer, Requirement::Type::Measured);

    nSize = pLeafNote.GetMemSize();
    if (nSize == ra::data::Memory::Size::Unknown)
        nSize = ra::data::Memory::Size::EightBit;

    AppendOperand(sBuffer, Requirement::OperandType::Address, nSize, nAddress);

    return sBuffer;
}

} // namespace util
} // namespace data
} // namespace ra
