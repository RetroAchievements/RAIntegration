#include "util\Strings.hh"

#include "testutil\CppUnitTest.hh"

namespace ra {
namespace util {

GSL_SUPPRESS_F6 _NODISCARD
static std::string TrimLineEnding(std::string&& str) noexcept
{
    auto sRet{ std::move_if_noexcept(str) };
    return String::TrimLineEnding(sRet);
}

GSL_SUPPRESS_F6 _NODISCARD
static std::wstring Trim(std::wstring&& str) noexcept
{
    auto sRet{ std::move_if_noexcept(str) };
    return String::Trim(sRet);
}

namespace tests {

TEST_CLASS(Strings_Tests)
{
public:
    TEST_METHOD(TestNarrow)
    {
        // ASCII
        Assert::AreEqual(std::string("Test"), String::Narrow("Test"));
        Assert::AreEqual(std::string("Test"), String::Narrow(L"Test"));
        Assert::AreEqual(std::string("Test"), String::Narrow(std::string("Test")));
        Assert::AreEqual(std::string("Test"), String::Narrow(std::wstring(L"Test")));

        // U+1F30F - EARTH GLOBE ASIA-AUSTRALIA
        Assert::AreEqual(std::string("\xF0\x9F\x8C\x8F"), String::Narrow("\xF0\x9F\x8C\x8F"));
        Assert::AreEqual(std::string("\xF0\x9F\x8C\x8F"), String::Narrow(L"\xD83C\xDF0F"));
        Assert::AreEqual(std::string("\xF0\x9F\x8C\x8F"), String::Narrow(std::string("\xF0\x9F\x8C\x8F")));
        Assert::AreEqual(std::string("\xF0\x9F\x8C\x8F"), String::Narrow(std::wstring(L"\xD83C\xDF0F")));
    }

    TEST_METHOD(TestWiden)
    {
        // ASCII
        Assert::AreEqual(std::wstring(L"Test"), String::Widen("Test"));
        Assert::AreEqual(std::wstring(L"Test"), String::Widen(L"Test"));
        Assert::AreEqual(std::wstring(L"Test"), String::Widen(std::string("Test")));
        Assert::AreEqual(std::wstring(L"Test"), String::Widen(std::wstring(L"Test")));
        Assert::AreEqual(std::wstring(L"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-=_+"), String::Widen(std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-=_+")));

        // U+1F30F - EARTH GLOBE ASIA-AUSTRALIA
        Assert::AreEqual(std::wstring(L"\xD83C\xDF0F"), String::Widen("\xF0\x9F\x8C\x8F"));
        Assert::AreEqual(std::wstring(L"\xD83C\xDF0F"), String::Widen(L"\xD83C\xDF0F"));
        Assert::AreEqual(std::wstring(L"\xD83C\xDF0F"), String::Widen(std::string("\xF0\x9F\x8C\x8F")));
        Assert::AreEqual(std::wstring(L"\xD83C\xDF0F"), String::Widen(std::wstring(L"\xD83C\xDF0F")));

        // invalid UTF-8 replaced with placeholder U+FFFD
        Assert::AreEqual(std::wstring(L"T\xFFFDst"), String::Widen("T\xA9st")); // should be \xC3\xA9
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

    TEST_METHOD(TestTrim)
    {
        Assert::AreEqual(std::wstring(L"test"), Trim(std::wstring(L"test")));
        Assert::AreEqual(std::wstring(L"test"), Trim(std::wstring(L"test \t\r\n")));
        Assert::AreEqual(std::wstring(L"test"), Trim(std::wstring(L" \t\r\ntest")));
        Assert::AreEqual(std::wstring(L"test"), Trim(std::wstring(L" \t\r\ntest \t\r\n")));
        Assert::AreEqual(std::wstring(L"test"), Trim(std::wstring(L"      test     ")));
        Assert::AreEqual(std::wstring(L"test test\r\ntest"), Trim(std::wstring(L"\ttest test\r\ntest\r\n\r\n")));
    }

    TEST_METHOD(TestStringPrintf)
    {
        Assert::AreEqual(std::string("This is a test."), ra::util::String::Printf("This is a %s.", "test"));
        Assert::AreEqual(std::string("1, 2, 3, 4"), ra::util::String::Printf("%d, %u, %zu, %li", 1, 2U, (size_t)3U, 4L));
        Assert::AreEqual(std::string("53.45%"), ra::util::String::Printf("%.2f%%", 53.45f));
        Assert::AreEqual(std::string("Nothing to replace"), ra::util::String::Printf("Nothing to replace"));
        Assert::AreEqual(std::string(), ra::util::String::Printf(""));
        Assert::AreEqual(std::string("'Twas the night before Christmas and all through the house, not a creature was stirring, not even a mouse."), 
            ra::util::String::Printf("'Twas the %s before %s and all through the %s, not a %s was %s, not even a %s.", "night", "Christmas", "house", "creature", "stirring", "mouse"));

        Assert::AreEqual(std::string("01AE"), ra::util::String::Printf("%04X", 0x1AEU));
        Assert::AreEqual(std::string("01ae"), ra::util::String::Printf("%04x", 0x1AEU));
        Assert::AreEqual(std::string("01ae"), ra::util::String::Printf("%04x", 0x1AEUL));
        Assert::AreEqual(std::string("01ae"), ra::util::String::Printf("%0*x", 4, 0x1AEUL));
        Assert::AreEqual(std::string("0080"), ra::util::String::Printf("%04d", 80));
        Assert::AreEqual(std::string("  80"), ra::util::String::Printf("%4d", 80));
        Assert::AreEqual(std::string("Appl"), ra::util::String::Printf("%.4s", "Apple"));
        Assert::AreEqual(std::string("Appl"), ra::util::String::Printf("%.*s", 4, "Apple"));
        Assert::AreEqual(std::string("Apple"), ra::util::String::Printf("%4s", "Apple"));
        Assert::AreEqual(std::string(" Boo"), ra::util::String::Printf("%4s", "Boo"));
        
        Assert::AreEqual(std::string("abc"), ra::util::String::Printf("%c%c%c", 'a', 'b', 'c'));
        Assert::AreEqual(std::string("616263"), ra::util::String::Printf("%02x%02x%02x", 'a', 'b', 'c'));
        Assert::AreEqual(std::string("52E331"), ra::util::String::Printf("%X", 5432113));
        Assert::AreEqual(std::string("52e331"), ra::util::String::Printf("%x", 5432113));
        Assert::AreEqual(std::string("70AF4E"), ra::util::String::Printf("%X", 0b11100001010111101001110));
        Assert::AreEqual(std::string("70af4e"), ra::util::String::Printf("%x", 0b11100001010111101001110));
    }

    TEST_METHOD(TestWStringPrintf)
    {
        Assert::AreEqual(std::wstring(L"This is a test."), ra::util::String::Printf(L"This is a %s.", "test"));
        Assert::AreEqual(std::wstring(L"1, 2, 3, 4"), ra::util::String::Printf(L"%d, %u, %zu, %li", 1, 2U, (size_t)3U, 4L));
        Assert::AreEqual(std::wstring(L"53.45%"), ra::util::String::Printf(L"%.2f%%", 53.45f));
        Assert::AreEqual(std::wstring(L"Nothing to replace"), ra::util::String::Printf(L"Nothing to replace"));
        Assert::AreEqual(std::wstring(), ra::util::String::Printf(L""));
        Assert::AreEqual(std::wstring(L"'Twas the night before Christmas and all through the house, not a creature was stirring, not even a mouse."),
            ra::util::String::Printf(L"'Twas the %s before %s and all through the %s, not a %s was %s, not even a %s.", "night", "Christmas", "house", "creature", "stirring", "mouse"));

        Assert::AreEqual(std::wstring(L"abc"), ra::util::String::Printf(L"%c%c%c", 'a', 'b', 'c'));
        Assert::AreEqual(std::wstring(L"616263"), ra::util::String::Printf(L"%02x%02x%02x", 'a', 'b', 'c'));
    }

    TEST_METHOD(TestStartsWith)
    {
        Assert::IsTrue(String::StartsWith(L"Test", L"Tes"));
        Assert::IsTrue(String::StartsWith(L"Test", L"Test"));
        Assert::IsFalse(String::StartsWith(L"Test", L"Test1"));
        Assert::IsFalse(String::StartsWith(L"Test", L"TES"));
        Assert::IsFalse(String::StartsWith(L"Test", L"est"));
        Assert::IsFalse(String::StartsWith(L"Test", L"1Test"));
    }

    TEST_METHOD(TestEndsWith)
    {
        Assert::IsTrue(String::EndsWith(L"Test", L"est"));
        Assert::IsTrue(String::EndsWith(L"Test", L"Test"));
        Assert::IsFalse(String::EndsWith(L"Test", L"1Test"));
        Assert::IsFalse(String::EndsWith(L"Test", L"EST"));
        Assert::IsFalse(String::EndsWith(L"Test", L"Tes"));
        Assert::IsFalse(String::EndsWith(L"Test", L"Test1"));
    }

    TEST_METHOD(TestContainsCaseInsensitive)
    {
        std::wstring input(L"ThIs Is ThE EnD");

        Assert::IsTrue(String::ContainsCaseInsensitive(input, std::wstring(L"ThIs")));
        Assert::IsTrue(String::ContainsCaseInsensitive(input, std::wstring(L"this")));
        Assert::IsTrue(String::ContainsCaseInsensitive(input, std::wstring(L"THIS")));
        Assert::IsTrue(String::ContainsCaseInsensitive(input, std::wstring(L"ThIs")));
        Assert::IsTrue(String::ContainsCaseInsensitive(input, std::wstring(L"this")));

        Assert::IsTrue(String::ContainsCaseInsensitive(input, std::wstring(L"EnD")));
        Assert::IsTrue(String::ContainsCaseInsensitive(input, std::wstring(L"end")));
        Assert::IsTrue(String::ContainsCaseInsensitive(input, std::wstring(L"END")));
        Assert::IsTrue(String::ContainsCaseInsensitive(input, std::wstring(L"EnD")));
        Assert::IsTrue(String::ContainsCaseInsensitive(input, std::wstring(L"end")));

        Assert::IsTrue(String::ContainsCaseInsensitive(input, std::wstring(L"ThE")));
        Assert::IsTrue(String::ContainsCaseInsensitive(input, std::wstring(L"the")));
        Assert::IsTrue(String::ContainsCaseInsensitive(input, std::wstring(L"THE")));
        Assert::IsTrue(String::ContainsCaseInsensitive(input, std::wstring(L"ThE")));
        Assert::IsTrue(String::ContainsCaseInsensitive(input, std::wstring(L"the")));

        Assert::IsFalse(String::ContainsCaseInsensitive(input, std::wstring(L"cat")));

        Assert::IsTrue(String::ContainsCaseInsensitive(input, std::wstring(L"E E")));
    }
};


} // namespace tests
} // namespace services
} // namespace ra
