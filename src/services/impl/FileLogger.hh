#ifndef RA_SERVICES_FILELOGGER_HH
#define RA_SERVICES_FILELOGGER_HH
#pragma once

#include "services\IClock.hh"
#include "services\IFileSystem.hh"
#include "services\ILogger.hh"
#include "services\ServiceLocator.hh"
#include "services\TextWriter.hh"
#include "services\impl\FileTextWriter.hh"

namespace ra {
namespace services {
namespace impl {

class FileLogger : public ra::services::ILogger
{
public:
    explicit FileLogger(const ra::services::IFileSystem& pFileSystem) noexcept
    {
        m_pWriter = pFileSystem.AppendTextFile(pFileSystem.BaseDirectory() + L"RACache\\RALog.txt");
        if (m_pWriter != nullptr)
            m_pWriter->WriteLine();
    }

    bool IsEnabled([[maybe_unused]] LogLevel level) const override
    {
        return true;
    }

    void LogMessage(LogLevel level, const std::string& sMessage) const override
    {
        if (m_pWriter == nullptr)
            return;

        // write a timestamp
        time_t tTime;
        unsigned int tMilliseconds;
        if (ServiceLocator::Exists<IClock>())
        {
            auto tNow = ServiceLocator::Get<IClock>().Now();
            tMilliseconds = static_cast<unsigned int>(std::chrono::time_point_cast<std::chrono::milliseconds>(tNow).time_since_epoch().count() % 1000);
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

        {
            std::scoped_lock<std::mutex> oLock(m_oMutex);
            m_pWriter->Write(sBuffer);

            // mark the level
            switch (level)
            {
                case LogLevel::Info: m_pWriter->Write("INFO"); break;
                case LogLevel::Warn: m_pWriter->Write("WARN"); break;
                case LogLevel::Error: m_pWriter->Write("ERR "); break;
            }
            m_pWriter->Write("| ");

            // write the message
            m_pWriter->Write(sMessage);

            // newline
            m_pWriter->WriteLine();
        }

        // if writing to a file, flush immediately
        auto* pFileWriter = dynamic_cast<ra::services::impl::FileTextWriter*>(m_pWriter.get());
        if (pFileWriter != nullptr)
            pFileWriter->GetFStream().flush();
    }

private:
    std::unique_ptr<ra::services::TextWriter> m_pWriter;
    mutable std::mutex m_oMutex;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_FILELOGGER_HH
