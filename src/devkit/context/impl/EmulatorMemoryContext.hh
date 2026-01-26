#ifndef RA_CONTEXT_IMPL_EMULATORMEMORYCONTEXT_HH
#define RA_CONTEXT_IMPL_EMULATORMEMORYCONTEXT_HH
#pragma once

#include "context/IEmulatorMemoryContext.hh"

namespace ra {
namespace context {
namespace impl {

class EmulatorMemoryContext : public IEmulatorMemoryContext
{
public:
    EmulatorMemoryContext() noexcept;
    virtual ~EmulatorMemoryContext() noexcept = default;
    EmulatorMemoryContext(const EmulatorMemoryContext&) noexcept = delete;
    EmulatorMemoryContext& operator=(const EmulatorMemoryContext&) noexcept = delete;
    EmulatorMemoryContext(EmulatorMemoryContext&&) noexcept = delete;
    EmulatorMemoryContext& operator=(EmulatorMemoryContext&&) noexcept = delete;

    typedef uint8_t(MemoryReadFunction)(uint32_t nAddress);
    typedef uint32_t(MemoryReadBlockFunction)(uint32_t nAddress, uint8_t* pBuffer, uint32_t nBytes);
    typedef void (MemoryWriteFunction)(uint32_t nAddress, uint8_t nValue);

    /// <summary>
    /// Specifies functions to read and write memory in the emulator.
    /// </summary>
    void AddMemoryBlock(gsl::index nIndex, size_t nBytes, MemoryReadFunction pReader, MemoryWriteFunction pWriter);

    /// <summary>
    /// Specifies functions to read chunks of memory in the emulator.
    /// </summary>
    void AddMemoryBlockReader(gsl::index nIndex, MemoryReadBlockFunction pReader);

    /// <summary>
    /// Clears all registered memory blocks so they can be rebuilt.
    /// </summary>
    void ClearMemoryBlocks();

    /// <summary>
    /// Gets the total amount of emulator-exposed memory.
    /// </summary>
    size_t TotalMemorySize() const noexcept override { return m_nTotalMemorySize; }

    /// <summary>
    /// Determines if any invalid regions were registered.
    /// </summary>
    bool HasInvalidRegions() const noexcept override;

    /// <summary>
    /// Determines if the specified address is valid.
    /// </summary>
    bool IsValidAddress(ra::data::ByteAddress nAddress) const noexcept override;

    /// <summary>
    /// Reads memory from the emulator.
    /// </summary>
    uint32_t ReadMemory(ra::data::ByteAddress nAddress, ra::data::Memory::Size nSize) const override;

    /// <summary>
    /// Reads memory from the emulator.
    /// </summary>
    uint8_t ReadMemoryByte(ra::data::ByteAddress nAddress) const override;

    /// <summary>
    /// Reads memory from the emulator.
    /// </summary>
    uint32_t ReadMemory(ra::data::ByteAddress nAddress, uint8_t pBuffer[], size_t nCount) const override;

    /// <summary>
    /// Writes memory to the emulator.
    /// </summary>
    void WriteMemoryByte(ra::data::ByteAddress nAddress, uint8_t nValue) const override;

    /// <summary>
    /// Writes memory to the emulator.
    /// </summary>
    void WriteMemory(ra::data::ByteAddress nAddress, ra::data::Memory::Size nSize, uint32_t nValue) const override;

    /// <summary>
    /// Captures the current state of memory
    /// </summary>
    void CaptureMemory(std::vector<ra::data::CapturedMemoryBlock>& vMemBlocks, ra::data::ByteAddress nAddress, uint32_t nCount, uint32_t nPadding) const override;

    /// <summary>
    /// Converts an address to a displayable string.
    /// </summary>
    std::wstring FormatAddress(ra::data::ByteAddress nAddress) const override { return m_fFormatAddress(nAddress); }

    /// <summary>
    /// Gets whether or not memory has been modified.
    /// </summary>
    bool WasMemoryModified() const noexcept override { return m_bMemoryModified; }

    /// <summary>
    /// Sets the memory modified flag.
    /// </summary>
    void SetMemoryModified() noexcept override { m_bMemoryModified = true; }

    /// <summary>
    /// Resets the memory modified flag.
    /// </summary>
    void ResetMemoryModified() override;

    /// <summary>
    /// Determines if the memory is considered secure.
    /// </summary>
    bool IsMemoryInsecure() const override;

private:
    void WriteMemory(ra::data::ByteAddress nAddress, const uint8_t* pBytes, size_t nByteCount) const;

protected:
    void OnTotalMemorySizeChanged();
    void AssertIsOnDoFrameThread() const noexcept(false);

    std::function<std::wstring(ra::data::ByteAddress)> m_fFormatAddress;

    struct MemoryBlock
    {
        size_t size;
        MemoryReadFunction* read;
        MemoryWriteFunction* write;
        MemoryReadBlockFunction* readBlock;
    };
    static uint32_t ReadMemory(ra::data::ByteAddress nAddress, uint8_t pBuffer[], size_t nCount, const MemoryBlock& pBlock);

    std::vector<MemoryBlock> m_vMemoryBlocks;
    size_t m_nTotalMemorySize = 0U;
    mutable bool m_bMemoryModified = false;
    mutable bool m_bMemoryInsecure = false;
    mutable std::chrono::steady_clock::time_point m_tLastInsecureCheck{};
};

} // namespace impl
} // namespace context
} // namespace ra

#endif // !RA_CONTEXT_IMPL_EMULATORMEMORYCONTEXT_HH
