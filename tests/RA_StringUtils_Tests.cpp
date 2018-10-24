#include "RA_Defs.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(RA_StringUtils_Tests)
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

        // invalid UTF-8 replaced with placeholder U+FFFD
        Assert::AreEqual(std::wstring(L"T\xFFFDst"), Widen("T\xA9st")); // should be \xC3\xA9
    }

    TEST_METHOD(TestTrimLineEnding)
    {
        Assert::AreEqual(std::string("test"), TrimLineEnding(std::string("test")));
        Assert::AreEqual(std::string("test"), TrimLineEnding(std::string("test\r")));
        Assert::AreEqual(std::string("test"), TrimLineEnding(std::string("test\n")));
        Assert::AreEqual(std::string("test"), TrimLineEnding(std::string("test\r\n")));
        Assert::AreEqual(std::string("test\n"), TrimLineEnding(std::string("test\n\n")));
        Assert::AreEqual(std::string("test\r\n"), TrimLineEnding(std::string("test\r\n\r\n")));
    }

    TEST_METHOD(TestFormat)
    {
        Assert::AreEqual(std::string("This is a test."), StringPrintf("This is a %s.", "test"));
        Assert::AreEqual(std::string("1, 2, 3, 4"), StringPrintf("%d, %u, %zu, %li", 1, 2U, 3U, 4L));
        Assert::AreEqual(std::string("53.45%"), StringPrintf("%.2f%%", 53.45f));
        Assert::AreEqual(std::string("Nothing to replace"), StringPrintf("Nothing to replace"));
        Assert::AreEqual(std::string(), StringPrintf(""));
        Assert::AreEqual(std::string("'Twas the night before Christmas and all through the house, not a creature was stirring, not even a mouse."), 
            StringPrintf("'Twas the %s before %s and all through the %s, not a %s was %s, not even a %s.", "night", "Christmas", "house", "creature", "stirring", "mouse"));
    }
};

} // namespace tests
} // namespace data
} // namespace ra
