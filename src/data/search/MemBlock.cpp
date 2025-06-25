#include "MemBlock.hh"

namespace ra {
namespace data {
namespace search {

uint8_t* MemBlock::AllocateMatchingAddresses() noexcept
{
    const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
    if (nAddressesSize > sizeof(m_vAddresses))
    {
        if (m_pAddresses == nullptr)
            m_pAddresses = new (std::nothrow) uint8_t[nAddressesSize];

        return m_pAddresses;
    }

    return &m_vAddresses[0];
}

void MemBlock::SetMatchingAddresses(std::vector<ra::ByteAddress>& vAddresses, gsl::index nFirstIndex, gsl::index nLastIndex)
{
    m_nMatchingAddresses = gsl::narrow_cast<unsigned int>(nLastIndex - nFirstIndex) + 1;

    if (m_nMatchingAddresses != m_nMaxAddresses)
    {
        const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
        unsigned char* pAddresses = AllocateMatchingAddresses();
        Expects(pAddresses != nullptr);

        memset(pAddresses, 0, nAddressesSize);
        for (gsl::index nIndex = nFirstIndex; nIndex <= nLastIndex; ++nIndex)
        {
            const auto nOffset = vAddresses.at(nIndex) - m_nFirstAddress;
            const auto nBit = 1 << (nOffset & 7);
            pAddresses[nOffset >> 3] |= nBit;
        }
    }
}

void MemBlock::CopyMatchingAddresses(const MemBlock& pSource)
{
    Expects(pSource.m_nMaxAddresses == m_nMaxAddresses);
    if (pSource.AreAllAddressesMatching())
    {
        m_nMatchingAddresses = m_nMaxAddresses;
    }
    else
    {
        const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
        unsigned char* pAddresses = AllocateMatchingAddresses();
        memcpy(pAddresses, pSource.GetMatchingAddressPointer(), nAddressesSize);
        m_nMatchingAddresses = pSource.m_nMatchingAddresses;
    }
}

void MemBlock::ExcludeMatchingAddress(ra::ByteAddress nAddress)
{
    const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
    unsigned char* pAddresses = nullptr;
    const auto nIndex = nAddress - m_nFirstAddress;
    const auto nBit = 1 << (nIndex & 7);

    if (AreAllAddressesMatching())
    {
        pAddresses = AllocateMatchingAddresses();
        Expects(pAddresses != nullptr);
        memset(pAddresses, 0xFF, nAddressesSize);
    }
    else
    {
        pAddresses = (nAddressesSize > sizeof(m_vAddresses)) ? m_pAddresses : &m_vAddresses[0];
        Expects(pAddresses != nullptr);
        if (!(pAddresses[nIndex >> 3] & nBit))
            return;
    }

    pAddresses[nIndex >> 3] &= ~nBit;
    --m_nMatchingAddresses;
}

bool MemBlock::ContainsAddress(ra::ByteAddress nAddress) const noexcept
{
    if (nAddress < m_nFirstAddress)
        return false;

    nAddress -= m_nFirstAddress;
    return (nAddress < m_nMaxAddresses);
}

bool MemBlock::ContainsMatchingAddress(ra::ByteAddress nAddress) const
{
    if (nAddress < m_nFirstAddress)
        return false;

    const auto nIndex = nAddress - m_nFirstAddress;
    if (nIndex >= m_nMaxAddresses)
        return false;

    if (AreAllAddressesMatching())
        return true;

    const uint8_t* pAddresses = GetMatchingAddressPointer();
    Expects(pAddresses != nullptr);
    const auto nBit = 1 << (nIndex & 7);
    return (pAddresses[nIndex >> 3] & nBit);
}

ra::ByteAddress MemBlock::GetMatchingAddress(gsl::index nIndex) const noexcept
{
    if (AreAllAddressesMatching())
        return m_nFirstAddress + gsl::narrow_cast<ra::ByteAddress>(nIndex);

    const auto nAddressesSize = (m_nMaxAddresses + 7) / 8;
    const uint8_t* pAddresses = (nAddressesSize > sizeof(m_vAddresses)) ? m_pAddresses : &m_vAddresses[0];
    ra::ByteAddress nAddress = m_nFirstAddress;
    const ra::ByteAddress nStop = m_nFirstAddress + m_nMaxAddresses;
    uint8_t nMask = 0x01;

    if (pAddresses != nullptr)
    {
        do
        {
            if (*pAddresses & nMask)
            {
                if (nIndex-- == 0)
                    return nAddress;
            }

            if (nMask == 0x80)
            {
                nMask = 0x01;
                pAddresses++;

                while (!*pAddresses)
                {
                    nAddress += 8;
                    if (nAddress >= nStop)
                        break;

                    pAddresses++;
                }
            }
            else
            {
                nMask <<= 1;
            }

            nAddress++;
        } while (nAddress < nStop);
    }

    return 0;
}

} // namespace search
} // namespace services
} // namespace ra
