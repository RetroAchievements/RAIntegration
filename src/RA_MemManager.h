#ifndef RA_MEMMANAGER_H
#define RA_MEMMANAGER_H
#pragma once

#include "RA_Condition.h" // ComparisonVariableSize

typedef unsigned char (_RAMByteReadFn)(unsigned int nOffs);
typedef void (_RAMByteWriteFn)(unsigned int nOffs, unsigned int nVal);

class MemManager
{
private:
    class BankData
    {
    public:
        BankData()
            : Reader(nullptr), Writer(nullptr), BankSize(0)
        {
        }

        BankData(_RAMByteReadFn* pReadFn, _RAMByteWriteFn* pWriteFn, size_t nBankSize)
            : Reader(pReadFn), Writer(pWriteFn), BankSize(nBankSize)
        {
        }

    private:
        //	Copying disabled
        BankData(const BankData&) = delete;
        BankData& operator=(BankData&) = delete;

    public:
        _RAMByteReadFn * Reader;
        _RAMByteWriteFn* Writer;
        size_t BankSize;
    };

public:
    MemManager();
    virtual ~MemManager();

public:
    void ClearMemoryBanks();
    void AddMemoryBank(size_t nBankID, _RAMByteReadFn* pReader, _RAMByteWriteFn* pWriter, size_t nBankSize);
    size_t NumMemoryBanks() const { return m_Banks.size(); }

    inline size_t BankSize(unsigned short nBank) const { return m_Banks.at(nBank).BankSize; }
    //inline size_t ActiveBankSize() const							{ return m_Banks.at( m_nActiveMemBank ).BankSize; }
    //inline unsigned short ActiveBankID() const					{ return m_nActiveMemBank; }
    inline size_t TotalBankSize() const { return m_nTotalBankSize; }

    std::vector<size_t> GetBankIDs() const;

    void ChangeActiveMemBank(unsigned short nMemBank);

    unsigned char ActiveBankRAMByteRead(ra::ByteAddress nOffs) const;
    void ActiveBankRAMByteWrite(ra::ByteAddress nOffs, unsigned int nVal);

    unsigned int ActiveBankRAMRead(ra::ByteAddress nOffs, ComparisonVariableSize size) const;

    void ActiveBankRAMRead(unsigned char buffer[], ra::ByteAddress nOffs, size_t count) const;

private:
    std::map<size_t, BankData> m_Banks;
    unsigned short m_nActiveMemBank;

    size_t m_nTotalBankSize;
};

extern MemManager g_MemManager;

EXTERN_C unsigned int rc_peek_callback(unsigned int nAddress, unsigned int nBytes, void* pData);

#endif // !RA_MEMMANAGER_H
