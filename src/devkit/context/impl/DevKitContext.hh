#ifndef RA_CONTEXT_IMPL_DEVKITCONTEXT_HH
#define RA_CONTEXT_IMPL_DEVKITCONTEXT_HH
#pragma once

#include "context/IDevKitContext.hh"

namespace ra {
namespace context {
namespace impl {

class DevKitContext : public IDevKitContext
{
public:
    DevKitContext() noexcept;
    virtual ~DevKitContext() noexcept = default;
    DevKitContext(const DevKitContext&) noexcept = delete;
    DevKitContext& operator=(const DevKitContext&) noexcept = delete;
    DevKitContext(DevKitContext&&) noexcept = delete;
    DevKitContext& operator=(DevKitContext&&) noexcept = delete;

    /// <summary>
    /// Gets the version of the devkit library.
    /// </summary>
    const std::string& Version() const noexcept override { return m_sVersion; }

private:
    std::string m_sVersion;
};

} // namespace impl
} // namespace context
} // namespace ra

#endif // !RA_CONTEXT_IMPL_DEVKITCONTEXT_HH
