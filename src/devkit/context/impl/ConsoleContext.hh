#ifndef RA_CONTEXT_IMPL_CONSOLECONTEXT_HH
#define RA_CONTEXT_IMPL_CONSOLECONTEXT_HH
#pragma once

#include "context/IConsoleContext.hh"

namespace ra {
namespace context {
namespace impl {

class ConsoleContext : public IConsoleContext
{
public:
    GSL_SUPPRESS_F6 ConsoleContext(ConsoleID nId) noexcept;
    virtual ~ConsoleContext() noexcept = default;
    ConsoleContext(const ConsoleContext&) noexcept = delete;
    ConsoleContext& operator=(const ConsoleContext&) noexcept = delete;
    ConsoleContext(ConsoleContext&&) noexcept = delete;
    ConsoleContext& operator=(ConsoleContext&&) noexcept = delete;

    const ra::data::MemoryRegion* GetMemoryRegion(ra::data::ByteAddress nAddress) const override;

    ra::data::ByteAddress ByteAddressFromRealAddress(ra::data::ByteAddress nRealAddress) const noexcept override;
    ra::data::ByteAddress RealAddressFromByteAddress(ra::data::ByteAddress nRealAddress) const noexcept override;
    bool GetRealAddressConversion(ra::data::Memory::Size* nReadSize, uint32_t* nMask, uint32_t* nOffset) const override;
};

} // namespace impl
} // namespace context
} // namespace ra

#endif // !RA_CONTEXT_IMPL_CONSOLECONTEXT_HH
