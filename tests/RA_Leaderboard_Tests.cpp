#include "RA_Leaderboard.h"
#include "RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace tests {

class LeaderboardHarness : public RA_Leaderboard
{
public:
    LeaderboardHarness() noexcept : RA_Leaderboard(1) {}

    bool IsActive() const noexcept { return m_bActive; }
    bool IsScoreSubmitted() const noexcept { return m_bScoreSubmitted; }
    unsigned int SubmittedScore() const noexcept { return m_nSubmittedScore; }

public:
    void Reset() noexcept override
    {
        RA_Leaderboard::Reset();

        m_bActive = false;
        m_bScoreSubmitted = false;
        m_nSubmittedScore = 0;
    }

    void Start() noexcept override
    {
        m_bActive = true;
    }

    void Cancel() noexcept override
    {
        m_bActive = false;
    }

    void Submit(unsigned int nScore) noexcept override
    {
        m_bActive = false;
        m_bScoreSubmitted = true;
        m_nSubmittedScore = nScore;
    }

private:
    unsigned int m_nSubmittedScore = 0;
    bool m_bScoreSubmitted = false;
    bool m_bActive = false;
};

TEST_CLASS(RA_Leaderboard_Tests)
{
    void AssertValue(const char* sSerialized, unsigned int nExpected)
    {
        // NOTE: requires value "1" in $00
        auto sString = StringPrintf("STA:0xH00=1::CAN:0xH00=2::SUB:0xH00=3::VAL:%s", sSerialized);

        LeaderboardHarness lb;
        lb.ParseFromString(sString.c_str(), "VALUE");
        lb.Test();

        Assert::AreEqual(nExpected, lb.GetCurrentValue(), ra::Widen(sSerialized).c_str());
    }

public:
    TEST_METHOD(TestSimpleLeaderboard)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        LeaderboardHarness lb;
        lb.ParseFromString("STA:0xH00=1::CAN:0xH00=2::SUB:0xH00=3::VAL:0xH02", "VALUE");
        Assert::IsFalse(lb.IsActive());
        Assert::IsFalse(lb.IsScoreSubmitted());

        lb.Test();
        Assert::IsFalse(lb.IsActive());
        Assert::IsFalse(lb.IsScoreSubmitted());

        memory.at(0) = 3; // submit value, but not active
        lb.Test();
        Assert::IsFalse(lb.IsActive());
        Assert::IsFalse(lb.IsScoreSubmitted());

        memory.at(0) = 2; // cancel value, but not active
        lb.Test();
        Assert::IsFalse(lb.IsActive());
        Assert::IsFalse(lb.IsScoreSubmitted());

        memory.at(0) = 1; // start value
        lb.Test();
        Assert::IsTrue(lb.IsActive());
        Assert::IsFalse(lb.IsScoreSubmitted());

        memory.at(0) = 2; // cancel value
        lb.Test();
        Assert::IsFalse(lb.IsActive());
        Assert::IsFalse(lb.IsScoreSubmitted());

        memory.at(0) = 3; // submit value, but not active
        lb.Test();
        Assert::IsFalse(lb.IsActive());
        Assert::IsFalse(lb.IsScoreSubmitted());

        memory.at(0) = 1; // start value
        lb.Test();
        Assert::IsTrue(lb.IsActive());
        Assert::IsFalse(lb.IsScoreSubmitted());

        memory.at(0) = 3; // submit value
        lb.Test();
        Assert::IsFalse(lb.IsActive());
        Assert::IsTrue(lb.IsScoreSubmitted());
        Assert::AreEqual(0x34U, lb.SubmittedScore());
    }

    TEST_METHOD(TestSimpleLeaderboardFormatted)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        LeaderboardHarness lb;
        lb.ParseFromString("STA:0xH00=1::CAN:0xH00=2::SUB:0xH00=3::VAL:0xH02", "SCORE");
        lb.Test();

        memory.at(0) = 1; // start value
        lb.Test();
        Assert::IsTrue(lb.IsActive());

        Assert::AreEqual("000018 Points", lb.FormatScore(18).c_str());
    }

    TEST_METHOD(TestStartAndCancelSameFrame)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        LeaderboardHarness lb;
        lb.ParseFromString("STA:0xH00=0::CAN:0xH01=18::SUB:0xH00=3::VAL:0xH02", "VALUE");

        lb.Test();
        Assert::IsFalse(lb.IsActive());
        Assert::IsFalse(lb.IsScoreSubmitted());

        memory.at(1) = 0x13; // disables cancel
        lb.Test();
        Assert::IsTrue(lb.IsActive());
        Assert::IsFalse(lb.IsScoreSubmitted());

        memory.at(1) = 0x12; // enables cancel
        lb.Test();
        Assert::IsFalse(lb.IsActive());
        Assert::IsFalse(lb.IsScoreSubmitted());

        memory.at(1) = 0x13; // disables cancel, but start condition still true, so it shouldn't restart
        lb.Test();
        Assert::IsFalse(lb.IsActive());
        Assert::IsFalse(lb.IsScoreSubmitted());

        memory.at(0) = 0x01; // disables start; no effect this frame, but next frame can restart
        lb.Test();
        Assert::IsFalse(lb.IsActive());
        Assert::IsFalse(lb.IsScoreSubmitted());

        memory.at(0) = 0x00; // enables start
        lb.Test();
        Assert::IsTrue(lb.IsActive());
        Assert::IsFalse(lb.IsScoreSubmitted());
    }

    TEST_METHOD(TestStartAndSubmitSameFrame)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        LeaderboardHarness lb;
        lb.ParseFromString("STA:0xH00=0::CAN:0xH01=10::SUB:0xH01=18::VAL:0xH02", "VALUE");

        lb.Test();
        Assert::IsFalse(lb.IsActive());
        Assert::IsTrue(lb.IsScoreSubmitted());
        Assert::AreEqual(0x34U, lb.SubmittedScore());

        memory.at(1) = 0; // disable submit, value should not be resubmitted, 
        lb.Test();     // start is still true, but leaderboard should not reactivate
        Assert::IsFalse(lb.IsActive());

        memory.at(0) = 1; // disable start
        lb.Test();
        Assert::IsFalse(lb.IsActive());

        memory.at(0) = 0; // reenable start, leaderboard should reactivate
        lb.Test();
        Assert::IsTrue(lb.IsActive());
    }

    TEST_METHOD(TestProgress)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        // if PRO: mapping is available, use that for GetCurrentValueProgress
        LeaderboardHarness lb;
        lb.ParseFromString("STA:0xH00=0::CAN:0xH00=2::SUB:0xH00=3::PRO:0xH04::VAL:0xH02", "VALUE");
        lb.Test();
        Assert::IsTrue(lb.IsActive());
        Assert::AreEqual(0x56U, lb.GetCurrentValue());

        // if PRO: mapping is not available, use VAL: for GetCurrentValueProgress
        LeaderboardHarness lb2;
        lb2.ParseFromString("STA:0xH00=0::CAN:0xH00=2::SUB:0xH00=3::VAL:0xH02", "VALUE");
        lb2.Test();
        Assert::IsTrue(lb2.IsActive());
        Assert::AreEqual(0x34U, lb2.GetCurrentValue());
    }

    TEST_METHOD(TestStartAndCondition)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        LeaderboardHarness lb;
        lb.ParseFromString("STA:0xH00=0_0xH01=0::CAN:0xH01=10::SUB:0xH01=18::VAL:0xH02", "VALUE");

        lb.Test();
        Assert::IsFalse(lb.IsActive());

        memory.at(1) = 0; // second part of start condition is true
        lb.Test();
        Assert::IsTrue(lb.IsActive());
    }

    TEST_METHOD(TestStartOrCondition)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        LeaderboardHarness lb;
        lb.ParseFromString("STA:S0xH00=1S0xH01=1::CAN:0xH01=10::SUB:0xH01=18::VAL:0xH02", "VALUE");

        lb.Test();
        Assert::IsFalse(lb.IsActive());

        memory.at(1) = 1; // second part of start condition is true
        lb.Test();
        Assert::IsTrue(lb.IsActive());

        memory.at(1) = 0;
        lb.Reset();
        lb.Test();
        Assert::IsFalse(lb.IsActive());

        memory.at(0) = 1; // first part of start condition is true
        lb.Test();
        Assert::IsTrue(lb.IsActive());
    }

    // NOTE: This test is invalid as long as the backwards compatibility code that treats underscores as ORs is present
    //TEST_METHOD(TestCancelAndCondition)
    //{
    //    std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
    //    InitializeMemory(memory);

    //    LeaderboardHarness lb;
    //    lb.ParseFromString("STA:0xH00=0::CAN:0xH01=18_0xH02=18::SUB:0xH00=3::VAL:0xH02", MemValueFormat::Value);

    //    lb.Test();
    //    Assert::IsTrue(lb.IsActive());

    //    memory.at(2) = 0x12; // second part of cancel condition is true
    //    lb.Test();
    //    Assert::IsFalse(lb.IsActive());
    //    Assert::IsFalse(lb.IsScoreSubmitted());
    //}

    TEST_METHOD(TestCancelOrCondition)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        LeaderboardHarness lb;
        lb.ParseFromString("STA:0xH00=0::CAN:S0xH01=12S0xH02=12::SUB:0xH00=3::VAL:0xH02", "VALUE");

        lb.Test();
        Assert::IsTrue(lb.IsActive());

        memory.at(2) = 12; // second part of cancel condition is true
        lb.Test();
        Assert::IsFalse(lb.IsActive());

        memory.at(2) = 0; // second part of cancel condition is false
        lb.Reset();
        lb.Test();
        Assert::IsTrue(lb.IsActive());

        memory.at(1) = 12; // first part of cancel condition is true
        lb.Test();
        Assert::IsFalse(lb.IsActive());
    }

    TEST_METHOD(TestSubmitAndCondition)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        LeaderboardHarness lb;
        lb.ParseFromString("STA:0xH00=0::CAN:0xH01=10::SUB:0xH01=18_0xH03=18::VAL:0xH02", "VALUE");

        lb.Test();
        Assert::IsTrue(lb.IsActive());

        memory.at(3) = 18; 
        lb.Test();
        Assert::IsFalse(lb.IsActive());
        Assert::IsTrue(lb.IsScoreSubmitted());
    }

    TEST_METHOD(TestSubmitOrCondition)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        LeaderboardHarness lb;
        lb.ParseFromString("STA:0xH00=0::CAN:0xH01=10::SUB:S0xH01=12S0xH03=12::VAL:0xH02", "VALUE");

        lb.Test();
        Assert::IsTrue(lb.IsActive());

        memory.at(3) = 12; // second part of submit condition is true
        lb.Test();
        Assert::IsFalse(lb.IsActive());
        Assert::IsTrue(lb.IsScoreSubmitted());

        memory.at(3) = 0;
        lb.Reset();
        lb.Test();
        Assert::IsTrue(lb.IsActive());

        memory.at(1) = 12; // first part of submit condition is true
        lb.Test();
        Assert::IsFalse(lb.IsActive());
        Assert::IsTrue(lb.IsScoreSubmitted());
    }

    TEST_METHOD(TestUnparsableStringWillNotStart)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        LeaderboardHarness lb;
        lb.ParseFromString("STA:0xH00=0::CAN:0x0H00=1::SUB:0xH01=18::GARBAGE", "VALUE");

        lb.Test();
        Assert::IsFalse(lb.IsActive());
    }

    TEST_METHOD(TestSubmitRankInfo)
    {
        LeaderboardHarness lb;
        lb.SubmitRankInfo(1U, "George", 100, 1234567890U);
        lb.SubmitRankInfo(2U, "Jane", 80, 1234567891U);
        lb.SubmitRankInfo(3U, "Paul", 70, 1234567892U);

        Assert::AreEqual(3U, lb.GetRankInfoCount());
        Assert::AreEqual("George", lb.GetRankInfo(0).m_sUsername.c_str());
        Assert::AreEqual("Jane", lb.GetRankInfo(1).m_sUsername.c_str());
        Assert::AreEqual("Paul", lb.GetRankInfo(2).m_sUsername.c_str());

        Assert::AreEqual(1234567890, static_cast<int>(lb.GetRankInfo(0).m_TimeAchieved));
        Assert::AreEqual(2U, lb.GetRankInfo(1).m_nRank);
        Assert::AreEqual(70, lb.GetRankInfo(2).m_nScore);

        lb.ClearRankInfo();
        Assert::AreEqual(0U, lb.GetRankInfoCount());
    }

    TEST_METHOD(TestSortRankInfo)
    {
        LeaderboardHarness lb;
        lb.SubmitRankInfo(4U, "Betty", 60, 1234567893U);
        lb.SubmitRankInfo(2U, "Jane", 80, 1234567891U);
        lb.SubmitRankInfo(5U, "Roger", 50, 1234567894U);
        lb.SubmitRankInfo(1U, "George", 100, 1234567890U);
        lb.SubmitRankInfo(3U, "Paul", 70, 1234567892U);

        Assert::AreEqual(5U, lb.GetRankInfoCount());
        lb.SortRankInfo();

        Assert::AreEqual("George", lb.GetRankInfo(0).m_sUsername.c_str());
        Assert::AreEqual("Jane", lb.GetRankInfo(1).m_sUsername.c_str());
        Assert::AreEqual("Paul", lb.GetRankInfo(2).m_sUsername.c_str());
        Assert::AreEqual("Betty", lb.GetRankInfo(3).m_sUsername.c_str());
        Assert::AreEqual("Roger", lb.GetRankInfo(4).m_sUsername.c_str());

        Assert::AreEqual(1234567890, static_cast<int>(lb.GetRankInfo(0).m_TimeAchieved));
        Assert::AreEqual(2U, lb.GetRankInfo(1).m_nRank);
        Assert::AreEqual(70, lb.GetRankInfo(2).m_nScore);
        Assert::AreEqual(4U, lb.GetRankInfo(3).m_nRank);
        Assert::AreEqual(50, lb.GetRankInfo(4).m_nScore);
    }

    TEST_METHOD(TestValue)
    {
        std::array<unsigned char, 5> memory{0x01, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        // simple accessors
        AssertValue("0xH0002", 0x34U);
        AssertValue("0x000002", 0xAB34U);
        AssertValue("0xL02", 4U);
        AssertValue("0xU2", 3U);

        // BCD
        AssertValue("B0xH02", 34U);

        // static values
        AssertValue("V1234", 1234U);
        AssertValue("V+1", 1U);
        AssertValue("V-1", 0xFFFFFFFFU);

        // multiplication
        AssertValue("0xH02*1", 0x34U);
        AssertValue("0xH02*3", 156U);
        AssertValue("0xH02*0.5", 26U);
        AssertValue("0xH02*-1", 0xFFFFFFCCU);
        AssertValue("0xH02*0xH01", 936U);
        AssertValue("0xH02*~0xT02", 0x34U);                // multiply by inverse bit - T02 = 0, inverse = 1
        AssertValue("0xH02*~0xQ02", 0U);                   // multiply by inverse bit - Q02 = 1, inverse = 0
        AssertValue("0xH02*~0xH03", 0x34U * 0x54U);        // multiply by inverse byte - H03 = 0xAB, inverse = 0x54

                                                           // addition
        AssertValue("0xH02_0xH03", 0x34U + 0xABU);
        AssertValue("0xH02_V10", 0x34U + 10U);
        AssertValue("0xH02_V-10", 0x34U - 10U);
        AssertValue("0xH02*100_0xH03*0.5", 0x34U * 100U + 0xABU / 2U);

        // maximum
        AssertValue("0xH02$0xH03", 0xABU);
        AssertValue("0xH02*4$0xH03", 0x34U * 4);
        AssertValue("0xH0001_0xH0004*3$0xH0002*0xL0003", 0x34U * 0xBU); // 0x12 + 0x56 * 3 <> 0x34 * 0xB (0x114 <> 0x23C)
    }
};

} // namespace tests
} // namespace data
} // namespace ra
