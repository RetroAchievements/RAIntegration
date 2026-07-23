#ifndef RA_CONTEXT_IDEVKITCONTEXT_HH
#define RA_CONTEXT_IDEVKITCONTEXT_HH
#pragma once

#include <string>

namespace ra {
namespace context {

class IDevKitContext
{
public:
    IDevKitContext() noexcept = default;
    virtual ~IDevKitContext() noexcept = default;
    IDevKitContext(const IDevKitContext&) noexcept = delete;
    IDevKitContext& operator=(const IDevKitContext&) noexcept = delete;
    IDevKitContext(IDevKitContext&&) noexcept = delete;
    IDevKitContext& operator=(IDevKitContext&&) noexcept = delete;

    /// <summary>
    /// Gets the version of the devkit library.
    /// </summary>
    virtual const std::string& Version() const noexcept = 0;
};

} // namespace context
} // namespace ra

#endif // !RA_CONTEXT_USERCONTEXT_HH
