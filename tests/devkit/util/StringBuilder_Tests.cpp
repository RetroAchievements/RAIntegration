#include "util\Strings.hh"

#include "testutil\CppUnitTest.hh"

namespace ra {
namespace util {
namespace tests {

TEST_CLASS(StringBuilder_Tests)
{
public:
    TEST_METHOD(TestNarrow)
    {
        // ASCII
        Assert::AreEqual(std::string("Test"), StringBuilder::Narrow(L"Test"));
        Assert::AreEqual(std::string("Test"), StringBuilder::Narrow(std::wstring(L"Test")));

        // U+1F30F - EARTH GLOBE ASIA-AUSTRALIA
        Assert::AreEqual(std::string("\xF0\x9F\x8C\x8F"), StringBuilder::Narrow(L"\xD83C\xDF0F"));
        Assert::AreEqual(std::string("\xF0\x9F\x8C\x8F"), StringBuilder::Narrow(std::wstring(L"\xD83C\xDF0F")));
    }

    TEST_METHOD(TestWiden)
    {
        // ASCII
        Assert::AreEqual(std::wstring(L"Test"), StringBuilder::Widen("Test"));
        Assert::AreEqual(std::wstring(L"Test"), StringBuilder::Widen(std::string("Test")));
        Assert::AreEqual(std::wstring(L"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-=_+"), StringBuilder::Widen(std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-=_+")));

        // U+1F30F - EARTH GLOBE ASIA-AUSTRALIA
        Assert::AreEqual(std::wstring(L"\xD83C\xDF0F"), StringBuilder::Widen("\xF0\x9F\x8C\x8F"));
        Assert::AreEqual(std::wstring(L"\xD83C\xDF0F"), StringBuilder::Widen(std::string("\xF0\x9F\x8C\x8F")));

        // invalid UTF-8 replaced with placeholder U+FFFD
        Assert::AreEqual(std::wstring(L"T\xFFFDst"), StringBuilder::Widen("T\xA9st")); // should be \xC3\xA9
    }

    TEST_METHOD(TestAppend)
    {
        auto ToAString = [](auto arg) {
            StringBuilder builder;
            builder.Append(arg);
            return builder.ToString();
        };

        Assert::AreEqual(std::string("0"), ToAString(0));
        Assert::AreEqual(std::string("1"), ToAString(1));
        Assert::AreEqual(std::string{'a'}, ToAString('a'));
        Assert::AreEqual(std::string{'a'}, ToAString(L'a'));
        Assert::AreEqual(std::string{'3'}, ToAString('3'));
        Assert::AreEqual(std::string{'3'}, ToAString(L'3'));
        Assert::AreEqual(std::string("99"), ToAString(99));
        Assert::AreEqual(std::string("-3"), ToAString(-3));
        Assert::AreEqual(std::string("0"), ToAString(0U));
        Assert::AreEqual(std::string("1"), ToAString(1U));
        Assert::AreEqual(std::string("99"), ToAString(99U));
        Assert::AreEqual(std::string("Apple"), ToAString("Apple"));
        Assert::AreEqual(std::string("Apple"), ToAString(std::string("Apple")));
        Assert::AreEqual(std::string("Apple"), ToAString(L"Apple"));
        Assert::AreEqual(std::string("Apple"), ToAString(std::wstring(L"Apple")));
    }

    TEST_METHOD(TestToWString)
    {
        auto ToWString = [](auto arg) {
            StringBuilder builder;
            builder.Append(arg);
            return builder.ToWString();
        };

        Assert::AreEqual(std::wstring(1, L'a'), ToWString(L'a'));
        Assert::AreEqual(std::wstring(1, L'a'), ToWString('a'));
        Assert::AreEqual(std::wstring(1, L'3'), ToWString(L'3'));
        Assert::AreEqual(std::wstring(1, L'3'), ToWString('3'));
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

    TEST_METHOD(TestAppendMultiple)
    {
        StringBuilder builder;
        builder.Append("Happy ", 40, std::string("th Birthday!"));
        Assert::AreEqual(std::string("Happy 40th Birthday!"), builder.ToString());
        Assert::AreEqual(std::wstring(L"Happy 40th Birthday!"), builder.ToWString());

        builder.Append(L"Happy ", 50U, std::wstring(L"th Birthday!"));
        Assert::AreEqual(std::string("Happy 40th Birthday!Happy 50th Birthday!"), builder.ToString());
        Assert::AreEqual(std::wstring(L"Happy 40th Birthday!Happy 50th Birthday!"), builder.ToWString());
    }

    TEST_METHOD(TestPrintf)
    {
        // Note: uses ra::util::String::Printf to forward multiple args and simplify test.

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
        // Note: uses ra::util::String::Printf to forward multiple args and simplify test.

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
};


} // namespace tests
} // namespace services
} // namespace ra
