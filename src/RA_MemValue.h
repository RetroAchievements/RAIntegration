#ifndef RA_MEMVALUE_H
#define RA_MEMVALUE_H
#pragma once

#include "RA_Condition.h"

#ifndef _VECTOR_
#include <vector>  
#endif /* !_VECTOR_ */

// Represents a value expression (one or more values which are added together to create a single value)
class MemValue
{
public:
    const char* ParseFromString(const char* sBuffer);
    unsigned int GetValue() const;

    bool IsEmpty() const { return m_vClauses.empty(); }


    enum class Format
    {
        TimeFrames,     //	Value is a number describing the amount of frames.
        TimeSecs,       //	Value is a number in amount of seconds.
        TimeMillisecs,  //	Value is a number accurate to hundredths of a second (not actually milliseconds)
        Score,          //	Value is a nondescript 'score' Points.
        Value,          //	Generic value - %01d
        Other,          //	Point-based value (i.e. 1942 ships killed). %06d
    };

    std::string GetFormattedValue(Format nFormat) const { return FormatValue(GetValue(), nFormat); }

    static std::string FormatValue(unsigned int nValue, Format nFormat);
    static Format ParseFormat(const std::string& sFormat);
    static const char* GetFormatString(Format format);

protected:
    enum class ClauseOperation
    {
        None,
        Addition,
        Maximum
    };

    // Represents a single value which may be scaled (multiplied by another value)
    class Clause
    {
    public:
        Clause(ClauseOperation nOperation) : m_nOperation(nOperation) {}

        const char* ParseFromString(const char* pBuffer);       //	Parse string into values, returns end of string
        double GetValue() const;                                //	Get the value in-memory with modifiers
        ClauseOperation GetOperation() const { return m_nOperation; }

    protected:
        unsigned int			m_nAddress = 0;                 //	Raw address of an 8-bit, or value.
        ComparisonVariableSize	m_nVarSize = EightBit;
        double					m_fModifier = 1.0f;             //	* 60 etc
        bool					m_bBCDParse = false;            //	Parse as a binary coded decimal.
        bool					m_bParseVal = false;            //	Parse as a value
        bool					m_bInvertBit = false;
        unsigned int			m_nSecondAddress = 0;
        ComparisonVariableSize	m_nSecondVarSize = EightBit;
        ClauseOperation         m_nOperation;
    };

    std::vector<Clause> m_vClauses;
};

#endif !RA_MEMVALUE_H
