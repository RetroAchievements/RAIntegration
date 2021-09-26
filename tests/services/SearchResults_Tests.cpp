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

        SearchResults::Result result;
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

        SearchResults::Result result;
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

        SearchResults::Result result;
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

        SearchResults::Result result;
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

        SearchResults::Result result;
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

        SearchResults::Result result;
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

        SearchResults::Result result;
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

        SearchResults::Result result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBitBigEndian, result.nSize);
        Assert::AreEqual(0x1234AB56U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryLowerNibble)
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
        SearchResults::Result result;
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

        SearchResults::Result result;
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

        SearchResults results;
        results.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, 0xABU);

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsFalse(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResults::Result result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);
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
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::Constant, 0xABU);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResults::Result result;
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
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::Constant, 0x34U);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsFalse(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResults::Result result;
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
        results.Initialize(results1, ComparisonType::GreaterThan, ra::services::SearchFilterType::Constant, 0x34U);

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsFalse(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResults::Result result;
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
        results.Initialize(results1, ComparisonType::GreaterThanOrEqual, ra::services::SearchFilterType::Constant, 0x34U);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsTrue(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResults::Result result;
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
        results.Initialize(results1, ComparisonType::LessThan, ra::services::SearchFilterType::Constant, 0x34U);

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsFalse(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResults::Result result;
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
        results.Initialize(results1, ComparisonType::LessThanOrEqual, ra::services::SearchFilterType::Constant, 0x34U);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResults::Result result;
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
        results.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, 0xABU);

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsFalse(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResults::Result result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);
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
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, 0U);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResults::Result result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x14U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x55U, result.nValue);
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
        results.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValuePlus, 2U);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResults::Result result;
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
        results.Initialize(results1, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValueMinus, 2U);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResults::Result result;
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
        results.Initialize(results1, ComparisonType::GreaterThan, ra::services::SearchFilterType::LastKnownValuePlus, 2U);

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResults::Result result;
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
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, 0U);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));

        SearchResults::Result result;
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
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, 0U);

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));

        SearchResults::Result result;
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
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, 0U);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(2U));

        SearchResults::Result result;
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
        results2.Initialize(results, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, 0U);

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
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, 0U);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(4U));

        SearchResults::Result result;
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
        results2.Initialize(results, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, 0U);

        Assert::AreEqual({ 1U }, results2.MatchingAddressCount());
        Assert::IsFalse(results2.ContainsAddress(0U));
        Assert::IsTrue(results2.ContainsAddress(4U));

        Assert::IsTrue(results2.GetMatchingAddress(0U, result));
        Assert::AreEqual(4U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0x2099CD66U, result.nValue);
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
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, 0U);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));

        SearchResults::Result result;
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
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, 0U);

        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));

        SearchResults::Result result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBitBigEndian, result.nSize);
        Assert::AreEqual(0x1234AB55U, result.nValue);
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
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, 0U);

        Assert::AreEqual({ 3U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResults::Result result;
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
        filtered1.Initialize(results, ComparisonType::Equals, ra::services::SearchFilterType::Constant, 1U);
        Assert::AreEqual({ 1U }, filtered1.MatchingAddressCount());

        SearchResults::Result result;
        Assert::IsTrue(filtered1.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::Nibble_Upper, result.nSize);
        Assert::AreEqual(1U, result.nValue);

        // Nibble_Upper no longer matches, but Nibble_Lower does. Neither should not be returned.
        memory.at(1) = 0x21;
        SearchResults filtered2;
        filtered2.Initialize(filtered1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, 1U);
        Assert::AreEqual({ 0U }, filtered2.MatchingAddressCount());

        // Both nibbles match, only previously matched one should be returned
        memory.at(1) = 0x11;
        SearchResults filtered3;
        filtered3.Initialize(filtered1, ComparisonType::Equals, ra::services::SearchFilterType::Constant, 1U);
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
        results2.Initialize(results1, ComparisonType::GreaterThan, ra::services::SearchFilterType::Constant, 0x10U);
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
        results3.Initialize(results1, results2, ComparisonType::Equals, ra::services::SearchFilterType::LastKnownValue, 0U);
        Assert::AreEqual({ 1U }, results3.MatchingAddressCount());
        Assert::IsFalse(results3.ContainsAddress(0U));
        Assert::IsFalse(results3.ContainsAddress(1U));
        Assert::IsFalse(results3.ContainsAddress(2U));
        Assert::IsTrue(results3.ContainsAddress(3U));
        Assert::IsFalse(results3.ContainsAddress(4U));

        SearchResults::Result result;
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
        results1.ExcludeAddress(1U);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(2) = 0x55;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, 0U);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        results.ExcludeAddress(1U);
        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResults::Result result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x55U, result.nValue);
    }

    TEST_METHOD(TestExcludeMatchingAddressEightBit)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        // exclude doesn't do anything to unfiltered results
        results1.ExcludeMatchingAddress(0U);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(2) = 0x55;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, 0U);

        Assert::AreEqual({ 2U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        results.ExcludeMatchingAddress(0U);
        Assert::AreEqual({ 1U }, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsFalse(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        SearchResults::Result result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x55U, result.nValue);
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

        SearchResults::Result result;
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

        SearchResults::Result result;
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

        SearchResults::Result result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(0U, result.nAddress);
        Assert::AreEqual(MemSize::Text, result.nSize);
        Assert::AreEqual({ 'S' }, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(35U, result));
        Assert::AreEqual(35U, result.nAddress);
        Assert::AreEqual(MemSize::Text, result.nSize);
        Assert::AreEqual({ '.' }, result.nValue);
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

        SearchResults::Result result;
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

        SearchResults::Result result;
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

        SearchResults::Result result;
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

    TEST_METHOD(TestCopyConstructor)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        ra::data::context::mocks::MockEmulatorContext mockEmulatorContext;
        mockEmulatorContext.MockMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, ra::services::SearchType::EightBit);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        // exclude doesn't do anything to unfiltered results
        results1.ExcludeMatchingAddress(0U);
        Assert::AreEqual({ 3U }, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(2) = 0x55;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, ra::services::SearchFilterType::LastKnownValue, 0U);

        SearchResults results2(results);

        Assert::AreEqual({ 2U }, results2.MatchingAddressCount());
        Assert::IsFalse(results2.ContainsAddress(0U));
        Assert::IsTrue(results2.ContainsAddress(1U));
        Assert::IsTrue(results2.ContainsAddress(2U));
        Assert::IsFalse(results2.ContainsAddress(3U));
        Assert::IsFalse(results2.ContainsAddress(4U));

        SearchResults::Result result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x14U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(MemSize::EightBit, result.nSize);
        Assert::AreEqual(0x55U, result.nValue);
    }
};

} // namespace tests
} // namespace services
} // namespace ra
