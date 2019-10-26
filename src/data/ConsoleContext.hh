#ifndef RA_DATA_CONSOLECONTEXT_HH
#define RA_DATA_CONSOLECONTEXT_HH
#pragma once

#include "RA_Interface.h"
#include "ra_fwd.h"
#include "data\Types.hh"

#include <string>

namespace ra {
namespace data {

class ConsoleContext
{
public:
    GSL_SUPPRESS_F6 GSL_SUPPRESS_ENUM3 
        ConsoleContext(ConsoleID nId, std::wstring&& sName) noexcept : m_nId(nId), m_sName(std::move(sName)) {}
    virtual ~ConsoleContext() noexcept = default;
    ConsoleContext(const ConsoleContext&) noexcept = delete;
    ConsoleContext& operator=(const ConsoleContext&) noexcept = delete;
    ConsoleContext(ConsoleContext&&) noexcept = delete;
    ConsoleContext& operator=(ConsoleContext&&) noexcept = delete;
    
    /// <summary>
    /// Gets the unique identifier of the console.
    /// </summary>
    ConsoleID Id() const noexcept { return m_nId; }

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
    };

    struct MemoryRegion
    {
        ra::ByteAddress StartAddress;
        ra::ByteAddress EndAddress;
        AddressType Type;
        std::string Description;
    };

    /// <summary>
    /// Gets the list of memory regions mapped for the system.
    /// </summary>
    virtual const std::vector<MemoryRegion>& MemoryRegions() const noexcept { return m_vEmptyRegions; }

    /// <summary>
    /// Gets the memory region containing the specified address.
    /// </summary>
    /// <returns>Matching <see cref="MemoryRegion"/>, <c>nullptr</c> if not found.
    const MemoryRegion* GetMemoryRegion(ra::ByteAddress nAddress) const;

    /// <summary>
    /// Gets a context object for the specified console.
    /// </summary>
    static std::unique_ptr<ConsoleContext> GetContext(ConsoleID nId);

protected:
    ConsoleID m_nId{};
    std::wstring m_sName;

    static const std::vector<MemoryRegion> m_vEmptyRegions;
};

} // namespace data
} // namespace ra

#endif // !RA_DATA_CONSOLECONTEXT_HH
