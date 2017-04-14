#include "RA_Condition.h"
#include "RA_MemManager.h"

const char* COMPARISONVARIABLESIZE_STR[] = { "Bit0", "Bit1", "Bit2", "Bit3", "Bit4", "Bit5", "Bit6", "Bit7", "Lower4", "Upper4", "8-bit", "16-bit", "32-bit" };
static_assert( SIZEOF_ARRAY( COMPARISONVARIABLESIZE_STR ) == NumComparisonVariableSizeTypes, "Must match!" );
const char* COMPARISONVARIABLETYPE_STR[] = { "Memory", "Value", "Delta", "DynVar" };
static_assert( SIZEOF_ARRAY( COMPARISONVARIABLETYPE_STR ) == NumComparisonVariableTypes, "Must match!" );
const char* COMPARISONTYPE_STR[] = { "=", "<", "<=", ">", ">=", "!=" };
static_assert( SIZEOF_ARRAY( COMPARISONTYPE_STR ) == NumComparisonTypes, "Must match!" );
const char* CONDITIONTYPE_STR[] = { "", "PauseIf", "ResetIf" };
static_assert( SIZEOF_ARRAY( CONDITIONTYPE_STR ) == Condition::NumConditionTypes, "Must match!" );


ComparisonVariableSize PrefixToComparisonSize( char cPrefix )
{
	//	Careful not to use ABCDEF here, this denotes part of an actual variable!
	switch( cPrefix )
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
const char* ComparisonSizeToPrefix( ComparisonVariableSize nSize )
{
	switch( nSize )
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

const char* ComparisonTypeToStr( ComparisonType nType )
{
	switch( nType )
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
	m_nCompareType = ReadOperator( pBuffer );
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
	m_nCompSource.ResetDelta();
	m_nCompTarget.ResetDelta();
}

void Condition::Clear()
{
	m_nConditionType = Standard;
	m_nCurrentHits = 0;
	m_nRequiredHits = 0;
	m_nCompSource.SetValues( 0, 0 );
	m_nCompareType = Equals;
	m_nCompTarget.SetValues( 0, 0 );
}


void CompVariable::ParseVariable( char*& pBufferInOut )
{
	char* nNextChar = NULL;
	unsigned int nBase = 16;	//	Assume hex address

	if( toupper( pBufferInOut[0] ) == 'D' && pBufferInOut[1] == '0' && toupper( pBufferInOut[2] ) == 'X' )
	{
		//	Assume 'd0x' and four hex following it.
		pBufferInOut += 3;
		m_nVarType = ComparisonVariableType::DeltaMem;
	}
	else if( pBufferInOut[0] == '0' && toupper( pBufferInOut[1] ) == 'X' )
	{
		//	Assume '0x' and four hex following it.
		pBufferInOut += 2;
		m_nVarType = ComparisonVariableType::Address;
	}
	else 
	{
		m_nVarType = ComparisonVariableType::ValueComparison;
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


	if( m_nVarType == ComparisonVariableType::ValueComparison )
	{
		//	Values don't have a size!
	}
	else
	{
		m_nVarSize = PrefixToComparisonSize( toupper( pBufferInOut[ 0 ] ) );
		if( m_nVarSize != ComparisonVariableSize::SixteenBit )
			pBufferInOut++;	//	In all cases except one, advance char ptr
	}

	m_nVal = strtol( pBufferInOut, &nNextChar, nBase );
	pBufferInOut = nNextChar;
}

//	Return the raw, live value of this variable. Advances 'deltamem'
unsigned int CompVariable::GetValue()
{
	unsigned int nPreviousVal = m_nPreviousVal;
	unsigned int nLiveVal = 0;

	if( m_nVarType == ComparisonVariableType::ValueComparison )
	{
		//	It's a raw value; return it.
		return m_nVal;
	}
	else if( ( m_nVarType == ComparisonVariableType::Address ) || ( m_nVarType == ComparisonVariableType::DeltaMem ) )
	{
		//	It's an address in memory: return it!

		if( m_nVarSize >= ComparisonVariableSize::Bit_0 && m_nVarSize <= ComparisonVariableSize::Bit_7 )
		{
			const unsigned int nBitmask = ( 1 << ( m_nVarSize - ComparisonVariableSize::Bit_0 ) );
			nLiveVal = ( g_MemManager.ActiveBankRAMByteRead( m_nVal ) & nBitmask ) != 0;
		}
		else if( m_nVarSize == ComparisonVariableSize::Nibble_Lower )
		{
			nLiveVal = g_MemManager.ActiveBankRAMByteRead( m_nVal ) & 0xf;
		}
		else if( m_nVarSize == ComparisonVariableSize::Nibble_Upper )
		{
			nLiveVal = ( g_MemManager.ActiveBankRAMByteRead( m_nVal ) >> 4 ) & 0xf;
		}
		else if( m_nVarSize == ComparisonVariableSize::EightBit )
		{
			nLiveVal = g_MemManager.ActiveBankRAMByteRead( m_nVal );
		}
		else if( m_nVarSize == ComparisonVariableSize::SixteenBit )
		{
			nLiveVal = g_MemManager.ActiveBankRAMByteRead( m_nVal );
			nLiveVal |= ( g_MemManager.ActiveBankRAMByteRead( m_nVal + 1 ) << 8 );
		}
		else if( m_nVarSize == ComparisonVariableSize::ThirtyTwoBit )
		{
			nLiveVal = g_MemManager.ActiveBankRAMByteRead( m_nVal );
			nLiveVal |= ( g_MemManager.ActiveBankRAMByteRead( m_nVal + 1 ) << 8 );
			nLiveVal |= ( g_MemManager.ActiveBankRAMByteRead( m_nVal + 2 ) << 16 );
			nLiveVal |= ( g_MemManager.ActiveBankRAMByteRead( m_nVal + 3 ) << 24 );
		}

		if( m_nVarType == ComparisonVariableType::DeltaMem )
		{
			//	Return the backed up (last frame) value, but store the new one for the next frame!
			m_nPreviousVal = nLiveVal;
			return nPreviousVal;
		}
		else
		{
			return nLiveVal;
		}
	}
	else
	{
		//	Panic!
		ASSERT( !"Undefined mem type!" );
		return 0;
	}
}

ComparisonType ReadOperator( char*& pBufferInOut )
{
	if( pBufferInOut[ 0 ] == '=' && pBufferInOut[ 1 ] == '=' )
	{
		pBufferInOut += 2; 
		return ComparisonType::Equals;
	}
	else if( pBufferInOut[ 0 ] == '=' )
	{
		pBufferInOut += 1;
		return ComparisonType::Equals;
	}
	else if( pBufferInOut[ 0 ] == '!' && pBufferInOut[ 1 ] == '=' )
	{
		pBufferInOut += 2;
		return ComparisonType::NotEqualTo;
	}
	else if( pBufferInOut[ 0 ] == '<' && pBufferInOut[ 1 ] == '=' )
	{
		pBufferInOut += 2;
		return ComparisonType::LessThanOrEqual;
	}
	else if( pBufferInOut[ 0 ] == '<' )
	{
		pBufferInOut += 1;
		return ComparisonType::LessThan;
	}
	else if( pBufferInOut[ 0 ] == '>' && pBufferInOut[ 1 ] == '=' )
	{
		pBufferInOut += 2;
		return ComparisonType::GreaterThanOrEqual;
	}
	else if( pBufferInOut[ 0 ] == '>' )
	{
		pBufferInOut += 1;
		return ComparisonType::GreaterThan;
	}
	else
	{
		ASSERT( !"Could not parse?!" );
		return ComparisonType::Equals;
	}
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
	switch( m_nCompareType )
	{
	case Equals:
		return( m_nCompSource.GetValue() == m_nCompTarget.GetValue() );
	case LessThan:
		return( m_nCompSource.GetValue() < m_nCompTarget.GetValue() );
	case LessThanOrEqual:
		return( m_nCompSource.GetValue() <= m_nCompTarget.GetValue() );
	case GreaterThan:
		return( m_nCompSource.GetValue() > m_nCompTarget.GetValue() );
	case GreaterThanOrEqual:
		return( m_nCompSource.GetValue() >= m_nCompTarget.GetValue() );
	case NotEqualTo:
		return( m_nCompSource.GetValue() != m_nCompTarget.GetValue() );
	default:
		return true;	//?
	}
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