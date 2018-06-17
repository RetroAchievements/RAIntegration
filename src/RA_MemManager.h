#ifndef RA_MEMMANAGER_H
#define RA_MEMMANAGER_H
#pragma once

#include "RA_Condition.h"

#pragma pack(push, 1)
struct MemCandidate
{
    // These are move assigned instead of default constructed so we can see the default values

    unsigned int m_nAddr ={};

    /// <summary>
    ///   A <see cref="MemCandidate" /> MAY be a 32-bit candidate!
    /// </summary>
    unsigned int m_nLastKnownValue ={};


    /// <summary>Used only for 4-bit comparisons</summary>
    bool m_bUpperNibble ={};
    bool m_bHasChanged ={};

};
#pragma pack(pop)

typedef unsigned char (_RAMByteReadFn)(unsigned int nOffs);
typedef void (_RAMByteWriteFn)(unsigned int nOffs, unsigned int nVal);

#pragma pack(push, 2)
#pragma pack(push, 1)
class MemManager
{
private:
    struct BankData
    {
        // This is literally the same thing as the default constructor
#pragma warning(push)
        // these will be used, the current structure didn't follow RAII so the compiler things they weren't used
#pragma warning(disable : 4514) // unreferenced inline functions
        inline constexpr BankData() noexcept = default;
        inline constexpr BankData(BankData&&) noexcept = default;
        inline constexpr BankData& operator=(BankData&&) noexcept = default;
#pragma warning(pop)

        ~BankData() noexcept = default;
        // this made no sense, how were you storing the data? - Samer
        //	Copying disabled
        BankData(const BankData&) = delete;
        BankData& operator=(const BankData&) = delete;
        
        

        _RAMByteReadFn * Reader{ nullptr };
        _RAMByteWriteFn* Writer{ nullptr };
        size_t BankSize ={};
    };

public:
    virtual ~MemManager();

public:
    void ClearMemoryBanks();
    void AddMemoryBank(size_t nBankID, _RAMByteReadFn* pReader, _RAMByteWriteFn* pWriter, size_t nBankSize);

    // Getters and setters are useless
#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
    size_t NumMemoryBanks() const { return m_Banks.size(); }
    inline DWORD ValidMemAddrFound(size_t iter) const { return m_Candidates[iter].m_nAddr; }
    inline ComparisonVariableSize MemoryComparisonSize() const { return m_nComparisonSizeMode; }
    inline bool UseLastKnownValue() const { return m_bUseLastKnownValue; }
    inline void SetUseLastKnownValue(bool bUseLastKnownValue) { m_bUseLastKnownValue = bUseLastKnownValue; }
    inline size_t BankSize(unsigned short nBank) const { return m_Banks.at(nBank).BankSize; }
    //inline size_t ActiveBankSize() const							{ return m_Banks.at( m_nActiveMemBank ).BankSize; }
    //inline unsigned short ActiveBankID() const					{ return m_nActiveMemBank; }
    inline size_t TotalBankSize() const { return m_nTotalBankSize; }
    size_t NumCandidates() const { return m_nNumCandidates; }
    const MemCandidate& GetCandidate(size_t nAt) const { return m_Candidates[nAt]; }

    inline void ChangeNumCandidates(unsigned int size) { m_nNumCandidates = size; }
    MemCandidate* GetCandidatePointer() { return m_Candidates; }
#pragma warning(pop)

    

    void Reset(unsigned short nSelectedMemBank, ComparisonVariableSize nNewComparisonVariableSize);
    void ResetAll(ComparisonVariableSize nNewComparisonVariableSize, ByteAddress start, ByteAddress end);

    size_t Compare(ComparisonType nCompareType, unsigned int nTestValue, bool& bResultsFound);



    std::vector<size_t> GetBankIDs() const;



    void ChangeActiveMemBank(unsigned short nMemBank);

    unsigned char ActiveBankRAMByteRead(ByteAddress nOffs) const;
    void ActiveBankRAMByteWrite(ByteAddress nOffs, unsigned int nVal);

    unsigned int ActiveBankRAMRead(ByteAddress nOffs, ComparisonVariableSize size) const;

    // this is asking for a hang

    void ActiveBankRAMRead(unsigned char buffer[], ByteAddress nOffs, size_t count) const;

private:

    std::map<size_t, BankData> m_Banks;

    unsigned short m_nActiveMemBank ={};
    MemCandidate* m_Candidates ={};		//	Pointer to an array
    size_t m_nNumCandidates ={};		//	Actual quantity of legal candidates

    ComparisonVariableSize m_nComparisonSizeMode{ComparisonVariableSize::SixteenBit};

    bool m_bUseLastKnownValue{true};

    size_t m_nTotalBankSize ={};


};
#pragma pack(pop)
#pragma pack(pop)

MemManager g_MemManager;


#endif // !RA_MEMMANAGER_H
