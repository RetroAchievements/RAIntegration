#pragma once

#include "RA_Condition.h"

#ifdef __cplusplus
extern "C" {
#endif

class MemCandidate
{
public:
	MemCandidate()
	 :	m_nAddr( 0 ),
		m_nLastKnownValue( 0 )
	{}
	
public:
	unsigned int m_nAddr;
	unsigned int m_nLastKnownValue;		//	A Candidate MAY be a 32-bit candidate!
	bool m_bUpperNibble;				//	Used only for 4-bit comparisons
};

typedef unsigned char (_RAMByteReadFn)( unsigned int nOffs );
typedef void (_RAMByteWriteFn)( unsigned int nOffs, unsigned int nVal );

class MemManager
{
private:
	class BankData
	{
	public:
		BankData( _RAMByteReadFn* pReadFn, _RAMByteWriteFn* pWriteFn, size_t nBankSize )
		 :	Reader( pReadFn ), Writer( pWriteFn ), BankSize( nBankSize )
		{}
		
	public:
		_RAMByteReadFn* Reader;
		_RAMByteWriteFn* Writer;
		size_t BankSize;
	};

public:
	MemManager();
	virtual ~MemManager();

public:
	void ClearMemoryBanks();
	void AddMemoryBank( size_t nBankID, _RAMByteReadFn* pReader, _RAMByteWriteFn* pWriter, size_t nBankSize );

	void Reset( unsigned short nSelectedMemBank );

	size_t Compare( ComparisonType nCompareType, unsigned int nTestValue, bool& bResultsFound );
	
	inline DWORD ValidMemAddrFound( size_t iter ) const				{ return m_Candidates[ iter ].m_nAddr; }
	inline ComparisonVariableSize MemoryComparisonSize() const		{ return m_nComparisonSizeMode; }
	inline bool UseLastKnownValue() const							{ return m_bUseLastKnownValue; }
	inline void SetUseLastKnownValue( bool bUseLastKnownValue )		{ m_bUseLastKnownValue = bUseLastKnownValue; }
	inline size_t BankSize( unsigned short nBank ) const			{ return m_Banks.at( nBank ).BankSize; }
	//inline unsigned short ActiveBankID() const						{ return m_nActiveMemBank; }
	inline size_t ActiveBankSize() const							{ return m_Banks.at( m_nActiveMemBank ).BankSize; }
	
	inline unsigned char ActiveBankRAMByteRead( unsigned int nOffs ) const								{ return( ( *( m_Banks.at( m_nActiveMemBank ).Reader ) )( nOffs ) ); }
	inline void ActiveBankRAMByteWrite( unsigned int nOffs, unsigned int nVal ) const					{ ( *( m_Banks.at( m_nActiveMemBank ).Writer ) )( nOffs, nVal ); }

	inline unsigned char RAMByte( unsigned short nBankID, unsigned int nOffs ) const					{ return( ( *( m_Banks.at( nBankID ).Reader ) )( nOffs ) ); }
	inline void RAMByteWrite( unsigned short nBankID, unsigned int nOffs, unsigned int nVal ) const		{ ( *( m_Banks.at( nBankID ).Writer ) )( nOffs, nVal ); }

	void PokeByte( unsigned int nAddr, unsigned int nValue );

private:
	std::map<unsigned short, BankData> m_Banks;
	unsigned short m_nActiveMemBank;

	MemCandidate* m_Candidates;						//	Pointer to an array
	size_t m_nNumCandidates;						//	Actual quantity of candidates

	ComparisonVariableSize m_nComparisonSizeMode;
	bool m_bUseLastKnownValue;
};

extern MemManager g_MemManager;


//	Move to game-side!!!
//
//unsigned char RAMByteStandard( unsigned int nOffs )
//{
//	return g_MemManager.
//	//if( nOffs >= g_MemManager.RAMTotalSize() )
//	//	return 0;
//	//
//	//return (nOffs<g_MemManager.m_nCoreRAMSize) ? *(g_MemManager.m_pCoreRAM+nOffs) : (nOffs<g_MemManager.RAMTotalSize()) ? *(g_MemManager.m_pRAMExtra+(nOffs-g_MemManager.m_nCoreRAMSize)) : 0;
//}
//
////	Alternative for GBC
//unsigned char RAMByteGBC( unsigned int nOffs )
//{
//	if( nOffs >= g_MemManager.RAMTotalSize() )
//		return 0;
//
//	return ( (unsigned char**)g_MemManager.m_pCoreRAM)[(nOffs) >> 12][(nOffs) & 0xfff];
//}
//
//void RAMByteStandardWrite( unsigned int nOffs, unsigned int nVal )
//{
//	if( nOffs >= g_MemManager.RAMTotalSize() )
//		return;
//
//	if( nOffs < g_MemManager.m_nCoreRAMSize )
//	{
//		*( g_MemManager.m_pCoreRAM+nOffs ) = nVal;
//	}
//	else
//	{
//		nOffs -= g_MemManager.m_nCoreRAMSize;
//		*( g_MemManager.m_pRAMExtra+nOffs ) = nVal;
//	}
//}
//
////	Alternative for GBC
//void RAMByteGBCWrite( unsigned int nOffs, unsigned int nVal )
//{
//	if( nOffs >= g_MemManager.RAMTotalSize() )
//		return;
//
//	( (unsigned char**)g_MemManager.m_pCoreRAM)[(nOffs) >> 12][(nOffs) & 0xfff] = nVal;
//}
//
//typedef void (*writefunc)( unsigned int A, unsigned char V );
//typedef unsigned char (*readfunc)( unsigned int A );
//
//unsigned char RAMByteNES( unsigned int nOffs )
//{
//	//	g_MemManager.m_pCoreRAM is an array of readfunc function ptrs which we cast to, THEN dereference, then call
//	return ( ( (readfunc*)( g_MemManager.m_pCoreRAM ) )[nOffs] )(nOffs);
//}
//
//void RAMByteNESWrite( unsigned int nOffs, unsigned int nVal )
//{
//	//	g_MemManager.m_pRAMExtra is an array of writefunc function ptrs which we cast to, THEN dereference, then call
//	( (writefunc*)( g_MemManager.m_pRAMExtra ) )[nOffs]( nOffs, nVal );
//}

//void RAMByteWriteStandard( unsigned int nOffs, unsigned int nVal );
//void RAMByteWriteGBC( unsigned int nOffs, unsigned int nVal );

//unsigned short MemManager_ValidMemAddrFound( int iter );

#ifdef __cplusplus
};
#endif
