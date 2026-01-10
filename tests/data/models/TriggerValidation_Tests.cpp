#include "CppUnitTest.h"

#include "data\models\TriggerValidation.hh"

#include "tests\devkit\context\mocks\MockConsoleContext.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockEmulatorContext.hh"
#include "tests\mocks\MockUserContext.hh"

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
        AssertValidation("0xH1234=1_A:0xH2345=2", L"Condition 2: AddSource condition type expects another condition to follow");
        AssertValidation("0xH1234=1_B:0xH2345=2", L"Condition 2: SubSource condition type expects another condition to follow");
        AssertValidation("0xH1234=1_C:0xH2345=2", L"Condition 2: AddHits condition type expects another condition to follow");
        AssertValidation("0xH1234=1_D:0xH2345=2", L"Condition 2: SubHits condition type expects another condition to follow");
        AssertValidation("0xH1234=1_N:0xH2345=2", L"Condition 2: AndNext condition type expects another condition to follow");
        AssertValidation("0xH1234=1_O:0xH2345=2", L"Condition 2: OrNext condition type expects another condition to follow");
        AssertValidation("0xH1234=1_Z:0xH2345=2", L"Condition 2: ResetNextIf condition type expects another condition to follow");

        AssertValidation("0xH1234=1_A:0xH2345=2S0x3456=1", L"Core Condition 2: AddSource condition type expects another condition to follow");
        AssertValidation("0x3456=1S0xH1234=1_A:0xH2345=2", L"Alt1 Condition 2: AddSource condition type expects another condition to follow");
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
        AssertValidation("0xH1234>255", L"Condition 1: Comparison is never true (max 255)");
        AssertValidation("0xH1234>=255", L"");
        AssertValidation("0xH1234<255", L"");
        AssertValidation("0xH1234<=255", L"Condition 1: Comparison is always true (max 255)");

        AssertValidation("R:0xH1234<255_0xH2345=1.1.", L"");
        AssertValidation("R:0xH1234<=255_0xH2345=1.1.", L"Condition 1: Comparison is always true (max 255)");

        AssertValidation("0xH1234=256", L"Condition 1: Comparison is never true (max 255)");
        AssertValidation("0xH1234!=256", L"Condition 1: Comparison is always true (max 255)");
        AssertValidation("0xH1234>256", L"Condition 1: Comparison is never true (max 255)");
        AssertValidation("0xH1234>=256", L"Condition 1: Comparison is never true (max 255)");
        AssertValidation("0xH1234<256", L"Condition 1: Comparison is always true (max 255)");
        AssertValidation("0xH1234<=256", L"Condition 1: Comparison is always true (max 255)");

        AssertValidation("0x 1234>=65535", L"");
        AssertValidation("0x 1234>=65536", L"Condition 1: Comparison is never true (max 65535)");

        AssertValidation("0xW1234>=16777215", L"");
        AssertValidation("0xW1234>=16777216", L"Condition 1: Comparison is never true (max 16777215)");

        AssertValidation("0xX1234>=4294967295", L"");
        AssertValidation("0xX1234>4294967295", L"Condition 1: Comparison is never true (max 4294967295)");

        AssertValidation("0xT1234>=1", L"");
        AssertValidation("0xT1234>1", L"Condition 1: Comparison is never true (max 1)");

        // AddSource max is the sum of all parts (255+255=510)
        AssertValidation("A:0xH1234_0xH1235<510", L"");
        AssertValidation("A:0xH1234_0xH1235<=510", L"Condition 2: Comparison is always true (max 510)");

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
        // basic checks for each side
        ra::context::mocks::MockConsoleContext mockConsoleContext(NES, L"NES");
        Assert::AreEqual(0xFFFFU, mockConsoleContext.MaxAddress());
        AssertValidation("0xH0234>0xH0235", L"");
        AssertValidation("0xH1234>0xH1235", L"Condition 1: Mirror RAM may not be exposed by emulator (address 1234)");
        AssertValidation("0xH12345>0xH0235", L"Condition 1: Address 12345 out of range (max FFFF)");
        AssertValidation("0xH0234>0xH12345", L"Condition 1: Address 12345 out of range (max FFFF)");
        AssertValidation("0xH12345>0xH12345", L"Condition 1: Address 12345 out of range (max FFFF)");
        AssertValidation("0xX0234>h12345", L"");

        // edge cases
        AssertValidation("0xH0000>5", L"");
        AssertValidation("0xHFFFF>5", L"");
        AssertValidation("0xH10000>5", L"Condition 1: Address 10000 out of range (max FFFF)");

        // AddAddress can use really big values for negative offsets, don't flag them.
        AssertValidation("I:0xX0234_0xHFFFFFF00>5", L"");
        AssertValidation("I:0xX0234_0xH0234>5_0xHFFFFFF00>5", L"Condition 3: Address FFFFFF00 out of range (max FFFF)");
    }

    TEST_METHOD(TestAddressRangeArcade)
    {
        // arcade doesn't provide a max address as each game can have a different amount of memory
        // instead, max valid address is determined by how much memory the emulator/core exposes
        ra::context::mocks::MockConsoleContext mockConsoleContext(Arcade, L"Arcade");
        Assert::AreEqual(0U, mockConsoleContext.MaxAddress());
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        unsigned char pMemory[256];
        mockEmulatorContext.MockMemory(pMemory, sizeof(pMemory));
        Assert::AreEqual({ 256U }, mockEmulatorContext.TotalMemorySize());

        AssertValidation("0xH12>0xH34", L"");
        AssertValidation("0xH123>0xH34", L"Condition 1: Address 0123 out of range (max 00FF)");
        AssertValidation("0xH12>0xH345", L"Condition 1: Address 0345 out of range (max 00FF)");
        AssertValidation("0xH123>0xH123", L"Condition 1: Address 0123 out of range (max 00FF)");
        AssertValidation("0xX12>h123", L"");

        // edge cases
        AssertValidation("0xH00>5", L"");
        AssertValidation("0xHFF>5", L"");
        AssertValidation("0xH100>5", L"Condition 1: Address 0100 out of range (max 00FF)");
    }

    TEST_METHOD(TestLeaderboardConditionTypes)
    {
        AssertLeaderboardValidation("0xH1234<99_0x2345<50", L"");
        AssertLeaderboardValidation("P:0xH1234<99_0x2345<50", L"");
        AssertLeaderboardValidation("R:0xH1234<99_0x2345<50", L"Condition 1: No captured hits to reset");
        AssertLeaderboardValidation("M:0xH1234<99_0x2345<50", L"Measured has no effect in leaderboard triggers");
        AssertLeaderboardValidation("Q:0xH1234<99_0x2345<50", L"Condition 1: MeasuredIf without Measured");
        AssertLeaderboardValidation("Q:0xH1234<99_0x2345<50_M:0x1235=30", L"MeasuredIf has no effect in leaderboard triggers");
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

    TEST_METHOD(TestCodeNoteSizeComparisons)
    {
        ra::data::context::mocks::MockUserContext mockUserContext;
        ra::data::context::mocks::MockGameContext mockGameContext;
        mockGameContext.SetCodeNote(0x0008, L"[8-bit] Byte address");
        mockGameContext.SetCodeNote(0x0010, L"[16-bit] Word address");
        mockGameContext.SetCodeNote(0x0018, L"[24-bit] TByte address");
        mockGameContext.SetCodeNote(0x0020, L"[32-bit] DWord address");
        mockGameContext.SetCodeNote(0x0028, L"Unsized Byte address");
        mockGameContext.SetCodeNote(0x0030, L"[16-bit BE] BE Word address");
        mockGameContext.SetCodeNote(0x0040, L"[40-bytes] array of words");

        // valid reads
        AssertValidation("0xH0008>8", L"");
        AssertValidation("0x 0010>8", L"");
        AssertValidation("0xW0018>8", L"");
        AssertValidation("0xX0020>8", L"");
        AssertValidation("0xH0028>8", L"");
        AssertValidation("0xI0030>8", L"");

        // size mismatch
        AssertValidation("0xH0001>8", L"Condition 1: No code note for address 0001");
        AssertValidation("0xH0010>8", L"Condition 1: 8-bit read of address 0010 differs from code note size 16-bit");
        AssertValidation("0x 0030>8", L"Condition 1: 16-bit read of address 0030 differs from code note size 16-bit BE");
        AssertValidation("0x 0010>0x 0008", L"Condition 1: 16-bit read of address 0008 differs from code note size 8-bit");
        AssertValidation("0x 0028>8", L"Condition 1: 16-bit read of address 0028 differs from implied code note size 8-bit");

        // bit sizes only require a note exist
        AssertValidation("0xN0008=1", L"");
        AssertValidation("0xS0018=1", L"");
        AssertValidation("0xK0020=1", L"");
        AssertValidation("0xR0028=1", L"");
        AssertValidation("0xN0004=1", L"Condition 1: No code note for address 0004");
        AssertValidation("0xK0004=1", L"Condition 1: No code note for address 0004");

        // sub-note addresses
        AssertValidation("0xH0009>8", L"Condition 1: No code note for address 0009");
        AssertValidation("0xH0011>8", L"Condition 1: 8-bit read of address 0011 differs from code note size 16-bit at 0010");
        AssertValidation("0xH0040>8", L""); // start of array
        AssertValidation("0xH0052>8", L""); // in array

        // chained value note
        AssertValidation("A:0xH0008_1=6", L""); // 1 is not an address so it shouldn't matter that it doesn't have a note
    }
};

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
