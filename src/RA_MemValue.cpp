#include "RA_MemValue.h"

#include "RA_MemManager.h"

#include <algorithm>
#include "ra_utility.h"

double MemValue::Clause::GetValue() const
{
    unsigned int nRetVal = 0;
    if (m_bParseVal)
    {
        nRetVal = m_nAddress;	//	insert address as value.
    }
    else
    {
        nRetVal = g_MemManager.ActiveBankRAMRead(m_nAddress, m_nVarSize);

        if (m_bBCDParse)
        {
            //	Reparse this value as a binary coded decimal.
            nRetVal = (((nRetVal >> 4) & 0xf) * 10) + (nRetVal & 0xf);
        }
    }

    if (m_nSecondAddress != 0)
    {
        unsigned int nSecondVal = g_MemManager.ActiveBankRAMRead(m_nSecondAddress, m_nSecondVarSize);

        if (m_bInvertBit && m_nSecondVarSize >= ComparisonVariableSize::Bit_0 && m_nSecondVarSize <= ComparisonVariableSize::Bit_7)
            nSecondVal ^= 1;

        return nRetVal * nSecondVal;
    }

    return nRetVal * m_fModifier;
}

const char* MemValue::Clause::ParseFromString(const char* pBuffer)
{
    const char* pIter = &pBuffer[0];

    m_bBCDParse = false;
    if (toupper(*pIter) == 'B')
    {
        m_bBCDParse = true;
        pIter++;
    }
    else if (toupper(*pIter) == 'V')
    {
        m_bParseVal = true;
        pIter++;
    }

    CompVariable varTemp;
    varTemp.ParseVariable(pIter);
    m_nAddress = varTemp.RawValue();	//	Fetch value ('address') as parsed. Note RawValue! Do not parse memory!
    m_nVarSize = varTemp.Size();

    m_fModifier = 1.0;
    if (*pIter == '*')
    {
        pIter++;

        // Invert bit flag - only applies if m_nSecondVarSize is Bit_0~Bit_7
        if (*pIter == '~')
        {
            m_bInvertBit = true;
            pIter++;
        }

        if (strncmp(pIter, "0x", sizeof("0x") - 1) == 0)
        {
            // Multiply by address
            varTemp.ParseVariable(pIter);
            m_nSecondAddress = varTemp.RawValue();
            m_nSecondVarSize = varTemp.Size();
        }
        else
        {
            // Multiply by constant
            char* pOut;
            m_fModifier = strtod(pIter, &pOut);
            pIter = pOut;
        }
    }

    return pIter;
}

//////////////////////////////////////////////////////////////////////////

unsigned int MemValue::GetValue() const
{
    double fVal = 0.0;
    for (const auto& clause : m_vClauses)
    {
        double fNextVal = clause.GetValue();
        switch (clause.GetOperation())
        {
            case ClauseOperation::Maximum:
                fVal = std::max(fVal, fNextVal);
                break;

            case ClauseOperation::Addition:
            default:
                fVal += fNextVal;
                break;
        }
    }

    return static_cast<unsigned int>(fVal);	//	Concern about rounding?
}

const char* MemValue::ParseFromString(const char* pChar)
{
    ClauseOperation nOperation = ClauseOperation::None;

    do
    {
        {
            while ((*pChar) == ' ' || (*pChar) == '_' || (*pChar) == '|' || (*pChar == '$'))
                pChar++; // Skip any chars up til this point :S
        }

        m_vClauses.emplace_back(nOperation);
        pChar = m_vClauses.back().ParseFromString(pChar);

        if (*pChar == '_')
        {
            nOperation = ClauseOperation::Addition;
            ++pChar;
            continue;
        }

        if (*pChar == '$')
        {
            nOperation = ClauseOperation::Maximum;
            ++pChar;
            continue;
        }

        break;
    } while (true);

    return pChar;
}

std::string MemValue::FormatValue(unsigned int nValue, MemValue::Format nFormat)
{
    static const int SECONDS_PER_MINUTE = 60;
    static const int FRAMES_PER_SECOND = 60; // TODO: this isn't true for all systems

    char buffer[128];
    switch (nFormat)
    {
        case Format::TimeFrames:
        {
            int nSecs = nValue / FRAMES_PER_SECOND;
            int nMins = nSecs / SECONDS_PER_MINUTE;
            int nMilli = static_cast<int>((nValue % FRAMES_PER_SECOND) * (100.0 / 60.0));	//	Convert from frames to hundredths of a second
            sprintf_s(buffer, sizeof(buffer), "%02d:%02d.%02d", nMins, nSecs % SECONDS_PER_MINUTE, nMilli);
        }
        break;

        case Format::TimeSecs:
        {
            int nMins = nValue / SECONDS_PER_MINUTE;
            int nSecs = nValue % SECONDS_PER_MINUTE;
            sprintf_s(buffer, sizeof(buffer), "%02d:%02d", nMins, nSecs);
        }
        break;

        case Format::TimeMillisecs:
        {
            int nSecs = nValue / 100; // hundredths of a second
            int nMilli = nValue % 100;
            int nMins = nSecs / SECONDS_PER_MINUTE;
            sprintf_s(buffer, sizeof(buffer), "%02d:%02d.%02d", nMins, nSecs % SECONDS_PER_MINUTE, nMilli);
        }
        break;

        case Format::Score:
            sprintf_s(buffer, sizeof(buffer), "%06d Points", nValue);
            break;

        case Format::Value:
            sprintf_s(buffer, sizeof(buffer), "%01d", nValue);
            break;

        default:
            sprintf_s(buffer, sizeof(buffer), "%06d", nValue);
            break;
    }

    return std::string(buffer);
}

MemValue::Format MemValue::ParseFormat(const std::string& sFormat)
{
    if (sFormat == "VALUE")
        return Format::Value;

    if (sFormat == "SECS" || sFormat == "TIMESECS")
        return Format::TimeSecs;

    if (sFormat == "FRAMES" || sFormat == "TIME")
        return Format::TimeFrames;

    if (sFormat == "POINTS" || sFormat == "SCORE")
        return Format::Score;

    if (sFormat == "MILLISECS")
        return Format::TimeMillisecs;

    if (sFormat == "OTHER")
        return Format::Other;

    return Format::Value;
}

const char* MemValue::GetFormatString(Format format)
{
    switch (format)
    {
        case Format::Other: return "OTHER";
        case Format::Score: return "POINTS";
        case Format::TimeFrames: return "FRAMES";
        case Format::TimeMillisecs: return "MILLISECS";
        case Format::TimeSecs: return "SECS";
        case Format::Value: return "VALUE";
        default: return "UNKNOWN";
    }
}
