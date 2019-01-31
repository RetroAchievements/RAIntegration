#ifndef RA_DATA_CONSOLECONTEXT_HH
#define RA_DATA_CONSOLECONTEXT_HH
#pragma once

#include "RA_Interface.h"

#include <string>

namespace ra {
namespace data {

class ConsoleContext
{
public:
    ConsoleContext(ConsoleID nId, const std::wstring&& sName) noexcept : m_nId(nId), m_sName(std::move(sName)) {}
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

    /// <summary>
    /// Gets a context object for the specified console.
    /// </summary>
    static std::unique_ptr<ConsoleContext> GetContext(ConsoleID nId);

protected:
    ConsoleID m_nId{};
    std::wstring m_sName;
};

} // namespace data
} // namespace ra

#endif // !RA_DATA_CONSOLECONTEXT_HH
