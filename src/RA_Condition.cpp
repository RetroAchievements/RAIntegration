#include "RA_Condition.h"
#include "RA_MemManager.h"

#include <cctype>

const char* COMPARISONVARIABLESIZE_STR[] = { "Bit0", "Bit1", "Bit2", "Bit3", "Bit4", "Bit5", "Bit6", "Bit7", "Lower4", "Upper4", "8-bit", "16-bit", "32-bit" };
static_assert(SIZEOF_ARRAY(COMPARISONVARIABLESIZE_STR) == NumComparisonVariableSizeTypes, "Must match!");
const char* COMPARISONVARIABLETYPE_STR[] = { "Memory", "Value", "Delta", "DynVar" };
static_assert(SIZEOF_ARRAY(COMPARISONVARIABLETYPE_STR) == NumComparisonVariableTypes, "Must match!");
const char* COMPARISONTYPE_STR[] = { "=", "<", "<=", ">", ">=", "!=" };
static_assert(SIZEOF_ARRAY(COMPARISONTYPE_STR) == NumComparisonTypes, "Must match!");
const char* CONDITIONTYPE_STR[] = { "", "Pause If", "Reset If", "Add Source", "Sub Source", "Add Hits" };
static_assert(SIZEOF_ARRAY(CONDITIONTYPE_STR) == Condition::NumConditionTypes, "Must match!");

static ComparisonVariableSize PrefixToComparisonSize(char cPrefix)
{
    //	Careful not to use ABCDEF here, this denotes part of an actual variable!
    switch (cPrefix)
    {
        case 'M':	return Bit_0;
        case 'N':	return Bit_1;
        case 'O':	return Bit_2;
        case 'P':	return Bit_3;
        case 'Q':	return Bit_4;
        case 'R':	return Bit_5;
        case 'S':	return Bit_6;
        case 'T':	return Bit_7;
        case 'L':	return Nibble_Lower;
        case 'U':	return Nibble_Upper;
        case 'H':	return EightBit;
        case 'X':	return ThirtyTwoBit;
        default:
        case ' ':	return SixteenBit;
    }
}

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

static ComparisonType ReadOperator(const char*& pBufferInOut)
{
    switch (pBufferInOut[0])
    {
        case '=':
            if (pBufferInOut[1] == '=')
                ++pBufferInOut;
            ++pBufferInOut;
            return ComparisonType::Equals;

        case '!':
            if (pBufferInOut[1] == '=')
            {
                pBufferInOut += 2;
                return ComparisonType::NotEqualTo;
            }
            break;

        case '<':
            if (pBufferInOut[1] == '=')
            {
                pBufferInOut += 2;
                return ComparisonType::LessThanOrEqual;
            }

            ++pBufferInOut;
            return ComparisonType::LessThan;

        case '>':
            if (pBufferInOut[1] == '=')
            {
                pBufferInOut += 2;
                return ComparisonType::GreaterThanOrEqual;
            }

            ++pBufferInOut;
            return ComparisonType::GreaterThan;

        default:
            break;
    }

    ASSERT(!"Could not parse?!");
    return ComparisonType::Equals;
}

static unsigned int ReadHits(const char*& pBufferInOut)
{
    if (pBufferInOut[0] == '(' || pBufferInOut[0] == '.')
    {
        unsigned int nNumHits = strtoul(pBufferInOut + 1, (char**)&pBufferInOut, 10);	//	dirty!
        pBufferInOut++;
        return nNumHits;
    }

    //	0 by default: disable hit-tracking!
    return 0;
}

