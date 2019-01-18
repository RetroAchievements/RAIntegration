#include "RA_RichPresence.h"
#include "RA_UnitTestHelpers.h"

#include "services\impl\StringTextReader.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(RA_RichPresence_Tests)
{
public:
    class RichPresenceInterpreterHarness : public RA_RichPresenceInterpreter
    {
    public:
        bool LoadTest(const std::string& sScript)
        {
            ra::services::impl::StringTextReader oReader(sScript);
            return RA_RichPresenceInterpreter::Load(oReader);
        }
    };

    TEST_METHOD(TestValue)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Format:Points\nFormatType=VALUE\n\nDisplay:\n@Points(0x 0001) Points");

        Assert::AreEqual("13330 Points", rp.GetRichPresenceString().c_str());

        memory.at(1) = 20;
        Assert::AreEqual("13332 Points", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestValueFormula)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Format:Points\nFormatType=VALUE\n\nDisplay:\n@Points(0xH0001*100_0xH0002) Points");

        Assert::AreEqual("1852 Points", rp.GetRichPresenceString().c_str());

        memory.at(1) = 0x20;
        Assert::AreEqual("3252 Points", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestLookup)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Lookup:Location\n0=Zero\n1=One\n\nDisplay:\nAt @Location(0xH0000)");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory.at(0) = 2; // no entry
        Assert::AreEqual("At ", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestLookupFormula)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Lookup:Location\n0=Zero\n1=One\n\nDisplay:\nAt @Location(0xH0000*0.5)");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory.at(0) = 2;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestLookupRepeated)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Lookup:Location\n0=Zero\n1=One\n\nDisplay:\nAt @Location(0xH0000), Near @Location(0xH0001)");

        Assert::AreEqual("At Zero, Near ", rp.GetRichPresenceString().c_str());

        memory.at(1) = 1;
        Assert::AreEqual("At Zero, Near One", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("At One, Near One", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestLookupHex)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Lookup:Location\n0x00=Zero\n0x01=One\n\nDisplay:\nAt @Location(0xH0000)");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory.at(0) = 2; // no entry
        Assert::AreEqual("At ", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestLookupDefault)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Lookup:Location\n0=Zero\n1=One\n*=Star\n\nDisplay:\nAt @Location(0xH0000)");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory.at(0) = 2; // no entry
        Assert::AreEqual("At Star", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestLookupCRLF)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Lookup:Location\r\n0=Zero\r\n1=One\r\n\r\nDisplay:\r\nAt @Location(0xH0000)\r\n");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory.at(0) = 2; // no entry
        Assert::AreEqual("At ", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestLookupAfterDisplay)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Display:\nAt @Location(0xH0000)\n\nLookup:Location\n0=Zero\n1=One");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory.at(0) = 2; // no entry
        Assert::AreEqual("At ", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestLookupWhitespace)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Lookup:Location\n0= Zero \n1= One \n\nDisplay:\nAt '@Location(0xH0000)' ");

        Assert::AreEqual("At ' Zero ' ", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("At ' One ' ", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestRandomTextBetweenSections)
    {
        // Anything that doesn't begin with "Format:" "Lookup:" or "Display:" is ignored. People sometimes
        // use this logic to add comments to the Rich Presence script - particularly author comments
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest(
            "Locations are fun!\nLookup:Location\n0=Zero\n1=One\n\nDisplay goes here\nDisplay:\nAt "
            "@Location(0xH0000)\n\nWritten by User3");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory.at(0) = 2; // no entry
        Assert::AreEqual("At ", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestComments)
    {
        // Double slash indicates the remaining portion of the line is a comment
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest(
            "// Locations are fun!\nLookup:Location // lookup\n0=Zero // 0\n1=One // 1\n\n//Display goes "
            "here\nDisplay: // display\nAt @Location(0xH0000) // text\n\n//Written by User3");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory.at(0) = 2; // no entry
        Assert::AreEqual("At ", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestConditionalDisplay)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Display:\n?0xH0000=0?Zero\n?0xH0000=1?One\nOther");

        Assert::AreEqual("Zero", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("One", rp.GetRichPresenceString().c_str());

        memory.at(0) = 2; // no entry
        Assert::AreEqual("Other", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestConditionalDisplayOutOfOrder)
    {
        // Display section ends immediately after the non-conditional string is found. Other strings are ignored as
        // between-section garbage
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Display:\nOther\n?0xH0000=0?Zero\n?0xH0000=1?One");

        Assert::AreEqual("Other", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestConditionalDisplayNoDefault)
    {
        // A non-conditional string must be present
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Display:\n?0xH0000=0?Zero");

        Assert::AreEqual(false, rp.Enabled());
    }

    TEST_METHOD(TestConditionalDisplaySharedLookup)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest(
            "Lookup:Location\n0x00=Zero\n0x01=One\n\nDisplay:\n?0xH0001=18?At @Location(0xH0000)\nNear "
            "@Location(0xH0000)");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory.at(1) = 17;
        Assert::AreEqual("Near One", rp.GetRichPresenceString().c_str());

        memory.at(0) = 0;
        Assert::AreEqual("Near Zero", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestConditionalDisplayCommonPrefix)
    {
        // ensures order matters
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Display:\n?0xH0000=0_0xH0001=18?First\n?0xH0000=0?Second\nThird");

        Assert::AreEqual("First", rp.GetRichPresenceString().c_str());

        memory.at(1) = 1;
        Assert::AreEqual("Second", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("Third", rp.GetRichPresenceString().c_str());

        memory.at(0) = 0;
        memory.at(1) = 18;
        rp.LoadTest("Display:\n?0xH0000=0?First\n?0xH0000=0_0xH0001=18?Second\nThird");

        Assert::AreEqual("First", rp.GetRichPresenceString().c_str());

        memory.at(1) = 1;
        Assert::AreEqual("First", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("Third", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestConditionalDisplayDuplicatedCondition)
    {
        // ensures order matters
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Display:\n?0xH0000=0?First\n?0xH0000=0?Second\nThird");

        Assert::AreEqual("First", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("Third", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestConditionalDisplayInvalid)
    {
        // invalid condition is ignored
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest(
            "Lookup:Location\n0x00=Zero\n0x01=One\n\nDisplay:\n?BANANA?At @Location(0xH0000)\nNear @Location(0xH0000)");

        Assert::AreEqual("Near Zero", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("Near One", rp.GetRichPresenceString().c_str());

        memory.at(1) = 17;
        Assert::AreEqual("Near One", rp.GetRichPresenceString().c_str());

        memory.at(0) = 0;
        Assert::AreEqual("Near Zero", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestUndefinedTag)
    {
        // An "@XXXX" tag that cannot be resolved is just ignored
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Display:\n@Points(0x 0001) Points");

        Assert::AreEqual(" Points", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestEscapedComment)
    {
        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Display:\nWhat \\// Where");

        Assert::AreEqual("What // Where", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestEscapedBackslash)
    {
        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Display:\nWhat \\\\ Where");

        Assert::AreEqual("What \\ Where", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestPartiallyEscapedComment)
    {
        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Display:\nWhat \\/// Where");

        Assert::AreEqual("What /", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestTrailingBackslash)
    {
        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Display:\nWhat \\");

        Assert::AreEqual("What ", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestEscapedLookup)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Lookup:Location\n0=Zero\n1=One\n\nDisplay:\n\\@@Location(0xH0000)");

        Assert::AreEqual("@Zero", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1;
        Assert::AreEqual("@One", rp.GetRichPresenceString().c_str());

        memory.at(0) = 2; // no entry
        Assert::AreEqual("@", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestHitCounts)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        RichPresenceInterpreterHarness rp;
        rp.LoadTest("Display:\n?0xh00=0.1._R:0xh00=2?Zero\n?0xh00=1.1._R:0xh00=3?One\nDefault");

        Assert::AreEqual("Zero", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1; // because the hit count is still set on the first condition, it's displayed
        Assert::AreEqual("Zero", rp.GetRichPresenceString().c_str());

        memory.at(0) = 2; // this clears the hit count on the first condition, and doesn't match the second, so
                            // default is displayed
        Assert::AreEqual("Default", rp.GetRichPresenceString().c_str());

        memory.at(0) = 1; // the second one captures a hit count
        Assert::AreEqual("One", rp.GetRichPresenceString().c_str());

        memory.at(0) = 0; // the first captures a hit count
        Assert::AreEqual("Zero", rp.GetRichPresenceString().c_str());

        memory.at(0) =
            2; // this clears the hit count on the first, but the second still has a hit count, so it's used
        Assert::AreEqual("One", rp.GetRichPresenceString().c_str());

        memory.at(0) = 3; // this clears the hit count on the second, so the default is shown
        Assert::AreEqual("Default", rp.GetRichPresenceString().c_str());
    }
};

} // namespace tests
} // namespace data
} // namespace ra
