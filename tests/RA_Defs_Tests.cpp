#include "CppUnitTest.h"

#include "RA_Defs.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(RA_Defs_Tests)
{
public:
    TEST_METHOD(TestNarrow)
    {
        // ASCII
        Assert::AreEqual(std::string("Test"), Narrow("Test"));
        Assert::AreEqual(std::string("Test"), Narrow(L"Test"));
        Assert::AreEqual(std::string("Test"), Narrow(std::string("Test")));
        Assert::AreEqual(std::string("Test"), Narrow(std::wstring(L"Test")));

        // U+1F30F - EARTH GLOBE ASIA-AUSTRALIA
        Assert::AreEqual(std::string("\xF0\x9F\x8C\x8F"), Narrow("\xF0\x9F\x8C\x8F"));
        Assert::AreEqual(std::string("\xF0\x9F\x8C\x8F"), Narrow(L"\xD83C\xDF0F"));
        Assert::AreEqual(std::string("\xF0\x9F\x8C\x8F"), Narrow(std::string("\xF0\x9F\x8C\x8F")));
        Assert::AreEqual(std::string("\xF0\x9F\x8C\x8F"), Narrow(std::wstring(L"\xD83C\xDF0F")));
    }

    TEST_METHOD(TestWiden)
    {
        // ASCII
        Assert::AreEqual(std::wstring(L"Test"), Widen("Test"));
        Assert::AreEqual(std::wstring(L"Test"), Widen(L"Test"));
        Assert::AreEqual(std::wstring(L"Test"), Widen(std::string("Test")));
        Assert::AreEqual(std::wstring(L"Test"), Widen(std::wstring(L"Test")));

        // U+1F30F - EARTH GLOBE ASIA-AUSTRALIA
        Assert::AreEqual(std::wstring(L"\xD83C\xDF0F"), Widen("\xF0\x9F\x8C\x8F"));
        Assert::AreEqual(std::wstring(L"\xD83C\xDF0F"), Widen(L"\xD83C\xDF0F"));
        Assert::AreEqual(std::wstring(L"\xD83C\xDF0F"), Widen(std::string("\xF0\x9F\x8C\x8F")));
        Assert::AreEqual(std::wstring(L"\xD83C\xDF0F"), Widen(std::wstring(L"\xD83C\xDF0F")));
    }

    TEST_METHOD(TestDataStreamAsString)
    {
        DataStream s1{ 'A', 'p', 'p', 'l', 'e', '\0' };
        Assert::AreEqual("Apple", DataStreamAsString(s1));
    }
};

} // namespace tests
} // namespace data
} // namespace ra
