#ifndef RA_REQUIREMENT_ASSERTS_H
#define RA_REQUIREMENT_ASSERTS_H
#pragma once

#include "CppUnitTest.hh"

#include "data/Requirement.hh"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.

template<>
std::wstring ToString<ra::data::Requirement::Type>(const ra::data::Requirement::Type& nConditionType)
{
    switch (nConditionType)
    {
        case ra::data::Requirement::Type::Standard:
            return L"Standard";
        case ra::data::Requirement::Type::PauseIf:
            return L"PauseIf";
        case ra::data::Requirement::Type::ResetIf:
            return L"ResetIf";
        case ra::data::Requirement::Type::AddSource:
            return L"AddSource";
        case ra::data::Requirement::Type::SubSource:
            return L"SubSource";
        case ra::data::Requirement::Type::AddHits:
            return L"AddHits";
        case ra::data::Requirement::Type::SubHits:
            return L"SubHits";
        case ra::data::Requirement::Type::Remember:
            return L"Remember";
        case ra::data::Requirement::Type::AndNext:
            return L"AndNext";
        case ra::data::Requirement::Type::Measured:
            return L"Measured";
        case ra::data::Requirement::Type::AddAddress:
            return L"AddAddress";
        case ra::data::Requirement::Type::OrNext:
            return L"OrNext";
        case ra::data::Requirement::Type::Trigger:
            return L"Trigger";
        case ra::data::Requirement::Type::MeasuredIf:
            return L"MeasuredIf";
        case ra::data::Requirement::Type::ResetNextIf:
            return L"ResetNextIf";
        default:
            return std::to_wstring(static_cast<int>(nConditionType));
    }
}

template<>
std::wstring ToString<ra::data::Requirement::OperandType>(const ra::data::Requirement::OperandType& nOperandType)
{
    switch (nOperandType)
    {
        case ra::data::Requirement::OperandType::Address:
            return L"Address";
        case ra::data::Requirement::OperandType::Delta:
            return L"Delta";
        case ra::data::Requirement::OperandType::Value:
            return L"Value";
        case ra::data::Requirement::OperandType::Prior:
            return L"Prior";
        case ra::data::Requirement::OperandType::BCD:
            return L"BCD";
        case ra::data::Requirement::OperandType::Float:
            return L"Float";
        case ra::data::Requirement::OperandType::Inverted:
            return L"Inverted";
        case ra::data::Requirement::OperandType::Recall:
            return L"Recall";
        default:
            return std::to_wstring(static_cast<int>(nOperandType));
    }
}

template<>
std::wstring ToString<ra::data::Requirement::OperatorType>(const ra::data::Requirement::OperatorType& nConditionType)
{
    switch (nConditionType)
    {
        case ra::data::Requirement::OperatorType::Equals:
            return L"Equals";
        case ra::data::Requirement::OperatorType::LessThan:
            return L"LessThan";
        case ra::data::Requirement::OperatorType::LessThanOrEqual:
            return L"LessThanOrEqual";
        case ra::data::Requirement::OperatorType::GreaterThan:
            return L"GreaterThan";
        case ra::data::Requirement::OperatorType::GreaterThanOrEqual:
            return L"GreaterThanOrEqual";
        case ra::data::Requirement::OperatorType::NotEquals:
            return L"NotEquals";
        case ra::data::Requirement::OperatorType::None:
            return L"None";
        case ra::data::Requirement::OperatorType::Multiply:
            return L"Multiply";
        case ra::data::Requirement::OperatorType::Divide:
            return L"Divide";
        case ra::data::Requirement::OperatorType::BitwiseAnd:
            return L"BitwiseAnd";
        case ra::data::Requirement::OperatorType::Modulus:
            return L"Modulus";
        case ra::data::Requirement::OperatorType::Add:
            return L"Add";
        case ra::data::Requirement::OperatorType::Subtract:
            return L"Subtract";
        default:
            return std::to_wstring(static_cast<int>(nConditionType));
    }
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

#endif /* !RA_REQUIREMENT_ASSERTS_H */
