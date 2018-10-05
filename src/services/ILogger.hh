#ifndef RA_SERVICES_ILOGGER_HH
#define RA_SERVICES_ILOGGER_HH
#pragma once

#include <string>

namespace ra {
namespace services {

enum class LogLevel
{
    Info,
    Warn,
    Error,
};

class ILogger
{
public:
    virtual ~ILogger() noexcept = default;

    /// <summary>
    /// Determines if a message should be logged for a given level.
    /// </summary>
    virtual bool IsEnabled(LogLevel level) const = 0;

    /// <summary>
    /// Logs a message.
    /// </summary>
    virtual void LogMessage(LogLevel level, const std::string& sMessage) const = 0;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_ILOGGER_HH
