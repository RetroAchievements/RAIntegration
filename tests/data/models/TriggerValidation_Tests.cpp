#include "CppUnitTest.h"

#include "data\models\TriggerValidation.hh"

#include "tests\mocks\MockEmulatorContext.hh"

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
        const bool bResult = TriggerValidation::Validate(sTrigger, sError, AssetType::Achievement);
        Assert::AreEqual(sExpectedError, sError);
        Assert::AreEqual(bResult, sExpectedError.empty());
    }

    void AssertLeaderboardValidation(const std::string& sTrigger, const std::wstring& sExpectedError)
    {
        std::wstring sError;
        const bool bResult = TriggerValidation::Validate(sTrigger, sError, AssetType::Leaderboard);
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
        AssertValidation("0xH1234=1_A:0xH2345=2", L"Final condition type expects another condition to follow");
        AssertValidation("0xH1234=1_B:0xH2345=2", L"Final condition type expects another condition to follow");
        AssertValidation("0xH1234=1_C:0xH2345=2", L"Final condition type expects another condition to follow");
        AssertValidation("0xH1234=1_D:0xH2345=2", L"Final condition type expects another condition to follow");
        AssertValidation("0xH1234=1_N:0xH2345=2", L"Final condition type expects another condition to follow");
        AssertValidation("0xH1234=1_O:0xH2345=2", L"Final condition type expects another condition to follow");
        AssertValidation("0xH1234=1_Z:0xH2345=2", L"Final condition type expects another condition to follow");

        AssertValidation("0xH1234=1_A:0xH2345=2S0x3456=1", L"Core Final condition type expects another condition to follow");
        AssertValidation("0x3456=1S0xH1234=1_A:0xH2345=2", L"Alt1 Final condition type expects another condition to follow");
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
        AssertValidation("C:0xH1234=1_0xH2345=2", L"Condition 2: Final condition in AddHits chain must have a hit target");
        AssertValidation("D:0xH1234=1_0xH2345=2", L"Condition 2: Final condition in AddHits chain must have a hit target");
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

    TEST_METHOD(TestAddressRange)
    {
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;

        // no range registered, don't invalidate
        AssertValidation("0xH1234>0xH1235", L"");

        mockEmulatorContext.AddMemoryBlock(0, 0x10000, nullptr, nullptr);

        // basic checks for each side
        AssertValidation("0xH1234>0xH1235", L"");
        AssertValidation("0xH12345>0xH1235", L"Condition 1: Address 12345 out of range (max FFFF)");
        AssertValidation("0xH1234>0xH12345", L"Condition 1: Address 12345 out of range (max FFFF)");
        AssertValidation("0xH12345>0xH12345", L"Condition 1: Address 12345 out of range (max FFFF)");
        AssertValidation("0xX1234>h12345", L"");

        // support for multiple memory blocks and edge addresses
        mockEmulatorContext.AddMemoryBlock(1, 0x10000, nullptr, nullptr);
        AssertValidation("0xH1234>0xH1235", L"");
        AssertValidation("0xH12345>0xH1235", L"");
        AssertValidation("0xH0000>5", L"");
        AssertValidation("0xH1FFFF>5", L"");
        AssertValidation("0xH20000>5", L"Condition 1: Address 20000 out of range (max 1FFFF)");

        // AddAddress can use really big values for negative offsets, don't flag them.
        AssertValidation("I:0xX1234_0xHFFFFFF00>5", L"");
        AssertValidation("I:0xX1234_0xH1234>5_0xHFFFFFF00>5", L"Condition 3: Address FFFFFF00 out of range (max 1FFFF)");
    }

    TEST_METHOD(TestLeaderboardConditionTypes)
    {
        AssertLeaderboardValidation("0xH1234<99_0x2345<50", L"");
        AssertLeaderboardValidation("P:0xH1234<99_0x2345<50", L"");
        AssertLeaderboardValidation("R:0xH1234<99_0x2345<50", L"");
        AssertLeaderboardValidation("M:0xH1234<99_0x2345<50", L"Measured has no effect in leaderboard triggers");
        AssertLeaderboardValidation("Q:0xH1234<99_0x2345<50", L"MeasuredIf has no effect in leaderboard triggers");
        AssertLeaderboardValidation("T:0xH1234<99_0x2345<50", L"Trigger has no effect in leaderboard triggers");
        AssertLeaderboardValidation("A:0xH1234<99_0x2345<50", L"");
        AssertLeaderboardValidation("B:0xH1234<99_0x2345<50", L"");
        AssertLeaderboardValidation("I:0xH1234<99_0x2345<50", L"");
        AssertLeaderboardValidation("C:0xH1234<99_0x2345<50.6.", L"");
        AssertLeaderboardValidation("D:0xH1234<99_0x2345<50.6.", L"");
        AssertLeaderboardValidation("Z:0xH1234<99_0x2345<50", L"");
        AssertLeaderboardValidation("N:0xH1234<99_0x2345<50", L"");
        AssertLeaderboardValidation("O:0xH1234<99_0x2345<50", L"");
    }
};


} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
