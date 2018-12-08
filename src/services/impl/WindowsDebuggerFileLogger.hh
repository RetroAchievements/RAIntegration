#ifndef RA_SERVICES_WINDOWS_DEBUGGER_FILELOGGER_HH
#define RA_SERVICES_WINDOWS_DEBUGGER_FILELOGGER_HH
#pragma once

#include "services\impl\FileLogger.hh"

namespace ra {
namespace services {
namespace impl {

class WindowsDebuggerFileLogger : public FileLogger
{
public:
    explicit WindowsDebuggerFileLogger(const ra::services::IFileSystem& pFileSystem)
        : FileLogger(pFileSystem)
    {
    }

    void LogMessage(LogLevel level, const std::string& sMessage) const override
    {
        FileLogger::LogMessage(level, sMessage);

        OutputDebugStringA(sMessage.c_str());
        OutputDebugStringA("\n");
    }
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_WINDOWS_DEBUGGER_FILELOGGER_HH
