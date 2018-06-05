#pragma once

#include "RA_Condition.h"

#include <vector>

class MemValue
{
public:
    const char* ParseFromString(const char* pBuffer);       //	Parse string into values, returns end of string
    double GetValue() const;                                //	Get the value in-memory with modifiers

    unsigned int			m_nAddress = 0;                 //	Raw address of an 8-bit, or value.
    ComparisonVariableSize	m_nVarSize = EightBit;
    double					m_fModifier = 1.0f;             //	* 60 etc
    bool					m_bBCDParse = false;            //	Parse as a binary coded decimal.
    bool					m_bParseVal = false;            //	Parse as a value
    bool					m_bInvertBit = false;
    unsigned int			m_nSecondAddress = 0;
    ComparisonVariableSize	m_nSecondVarSize = EightBit;
};

class MemValueSet
{
public:
    enum OperationType
    {
        Operation_None,
        Operation_Addition,
        Operation_Maximum
    };

    void ParseMemString(const char* sBuffer);
    double GetValue() const;
    double GetOperationsValue(std::vector<OperationType>) const;
    void AddNewValue(MemValue nVal);
    void Clear();

    size_t NumValues() const { return m_Values.size(); }

protected:
    std::vector<MemValue> m_Values;
};
