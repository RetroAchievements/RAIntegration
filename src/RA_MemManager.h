#pragma once

#include "RA_Condition.h"

struct MemCandidate
{
	// If you can use 0 constructors, that's the best. Just use in-class initilizers
	// Was this supposed be "ByteAddress"?
	std::size_t m_nAddr{ 0_z };
	std::size_t m_nLastKnownValue{ 0_z }; //	A Candidate MAY be a 32-bit candidate!
	bool m_bUpperNibble{ false };		  //	Used only for 4-bit comparisons
	bool m_bHasChanged{ false };
};

// TODO: We should try to make these function objects, or a function type, not sure the intention here
// It's gonna have the same semantics for now it just looks weird
// Note: It's still a function pointer

using _RAMByteReadFn  = std::add_pointer_t<unsigned char(unsigned int nOffs)>;
using _RAMByteWriteFn = std::add_pointer_t<void(unsigned int nOffs, unsigned int nVal)>;

// lets check
using test_type = unsigned char(*)(unsigned int);
static_assert(std::is_same_v<test_type, _RAMByteReadFn>);

// Ok the other one
using test_type2 = void(*)(unsigned int, unsigned int);
static_assert(std::is_same_v<test_type2, _RAMByteWriteFn>);

class MemManager
{
private:
	class BankData
	{
	public:
		~BankData() noexcept {
			// got a access violation before
			Reader   = nullptr;
			Writer   = nullptr;
			BankSize = 0_z;
		}

		BankData( _RAMByteReadFn* pReadFn, _RAMByteWriteFn* pWriteFn, size_t nBankSize ) noexcept :
			Reader{ pReadFn }, Writer{ pWriteFn }, BankSize{ nBankSize } {}
		
		BankData( const BankData& ) = delete;
		BankData& operator=( BankData& ) = delete;
		// Moving invalidates pointers, meaning you cannot use the right side again
		BankData(BankData&&) noexcept = default;
		BankData& operator=(BankData&&) noexcept = default;
		// parameterless constructor must be at the end or it will delete the rest
		BankData() noexcept = default;
	public:
		_RAMByteReadFn * Reader{ nullptr };
		_RAMByteWriteFn* Writer{ nullptr };
		size_t BankSize{ 0_z };
	};

public:
	
	// unless this is inherited, making this virtual just wastes space
	~MemManager() noexcept;

	// can't be noexcept because of std::map, but that's fine, just handle it when it's thrown
	MemManager() = default;
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

	inline void ChangeNumCandidates( unsigned int size )			{ m_nNumCandidates = size; }
	MemCandidate* GetCandidatePointer()								{ return m_Candidates; }

	void ChangeActiveMemBank( unsigned short nMemBank );

	unsigned char ActiveBankRAMByteRead(ByteAddress nOffs) const;
	void ActiveBankRAMByteWrite(ByteAddress nOffs, unsigned int nVal);

	unsigned int ActiveBankRAMRead(ByteAddress nOffs, ComparisonVariableSize size) const;

	void ActiveBankRAMRead(unsigned char buffer[], ByteAddress nOffs, size_t count) const;

private:
	std::map<size_t, BankData> m_Banks;
	unsigned short m_nActiveMemBank{ 0_hu };

	MemCandidate* m_Candidates{nullptr};		//	Pointer to an array
	size_t m_nNumCandidates{ 0_z };		//	Actual quantity of legal candidates

	ComparisonVariableSize m_nComparisonSizeMode{ ComparisonVariableSize::SixteenBit };
	bool m_bUseLastKnownValue{ true };
	size_t m_nTotalBankSize{ 0_z };
};

extern MemManager g_MemManager;
