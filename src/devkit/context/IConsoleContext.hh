#ifndef RA_CONTEXT_ICONSOLECONTEXT_HH
#define RA_CONTEXT_ICONSOLECONTEXT_HH
#pragma once

#include "RAInterface/RA_Consoles.h"
#include "data/MemoryRegion.hh"
#include "util/GSL.hh"

#include <string>

namespace ra {
namespace context {

class IConsoleContext
{
public:
    IConsoleContext() noexcept = default;
    virtual ~IConsoleContext() noexcept = default;
    IConsoleContext(const IConsoleContext&) noexcept = delete;
    IConsoleContext& operator=(const IConsoleContext&) noexcept = delete;
    IConsoleContext(IConsoleContext&&) noexcept = delete;
    IConsoleContext& operator=(IConsoleContext&&) noexcept = delete;
    
    /// <summary>
    /// Gets the unique identifier of the console.
    /// </summary>
    ConsoleID Id() const noexcept { GSL_SUPPRESS_ENUM3 return m_nId; }

    /// <summary>
    /// Gets a descriptive name for the console.
    /// </summary>
    const std::wstring& Name() const noexcept { return m_sName; }

    /// <summary>
    /// Gets the list of memory regions mapped for the system.
    /// </summary>
    const std::vector<ra::data::MemoryRegion>& MemoryRegions() const noexcept { return m_vRegions; }

    /// <summary>
    /// Gets the memory region containing the specified address.
    /// </summary>
    /// <returns>Matching <see cref="MemoryRegion"/>, <c>nullptr</c> if not found.
    virtual const ra::data::MemoryRegion* GetMemoryRegion(ra::data::ByteAddress nAddress) const = 0;

    /// <summary>
    /// Converts a real address into the "RetroAchievements" address where the data should be.
    /// </summary>
    /// <returns>Converted address, or <c>0xFFFFFFFF</c> if conversion could not be completed.
    virtual ra::data::ByteAddress ByteAddressFromRealAddress(ra::data::ByteAddress nByteAddress) const noexcept
    {
        return nByteAddress;
    }

    /// <summary>
    /// Converts an "RetroAchievements" address into a real address where the data might be found.
    /// </summary>
    /// <returns>Converted address, or <c>0xFFFFFFFF</c> if conversion could not be completed.
    virtual ra::data::ByteAddress RealAddressFromByteAddress(ra::data::ByteAddress nRealAddress) const noexcept
    {
        return nRealAddress;
    }

    /// <summary>
    /// Gets the read size and mask to use for reading a pointer from memory and converting it to
    /// a RetroAchievements address.
    /// </summary>
    /// <param name="nReadSize">[out] the size to read.</param>
    /// <param name="nMask">[out] the mask to apply (if 0xFFFFFFFF, no mask is necessary).</param>
    /// <param name="nOffset">[out] the offset to apply (if 0, no offset is necessary).</param>
    /// <returns><c>true<c> if a mapping was found, <c>false</c> if not.</returns>
    virtual bool GetRealAddressConversion(ra::data::Memory::Size* nReadSize, uint32_t* nMask, uint32_t* nOffset) const
    {
        Expects(nReadSize != nullptr);
        Expects(nMask != nullptr);
        Expects(nOffset != nullptr);

        *nReadSize = ra::data::Memory::Size::ThirtyTwoBit;
        *nMask = 0xFFFFFFFF;
        *nOffset = 0;
        return false;
    }

    /// <summary>
    /// Gets the maximum valid address for the console.
    /// </summary>
    ra::data::ByteAddress MaxAddress() const noexcept { return m_nMaxAddress; }

protected:
    ConsoleID m_nId{};
    std::wstring m_sName;

    std::vector<ra::data::MemoryRegion> m_vRegions;
    ra::data::ByteAddress m_nMaxAddress = 0;
};

} // namespace context
} // namespace ra

#endif // !RA_CONTEXT_ICONSOLECONTEXT_HH
