#pragma once

#include "CppUnitTest.h"

#include "RA_Condition.h"
#include "RA_Defs.h"
#include "RA_MemValue.h"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

template<> static std::wstring ToString<ComparisonVariableSize>(const ComparisonVariableSize& t)
{
    return ra::Widen(COMPARISONVARIABLESIZE_STR[(int)t]);
}

template<> static std::wstring ToString<ComparisonVariableType>(const ComparisonVariableType& t)
{
    return ra::Widen(COMPARISONVARIABLETYPE_STR[(int)t]);
}

template<> static std::wstring ToString<ComparisonType>(const ComparisonType& t)
{
    return ra::Widen(COMPARISONTYPE_STR[(int)t]);
}

template<> static std::wstring ToString<Condition::ConditionType>(const Condition::ConditionType& t)
{
    return ra::Widen(CONDITIONTYPE_STR[(int)t]);
}

template<> static std::wstring ToString<MemValue::Format>(const MemValue::Format& format)
{
    return ra::Widen(MemValue::GetFormatString(format));
}

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace CppUnitTestFramework


// Loads memory into the MemoryManager
void InitializeMemory(unsigned char* pMemory, size_t szMemorySize);

void AssertContains(const std::string& sHaystack, const std::string& sNeedle);