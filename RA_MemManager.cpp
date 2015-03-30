#include "RA_MemManager.h"

#include "RA_Core.h"
#include "RA_Achievement.h"

MemManager g_MemManager;

MemManager::MemManager()
 :	m_nComparisonSizeMode( ComparisonVariableSize::SixteenBit ),
	m_bUseLastKnownValue( true ),
	m_Candidates( nullptr )
{
}

//	virtual
MemManager::~MemManager()
{
	ClearMemoryBanks();
}

void MemManager::ClearMemoryBanks()
{
	m_Banks.clear();
	if( m_Candidates != nullptr )
	{
		delete[] m_Candidates;
		m_Candidates = nullptr;
	}
}

void MemManager::AddMemoryBank( size_t nBankID, _RAMByteReadFn* pReader, _RAMByteWriteFn* pWriter, size_t nBankSize )
{
	if( m_Banks.find( nBankID ) != m_Banks.end() )
	{
		ASSERT( !"Failed! Bank already added!" );
	}
	
	m_Banks[ nBankID ] = BankData( pReader, pWriter, nBankSize );
}

void MemManager::Reset( unsigned short nSelectedMemBank )
{
	//	RAM must be installed for this to work!
	if( m_Banks.size() == 0 )
		return;

	if( m_Banks.find( nSelectedMemBank ) == m_Banks.end() )
		return;

	m_nActiveMemBank = nSelectedMemBank;

	const size_t RAM_SIZE = m_Banks[ m_nActiveMemBank ].BankSize;

	//	Initialize the memory cache: i.e. every memory address is valid!
	if( ( m_nComparisonSizeMode == ComparisonVariableSize::Nibble_Lower ) || ( m_nComparisonSizeMode == ComparisonVariableSize::Nibble_Upper ) )
	{
		for( size_t i = 0; i < RAM_SIZE; ++i )
		{
			m_Candidates[ ( i * 2 ) ].m_nAddr = i;
			m_Candidates[ ( i * 2 ) ].m_bUpperNibble = false;			//lower first?
			m_Candidates[ ( i * 2 ) ].m_nLastKnownValue = ( RAMByte( m_nActiveMemBank, i ) & 0xf );

			m_Candidates[ ( i * 2 ) + 1 ].m_nAddr = i;
			m_Candidates[ ( i * 2 ) + 1 ].m_bUpperNibble = true;
			m_Candidates[ ( i * 2 ) + 1 ].m_nLastKnownValue = ( ( RAMByte( m_nActiveMemBank, i ) >> 4 ) & 0xf );
		}
		m_nNumCandidates = RAM_SIZE * 2;
	}
	else if( m_nComparisonSizeMode == ComparisonVariableSize::EightBit )
	{
		for( DWORD i = 0; i < RAM_SIZE; ++i )
		{
			m_Candidates[ i ].m_nAddr = i;
			m_Candidates[ i ].m_nLastKnownValue = RAMByte( m_nActiveMemBank, i );
		}
		m_nNumCandidates = RAM_SIZE;
	}
	else if( m_nComparisonSizeMode == ComparisonVariableSize::SixteenBit )
	{
		for( DWORD i = 0; i < ( RAM_SIZE / 2 ); ++i )
		{
			m_Candidates[ i ].m_nAddr = i*2;
			m_Candidates[ i ].m_nLastKnownValue = ( RAMByte( m_nActiveMemBank, i * 2 ) ) | ( RAMByte( m_nActiveMemBank, ( ( i * 2 ) + 1 ) << 8 ) );
		}
		m_nNumCandidates = RAM_SIZE / 2;
	}
// 	else if( m_nComparisonSizeMode == CMP_SZ_32BIT )
// 	{
// 		//	Assuming 32-bit-aligned!
// 
// 		for( unsigned int i = 0; i < (MemorySize/4); ++i )
// 		{
// 			m_Candidates[ i ].m_nAddr = (unsigned short)i*4;
// 			m_Candidates[ i ].m_nLastKnownValue = (Ram_68k[i] | (Ram_68k[i+1] << 8) | (Ram_68k[i+2] << 16) | (Ram_68k[i+3] << 24));
// 		}
// 		m_nNumCandidates = MemorySize/4;
// 	}
}

