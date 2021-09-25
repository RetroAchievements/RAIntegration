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
};


} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
