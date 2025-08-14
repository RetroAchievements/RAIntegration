#include "ConsoleContext.hh"

#include "RA_Log.h"
#include "RA_StringUtils.h"

#include <rc_consoles.h>

namespace ra {
namespace data {
namespace context {

ConsoleContext::ConsoleContext(ConsoleID nId) noexcept
{
    m_nId = nId;
    m_sName = ra::Widen(rc_console_name(ra::etoi(nId)));
    m_nMaxAddress = 0;

    const auto* pRegions = rc_console_memory_regions(ra::etoi(nId));
    if (pRegions)
    {
        for (unsigned i = 0; i < pRegions->num_regions; ++i)
        {
            const auto& pRegion = pRegions->region[i];

            auto& pMemoryRegion = m_vRegions.emplace_back();
            pMemoryRegion.StartAddress = pRegion.start_address;
            pMemoryRegion.EndAddress = pRegion.end_address;
            pMemoryRegion.RealAddress = pRegion.real_address;
            pMemoryRegion.Description = pRegion.description;

            switch (pRegion.type)
            {
                case RC_MEMORY_TYPE_HARDWARE_CONTROLLER:
                    pMemoryRegion.Type = AddressType::HardwareController;
                    break;
                case RC_MEMORY_TYPE_READONLY:
                    pMemoryRegion.Type = AddressType::ReadOnlyMemory;
                    break;
                case RC_MEMORY_TYPE_SAVE_RAM:
                    pMemoryRegion.Type = AddressType::SaveRAM;
                    break;
                case RC_MEMORY_TYPE_SYSTEM_RAM:
                    pMemoryRegion.Type = AddressType::SystemRAM;
                    break;
                case RC_MEMORY_TYPE_UNUSED:
                    pMemoryRegion.Type = AddressType::Unused;
                    break;
                case RC_MEMORY_TYPE_VIDEO_RAM:
                    pMemoryRegion.Type = AddressType::VideoRAM;
                    break;
                case RC_MEMORY_TYPE_VIRTUAL_RAM:
                    pMemoryRegion.Type = AddressType::VirtualRAM;
                    break;
                default:
                    pMemoryRegion.Type = AddressType::Unknown;
                    break;
            }

            m_nMaxAddress = std::max(m_nMaxAddress, pRegion.end_address);
        }
    }
}

const ConsoleContext::MemoryRegion* ConsoleContext::GetMemoryRegion(ra::ByteAddress nAddress) const
{
    const auto& vRegions = MemoryRegions();
    gsl::index nStart = 0;
    gsl::index nEnd = vRegions.size() - 1;
    while (nStart <= nEnd)
    {
        const gsl::index nMid = (nStart + nEnd) / 2;
        const auto& pRegion = vRegions.at(nMid);
        if (pRegion.StartAddress > nAddress)
            nEnd = nMid - 1;
        else if (pRegion.EndAddress < nAddress)
            nStart = nMid + 1;
        else
            return &pRegion;
    }

    return nullptr;
}

ra::ByteAddress ConsoleContext::ByteAddressFromRealAddress(ra::ByteAddress nRealAddress) const noexcept
{
    for (const auto& pRegion : m_vRegions)
    {
        if (pRegion.RealAddress < nRealAddress)
        {
            const auto nRegionSize = pRegion.EndAddress - pRegion.StartAddress;
            if (nRealAddress < pRegion.RealAddress + nRegionSize)
                return (nRealAddress - pRegion.RealAddress) + pRegion.StartAddress;
        }
    }

    // additional mirror/shadow ram mappings not directly exposed by rc_console_memory_regions
    switch (m_nId)
    {
        case ConsoleID::Dreamcast:
            // http://archiv.sega-dc.de/munkeechuff/hardware/Memory.html
            if (nRealAddress >= 0x8C000000 && nRealAddress <= 0x8CFFFFFF) // System Memory (MMU enabled)
                return ByteAddressFromRealAddress(nRealAddress & 0x0FFFFFFF);
            if (nRealAddress >= 0xAC000000 && nRealAddress <= 0xACFFFFFF) // System Memory (cache enabled)
                return ByteAddressFromRealAddress(nRealAddress & 0x0FFFFFFF);
            break;

        case ConsoleID::GameCube:
            // https://wiibrew.org/wiki/Memory_map
            if (nRealAddress >= 0xC0000000 && nRealAddress <= 0xC17FFFFF) // System Memory (uncached)
                return ByteAddressFromRealAddress(nRealAddress - (0xC0000000-0x80000000));
            break;

        case ConsoleID::DSi:
            // https://problemkaputt.de/gbatek.htm#dsiiomap
            if (nRealAddress >= 0x0C000000 && nRealAddress <= 0x0CFFFFFF) // Mirror of Main RAM
                return ByteAddressFromRealAddress(nRealAddress - (0x0C000000-0x02000000));
            break;

        case ConsoleID::PlayStation:
            // https://www.raphnet.net/electronique/psx_adaptor/Playstation.txt
            if (nRealAddress >= 0x80000000 && nRealAddress <= 0x801FFFFF) // Kernel and User Memory Mirror (cached)
                return ByteAddressFromRealAddress(nRealAddress & 0x001FFFFF);
            if (nRealAddress >= 0xA0000000 && nRealAddress <= 0xA01FFFFF) // Kernel and User Memory Mirror (uncached)
                return ByteAddressFromRealAddress(nRealAddress & 0x001FFFFF);
            break;

        case ConsoleID::PlayStation2:
            // https://psi-rockin.github.io/ps2tek/
            if (nRealAddress >= 0x20000000 && nRealAddress <= 0x21FFFFFF) // Main RAM Mirror (uncached)
                return ByteAddressFromRealAddress(nRealAddress & 0x01FFFFFF);
            if (nRealAddress >= 0x30100000 && nRealAddress <= 0x31FFFFFF) // Main RAM Mirror (uncached and accelerated)
                return ByteAddressFromRealAddress(nRealAddress & 0x01FFFFFF);
            break;

        case ConsoleID::WII:
            // https://wiibrew.org/wiki/Memory_map
            if (nRealAddress >= 0xC0000000 && nRealAddress <= 0xC17FFFFF) // System Memory (uncached)
                return ByteAddressFromRealAddress(nRealAddress - (0xC0000000 - 0x80000000));
            if (nRealAddress >= 0xD0000000 && nRealAddress <= 0xD3FFFFFF) // System Memory (uncached)
                return ByteAddressFromRealAddress(nRealAddress - (0xD0000000 - 0x90000000));
            break;

        default:
            break;
    }

    return 0xFFFFFFFF;
}

ra::ByteAddress ConsoleContext::RealAddressFromByteAddress(ra::ByteAddress nByteAddress) const noexcept
{
    for (const auto& pRegion : m_vRegions)
    {
        if (pRegion.EndAddress >= nByteAddress && pRegion.StartAddress <= nByteAddress)
            return nByteAddress + pRegion.RealAddress;
    }

    return 0xFFFFFFFF;
}

bool ConsoleContext::GetRealAddressConversion(MemSize* nReadSize, uint32_t* nMask, uint32_t* nOffset) const
{
    Expects(nReadSize != nullptr);
    Expects(nMask != nullptr);
    Expects(nOffset != nullptr);

    switch (m_nId)
    {
        case ConsoleID::Dreamcast:
        case ConsoleID::DSi:
        case ConsoleID::N64:
        case ConsoleID::PlayStation:
            *nReadSize = MemSize::TwentyFourBit;
            *nMask = 0xFFFFFFFF;
            *nOffset = 0;
            return true;

        case ConsoleID::GameCube:
        case ConsoleID::WII:
            *nReadSize = MemSize::ThirtyTwoBitBigEndian;
            *nMask = 0x01FFFFFF;
            *nOffset = 0;
            return true;

        case ConsoleID::PlayStation2:
        case ConsoleID::PSP:
            *nReadSize = MemSize::ThirtyTwoBit;
            *nMask = 0x01FFFFFF;
            *nOffset = 0;
            return true;

        default:
            for (const auto& pRegion : m_vRegions)
            {
                if (pRegion.Type == AddressType::SystemRAM)
                {
                    *nOffset = (pRegion.RealAddress - pRegion.StartAddress);
                    *nMask = 0xFFFFFFFF;

                    if (m_nMaxAddress > 0x00FFFFFF)
                        *nReadSize = MemSize::ThirtyTwoBit;
                    else if (m_nMaxAddress > 0x0000FFFF)
                        *nReadSize = MemSize::TwentyFourBit;
                    else if (m_nMaxAddress > 0x000000FF)
                        *nReadSize = MemSize::SixteenBit;
                    else
                        *nReadSize = MemSize::EightBit;

                    return true;
                }
            }

            *nReadSize = MemSize::ThirtyTwoBit;
            *nMask = 0xFFFFFFFF;
            *nOffset = 0;
            return false;
    }
}

} // namespace context
} // namespace data
} // namespace ra
