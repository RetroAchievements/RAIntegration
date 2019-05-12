#ifndef RA_SERVICES_FILELOGGER_HH
#define RA_SERVICES_FILELOGGER_HH
#pragma once

#include "services\IClock.hh"
#include "services\IFileSystem.hh"
#include "services\ILogger.hh"
#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"
#include "services\TextWriter.hh"
#include "services\impl\FileTextWriter.hh"

namespace ra {
namespace services {
namespace impl {

class FileLogger : public ra::services::ILogger
{
public:
    explicit FileLogger(const ra::services::IFileSystem& pFileSystem)
    {
        const std::wstring sLogFilePath = ra::BuildWString(pFileSystem.BaseDirectory().c_str(), L"RACache\\RALog.txt");

        // if the file is over 1MB, rename it and start a new one
        const int64_t nLogSize = pFileSystem.GetFileSize(sLogFilePath);
        if (nLogSize > 1024 * 1024)
        {
            const std::wstring sOldLogFilePath = ra::BuildWString(pFileSystem.BaseDirectory().c_str(), L"RACache\\RALog-old.txt");
            pFileSystem.DeleteFile(sOldLogFilePath);
            pFileSystem.MoveFile(sLogFilePath, sOldLogFilePath);
        }
        else if (nLogSize < 0)
        {
            const std::wstring sCacheDirectory = ra::BuildWString(pFileSystem.BaseDirectory().c_str(), L"RACache");
            if (!pFileSystem.DirectoryExists(sCacheDirectory))
                pFileSystem.CreateDirectory(sCacheDirectory);
        }

        m_pWriter = pFileSystem.AppendTextFile(sLogFilePath);
        if (m_pWriter != nullptr)
            m_pWriter->WriteLine();
    }

    bool IsEnabled([[maybe_unused]] LogLevel level) const noexcept override { return true; }

    void LogMessage(LogLevel level, const std::string& sMessage) const override
    {
        if (m_pWriter == nullptr)
            return;

        // write a timestamp
        time_t tTime{};
        unsigned int tMilliseconds{};
        if (ServiceLocator::Exists<IClock>())
        {
            const auto tNow = ServiceLocator::Get<IClock>().Now();
            tMilliseconds = gsl::narrow_cast<unsigned int>(
                std::chrono::time_point_cast<std::chrono::milliseconds>(tNow).time_since_epoch().count() % 1000);
            tTime = std::chrono::system_clock::to_time_t(tNow);
        }
        else
        {
            tTime = time(nullptr);
            tMilliseconds = 0;
        }

        std::tm tTimeStruct;
        localtime_s(&tTimeStruct, &tTime);

        char sBuffer[16];
        strftime(sBuffer, sizeof(sBuffer), "%H%M%S", &tTimeStruct);
        sprintf_s(&sBuffer[6], sizeof(sBuffer) - 6, ".%03u|", tMilliseconds);

        // WinXP hangs if we try to acquire a mutex while the DLL in initializing. Since DllMain writes
        // a header block to the log file, we have to do that without using a mutex. Luckily, we're not
        // going to have multiple threads trying to write to the file, so it'll be safe, and we can
        // use the presence (or lack thereof) of the ThreadPool implementation to determine if we're
        // being called from DllMain.
        if (ServiceLocator::Exists<IThreadPool>())
        {
            std::scoped_lock<std::mutex> oLock(m_oMutex);
            LogMessage(*m_pWriter, sBuffer, level, sMessage);
        }
        else
        {
            LogMessage(*m_pWriter, sBuffer, level, sMessage);
        }

        // if writing to a file, flush immediately
        auto* pFileWriter = dynamic_cast<ra::services::impl::FileTextWriter*>(m_pWriter.get());
        if (pFileWriter != nullptr)
            pFileWriter->GetFStream().flush();
    }

private:
    static void LogMessage(ra::services::TextWriter& pWriter, const char* const sTimestamp, LogLevel level,
                           const std::string& sMessage)
    {
        pWriter.Write(sTimestamp);

        // mark the level
        switch (level)
        {
            case LogLevel::Info:
                pWriter.Write("INFO");
                break;
            case LogLevel::Warn:
                pWriter.Write("WARN");
                break;
            case LogLevel::Error:
                pWriter.Write("ERR ");
                break;
        }
        pWriter.Write("| ");

        // write the message
        pWriter.Write(sMessage);

        // newline
        pWriter.WriteLine();
    }

    std::unique_ptr<ra::services::TextWriter> m_pWriter;
    mutable std::mutex m_oMutex;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_FILELOGGER_HH
