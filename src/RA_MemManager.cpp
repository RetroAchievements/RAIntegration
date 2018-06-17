#include "RA_MemManager.h"

//MemManager g_MemManager;



//	virtual
MemManager::~MemManager()
{
    ClearMemoryBanks();
}

void MemManager::ClearMemoryBanks()
{
    m_Banks.clear();
    m_nTotalBankSize = 0;
    if (m_Candidates != nullptr)
    {
        delete[] m_Candidates;
        m_Candidates = nullptr;
    }
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

void MemManager::ResetAll(ComparisonVariableSize nNewVarSize, ByteAddress start, ByteAddress end)
{
    //	RAM must be installed for this to work!
    if (m_Banks.size() == 0)
        return;

    m_nActiveMemBank = 0;
    m_nComparisonSizeMode = nNewVarSize;

    const size_t RAM_SIZE = TotalBankSize();

    if (m_Candidates == nullptr)
        m_Candidates = new MemCandidate[RAM_SIZE * 2];	//	To allow for upper and lower nibbles
    MemCandidate* pCandidate = &m_Candidates[0];

    if (end >= RAM_SIZE)
        end = RAM_SIZE - 1;

    if ((m_nComparisonSizeMode == Nibble_Lower) ||
        (m_nComparisonSizeMode == Nibble_Upper))
    {
        for (ByteAddress i = start; i <= end; ++i)
        {
            unsigned char byte = ActiveBankRAMByteRead(i);

            pCandidate->m_nAddr = i;
            pCandidate->m_bUpperNibble = false;			//lower first?
            pCandidate->m_nLastKnownValue = static_cast<unsigned int>(byte & 0xf);
            ++pCandidate;

            pCandidate->m_nAddr = i;
            pCandidate->m_bUpperNibble = true;
            pCandidate->m_nLastKnownValue = static_cast<unsigned int>((byte >> 4) & 0xf);
            ++pCandidate;
        }
    }
    else if (m_nComparisonSizeMode == EightBit)
    {
        for (ByteAddress i = start; i <= end; ++i)
        {
            pCandidate->m_nAddr = i;
            pCandidate->m_nLastKnownValue = ActiveBankRAMByteRead(i);
            ++pCandidate;
        }
    }
    else if (m_nComparisonSizeMode == SixteenBit)
    {
        unsigned char low = ActiveBankRAMByteRead(start);
        for (ByteAddress i = start + 1; i <= end; ++i)
        {
            unsigned char high = ActiveBankRAMByteRead(i);
            pCandidate->m_nAddr = i - 1;
            pCandidate->m_nLastKnownValue = ra::to_unsigned(low | (high << 8));
            ++pCandidate;
            low = high;
        }
    }
    else if (m_nComparisonSizeMode == ThirtyTwoBit)
    {
        if (end - start > 3)
        {
            auto value = ra::to_unsigned(ActiveBankRAMByteRead(start) |
                (ActiveBankRAMByteRead(start + 1) << 8) | (ActiveBankRAMByteRead(start + 2) << 16));

            for (ByteAddress i = start + 3; i <= end; ++i)
            {
                value |= (ActiveBankRAMByteRead(i) << 24);
                pCandidate->m_nAddr = i - 3;
                pCandidate->m_nLastKnownValue = value;
                ++pCandidate;
                value >>= 8;
            }
        }
    }

    m_nNumCandidates = ra::to_unsigned(pCandidate - &m_Candidates[0]);
}

void MemManager::Reset(unsigned short nSelectedMemBank, ComparisonVariableSize nNewVarSize)
{
    //	RAM must be installed for this to work!
    if (m_Banks.size() == 0)
        return;

    if (m_Banks.find(nSelectedMemBank) == m_Banks.end())
        return;

    m_nActiveMemBank = nSelectedMemBank;
    m_nComparisonSizeMode = nNewVarSize;

    const size_t RAM_SIZE = m_Banks[m_nActiveMemBank].BankSize;

    if (m_Candidates == nullptr)
        m_Candidates = new MemCandidate[RAM_SIZE * 2];	//	To allow for upper and lower nibbles

    //	Initialize the memory cache: i.e. every memory address is valid!
    if ((m_nComparisonSizeMode == Nibble_Lower) ||
        (m_nComparisonSizeMode == Nibble_Upper))
    {
        for (size_t i = 0; i < RAM_SIZE; ++i)
        {
            m_Candidates[(i * 2)].m_nAddr = i;
            m_Candidates[(i * 2)].m_bUpperNibble = false;			//lower first?
            m_Candidates[(i * 2)].m_nLastKnownValue = static_cast<unsigned int>(ActiveBankRAMByteRead(i) & 0xf);

            m_Candidates[(i * 2) + 1].m_nAddr = i;
            m_Candidates[(i * 2) + 1].m_bUpperNibble = true;
            m_Candidates[(i * 2) + 1].m_nLastKnownValue = static_cast<unsigned int>((ActiveBankRAMByteRead(i) >> 4) & 0xf);
        }
        m_nNumCandidates = RAM_SIZE * 2;
    }
    else if (m_nComparisonSizeMode == EightBit)
    {
        for (DWORD i = 0; i < RAM_SIZE; ++i)
        {
            m_Candidates[i].m_nAddr = i;
            m_Candidates[i].m_nLastKnownValue = ActiveBankRAMByteRead(i);
        }
        m_nNumCandidates = RAM_SIZE;
    }
    else if (m_nComparisonSizeMode == SixteenBit)
    {
        for (DWORD i = 0; i < (RAM_SIZE / 2); ++i)
        {
            m_Candidates[i].m_nAddr = (i * 2);
            m_Candidates[i].m_nLastKnownValue =
                ra::to_unsigned((ActiveBankRAMByteRead((i * 2))) |
                (ActiveBankRAMByteRead((i * 2) + 1) << 8));
        }
        m_nNumCandidates = RAM_SIZE / 2;
    }
    else if (m_nComparisonSizeMode == ThirtyTwoBit)
    {
        //	Assuming 32-bit-aligned! 		
        for (DWORD i = 0; i < (RAM_SIZE / 4); ++i)
        {
            m_Candidates[i].m_nAddr = (i * 4);
            m_Candidates[i].m_nLastKnownValue =
                ra::to_unsigned(
                (ActiveBankRAMByteRead((i * 4))) |
                    (ActiveBankRAMByteRead((i * 4) + 1) << 8) |
                    (ActiveBankRAMByteRead((i * 4) + 2) << 16) |
                    (ActiveBankRAMByteRead((i * 4) + 3) << 24)
                );
        }
        m_nNumCandidates = RAM_SIZE / 4;
    }
}

size_t MemManager::Compare(ComparisonType nCompareType, unsigned int nTestValue, bool& bResultsFound)
{
    size_t nGoodResults = 0;
    for (size_t i = 0; i < m_nNumCandidates; ++i)
    {
        // Spectre security risk
        auto myCandiate{
            std::make_unique<MemCandidate>(MemCandidate{m_Candidates[i].m_nAddr, m_Candidates[i].m_nLastKnownValue,
            m_Candidates[i].m_bUpperNibble, m_Candidates[i].m_bHasChanged })
        };
        unsigned int nAddr = myCandiate->m_nAddr;
        unsigned int nLiveValue = 0;

        if ((m_nComparisonSizeMode == Nibble_Lower) ||
            (m_nComparisonSizeMode == Nibble_Upper))
        {
            if (myCandiate->m_bUpperNibble)
                nLiveValue = ra::to_unsigned((ActiveBankRAMByteRead(nAddr) >> 4) & 0xf);
            else
                nLiveValue = ra::to_unsigned(ActiveBankRAMByteRead(nAddr) & 0xf);
        }
        else
        {
            nLiveValue = ActiveBankRAMRead(nAddr, m_nComparisonSizeMode);
        }

        bool bValid = false;
        unsigned int nQueryValue = (m_bUseLastKnownValue ? myCandiate->m_nLastKnownValue : nTestValue);
        switch (nCompareType)
        {
            case ComparisonType::Equals:             bValid = (nLiveValue == nQueryValue);	break;
            case ComparisonType::LessThan:           bValid = (nLiveValue < nQueryValue);	break;
            case ComparisonType::LessThanOrEqual:    bValid = (nLiveValue <= nQueryValue);	break;
            case ComparisonType::GreaterThan:        bValid = (nLiveValue > nQueryValue);	break;
            case ComparisonType::GreaterThanOrEqual: bValid = (nLiveValue >= nQueryValue);	break;
            case ComparisonType::NotEqualTo:         bValid = (nLiveValue != nQueryValue);	break;
            case ComparisonType::NumComparisonTypes:
                _FALLTHROUGH;
            default:
                ASSERT(!"Unknown comparison type?!");
        }
        m_Candidates[i] = *myCandiate;

        //	If the current address in ram still matches the query, store in result[]
        if (bValid)
        {
            //	Optimization: just store it back in m_Candidates
            m_Candidates[nGoodResults].m_nLastKnownValue = nLiveValue;
            m_Candidates[nGoodResults].m_nAddr = m_Candidates[i].m_nAddr;
            m_Candidates[nGoodResults].m_bUpperNibble = m_Candidates[i].m_bUpperNibble;
            nGoodResults++;
        }
    }

    //	If we have any results, this is good :)
    bResultsFound = (nGoodResults > 0);
    if (bResultsFound)
        m_nNumCandidates = nGoodResults;

    return m_nNumCandidates;
}

void MemManager::ChangeActiveMemBank(_UNUSED unsigned short nMemBank)
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

unsigned int MemManager::ActiveBankRAMRead(ByteAddress nOffs, ComparisonVariableSize size) const
{
    unsigned char buffer[4];
    switch (size)
    {
        case ComparisonVariableSize::Bit_0:
            return ra::to_unsigned(ActiveBankRAMByteRead(nOffs) & 0x01);
        case ComparisonVariableSize::Bit_1:
            return ra::to_unsigned((ActiveBankRAMByteRead(nOffs) & 0x02) ? 1 : 0);
        case ComparisonVariableSize::Bit_2:
            return ra::to_unsigned((ActiveBankRAMByteRead(nOffs) & 0x04) ? 1 : 0);
        case ComparisonVariableSize::Bit_3:
            return ra::to_unsigned((ActiveBankRAMByteRead(nOffs) & 0x08) ? 1 : 0);
        case ComparisonVariableSize::Bit_4:
            return ra::to_unsigned((ActiveBankRAMByteRead(nOffs) & 0x10) ? 1 : 0);
        case ComparisonVariableSize::Bit_5:
            return ra::to_unsigned((ActiveBankRAMByteRead(nOffs) & 0x20) ? 1 : 0);
        case ComparisonVariableSize::Bit_6:
            return ra::to_unsigned((ActiveBankRAMByteRead(nOffs) & 0x40) ? 1 : 0);
        case ComparisonVariableSize::Bit_7:
            return ra::to_unsigned((ActiveBankRAMByteRead(nOffs) & 0x80) ? 1 : 0);
        case ComparisonVariableSize::Nibble_Lower:
            return ra::to_unsigned((ActiveBankRAMByteRead(nOffs) & 0x0F));
        case ComparisonVariableSize::Nibble_Upper:
            return ra::to_unsigned((ActiveBankRAMByteRead(nOffs) >> 4) & 0x0F);
        case ComparisonVariableSize::EightBit:
            return ActiveBankRAMByteRead(nOffs);
        case ComparisonVariableSize::NumComparisonVariableSizeTypes:
            _FALLTHROUGH;
        default:
        case ComparisonVariableSize::SixteenBit:
            ActiveBankRAMRead(buffer, nOffs, 2);
            return ra::to_unsigned(buffer[0] | (buffer[1] << 8));
        case ComparisonVariableSize::ThirtyTwoBit:
            ActiveBankRAMRead(buffer, nOffs, 4);
            return ra::to_unsigned(buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24));
    }
}

