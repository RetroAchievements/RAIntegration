#include "RA_Condition.h"
#include <assert.h>

#include "RA_MemManager.h"

namespace
{
	const char* g_MemSizeStrings[] = { "Bit0", "Bit1", "Bit2", "Bit3", "Bit4", "Bit5", "Bit6", "Bit7", "Lower4", "Upper4", "8-bit", "16-bit" };
	const int g_NumMemSizeStrings = sizeof( g_MemSizeStrings ) / sizeof( g_MemSizeStrings[0] );

	const char* g_MemTypeStrings[] = { "Memory", "Value", "Delta" };
	const int g_NumMemTypeStrings = sizeof( g_MemTypeStrings ) / sizeof( g_MemTypeStrings[0] );

	const char* g_CmpStrings[] = { "=", "<", "<=", ">", ">=", "!=" };
	const int g_NumCompTypes = sizeof( g_CmpStrings ) / sizeof( g_CmpStrings[0] );
};


void Condition::ParseFromString( char*& pBuffer )
{
	if( pBuffer[0] == 'R' && pBuffer[1] == ':' )
	{
		SetIsResetCondition();
		pBuffer+=2;
	}
	else if( pBuffer[0] == 'P' && pBuffer[1] == ':' )
	{
		SetIsPauseCondition();
		pBuffer+=2;
	}
	else
	{
		SetIsBasicCondition();
	} 

	m_nCompSource.ParseVariable( pBuffer );
	m_nComparison = ReadOperator( pBuffer );
	m_nCompTarget.ParseVariable( pBuffer );
	ResetHits();
	m_nRequiredHits = ReadHits( pBuffer );

}

BOOL Condition::ResetHits()
{ 
	if( m_nCurrentHits == 0 )
		return FALSE;

	m_nCurrentHits = 0;
	return TRUE;
}

void Condition::ResetDeltas()
{
	m_nCompSource.m_nLastVal = m_nCompSource.m_nVal;
	m_nCompTarget.m_nLastVal = m_nCompTarget.m_nVal;
}

void Condition::Clear()
{
	m_nCurrentHits = 0;
	m_nRequiredHits = 0;
	m_nCompSource.Clear();
	m_nCompTarget.Clear();
	m_nComparison = CMP_EQ;
	m_bIsResetCondition = FALSE;
	m_bIsPauseCondition = FALSE;
}


