#include "CppUnitTest.h"

#include "RA_RichPresence.h"
#include "RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace tests {

TEST_CLASS(RA_RichPresence_Tests)
{
public:
    TEST_METHOD(TestValue)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RA_RichPresenceInterpretter rp;
        rp.ParseFromString("Format:Points\nFormatType=VALUE\n\nDisplay:\n@Points(0x 0001) Points");

        Assert::AreEqual("13330 Points", rp.GetRichPresenceString().c_str());

        memory[1] = 20;
        Assert::AreEqual("13332 Points", rp.GetRichPresenceString().c_str());
    }
    
    TEST_METHOD(TestValueFormula)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RA_RichPresenceInterpretter rp;
        rp.ParseFromString("Format:Points\nFormatType=VALUE\n\nDisplay:\n@Points(0xH0001*100_0xH0002) Points");

        Assert::AreEqual("1852 Points", rp.GetRichPresenceString().c_str());

        memory[1] = 0x20;
        Assert::AreEqual("3252 Points", rp.GetRichPresenceString().c_str());
    }
    
    TEST_METHOD(TestLookup)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RA_RichPresenceInterpretter rp;
        rp.ParseFromString("Lookup:Location\n0=Zero\n1=One\n\nDisplay:\nAt @Location(0xH0000)");

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

        RA_RichPresenceInterpretter rp;
        rp.ParseFromString("Lookup:Location\n0=Zero\n1=One\n\nDisplay:\nAt @Location(0xH0000*0.5)");

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

        RA_RichPresenceInterpretter rp;
        rp.ParseFromString("Lookup:Location\n0=Zero\n1=One\n\nDisplay:\nAt @Location(0xH0000), Near @Location(0xH0001)");

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

        RA_RichPresenceInterpretter rp;
        rp.ParseFromString("Lookup:Location\n0x00=Zero\n0x01=One\n\nDisplay:\nAt @Location(0xH0000)");

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

        RA_RichPresenceInterpretter rp;
        rp.ParseFromString("Lookup:Location\n0=Zero\n1=One\n*=Star\n\nDisplay:\nAt @Location(0xH0000)");

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

        RA_RichPresenceInterpretter rp;
        rp.ParseFromString("Lookup:Location\r\n0=Zero\r\n1=One\r\n\r\nDisplay:\r\nAt @Location(0xH0000)\r\n");

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

        RA_RichPresenceInterpretter rp;
        rp.ParseFromString("Display:\nAt @Location(0xH0000)\n\nLookup:Location\n0=Zero\n1=One");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory[0] = 2; // no entry
        Assert::AreEqual("At ", rp.GetRichPresenceString().c_str());
    }

    TEST_METHOD(TestRandomText)
    {
        // anything that doesn't begin with "Format:" "Lookup:" or "Display:" is ignored. people sometimes
        // use this logic to add comments to the Rich Presence script - particularly author comments
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RA_RichPresenceInterpretter rp;
        rp.ParseFromString("Locations are fun!\nLookup:Location\n0=Zero\n1=One\n\nDisplay goes here\nDisplay:\nAt @Location(0xH0000)\n\nWritten by User3");

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

        RA_RichPresenceInterpretter rp;
        rp.ParseFromString("Display:\n?0xH0000=0?Zero\n?0xH0000=1?One\nOther");

        Assert::AreEqual("Zero", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("One", rp.GetRichPresenceString().c_str());

        memory[0] = 2; // no entry
        Assert::AreEqual("Other", rp.GetRichPresenceString().c_str());
    }
        
    TEST_METHOD(TestConditionalDisplaySharedLookup)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RA_RichPresenceInterpretter rp;
        rp.ParseFromString("Lookup:Location\n0x00=Zero\n0x01=One\n\nDisplay:\n?0xH0001=18?At @Location(0xH0000)\nNear @Location(0xH0000)");

        Assert::AreEqual("At Zero", rp.GetRichPresenceString().c_str());

        memory[0] = 1;
        Assert::AreEqual("At One", rp.GetRichPresenceString().c_str());

        memory[1] = 17;
        Assert::AreEqual("Near One", rp.GetRichPresenceString().c_str());

        memory[0] = 0;
        Assert::AreEqual("Near Zero", rp.GetRichPresenceString().c_str());
    }
    
    TEST_METHOD(TestUndefinedTag)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        RA_RichPresenceInterpretter rp;
        rp.ParseFromString("Display:\n@Points(0x 0001) Points");

        Assert::AreEqual(" Points", rp.GetRichPresenceString().c_str());
    }
};

} // namespace tests
} // namespace data
} // namespace ra