unsigned char MemManager::ActiveBankRAMByteRead(ByteAddress nOffs) const
{
    const BankData* bank = nullptr;

    int bankID = 0;
    int numBanks = ra::to_signed(m_Banks.size());
    while (bankID < numBanks)
    {
        bank = &m_Banks.at(ra::to_unsigned(bankID));
        if (nOffs < bank->BankSize)
            return bank->Reader(nOffs);

        nOffs -= bank->BankSize;
        bankID++;
    }

    return 0;
}

void MemManager::ActiveBankRAMRead(unsigned char buffer[], ByteAddress nOffs, size_t count) const
{
    const BankData* bank = nullptr;

    int bankID = 0;
    int numBanks = ra::to_signed(m_Banks.size());
    while (bankID < numBanks)
    {
        bank = &m_Banks.at(ra::to_unsigned(bankID));
        if (nOffs < bank->BankSize)
            break;

        nOffs -= bank->BankSize;
        bankID++;
    }

    if (bank == nullptr)
        return;

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

        bank = &m_Banks.at(ra::to_unsigned(bankID));
        reader = bank->Reader;
    }

    while (count-- > 0)
        *buffer++ = reader(nOffs++);
}

void MemManager::ActiveBankRAMByteWrite(ByteAddress nOffs, unsigned int nVal)
{
    int bankID = 0;
    /*int numBanks = ra::to_signed(m_Banks.size());*/
    // if you really want a signed int you could do this instead
    // difference_type (signed) instead of size_type (unsigned), it's a constant expression so it's fast
    auto numBanks{ std::distance(m_Banks.begin(), m_Banks.end()) };
    while ((bankID < numBanks) && (nOffs >= m_Banks.at(ra::to_unsigned(bankID)).BankSize))
    {
        nOffs -= m_Banks.at(ra::to_unsigned(bankID)).BankSize;
        bankID++;
    }

    if (bankID < numBanks)
    {
        m_Banks.at(ra::to_unsigned(bankID)).Writer(nOffs, nVal);
    }
}
