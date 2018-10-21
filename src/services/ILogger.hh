#ifndef RA_SERVICES_ILOGGER_HH
#define RA_SERVICES_ILOGGER_HH
#pragma once

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
    ILogger(const ILogger&) noexcept = delete;
    ILogger& operator=(const ILogger&) noexcept = delete;
    ILogger(ILogger&&) noexcept = delete;
    ILogger& operator=(ILogger&&) noexcept = delete;

    /// <summary>
    /// Determines if a message should be logged for a given level.
    /// </summary>
    virtual bool IsEnabled(LogLevel level) const = 0;

    /// <summary>
    /// Logs a message.
    /// </summary>
    virtual void LogMessage(LogLevel level, const std::string& sMessage) const = 0;

protected:
    ILogger() noexcept = default;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_ILOGGER_HH
