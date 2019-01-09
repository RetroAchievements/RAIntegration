#include "services\impl\FileLogger.hh"

#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockFileSystem.hh"
#include "tests\RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::services::mocks::MockClock;
using ra::services::mocks::MockFileSystem;

namespace ra {
namespace services {
namespace impl {
namespace tests {

TEST_CLASS(FileLogger_Tests)
{
private:
    const std::wstring mockLogFileName{L".\\RACache\\RALog.txt"};
    const std::wstring mockOldLogFileName{L".\\RACache\\RALog-old.txt"};

public:
    TEST_METHOD(TestInitialization)
    {
        MockFileSystem mockFileSystem;
        FileLogger logger(mockFileSystem);
        Assert::AreEqual(std::string("\n"), mockFileSystem.GetFileContents(mockLogFileName));
    }

    TEST_METHOD(TestLogMessage)
    {
        MockClock mockClock;
        MockFileSystem mockFileSystem;
        FileLogger logger(mockFileSystem);

        logger.LogMessage(LogLevel::Info, "This is a message.");
        logger.LogMessage(LogLevel::Warn, "This is another message.");
        using namespace std::chrono_literals;
        mockClock.AdvanceTime(375ms);
        logger.LogMessage(LogLevel::Error, "This is the third message.");

        Assert::AreEqual(std::string("\n"
            "220843.000|INFO| This is a message.\n"
            "220843.000|WARN| This is another message.\n"
            "220843.375|ERR | This is the third message.\n"), mockFileSystem.GetFileContents(mockLogFileName));
    }

    TEST_METHOD(TestRotate)
    {
        MockClock mockClock;
        MockFileSystem mockFileSystem;
        mockFileSystem.MockFile(mockLogFileName, "Test");
        mockFileSystem.MockFileSize(mockLogFileName, 1100000LL); // more than 1024*1024 bytes
        Assert::AreEqual(mockFileSystem.GetFileSize(mockLogFileName), 1100000LL);
        Assert::AreEqual(mockFileSystem.GetFileSize(mockOldLogFileName), -1LL);

        FileLogger logger(mockFileSystem);
        logger.LogMessage(LogLevel::Info, "This is a message.");
        Assert::AreEqual(mockFileSystem.GetFileSize(mockLogFileName), 37LL);
        Assert::AreEqual(mockFileSystem.GetFileSize(mockOldLogFileName), 1100000LL);
    }

    TEST_METHOD(TestRotateReplace)
    {
        MockClock mockClock;
        MockFileSystem mockFileSystem;
        mockFileSystem.MockFile(mockLogFileName, "Test");
        mockFileSystem.MockFileSize(mockLogFileName, 1100000); // more than 1024*1024 bytes
        mockFileSystem.MockFile(mockOldLogFileName, std::string(80, 'A'));   // previously rotated log
        Assert::AreEqual(mockFileSystem.GetFileSize(mockLogFileName), 1100000LL);
        Assert::AreEqual(mockFileSystem.GetFileSize(mockOldLogFileName), 80LL);

        FileLogger logger(mockFileSystem);
        logger.LogMessage(LogLevel::Info, "This is a message.");

        Assert::AreEqual(mockFileSystem.GetFileSize(mockLogFileName), 37LL);
        Assert::AreEqual(mockFileSystem.GetFileSize(mockOldLogFileName), 1100000LL);
    }
};

} // namespace tests
} // namespace impl
} // namespace services
} // namespace ra
