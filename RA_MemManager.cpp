#include "RA_MemManager.h"

#include <windows.h>
#include <assert.h>
#include <stdio.h>

#include "RA_Core.h"
#include "RA_Achievement.h"


MemManager g_MemManager;


MemManager::MemManager()
{
	m_pfnRAMByteRead = RAMByteStandard;
	m_pfnRAMByteWrite = RAMByteStandardWrite;
	Init();
}

MemManager::~MemManager()
{
	if( m_Candidates != NULL )
	{
		delete [] m_Candidates;
		m_Candidates = NULL;
	}
}

void MemManager::Init( )
{
	m_nComparisonSizeMode = CMP_SZ_8BIT;
	m_bUseLastKnownValue = TRUE;
	m_pCoreRAM = NULL;
	m_pRAMExtra = NULL;
	m_nCoreRAMSize = 0;
	m_nRAMExtraSize = 0;
	m_hMemDialog = NULL;
	m_Candidates = NULL;
}

void MemManager::Reset( /*ComparisonSize*/int cmpSize )
{
	DWORD i = 0;

	if( m_pCoreRAM == NULL )
	{
		//assert(0);
		return;
	}

	m_nComparisonSizeMode = cmpSize;

	//	Initialize the memory cache: i.e. every memory address is valid!

	if( m_nComparisonSizeMode == CMP_SZ_4BIT_LOWER || m_nComparisonSizeMode == CMP_SZ_4BIT_UPPER )
	{
		for( i = 0; i < RAMTotalSize(); ++i )
		{
			m_Candidates[ (i*2) ].m_nAddr = i;
			m_Candidates[ (i*2) ].m_bUpperNibble = FALSE;			//lower first?
			m_Candidates[ (i*2) ].m_nLastKnownValue = RAMByte( i )&0xf;

			m_Candidates[ (i*2)+1 ].m_nAddr = i;
			m_Candidates[ (i*2)+1 ].m_bUpperNibble = TRUE;
			m_Candidates[ (i*2)+1 ].m_nLastKnownValue = (RAMByte( i )>>4)&0xf;
		}
		m_NumCandidates = RAMTotalSize()*2;
	}
	else if( m_nComparisonSizeMode == CMP_SZ_8BIT )
	{
		for( i = 0; i < RAMTotalSize(); ++i )
		{
			m_Candidates[ i ].m_nAddr = i;
			m_Candidates[ i ].m_nLastKnownValue = RAMByte( i );
		}
		m_NumCandidates = RAMTotalSize();
	}
	else if( m_nComparisonSizeMode == CMP_SZ_16BIT )
	{
		for( i = 0; i < (RAMTotalSize()/2); ++i )
		{
			m_Candidates[ i ].m_nAddr = i*2;

			//unsigned int nOffs = (i<m_nCoreRAMSize) ? i : i-m_nCoreRAMSize;
			//m_Candidates[ i ].m_nLastKnownValue = (m_pCoreRAM[i*2] | (m_pCoreRAM[(i*2)+1] << 8));
			//m_Candidates[ i ].m_nLastKnownValue = (i*2<m_nCoreRAMSize) ? 
			//	(m_pCoreRAM[nOffs*2] | (m_pCoreRAM[(nOffs*2)+1] << 8)) : 
			//	(m_pRAMExtra[nOffs*2] | (m_pRAMExtra[(nOffs*2)+1] << 8));
			m_Candidates[i].m_nLastKnownValue = RAMByte( i*2 ) | (RAMByte( (i*2)+1 ) << 8 );
		}
		m_NumCandidates = RAMTotalSize()/2;
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
// 		m_NumCandidates = MemorySize/4;
// 	}

}

unsigned int MemManager::Compare( unsigned int nCompareType, unsigned int nTestValue, BOOL& bResultsFound )
{
	unsigned int i = 0;
	unsigned int nGoodResults = 0;
	MemCandidate* pResults = new MemCandidate[m_NumCandidates];

	if( pResults == NULL )
	{
		char buffer[1024];
		sprintf_s( buffer, 1024, "Could not allocate %d KB for comparison in RA MemManager!", (m_NumCandidates)/1024 );
		MessageBox( NULL, buffer, "Error in allocation!", MB_OK );
		return 0;
	}
	
	for( i = 0; i < m_NumCandidates; ++i )
	{
		unsigned int nNextAddr = m_Candidates[ i ].m_nAddr;
		BOOL bUpperNibble = m_Candidates[ i ].m_bUpperNibble;
		unsigned int nLiveVal = 0;
		BOOL bValid = FALSE;

		unsigned int nQuery = nTestValue;
		if( m_bUseLastKnownValue )
			nQuery = (unsigned int)m_Candidates[i].m_nLastKnownValue;


		if( m_nComparisonSizeMode == CMP_SZ_4BIT_LOWER || m_nComparisonSizeMode == CMP_SZ_4BIT_UPPER )
		{
			if( bUpperNibble )
				nLiveVal = (RAMByte( nNextAddr )>>4)&0xf;
			else
				nLiveVal = RAMByte( nNextAddr )&0xf;
		}
		else if( m_nComparisonSizeMode == CMP_SZ_8BIT )
		{
			nLiveVal = RAMByte( nNextAddr );
		}
		else if( m_nComparisonSizeMode == CMP_SZ_16BIT )
		{
			nLiveVal = RAMByte( nNextAddr );
			nLiveVal |= (RAMByte( nNextAddr+1 ) << 8);
		}
// 		else if( m_nComparisonSizeMode == CMP_SZ_32BIT )
// 		{
// 			nLiveVal |= (m_pCoreRAM[ nNextAddr+1 ] << 8);
// 			nLiveVal |= (m_pCoreRAM[ nNextAddr+2 ] << 16);	//	TBD: Check this is the right order!
// 			nLiveVal |= (m_pCoreRAM[ nNextAddr+3 ] << 24);
// 		}

		switch( nCompareType )
		{
		case CMP_EQ:
			bValid = (nLiveVal == nQuery); break;
		case CMP_LT:
			bValid = (nLiveVal < nQuery); break;
		case CMP_LTE:
			bValid = (nLiveVal <= nQuery); break;
		case CMP_GT:
			bValid = (nLiveVal > nQuery); break;
		case CMP_GTE:
			bValid = (nLiveVal >= nQuery); break;
		case CMP_NEQ:
			bValid = (nLiveVal != nQuery); break;
		default:
			break;
		}

		//	If the current address in ram still matches the query, store in result[]
		if( bValid )
		{
			pResults[nGoodResults].m_nAddr = nNextAddr;
			pResults[nGoodResults].m_nLastKnownValue = nLiveVal;
			pResults[nGoodResults].m_bUpperNibble = bUpperNibble;
			nGoodResults++;
		}
	}

	if( nGoodResults == 0 )
	{
		//MessageBox( NULL, "No results found: backing up old values!", "Warning!", MB_OK );
		//MessageBox( m_hMemDialog, "No results found: backing up old values!", "Warning!", MB_OK );
		bResultsFound = FALSE;
	}
	else
	{
		m_NumCandidates = nGoodResults;

		for( i = 0; i < m_NumCandidates; ++i )
		{
			m_Candidates[i].m_nAddr = pResults[i].m_nAddr;
			m_Candidates[i].m_nLastKnownValue = pResults[i].m_nLastKnownValue;
			m_Candidates[i].m_bUpperNibble = pResults[i].m_bUpperNibble;
		}

		bResultsFound = TRUE;
	}

	delete[] pResults;
	return m_NumCandidates;
}

void MemManager::InstallRAM( unsigned char* pRAM, unsigned int nRAMBytes, unsigned char* pRAMExtra, unsigned int nRAMExtraBytes )
{
	if( g_ConsoleID == 4 || g_ConsoleID == 6 )
	{
		//	GB/GBC override
 		m_pfnRAMByteRead  = RAMByteGBC;
 		m_pfnRAMByteWrite = RAMByteGBCWrite;
	}
	else if( g_ConsoleID == 7 )
	{
		//	NES override
		m_pfnRAMByteRead  = RAMByteNES;
		m_pfnRAMByteWrite = RAMByteNESWrite;
	}
	else
	{
		m_pfnRAMByteRead  = RAMByteStandard;
		m_pfnRAMByteWrite = RAMByteStandardWrite;
	}
	
	
	m_pCoreRAM = pRAM;
	m_nCoreRAMSize = nRAMBytes;

	m_pRAMExtra = pRAMExtra;
	m_nRAMExtraSize = nRAMExtraBytes;

	if( m_Candidates != NULL )
	{
		delete [] m_Candidates;
		m_Candidates = NULL;
	}

	m_Candidates = new MemCandidate[ RAMTotalSize()*2 ];	//	Doubled so we can inspect nibbles!
	if( m_Candidates == NULL )
	{
		char buffer[1024];
		sprintf_s( buffer, 1024, "Could not allocate %d KB for RA MemManager!", RAMTotalSize()/1024 );
		MessageBox( NULL, buffer, "Error in allocation!", MB_OK );
	}

	Reset( CMP_SZ_8BIT );
}
