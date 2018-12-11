#include "RA_Defs.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {

[[gsl::suppress(f.6), nodiscard]]
static std::string TrimLineEnding(std::string&& str) noexcept
{
    auto sRet{ std::move_if_noexcept(str) };
    return TrimLineEnding(sRet);
}

namespace services {
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

    TEST_METHOD(TestToString)
    {
        Assert::AreEqual(std::string("0"), ToString(0));
        Assert::AreEqual(std::string("1"), ToString(1));
        Assert::AreEqual(std::string("99"), ToString(99));
        Assert::AreEqual(std::string("-3"), ToString(-3));
        Assert::AreEqual(std::string("0"), ToString(0U));
        Assert::AreEqual(std::string("1"), ToString(1U));
        Assert::AreEqual(std::string("99"), ToString(99U));
        Assert::AreEqual(std::string("Apple"), ToString("Apple"));
        Assert::AreEqual(std::string("Apple"), ToString(std::string("Apple")));
        Assert::AreEqual(std::string("Apple"), ToString(L"Apple"));
        Assert::AreEqual(std::string("Apple"), ToString(std::wstring(L"Apple")));
    }

    TEST_METHOD(TestToWString)
    {
        Assert::AreEqual(std::wstring(L"0"), ToWString(0));
        Assert::AreEqual(std::wstring(L"1"), ToWString(1));
        Assert::AreEqual(std::wstring(L"99"), ToWString(99));
        Assert::AreEqual(std::wstring(L"-3"), ToWString(-3));
        Assert::AreEqual(std::wstring(L"0"), ToWString(0U));
        Assert::AreEqual(std::wstring(L"1"), ToWString(1U));
        Assert::AreEqual(std::wstring(L"99"), ToWString(99U));
        Assert::AreEqual(std::wstring(L"Apple"), ToWString("Apple"));
        Assert::AreEqual(std::wstring(L"Apple"), ToWString(std::string("Apple")));
        Assert::AreEqual(std::wstring(L"Apple"), ToWString(L"Apple"));
        Assert::AreEqual(std::wstring(L"Apple"), ToWString(std::wstring(L"Apple")));
    }

    TEST_METHOD(TestAppendString)
    {
        std::string sTest;
        AppendString(sTest, "Happy ", 40, std::string("th Birthday!"));
        Assert::AreEqual(std::string("Happy 40th Birthday!"), sTest);

        AppendString(sTest, L"Happy ", 50U, std::wstring(L"th Birthday!"));
        Assert::AreEqual(std::string("Happy 40th Birthday!Happy 50th Birthday!"), sTest);
    }

    TEST_METHOD(TestBuildString)
    {
        std::string sTest = BuildString("Happy ", 40, std::string("th Birthday!"));
        Assert::AreEqual(std::string("Happy 40th Birthday!"), sTest);
    }

    TEST_METHOD(TestAppendWString)
    {
        std::wstring sTest;
        AppendWString(sTest, "Happy ", 40, std::string("th Birthday!"));
        Assert::AreEqual(std::wstring(L"Happy 40th Birthday!"), sTest);

        AppendWString(sTest, L"Happy ", 50U, std::wstring(L"th Birthday!"));
        Assert::AreEqual(std::wstring(L"Happy 40th Birthday!Happy 50th Birthday!"), sTest);
    }

    TEST_METHOD(TestBuildWString)
    {
        std::wstring sTest = BuildWString(L"Happy ", 40, std::wstring(L"th Birthday!"));
        Assert::AreEqual(std::wstring(L"Happy 40th Birthday!"), sTest);
    }

    TEST_METHOD(TestStringPrintf)
    {
        Assert::AreEqual(std::string("This is a test."), StringPrintf("This is a %s.", "test"));
        Assert::AreEqual(std::string("1, 2, 3, 4"), StringPrintf("%d, %u, %zu, %li", 1, 2U, 3U, 4L));
        Assert::AreEqual(std::string("53.45%"), StringPrintf("%.2f%%", 53.45f));
        Assert::AreEqual(std::string("Nothing to replace"), StringPrintf("Nothing to replace"));
        Assert::AreEqual(std::string(), StringPrintf(""));
        Assert::AreEqual(std::string("'Twas the night before Christmas and all through the house, not a creature was stirring, not even a mouse."), 
            StringPrintf("'Twas the %s before %s and all through the %s, not a %s was %s, not even a %s.", "night", "Christmas", "house", "creature", "stirring", "mouse"));

        Assert::AreEqual(std::string("01AE"), StringPrintf("%04X", 0x1AEU));
        Assert::AreEqual(std::string("01ae"), StringPrintf("%04x", 0x1AEU));
        Assert::AreEqual(std::string("01ae"), StringPrintf("%04x", 0x1AEUL));
        Assert::AreEqual(std::string("01ae"), StringPrintf("%0*x", 4, 0x1AEUL));
        Assert::AreEqual(std::string("0080"), StringPrintf("%04d", 80));
        Assert::AreEqual(std::string("  80"), StringPrintf("%4d", 80));
        Assert::AreEqual(std::string("Appl"), StringPrintf("%.4s", "Apple"));
        Assert::AreEqual(std::string("Appl"), StringPrintf("%.*s", 4, "Apple"));
        Assert::AreEqual(std::string("Apple"), StringPrintf("%4s", "Apple"));
        Assert::AreEqual(std::string(" Boo"), StringPrintf("%4s", "Boo"));
    }

    TEST_METHOD(TestWStringPrintf)
    {
        Assert::AreEqual(std::wstring(L"This is a test."), StringPrintf(L"This is a %s.", "test"));
        Assert::AreEqual(std::wstring(L"1, 2, 3, 4"), StringPrintf(L"%d, %u, %zu, %li", 1, 2U, 3U, 4L));
        Assert::AreEqual(std::wstring(L"53.45%"), StringPrintf(L"%.2f%%", 53.45f));
        Assert::AreEqual(std::wstring(L"Nothing to replace"), StringPrintf(L"Nothing to replace"));
        Assert::AreEqual(std::wstring(), StringPrintf(L""));
        Assert::AreEqual(std::wstring(L"'Twas the night before Christmas and all through the house, not a creature was stirring, not even a mouse."),
            StringPrintf(L"'Twas the %s before %s and all through the %s, not a %s was %s, not even a %s.", "night", "Christmas", "house", "creature", "stirring", "mouse"));
    }

    TEST_METHOD(TestStringStartsWith)
    {
        Assert::IsTrue(StringStartsWith(L"Test", L"Tes"));
        Assert::IsTrue(StringStartsWith(L"Test", L"Test"));
        Assert::IsFalse(StringStartsWith(L"Test", L"Test1"));
        Assert::IsFalse(StringStartsWith(L"Test", L"TES"));
        Assert::IsFalse(StringStartsWith(L"Test", L"est"));
        Assert::IsFalse(StringStartsWith(L"Test", L"1Test"));
    }

    TEST_METHOD(TestStringEndsWith)
    {
        Assert::IsTrue(StringEndsWith(L"Test", L"est"));
        Assert::IsTrue(StringEndsWith(L"Test", L"Test"));
        Assert::IsFalse(StringEndsWith(L"Test", L"1Test"));
        Assert::IsFalse(StringEndsWith(L"Test", L"EST"));
        Assert::IsFalse(StringEndsWith(L"Test", L"Tes"));
        Assert::IsFalse(StringEndsWith(L"Test", L"Test1"));
    }
};

} // namespace tests
} // namespace services
} // namespace ra
