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

    return 0xFFFFFFFF;
}


} // namespace context
} // namespace data
} // namespace ra
