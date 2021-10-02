#include "CppUnitTest.h"

#include "data\models\TriggerValidation.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

TEST_CLASS(TriggerValidation_Tests)
{
private:
    void AssertValidation(const std::string& sTrigger, const std::wstring& sExpectedError)
    {
        std::wstring sError;
        const bool bResult = TriggerValidation::Validate(sTrigger, sError);
        Assert::AreEqual(sExpectedError, sError);
        Assert::AreEqual(bResult, sExpectedError.empty());
    }

public:
    TEST_METHOD(TestEmpty)
    {
        AssertValidation("", L"");
    }

    TEST_METHOD(TestValid)
    {
        AssertValidation("0xH1234=1_0xH2345=2S0xH3456=1S0xH3456=2", L"");
    }

    TEST_METHOD(TestLastConditionCombining)
    {
        AssertValidation("0xH1234=1_A:0xH2345=2", L"Final condition type expects another condition to follow.");
        AssertValidation("0xH1234=1_B:0xH2345=2", L"Final condition type expects another condition to follow.");
        AssertValidation("0xH1234=1_C:0xH2345=2", L"Final condition type expects another condition to follow.");
        AssertValidation("0xH1234=1_D:0xH2345=2", L"Final condition type expects another condition to follow.");
        AssertValidation("0xH1234=1_N:0xH2345=2", L"Final condition type expects another condition to follow.");
        AssertValidation("0xH1234=1_O:0xH2345=2", L"Final condition type expects another condition to follow.");
        AssertValidation("0xH1234=1_Z:0xH2345=2", L"Final condition type expects another condition to follow.");

        AssertValidation("0xH1234=1_A:0xH2345=2S0x3456=1", L"Core: Final condition type expects another condition to follow.");
        AssertValidation("0x3456=1S0xH1234=1_A:0xH2345=2", L"Alt 1: Final condition type expects another condition to follow.");
    }

    TEST_METHOD(TestMiddleConditionCombining)
    {
        AssertValidation("A:0xH1234=1_0xH2345=2", L"");
        AssertValidation("B:0xH1234=1_0xH2345=2", L"");
        AssertValidation("N:0xH1234=1_0xH2345=2", L"");
        AssertValidation("O:0xH1234=1_0xH2345=2", L"");
        AssertValidation("Z:0xH1234=1_0xH2345=2", L"");
    }

    TEST_METHOD(TestAddHitsChainWithoutTarget)
    {
        AssertValidation("C:0xH1234=1_0xH2345=2", L"Condition 2: Final condition in AddHits chain must have a hit target.");
        AssertValidation("D:0xH1234=1_0xH2345=2", L"Condition 2: Final condition in AddHits chain must have a hit target.");
        AssertValidation("C:0xH1234=1_0xH2345=2.1.", L"");
        AssertValidation("D:0xH1234=1_0xH2345=2.1.", L"");
    }

    TEST_METHOD(TestRangeComparisons)
    {
        AssertValidation("0xH1234>1", L"");

        AssertValidation("0xH1234=255", L"");
        AssertValidation("0xH1234!=255", L"");
        AssertValidation("0xH1234>255", L"Condition 1: Comparison is never true");
        AssertValidation("0xH1234>=255", L"");
        AssertValidation("0xH1234<255", L"");
        AssertValidation("0xH1234<=255", L"Condition 1: Comparison is always true");

        AssertValidation("R:0xH1234<255", L"");
        AssertValidation("R:0xH1234<=255", L"Condition 1: Comparison is always true");

        AssertValidation("0xH1234=256", L"Condition 1: Comparison is never true");
        AssertValidation("0xH1234!=256", L"Condition 1: Comparison is always true");
        AssertValidation("0xH1234>256", L"Condition 1: Comparison is never true");
        AssertValidation("0xH1234>=256", L"Condition 1: Comparison is never true");
        AssertValidation("0xH1234<256", L"Condition 1: Comparison is always true");
        AssertValidation("0xH1234<=256", L"Condition 1: Comparison is always true");

        AssertValidation("0x 1234>=65535", L"");
        AssertValidation("0x 1234>=65536", L"Condition 1: Comparison is never true");

        AssertValidation("0xW1234>=16777215", L"");
        AssertValidation("0xW1234>=16777216", L"Condition 1: Comparison is never true");

        AssertValidation("0xX1234>=4294967295", L"");
        AssertValidation("0xX1234>4294967295", L"Condition 1: Comparison is never true");

        AssertValidation("0xT1234>=1", L"");
        AssertValidation("0xT1234>1", L"Condition 1: Comparison is never true");

        // AddSource max is the sum of all parts (255+255=510)
        AssertValidation("A:0xH1234_0xH1235<510", L"");
        AssertValidation("A:0xH1234_0xH1235<=510", L"Condition 2: Comparison is always true");

        // SubSource max is always 0xFFFFFFFF
        AssertValidation("B:0xH1234_0xH1235<510", L"");
        AssertValidation("B:0xH1234_0xH1235<=510", L"");
    }

    TEST_METHOD(TestSizeComparisons)
    {
        AssertValidation("0xH1234>0xH1235", L"");
        AssertValidation("0xH1234>0x 1235", L"Condition 1: Comparing different memory sizes");

        // AddSource chain may compare different sizes without warning as the chain changes the
        // size of the final result.
        AssertValidation("A:0xH1234_0xH1235=0xH2345", L"");
        AssertValidation("A:0xH1234_0xH1235=0x 2345", L"");
    }
};


} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
