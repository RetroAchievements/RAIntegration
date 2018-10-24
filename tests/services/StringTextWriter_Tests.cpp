#include "services\impl\StringTextWriter.hh"
#include "tests\RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace services {
namespace impl {
namespace tests {

TEST_CLASS(StringTextWriter_Tests)
{
public:
    TEST_METHOD(TestEmpty)
    {
        StringTextWriter writer;
        Assert::AreEqual(std::string(""), writer.GetString());
    }

    TEST_METHOD(TestWriteLine)
    {
        StringTextWriter writer;
        writer.Write(std::string("Test"));
        writer.Write(std::string("1\n"));
        Assert::AreEqual(std::string("Test1\n"), writer.GetString());
    }
};

} // namespace tests
} // namespace impl
} // namespace services
} // namespace ra
