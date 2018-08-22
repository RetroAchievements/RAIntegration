#pragma once

#include "CppUnitTest.h"

#include "RA_Condition.h"
#include "RA_Defs.h"
#include "RA_MemValue.h"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

template<> static std::wstring ToString<ra::ComparisonVariableSize>(const ra::ComparisonVariableSize& t)
{
    return ra::Widen(ra::COMPARISONVARIABLESIZE_STR.at(ra::etoi(t)));
}

template<> static std::wstring ToString<ra::ComparisonVariableType>(const ra::ComparisonVariableType& t)
{
    return ra::Widen(ra::COMPARISONVARIABLETYPE_STR.at(ra::etoi(t)));
}

template<> static std::wstring ToString<ra::ComparisonType>(const ra::ComparisonType& t)
{
    return ra::Widen(ra::COMPARISONTYPE_STR.at(ra::etoi(t)));
}

template<> static std::wstring ToString<ra::ConditionType>(const ra::ConditionType& t)
{
    return ra::Widen(ra::CONDITIONTYPE_STR.at(ra::etoi(t)));
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
