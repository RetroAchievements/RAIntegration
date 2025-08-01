#include "services\SearchResults.h"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockEmulatorContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace services {
namespace tests {

_CONSTANT_VAR MAX_BLOCK_SIZE = 256U * 1024U; // assert: matches value in SearchResults.cpp
_CONSTANT_VAR BIG_BLOCK_SIZE = MAX_BLOCK_SIZE + MAX_BLOCK_SIZE + (MAX_BLOCK_SIZE / 2);

TEST_CLASS(SearchResults_Tests)
{
public:
    TEST_METHOD(TestEmpty)
    {
        SearchResults results;
        Assert::AreEqual({ 0U }, results.MatchingAddressCount());

        SearchResult result;
        Assert::IsFalse(results.GetMatchingAddress(0, result));
    }

    TEST_METHOD(TestInitializeFromMemoryEightBit)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(1U, 3U, ra::services::SearchType::EightBit);

        Assert::AreEqual({ 3U }, results.MatchingAddressCount());

        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x12U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x34U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(2U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemorySixteenBit)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(1U, 3U, ra::services::SearchType::SixteenBit);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());

        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBit, result.nSize);
        Assert::AreEqual(0x3412U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBit, result.nSize);
        Assert::AreEqual(0xAB34U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryThirtyTwoBit)
    {
        std::array<unsigned char, 6> memory{0x00, 0x12, 0x34, 0xAB, 0x56, 0xCD};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(1U, 4U, ra::services::SearchType::ThirtyTwoBit);

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());

        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsFalse(results.ContainsAddress(2U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0x56AB3412U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemorySixteenBitAligned)
    {
        std::array<unsigned char, 8> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56, 0xCD };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, 4U, ra::services::SearchType::SixteenBitAligned);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());

        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBit, result.nSize);
        Assert::AreEqual(0x1200U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBit, result.nSize);
        Assert::AreEqual(0xAB34U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryThirtyTwoBitAligned)
    {
        std::array<unsigned char, 8> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56, 0xCD, 0x44, 0x20 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, 8U, ra::services::SearchType::ThirtyTwoBitAligned);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());

        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsFalse(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsTrue(results.ContainsAddress(4U));
        Assert::IsFalse(results.ContainsAddress(5U));
        Assert::IsFalse(results.ContainsAddress(6U));
        Assert::IsFalse(results.ContainsAddress(7U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0xAB341200U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0x2044CD56U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemorySixteenBitBigEndian)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(1U, 3U, ra::services::SearchType::SixteenBitBigEndian);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());

        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBitBigEndian, result.nSize);
        Assert::AreEqual(0x1234U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBitBigEndian, result.nSize);
        Assert::AreEqual(0x34ABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryThirtyTwoBitBigEndian)
    {
        std::array<unsigned char, 6> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56, 0xCD };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(1U, 4U, ra::services::SearchType::ThirtyTwoBitBigEndian);

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());

        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsFalse(results.ContainsAddress(2U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBitBigEndian, result.nSize);
        Assert::AreEqual(0x1234AB56U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemorySixteenBitBigEndianAligned)
    {
        std::array<unsigned char, 8> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56, 0xCD };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, 4U, ra::services::SearchType::SixteenBitBigEndianAligned);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());

        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBitBigEndian, result.nSize);
        Assert::AreEqual(0x0012U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBitBigEndian, result.nSize);
        Assert::AreEqual(0x34ABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryThirtyTwoBitBigEndianAligned)
    {
        std::array<unsigned char, 8> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56, 0xCD, 0x44, 0x20 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, 8U, ra::services::SearchType::ThirtyTwoBitBigEndianAligned);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());

        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsFalse(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsTrue(results.ContainsAddress(4U));
        Assert::IsFalse(results.ContainsAddress(5U));
        Assert::IsFalse(results.ContainsAddress(6U));
        Assert::IsFalse(results.ContainsAddress(7U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBitBigEndian, result.nSize);
        Assert::AreEqual(0x001234ABU, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBitBigEndian, result.nSize);
        Assert::AreEqual(0x56CD4420U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryNibble)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(1U, 3U, ra::services::SearchType::FourBit);

        Assert::AreEqual({ 6U }, results.MatchingAddressCount());

        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        // lower nibble always returned first
        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::Nibble_Lower, result.nSize);
        Assert::AreEqual(0x2U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::Nibble_Upper, result.nSize);
        Assert::AreEqual(0x1U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(2U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::Nibble_Lower, result.nSize);
        Assert::AreEqual(0x4U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(3U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::Nibble_Upper, result.nSize);
        Assert::AreEqual(0x3U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(4U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(MemSize::Nibble_Lower, result.nSize);
        Assert::AreEqual(0xBU, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(5U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(MemSize::Nibble_Upper, result.nSize);
        Assert::AreEqual(0xAU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemorySixteenBitBounded)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(1U, 8U, ra::services::SearchType::SixteenBit);

        Assert::AreEqual({ 3U }, results.MatchingAddressCount());

        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBit, result.nSize);
        Assert::AreEqual(0x3412U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBit, result.nSize);
        Assert::AreEqual(0xAB34U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(2U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBit, result.nSize);
        Assert::AreEqual(0x56ABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitEqualsConstant)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        SearchResults results2;
        results2.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, L"0xAB");

        Assert::AreEqual({ 1U }, results2.MatchingAddressCount());
        Assert::IsFalse(results2.ContainsAddress(0U));
        Assert::IsFalse(results2.ContainsAddress(1U));
        Assert::IsFalse(results2.ContainsAddress(2U));
        Assert::IsTrue(results2.ContainsAddress(3U));
        Assert::IsFalse(results2.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitEqualsInvalidConstant)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        SearchResults results2;
        Assert::IsFalse(results2.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, L""));
        Assert::IsFalse(results2.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, L"Banana"));
        Assert::IsFalse(results2.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, L"3.14"));
        Assert::IsFalse(results2.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, L"$1234"));
        Assert::IsTrue(results2.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, L"ABBA")); // can be read as hex
        Assert::IsTrue(results2.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, L"0xAB"));
    }

    TEST_METHOD(TestInitializeFromResultsEightBitEqualsConstantGap)
    {
        std::array<unsigned char, 6> memory{ 0x00, 0x12, 0x00, 0x00, 0x56, 0x00 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 6U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 6U }, results1.MatchingAddressCount());

        SearchResults results2;
        results2.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, L"0");
        Assert::AreEqual({ 4U }, results2.MatchingAddressCount());

        memory.at(1) = 0x00; // not previously matched, should be ignored
        memory.at(3) = 0x01; // previously matched, should no longer match
        SearchResults results3;
        results3.Initialize(results2, ComparisonType::Equals, ra::services::SearchFilterType::Constant, L"0");

        Assert::AreEqual({ 3U }, results3.MatchingAddressCount());
        Assert::IsTrue(results3.ContainsAddress(0U));
        Assert::IsFalse(results3.ContainsAddress(1U));
        Assert::IsTrue(results3.ContainsAddress(2U));
        Assert::IsFalse(results3.ContainsAddress(3U));
        Assert::IsFalse(results3.ContainsAddress(4U));
        Assert::IsTrue(results3.ContainsAddress(5U));

        SearchResult result;
        Assert::IsTrue(results3.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x00U, result.nValue);

        Assert::IsTrue(results3.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x00U, result.nValue);

        Assert::IsTrue(results3.GetMatchingAddress(2U, result));
        Assert::AreEqual(5U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x00U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitNotEqualsConstant)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::Constant, L"0xAB");

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x12U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x34U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitNotEqualsConstantGap)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::Constant, L"0x34");

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsFalse(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x12U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitGreaterThanConstant)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        SearchResults results;
        results.Initialize(results1, ComparisonType::GreaterThan, ra::services::SearchFilterType::Constant, L"0x34");

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsFalse(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitGreaterThanOrEqualConstant)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        SearchResults results;
        results.Initialize(results1, ComparisonType::GreaterThanOrEqual, ra::services::SearchFilterType::Constant, L"0x34");

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x34U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitLessThanConstant)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        SearchResults results;
        results.Initialize(results1, ComparisonType::LessThan, ra::services::SearchFilterType::Constant, L"0x34");

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsFalse(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x12U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitLessThanOrEqualConstant)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        SearchResults results;
        results.Initialize(results1, ComparisonType::LessThanOrEqual, ra::services::SearchFilterType::Constant, L"0x34");

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x12U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x34U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitEqualsConstantModified)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        // swap bytes 1 and 3, match should be found at address 1, not address 3
        memory.at(1) = 0xAB;
        memory.at(3) = 0x12;
        SearchResults results;
        results.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, L"0xAB");

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsFalse(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitEqualsPrevious)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 5U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 5U }, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(2) = 0x55;
        SearchResults results2;
        results2.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 3U }, results2.MatchingAddressCount());
        Assert::IsTrue(results2.ContainsAddress(0U));
        Assert::IsFalse(results2.ContainsAddress(1U));
        Assert::IsFalse(results2.ContainsAddress(2U));
        Assert::IsTrue(results2.ContainsAddress(3U));
        Assert::IsTrue(results2.ContainsAddress(4U));

        // no change - tests the "entire block matches" optimization
        SearchResults results3;
        results3.Initialize(results2, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 3U }, results3.MatchingAddressCount());
        Assert::IsTrue(results3.ContainsAddress(0U));
        Assert::IsFalse(results3.ContainsAddress(1U));
        Assert::IsFalse(results3.ContainsAddress(2U));
        Assert::IsTrue(results3.ContainsAddress(3U));
        Assert::IsTrue(results3.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results2.GetMatchingAddress(1U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);

        // no change - tests the "entire block matches" optimization
        SearchResults results4;
        memory.at(3) = 0x99;
        results4.Initialize(results3, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 2U }, results4.MatchingAddressCount());
        Assert::IsTrue(results4.ContainsAddress(0U));
        Assert::IsFalse(results4.ContainsAddress(1U));
        Assert::IsFalse(results4.ContainsAddress(2U));
        Assert::IsFalse(results4.ContainsAddress(3U));
        Assert::IsTrue(results4.ContainsAddress(4U));

        Assert::IsTrue(results4.GetMatchingAddress(1U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x56U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitNotEqualsPrevious)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(2) = 0x55;
        SearchResults results2;
        results2.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 2U }, results2.MatchingAddressCount());
        Assert::IsFalse(results2.ContainsAddress(0U));
        Assert::IsTrue(results2.ContainsAddress(1U));
        Assert::IsTrue(results2.ContainsAddress(2U));
        Assert::IsFalse(results2.ContainsAddress(3U));
        Assert::IsFalse(results2.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x14U, result.nValue);

        Assert::IsTrue(results2.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x55U, result.nValue);

        // no change - tests the "entire block matches" optimization
        SearchResults results3;
        results3.Initialize(results2, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");
        Assert::AreEqual({ 0U }, results3.MatchingAddressCount());
    }

    TEST_METHOD(TestInitializeFromResultsEightBitNotEqualsPreviousLargeData)
    {
        std::array<unsigned char, 1024> memory{};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        for (unsigned int i = 0; i < memory.size(); ++i)
            GSL_SUPPRESS_BOUNDS4 memory[i] = (i % 256);
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 1024U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 1024U }, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(4) = 0x22;
        memory.at(12) = 0x55;
        memory.at(99) = 0x99;
        memory.at(700) = 0x00;
        SearchResults results2;
        results2.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        // expectation: results2 should allocate several smaller blocks instead of one large one
        Assert::AreEqual({ 5U }, results2.MatchingAddressCount());
        Assert::IsTrue(results2.ContainsAddress(1U));
        Assert::IsTrue(results2.ContainsAddress(4U));
        Assert::IsTrue(results2.ContainsAddress(12U));
        Assert::IsTrue(results2.ContainsAddress(99U));
        Assert::IsTrue(results2.ContainsAddress(700U));

        SearchResult result;
        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x14U, result.nValue);

        Assert::IsTrue(results2.GetMatchingAddress(1U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x22U, result.nValue);

        Assert::IsTrue(results2.GetMatchingAddress(2U, result));
        Assert::AreEqual(12U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x55U, result.nValue);

        Assert::IsTrue(results2.GetMatchingAddress(3U, result));
        Assert::AreEqual(99U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x99U, result.nValue);

        Assert::IsTrue(results2.GetMatchingAddress(4U, result));
        Assert::AreEqual(700U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x00U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitEqualsPreviousPlusTwo)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(2) = 0x36;
        SearchResults results;
        results.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValuePlus, L"2");

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x14U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x36U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitEqualsPreviousMinusTwo)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        memory.at(1) = 0x10;
        memory.at(2) = 0x32;
        SearchResults results;
        results.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValueMinus, L"2");

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x10U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x32U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitGreaterThanPreviousPlusTwo)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(2) = 0x37;
        SearchResults results;
        results.Initialize(results1, ComparisonType::GreaterThan, ra::services::SearchFilterType::LastKnownValuePlus, L"2");

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x37U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsSixteenBitNotEqualPrevious)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 4U, ra::services::SearchType::SixteenBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        memory.at(2) = 0x55;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBit, result.nSize);
        Assert::AreEqual(0x5512U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBit, result.nSize);
        Assert::AreEqual(0xAB55U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsThirtyTwoBitNotEqualPrevious)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 5U, ra::services::SearchType::ThirtyTwoBit);
        Assert::AreEqual({ 2U }, results1.MatchingAddressCount());

        memory.at(4) = 0x55;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0x55AB3412U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsSixteenBitAlignedNotEqualPrevious)
    {
        std::array<unsigned char, 8> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56, 0xCD };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 4U, ra::services::SearchType::SixteenBitAligned);
        Assert::AreEqual({ 2U }, results1.MatchingAddressCount());

        memory.at(1) = 0x55;
        memory.at(3) = 0x66;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(2U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBit, result.nSize);
        Assert::AreEqual(0x5500U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBit, result.nSize);
        Assert::AreEqual(0x6634U, result.nValue);

        memory.at(2) = 0x99;
        SearchResults results2;
        results2.Initialize(results, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 1U }, results2.MatchingAddressCount());
        Assert::IsFalse(results2.ContainsAddress(0U));
        Assert::IsTrue(results2.ContainsAddress(2U));

        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBit, result.nSize);
        Assert::AreEqual(0x6699U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsThirtyTwoBitAlignedNotEqualPrevious)
    {
        std::array<unsigned char, 8> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56, 0xCD, 0x44, 0x20 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 8U, ra::services::SearchType::ThirtyTwoBitAligned);
        Assert::AreEqual({ 2U }, results1.MatchingAddressCount());

        memory.at(2) = 0x55;
        memory.at(4) = 0x66;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0xAB551200U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0x2044CD66U, result.nValue);

        memory.at(6) = 0x99;
        SearchResults results2;
        results2.Initialize(results, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 1U }, results2.MatchingAddressCount());
        Assert::IsFalse(results2.ContainsAddress(0U));
        Assert::IsTrue(results2.ContainsAddress(4U));

        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0x2099CD66U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsThirtyTwoBitAlignedNotEqualPreviousLargeMemory)
    {
        // WHY is the std::move necessary here, but not in other LargeMemory tests? (C26414)
        auto memory = std::move(std::make_unique<unsigned char[]>(BIG_BLOCK_SIZE));
        for (unsigned int i = 0; i < BIG_BLOCK_SIZE; ++i)
            GSL_SUPPRESS_BOUNDS4 memory[i] = (i % 256);
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory.get(), BIG_BLOCK_SIZE);

        SearchResults results;
        results.Initialize(0U, BIG_BLOCK_SIZE, ra::services::SearchType::ThirtyTwoBitAligned);
        Assert::AreEqual({ BIG_BLOCK_SIZE / 4 }, results.MatchingAddressCount());

        GSL_SUPPRESS_BOUNDS4 memory[0x000002] = 0x55;
        GSL_SUPPRESS_BOUNDS4 memory[0x010003] = 0x66;
        SearchResults results1;
        results1.Initialize(results, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 2U }, results1.MatchingAddressCount());
        Assert::IsTrue(results1.ContainsAddress(0x000000U));
        Assert::IsTrue(results1.ContainsAddress(0x010000U));

        SearchResult result;
        Assert::IsTrue(results1.GetMatchingAddress(0U, result));
        Assert::AreEqual(0x000000U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0x03550100U, result.nValue);
        Assert::IsTrue(results1.MatchesFilter(results, result));

        Assert::IsTrue(results1.GetMatchingAddress(1U, result));
        Assert::AreEqual(0x010000U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0x66020100U, result.nValue);
        Assert::IsTrue(results1.MatchesFilter(results, result));

        GSL_SUPPRESS_BOUNDS4 memory[0x010001] = 0x99;
        SearchResults results2;
        results2.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 1U }, results2.MatchingAddressCount());
        Assert::IsFalse(results2.ContainsAddress(0x000000U));
        Assert::IsTrue(results2.ContainsAddress(0x010000U));

        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(0x010000U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0x66029900U, result.nValue);
        Assert::IsTrue(results2.MatchesFilter(results1, result));
    }

    TEST_METHOD(TestInitializeFromResultsThirtyTwoBitAlignedOffset)
    {
        std::array<unsigned char, 12> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56, 0xCD, 0x44, 0x20, 0x11, 0x22, 0x33, 0x44 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(4U, 12U, ra::services::SearchType::ThirtyTwoBitAligned);
        Assert::AreEqual({ 2U }, results1.MatchingAddressCount());

        memory.at(6) = 0x55;
        SearchResults results2;
        results2.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 1U }, results2.MatchingAddressCount());
        Assert::IsFalse(results2.ContainsAddress(0U));
        Assert::IsFalse(results2.ContainsAddress(4U));
        Assert::IsTrue(results2.ContainsAddress(8U));

        SearchResults results;
        results.Initialize(results1, results2, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(4U));
        Assert::IsTrue(results.ContainsAddress(8U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(8U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0x44332211U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsSixteenBitBigEndianNotEqualPrevious)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 4U, ra::services::SearchType::SixteenBitBigEndian);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        memory.at(2) = 0x55;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBitBigEndian, result.nSize);
        Assert::AreEqual(0x1255U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBitBigEndian, result.nSize);
        Assert::AreEqual(0x55ABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsThirtyTwoBitBigEndianNotEqualPrevious)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 5U, ra::services::SearchType::ThirtyTwoBitBigEndian);
        Assert::AreEqual({ 2U }, results1.MatchingAddressCount());

        memory.at(4) = 0x55;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBitBigEndian, result.nSize);
        Assert::AreEqual(0x1234AB55U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsThirtyTwoBitBigEndianLessThanPrevious)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 5U, ra::services::SearchType::ThirtyTwoBitBigEndian);
        Assert::AreEqual({ 2U }, results1.MatchingAddressCount());

        memory.at(4) = 0x55;
        SearchResults results;
        results.Initialize(results1, ComparisonType::LessThan, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBitBigEndian, result.nSize);
        Assert::AreEqual(0x1234AB55U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsSixteenBitBigEndianAlignedNotEqualPrevious)
    {
        std::array<unsigned char, 8> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56, 0xCD };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 4U, ra::services::SearchType::SixteenBitBigEndianAligned);
        Assert::AreEqual({ 2U }, results1.MatchingAddressCount());

        memory.at(1) = 0x55;
        memory.at(3) = 0x66;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(2U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBitBigEndian, result.nSize);
        Assert::AreEqual(0x0055U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBitBigEndian, result.nSize);
        Assert::AreEqual(0x3466U, result.nValue);

        memory.at(2) = 0x99;
        SearchResults results2;
        results2.Initialize(results, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 1U }, results2.MatchingAddressCount());
        Assert::IsFalse(results2.ContainsAddress(0U));
        Assert::IsTrue(results2.ContainsAddress(2U));

        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBitBigEndian, result.nSize);
        Assert::AreEqual(0x9966U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsThirtyTwoBitBigEndianAlignedNotEqualPrevious)
    {
        std::array<unsigned char, 8> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56, 0xCD, 0x44, 0x20 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 8U, ra::services::SearchType::ThirtyTwoBitBigEndianAligned);
        Assert::AreEqual({ 2U }, results1.MatchingAddressCount());

        memory.at(2) = 0x55;
        memory.at(4) = 0x66;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBitBigEndian, result.nSize);
        Assert::AreEqual(0x001255ABU, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBitBigEndian, result.nSize);
        Assert::AreEqual(0x66CD4420U, result.nValue);

        memory.at(6) = 0x99;
        SearchResults results2;
        results2.Initialize(results, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 1U }, results2.MatchingAddressCount());
        Assert::IsFalse(results2.ContainsAddress(0U));
        Assert::IsTrue(results2.ContainsAddress(4U));

        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBitBigEndian, result.nSize);
        Assert::AreEqual(0x66CD9920U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsFourBitNotEqualsPrevious)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::FourBit);
        Assert::AreEqual({ 6U }, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(2) = 0x55;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 3U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::Nibble_Lower, result.nSize);
        Assert::AreEqual(4U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::Nibble_Lower, result.nSize);
        Assert::AreEqual(5U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(2U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::Nibble_Upper, result.nSize);
        Assert::AreEqual(5U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsFourBitEqualsSameByte)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(1U, 3U, ra::services::SearchType::FourBit);
        Assert::AreEqual({ 6U }, results.MatchingAddressCount());

        SearchResults filtered1;
        filtered1.Initialize(results, ComparisonType::Equals, ra::services::SearchFilterType::Constant, L"1");
        Assert::AreEqual({ 1U }, filtered1.MatchingAddressCount());

        SearchResult result;
        Assert::IsTrue(filtered1.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::Nibble_Upper, result.nSize);
        Assert::AreEqual(1U, result.nValue);

        // Nibble_Upper no longer matches, but Nibble_Lower does. Neither should not be returned.
        memory.at(1) = 0x21;
        SearchResults filtered2;
        filtered2.Initialize(filtered1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, L"1");
        Assert::AreEqual({ 0U }, filtered2.MatchingAddressCount());

        // Both nibbles match, only previously matched one should be returned
        memory.at(1) = 0x11;
        SearchResults filtered3;
        filtered3.Initialize(filtered1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, L"1");
        Assert::AreEqual({ 1U }, filtered3.MatchingAddressCount());

        Assert::IsTrue(filtered3.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::Nibble_Upper, result.nSize);
        Assert::AreEqual(1U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsWithAddressLimits)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x04, 0x16, 0x08};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 5U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 5U }, results1.MatchingAddressCount());

        SearchResults results2;
        results2.Initialize(results1, ComparisonType::GreaterThan, ra::services::SearchFilterType::Constant, L"0x10");
        Assert::AreEqual({ 2U }, results2.MatchingAddressCount());
        Assert::IsFalse(results2.ContainsAddress(0U));
        Assert::IsTrue(results2.ContainsAddress(1U));
        Assert::IsFalse(results2.ContainsAddress(2U));
        Assert::IsTrue(results2.ContainsAddress(3U));
        Assert::IsFalse(results2.ContainsAddress(4U));

        memory.at(1) = 1;
        memory.at(2) = 2;
        // Last Known Value would match indices 0, 3, and 4 as indices 1 and 2 were just changed
        // results2 limits search to indices 1 and 3, so only 3 should be matched
        SearchResults results3;
        results3.Initialize(results1, results2, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValue, L"");
        Assert::AreEqual({ 1U }, results3.MatchingAddressCount());
        Assert::IsFalse(results3.ContainsAddress(0U));
        Assert::IsFalse(results3.ContainsAddress(1U));
        Assert::IsFalse(results3.ContainsAddress(2U));
        Assert::IsTrue(results3.ContainsAddress(3U));
        Assert::IsFalse(results3.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results3.GetMatchingAddress(0U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x16U, result.nValue);
    }

    TEST_METHOD(TestExcludeAddressEightBit)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        // exclude doesn't do anything to unfiltered results
        const SearchResult excludeResult{ 1U, 0U, MemSize::EightBit };
        results1.ExcludeResult(excludeResult);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(2) = 0x55;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        results.ExcludeResult(excludeResult);
        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x55U, result.nValue);
    }

    TEST_METHOD(TestExcludeAddressFourBit)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::FourBit);
        Assert::AreEqual({ 6U }, results1.MatchingAddressCount());

        // exclude doesn't do anything to unfiltered results
        SearchResult excludeResult{ 1U, 0U, MemSize::Nibble_Lower };
        results1.ExcludeResult(excludeResult);
        Assert::AreEqual({ 6U }, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(2) = 0x65;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 3U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        results.ExcludeResult(excludeResult);
        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::Nibble_Lower, result.nSize);
        Assert::AreEqual(0x5U, result.nValue);

        excludeResult.nAddress = 2;
        excludeResult.nSize = MemSize::Nibble_Upper;
        results.ExcludeResult(excludeResult);
        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::Nibble_Lower, result.nSize);
        Assert::AreEqual(0x5U, result.nValue);
    }

    TEST_METHOD(TestExcludeAddressSixteenBitAligned)
    {
        std::array<unsigned char, 6> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56, 0x88 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 5U, ra::services::SearchType::SixteenBitAligned);
        Assert::AreEqual({ 2U }, results1.MatchingAddressCount());

        // exclude doesn't do anything to unfiltered results
        const SearchResult excludeResult{ 0U, 0U, MemSize::EightBit };
        results1.ExcludeResult(excludeResult);
        Assert::AreEqual({ 2U }, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(2) = 0x55;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        results.ExcludeResult(excludeResult);
        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBit, result.nSize);
        Assert::AreEqual(0xAB55U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryEightBitLargeMemory)
    {
        auto memory = std::make_unique<unsigned char[]>(BIG_BLOCK_SIZE);
        for (unsigned int i = 0; i < BIG_BLOCK_SIZE; ++i)
            GSL_SUPPRESS_BOUNDS4 memory[i] = (i % 256);
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory.get(), BIG_BLOCK_SIZE);

        SearchResults results;
        results.Initialize(0U, BIG_BLOCK_SIZE, ra::services::SearchType::EightBit);

        Assert::AreEqual({ BIG_BLOCK_SIZE }, results.MatchingAddressCount());
        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(MAX_BLOCK_SIZE - 1));
        Assert::IsTrue(results.ContainsAddress(MAX_BLOCK_SIZE));
        Assert::IsTrue(results.ContainsAddress(BIG_BLOCK_SIZE - 1));
        Assert::IsFalse(results.ContainsAddress(BIG_BLOCK_SIZE));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(MAX_BLOCK_SIZE - 1, result));
        Assert::AreEqual(MAX_BLOCK_SIZE - 1, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0xFFU, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(MAX_BLOCK_SIZE, result));
        Assert::AreEqual(MAX_BLOCK_SIZE, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x00U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(BIG_BLOCK_SIZE - 1, result));
        Assert::AreEqual(BIG_BLOCK_SIZE - 1, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0xFFU, result.nValue);

        memory.reset();
    }

    TEST_METHOD(TestInitializeFromMemorySixteenBitLargeMemory)
    {
        auto memory = std::make_unique<unsigned char[]>(BIG_BLOCK_SIZE);
        for (unsigned int i = 0; i < BIG_BLOCK_SIZE; ++i)
            GSL_SUPPRESS_BOUNDS4 memory[i] = (i % 256);
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory.get(), BIG_BLOCK_SIZE);

        SearchResults results;
        results.Initialize(0U, BIG_BLOCK_SIZE, ra::services::SearchType::SixteenBit);

        Assert::AreEqual({ BIG_BLOCK_SIZE - 1 }, results.MatchingAddressCount());
        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(MAX_BLOCK_SIZE - 1));
        Assert::IsTrue(results.ContainsAddress(MAX_BLOCK_SIZE));
        Assert::IsTrue(results.ContainsAddress(BIG_BLOCK_SIZE - 2));
        Assert::IsFalse(results.ContainsAddress(BIG_BLOCK_SIZE - 1));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(MAX_BLOCK_SIZE - 1, result));
        Assert::AreEqual(MAX_BLOCK_SIZE - 1, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBit, result.nSize);
        Assert::AreEqual(0x00FFU, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(MAX_BLOCK_SIZE, result));
        Assert::AreEqual(MAX_BLOCK_SIZE, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBit, result.nSize);
        Assert::AreEqual(0x0100U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(BIG_BLOCK_SIZE - 2, result));
        Assert::AreEqual(BIG_BLOCK_SIZE - 2, result.nAddress);
        Assert::AreEqual(MemSize::SixteenBit, result.nSize);
        Assert::AreEqual(0xFFFEU, result.nValue);

        memory.reset();
    }

    TEST_METHOD(TestInitializeFromMemorySixteenBitZeroBytes)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, 0U, ra::services::SearchType::SixteenBit);

        Assert::AreEqual({ 0U }, results.MatchingAddressCount());
    }

    TEST_METHOD(TestInitializeFromMemoryAsciiText)
    {
        std::array<unsigned char, 36> memory {
            'S', 'h', 'e', ' ', 's', 'e', 'l', 'l', 's', ' ', 's', 'e', 'a', 's', 'h', 'e',
            'l', 'l', 's', ' ', 'b', 'y', ' ', 't', 'h', 'e', ' ', 's', 'e', 'a', 's', 'h',
            'o', 'r', 'e', '.'
        };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, memory.size(), ra::services::SearchType::AsciiText);

        Assert::AreEqual(memory.size(), results.MatchingAddressCount());

        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(35U));
        Assert::IsFalse(results.ContainsAddress(36U));

        // nValue for ASCII text is the first byte of the string. use FormattedValue to get the whole string
        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::Text, result.nSize);
        Assert::AreEqual({ 'S' }, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(35U, result));
        Assert::AreEqual(35U, result.nAddress);
        Assert::AreEqual(MemSize::Text, result.nSize);
        Assert::AreEqual({ '.' }, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryFloat)
    {
        std::array<unsigned char, 8> memory{ 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x46, 0x41 }; // -2.0, 12.375
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, memory.size(), ra::services::SearchType::Float);
        Assert::AreEqual({ 5U }, results.MatchingAddressCount());

        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsTrue(results.ContainsAddress(4U));
        Assert::IsFalse(results.ContainsAddress(5U));
        Assert::IsFalse(results.ContainsAddress(6U));
        Assert::IsFalse(results.ContainsAddress(7U));

        // nValue is an IEE-754 encoded float
        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::Float, result.nSize);
        Assert::AreEqual(0xC0000000U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(4U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::Float, result.nSize);
        Assert::AreEqual(0x41460000U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryFloatBE)
    {
        std::array<unsigned char, 8> memory{ 0xC0, 0x00, 0x00, 0x00, 0x41, 0x46, 0x00, 0x00 }; // -2.0, 12.375
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, memory.size(), ra::services::SearchType::FloatBigEndian);
        Assert::AreEqual({5U}, results.MatchingAddressCount());

        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsTrue(results.ContainsAddress(4U));
        Assert::IsFalse(results.ContainsAddress(5U));
        Assert::IsFalse(results.ContainsAddress(6U));
        Assert::IsFalse(results.ContainsAddress(7U));

        // nValue is an BigEndian IEE-754 encoded float
        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::FloatBigEndian, result.nSize);
        Assert::AreEqual(0x000000C0U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(4U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::FloatBigEndian, result.nSize);
        Assert::AreEqual(0x00004641U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryDouble32)
    {
        std::array<unsigned char, 16> memory{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0,  // -2.0
                                             0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x28, 0x40}; // 12.375
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, memory.size(), ra::services::SearchType::Double32);
        Assert::AreEqual({13U}, results.MatchingAddressCount());

        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsTrue(results.ContainsAddress(4U));
        Assert::IsTrue(results.ContainsAddress(8U));
        Assert::IsTrue(results.ContainsAddress(12U));
        Assert::IsFalse(results.ContainsAddress(13U));
        Assert::IsFalse(results.ContainsAddress(14U));
        Assert::IsFalse(results.ContainsAddress(15U));

        // nValue is the four most signifcant bytes of an IEE-754 encoded double
        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(4U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::Double32, result.nSize);
        Assert::AreEqual(0xC0000000U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(12U, result));
        Assert::AreEqual(12U, result.nAddress);
        Assert::AreEqual(MemSize::Double32, result.nSize);
        Assert::AreEqual(0x4028C000U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryDouble32BE)
    {
        std::array<unsigned char, 16> memory{0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // -2.0
                                             0x40, 0x28, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00}; // 12.375
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, memory.size(), ra::services::SearchType::Double32BigEndian);
        Assert::AreEqual({13U}, results.MatchingAddressCount());

        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsTrue(results.ContainsAddress(4U));
        Assert::IsTrue(results.ContainsAddress(8U));
        Assert::IsTrue(results.ContainsAddress(12U));
        Assert::IsFalse(results.ContainsAddress(13U));
        Assert::IsFalse(results.ContainsAddress(14U));
        Assert::IsFalse(results.ContainsAddress(15U));

        // nValue is the four most signifcant bytes of an BigEndian IEE-754 encoded double
        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::Double32BigEndian, result.nSize);
        Assert::AreEqual(0x000000C0U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(8U, result));
        Assert::AreEqual(8U, result.nAddress);
        Assert::AreEqual(MemSize::Double32BigEndian, result.nSize);
        Assert::AreEqual(0x00C02840U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryMBF32)
    {
        std::array<unsigned char, 8> memory{ 0x87, 0x46, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00 }; // 99.0, -0.5
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, memory.size(), ra::services::SearchType::MBF32);
        Assert::AreEqual({ 5U }, results.MatchingAddressCount());

        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsTrue(results.ContainsAddress(4U));
        Assert::IsFalse(results.ContainsAddress(5U));
        Assert::IsFalse(results.ContainsAddress(6U));
        Assert::IsFalse(results.ContainsAddress(7U));

        // nValue is an MBF32 encoded float
        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::MBF32, result.nSize);
        Assert::AreEqual(0x00004687U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(4U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::MBF32, result.nSize);
        Assert::AreEqual(0x00008080U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryMBF32LE)
    {
        std::array<unsigned char, 8> memory{ 0x00, 0x00, 0x46, 0x87, 0x00, 0x00, 0x80, 0x80 }; // 99.0, -0.5
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, memory.size(), ra::services::SearchType::MBF32LE);
        Assert::AreEqual({ 5U }, results.MatchingAddressCount());

        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsTrue(results.ContainsAddress(4U));
        Assert::IsFalse(results.ContainsAddress(5U));
        Assert::IsFalse(results.ContainsAddress(6U));
        Assert::IsFalse(results.ContainsAddress(7U));

        // nValue is an MBF32 encoded float
        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::MBF32LE, result.nSize);
        Assert::AreEqual(0x87460000U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(4U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::MBF32LE, result.nSize);
        Assert::AreEqual(0x80800000U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsAsciiText)
    {
        std::array<unsigned char, 36> memory{
            'S', 'h', 'e', ' ', 's', 'e', 'l', 'l', 's', ' ', 's', 'e', 'a', 's', 'h', 'e',
            'l', 'l', 's', ' ', 'b', 'y', ' ', 't', 'h', 'e', ' ', 's', 'e', 'a', 's', 'h',
            'o', 'r', 'e', '.'
        };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, memory.size(), ra::services::SearchType::AsciiText);
        Assert::AreEqual(memory.size(), results1.MatchingAddressCount());

        SearchResults results2;
        results2.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, L"he");
        Assert::AreEqual({ 3U }, results2.MatchingAddressCount());
        Assert::IsFalse(results2.ContainsAddress(0U));
        Assert::IsTrue(results2.ContainsAddress(1U));
        Assert::IsFalse(results2.ContainsAddress(2U));
        Assert::IsTrue(results2.ContainsAddress(14U));
        Assert::IsTrue(results2.ContainsAddress(24U));

        SearchResult result;
        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);

        Assert::IsTrue(results2.GetMatchingAddress(1U, result));
        Assert::AreEqual(14U, result.nAddress);

        Assert::IsTrue(results2.GetMatchingAddress(2U, result));
        Assert::AreEqual(24U, result.nAddress);

        memory.at(15U) = 'i';
        SearchResults results3;
        results3.Initialize(results2, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValue, results2.GetFilterString());
        Assert::AreEqual({ 2U }, results3.MatchingAddressCount());
        Assert::IsTrue(results3.ContainsAddress(1U));
        Assert::IsFalse(results3.ContainsAddress(14U));
        Assert::IsTrue(results3.ContainsAddress(24U));

        Assert::IsTrue(results3.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);

        Assert::IsTrue(results3.GetMatchingAddress(1U, result));
        Assert::AreEqual(24U, result.nAddress);
    }

    TEST_METHOD(TestInitializeFromResultsAsciiTextSingleMatch)
    {
        std::array<unsigned char, 36> memory{
            'S', 'h', 'e', ' ', 's', 'e', 'l', 'l', 's', ' ', 's', 'e', 'a', 's', 'h', 'e',
            'l', 'l', 's', ' ', 'b', 'y', ' ', 't', 'h', 'e', ' ', 's', 'e', 'a', 's', 'h',
            'o', 'r', 'e', '.'
        };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, memory.size(), ra::services::SearchType::AsciiText);
        Assert::AreEqual(memory.size(), results1.MatchingAddressCount());

        SearchResults results2;
        results2.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, L"by");
        Assert::AreEqual({ 1U }, results2.MatchingAddressCount());
        Assert::IsTrue(results2.ContainsAddress(20U));

        SearchResult result;
        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(20U, result.nAddress);
    }

    TEST_METHOD(TestInitializeFromResultsAsciiTextComparison)
    {
        std::array<unsigned char, 36> memory{
            'S', 'h', 'e', ' ', 's', 'e', 'l', 'l', 's', ' ', 's', 'e', 'a', 's', 'h', 'e',
            'l', 'l', 's', ' ', 'b', 'y', ' ', 't', 'h', 'e', ' ', 's', 'e', 'a', 's', 'h',
            'o', 'r', 'e', '.'
        };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, memory.size(), ra::services::SearchType::AsciiText);
        Assert::AreEqual(memory.size(), results1.MatchingAddressCount());

        SearchResults results2;
        results2.Initialize(results1, ComparisonType::GreaterThanOrEqual, ra::services::SearchFilterType::Constant, L"se");
        Assert::AreEqual({ 7U }, results2.MatchingAddressCount());

        SearchResult result;
        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(4U, result.nAddress); // sells

        Assert::IsTrue(results2.GetMatchingAddress(1U, result));
        Assert::AreEqual(10U, result.nAddress); // seashells

        Assert::IsTrue(results2.GetMatchingAddress(2U, result));
        Assert::AreEqual(13U, result.nAddress); // shells

        Assert::IsTrue(results2.GetMatchingAddress(3U, result));
        Assert::AreEqual(21U, result.nAddress); // y the

        Assert::IsTrue(results2.GetMatchingAddress(4U, result));
        Assert::AreEqual(23U, result.nAddress); // the

        Assert::IsTrue(results2.GetMatchingAddress(5U, result));
        Assert::AreEqual(27U, result.nAddress); // seashore

        Assert::IsTrue(results2.GetMatchingAddress(6U, result));
        Assert::AreEqual(30U, result.nAddress); // shore

        // should filter out "y the" and "the"
        SearchResults results3;
        results3.Initialize(results2, ComparisonType::LessThan, ra::services::SearchFilterType::Constant, L"the");
        Assert::AreEqual({ 5U }, results3.MatchingAddressCount());

        Assert::IsTrue(results3.GetMatchingAddress(0U, result));
        Assert::AreEqual(4U, result.nAddress); // sells

        Assert::IsTrue(results3.GetMatchingAddress(1U, result));
        Assert::AreEqual(10U, result.nAddress); // seashells

        Assert::IsTrue(results3.GetMatchingAddress(2U, result));
        Assert::AreEqual(13U, result.nAddress); // shells

        Assert::IsTrue(results3.GetMatchingAddress(3U, result));
        Assert::AreEqual(27U, result.nAddress); // seashore

        Assert::IsTrue(results3.GetMatchingAddress(4U, result));
        Assert::AreEqual(30U, result.nAddress); // shore

        // should filter out "seashells", "seashore"
        SearchResults results4;
        results4.Initialize(results3, ComparisonType::NotEqualTo, ra::services::SearchFilterType::Constant, L"sea");
        Assert::AreEqual({ 3U }, results4.MatchingAddressCount());

        Assert::IsTrue(results4.GetMatchingAddress(0U, result));
        Assert::AreEqual(4U, result.nAddress); // sells

        Assert::IsTrue(results4.GetMatchingAddress(1U, result));
        Assert::AreEqual(13U, result.nAddress); // shells

        Assert::IsTrue(results4.GetMatchingAddress(2U, result));
        Assert::AreEqual(30U, result.nAddress); // shore
    }

    TEST_METHOD(TestInitializeFromResultsFloatEqualsConstant)
    {
        std::array<unsigned char, 8> memory{ 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x46, 0x41 }; // -2.0, 12.375
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, memory.size(), ra::services::SearchType::Float);
        Assert::AreEqual({ 5U }, results.MatchingAddressCount());

        SearchResults results2;
        results2.Initialize(results, ComparisonType::Equals, SearchFilterType::Constant, L"-2");
        Assert::AreEqual({ 1U }, results2.MatchingAddressCount());

        // nValue is an IEE-754 encoded float
        SearchResult result;
        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::Float, result.nSize);
        Assert::AreEqual(0xC0000000U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsFloatGreaterThanLast)
    {
        std::array<unsigned char, 8> memory{ 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x46, 0x41 }; // -2.0, 12.375
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, memory.size(), ra::services::SearchType::Float);
        Assert::AreEqual({ 5U }, results.MatchingAddressCount());

        memory.at(3) = 0xBF; // -2.0 -> -1.0
        memory.at(2) = 0x80;

        SearchResults results2;
        results2.Initialize(results, ComparisonType::GreaterThan, SearchFilterType::LastKnownValue, L"");
        Assert::AreEqual({ 1U }, results2.MatchingAddressCount());

        // nValue is an IEE-754 encoded float
        SearchResult result;
        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::Float, result.nSize);
        Assert::AreEqual(0xBF800000U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsFloatBEGreaterThanLast)
    {
        std::array<unsigned char, 8> memory{ 0xC0, 0x00, 0x00, 0x00, 0x41, 0x46, 0x00, 0x00 }; // -2.0, 12.375
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, memory.size(), ra::services::SearchType::FloatBigEndian);
        Assert::AreEqual({ 5U }, results.MatchingAddressCount());

        memory.at(0) = 0xBF; // -2.0 -> -1.0
        memory.at(1) = 0x80;

        SearchResults results2;
        results2.Initialize(results, ComparisonType::GreaterThan, SearchFilterType::LastKnownValue, L"");
        Assert::AreEqual({ 1U }, results2.MatchingAddressCount());

        // nValue is an BigEndian IEE-754 encoded float
        SearchResult result;
        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::FloatBigEndian, result.nSize);
        Assert::AreEqual(0x000080BFU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsMBF32EqualsConstant)
    {
        std::array<unsigned char, 8> memory{ 0x87, 0x46, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00 }; // 99.0, -0.5
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, memory.size(), ra::services::SearchType::MBF32);
        Assert::AreEqual({ 5U }, results.MatchingAddressCount());

        SearchResults results2;
        results2.Initialize(results, ComparisonType::Equals, SearchFilterType::Constant, L"-0.5");
        Assert::AreEqual({ 1U }, results2.MatchingAddressCount());

        // nValue is an MBF32 encoded float
        SearchResult result;
        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::MBF32, result.nSize);
        Assert::AreEqual(0x00008080U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsMBF32GreaterThanLast)
    {
        std::array<unsigned char, 8> memory{ 0x87, 0x46, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00 }; // 99.0, -0.5
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, memory.size(), ra::services::SearchType::MBF32);
        Assert::AreEqual({ 5U }, results.MatchingAddressCount());

        memory.at(4) = 0x00; // -0.5 -> -0.0

        SearchResults results2;
        results2.Initialize(results, ComparisonType::GreaterThan, SearchFilterType::LastKnownValue, L"");
        Assert::AreEqual({ 2U }, results2.MatchingAddressCount());

        // nValue is an MBF32 encoded float
        SearchResult result;
        Assert::IsTrue(results2.GetMatchingAddress(1U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::MBF32, result.nSize);
        Assert::AreEqual(0x00008000U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsMBF32LEEqualsConstant)
    {
        std::array<unsigned char, 8> memory{ 0x00, 0x00, 0x46, 0x87, 0x00, 0x00, 0x80, 0x80 }; // 99.0, -0.5
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, memory.size(), ra::services::SearchType::MBF32LE);
        Assert::AreEqual({ 5U }, results.MatchingAddressCount());

        SearchResults results2;
        results2.Initialize(results, ComparisonType::Equals, SearchFilterType::Constant, L"-0.5");
        Assert::AreEqual({ 1U }, results2.MatchingAddressCount());

        // nValue is an MBF32 encoded float
        SearchResult result;
        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::MBF32LE, result.nSize);
        Assert::AreEqual(0x80800000U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsMBF32LEGreaterThanLast)
    {
        std::array<unsigned char, 8> memory{ 0x00, 0x00, 0x46, 0x87, 0x00, 0x00, 0x80, 0x80 }; // 99.0, -0.5
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(0U, memory.size(), ra::services::SearchType::MBF32LE);
        Assert::AreEqual({ 5U }, results.MatchingAddressCount());

        memory.at(7) = 0x00; // -0.5 -> -0.0

        SearchResults results2;
        results2.Initialize(results, ComparisonType::GreaterThan, SearchFilterType::LastKnownValue, L"");
        Assert::AreEqual({ 1U }, results2.MatchingAddressCount());

        // nValue is an MBF32 encoded float
        SearchResult result;
        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::MBF32LE, result.nSize);
        Assert::AreEqual(0x00800000U, result.nValue);
    }

    TEST_METHOD(TestCopyConstructor)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        // exclude doesn't do anything to unfiltered results
        const SearchResult excludeResult{ 0U, 0U, MemSize::EightBit };
        results1.ExcludeResult(excludeResult);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(2) = 0x55;
        SearchResults results2;
        results2.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, L"");
        Assert::AreEqual({ 2U }, results2.MatchingAddressCount());

        SearchResults results3(results2);

        Assert::AreEqual({ 2U }, results3.MatchingAddressCount());
        Assert::IsFalse(results3.ContainsAddress(0U));
        Assert::IsTrue(results3.ContainsAddress(1U));
        Assert::IsTrue(results3.ContainsAddress(2U));
        Assert::IsFalse(results3.ContainsAddress(3U));
        Assert::IsFalse(results3.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results3.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x14U, result.nValue);

        Assert::IsTrue(results3.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x55U, result.nValue);
    }

    TEST_METHOD(TestGetFormattedValueEightBit)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x02, 0x34, 0xAB, 0x56 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        Assert::AreEqual(std::wstring(L""), results1.GetFormattedValue(0U, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"0x02"), results1.GetFormattedValue(1U, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"0x34"), results1.GetFormattedValue(2U, MemSize::EightBit));
        Assert::AreEqual(std::wstring(L"0xab"), results1.GetFormattedValue(3U, MemSize::EightBit));
    }

    TEST_METHOD(TestGetFormattedValueSixteenBit)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x02, 0x34, 0xAB, 0x56 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::SixteenBit);
        Assert::AreEqual({ 2U }, results1.MatchingAddressCount());

        Assert::AreEqual(std::wstring(L""), results1.GetFormattedValue(0U, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"0x3402"), results1.GetFormattedValue(1U, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L"0xab34"), results1.GetFormattedValue(2U, MemSize::SixteenBit));
        Assert::AreEqual(std::wstring(L""), results1.GetFormattedValue(3U, MemSize::SixteenBit));
    }

    TEST_METHOD(TestGetFormattedValueFourBit)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x02, 0x34, 0xAB, 0x56 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::FourBit);
        Assert::AreEqual({ 6U }, results1.MatchingAddressCount());

        Assert::AreEqual(std::wstring(L""), results1.GetFormattedValue(0U, MemSize::Nibble_Lower));
        Assert::AreEqual(std::wstring(L""), results1.GetFormattedValue(0U, MemSize::Nibble_Upper));
        Assert::AreEqual(std::wstring(L"0x2"), results1.GetFormattedValue(1U, MemSize::Nibble_Lower));
        Assert::AreEqual(std::wstring(L"0x0"), results1.GetFormattedValue(1U, MemSize::Nibble_Upper));
        Assert::AreEqual(std::wstring(L"0x4"), results1.GetFormattedValue(2U, MemSize::Nibble_Lower));
        Assert::AreEqual(std::wstring(L"0x3"), results1.GetFormattedValue(2U, MemSize::Nibble_Upper));
        Assert::AreEqual(std::wstring(L"0xb"), results1.GetFormattedValue(3U, MemSize::Nibble_Lower));
        Assert::AreEqual(std::wstring(L"0xa"), results1.GetFormattedValue(3U, MemSize::Nibble_Upper));
        Assert::AreEqual(std::wstring(L""), results1.GetFormattedValue(4U, MemSize::Nibble_Lower));
        Assert::AreEqual(std::wstring(L""), results1.GetFormattedValue(4U, MemSize::Nibble_Upper));
    }

    TEST_METHOD(TestGetFormattedValueFloat)
    {
        std::array<unsigned char, 8> memory{ 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x46, 0x41 }; // -2.0, 12.375
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 8U, ra::services::SearchType::Float);
        Assert::AreEqual({ 5U }, results1.MatchingAddressCount());

        Assert::AreEqual(std::wstring(L"-2.0"), results1.GetFormattedValue(0U, MemSize::Float));
        Assert::AreEqual(std::wstring(L"12.375"), results1.GetFormattedValue(4U, MemSize::Float));
        Assert::AreEqual(std::wstring(L"6.88766e-41"), results1.GetFormattedValue(2U, MemSize::Float));
        Assert::AreEqual(std::wstring(L"8192.1875"), results1.GetFormattedValue(3U, MemSize::Float));
    }

    TEST_METHOD(TestGetFormattedValueMBF32)
    {
        std::array<unsigned char, 8> memory{ 0x87, 0x46, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00 }; // 99.0, -0.5
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 8U, ra::services::SearchType::MBF32);
        Assert::AreEqual({ 5U }, results1.MatchingAddressCount());

        Assert::AreEqual(std::wstring(L"99.0"), results1.GetFormattedValue(0U, MemSize::MBF32));
        Assert::AreEqual(std::wstring(L"-0.5"), results1.GetFormattedValue(4U, MemSize::MBF32));
        Assert::AreEqual(std::wstring(L"1.47513e-39"), results1.GetFormattedValue(2U, MemSize::MBF32));
        Assert::AreEqual(std::wstring(L"1.73475e-18"), results1.GetFormattedValue(1U, MemSize::MBF32));
    }

    TEST_METHOD(TestMatchesFilterEightBitGreaterThanLast)
    {
        std::array<unsigned char, 5> memory{ 6, 12, 18, 24, 30 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1, results2;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);

        SearchResult result1, result2, result3;
        results1.GetMatchingAddress(0, result1);
        results1.GetMatchingAddress(1, result2);
        results1.GetMatchingAddress(2, result3);

        results2.Initialize(results1, ComparisonType::GreaterThanOrEqual, SearchFilterType::LastKnownValue, L"");

        Assert::IsTrue(results2.MatchesFilter(results1, result1));  // 12 >= 12 ?
        Assert::IsTrue(results2.MatchesFilter(results1, result2));  // 18 >= 18 ?
        Assert::IsTrue(results2.MatchesFilter(results1, result3));  // 24 >= 24 ?

        result1.nValue = 15;
        result2.nValue = 15;
        result3.nValue = 30;

        Assert::IsTrue(results2.MatchesFilter(results1, result1));  // 15 >= 12 ?
        Assert::IsFalse(results2.MatchesFilter(results1, result2)); // 15 >= 18 ?
        Assert::IsTrue(results2.MatchesFilter(results1, result3));  // 30 >= 24 ?
    }

    TEST_METHOD(TestMatchesFilterEightBitGreaterThanLastPlus)
    {
        std::array<unsigned char, 5> memory{ 6, 12, 18, 24, 30 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1, results2;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);

        SearchResult result1, result2, result3;
        results1.GetMatchingAddress(0, result1);
        results1.GetMatchingAddress(1, result2);
        results1.GetMatchingAddress(2, result3);

        results2.Initialize(results1, ComparisonType::GreaterThanOrEqual, SearchFilterType::LastKnownValuePlus, L"4");

        Assert::IsFalse(results2.MatchesFilter(results1, result1)); // 12 >= 12 + 4 ?
        Assert::IsFalse(results2.MatchesFilter(results1, result2)); // 18 >= 18 + 4 ?
        Assert::IsFalse(results2.MatchesFilter(results1, result3)); // 24 >= 24 + 4 ?

        result1.nValue = 15;
        result2.nValue = 22;
        result3.nValue = 30;

        Assert::IsFalse(results2.MatchesFilter(results1, result1)); // 15 >= 12 + 4 ?
        Assert::IsTrue(results2.MatchesFilter(results1, result2));  // 22 >= 18 + 4 ?
        Assert::IsTrue(results2.MatchesFilter(results1, result3));  // 30 >= 24 + 4 ?
    }

    TEST_METHOD(TestMatchesFilterEightBitGreaterThanLastMinus)
    {
        std::array<unsigned char, 5> memory{ 6, 12, 18, 24, 30 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1, results2;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);

        SearchResult result1, result2, result3;
        results1.GetMatchingAddress(0, result1);
        results1.GetMatchingAddress(1, result2);
        results1.GetMatchingAddress(2, result3);

        results2.Initialize(results1, ComparisonType::GreaterThanOrEqual, SearchFilterType::LastKnownValueMinus, L"4");

        Assert::IsTrue(results2.MatchesFilter(results1, result1));  // 12 >= 12 - 4 ?
        Assert::IsTrue(results2.MatchesFilter(results1, result2));  // 18 >= 18 - 4 ?
        Assert::IsTrue(results2.MatchesFilter(results1, result3));  // 24 >= 24 - 4 ?

        result1.nValue = 10;
        result2.nValue = 14;
        result3.nValue = 18;

        Assert::IsTrue(results2.MatchesFilter(results1, result1));  // 10 >= 12 - 4 ?
        Assert::IsTrue(results2.MatchesFilter(results1, result2));  // 14 >= 18 - 4 ?
        Assert::IsFalse(results2.MatchesFilter(results1, result3)); // 18 >= 24 - 4 ?
    }

    TEST_METHOD(TestMatchesFilterEightBitGreaterThanConstant)
    {
        std::array<unsigned char, 5> memory{ 6, 12, 18, 24, 30 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1, results2;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);

        SearchResult result1, result2, result3;
        results1.GetMatchingAddress(0, result1);
        results1.GetMatchingAddress(1, result2);
        results1.GetMatchingAddress(2, result3);

        results2.Initialize(results1, ComparisonType::GreaterThanOrEqual, SearchFilterType::Constant, L"18");

        Assert::IsFalse(results2.MatchesFilter(results1, result1)); // 12 >= 18 (12) ?
        Assert::IsTrue(results2.MatchesFilter(results1, result2));  // 18 >= 18 (18) ?
        Assert::IsTrue(results2.MatchesFilter(results1, result3));  // 24 >= 18 (24) ?
    }

    TEST_METHOD(TestInitializeFromMemoryAsciiEmpty)
    {
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        Assert::AreEqual({0U}, mockEmulatorContext.TotalMemorySize());

        SearchResults results;
        results.Initialize(0U, 16, ra::services::SearchType::AsciiText);

        Assert::AreEqual({0U}, results.MatchingAddressCount());

        Assert::AreEqual(std::wstring(), results.GetFormattedValue(0U, MemSize::Text));
    }

    TEST_METHOD(TestInitializeFromMemoryBitCount)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results;
        results.Initialize(1U, 3U, ra::services::SearchType::BitCount);

        Assert::AreEqual({ 3U }, results.MatchingAddressCount());

        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::BitCount, result.nSize);
        Assert::AreEqual(0x12U, result.nValue); // captured values are raw bytes for GetFormattedValue

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::BitCount, result.nSize);
        Assert::AreEqual(0x34U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(2U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(MemSize::BitCount, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsBitCountEqualsPrevious)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 5U, ra::services::SearchType::BitCount);
        Assert::AreEqual({ 5U }, results1.MatchingAddressCount());

        memory.at(1) = 0x14; // 0x12 (3) => 0x14 (3)
        memory.at(2) = 0x55; // 0x34 (3) => 0x55 (4)
        SearchResults results2;
        results2.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 4U }, results2.MatchingAddressCount());
        Assert::IsTrue(results2.ContainsAddress(0U));
        Assert::IsTrue(results2.ContainsAddress(1U));
        Assert::IsFalse(results2.ContainsAddress(2U));
        Assert::IsTrue(results2.ContainsAddress(3U));
        Assert::IsTrue(results2.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results2.GetMatchingAddress(2U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(MemSize::BitCount, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);

        // no change - tests the "entire block matches" optimization
        SearchResults results3;
        memory.at(3) = 0x99; // 0xAB (5) => 0x99 (4)
        results3.Initialize(results2, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValue, L"");

        Assert::AreEqual({ 3U }, results3.MatchingAddressCount());
        Assert::IsTrue(results3.ContainsAddress(0U));
        Assert::IsTrue(results3.ContainsAddress(1U));
        Assert::IsFalse(results3.ContainsAddress(2U));
        Assert::IsFalse(results3.ContainsAddress(3U));
        Assert::IsTrue(results3.ContainsAddress(4U));

        Assert::IsTrue(results3.GetMatchingAddress(2U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::BitCount, result.nSize);
        Assert::AreEqual(0x56U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsBitCountEqualsPreviousPlusOne)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 5U, ra::services::SearchType::BitCount);
        Assert::AreEqual({ 5U }, results1.MatchingAddressCount());

        memory.at(1) = 0x14; // 0x12 (3) => 0x14 (3)
        memory.at(2) = 0x55; // 0x34 (3) => 0x55 (4)
        memory.at(3) = 0xAC; // 0xAB (5) => 0xAC (4)
        SearchResults results2;
        results2.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValuePlus, L"1");

        Assert::AreEqual({ 1U }, results2.MatchingAddressCount());
        Assert::IsFalse(results2.ContainsAddress(0U));
        Assert::IsFalse(results2.ContainsAddress(1U));
        Assert::IsTrue(results2.ContainsAddress(2U));
        Assert::IsFalse(results2.ContainsAddress(3U));
        Assert::IsFalse(results2.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::BitCount, result.nSize);
        Assert::AreEqual(0x55U, result.nValue);
    }

    TEST_METHOD(TestGetFormattedValueBitCount)
    {
        std::array<unsigned char, 5> memory{ 0x00, 0x12, 0x34, 0xAB, 0x56 };
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(0U, 8U, ra::services::SearchType::BitCount);
        Assert::AreEqual({ 5U }, results1.MatchingAddressCount());

        Assert::AreEqual(std::wstring(L"0 (00000000)"), results1.GetFormattedValue(0U, MemSize::BitCount));
        Assert::AreEqual(std::wstring(L"2 (00010010)"), results1.GetFormattedValue(1U, MemSize::BitCount));
        Assert::AreEqual(std::wstring(L"3 (00110100)"), results1.GetFormattedValue(2U, MemSize::BitCount));
        Assert::AreEqual(std::wstring(L"5 (10101011)"), results1.GetFormattedValue(3U, MemSize::BitCount));
        Assert::AreEqual(std::wstring(L"4 (01010110)"), results1.GetFormattedValue(4U, MemSize::BitCount));

        memory.at(3) = 0x3B; // still 5 bits, but different value

        std::wstring sFormattedValue;
        SearchResult pResult;
        results1.GetMatchingAddress(3, pResult);
        Assert::IsTrue(results1.UpdateValue(pResult, &sFormattedValue, mockEmulatorContext));
        Assert::AreEqual(std::wstring(L"5 (00111011)"), sFormattedValue);
    }
    
    TEST_METHOD(TestInitializeFromListEightBit)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        std::vector<SearchResult> vResults;
        vResults.push_back({2, 0x07, MemSize::Unknown});
        vResults.push_back({3, 0x16, MemSize::Unknown});

        SearchResults results;
        results.Initialize(vResults, ra::services::SearchType::EightBit);

        Assert::AreEqual({2U}, results.MatchingAddressCount());

        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x07U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x16U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromListThirtyTwoBit)
    {
        std::array<unsigned char, 16> memory{};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        std::vector<SearchResult> vResults;
        vResults.push_back({2, 0x12345678, MemSize::Unknown});
        vResults.push_back({4, 0x55551234, MemSize::Unknown});
        vResults.push_back({12, 0xdeadbeef, MemSize::Unknown});

        SearchResults results;
        results.Initialize(vResults, ra::services::SearchType::ThirtyTwoBit);

        Assert::AreEqual({3U}, results.MatchingAddressCount());

        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsTrue(results.ContainsAddress(4U));
        Assert::IsFalse(results.ContainsAddress(5U));
        Assert::IsTrue(results.ContainsAddress(12U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0x12345678U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0x55551234U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(2U, result));
        Assert::AreEqual(12U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0xdeadbeefU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromListThirtyTwoBitAligned)
    {
        std::array<unsigned char, 16> memory{};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        std::vector<SearchResult> vResults;
        vResults.push_back({2, 0x12345678, MemSize::Unknown}); // not aligned - should be ignored
        vResults.push_back({4, 0x55551234, MemSize::Unknown});
        vResults.push_back({12, 0xdeadbeef, MemSize::Unknown});

        SearchResults results;
        results.Initialize(vResults, ra::services::SearchType::ThirtyTwoBitAligned);

        Assert::AreEqual({2U}, results.MatchingAddressCount());

        Assert::IsFalse(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsTrue(results.ContainsAddress(4U));
        Assert::IsFalse(results.ContainsAddress(5U));
        Assert::IsTrue(results.ContainsAddress(12U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0x55551234U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(12U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0xdeadbeefU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromListThirtyTwoBitBigEndian)
    {
        std::array<unsigned char, 16> memory{};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        std::vector<SearchResult> vResults;
        vResults.push_back({2, 0x12345678, MemSize::Unknown});
        vResults.push_back({4, 0x56785555, MemSize::Unknown});
        vResults.push_back({12, 0xdeadbeef, MemSize::Unknown});

        SearchResults results;
        results.Initialize(vResults, ra::services::SearchType::ThirtyTwoBitBigEndian);

        Assert::AreEqual({3U}, results.MatchingAddressCount());

        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsTrue(results.ContainsAddress(4U));
        Assert::IsFalse(results.ContainsAddress(5U));
        Assert::IsTrue(results.ContainsAddress(12U));

        SearchResult result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBitBigEndian, result.nSize);
        Assert::AreEqual(0x12345678U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBitBigEndian, result.nSize);
        Assert::AreEqual(0x56785555U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(2U, result));
        Assert::AreEqual(12U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBitBigEndian, result.nSize);
        Assert::AreEqual(0xdeadbeefU, result.nValue);
    }

};

} // namespace tests
} // namespace services
} // namespace ra
