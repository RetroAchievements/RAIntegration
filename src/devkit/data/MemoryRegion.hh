#ifndef RA_DATA_MEMORYREGION_H
#define RA_DATA_MEMORYREGION_H
#pragma once

#include "Memory.hh"

namespace ra {
namespace data {

class MemoryRegion
{
public:
    MemoryRegion(ra::data::ByteAddress nStartAddress, ra::data::ByteAddress nEndAddress, const std::wstring& sDescription)
        : m_sDescription(sDescription), m_nStartAddress(nStartAddress), m_nEndAddress(nEndAddress)
    {
    }

    ~MemoryRegion() = default;
    MemoryRegion(const MemoryRegion&) noexcept = default;
    MemoryRegion& operator=(const MemoryRegion&) noexcept = default;
    MemoryRegion(MemoryRegion&&) noexcept = default;
    MemoryRegion& operator=(MemoryRegion&&) noexcept = default;

    enum class Type
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

    /// <summary>
    /// Gets the description of the region.
    /// </summary>
    const std::wstring& GetDescription() const noexcept { return m_sDescription; }

    /// <summary>
    /// Gets the first RetroAchievements address of the region.
    /// </summary>
    const ra::data::ByteAddress GetStartAddress() const noexcept { return m_nStartAddress; }

    /// <summary>
    /// Gets the last RetroAchievements address of the region.
    /// </summary>
    const ra::data::ByteAddress GetEndAddress() const noexcept { return m_nEndAddress; }

    /// <summary>
    /// Sets the last RetroAchievements address of the region.
    /// </summary>
    void SetEndAddress(ra::data::ByteAddress nAddress) noexcept { m_nEndAddress = nAddress; }

    /// <summary>
    /// Gets the number of addresses in the region.
    /// </summary>
    const uint32_t GetSize() const noexcept { return m_nEndAddress - m_nStartAddress + 1; }

    /// <summary>
    /// Determines if an RetroAchievements address is in the region.
    /// </summary>
    bool ContainsAddress(ra::data::ByteAddress nAddress) const noexcept
    {
        return (nAddress >= m_nStartAddress && nAddress <= m_nEndAddress);
    }

    /// <summary>
    /// Gets the type of the region.
    /// </summary>
    const Type GetType() const noexcept { return m_nType; }

    /// <summary>
    /// Sets the type of the region.
    /// </summary>
    void SetType(Type nType) noexcept { m_nType = nType; }

    /// <summary>
    /// Gets the first real address of the region.
    /// </summary>
    const ra::data::ByteAddress GetRealStartAddress() const noexcept { return m_nRealStartAddress; }

    /// <summary>
    /// Sets the first real address of the region.
    /// </summary>
    void SetRealStartAddress(ra::data::ByteAddress nAddress) noexcept { m_nRealStartAddress = nAddress; }

    /// <summary>
    /// Determines if a real address is in the region.
    /// </summary>
    bool ContainsRealAddress(ra::data::ByteAddress nAddress) const noexcept
    {
        return (nAddress >= m_nRealStartAddress && 
                nAddress <= m_nRealStartAddress + (m_nEndAddress - m_nStartAddress));
    }

private:
    std::wstring m_sDescription;
    ra::data::ByteAddress m_nStartAddress = 0U;
    ra::data::ByteAddress m_nEndAddress = 0U;
    ra::data::ByteAddress m_nRealStartAddress = 0U;
    Type m_nType = Type::Unknown;
};

} // namespace data
} // namespace ra

#endif RA_DATA_MEMORYREGION_H
