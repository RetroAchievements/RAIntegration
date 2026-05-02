#include "EmulatorMemoryContext.hh"

#include "services/IClock.hh"
#include "services/IDebuggerDetector.hh"
#include "services/ServiceLocator.hh"

#include "util/Compat.hh"
#include "util/TypeCasts.hh"

namespace ra {
namespace context {
namespace impl {

static std::wstring FormatAddressLarge(ra::data::ByteAddress nAddress)
{
    std::wstring sAddress;
    sAddress.resize(10);
    swprintf_s(sAddress.data(), 11, L"0x%08x", nAddress);
    return sAddress;
}

static std::wstring FormatAddressMedium(ra::data::ByteAddress nAddress)
{
    if (nAddress & 0xFF000000)
        return FormatAddressLarge(nAddress);

    std::wstring sAddress;
    sAddress.resize(8);
    swprintf_s(sAddress.data(), 9, L"0x%06x", nAddress);
    return sAddress;
}

static std::wstring FormatAddressSmall(ra::data::ByteAddress nAddress)
{
    if (nAddress & 0xFFFF0000)
        return FormatAddressMedium(nAddress);

    std::wstring sAddress;
    sAddress.resize(6);
    swprintf_s(sAddress.data(), 7, L"0x%04x", nAddress);
    return sAddress;
}

GSL_SUPPRESS_F6
EmulatorMemoryContext::EmulatorMemoryContext() noexcept
{
    m_fFormatAddress = FormatAddressSmall;
}

void EmulatorMemoryContext::ClearMemoryBlocks()
{
    m_vMemoryBlocks.clear();

    if (m_nTotalMemorySize != 0U)
    {
        m_nTotalMemorySize = 0U;
        OnTotalMemorySizeChanged();
    }
}

void EmulatorMemoryContext::AddMemoryBlock(gsl::index nIndex, size_t nBytes,
    EmulatorMemoryContext::MemoryReadFunction pReader, EmulatorMemoryContext::MemoryWriteFunction pWriter)
{
    while (m_vMemoryBlocks.size() <= ra::to_unsigned(nIndex))
        m_vMemoryBlocks.emplace_back();

    MemoryBlock& pBlock = m_vMemoryBlocks.at(nIndex);
    if (pBlock.size == 0)
    {
        pBlock.size = nBytes;
        pBlock.read = pReader;
        pBlock.write = pWriter;
        pBlock.readBlock = nullptr;

        m_nTotalMemorySize += nBytes;

        OnTotalMemorySizeChanged();
    }
}

void EmulatorMemoryContext::AddMemoryBlockReader(gsl::index nIndex,
    EmulatorMemoryContext::MemoryReadBlockFunction pReader)
{
    if (nIndex < gsl::narrow_cast<gsl::index>(m_vMemoryBlocks.size()))
        m_vMemoryBlocks.at(nIndex).readBlock = pReader;
}

void EmulatorMemoryContext::OnTotalMemorySizeChanged()
{
    if (m_nTotalMemorySize <= 0x10000)
        m_fFormatAddress = FormatAddressSmall;
    else if (m_nTotalMemorySize <= 0x1000000)
        m_fFormatAddress = FormatAddressMedium;
    else
        m_fFormatAddress = FormatAddressLarge;

    if (m_vNotifyTargets.LockIfNotEmpty())
    {
        for (auto& target : m_vNotifyTargets.Targets())
            target.OnTotalMemorySizeChanged();

        m_vNotifyTargets.Unlock();
    }
}

bool EmulatorMemoryContext::HasInvalidRegions() const noexcept
{
    for (const auto& pBlock : m_vMemoryBlocks)
    {
        if (!pBlock.read)
            return true;
    }

    return false;
}

bool EmulatorMemoryContext::IsValidAddress(ra::data::ByteAddress nAddress) const noexcept
{
    for (const auto& pBlock : m_vMemoryBlocks)
    {
        if (nAddress < pBlock.size)
        {
            if (pBlock.read)
                return true;

            break;
        }

        nAddress -= gsl::narrow_cast<ra::data::ByteAddress>(pBlock.size);
    }

    return false;
}

void EmulatorMemoryContext::AssertIsOnDoFrameThread() const
{
#if RA_INTEGRATION_VERSION_REVISION != 0
 #if !defined(RA_UTEST)
    const auto* pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>().GetClient();
    const auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
    if (!pClient->state.allow_background_memory_reads &&
        !pRuntime.IsOnDoFrameThread())
    {
        const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
        if (pGameContext.GameId() != 0 && !pGameContext.IsGameLoading())
        {
            ra::ui::viewmodels::MessageBoxViewModel::ShowWarningMessage(
                L"Memory Read on Background Thread",
                L"Memory is being read on a thread other than the thread that last called rc_client_do_frame and the "
                L"client has specified that should not be allowed. Please notify a RetroAchievements engineer what you "
                L"were doing when this occurred.");
        }
    }
 #endif
#endif
}

uint8_t EmulatorMemoryContext::ReadMemoryByte(ra::data::ByteAddress nAddress) const
{
    AssertIsOnDoFrameThread();

    for (const auto& pBlock : m_vMemoryBlocks)
    {
        if (nAddress < pBlock.size)
        {
            if (pBlock.read)
                return pBlock.read(nAddress);

            break;
        }

        nAddress -= gsl::narrow_cast<ra::data::ByteAddress>(pBlock.size);
    }

    return 0;
}

uint32_t EmulatorMemoryContext::ReadMemory(ra::data::ByteAddress nAddress, uint8_t pBuffer[], size_t nCount,
                                   const EmulatorMemoryContext::MemoryBlock& pBlock)
{
    Expects(pBuffer != nullptr);

    if (pBlock.readBlock)
    {
        const size_t nRead = pBlock.readBlock(nAddress, pBuffer, gsl::narrow_cast<uint32_t>(nCount));
        if (nRead < nCount)
            memset(pBuffer + nRead, 0, nCount - nRead);

        return gsl::narrow_cast<uint32_t>(nRead);
    }

    if (!pBlock.read)
    {
        memset(pBuffer, 0, nCount);
        return 0;
    }

    auto nRemaining = nCount;
    while (nRemaining >= 8) // unrolled loop to read 8-byte chunks
    {
        *pBuffer++ = pBlock.read(nAddress++);
        *pBuffer++ = pBlock.read(nAddress++);
        *pBuffer++ = pBlock.read(nAddress++);
        *pBuffer++ = pBlock.read(nAddress++);
        *pBuffer++ = pBlock.read(nAddress++);
        *pBuffer++ = pBlock.read(nAddress++);
        *pBuffer++ = pBlock.read(nAddress++);
        *pBuffer++ = pBlock.read(nAddress++);
        nRemaining -= 8;
    }

    switch (nRemaining) // partial Duff's device to read remaining bytes
    {
        case 7:
            *pBuffer++ = pBlock.read(nAddress++);
            _FALLTHROUGH;
        case 6:
            *pBuffer++ = pBlock.read(nAddress++);
            _FALLTHROUGH;
        case 5:
            *pBuffer++ = pBlock.read(nAddress++);
            _FALLTHROUGH;
        case 4:
            *pBuffer++ = pBlock.read(nAddress++);
            _FALLTHROUGH;
        case 3:
            *pBuffer++ = pBlock.read(nAddress++);
            _FALLTHROUGH;
        case 2:
            *pBuffer++ = pBlock.read(nAddress++);
            _FALLTHROUGH;
        case 1:
            *pBuffer++ = pBlock.read(nAddress++);
            _FALLTHROUGH;
        default:
            break;
    }

    return gsl::narrow_cast<uint32_t>(nCount);
}

uint32_t EmulatorMemoryContext::ReadMemory(ra::data::ByteAddress nAddress, uint8_t pBuffer[], size_t nCount) const
{
    AssertIsOnDoFrameThread();

    uint32_t nBytesRead = 0;
    Expects(pBuffer != nullptr);

    for (const auto& pBlock : m_vMemoryBlocks)
    {
        if (nAddress >= pBlock.size)
        {
            nAddress -= gsl::narrow_cast<ra::data::ByteAddress>(pBlock.size);
            continue;
        }

        const size_t nBlockRemaining = pBlock.size - nAddress;
        size_t nToRead = std::min(nCount, nBlockRemaining);

        nBytesRead += ReadMemory(nAddress, pBuffer, nToRead, pBlock);

        pBuffer += nToRead;
        nCount -= nToRead;

        if (nCount == 0)
            return nBytesRead;

        nAddress = 0;
    }

    if (nCount > 0)
        memset(pBuffer, 0, nCount);

    return nBytesRead;
}

uint32_t EmulatorMemoryContext::ReadMemory(ra::data::ByteAddress nAddress, ra::data::Memory::Size nSize) const
{
    switch (nSize)
    {
        case ra::data::Memory::Size::Bit0:
            return (ReadMemoryByte(nAddress) & 0x01);
        case ra::data::Memory::Size::Bit1:
            return (ReadMemoryByte(nAddress) & 0x02) ? 1 : 0;
        case ra::data::Memory::Size::Bit2:
            return (ReadMemoryByte(nAddress) & 0x04) ? 1 : 0;
        case ra::data::Memory::Size::Bit3:
            return (ReadMemoryByte(nAddress) & 0x08) ? 1 : 0;
        case ra::data::Memory::Size::Bit4:
            return (ReadMemoryByte(nAddress) & 0x10) ? 1 : 0;
        case ra::data::Memory::Size::Bit5:
            return (ReadMemoryByte(nAddress) & 0x20) ? 1 : 0;
        case ra::data::Memory::Size::Bit6:
            return (ReadMemoryByte(nAddress) & 0x40) ? 1 : 0;
        case ra::data::Memory::Size::Bit7:
            return (ReadMemoryByte(nAddress) & 0x80) ? 1 : 0;
        case ra::data::Memory::Size::NibbleLower:
            return (ReadMemoryByte(nAddress) & 0x0F);
        case ra::data::Memory::Size::NibbleUpper:
            return ((ReadMemoryByte(nAddress) >> 4) & 0x0F);
        case ra::data::Memory::Size::EightBit:
            return ReadMemoryByte(nAddress);
        default:
        case ra::data::Memory::Size::SixteenBit:
        {
            uint8_t buffer[2];
            ReadMemory(nAddress, buffer, 2);
            return buffer[0] | (buffer[1] << 8);
        }
        case ra::data::Memory::Size::TwentyFourBit:
        {
            uint8_t buffer[3];
            ReadMemory(nAddress, buffer, 3);
            return buffer[0] | (buffer[1] << 8) | (buffer[2] << 16);
        }
        case ra::data::Memory::Size::Float:
        case ra::data::Memory::Size::FloatBigEndian:
        case ra::data::Memory::Size::Double32:
        case ra::data::Memory::Size::Double32BigEndian:
        case ra::data::Memory::Size::MBF32:
        case ra::data::Memory::Size::MBF32LE:
        case ra::data::Memory::Size::ThirtyTwoBit:
        {
            uint8_t buffer[4];
            ReadMemory(nAddress, buffer, 4);
            return buffer[0] | (buffer[1] << 8) | (buffer[2] << 16) | (buffer[3] << 24);
        }
        case ra::data::Memory::Size::SixteenBitBigEndian:
        {
            uint8_t buffer[2];
            ReadMemory(nAddress, buffer, 2);
            return buffer[1] | (buffer[0] << 8);
        }
        case ra::data::Memory::Size::TwentyFourBitBigEndian:
        {
            uint8_t buffer[3];
            ReadMemory(nAddress, buffer, 3);
            return buffer[2] | (buffer[1] << 8) | (buffer[0] << 16);
        }
        case ra::data::Memory::Size::ThirtyTwoBitBigEndian:
        {
            uint8_t buffer[4];
            ReadMemory(nAddress, buffer, 4);
            return buffer[3] | (buffer[2] << 8) | (buffer[1] << 16) | (buffer[0] << 24);
        }
        case ra::data::Memory::Size::BitCount:
        {
            const uint8_t nValue = ReadMemoryByte(nAddress);
            static const std::array<uint8_t, 16> nBitsSet = { 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4 };
            return nBitsSet.at(nValue & 0x0F) + nBitsSet.at((nValue >> 4) & 0x0F);
        }
    }
}

void EmulatorMemoryContext::WriteMemory(ra::data::ByteAddress nAddress, const uint8_t* pBytes, size_t nBytes) const
{
    Expects(pBytes != nullptr);
    size_t nBytesWritten = 0;
    auto nBlockAddress = nAddress;

    for (const auto& pBlock : m_vMemoryBlocks)
    {
        if (nBlockAddress < pBlock.size)
        {
            if (!pBlock.write)
                break;

            m_bMemoryModified = true;

            if (nBytes == 1)
            {
                pBlock.write(nBlockAddress, pBytes[0]);
                nBytesWritten = 1;
                break;
            }
            else
            {
                do
                {
                    pBlock.write(nBlockAddress++, pBytes[nBytesWritten++]);
                } while (nBytesWritten < nBytes && nBlockAddress < pBlock.size);

                if (nBytesWritten == nBytes)
                    break;
            }
        }

        nBlockAddress -= gsl::narrow_cast<ra::data::ByteAddress>(pBlock.size);
    }

    if (nBytesWritten > 0)
    {
        if (m_vNotifyTargets.LockIfNotEmpty())
        {
            for (auto& target : m_vNotifyTargets.Targets())
            {
                for (unsigned nIndex = 0; nIndex < nBytesWritten; ++nIndex)
                    target.OnByteWritten(nAddress + nIndex, pBytes[nIndex]);
            }

            m_vNotifyTargets.Unlock();
        }
    }
}

void EmulatorMemoryContext::WriteMemoryByte(ra::data::ByteAddress nAddress, uint8_t nValue) const
{
    WriteMemory(nAddress, &nValue, 1);
}

void EmulatorMemoryContext::WriteMemory(ra::data::ByteAddress nAddress, ra::data::Memory::Size nSize, uint32_t nValue) const
{
    union
    {
        uint32_t u32;
        uint8_t u8[4];
    } u{};
    u.u32 = nValue;

    switch (nSize)
    {
        case ra::data::Memory::Size::EightBit:
            WriteMemory(nAddress, u.u8, 1);
            break;

        case ra::data::Memory::Size::SixteenBit:
            WriteMemory(nAddress, u.u8, 2);
            break;

        case ra::data::Memory::Size::TwentyFourBit:
            WriteMemory(nAddress, u.u8, 3);
            break;

        case ra::data::Memory::Size::ThirtyTwoBit:
            // already little endian
            WriteMemory(nAddress, u.u8, 4);
            break;

        case ra::data::Memory::Size::Float:
        case ra::data::Memory::Size::FloatBigEndian:
        case ra::data::Memory::Size::Double32:
        case ra::data::Memory::Size::Double32BigEndian:
        case ra::data::Memory::Size::MBF32:
        case ra::data::Memory::Size::MBF32LE:
            // assume the value has already been encoded into a 32-bit little endian value
            WriteMemory(nAddress, u.u8, 4);
            break;

        case ra::data::Memory::Size::SixteenBitBigEndian:
            u.u8[3] = u.u8[0];
            u.u8[2] = u.u8[1];
            WriteMemory(nAddress, &u.u8[2], 2);
            break;

        case ra::data::Memory::Size::TwentyFourBitBigEndian:
            u.u8[3] = u.u8[0];
            u.u8[0] = u.u8[2];
            u.u8[2] = u.u8[3];
            WriteMemory(nAddress, u.u8, 3);
            break;

        case ra::data::Memory::Size::ThirtyTwoBitBigEndian:
            u.u32 = ra::data::Memory::ReverseBytes(nValue);
            WriteMemory(nAddress, u.u8, 4);
            break;

        case ra::data::Memory::Size::Bit0:
            u.u32 = (ReadMemoryByte(nAddress) & ~0x01) | (nValue & 1);
            WriteMemory(nAddress, u.u8, 1);
            break;
        case ra::data::Memory::Size::Bit1:
            u.u32 = (ReadMemoryByte(nAddress) & ~0x02) | ((nValue & 1) << 1);
            WriteMemory(nAddress, u.u8, 1);
            break;
        case ra::data::Memory::Size::Bit2:
            u.u32 = (ReadMemoryByte(nAddress) & ~0x04) | ((nValue & 1) << 2);
            WriteMemory(nAddress, u.u8, 1);
            break;
        case ra::data::Memory::Size::Bit3:
            u.u32 = (ReadMemoryByte(nAddress) & ~0x08) | ((nValue & 1) << 3);
            WriteMemory(nAddress, u.u8, 1);
            break;
        case ra::data::Memory::Size::Bit4:
            u.u32 = (ReadMemoryByte(nAddress) & ~0x10) | ((nValue & 1) << 4);
            WriteMemory(nAddress, u.u8, 1);
            break;
        case ra::data::Memory::Size::Bit5:
            u.u32 = (ReadMemoryByte(nAddress) & ~0x20) | ((nValue & 1) << 5);
            WriteMemory(nAddress, u.u8, 1);
            break;
        case ra::data::Memory::Size::Bit6:
            u.u32 = (ReadMemoryByte(nAddress) & ~0x40) | ((nValue & 1) << 6);
            WriteMemory(nAddress, u.u8, 1);
            break;
        case ra::data::Memory::Size::Bit7:
            u.u32 = (ReadMemoryByte(nAddress) & ~0x80) | ((nValue & 1) << 7);
            WriteMemory(nAddress, u.u8, 1);
            break;

        case ra::data::Memory::Size::NibbleLower:
            u.u32 = (ReadMemoryByte(nAddress) & ~0x0F) | (nValue & 0x0F);
            WriteMemory(nAddress, u.u8, 1);
            break;
        case ra::data::Memory::Size::NibbleUpper:
            u.u32 = (ReadMemoryByte(nAddress) & ~0xF0) | ((nValue & 0x0F) << 4);
            WriteMemory(nAddress, u.u8, 1);
            break;

        default:
            break;
    }
}

void EmulatorMemoryContext::ResetMemoryModified()
{
    m_bMemoryModified = false;

    if (m_bMemoryInsecure)
    {
        // memory was insecure, immediately do a check
        const auto& pDesktop = ra::services::ServiceLocator::Get<ra::services::IDebuggerDetector>();
        m_bMemoryInsecure = pDesktop.IsDebuggerPresent();

        const auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
        m_tLastInsecureCheck = pClock.UpTime();
    }
}

bool EmulatorMemoryContext::IsMemoryInsecure() const
{
    if (m_bMemoryInsecure)
        return true;

    // limit checks to once every 10 seconds
    const auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
    const auto tNow = pClock.UpTime();
    const auto tElapsed = tNow - m_tLastInsecureCheck;
    if (tElapsed > std::chrono::seconds(10))
    {
        const auto& pDesktop = ra::services::ServiceLocator::Get<ra::services::IDebuggerDetector>();
        m_bMemoryInsecure = pDesktop.IsDebuggerPresent();
        m_tLastInsecureCheck = tNow;
    }

    return m_bMemoryInsecure;
}

_CONSTANT_VAR MAX_BLOCK_SIZE = 256U * 1024; // 256K

void EmulatorMemoryContext::CaptureMemory(std::vector<ra::data::CapturedMemoryBlock>& vBlocks, ra::data::ByteAddress nAddress, uint32_t nCount, uint32_t nPadding) const
{
    ra::data::ByteAddress nAdjustedAddress = nAddress;
    for (const auto& pMemoryBlock : m_vMemoryBlocks)
    {
        if (nAdjustedAddress >= pMemoryBlock.size)
        {
            nAdjustedAddress -= gsl::narrow_cast<ra::data::ByteAddress>(pMemoryBlock.size);
            continue;
        }

        if (!pMemoryBlock.read && !pMemoryBlock.readBlock)
        {
            nCount -= gsl::narrow_cast<uint32_t>(pMemoryBlock.size);
            nAddress += gsl::narrow_cast<ra::data::ByteAddress>(pMemoryBlock.size);
            nAdjustedAddress = 0;
            continue;
        }

        const uint32_t nBlockRemaining = gsl::narrow_cast<uint32_t>(pMemoryBlock.size) - nAdjustedAddress;
        uint32_t nToRead = std::min(nCount, nBlockRemaining);
        nCount -= nToRead;

        while (nToRead > 0)
        {
            ra::data::CapturedMemoryBlock* pBlock = nullptr;

            const uint32_t nBlockSize = std::min(nToRead, MAX_BLOCK_SIZE);
            const bool bIsLastBlock = (nCount == 0 && nBlockSize == nToRead);
            if (!bIsLastBlock)
            {
                // capture nPadding overlapping bytes so we don't have to stitch multiple
                // blocks together if a read occurs at the block boundary
                pBlock = &vBlocks.emplace_back(nAddress, nBlockSize + nPadding, nBlockSize);
            }
            else
            {
                pBlock = &vBlocks.emplace_back(nAddress, nBlockSize, nBlockSize);
            }

            Expects(pBlock != nullptr);
            ReadMemory(nAdjustedAddress, pBlock->GetBytes(), nBlockSize, pMemoryBlock);
            pBlock->OptimizeMemory(vBlocks);

            nAddress += nBlockSize;
            nAdjustedAddress += nBlockSize;
            nToRead -= nBlockSize;
        }

        if (nCount == 0)
            break;

        nAdjustedAddress = 0;
    }

    if (nPadding)
    {
        // copy the first four bytes of each block to the last four bytes of the previous
        // block to cover the overlap
        for (size_t i = 1; i < vBlocks.size(); ++i)
        {
            auto& pDstBlock = vBlocks.at(i - 1);
            // ASSERT: pDstBlock->GetBytes() is a size that's a multiple of 4 even if
            //         pDstBlock->GetBytesSize() is not.
            auto* pDstBytes = pDstBlock.GetBytes() + (pDstBlock.GetBytesSize() & ~3);
            const auto* pSrcBytes = vBlocks.at(i).GetBytes();
            memcpy(pDstBytes, pSrcBytes, 4);
        }
    }
}

} // namespace impl
} // namespace context
} // namespace ra
