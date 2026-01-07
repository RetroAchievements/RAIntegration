#ifndef RA_CONTEXT_IEMULATORMEMORYCONTEXT_HH
#define RA_CONTEXT_IEMULATORMEMORYCONTEXT_HH
#pragma once

#include "data/Memory.hh"

#include <chrono>

#include "data/CapturedMemoryBlock.hh"
#include "data/NotifyTargetSet.hh"

namespace ra {
namespace context {

class IEmulatorMemoryContext
{
public:
    virtual ~IEmulatorMemoryContext() noexcept = default;
    IEmulatorMemoryContext(const IEmulatorMemoryContext&) noexcept = delete;
    IEmulatorMemoryContext& operator=(const IEmulatorMemoryContext&) noexcept = delete;
    IEmulatorMemoryContext(IEmulatorMemoryContext&&) noexcept = delete;
    IEmulatorMemoryContext& operator=(IEmulatorMemoryContext&&) noexcept = delete;

    /// <summary>
    /// Gets the total amount of emulator-exposed memory.
    /// </summary>
    virtual size_t TotalMemorySize() const noexcept = 0;

    /// <summary>
    /// Determines if any invalid regions were registered.
    /// </summary>
    virtual bool HasInvalidRegions() const noexcept = 0;

    /// <summary>
    /// Determines if the specified address is valid.
    /// </summary>
    virtual bool IsValidAddress(ra::data::ByteAddress nAddress) const noexcept = 0;

    /// <summary>
    /// Reads memory from the emulator.
    /// </summary>
    virtual uint32_t ReadMemory(ra::data::ByteAddress nAddress, ra::data::Memory::Size nSize) const = 0;

    /// <summary>
    /// Reads memory from the emulator.
    /// </summary>
    virtual uint8_t ReadMemoryByte(ra::data::ByteAddress nAddress) const = 0;

    /// <summary>
    /// Reads memory from the emulator.
    /// </summary>
    virtual uint32_t ReadMemory(ra::data::ByteAddress nAddress, uint8_t pBuffer[], size_t nCount) const = 0;

    /// <summary>
    /// Writes memory to the emulator.
    /// </summary>
    virtual void WriteMemoryByte(ra::data::ByteAddress nAddress, uint8_t nValue) const = 0;

    /// <summary>
    /// Writes memory to the emulator.
    /// </summary>
    virtual void WriteMemory(ra::data::ByteAddress nAddress, ra::data::Memory::Size nSize, uint32_t nValue) const = 0;

    /// <summary>
    /// Captures the current state of memory
    /// </summary>
    virtual void CaptureMemory(std::vector<ra::data::CapturedMemoryBlock>& vMemBlocks, ra::data::ByteAddress nAddress, uint32_t nCount, uint32_t nPadding) const = 0;

    /// <summary>
    /// Converts an address to a displayable string.
    /// </summary>
    virtual std::string FormatAddress(ra::data::ByteAddress nAddress) const = 0;

    /// <summary>
    /// Gets whether or not memory has been modified.
    /// </summary>
    virtual bool WasMemoryModified() const noexcept = 0;

    /// <summary>
    /// Sets the memory modified flag.
    /// </summary>
    virtual void SetMemoryModified() noexcept = 0;

    /// <summary>
    /// Resets the memory modified flag.
    /// </summary>
    virtual void ResetMemoryModified() = 0;

    /// <summary>
    /// Determines if the memory is considered secure.
    /// </summary>
    virtual bool IsMemoryInsecure() const = 0;

    class NotifyTarget
    {
    public:
        NotifyTarget() noexcept = default;
        virtual ~NotifyTarget() noexcept = default;
        NotifyTarget(const NotifyTarget&) noexcept = delete;
        NotifyTarget& operator=(const NotifyTarget&) noexcept = delete;
        NotifyTarget(NotifyTarget&&) noexcept = default;
        NotifyTarget& operator=(NotifyTarget&&) noexcept = default;

        virtual void OnTotalMemorySizeChanged() noexcept(false) {}
        virtual void OnByteWritten(ra::data::ByteAddress, uint8_t) noexcept(false) {}
    };

    void AddNotifyTarget(NotifyTarget& pTarget) noexcept { m_vNotifyTargets.Add(pTarget); }
    void RemoveNotifyTarget(NotifyTarget& pTarget) noexcept { m_vNotifyTargets.Remove(pTarget); }

protected:
    IEmulatorMemoryContext() noexcept = default;

    mutable ra::data::NotifyTargetSet<NotifyTarget> m_vNotifyTargets;
};

} // namespace context
} // namespace ra

#endif // !RA_CONTEXT_IEMULATORMEMORYCONTEXT_HH
