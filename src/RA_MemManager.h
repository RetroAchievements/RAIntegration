#ifndef RA_MEMMANAGER_H
#define RA_MEMMANAGER_H
#pragma once

#include "RA_Condition.h" // MemSize

typedef unsigned char (_RAMByteReadFn)(unsigned int nOffs);
typedef void (_RAMByteWriteFn)(unsigned int nOffs, unsigned int nVal);

class MemManager
{
private:
    
    class BankData
    {
    public:
        BankData() noexcept = default;
        explicit BankData(_RAMByteReadFn* pReadFn, _RAMByteWriteFn* pWriteFn, size_t nBankSize) noexcept
            : Reader(pReadFn), Writer(pWriteFn), BankSize(nBankSize)
        {
        }
        ~BankData() noexcept
        {
            Reader   = nullptr;
            Writer   = nullptr;
            BankSize = 0U;
        }
        //	Copying disabled
        BankData(const BankData&) = delete;
        BankData& operator=(BankData&) = delete;
        BankData(BankData&&) = delete;
        BankData& operator=(BankData&&) = delete;

    public:
        _RAMByteReadFn* Reader{ nullptr };
        _RAMByteWriteFn* Writer{ nullptr };
        size_t BankSize{ 0U };
    };
    using Banks = std::map<size_t, BankData>;
public:
    MemManager() noexcept(std::is_nothrow_default_constructible_v<Banks>) = default;
    ~MemManager() noexcept;
    MemManager(const MemManager&) noexcept = delete;
    MemManager& operator=(const MemManager&) noexcept = delete;
    MemManager(MemManager&&) noexcept = delete;
    MemManager& operator=(MemManager&&) noexcept = delete;

public:
    void ClearMemoryBanks() noexcept;
    void AddMemoryBank(size_t nBankID, _RAMByteReadFn* pReader, _RAMByteWriteFn* pWriter, size_t nBankSize);
    size_t NumMemoryBanks() const { return m_Banks.size(); }

    inline size_t BankSize(unsigned short nBank) const { return m_Banks.at(nBank).BankSize; }
    //inline size_t ActiveBankSize() const							{ return m_Banks.at( m_nActiveMemBank ).BankSize; }
    //inline unsigned short ActiveBankID() const					{ return m_nActiveMemBank; }
    inline size_t TotalBankSize() const { return m_nTotalBankSize; }

    std::vector<size_t> GetBankIDs() const;

    void ChangeActiveMemBank(_UNUSED unsigned short);

    unsigned char ActiveBankRAMByteRead(ra::ByteAddress nOffs) const;
    void ActiveBankRAMByteWrite(ra::ByteAddress nOffs, unsigned int nVal);

    unsigned int ActiveBankRAMRead(ra::ByteAddress nOffs, MemSize size) const;

    void ActiveBankRAMRead(unsigned char buffer[], ra::ByteAddress nOffs, size_t count) const;

private:
    std::map<size_t, BankData> m_Banks;
    unsigned short m_nActiveMemBank{};

    size_t m_nTotalBankSize{};
};

extern MemManager g_MemManager;


#endif // !RA_MEMMANAGER_H
