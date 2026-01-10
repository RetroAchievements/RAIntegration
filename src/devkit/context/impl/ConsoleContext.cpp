#include "ConsoleContext.hh"

#include "util/Strings.hh"

#include <rc_consoles.h>

namespace ra {
namespace context {
namespace impl {

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

            auto& pMemoryRegion = m_vRegions.emplace_back(pRegion.start_address, pRegion.end_address, ra::Widen(pRegion.description));
            pMemoryRegion.SetRealStartAddress(pRegion.real_address);

            switch (pRegion.type)
            {
                case RC_MEMORY_TYPE_HARDWARE_CONTROLLER:
                    pMemoryRegion.SetType(ra::data::MemoryRegion::Type::HardwareController);
                    break;
                case RC_MEMORY_TYPE_READONLY:
                    pMemoryRegion.SetType(ra::data::MemoryRegion::Type::ReadOnlyMemory);
                    break;
                case RC_MEMORY_TYPE_SAVE_RAM:
                    pMemoryRegion.SetType(ra::data::MemoryRegion::Type::SaveRAM);
                    break;
                case RC_MEMORY_TYPE_SYSTEM_RAM:
                    pMemoryRegion.SetType(ra::data::MemoryRegion::Type::SystemRAM);
                    break;
                case RC_MEMORY_TYPE_UNUSED:
                    pMemoryRegion.SetType(ra::data::MemoryRegion::Type::Unused);
                    break;
                case RC_MEMORY_TYPE_VIDEO_RAM:
                    pMemoryRegion.SetType(ra::data::MemoryRegion::Type::VideoRAM);
                    break;
                case RC_MEMORY_TYPE_VIRTUAL_RAM:
                    pMemoryRegion.SetType(ra::data::MemoryRegion::Type::VirtualRAM);
                    break;
                default:
                    pMemoryRegion.SetType(ra::data::MemoryRegion::Type::Unknown);
                    break;
            }

            m_nMaxAddress = std::max(m_nMaxAddress, pRegion.end_address);
        }
    }
}

const ra::data::MemoryRegion* ConsoleContext::GetMemoryRegion(ra::data::ByteAddress nAddress) const
{
    const auto& vRegions = MemoryRegions();
    gsl::index nStart = 0;
    gsl::index nEnd = vRegions.size() - 1;
    while (nStart <= nEnd)
    {
        const gsl::index nMid = (nStart + nEnd) / 2;
        const auto& pRegion = vRegions.at(nMid);
        if (pRegion.GetStartAddress() > nAddress)
            nEnd = nMid - 1;
        else if (pRegion.GetEndAddress() < nAddress)
            nStart = nMid + 1;
        else
            return &pRegion;
    }

    return nullptr;
}

ra::data::ByteAddress ConsoleContext::ByteAddressFromRealAddress(ra::data::ByteAddress nRealAddress) const noexcept
{
    for (const auto& pRegion : m_vRegions)
    {
        if (pRegion.ContainsRealAddress(nRealAddress))
        {
            if (pRegion.GetType() == ra::data::MemoryRegion::Type::Unused)
                break;

            return (nRealAddress - pRegion.GetRealStartAddress()) + pRegion.GetStartAddress();
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

ra::data::ByteAddress ConsoleContext::RealAddressFromByteAddress(ra::data::ByteAddress nByteAddress) const noexcept
{
    for (const auto& pRegion : m_vRegions)
    {
        if (pRegion.ContainsAddress(nByteAddress))
            return pRegion.GetRealStartAddress() + nByteAddress;
    }

    return 0xFFFFFFFF;
}

bool ConsoleContext::GetRealAddressConversion(ra::data::Memory::Size* nReadSize, uint32_t* nMask, uint32_t* nOffset) const
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
            *nReadSize = ra::data::Memory::Size::TwentyFourBit;
            *nMask = 0xFFFFFFFF;
            *nOffset = 0;
            return true;

        case ConsoleID::GameCube:
            *nReadSize = ra::data::Memory::Size::ThirtyTwoBitBigEndian;
            *nMask = 0x01FFFFFF;
            *nOffset = 0;
            return true;

        case ConsoleID::WII:
            *nReadSize = ra::data::Memory::Size::ThirtyTwoBitBigEndian;
            *nMask = 0x1FFFFFFF;
            *nOffset = 0;
            return true;

        case ConsoleID::PlayStation2:
        case ConsoleID::PSP:
            *nReadSize = ra::data::Memory::Size::ThirtyTwoBit;
            *nMask = 0x01FFFFFF;
            *nOffset = 0;
            return true;

        case ConsoleID::GBA:
            // This is technically wrong as there's two distinct maps required.
            //  $03000000 -> $00000000 (via just doing a 24-bit read)
            //  $02000000 -> $00008000 (via an offset)
            // However, most developers who have implemented sets just do a 24-bit
            // read _and_ use a +0x8000 offset when necessary. As these offsets are
            // encoded in the code notes, we shouldn't provide an explicit offset here.
            *nReadSize = ra::data::Memory::Size::TwentyFourBit;
            *nMask = 0x00FFFFFF;
            *nOffset = 0;
            return true;

        default:
            for (const auto& pRegion : m_vRegions)
            {
                if (pRegion.GetType() == ra::data::MemoryRegion::Type::SystemRAM)
                {
                    if (m_nMaxAddress > 0x00FFFFFF)
                    {
                        *nReadSize = ra::data::Memory::Size::ThirtyTwoBit;
                        *nMask = 0xFFFFFFFF;
                    }
                    else if (m_nMaxAddress > 0x0000FFFF)
                    {
                        *nReadSize = ra::data::Memory::Size::TwentyFourBit;
                        *nMask = 0x00FFFFFF;
                    }
                    else if (m_nMaxAddress > 0x000000FF)
                    {
                        *nReadSize = ra::data::Memory::Size::SixteenBit;
                        *nMask = 0x0000FFFF;
                    }
                    else
                    {
                        *nReadSize = ra::data::Memory::Size::EightBit;
                        *nMask = 0x000000FF;
                    }

                    *nOffset = (pRegion.GetRealStartAddress() - pRegion.GetStartAddress()) & *nMask;
                    return true;
                }
            }

            *nReadSize = ra::data::Memory::Size::ThirtyTwoBit;
            *nMask = 0xFFFFFFFF;
            *nOffset = 0;
            return false;
    }
}

} // namespace impl
} // namespace context
} // namespace ra
