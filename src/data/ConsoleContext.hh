#ifndef RA_DATA_CONSOLECONTEXT_HH
#define RA_DATA_CONSOLECONTEXT_HH
#pragma once

#include "RAInterface\RA_Consoles.h"
#include "ra_fwd.h"
#include "data\Types.hh"

#include <string>

namespace ra {
namespace data {

class ConsoleContext
{
public:
    GSL_SUPPRESS_F6 ConsoleContext(ConsoleID nId) noexcept;
    virtual ~ConsoleContext() noexcept = default;
    ConsoleContext(const ConsoleContext&) noexcept = delete;
    ConsoleContext& operator=(const ConsoleContext&) noexcept = delete;
    ConsoleContext(ConsoleContext&&) noexcept = delete;
    ConsoleContext& operator=(ConsoleContext&&) noexcept = delete;
    
    /// <summary>
    /// Gets the unique identifier of the console.
    /// </summary>
    ConsoleID Id() const noexcept { GSL_SUPPRESS_ENUM3 return m_nId; }

    /// <summary>
    /// Gets a descriptive name for the console.
    /// </summary>
    const std::wstring& Name() const noexcept { return m_sName; }

    enum class AddressType
    {        
        /// <summary>
        /// Address use is unknown.
        /// </summary>
        Unknown,

        /// <summary>
        /// RAM that persists between sessions. Typically found on a cratridge backed up by a battery.
        /// </summary>
        SaveRAM,

        /// <summary>
        /// Generic RAM available for temporary storage.
        /// </summary>
        SystemRAM,

        /// <summary>
        /// Secondary address space that maps to real memory in SystemRAM. Also known as Echo RAM or Mirror RAM.
        /// </summary>
        VirtualRAM,

        /// <summary>
        /// RAM reserved for graphical processing.
        /// </summary>
        VideoRAM,

        /// <summary>
        /// Address reserved for interacting with system components.
        /// </summary>
        HardwareController,

        /// <summary>
        /// Address space that maps to some segment of read only memory (typically ROM chips on the loaded cartridge)
        /// </summary>
        ReadOnlyMemory,

        /// <summary>
        /// Address space that doesn't actually map to memory.
        /// </summary>
        Unused,
    };

    struct MemoryRegion
    {
        ra::ByteAddress StartAddress = 0U;
        ra::ByteAddress EndAddress = 0U;
        AddressType Type = AddressType::Unknown;
        std::string Description;
    };

    /// <summary>
    /// Gets the list of memory regions mapped for the system.
    /// </summary>
    const std::vector<MemoryRegion>& MemoryRegions() const noexcept { return m_vRegions; }

    /// <summary>
    /// Gets the memory region containing the specified address.
    /// </summary>
    /// <returns>Matching <see cref="MemoryRegion"/>, <c>nullptr</c> if not found.
    const MemoryRegion* GetMemoryRegion(ra::ByteAddress nAddress) const;

protected:
    ConsoleID m_nId{};
    std::wstring m_sName;

    std::vector<MemoryRegion> m_vRegions;
};

} // namespace data
} // namespace ra

#endif // !RA_DATA_CONSOLECONTEXT_HH
