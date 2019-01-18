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
        Assert::AreEqual(std::string{'a'}, ToString('a'));
        Assert::AreEqual(std::string{'a'}, ToString(L'a'));
        Assert::AreEqual(std::string{'3'}, ToString('3'));
        Assert::AreEqual(std::string{'3'}, ToString(L'3'));
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
        
        Assert::AreEqual(std::string("abc"), StringPrintf("%c%c%c", 'a', 'b', 'c'));
        Assert::AreEqual(std::string("616263"), StringPrintf("%02x%02x%02x", 'a', 'b', 'c'));
        Assert::AreEqual(std::string("52E331"), StringPrintf("%X", 5432113));
        Assert::AreEqual(std::string("52e331"), StringPrintf("%x", 5432113));
        Assert::AreEqual(std::string("70AF4E"), StringPrintf("%X", 0b11100001010111101001110));
        Assert::AreEqual(std::string("70af4e"), StringPrintf("%x", 0b11100001010111101001110));
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

        Assert::AreEqual(std::wstring(L"abc"), StringPrintf(L"%c%c%c", 'a', 'b', 'c'));
        Assert::AreEqual(std::wstring(L"616263"), StringPrintf(L"%02x%02x%02x", 'a', 'b', 'c'));
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

    TEST_METHOD(TestTokenizerReadTo)
    {
        std::string input("This is a test.");
        Tokenizer tokenizer(input);
        Assert::AreEqual('T', tokenizer.PeekChar());
        Assert::AreEqual(0U, tokenizer.CurrentPosition());

        Assert::AreEqual(std::string("This"), tokenizer.ReadTo(' '));
        Assert::AreEqual(' ', tokenizer.PeekChar());
        Assert::AreEqual(4U, tokenizer.CurrentPosition());

        tokenizer.Advance();

        Assert::AreEqual(std::string("is"), tokenizer.ReadTo(' '));
        Assert::AreEqual(7U, tokenizer.CurrentPosition());

        tokenizer.Advance(3); // skip over " a "
        Assert::AreEqual(10U, tokenizer.CurrentPosition());
        Assert::IsFalse(tokenizer.EndOfString());

        Assert::AreEqual(std::string("test."), tokenizer.ReadTo(' '));
        Assert::AreEqual(15U, tokenizer.CurrentPosition());
        Assert::IsTrue(tokenizer.EndOfString());
        Assert::AreEqual('\0', tokenizer.PeekChar());
        Assert::AreEqual(std::string(""), tokenizer.ReadTo(' '));
        Assert::AreEqual(15U, tokenizer.CurrentPosition());

        tokenizer.Seek(2U);
        Assert::AreEqual(2U, tokenizer.CurrentPosition());
        Assert::AreEqual(std::string("is is a t"), tokenizer.ReadTo('e'));
        Assert::AreEqual('e', tokenizer.PeekChar());
        Assert::AreEqual(11U, tokenizer.CurrentPosition());
    }

    TEST_METHOD(TestTokenizerAdvancePastEndOfString)
    {
        std::string input("This is a test.");
        Tokenizer tokenizer(input);
        tokenizer.Seek(12U);
        Assert::AreEqual(12U, tokenizer.CurrentPosition());
        Assert::IsFalse(tokenizer.EndOfString());

        tokenizer.Advance(6U);
        Assert::AreEqual(15U, tokenizer.CurrentPosition());
        Assert::IsTrue(tokenizer.EndOfString());

        tokenizer.Advance();
        Assert::AreEqual(15U, tokenizer.CurrentPosition());
        Assert::IsTrue(tokenizer.EndOfString());

        tokenizer.AdvanceTo(' ');
        Assert::AreEqual(15U, tokenizer.CurrentPosition());
        Assert::IsTrue(tokenizer.EndOfString());
    }

    TEST_METHOD(TestTokenizerReadNumber)
    {
        std::string input("11:2.3:0:123456789");
        Tokenizer tokenizer(input);
        Assert::AreEqual(11U, tokenizer.PeekNumber());
        Assert::AreEqual(0U, tokenizer.CurrentPosition());
        Assert::AreEqual('1', tokenizer.PeekChar());

        Assert::AreEqual(11U, tokenizer.ReadNumber());
        Assert::AreEqual(2U, tokenizer.CurrentPosition());
        Assert::AreEqual(':', tokenizer.PeekChar());

        tokenizer.Advance();
        Assert::AreEqual(2U, tokenizer.ReadNumber());
        Assert::AreEqual(4U, tokenizer.CurrentPosition());
        Assert::AreEqual('.', tokenizer.PeekChar());

        tokenizer.Advance();
        Assert::AreEqual(3U, tokenizer.ReadNumber());
        Assert::AreEqual(6U, tokenizer.CurrentPosition());
        Assert::AreEqual(':', tokenizer.PeekChar());

        tokenizer.Advance();
        Assert::AreEqual(0U, tokenizer.ReadNumber());
        Assert::AreEqual(8U, tokenizer.CurrentPosition());
        Assert::AreEqual(':', tokenizer.PeekChar());

        tokenizer.Advance();
        Assert::AreEqual(123456789U, tokenizer.ReadNumber());
        Assert::AreEqual(18U, tokenizer.CurrentPosition());
        Assert::IsTrue(tokenizer.EndOfString());
    }

    TEST_METHOD(TestTokenizerGetPointer)
    {
        std::string input("This is a test.");
        const Tokenizer tokenizer(input);
        const auto* pStart = input.c_str();
        Assert::AreEqual(0, tokenizer.GetPointer(0U) - pStart);
        Assert::AreEqual(4, tokenizer.GetPointer(4U) - pStart);
        Assert::AreEqual(8, tokenizer.GetPointer(8U) - pStart);
        Assert::AreEqual(12, tokenizer.GetPointer(12U) - pStart);
        Assert::AreEqual(15, tokenizer.GetPointer(16U) - pStart); // string is 15 characters
    }

    TEST_METHOD(TestTokenizerReadQuotedString)
    {
        std::string input("\"This string is \\\"quoted\\\".\" - Author");
        Tokenizer tokenizer(input);
        auto sString = tokenizer.ReadQuotedString();
        Assert::AreEqual(std::string("This string is \"quoted\"."), sString);
        Assert::AreEqual(' ', tokenizer.PeekChar());
        Assert::AreEqual(28U, tokenizer.CurrentPosition());

        // PeekChar is not a quote, nothing should be returned
        sString = tokenizer.ReadQuotedString();
        Assert::AreEqual(std::string(""), sString);

        // no closing quote, should return remaining portion of string
        tokenizer.Seek(tokenizer.CurrentPosition() - 1);
        sString = tokenizer.ReadQuotedString();
        Assert::AreEqual(std::string(" - Author"), sString);
        Assert::IsTrue(tokenizer.EndOfString());
    }

    TEST_METHOD(TestTokenizerReadQuotedStringEscapeSequences)
    {
        // \n -> newline, \(anything else) -> (anything else)
        std::string input("\"\\\\\\8\\n\\\"\\!\"");
        Tokenizer tokenizer(input);
        auto sString = tokenizer.ReadQuotedString();
        Assert::AreEqual(std::string("\\8\n\"!"), sString);
    }

    TEST_METHOD(TestTokenizerConsume)
    {
        std::string input("abc");
        Tokenizer tokenizer(input);

        Assert::IsFalse(tokenizer.Consume('b')); // first character is 'a'
        Assert::IsTrue(tokenizer.Consume('a'));
        Assert::IsTrue(tokenizer.Consume('b'));
        Assert::AreEqual('c', tokenizer.PeekChar());
        Assert::IsFalse(tokenizer.Consume('b'));
        Assert::IsTrue(tokenizer.Consume('c'));
        Assert::IsTrue(tokenizer.EndOfString());
        Assert::IsFalse(tokenizer.Consume('b'));
        Assert::IsFalse(tokenizer.Consume('c'));
        Assert::IsFalse(tokenizer.Consume('d'));
        Assert::IsFalse(tokenizer.Consume('\0'));
    }
};

} // namespace tests
} // namespace services
} // namespace ra
