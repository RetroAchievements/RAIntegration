#include "RA_Condition.h"

#include "RA_Defs.h"

_NODISCARD inline static constexpr auto ComparisonSizeToPrefix(_In_ MemSize nSize) noexcept
{
    switch (nSize)
    {
        case MemSize::Bit_0:        return "M";
        case MemSize::Bit_1:        return "N";
        case MemSize::Bit_2:        return "O";
        case MemSize::Bit_3:        return "P";
        case MemSize::Bit_4:        return "Q";
        case MemSize::Bit_5:        return "R";
        case MemSize::Bit_6:        return "S";
        case MemSize::Bit_7:        return "T";
        case MemSize::Nibble_Lower: return "L";
        case MemSize::Nibble_Upper: return "U";
        case MemSize::EightBit:     return "H";
        case MemSize::ThirtyTwoBit: return "X";
        default:
        case MemSize::SixteenBit:   return " ";
    }
}

static const char* ComparisonTypeToStr(ComparisonType nType)
{
    switch (nType)
    {
        case Equals:                return "=";
        case GreaterThan:           return ">";
        case GreaterThanOrEqual:    return ">=";
        case LessThan:              return "<";
        case LessThanOrEqual:       return "<=";
        case NotEqualTo:            return "!=";
        default:                    return "";
    }
}

void Condition::SerializeAppend(std::string& buffer) const
{
    switch (m_nConditionType)
    {
        case Condition::ResetIf:
            buffer.append("R:");
            break;
        case Condition::PauseIf:
            buffer.append("P:");
            break;
        case Condition::AddSource:
            buffer.append("A:");
            break;
        case Condition::SubSource:
            buffer.append("B:");
            break;
        case Condition::AddHits:
            buffer.append("C:");
            break;
        default:
            break;
    }

    m_nCompSource.SerializeAppend(buffer);

    buffer.append(ComparisonTypeToStr(m_nCompareType));

    m_nCompTarget.SerializeAppend(buffer);

    if (m_nRequiredHits > 0)
    {
        char reqHitsBuffer[24];
        snprintf(reqHitsBuffer, sizeof(reqHitsBuffer), ".%zu.", m_nRequiredHits);
        buffer.append(reqHitsBuffer);
    }
}

_Use_decl_annotations_
void CompVariable::SerializeAppend(std::string& buffer) const
{
    char valueBuffer[20];
    switch (m_nVarType)
    {
        case Type::ValueComparison:
            sprintf_s(valueBuffer, sizeof(valueBuffer), "%zu", m_nVal);
            buffer.append(valueBuffer);
            break;

        case Type::DeltaMem:
            buffer.append(1, 'd');
            // explicit fallthrough to Address

        case Type::Address:
            buffer.append("0x");

            buffer.append(ComparisonSizeToPrefix(m_nVarSize));

            if (m_nVal >= 0x10000)
                sprintf_s(valueBuffer, sizeof(valueBuffer), "%06x", m_nVal);
            else
                sprintf_s(valueBuffer, sizeof(valueBuffer), "%04x", m_nVal);
            buffer.append(valueBuffer);
            break;

        default:
            ASSERT(!"Unknown type? (DynMem)?");
            break;
    }
}

void ConditionGroup::RemoveAt(size_t nID)
{
    size_t nCount = 0;
    std::vector<Condition>::iterator iter = m_Conditions.begin();
    while (iter != m_Conditions.end())
    {
        if (nCount == nID)
        {
            iter = m_Conditions.erase(iter);
            break;
        }

        nCount++;
        iter++;
    }
}

void ConditionGroup::SerializeAppend(std::string& buffer) const
{
    if (m_Conditions.empty())
        return;

    m_Conditions[0].SerializeAppend(buffer);
    for (size_t i = 1; i < m_Conditions.size(); ++i)
    {
        buffer.append(1, '_');
        m_Conditions[i].SerializeAppend(buffer);
    }
}

void ConditionSet::Serialize(std::string& buffer) const
{
    buffer.clear();
    if (m_vConditionGroups.empty())
        return;

    int conditions = 0;
    for (const ConditionGroup& group : m_vConditionGroups)
        conditions += group.Count();

    // estimate 16 bytes per condition: "R:0xH0001=12" is only 12, so we give ourselves a little leeway
    buffer.reserve(conditions * 16);

    m_vConditionGroups[0].SerializeAppend(buffer);
    for (size_t i = 1; i < m_vConditionGroups.size(); ++i)
    {
        // ignore empty groups when serializing
        if (m_vConditionGroups[i].Count() == 0)
            continue;

        buffer.append(1, 'S');
        m_vConditionGroups[i].SerializeAppend(buffer);
    }
}
