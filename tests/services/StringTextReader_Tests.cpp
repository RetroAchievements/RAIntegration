#include "services\impl\StringTextReader.hh"
#include "tests\RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace services {
namespace impl {
namespace tests {

TEST_CLASS(StringTextReader_Tests)
{
public:
    TEST_METHOD(TestEmpty)
    {
        StringTextReader reader(std::string(""));
        std::string sLine;
        Assert::IsFalse(reader.GetLine(sLine));
    }

    TEST_METHOD(TestOneLine)
    {
        StringTextReader reader(std::string("test"));
        std::string sLine;
        Assert::IsTrue(reader.GetLine(sLine));
        Assert::AreEqual(std::string("test"), sLine);
        Assert::IsFalse(reader.GetLine(sLine));
    }

    TEST_METHOD(TestOneLineWithLineEnding)
    {
        StringTextReader reader(std::string("test\r\n"));
        std::string sLine;
        Assert::IsTrue(reader.GetLine(sLine));
        Assert::AreEqual(std::string("test"), sLine);
        Assert::IsFalse(reader.GetLine(sLine));
    }

    TEST_METHOD(TestTwoLines)
    {
        StringTextReader reader(std::string("test\r\ntwo"));
        std::string sLine;
        Assert::IsTrue(reader.GetLine(sLine));
        Assert::AreEqual(std::string("test"), sLine);
        Assert::IsTrue(reader.GetLine(sLine));
        Assert::AreEqual(std::string("two"), sLine);
        Assert::IsFalse(reader.GetLine(sLine));
    }

    TEST_METHOD(TestThreeLinesEmptyMiddle)
    {
        StringTextReader reader(std::string("test\r\n\r\ntwo"));
        std::string sLine;
        Assert::IsTrue(reader.GetLine(sLine));
        Assert::AreEqual(std::string("test"), sLine);
        Assert::IsTrue(reader.GetLine(sLine));
        Assert::AreEqual(std::string(""), sLine);
        Assert::IsTrue(reader.GetLine(sLine));
        Assert::AreEqual(std::string("two"), sLine);
        Assert::IsFalse(reader.GetLine(sLine));
    }
};

} // namespace tests
} // namespace impl
} // namespace services
} // namespace ra
