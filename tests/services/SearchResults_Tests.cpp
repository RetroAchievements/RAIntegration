#include "services\SearchResults.h"
#include "tests\RA_UnitTestHelpers.h"

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
        Assert::AreEqual(0U, results.MatchingAddressCount());
        Assert::AreEqual(std::string(""), results.Summary());

        SearchResults::Result result;
        Assert::IsFalse(results.GetMatchingAddress(0, result));
    }

    TEST_METHOD(TestInitializeFromMemoryEightBit)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        SearchResults results;
        results.Initialize(1U, 3U, MemSize::EightBit);

        Assert::AreEqual(3U, results.MatchingAddressCount());
        Assert::AreEqual(std::string("Cleared: (8-bit) mode. Aware of 3 RAM locations."), results.Summary());

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
        InitializeMemory(memory);

        SearchResults results;
        results.Initialize(1U, 3U, MemSize::SixteenBit);

        Assert::AreEqual(2U, results.MatchingAddressCount());
        Assert::AreEqual(std::string("Cleared: (16-bit) mode. Aware of 2 RAM locations."), results.Summary());

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
        InitializeMemory(memory);

        SearchResults results;
        results.Initialize(1U, 4U, MemSize::ThirtyTwoBit);

        Assert::AreEqual(1U, results.MatchingAddressCount());
        Assert::AreEqual(std::string("Cleared: (32-bit) mode. Aware of 1 RAM locations."), results.Summary());

        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsFalse(results.ContainsAddress(2U));

        SearchResults::Result result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0x56AB3412U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryUpperNibble)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        SearchResults results;
        results.Initialize(1U, 3U, MemSize::Nibble_Upper);

        Assert::AreEqual(6U, results.MatchingAddressCount());
        Assert::AreEqual(std::string("Cleared: (Lower4) mode. Aware of 6 RAM locations."), results.Summary());

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

    TEST_METHOD(TestInitializeFromMemoryLowerNibble)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        SearchResults results;
        results.Initialize(1U, 3U, MemSize::Nibble_Lower); // should behave identical to Nibble_Upper

        Assert::AreEqual(6U, results.MatchingAddressCount());
        Assert::AreEqual(std::string("Cleared: (Lower4) mode. Aware of 6 RAM locations."), results.Summary());

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
        InitializeMemory(memory);

        SearchResults results;
        results.Initialize(1U, 8U, MemSize::SixteenBit);

        Assert::AreEqual(3U, results.MatchingAddressCount());
        Assert::AreEqual(std::string("Cleared: (16-bit) mode. Aware of 3 RAM locations."), results.Summary());

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
        InitializeMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, MemSize::EightBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        SearchResults results;
        results.Initialize(results1, ComparisonType::Equals, 0xABU);
        Assert::AreEqual(std::string("Filtering for EQUAL 171..."), results.Summary());

        Assert::AreEqual(1U, results.MatchingAddressCount());
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
        InitializeMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, MemSize::EightBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, 0xABU);
        Assert::AreEqual(std::string("Filtering for NOT EQUAL 171..."), results.Summary());

        Assert::AreEqual(2U, results.MatchingAddressCount());
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
        InitializeMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, MemSize::EightBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo, 0x34U);
        Assert::AreEqual(std::string("Filtering for NOT EQUAL 52..."), results.Summary());

        Assert::AreEqual(2U, results.MatchingAddressCount());
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
        InitializeMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, MemSize::EightBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        SearchResults results;
        results.Initialize(results1, ComparisonType::GreaterThan, 0x34U);
        Assert::AreEqual(std::string("Filtering for GREATER THAN 52..."), results.Summary());

        Assert::AreEqual(1U, results.MatchingAddressCount());
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
        InitializeMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, MemSize::EightBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        SearchResults results;
        results.Initialize(results1, ComparisonType::GreaterThanOrEqual, 0x34U);
        Assert::AreEqual(std::string("Filtering for GREATER THAN/EQUAL 52..."), results.Summary());

        Assert::AreEqual(2U, results.MatchingAddressCount());
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
        InitializeMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, MemSize::EightBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        SearchResults results;
        results.Initialize(results1, ComparisonType::LessThan, 0x34U);
        Assert::AreEqual(std::string("Filtering for LESS THAN 52..."), results.Summary());

        Assert::AreEqual(1U, results.MatchingAddressCount());
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
        InitializeMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, MemSize::EightBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        SearchResults results;
        results.Initialize(results1, ComparisonType::LessThanOrEqual, 0x34U);
        Assert::AreEqual(std::string("Filtering for LESS THAN/EQUAL 52..."), results.Summary());

        Assert::AreEqual(2U, results.MatchingAddressCount());
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
        InitializeMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, MemSize::EightBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        // swap bytes 1 and 3, match should be found at address 1, not address 3
        memory.at(1) = 0xAB;
        memory.at(3) = 0x12;
        SearchResults results;
        results.Initialize(results1, ComparisonType::Equals, 0xABU);
        Assert::AreEqual(std::string("Filtering for EQUAL 171..."), results.Summary());

        Assert::AreEqual(1U, results.MatchingAddressCount());
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
        InitializeMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, MemSize::EightBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(2) = 0x55;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo);
        Assert::AreEqual(std::string("Filtering for NOT EQUAL last known value..."), results.Summary());

        Assert::AreEqual(2U, results.MatchingAddressCount());
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

    TEST_METHOD(TestInitializeFromResultsSixteenBitNotEqualPrevious)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 4U, MemSize::SixteenBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        memory.at(2) = 0x55;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo);
        Assert::AreEqual(std::string("Filtering for NOT EQUAL last known value..."), results.Summary());

        Assert::AreEqual(2U, results.MatchingAddressCount());
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

    TEST_METHOD(TestInitializeFromResultsFourBitNotEqualsPrevious)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, MemSize::Nibble_Lower);
        Assert::AreEqual(6U, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(2) = 0x55;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo);
        Assert::AreEqual(std::string("Filtering for NOT EQUAL last known value..."), results.Summary());

        Assert::AreEqual(3U, results.MatchingAddressCount());
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
        InitializeMemory(memory);

        SearchResults results;
        results.Initialize(1U, 3U, MemSize::Nibble_Lower);
        Assert::AreEqual(6U, results.MatchingAddressCount());

        SearchResults filtered1;
        filtered1.Initialize(results, ComparisonType::Equals, 1);
        Assert::AreEqual(1U, filtered1.MatchingAddressCount());

        SearchResults::Result result;
        Assert::IsTrue(filtered1.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::Nibble_Upper, result.nSize);
        Assert::AreEqual(1U, result.nValue);

        // Nibble_Upper no longer matches, but Nibble_Lower does. Neither should not be returned.
        memory.at(1) = 0x21;
        SearchResults filtered2;
        filtered2.Initialize(filtered1, ComparisonType::Equals, 1);
        Assert::AreEqual(0U, filtered2.MatchingAddressCount());

        // Both nibbles match, only previously matched one should be returned
        memory.at(1) = 0x11;
        SearchResults filtered3;
        filtered3.Initialize(filtered1, ComparisonType::Equals, 1);
        Assert::AreEqual(1U, filtered3.MatchingAddressCount());

        Assert::IsTrue(filtered3.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(MemSize::Nibble_Upper, result.nSize);
        Assert::AreEqual(1U, result.nValue);
    }

    TEST_METHOD(TestExcludeAddressEightBit)
    {
        std::array<unsigned char, 5> memory{0x00, 0x12, 0x34, 0xAB, 0x56};
        InitializeMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, MemSize::EightBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        // exclude doesn't do anything to unfiltered results
        results1.ExcludeAddress(1U);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(2) = 0x55;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo);
        Assert::AreEqual(std::string("Filtering for NOT EQUAL last known value..."), results.Summary());

        Assert::AreEqual(2U, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        results.ExcludeAddress(1U);
        Assert::AreEqual(1U, results.MatchingAddressCount());
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
        InitializeMemory(memory);

        SearchResults results1;
        results1.Initialize(1U, 3U, MemSize::EightBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        // exclude doesn't do anything to unfiltered results
        results1.ExcludeMatchingAddress(0U);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        memory.at(1) = 0x14;
        memory.at(2) = 0x55;
        SearchResults results;
        results.Initialize(results1, ComparisonType::NotEqualTo);
        Assert::AreEqual(std::string("Filtering for NOT EQUAL last known value..."), results.Summary());

        Assert::AreEqual(2U, results.MatchingAddressCount());
        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));
        Assert::IsFalse(results.ContainsAddress(4U));

        results.ExcludeMatchingAddress(0U);
        Assert::AreEqual(1U, results.MatchingAddressCount());
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
            memory[i] = (i % 256);
        InitializeMemory(std::move(memory), BIG_BLOCK_SIZE);

        SearchResults results;
        results.Initialize(0U, BIG_BLOCK_SIZE, MemSize::EightBit);

        Assert::AreEqual(BIG_BLOCK_SIZE, results.MatchingAddressCount());
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
    }

    TEST_METHOD(TestInitializeFromMemorySixteenBitLargeMemory)
    {
        auto memory = std::make_unique<unsigned char[]>(BIG_BLOCK_SIZE);
        for (unsigned int i = 0; i < BIG_BLOCK_SIZE; ++i)
            memory[i] = (i % 256);
        InitializeMemory(std::move(memory), BIG_BLOCK_SIZE);

        SearchResults results;
        results.Initialize(0U, BIG_BLOCK_SIZE, MemSize::SixteenBit);

        Assert::AreEqual(BIG_BLOCK_SIZE - 1, results.MatchingAddressCount());
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
    }
};

} // namespace tests
} // namespace services
} // namespace ra
