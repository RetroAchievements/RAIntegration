#pragma once

#include "CppUnitTest.h"

#include "RA_Condition.h"
#include "RA_Defs.h"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

template<> static std::wstring ToString<ComparisonVariableSize>(const ComparisonVariableSize& t)
{
    return Widen(COMPARISONVARIABLESIZE_STR[(int)t]);
}

template<> static std::wstring ToString<ComparisonVariableType>(const ComparisonVariableType& t)
{
    return Widen(COMPARISONVARIABLETYPE_STR[(int)t]);
}

template<> static std::wstring ToString<ComparisonType>(const ComparisonType& t)
{
    return Widen(COMPARISONTYPE_STR[(int)t]);
}

template<> static std::wstring ToString<Condition::ConditionType>(const Condition::ConditionType& t)
{
    return Widen(CONDITIONTYPE_STR[(int)t]);
}

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace CppUnitTestFramework


// Loads memory into the MemoryManager
void InitializeMemory(unsigned char* pMemory, size_t szMemorySize);