bool Condition::ParseFromString(const char*& pBuffer)
{
    if (pBuffer[0] == 'R' && pBuffer[1] == ':')
    {
        m_nConditionType = Condition::ResetIf;
        pBuffer += 2;
    }
    else if (pBuffer[0] == 'P' && pBuffer[1] == ':')
    {
        m_nConditionType = Condition::PauseIf;
        pBuffer += 2;
    }
    else if (pBuffer[0] == 'A' && pBuffer[1] == ':')
    {
        m_nConditionType = Condition::AddSource;
        pBuffer += 2;
    }
    else if (pBuffer[0] == 'B' && pBuffer[1] == ':')
    {
        m_nConditionType = Condition::SubSource;
        pBuffer += 2;
    }
    else if (pBuffer[0] == 'C' && pBuffer[1] == ':')
    {
        m_nConditionType = Condition::AddHits;
        pBuffer += 2;
    }
    else
    {
        m_nConditionType = Condition::Standard;
    }

    if (!m_nCompSource.ParseVariable(pBuffer))
        return false;

    m_nCompareType = ReadOperator(pBuffer);

    if (!m_nCompTarget.ParseVariable(pBuffer))
        return false;

    ResetHits();
    m_nRequiredHits = ReadHits(pBuffer);

    return true;
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

bool Condition::ResetHits()
{
    if (m_nCurrentHits == 0)
        return false;

    m_nCurrentHits = 0;
    return true;
}

void Condition::ResetDeltas()
{
    m_nCompSource.ResetDelta();
    m_nCompTarget.ResetDelta();
}

bool CompVariable::ParseVariable(const char*& pBufferInOut)
{
    char* nNextChar = nullptr;
    unsigned int nBase = 16;	//	Assume hex address

    if (toupper(pBufferInOut[0]) == 'D' && pBufferInOut[1] == '0' && toupper(pBufferInOut[2]) == 'X')
    {
        //	Assume 'd0x' and four hex following it.
        pBufferInOut += 3;
        m_nVarType = ComparisonVariableType::DeltaMem;
    }
    else if (pBufferInOut[0] == '0' && toupper(pBufferInOut[1]) == 'X')
    {
        //	Assume '0x' and four hex following it.
        pBufferInOut += 2;
        m_nVarType = ComparisonVariableType::Address;
    }
    else
    {
        m_nVarType = ComparisonVariableType::ValueComparison;
        //	Val only
        if (toupper(pBufferInOut[0]) == 'H')
        {
            nBase = 16;
            pBufferInOut++;
        }
        else
        {
            //	Values in decimal.
            nBase = 10;
        }
    }


    if (m_nVarType == ComparisonVariableType::ValueComparison)
    {
        //	Values don't have a size!
    }
    else
    {
        m_nVarSize = PrefixToComparisonSize(static_cast<char>(std::toupper(static_cast<int>(pBufferInOut[0]))));
        if (m_nVarSize != ComparisonVariableSize::SixteenBit)
            pBufferInOut++;	//	In all cases except one, advance char ptr
    }

    m_nVal = strtoul(pBufferInOut, &nNextChar, nBase);
    pBufferInOut = nNextChar;

    return true;
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

//	Return the raw, live value of this variable. Advances 'deltamem'
unsigned int CompVariable::GetValue()
{
    unsigned int nPreviousVal;

    switch (m_nVarType)
    {
        case ValueComparison:
            //	It's a raw value. Return it.
            return m_nVal;

        case Address:
            //	It's an address in memory. Return it!
            return g_MemManager.ActiveBankRAMRead(m_nVal, m_nVarSize);

        case DeltaMem:
            //	Return the backed up (last frame) value, but store the new one for the next frame!
            nPreviousVal = m_nPreviousVal;
            m_nPreviousVal = g_MemManager.ActiveBankRAMRead(m_nVal, m_nVarSize);
            return nPreviousVal;

        default:
            //	Panic!
            ASSERT(!"Undefined mem type!");
            return 0;
    }
}

bool Condition::Compare(unsigned int nAddBuffer)
{
    switch (m_nCompareType)
    {
        case Equals:
            return(m_nCompSource.GetValue() + nAddBuffer == m_nCompTarget.GetValue());
        case LessThan:
            return(m_nCompSource.GetValue() + nAddBuffer < m_nCompTarget.GetValue());
        case LessThanOrEqual:
            return(m_nCompSource.GetValue() + nAddBuffer <= m_nCompTarget.GetValue());
        case GreaterThan:
            return(m_nCompSource.GetValue() + nAddBuffer > m_nCompTarget.GetValue());
        case GreaterThanOrEqual:
            return(m_nCompSource.GetValue() + nAddBuffer >= m_nCompTarget.GetValue());
        case NotEqualTo:
            return(m_nCompSource.GetValue() + nAddBuffer != m_nCompTarget.GetValue());
        default:
            return true;	//?
    }
}

bool ConditionGroup::Test(bool& bDirtyConditions, bool& bResetAll)
{
    const unsigned int nNumConditions = m_Conditions.size();
    if (nNumConditions == 0)
        return true; // important: empty group must evaluate true

    // identify any Pause conditions and their dependent AddSource/AddHits
    std::vector<bool> vPauseConditions(nNumConditions, false);
    bool bInPause = false;
    bool bHasPause = false;
    for (int i = nNumConditions - 1; i >= 0; --i)
    {
        switch (m_Conditions[i].GetConditionType())
        {
            case Condition::PauseIf:
                bHasPause = true;
                bInPause = true;
                vPauseConditions[i] = true;
                break;

            case Condition::AddSource:
            case Condition::SubSource:
            case Condition::AddHits:
                vPauseConditions[i] = bInPause;
                break;

            default:
                bInPause = false;
                break;
        }
    }

    if (bHasPause)
    {
        // one or more Pause conditions exists, if any of them are true, stop processing this group
        if (Test(bDirtyConditions, bResetAll, vPauseConditions, true))
            return false;
    }

    // process the non-Pause conditions to see if the group is true
    return Test(bDirtyConditions, bResetAll, vPauseConditions, false);
}

bool ConditionGroup::Test(bool& bDirtyConditions, bool& bResetAll, const std::vector<bool>& vPauseConditions, bool bProcessingPauseIfs)
{
    unsigned int nAddBuffer = 0;
    unsigned int nAddHits = 0;
    bool bSetValid = true; // must start true so AND logic works
                          
    for (size_t i = 0; i < m_Conditions.size(); ++i)
    {
        if (vPauseConditions[i] != bProcessingPauseIfs)
            continue;

        Condition* pNextCond = &m_Conditions[i];
        switch (pNextCond->GetConditionType())
        {
            case Condition::AddSource:
                nAddBuffer += pNextCond->CompSource().GetValue();
                continue;

            case Condition::SubSource:
                nAddBuffer -= pNextCond->CompSource().GetValue();
                continue;

            case Condition::AddHits:
                if (pNextCond->Compare())
                {
                    if (pNextCond->RequiredHits() == 0 || pNextCond->CurrentHits() < pNextCond->RequiredHits())
                    {
                        pNextCond->IncrHits();
                        bDirtyConditions = true;
                    }
                }

                nAddHits += pNextCond->CurrentHits();
                continue;

            default:
                break;
        }

        // always evaluate the condition to ensure delta values get tracked correctly
        bool bConditionValid = pNextCond->Compare(nAddBuffer);

        // if the condition has a target hit count that has already been met, it's automatically true, even if not currently true.
        if (pNextCond->RequiredHits() != 0 && (pNextCond->CurrentHits() + nAddHits >= pNextCond->RequiredHits()))
        {
            bConditionValid = true;
        }
        else if (bConditionValid)
        {
            pNextCond->IncrHits();
            bDirtyConditions = true;

            if (pNextCond->RequiredHits() == 0)
            {
                // not a hit-based requirement: ignore any additional logic!
            }
            else if (pNextCond->CurrentHits() + nAddHits < pNextCond->RequiredHits())
            {
                // HitCount target has not yet been met, condition is not yet valid
                bConditionValid = false;
            }
        }

        nAddBuffer = 0;
        nAddHits = 0;

        switch (pNextCond->GetConditionType())
        {
            case Condition::PauseIf:
                // as soon as we find a PauseIf that evaluates to true, stop processing the rest of the group
                if (bConditionValid)
                    return true;
                
                // if we make it to the end of the function, make sure we indicate that nothing matched. if we do find
                // a later PauseIf match, it'll automatically return true via the previous condition.
                bSetValid = false; 

                if (pNextCond->RequiredHits() == 0)
                {
                    // PauseIf didn't evaluate true, and doesn't have a HitCount, reset the HitCount to indicate the condition didn't match
                    if (pNextCond->ResetHits())
                        bDirtyConditions = true;
                }
                else
                {
                    // PauseIf has a HitCount that hasn't been met, ignore it for now.
                }
                break;

            case Condition::ResetIf:
                if (bConditionValid)
                {
                    bResetAll = true;  // let caller know to reset all hit counts
                    bSetValid = false; // cannot be valid if we've hit a reset condition
                }
                break;

            default:
                bSetValid &= bConditionValid;
                break;
        }
    }

    return bSetValid;
}

bool ConditionGroup::Reset(bool bIncludingDeltas)
{
    bool bWasReset = false;
    for (size_t i = 0; i < m_Conditions.size(); ++i)
    {
        bWasReset |= m_Conditions[i].ResetHits();

        if (bIncludingDeltas)
            m_Conditions[i].ResetDeltas();
    }

    return bWasReset;
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

bool ConditionSet::ParseFromString(const char*& sSerialized)
{
    ConditionGroup* group = nullptr;
    m_vConditionGroups.clear();

    // if string starts with 'S', there's no Core group. generate an empty one and leave 'group' as null
    // so the alt group will get created by the first condition.
    if (*sSerialized == 'S')
    {
        ++sSerialized;
        m_vConditionGroups.emplace_back();
    }

    do
    {
        Condition condition;
        if (!condition.ParseFromString(sSerialized))
            return false;

        if (!group)
        {
            m_vConditionGroups.emplace_back();
            group = &m_vConditionGroups.back();
        }

        group->Add(condition);

        if (*sSerialized == '_')
        {
            // AND
            ++sSerialized;
        }
        else if (*sSerialized == 'S')
        {
            // OR
            ++sSerialized;
            group = nullptr;
        }
        else
        {
            // EOF or invalid character
            break;
        }

    } while (true);

    return true;
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

bool ConditionSet::Test(bool& bDirtyConditions, bool& bResetConditions)
{
    // bDirtyConditions is set if any condition's HitCount changes
    bDirtyConditions = false;

    // bResetConditions is set if any condition's ResetIf triggers
    bResetConditions = false;

    if (m_vConditionGroups.empty())
        return false;

    // use local variable for tracking need to reset in case caller passes the same address for 
    // both variables when they don't care about the result
    bool bNeedsReset = false;

    // for a set to be true, the first group (core) must be true. if any additional groups (alt)
    // exist, at least one of them must also be true.
    bool bResult = m_vConditionGroups[0].Test(bDirtyConditions, bNeedsReset);
    if (m_vConditionGroups.size() > 1)
    {
        bool bAltResult = false;
        for (size_t i = 1; i < m_vConditionGroups.size(); ++i)
        {
            if (m_vConditionGroups[i].Test(bDirtyConditions, bNeedsReset))
                bAltResult = true;
        }

        if (!bAltResult || bNeedsReset)
            bResult = false;
    }

    if (bNeedsReset)
    {
        bResetConditions = true;
        bDirtyConditions = Reset();
    }

    return bResult;
}

void ConditionSet::SetAlwaysTrue()
{
    // a single empty core group always evaluates true
    m_vConditionGroups.clear();
    AddGroup();
}

void ConditionSet::SetAlwaysFalse()
{
    // an empty condition set always evaluates false
    Clear();
}

bool ConditionSet::Reset()
{
    bool bWasReset = false;
    for (ConditionGroup& group : m_vConditionGroups)
    {
        if (group.Reset(false))
            bWasReset = true;
    }

    return bWasReset;
}
