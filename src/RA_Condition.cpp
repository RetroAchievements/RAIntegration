#include "RA_Condition.h"

#include "RA_Defs.h"

#include <cctype>

const char* COMPARISONVARIABLESIZE_STR[] = { "Bit0", "Bit1", "Bit2", "Bit3", "Bit4", "Bit5", "Bit6", "Bit7", "Lower4", "Upper4", "8-bit", "16-bit", "32-bit" };
static_assert(SIZEOF_ARRAY(COMPARISONVARIABLESIZE_STR) == NumComparisonVariableSizeTypes, "Must match!");
const char* COMPARISONVARIABLETYPE_STR[] = { "Memory", "Value", "Delta", "DynVar" };
static_assert(SIZEOF_ARRAY(COMPARISONVARIABLETYPE_STR) == NumComparisonVariableTypes, "Must match!");
const char* COMPARISONTYPE_STR[] = { "=", "<", "<=", ">", ">=", "!=" };
static_assert(SIZEOF_ARRAY(COMPARISONTYPE_STR) == NumComparisonTypes, "Must match!");
const char* CONDITIONTYPE_STR[] = { "", "Pause If", "Reset If", "Add Source", "Sub Source", "Add Hits" };
static_assert(SIZEOF_ARRAY(CONDITIONTYPE_STR) == Condition::NumConditionTypes, "Must match!");

static const char* ComparisonSizeToPrefix(ComparisonVariableSize nSize)
{
    switch (nSize)
    {
        case Bit_0:			return "M";
        case Bit_1:			return "N";
        case Bit_2:			return "O";
        case Bit_3:			return "P";
        case Bit_4:			return "Q";
        case Bit_5:			return "R";
        case Bit_6:			return "S";
        case Bit_7:			return "T";
        case Nibble_Lower:	return "L";
        case Nibble_Upper:	return "U";
        case EightBit:		return "H";
        case ThirtyTwoBit:	return "X";
        default:
        case SixteenBit:	return " ";
    }
}

static const char* ComparisonTypeToStr(ComparisonType nType)
{
    switch (nType)
    {
        case Equals:				return "=";
        case GreaterThan:			return ">";
        case GreaterThanOrEqual:	return ">=";
        case LessThan:				return "<";
        case LessThanOrEqual:		return "<=";
        case NotEqualTo:			return "!=";
        default:					return "";
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

void CompVariable::SerializeAppend(std::string& buffer) const
{
    char valueBuffer[20];
    switch (m_nVarType)
    {
        case ValueComparison:
            sprintf_s(valueBuffer, sizeof(valueBuffer), "%zu", m_nVal);
            buffer.append(valueBuffer);
            break;

        case DeltaMem:
            buffer.append(1, 'd');
            // explicit fallthrough to Address

        case Address:
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
