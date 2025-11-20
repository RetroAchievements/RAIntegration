#ifndef RA_SERVICES_IMESSAGE_DISPATCHER_HH
#define RA_SERVICES_IMESSAGE_DISPATCHER_HH
#pragma once

#include <string>

namespace ra {
namespace services {

class IMessageDispatcher
{
public:
    virtual ~IMessageDispatcher() noexcept = default;
    IMessageDispatcher(const IMessageDispatcher&) noexcept = delete;
    IMessageDispatcher& operator=(const IMessageDispatcher&) noexcept = delete;
    IMessageDispatcher(IMessageDispatcher&&) noexcept = delete;
    IMessageDispatcher& operator=(IMessageDispatcher&&) noexcept = delete;

    /// <summary>
    /// Dispatches a message.
    /// </summary>
    virtual void ReportMessage(const std::wstring& sSummary, const std::wstring& sDetail) const = 0;

    /// <summary>
    /// Dispatches an error message.
    /// </summary>
    virtual void ReportErrorMessage(const std::wstring& sSummary, const std::wstring& sDetail) const = 0;

protected:
    IMessageDispatcher() noexcept = default;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_IMESSAGE_DISPATCHER_HH
