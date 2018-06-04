#pragma once

#include "RA_Condition.h"

struct MemCandidate
{
    unsigned int m_nAddr{ 0U };
    unsigned int m_nLastKnownValue{ 0U };		//	A Candidate MAY be a 32-bit candidate!
    bool m_bUpperNibble{ false };				//	Used only for 4-bit comparisons
    bool m_bHasChanged{ false };
};

typedef unsigned char (_RAMByteReadFn)(unsigned int nOffs);
typedef void (_RAMByteWriteFn)(unsigned int nOffs, unsigned int nVal);

class MemManager
{
private:
    class BankData
    {
    public:
        // This should give NRVO ("named return value optimization"), since not
        // all constructors are defaulted you have to mark them constexpr
        // explicitly
        inline constexpr BankData() noexcept = default;
        ~BankData() noexcept = default;

        inline constexpr BankData(_RAMByteReadFn* pReadFn, _RAMByteWriteFn* pWriteFn, size_t nBankSize) noexcept :
            Reader{ pReadFn },
            Writer{ pWriteFn },
            BankSize{ nBankSize }
        {
        }
        
        // If you are trying to use copy epsilon technique we should delete
        // moving as well but we may need it for unique_ptr later. Deleting
        // copying should give it sufficient optimization.

        //	Copying deleted
        BankData(const BankData&) = delete;
        BankData& operator=(BankData&) = delete;
        inline constexpr BankData(BankData&&) noexcept = default;
        inline constexpr BankData& operator=(BankData&&) noexcept = default;

    public:
        _RAMByteReadFn* Reader{ nullptr };
        _RAMByteWriteFn* Writer{ nullptr };
        size_t BankSize{ std::size_t{} };
    };

public:
    // Default constructors aren't allowed to throw but the default does so we have to define it
    MemManager() noexcept; 

    MemManager(const MemManager&) = delete;
    MemManager& operator=(MemManager&) = delete;
    MemManager(MemManager&&) = delete;  // might throw but there's a global so moving is unnecessary for now
    MemManager& operator=(MemManager&&) noexcept = delete;
    ~MemManager() noexcept;
public:
    void ClearMemoryBanks();
    void AddMemoryBank(size_t nBankID, _RAMByteReadFn* pReader, _RAMByteWriteFn* pWriter, size_t nBankSize);
    size_t NumMemoryBanks() const { return m_Banks.size(); }

    void Reset(unsigned short nSelectedMemBank, ComparisonVariableSize nNewComparisonVariableSize);
    void ResetAll(ComparisonVariableSize nNewComparisonVariableSize, ByteAddress start, ByteAddress end);

    size_t Compare(ComparisonType nCompareType, unsigned int nTestValue, bool& bResultsFound);

    inline DWORD ValidMemAddrFound(size_t iter) const { return m_Candidates[iter].m_nAddr; }
    inline ComparisonVariableSize MemoryComparisonSize() const { return m_nComparisonSizeMode; }
    inline bool UseLastKnownValue() const { return m_bUseLastKnownValue; }
    inline void SetUseLastKnownValue(bool bUseLastKnownValue) { m_bUseLastKnownValue = bUseLastKnownValue; }
    inline size_t BankSize(unsigned short nBank) const { return m_Banks.at(nBank).BankSize; }
    //inline size_t ActiveBankSize() const							{ return m_Banks.at( m_nActiveMemBank ).BankSize; }
    //inline unsigned short ActiveBankID() const					{ return m_nActiveMemBank; }
    inline size_t TotalBankSize() const
    {
        return m_nTotalBankSize;
    }

    std::vector<size_t> GetBankIDs() const;

    size_t NumCandidates() const { return m_nNumCandidates; }
    const MemCandidate& GetCandidate(size_t nAt) const { return m_Candidates[nAt]; }

    inline void ChangeNumCandidates(unsigned int size) { m_nNumCandidates = size; }
    MemCandidate* GetCandidatePointer() { return m_Candidates.get(); }

    void ChangeActiveMemBank(unsigned short nMemBank);

    unsigned char ActiveBankRAMByteRead(ByteAddress nOffs) const;
    void ActiveBankRAMByteWrite(ByteAddress nOffs, unsigned int nVal);

    unsigned int ActiveBankRAMRead(ByteAddress nOffs, ComparisonVariableSize size) const;

    void ActiveBankRAMRead(unsigned char buffer[], ByteAddress nOffs, size_t count) const;

private:
    std::map<size_t, BankData> m_Banks;
    unsigned short m_nActiveMemBank = unsigned short{};

    //	Pointer to an array
    std::unique_ptr<MemCandidate[]> m_Candidates;
    size_t m_nNumCandidates = std::size_t{};		//	Actual quantity of legal candidates

    ComparisonVariableSize m_nComparisonSizeMode{ ComparisonVariableSize::SixteenBit };
    bool m_bUseLastKnownValue{ true };
    size_t m_nTotalBankSize = std::size_t{};
};

extern MemManager g_MemManager;