void CompVariable::ParseVariable( char*& pBufferInOut )
{
	char* nNextChar = NULL;
	unsigned int nBase = 16;	//	Assume hex address

	if( toupper( pBufferInOut[0] ) == 'D' && pBufferInOut[1] == '0' && toupper( pBufferInOut[2] ) == 'X' )
	{
		//	Assume 'd0x' and four hex following it.
		pBufferInOut += 3;
		m_nVarType = CMPTYPE_DELTAMEM;
	}
	else if( pBufferInOut[0] == '0' && toupper( pBufferInOut[1] ) == 'X' )
	{
		//	Assume '0x' and four hex following it.
		pBufferInOut += 2;
		m_nVarType = CMPTYPE_ADDRESS;
	}
	else 
	{
		m_nVarType = CMPTYPE_VALUE;
		//	Val only
		if( toupper( pBufferInOut[0] ) == 'H' )
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


	if( m_nVarType == CMPTYPE_VALUE )
	{
		//	Values don't have a size!
	}
	else
	{
		if( toupper( pBufferInOut[0] ) == 'H' )
		{
			//	Half: 8-bit
			m_nVarSize = CMP_SZ_8BIT;
			pBufferInOut++;
		}
		else if( toupper( pBufferInOut[0] ) == 'L' )
		{
			//	Nibble: 4-bit, LOWER
			m_nVarSize = CMP_SZ_4BIT_LOWER;
			pBufferInOut++;
		}
		else if( toupper( pBufferInOut[0] ) == 'U' )
		{
			//	Nibble: 4-bit, UPPER
			m_nVarSize = CMP_SZ_4BIT_UPPER;
			pBufferInOut++;
		}
		else if( toupper( pBufferInOut[0] ) == 'M' )	//	MNOPQRST=bits
		{
			//	Bit: 0
			m_nVarSize = CMP_SZ_1BIT_0;
			pBufferInOut++;
		}
		else if( toupper( pBufferInOut[0] ) == 'N' )	//	MNOPQRST=bits
		{
			//	Bit: 1
			m_nVarSize = CMP_SZ_1BIT_1;
			pBufferInOut++;
		}
		else if( toupper( pBufferInOut[0] ) == 'O' )	//	MNOPQRST=bits
		{
			//	Bit: 2
			m_nVarSize = CMP_SZ_1BIT_2;
			pBufferInOut++;
		}
		else if( toupper( pBufferInOut[0] ) == 'P' )	//	MNOPQRST=bits
		{
			//	Bit: 3
			m_nVarSize = CMP_SZ_1BIT_3;
			pBufferInOut++;
		}
		else if( toupper( pBufferInOut[0] ) == 'Q' )	//	MNOPQRST=bits
		{
			//	Bit: 4
			m_nVarSize = CMP_SZ_1BIT_4;
			pBufferInOut++;
		}
		else if( toupper( pBufferInOut[0] ) == 'R' )	//	MNOPQRST=bits
		{
			//	Bit: 5
			m_nVarSize = CMP_SZ_1BIT_5;
			pBufferInOut++;
		}
		else if( toupper( pBufferInOut[0] ) == 'S' )	//	MNOPQRST=bits
		{
			//	Bit: 6
			m_nVarSize = CMP_SZ_1BIT_6;
			pBufferInOut++;
		}
		else if( toupper( pBufferInOut[0] ) == 'T' )	//	MNOPQRST=bits
		{
			//	Bit: 7
			m_nVarSize = CMP_SZ_1BIT_7;
			pBufferInOut++;
		}
		else
		{
			//	Regular: 16-bit
			m_nVarSize = CMP_SZ_16BIT;
		}
	}

	m_nVal = strtol( pBufferInOut, &nNextChar, nBase );
	pBufferInOut = nNextChar;
}

//	Return the raw, live value of this variable
unsigned int CompVariable::GetValue()
{
	unsigned int nLastVal = m_nLastVal;
	unsigned int nLiveVal = 0;

	if( m_nVarType == CMPTYPE_VALUE )
	{
		//	It's a raw value; return it.
		return m_nVal;
	}
	else 
	{
		//	It's an address in memory: return it!

		if( m_nVarSize == CMP_SZ_4BIT_LOWER )
		{
			nLiveVal = g_MemManager.RAMByte( m_nVal )&0xf;
		}
		else if( m_nVarSize == CMP_SZ_4BIT_UPPER )
		{
			nLiveVal = (g_MemManager.RAMByte( m_nVal )>>4)&0xf;
		}
		else if( m_nVarSize == CMP_SZ_8BIT )
		{
			nLiveVal = g_MemManager.RAMByte( m_nVal );
		}
		else if( m_nVarSize == CMP_SZ_16BIT )
		{
			nLiveVal = g_MemManager.RAMByte( m_nVal );
			nLiveVal |= (g_MemManager.RAMByte( m_nVal+1 ) << 8);
		}
		else if( m_nVarSize >= CMP_SZ_1BIT_0 && m_nVarSize <= CMP_SZ_1BIT_7 )
		{
			const unsigned int nBitmask = ( 1 << (m_nVarSize-CMP_SZ_1BIT_0) );
			nLiveVal = ( g_MemManager.RAMByte( m_nVal ) & nBitmask ) != 0;
		}

		if( m_nVarType == CMPTYPE_DELTAMEM )
		{
			//	Return the backed up (last frame) value, but
			//	 store the new one for the next frame!
			m_nLastVal = nLiveVal;
			return nLastVal;
		}
		else
		{
			return nLiveVal;
		}
	}
}

CompType ReadOperator( char*& pBufferInOut )
{
	if( pBufferInOut[0] == '=' && pBufferInOut[1] == '=' )
	{
		pBufferInOut += 2; 
		return CMP_EQ;
	}
	else if( pBufferInOut[0] == '=' )
	{
		pBufferInOut += 1;
		return CMP_EQ;
	}

	if( pBufferInOut[0] == '!' && pBufferInOut[1] == '=' )
	{
		pBufferInOut += 2;
		return CMP_NEQ;
	}

	if( pBufferInOut[0] == '<' && pBufferInOut[1] == '=' )
	{
		pBufferInOut += 2;
		return CMP_LTE;
	}
	else if( pBufferInOut[0] == '<' )
	{
		pBufferInOut += 1;
		return CMP_LT;
	}

	if( pBufferInOut[0] == '>' && pBufferInOut[1] == '=' )
	{
		pBufferInOut += 2;
		return CMP_GTE;
	}
	else if( pBufferInOut[0] == '>' )
	{
		pBufferInOut += 1;
		return CMP_GT;
	}

	assert(0);	
	return CMP_EQ;
}

unsigned int ReadHits( char*& pBufferInOut )
{
	unsigned int nNumHits = 0;
	if( pBufferInOut[0] == '(' || pBufferInOut[0] == '.' )
	{
		nNumHits = strtol( pBufferInOut+1, (char**)&pBufferInOut, 10 );	//	dirty!
		pBufferInOut++;
	}
	else
	{	
		//	0 by default: disable hit-tracking!
	}
	
	return nNumHits;
}

BOOL Condition::Compare()
{
	BOOL bValid = TRUE;

	unsigned int nLHS = m_nCompSource.GetValue();
	unsigned int nRHS = m_nCompTarget.GetValue();

	switch( m_nComparison )
	{
	case CMP_EQ:
		bValid = (nLHS == nRHS);
		break;
	case CMP_LT:
		bValid = (nLHS < nRHS);
		break;
	case CMP_LTE:
		bValid = (nLHS <= nRHS);
		break;
	case CMP_GT:
		bValid = (nLHS > nRHS);
		break;
	case CMP_GTE:
		bValid = (nLHS >= nRHS);
		break;
	case CMP_NEQ:
		bValid = (nLHS != nRHS);
		break;
	default:
		//	bValid = true;
		break;
	}

	return bValid;
}

BOOL ConditionSet::Test( BOOL& bDirtyConditions, BOOL& bResetRead, BOOL bMatchAny )
{
	BOOL bConditionValid = FALSE;
	BOOL bSetValid = TRUE;
	BOOL bPauseActive = FALSE;
	unsigned int i = 0;
	unsigned int nSrc = 0;
	unsigned int nTgt = 0;

	const unsigned int nNumConditions = m_Conditions.size();

	//	Now, read all Pause conditions, and if any are true, do not process further (retain old state)
	for( i = 0; i < nNumConditions; ++i )
	{
		Condition* pNextCond = &m_Conditions[ i ];
		if( pNextCond->IsPauseCondition() )
		{
			//	Reset by default, set to 1 if hit!
			pNextCond->ResetHits();

			if( pNextCond->Compare() )
			{
				pNextCond->OverrideCurrentHits( 1 );
				//SetDirtyFlag( Dirty_Conditions );
				bDirtyConditions = TRUE;

				//	Early out: this achievement is paused, do not process any further!
				return FALSE;
			}
		}
	}

	//	Read all standard conditions, and process as normal:
	for( i = 0; i < nNumConditions; ++i )
	{
		Condition* pNextCond = &m_Conditions[ i ];
		if( pNextCond->IsPauseCondition() || pNextCond->IsResetCondition() )
			continue;

		if( pNextCond->RequiredHits() != 0 && pNextCond->IsComplete() )
			continue;

		bConditionValid = pNextCond->Compare();
		if( bConditionValid )
		{
			pNextCond->IncrHits();
			bDirtyConditions = TRUE;

			//	Process this logic, if this condition is true:

			if( pNextCond->RequiredHits() == 0 )
			{
				//	Not a hit-based requirement: ignore any additional logic!
			}
			else if( !pNextCond->IsComplete() )
			{
				//	Not entirely valid yet!
				bConditionValid = FALSE;
			}

			if( bMatchAny )	//	'or'
				break;
		}

		//	Sequential or non-sequential?
		bSetValid &= bConditionValid;
	}

	//	Now, ONLY read reset conditions!
	for( i = 0; i < nNumConditions; ++i )
	{
		Condition* pNextCond = &m_Conditions[ i ];
		if( pNextCond->IsResetCondition() )
		{
			bConditionValid = pNextCond->Compare();
			if( bConditionValid )
			{
				bResetRead = TRUE;			//	Resets all hits found so far
				bSetValid = FALSE;			//	Cannot be valid if we've hit a reset condition.
				break;						//	No point processing any further reset conditions.
			}
		}
	}

	return bSetValid;
}

BOOL ConditionSet::Reset( BOOL bIncludingDeltas )
{
	BOOL bDirty = FALSE;
	for( size_t i = 0; i < m_Conditions.size(); ++i )
	{
		bDirty |= m_Conditions[i].ResetHits();
		if( bIncludingDeltas )
			m_Conditions[i].ResetDeltas();
	}

	return bDirty;
}

void ConditionSet::RemoveAt( size_t nID )
{
	size_t nCount = 0;
	std::vector<Condition>::iterator iter = m_Conditions.begin();
	while( iter != m_Conditions.end() )
	{
		if( nCount == nID )
		{
			iter = m_Conditions.erase( iter );
			break;
		}

		nCount++;
		iter++;
	}
}