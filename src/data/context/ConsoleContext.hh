#ifndef RA_DATA_CONSOLECONTEXT_HH
#define RA_DATA_CONSOLECONTEXT_HH
#pragma once

#include "RAInterface\RA_Consoles.h"
#include "data\Types.hh"
#include "util\GSL.hh"

#include <string>

namespace ra {
namespace data {
namespace context {

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
        ra::ByteAddress RealAddress = 0U;
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

    /// <summary>
    /// Converts a real address into the "RetroAchievements" address where the data should be.
    /// </summary>
    /// <returns>Converted address, or <c>0xFFFFFFFF</c> if conversion could not be completed.
    ra::ByteAddress ByteAddressFromRealAddress(ra::ByteAddress nRealAddress) const noexcept;

    /// <summary>
    /// Converts an "RetroAchievements" address into a real address where the data might be found.
    /// </summary>
    /// <returns>Converted address, or <c>0xFFFFFFFF</c> if conversion could not be completed.
    ra::ByteAddress RealAddressFromByteAddress(ra::ByteAddress nRealAddress) const noexcept;

    /// <summary>
    /// Gets the read size and mask to use for reading a pointer from memory and converting it to
    /// a RetroAchievements address.
    /// </summary>
    /// <param name="nReadSize">[out] the size to read.</param>
    /// <param name="nMask">[out] the mask to apply (if 0xFFFFFFFF, no mask is necessary).</param>
    /// <param name="nOffset">[out] the offset to apply (if 0, no offset is necessary).</param>
    /// <returns><c>true<c> if a mapping was found, <c>false</c> if not.</returns>
    bool GetRealAddressConversion(MemSize* nReadSize, uint32_t* nMask, uint32_t* nOffset) const;

    /// <summary>
    /// Gets the maximum valid address for the console.
    /// </summary>
    ra::ByteAddress MaxAddress() const noexcept { return m_nMaxAddress; }

protected:
    ConsoleID m_nId{};
    std::wstring m_sName;

    std::vector<MemoryRegion> m_vRegions;
    ra::ByteAddress m_nMaxAddress = 0;
};

} // namespace context
} // namespace data
} // namespace ra

#endif // !RA_DATA_CONSOLECONTEXT_HH
