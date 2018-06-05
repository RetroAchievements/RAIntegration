#include "RA_MemValue.h"

#include "RA_MemManager.h"

#include <time.h>

double MemValue::GetValue() const
{
    int nRetVal = 0;
    if (m_bParseVal)
    {
        nRetVal = m_nAddress;	//	insert address as value.
    }
    else
    {
        nRetVal = g_MemManager.ActiveBankRAMRead(m_nAddress, m_nVarSize);

        if (m_bInvertBit && (m_nVarSize >= ComparisonVariableSize::Bit_0) && (m_nVarSize <= ComparisonVariableSize::Bit_7))
        {
            nRetVal = (nRetVal == 1) ? 0 : 1;
        }

        if (m_bBCDParse)
        {
            //	Reparse this value as a binary coded decimal.
            nRetVal = (((nRetVal >> 4) & 0xf) * 10) + (nRetVal & 0xf);
        }
    }

    if (m_nSecondAddress > 0)
    {
        MemValue tempMem;
        tempMem.m_bInvertBit = m_bInvertBit;
        tempMem.m_nAddress = m_nSecondAddress;
        tempMem.m_nVarSize = m_nSecondVarSize;

        return nRetVal * tempMem.GetValue();
    }

    return nRetVal * m_fModifier;
}

const char* MemValue::ParseFromString(const char* pBuffer)
{
    const char* pIter = &pBuffer[0];

    //	Borrow parsing from CompVariable

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
        pIter++;						//	Skip over modifier type.. assume mult( '*' );

        // Invert bit flag results
        if (*pIter == '~')
        {
            m_bInvertBit = true;
            pIter++;
        }

        // Multiply by addresses
        if (strncmp(pIter, "0x", sizeof("0x") - 1) == 0)
        {
            varTemp.ParseVariable(pIter);
            m_nSecondAddress = varTemp.RawValue();
            m_nSecondVarSize = varTemp.Size();
        }
        else
        {
            char* pOut;
            m_fModifier = strtod(pIter, &pOut);
            pIter = pOut;
        }
    }

    return pIter;
}

//////////////////////////////////////////////////////////////////////////
double MemValueSet::GetValue() const
{
    double fVal = 0.0;
    std::vector<MemValue>::const_iterator iter = m_Values.begin();
    while (iter != m_Values.end())
    {
        fVal += (*iter).GetValue();
        iter++;
    }

    return fVal;
}

double MemValueSet::GetOperationsValue(std::vector<OperationType> sOperations) const
{
    double fVal = 0.0;
    std::vector<MemValue>::const_iterator iter = m_Values.begin();
    std::vector<OperationType>::const_iterator sOp = sOperations.begin();

    if (iter != m_Values.end())
    {
        fVal += (*iter).GetValue();
        iter++;
    }

    while (iter != m_Values.end())
    {
        if (sOp != sOperations.end() && *sOp == Operation_Maximum)
        {
            double maxValue = (*iter).GetValue();
            iter++;
            maxValue = (maxValue < (*iter).GetValue()) ? (*iter).GetValue() : maxValue;
            fVal = (fVal < maxValue) ? maxValue : fVal;
        }
        else
            fVal += (*iter).GetValue();

        if (sOp == sOperations.end())
            break;

        iter++;
        sOp++;
    }

    return fVal;
}

void MemValueSet::AddNewValue(MemValue nMemVal)
{
    m_Values.push_back(nMemVal);
}

void MemValueSet::ParseMemString(const char* pChar)
{
    do
    {
        {
            while ((*pChar) == ' ' || (*pChar) == '_' || (*pChar) == '|')
                pChar++; // Skip any chars up til this point :S
        }

        MemValue newMemVal;
        pChar = newMemVal.ParseFromString(pChar);
        AddNewValue(newMemVal);
    } while (*pChar == '_');
}

void MemValueSet::Clear()
{
    m_Values.clear();
}
