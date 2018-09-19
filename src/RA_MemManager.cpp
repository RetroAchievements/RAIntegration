#include "RA_MemManager.h"

#include "RA_Defs.h"

MemManager g_MemManager;

MemManager::MemManager()
    : m_nTotalBankSize(0)
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
    m_nTotalBankSize = 0;
}

void MemManager::AddMemoryBank(size_t nBankID, _RAMByteReadFn* pReader, _RAMByteWriteFn* pWriter, size_t nBankSize)
{
    if (m_Banks.find(nBankID) != m_Banks.end())
    {
        ASSERT(!"Failed! Bank already added! Did you remove the existing bank?");
        return;
    }

    m_nTotalBankSize += nBankSize;

    m_Banks[nBankID].BankSize = nBankSize;
    m_Banks[nBankID].Reader = pReader;
    m_Banks[nBankID].Writer = pWriter;
}

void MemManager::ChangeActiveMemBank(_UNUSED unsigned short)
{
    ASSERT(!"Not Implemented!");
    return;

    //if( m_Banks.find( nMemBank ) == m_Banks.end() )
    //{
    //	ASSERT( !"Cannot find memory bank!" );
    //	return;
    //}
    //
    //if( m_Candidates != nullptr )
    //{
    //	delete[] m_Candidates;
    //	m_Candidates = nullptr;
    //}
    //
    //Reset( nMemBank, m_nComparisonSizeMode );
}

std::vector<size_t> MemManager::GetBankIDs() const
{
    std::vector<size_t> bankIDs;
    std::map<size_t, BankData>::const_iterator iter = m_Banks.begin();
    while (iter != m_Banks.end())
    {
        bankIDs.push_back(iter->first);
        iter++;
    }
    return bankIDs;
}

unsigned int MemManager::ActiveBankRAMRead(ra::ByteAddress nOffs, ComparisonVariableSize size) const
{
    unsigned char buffer[4];
    switch (size)
    {
        case Bit_0:
            return (ActiveBankRAMByteRead(nOffs) & 0x01);
        case Bit_1:
            return (ActiveBankRAMByteRead(nOffs) & 0x02) ? 1 : 0;
        case Bit_2:
            return (ActiveBankRAMByteRead(nOffs) & 0x04) ? 1 : 0;
        case Bit_3:
            return (ActiveBankRAMByteRead(nOffs) & 0x08) ? 1 : 0;
        case Bit_4:
            return (ActiveBankRAMByteRead(nOffs) & 0x10) ? 1 : 0;
        case Bit_5:
            return (ActiveBankRAMByteRead(nOffs) & 0x20) ? 1 : 0;
        case Bit_6:
            return (ActiveBankRAMByteRead(nOffs) & 0x40) ? 1 : 0;
        case Bit_7:
            return (ActiveBankRAMByteRead(nOffs) & 0x80) ? 1 : 0;
        case Nibble_Lower:
            return (ActiveBankRAMByteRead(nOffs) & 0x0F);
        case Nibble_Upper:
            return ((ActiveBankRAMByteRead(nOffs) >> 4) & 0x0F);
        case EightBit:
            return ActiveBankRAMByteRead(nOffs);
        default:
        case SixteenBit:
            ActiveBankRAMRead(buffer, nOffs, 2);
            return buffer[0] | (buffer[1] << 8);
        case ThirtyTwoBit:
            ActiveBankRAMRead(buffer, nOffs, 4);
            return buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
    }
}

unsigned char MemManager::ActiveBankRAMByteRead(ra::ByteAddress nOffs) const
{
    const BankData* bank = nullptr;

    int bankID = 0;
    int numBanks = m_Banks.size();
    while (bankID < numBanks)
    {
        bank = &m_Banks.at(bankID);
        if (nOffs < bank->BankSize)
            return bank->Reader(nOffs);

        nOffs -= bank->BankSize;
        bankID++;
    }

    return 0;
}

void MemManager::ActiveBankRAMRead(unsigned char buffer[], ra::ByteAddress nOffs, size_t count) const
{
    const BankData* bank = nullptr;

    int bankID = 0;
    int numBanks = m_Banks.size();
    do
    {
        if (bankID == numBanks)
        {
            memset(buffer, 0, count);
            return;
        }

        bank = &m_Banks.at(bankID);
        if (nOffs < bank->BankSize)
            break;

        nOffs -= bank->BankSize;
        bankID++;
    } while (true);

    _RAMByteReadFn* reader = bank->Reader;

    while (nOffs + count >= bank->BankSize)
    {
        size_t firstBankCount = bank->BankSize - nOffs;
        count -= firstBankCount;

        while (firstBankCount-- > 0)
            *buffer++ = reader(nOffs++);

        nOffs -= bank->BankSize;
        bankID++;
        if (bankID >= numBanks)
        {
            while (count-- > 0)
                *buffer++ = 0;
            return;
        }

        bank = &m_Banks.at(bankID);
        reader = bank->Reader;
    }

    while (count-- > 0)
        *buffer++ = reader(nOffs++);
}

void MemManager::ActiveBankRAMByteWrite(ra::ByteAddress nOffs, unsigned int nVal)
{
    int bankID = 0;
    int numBanks = m_Banks.size();
    while (bankID < numBanks && nOffs >= m_Banks.at(bankID).BankSize)
    {
        nOffs -= m_Banks.at(bankID).BankSize;
        bankID++;
    }

    if (bankID < numBanks)
    {
        m_Banks.at(bankID).Writer(nOffs, nVal);
    }
}

extern "C" unsigned int rc_peek_callback(unsigned int nAddress, unsigned int nBytes, _UNUSED void* pData)
{
    switch (nBytes)
    {
        case 1:
            return g_MemManager.ActiveBankRAMRead(nAddress, EightBit);
        case 2:
            return g_MemManager.ActiveBankRAMRead(nAddress, SixteenBit);
        case 4:
            return g_MemManager.ActiveBankRAMRead(nAddress, ThirtyTwoBit);
        default:
            return 0;
    }
}
