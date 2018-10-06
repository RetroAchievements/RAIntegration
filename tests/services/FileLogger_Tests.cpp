#include "CppUnitTest.h"

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
    const std::wstring mockLogFileName = L".\\RACache\\RALog.txt";

public:
    TEST_METHOD(TestInitialization)
    {
        MockFileSystem mockFileSystem;
        FileLogger logger(mockFileSystem);
        Assert::AreEqual(std::string("\r\n"), mockFileSystem.GetFileContents(mockLogFileName));
    }

    TEST_METHOD(TestLogMessage)
    {
        MockClock mockClock;
        MockFileSystem mockFileSystem;
        FileLogger logger(mockFileSystem);

        logger.LogMessage(LogLevel::Info, "This is a message.");
        logger.LogMessage(LogLevel::Warn, "This is another message.");
        mockClock.AdvanceTime(std::chrono::milliseconds(375));
        logger.LogMessage(LogLevel::Error, "This is the third message.");

        Assert::AreEqual(std::string("\r\n"
            "220843.000|INFO| This is a message.\r\n"
            "220843.000|WARN| This is another message.\r\n"
            "220843.375|ERR | This is the third message.\r\n"), mockFileSystem.GetFileContents(mockLogFileName));
    }
};

} // namespace tests
} // namespace impl
} // namespace services
} // namespace ra