size_t MemManager::Compare( ComparisonType nCompareType, unsigned int nTestValue, bool& bResultsFound )
{
	size_t nGoodResults = 0;
	MemCandidate* pResults = new MemCandidate[ m_nNumCandidates ];
	if( pResults == NULL )
	{
		char buffer[ 1024 ];
		sprintf_s( buffer, 1024, "Could not allocate %d KB for comparison in RA MemManager!", ( m_nNumCandidates ) / 1024 );
		MessageBox( NULL, buffer, "Error in allocation!", MB_OK );
		return 0;
	}
	
	for( size_t i = 0; i < m_nNumCandidates; ++i )
	{
		unsigned int nAddr = m_Candidates[ i ].m_nAddr;
		unsigned int nLiveValue = 0;

		if( ( m_nComparisonSizeMode == ComparisonVariableSize::Nibble_Lower ) || 
			( m_nComparisonSizeMode == ComparisonVariableSize::Nibble_Upper ) )
		{
			if( m_Candidates[ i ].m_bUpperNibble )
				nLiveValue = ( RAMByte( m_nActiveMemBank, nAddr ) >> 4 ) & 0xf;
			else
				nLiveValue = RAMByte( m_nActiveMemBank, nAddr ) & 0xf;
		}
		else if( m_nComparisonSizeMode == ComparisonVariableSize::EightBit )
		{
			nLiveValue = RAMByte( m_nActiveMemBank, nAddr );
		}
		else if( m_nComparisonSizeMode == ComparisonVariableSize::SixteenBit )
		{
			nLiveValue = RAMByte( m_nActiveMemBank, nAddr );
			nLiveValue |= ( RAMByte( m_nActiveMemBank, nAddr + 1 ) << 8 );
		}
 		else if( m_nComparisonSizeMode == ComparisonVariableSize::ThirtyTwoBit )
 		{
			nLiveValue = RAMByte( m_nActiveMemBank, nAddr );
			nLiveValue |= ( RAMByte( m_nActiveMemBank, nAddr + 1 ) << 8 );
			nLiveValue |= ( RAMByte( m_nActiveMemBank, nAddr + 2 ) << 16 );
			nLiveValue |= ( RAMByte( m_nActiveMemBank, nAddr + 3 ) << 24 );
 		}
		
		bool bValid = false;
		unsigned int nQueryValue = ( m_bUseLastKnownValue ? m_Candidates[ i ].m_nLastKnownValue : nTestValue );
		switch( nCompareType )
		{
		case Equals:				bValid = ( nLiveValue == nQueryValue );	break;
		case LessThan:				bValid = ( nLiveValue < nQueryValue );	break;
		case LessThanOrEqual:		bValid = ( nLiveValue <= nQueryValue );	break;
		case GreaterThan:			bValid = ( nLiveValue > nQueryValue );	break;
		case GreaterThanOrEqual:	bValid = ( nLiveValue >= nQueryValue );	break;
		case NotEqualTo:			bValid = ( nLiveValue != nQueryValue );	break;
		default:
			ASSERT( !"Unknown comparison type?!" );
			break;
		}

		//	If the current address in ram still matches the query, store in result[]
		if( bValid )
		{
			pResults[ nGoodResults ].m_nLastKnownValue = nLiveValue;
			pResults[ nGoodResults ].m_nAddr = m_Candidates[ i ].m_nAddr;
			pResults[ nGoodResults ].m_bUpperNibble = m_Candidates[ i ].m_bUpperNibble;
			nGoodResults++;
		}
	}
	
	bResultsFound = ( nGoodResults > 0 );
	if( bResultsFound )
	{
		m_nNumCandidates = nGoodResults;
		for( size_t i = 0; i < m_nNumCandidates; ++i )
		{
			m_Candidates[ i ].m_nAddr = pResults[ i ].m_nAddr;
			m_Candidates[ i ].m_nLastKnownValue = pResults[ i ].m_nLastKnownValue;
			m_Candidates[ i ].m_bUpperNibble = pResults[ i ].m_bUpperNibble;
		}
	}

	delete[] pResults;
	return m_nNumCandidates;
}

//void MemManager::InstallRAM( unsigned char* pRAM, unsigned int nRAMBytes, unsigned char* pRAMExtra, unsigned int nRAMExtraBytes )
//{
//	if( g_ConsoleID == 4 || g_ConsoleID == 6 )
//	{
//		//	GB/GBC override
// 		m_pfnRAMByteRead  = RAMByteGBC;
// 		m_pfnRAMByteWrite = RAMByteGBCWrite;
//	}
//	else if( g_ConsoleID == 7 )
//	{
//		//	NES override
//		m_pfnRAMByteRead  = RAMByteNES;
//		m_pfnRAMByteWrite = RAMByteNESWrite;
//	}
//	else
//	{
//		m_pfnRAMByteRead  = RAMByteStandard;
//		m_pfnRAMByteWrite = RAMByteStandardWrite;
//	}
//	
//	m_pCoreRAM = pRAM;
//	m_nCoreRAMSize = nRAMBytes;
//
//	m_pRAMExtra = pRAMExtra;
//	m_nRAMExtraSize = nRAMExtraBytes;
//
//	if( m_Candidates != NULL )
//	{
//		delete[] m_Candidates;
//		m_Candidates = NULL;
//	}
//
//	m_Candidates = new MemCandidate[ RAMTotalSize()*2 ];	//	Doubled so we can inspect nibbles!
//	if( m_Candidates == NULL )
//	{
//		char buffer[ 1024 ];
//		sprintf_s( buffer, 1024, "Could not allocate %d KB for RA MemManager!", RAMTotalSize()/1024 );
//		MessageBox( NULL, buffer, "Error in allocation!", MB_OK );
//	}
//
//	Reset();
//}
