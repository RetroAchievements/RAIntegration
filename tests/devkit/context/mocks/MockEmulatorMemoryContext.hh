#ifndef RA_CONTEXT_MOCK_EMULATORMEMORYCONTEXT_HH
#define RA_CONTEXT_MOCK_EMULATORMEMORYCONTEXT_HH
#pragma once

#include "context/impl/EmulatorMemoryContext.hh"

#include "services/ServiceLocator.hh"

#pragma warning(push)
#pragma warning(disable: 26440)
#include <GSL/span>
#pragma warning(pop)

#include <map>

namespace ra {
namespace context {
namespace mocks {

class MockEmulatorMemoryContext : public impl::EmulatorMemoryContext
{
public:
    MockEmulatorMemoryContext() noexcept
        : m_Override(this)
    {
    }

    template <size_t N>
    void MockMemory(std::array<uint8_t, N>& arr)
    {
        MockMemory(arr.data(), N);
    }

    void MockMemory(uint8_t pMemory[], size_t nBytes);

    /// <summary>
    /// Mocks a specific memory address without having to allocate the entire memory space
    /// </summary>
    void MockMemoryValue(uint32_t nAddress, uint32_t nValue) 
    {
        m_mMemoryValues[nAddress] = nValue;

        if (gsl::narrow_cast<size_t>(nAddress) + 4 > m_nTotalMemorySize)
        {
            const size_t nNewMemorySize = gsl::narrow_cast<size_t>(nAddress) + 4;
            ClearMemoryBlocks();
            AddMemoryBlock(0, nNewMemorySize, ReadMemoryHelper, WriteMemoryHelper);
        }
    }

    void MockMemoryModified(bool bModified) noexcept
    {
        m_bMemoryModified = bModified;
    }

    void MockTotalMemorySizeChanged(size_t nNewSize);

    void MockMemoryInsecure(bool bValue) noexcept { m_bMemoryInsecure = bValue; }
    bool IsMemoryInsecure() const noexcept override { return m_bMemoryInsecure; }

private:
    static uint8_t ReadMemoryHelper(uint32_t nAddress);
    static void WriteMemoryHelper(uint32_t nAddress, uint8_t nValue);

    ra::services::ServiceLocator::ServiceOverride<ra::context::IEmulatorMemoryContext> m_Override;
    uint8_t* m_pMemory = nullptr;

    std::map<uint32_t, uint32_t> m_mMemoryValues;
};

} // namespace mocks
} // namespace context
} // namespace ra

#endif // !RA_CONTEXT_MOCK_EMULATORMEMORYCONTEXT_HH
