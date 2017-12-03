#pragma once

#include "RA_Condition.h"

class MemCandidate
{
public:
	MemCandidate()
	 :	m_nAddr( 0 ),
		m_nLastKnownValue( 0 ),
		m_bUpperNibble( FALSE )
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
		BankData() 
		 :	Reader( nullptr ), Writer( nullptr ), BankSize( 0 ) 
		{}

		BankData( _RAMByteReadFn* pReadFn, _RAMByteWriteFn* pWriteFn, size_t nBankSize )
		 :	Reader( pReadFn ), Writer( pWriteFn ), BankSize( nBankSize )
		{}
		
	private:
		//	Copying disabled
		BankData( const BankData& );
		BankData& operator=( BankData& );
	
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
	size_t NumMemoryBanks() const									{ return m_Banks.size(); }

	void Reset( unsigned short nSelectedMemBank, ComparisonVariableSize nNewComparisonVariableSize );
	void ResetAll( ComparisonVariableSize nNewComparisonVariableSize, ByteAddress start, ByteAddress end );

	size_t Compare( ComparisonType nCompareType, unsigned int nTestValue, bool& bResultsFound );
	
	inline DWORD ValidMemAddrFound( size_t iter ) const				{ return m_Candidates[ iter ].m_nAddr; }
	inline ComparisonVariableSize MemoryComparisonSize() const		{ return m_nComparisonSizeMode; }
	inline bool UseLastKnownValue() const							{ return m_bUseLastKnownValue; }
	inline void SetUseLastKnownValue( bool bUseLastKnownValue )		{ m_bUseLastKnownValue = bUseLastKnownValue; }
	inline size_t BankSize( unsigned short nBank ) const			{ return m_Banks.at( nBank ).BankSize; }
	//inline size_t ActiveBankSize() const							{ return m_Banks.at( m_nActiveMemBank ).BankSize; }
	//inline unsigned short ActiveBankID() const					{ return m_nActiveMemBank; }
	inline size_t TotalBankSize() const								{ return m_nTotalBankSize; }

	std::vector<size_t> GetBankIDs() const;
	
	size_t NumCandidates() const									{ return m_nNumCandidates; }
	const MemCandidate& GetCandidate( size_t nAt ) const			{ return m_Candidates[ nAt ]; }

	void ChangeActiveMemBank( unsigned short nMemBank );

	unsigned char ActiveBankRAMByteRead(ByteAddress nOffs) const;
	void ActiveBankRAMByteWrite(ByteAddress nOffs, unsigned int nVal);

	void ActiveBankRAMRead(unsigned char buffer[], ByteAddress nOffs, size_t count) const;

private:
	std::map<size_t, BankData> m_Banks;
	unsigned short m_nActiveMemBank;

	MemCandidate* m_Candidates;		//	Pointer to an array
	size_t m_nNumCandidates;		//	Actual quantity of legal candidates

	ComparisonVariableSize m_nComparisonSizeMode;
	bool m_bUseLastKnownValue;
	size_t m_nTotalBankSize;
};

extern MemManager g_MemManager;
