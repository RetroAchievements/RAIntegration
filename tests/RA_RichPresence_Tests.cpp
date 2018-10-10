#include "CppUnitTest.h"

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
        bool Load(const std::string& sScript)
        {
            ra::services::impl::StringTextReader oReader(sScript);
            return RA_RichPresenceInterpreter::Load(oReader);
        }
    };

    TEST_METHOD(TestValue)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Format:Points\nFormatType=VALUE\n\nDisplay:\n@Points(0x 0001) Points");

        Assert::AreEqual("13330 Points", rp.GetRichPresenceString().c_str());

        memory[1] = 20;
        Assert::AreEqual("13332 Points", rp.GetRichPresenceString().c_str());
    }
    
    TEST_METHOD(TestValueFormula)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Format:Points\nFormatType=VALUE\n\nDisplay:\n@Points(0xH0001*100_0xH0002) Points");

        Assert::AreEqual("1852 Points", rp.GetRichPresenceString().c_str());

        memory[1] = 0x20;
        Assert::AreEqual("3252 Points", rp.GetRichPresenceString().c_str());
    }
    
    TEST_METHOD(TestLookup)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Lookup:Location\n0=Zero\n1=One\n\nDisplay:\nAt @Location(0xH0000)");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory[0] = 2; // no entry
        Assert::AreEqual("At ", rp.GetRichPresenceString().c_str());
    }
                
    TEST_METHOD(TestLookupFormula)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Lookup:Location\n0=Zero\n1=One\n\nDisplay:\nAt @Location(0xH0000*0.5)");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory[0] = 2;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());
    }
                
    TEST_METHOD(TestLookupRepeated)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Lookup:Location\n0=Zero\n1=One\n\nDisplay:\nAt @Location(0xH0000), Near @Location(0xH0001)");

        Assert::AreEqual("At Zero, Near ", rp.GetRichPresenceString().c_str());

        memory[1] = 1;
        Assert::AreEqual("At Zero, Near One", rp.GetRichPresenceString().c_str());

        memory[0] = 1; 
        Assert::AreEqual("At One, Near One", rp.GetRichPresenceString().c_str());
    }
    
    TEST_METHOD(TestLookupHex)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Lookup:Location\n0x00=Zero\n0x01=One\n\nDisplay:\nAt @Location(0xH0000)");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory[0] = 2; // no entry
        Assert::AreEqual("At ", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestLookupDefault)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Lookup:Location\n0=Zero\n1=One\n*=Star\n\nDisplay:\nAt @Location(0xH0000)");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory[0] = 2; // no entry
        Assert::AreEqual("At Star", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestLookupCRLF)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Lookup:Location\r\n0=Zero\r\n1=One\r\n\r\nDisplay:\r\nAt @Location(0xH0000)\r\n");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory[0] = 2; // no entry
        Assert::AreEqual("At ", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestLookupAfterDisplay)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Display:\nAt @Location(0xH0000)\n\nLookup:Location\n0=Zero\n1=One");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory[0] = 2; // no entry
        Assert::AreEqual("At ", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestLookupWhitespace)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Lookup:Location\n0= Zero \n1= One \n\nDisplay:\nAt '@Location(0xH0000)' ");

        Assert::AreEqual("At ' Zero ' ", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("At ' One ' ", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestRandomTextBetweenSections)
    {
        // Anything that doesn't begin with "Format:" "Lookup:" or "Display:" is ignored. People sometimes
        // use this logic to add comments to the Rich Presence script - particularly author comments
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Locations are fun!\nLookup:Location\n0=Zero\n1=One\n\nDisplay goes here\nDisplay:\nAt @Location(0xH0000)\n\nWritten by User3");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory[0] = 2; // no entry
        Assert::AreEqual("At ", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestComments)
    {
        // Double slash indicates the remaining portion of the line is a comment
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("// Locations are fun!\nLookup:Location // lookup\n0=Zero // 0\n1=One // 1\n\n//Display goes here\nDisplay: // display\nAt @Location(0xH0000) // text\n\n//Written by User3");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory[0] = 2; // no entry
        Assert::AreEqual("At ", rp.GetRichPresenceString().c_str());
    }
    
    TEST_METHOD(TestConditionalDisplay)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Display:\n?0xH0000=0?Zero\n?0xH0000=1?One\nOther");

        Assert::AreEqual("Zero", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("One", rp.GetRichPresenceString().c_str());

        memory[0] = 2; // no entry
        Assert::AreEqual("Other", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestConditionalDisplayOutOfOrder)
    {
        // Display section ends immediately after the non-conditional string is found. Other strings are ignored as between-section garbage
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Display:\nOther\n?0xH0000=0?Zero\n?0xH0000=1?One");

        Assert::AreEqual("Other", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestConditionalDisplayNoDefault)
    {
        // A non-conditional string must be present
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Display:\n?0xH0000=0?Zero");

        Assert::AreEqual(false, rp.Enabled());
    }
        
    TEST_METHOD(TestConditionalDisplaySharedLookup)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Lookup:Location\n0x00=Zero\n0x01=One\n\nDisplay:\n?0xH0001=18?At @Location(0xH0000)\nNear @Location(0xH0000)");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory[1] = 17;
        Assert::AreEqual("Near One", rp.GetRichPresenceString().c_str());

        memory[0] = 0;
        Assert::AreEqual("Near Zero", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestConditionalDisplayCommonPrefix)
    {
        // ensures order matters
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Display:\n?0xH0000=0_0xH0001=18?First\n?0xH0000=0?Second\nThird");

        Assert::AreEqual("First", rp.GetRichPresenceString().c_str());

        memory[1] = 1;
        Assert::AreEqual("Second", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("Third", rp.GetRichPresenceString().c_str());

        memory[0] = 0;
        memory[1] = 18;
        rp.Load("Display:\n?0xH0000=0?First\n?0xH0000=0_0xH0001=18?Second\nThird");

        Assert::AreEqual("First", rp.GetRichPresenceString().c_str());

        memory[1] = 1;
        Assert::AreEqual("First", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("Third", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestConditionalDisplayDuplicatedCondition)
    {
        // ensures order matters
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Display:\n?0xH0000=0?First\n?0xH0000=0?Second\nThird");

        Assert::AreEqual("First", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("Third", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestConditionalDisplayInvalid)
    {
        // invalid condition is ignored
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Lookup:Location\n0x00=Zero\n0x01=One\n\nDisplay:\n?BANANA?At @Location(0xH0000)\nNear @Location(0xH0000)");

        Assert::AreEqual("Near Zero", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("Near One", rp.GetRichPresenceString().c_str());

        memory[1] = 17;
        Assert::AreEqual("Near One", rp.GetRichPresenceString().c_str());

        memory[0] = 0;
        Assert::AreEqual("Near Zero", rp.GetRichPresenceString().c_str());
    }
    
    TEST_METHOD(TestUndefinedTag)
    {
        // An "@XXXX" tag that cannot be resolved is just ignored
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Display:\n@Points(0x 0001) Points");

        Assert::AreEqual(" Points", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestEscapedComment)
    {
        RichPresenceInterpreterHarness rp;
        rp.Load("Display:\nWhat \\// Where");

        Assert::AreEqual("What // Where", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestEscapedBackslash)
    {
        RichPresenceInterpreterHarness rp;
        rp.Load("Display:\nWhat \\\\ Where");

        Assert::AreEqual("What \\ Where", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestPartiallyEscapedComment)
    {
        RichPresenceInterpreterHarness rp;
        rp.Load("Display:\nWhat \\/// Where");

        Assert::AreEqual("What /", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestTrailingBackslash)
    {
        RichPresenceInterpreterHarness rp;
        rp.Load("Display:\nWhat \\");

        Assert::AreEqual("What ", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestEscapedLookup)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RichPresenceInterpreterHarness rp;
        rp.Load("Lookup:Location\n0=Zero\n1=One\n\nDisplay:\n\\@@Location(0xH0000)");

        Assert::AreEqual("@Zero", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("@One", rp.GetRichPresenceString().c_str());

        memory[0] = 2; // no entry
        Assert::AreEqual("@", rp.GetRichPresenceString().c_str());
    }
};

} // namespace tests
} // namespace data
} // namespace ra
