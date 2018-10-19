#include "CppUnitTest.h"

#include "services\SearchResults.h"
#include "tests\RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace services {
namespace tests {

const unsigned int MAX_BLOCK_SIZE = 256 * 1024; // assert: matches value in SearchResults.cpp
const unsigned int BIG_BLOCK_SIZE = MAX_BLOCK_SIZE + MAX_BLOCK_SIZE + (MAX_BLOCK_SIZE / 2);

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
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results;
        results.Initialize(1U, 3U, ComparisonVariableSize::EightBit);

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
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0x12U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0x34U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(2U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemorySixteenBit)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results;
        results.Initialize(1U, 3U, ComparisonVariableSize::SixteenBit);

        Assert::AreEqual(2U, results.MatchingAddressCount());
        Assert::AreEqual(std::string("Cleared: (16-bit) mode. Aware of 2 RAM locations."), results.Summary());

        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsTrue(results.ContainsAddress(2U));
        Assert::IsFalse(results.ContainsAddress(3U));

        SearchResults::Result result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::SixteenBit, result.nSize);
        Assert::AreEqual(0x3412U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::SixteenBit, result.nSize);
        Assert::AreEqual(0xAB34U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryThirtyTwoBit)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56, 0xCD };
        InitializeMemory(memory, 6);

        SearchResults results;
        results.Initialize(1U, 4U, ComparisonVariableSize::ThirtyTwoBit);

        Assert::AreEqual(1U, results.MatchingAddressCount());
        Assert::AreEqual(std::string("Cleared: (32-bit) mode. Aware of 1 RAM locations."), results.Summary());

        Assert::IsFalse(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(1U));
        Assert::IsFalse(results.ContainsAddress(2U));

        SearchResults::Result result;
        Assert::IsTrue(results.GetMatchingAddress(0U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::ThirtyTwoBit, result.nSize);
        Assert::AreEqual(0x56AB3412U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryUpperNibble)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results;
        results.Initialize(1U, 3U, ComparisonVariableSize::Nibble_Upper);

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
        Assert::AreEqual(ComparisonVariableSize::Nibble_Lower, result.nSize);
        Assert::AreEqual(0x2U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::Nibble_Upper, result.nSize);
        Assert::AreEqual(0x1U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(2U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::Nibble_Lower, result.nSize);
        Assert::AreEqual(0x4U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(3U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::Nibble_Upper, result.nSize);
        Assert::AreEqual(0x3U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(4U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::Nibble_Lower, result.nSize);
        Assert::AreEqual(0xBU, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(5U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::Nibble_Upper, result.nSize);
        Assert::AreEqual(0xAU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryLowerNibble)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results;
        results.Initialize(1U, 3U, ComparisonVariableSize::Nibble_Lower); // should behave identical to Nibble_Upper

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
        Assert::AreEqual(ComparisonVariableSize::Nibble_Lower, result.nSize);
        Assert::AreEqual(0x2U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(1U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::Nibble_Upper, result.nSize);
        Assert::AreEqual(0x1U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(2U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::Nibble_Lower, result.nSize);
        Assert::AreEqual(0x4U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(3U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::Nibble_Upper, result.nSize);
        Assert::AreEqual(0x3U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(4U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::Nibble_Lower, result.nSize);
        Assert::AreEqual(0xBU, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(5U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::Nibble_Upper, result.nSize);
        Assert::AreEqual(0xAU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemorySixteenBitBounded)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results;
        results.Initialize(1U, 8U, ComparisonVariableSize::SixteenBit);

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
        Assert::AreEqual(ComparisonVariableSize::SixteenBit, result.nSize);
        Assert::AreEqual(0x3412U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::SixteenBit, result.nSize);
        Assert::AreEqual(0xAB34U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(2U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::SixteenBit, result.nSize);
        Assert::AreEqual(0x56ABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitEqualsConstant)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results1;
        results1.Initialize(1U, 3U, ComparisonVariableSize::EightBit);
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
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitNotEqualsConstant)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results1;
        results1.Initialize(1U, 3U, ComparisonVariableSize::EightBit);
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
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0x12U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0x34U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitNotEqualsConstantGap)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results1;
        results1.Initialize(1U, 3U, ComparisonVariableSize::EightBit);
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
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0x12U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitGreaterThanConstant)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results1;
        results1.Initialize(1U, 3U, ComparisonVariableSize::EightBit);
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
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitGreaterThanOrEqualConstant)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results1;
        results1.Initialize(1U, 3U, ComparisonVariableSize::EightBit);
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
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0x34U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(3U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitLessThanConstant)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results1;
        results1.Initialize(1U, 3U, ComparisonVariableSize::EightBit);
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
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0x12U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitLessThanOrEqualConstant)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results1;
        results1.Initialize(1U, 3U, ComparisonVariableSize::EightBit);
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
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0x12U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0x34U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitEqualsConstantModified)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results1;
        results1.Initialize(1U, 3U, ComparisonVariableSize::EightBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        // swap bytes 1 and 3, match should be found at address 1, not address 3
        memory[1] = 0xAB;
        memory[3] = 0x12;
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
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0xABU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsEightBitNotEqualsPrevious)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results1;
        results1.Initialize(1U, 3U, ComparisonVariableSize::EightBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        memory[1] = 0x14;
        memory[2] = 0x55;
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
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0x14U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0x55U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsSixteenBitNotEqualPrevious)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results1;
        results1.Initialize(1U, 4U, ComparisonVariableSize::SixteenBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        memory[2] = 0x55;
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
        Assert::AreEqual(ComparisonVariableSize::SixteenBit, result.nSize);
        Assert::AreEqual(0x5512U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::SixteenBit, result.nSize);
        Assert::AreEqual(0xAB55U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromResultsFourBitNotEqualsPrevious)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results1;
        results1.Initialize(1U, 3U, ComparisonVariableSize::Nibble_Lower);
        Assert::AreEqual(6U, results1.MatchingAddressCount());

        memory[1] = 0x14;
        memory[2] = 0x55;
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
        Assert::AreEqual(ComparisonVariableSize::Nibble_Lower, result.nSize);
        Assert::AreEqual(4U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(1U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::Nibble_Lower, result.nSize);
        Assert::AreEqual(5U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(2U, result));
        Assert::AreEqual(2U, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::Nibble_Upper, result.nSize);
        Assert::AreEqual(5U, result.nValue);
    }

    TEST_METHOD(TestExcludeAddressEightBit)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results1;
        results1.Initialize(1U, 3U, ComparisonVariableSize::EightBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        // exclude doesn't do anything to unfiltered results
        results1.ExcludeAddress(1U);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        memory[1] = 0x14;
        memory[2] = 0x55;
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
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0x55U, result.nValue);
    }

    TEST_METHOD(TestExcludeMatchingAddressEightBit)
    {
        unsigned char memory[] = { 0x00, 0x12, 0x34, 0xAB, 0x56 };
        InitializeMemory(memory, 5);

        SearchResults results1;
        results1.Initialize(1U, 3U, ComparisonVariableSize::EightBit);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        // exclude doesn't do anything to unfiltered results
        results1.ExcludeMatchingAddress(0U);
        Assert::AreEqual(3U, results1.MatchingAddressCount());

        memory[1] = 0x14;
        memory[2] = 0x55;
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
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0x55U, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemoryEightBitLargeMemory)
    {
        auto memory = std::make_unique<unsigned char[]>(BIG_BLOCK_SIZE);
        for (unsigned int i = 0; i < BIG_BLOCK_SIZE; ++i)
            memory.get()[i] = (i % 256);
        InitializeMemory(memory.get(), BIG_BLOCK_SIZE);

        SearchResults results;
        results.Initialize(0U, BIG_BLOCK_SIZE, ComparisonVariableSize::EightBit);

        Assert::AreEqual(BIG_BLOCK_SIZE, results.MatchingAddressCount());
        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(MAX_BLOCK_SIZE - 1));
        Assert::IsTrue(results.ContainsAddress(MAX_BLOCK_SIZE));
        Assert::IsTrue(results.ContainsAddress(BIG_BLOCK_SIZE - 1));
        Assert::IsFalse(results.ContainsAddress(BIG_BLOCK_SIZE));

        SearchResults::Result result;
        Assert::IsTrue(results.GetMatchingAddress(MAX_BLOCK_SIZE - 1, result));
        Assert::AreEqual(MAX_BLOCK_SIZE - 1, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0xFFU, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(MAX_BLOCK_SIZE, result));
        Assert::AreEqual(MAX_BLOCK_SIZE, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0x00U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(BIG_BLOCK_SIZE - 1, result));
        Assert::AreEqual(BIG_BLOCK_SIZE - 1, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::EightBit, result.nSize);
        Assert::AreEqual(0xFFU, result.nValue);
    }

    TEST_METHOD(TestInitializeFromMemorySixteenBitLargeMemory)
    {
        auto memory = std::make_unique<unsigned char[]>(BIG_BLOCK_SIZE);
        for (unsigned int i = 0; i < BIG_BLOCK_SIZE; ++i)
            memory.get()[i] = (i % 256);
        InitializeMemory(memory.get(), BIG_BLOCK_SIZE);

        SearchResults results;
        results.Initialize(0U, BIG_BLOCK_SIZE, ComparisonVariableSize::SixteenBit);

        Assert::AreEqual(BIG_BLOCK_SIZE - 1, results.MatchingAddressCount());
        Assert::IsTrue(results.ContainsAddress(0U));
        Assert::IsTrue(results.ContainsAddress(MAX_BLOCK_SIZE - 1));
        Assert::IsTrue(results.ContainsAddress(MAX_BLOCK_SIZE));
        Assert::IsTrue(results.ContainsAddress(BIG_BLOCK_SIZE - 2));
        Assert::IsFalse(results.ContainsAddress(BIG_BLOCK_SIZE - 1));

        SearchResults::Result result;
        Assert::IsTrue(results.GetMatchingAddress(MAX_BLOCK_SIZE - 1, result));
        Assert::AreEqual(MAX_BLOCK_SIZE - 1, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::SixteenBit, result.nSize);
        Assert::AreEqual(0x00FFU, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(MAX_BLOCK_SIZE, result));
        Assert::AreEqual(MAX_BLOCK_SIZE, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::SixteenBit, result.nSize);
        Assert::AreEqual(0x0100U, result.nValue);

        Assert::IsTrue(results.GetMatchingAddress(BIG_BLOCK_SIZE - 2, result));
        Assert::AreEqual(BIG_BLOCK_SIZE - 2, result.nAddress);
        Assert::AreEqual(ComparisonVariableSize::SixteenBit, result.nSize);
        Assert::AreEqual(0xFFFEU, result.nValue);
    }
};

} // namespace tests
} // namespace services
} // namespace ra
