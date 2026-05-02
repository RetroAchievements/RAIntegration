#include "util\Tokenizer.hh"

#include "testutil\CppUnitTest.hh"

namespace ra {
namespace util {
namespace tests {

TEST_CLASS(Tokenizer_Tests)
{
public:
    TEST_METHOD(TestReadTo)
    {
        std::string input("This is a test.");
        Tokenizer tokenizer(input);
        Assert::AreEqual('T', tokenizer.PeekChar());
        Assert::AreEqual({ 0U }, tokenizer.CurrentPosition());

        Assert::AreEqual(std::string("This"), std::string(tokenizer.ReadTo(' ')));
        Assert::AreEqual(' ', tokenizer.PeekChar());
        Assert::AreEqual({ 4U }, tokenizer.CurrentPosition());

        tokenizer.Advance();

        Assert::AreEqual(std::string("is"), std::string(tokenizer.ReadTo(' ')));
        Assert::AreEqual({ 7U }, tokenizer.CurrentPosition());

        tokenizer.Advance(3); // skip over " a "
        Assert::AreEqual({ 10U }, tokenizer.CurrentPosition());
        Assert::IsFalse(tokenizer.EndOfString());

        Assert::AreEqual(std::string("test."), std::string(tokenizer.ReadTo(' ')));
        Assert::AreEqual({ 15U }, tokenizer.CurrentPosition());
        Assert::IsTrue(tokenizer.EndOfString());
        Assert::AreEqual('\0', tokenizer.PeekChar());
        Assert::AreEqual(std::string(""), std::string(tokenizer.ReadTo(' ')));
        Assert::AreEqual({ 15U }, tokenizer.CurrentPosition());

        tokenizer.Seek(2U);
        Assert::AreEqual({ 2U }, tokenizer.CurrentPosition());
        Assert::AreEqual(std::string("is is a t"), std::string(tokenizer.ReadTo('e')));
        Assert::AreEqual('e', tokenizer.PeekChar());
        Assert::AreEqual({ 11U }, tokenizer.CurrentPosition());
    }

    TEST_METHOD(TestAdvancePastEndOfString)
    {
        std::string input("This is a test.");
        Tokenizer tokenizer(input);
        tokenizer.Seek(12U);
        Assert::AreEqual({ 12U }, tokenizer.CurrentPosition());
        Assert::IsFalse(tokenizer.EndOfString());

        tokenizer.Advance(6U);
        Assert::AreEqual({ 15U }, tokenizer.CurrentPosition());
        Assert::IsTrue(tokenizer.EndOfString());

        tokenizer.Advance();
        Assert::AreEqual({ 15U }, tokenizer.CurrentPosition());
        Assert::IsTrue(tokenizer.EndOfString());

        tokenizer.AdvanceTo(' ');
        Assert::AreEqual({ 15U }, tokenizer.CurrentPosition());
        Assert::IsTrue(tokenizer.EndOfString());
    }

    TEST_METHOD(TestReadNumber)
    {
        std::string input("11:2.3:0:123456789");
        Tokenizer tokenizer(input);
        Assert::AreEqual(11U, tokenizer.PeekNumber());
        Assert::AreEqual({ 0U }, tokenizer.CurrentPosition());
        Assert::AreEqual('1', tokenizer.PeekChar());

        Assert::AreEqual(11U, tokenizer.ReadNumber());
        Assert::AreEqual({ 2U }, tokenizer.CurrentPosition());
        Assert::AreEqual(':', tokenizer.PeekChar());

        tokenizer.Advance();
        Assert::AreEqual(2U, tokenizer.ReadNumber());
        Assert::AreEqual({ 4U }, tokenizer.CurrentPosition());
        Assert::AreEqual('.', tokenizer.PeekChar());

        tokenizer.Advance();
        Assert::AreEqual(3U, tokenizer.ReadNumber());
        Assert::AreEqual({ 6U }, tokenizer.CurrentPosition());
        Assert::AreEqual(':', tokenizer.PeekChar());

        tokenizer.Advance();
        Assert::AreEqual(0U, tokenizer.ReadNumber());
        Assert::AreEqual({ 8U }, tokenizer.CurrentPosition());
        Assert::AreEqual(':', tokenizer.PeekChar());

        tokenizer.Advance();
        Assert::AreEqual(123456789U, tokenizer.ReadNumber());
        Assert::AreEqual({ 18U }, tokenizer.CurrentPosition());
        Assert::IsTrue(tokenizer.EndOfString());
    }

    TEST_METHOD(TestGetPointer)
    {
        std::string input("This is a test.");
        const Tokenizer tokenizer(input);
        const auto* pStart = input.c_str();
        Assert::AreEqual({ 0 }, tokenizer.GetPointer(0U) - pStart);
        Assert::AreEqual({ 4 }, tokenizer.GetPointer(4U) - pStart);
        Assert::AreEqual({ 8 }, tokenizer.GetPointer(8U) - pStart);
        Assert::AreEqual({ 12 }, tokenizer.GetPointer(12U) - pStart);
        Assert::AreEqual({ 15 }, tokenizer.GetPointer(16U) - pStart); // string is 15 characters
    }

    TEST_METHOD(TestReadQuotedString)
    {
        std::string input("\"This string is \\\"quoted\\\".\" - Author");
        Tokenizer tokenizer(input);
        auto sString = tokenizer.ReadQuotedString();
        Assert::AreEqual(std::string("This string is \"quoted\"."), sString);
        Assert::AreEqual(' ', tokenizer.PeekChar());
        Assert::AreEqual({ 28U }, tokenizer.CurrentPosition());

        // PeekChar is not a quote, nothing should be returned
        sString = tokenizer.ReadQuotedString();
        Assert::AreEqual(std::string(""), sString);

        // no closing quote, should return remaining portion of string
        tokenizer.Seek(tokenizer.CurrentPosition() - 1);
        sString = tokenizer.ReadQuotedString();
        Assert::AreEqual(std::string(" - Author"), sString);
        Assert::IsTrue(tokenizer.EndOfString());
    }

    TEST_METHOD(TestReadQuotedStringEscapeSequences)
    {
        // \n -> newline, \(anything else) -> (anything else)
        std::string input("\"\\\\\\8\\n\\\"\\!\"");
        Tokenizer tokenizer(input);
        auto sString = tokenizer.ReadQuotedString();
        Assert::AreEqual(std::string("\\8\n\"!"), sString);
    }

    TEST_METHOD(TestConsume)
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
