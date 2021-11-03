#ifndef RA_UNITTEST_TRIGGERCONDITIONASSERTS_H
#define RA_UNITTEST_TRIGGERCONDITIONASSERTS_H
#pragma once

#include "ui\viewmodels\TriggerConditionViewModel.hh"

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

// converters for asserting enum values

#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced inline functions, they are referenced. Must be a bug.

template<>
std::wstring ToString<ra::ui::viewmodels::TriggerConditionType>(
    const ra::ui::viewmodels::TriggerConditionType& nConditionType)
{
    switch (nConditionType)
    {
        case ra::ui::viewmodels::TriggerConditionType::Standard:
            return L"Standard";
        case ra::ui::viewmodels::TriggerConditionType::PauseIf:
            return L"PauseIf";
        case ra::ui::viewmodels::TriggerConditionType::ResetIf:
            return L"ResetIf";
        case ra::ui::viewmodels::TriggerConditionType::AddSource:
            return L"AddSource";
        case ra::ui::viewmodels::TriggerConditionType::SubSource:
            return L"SubSource";
        case ra::ui::viewmodels::TriggerConditionType::AddHits:
            return L"AddHits";
        case ra::ui::viewmodels::TriggerConditionType::SubHits:
            return L"SubHits";
        case ra::ui::viewmodels::TriggerConditionType::AndNext:
            return L"AndNext";
        case ra::ui::viewmodels::TriggerConditionType::Measured:
            return L"Measured";
        case ra::ui::viewmodels::TriggerConditionType::AddAddress:
            return L"AddAddress";
        case ra::ui::viewmodels::TriggerConditionType::OrNext:
            return L"OrNext";
        case ra::ui::viewmodels::TriggerConditionType::Trigger:
            return L"Trigger";
        case ra::ui::viewmodels::TriggerConditionType::MeasuredIf:
            return L"MeasuredIf";
        case ra::ui::viewmodels::TriggerConditionType::ResetNextIf:
            return L"ResetNextIf";
        default:
            return std::to_wstring(static_cast<int>(nConditionType));
    }
}

template<>
std::wstring ToString<ra::ui::viewmodels::TriggerOperandType>(
    const ra::ui::viewmodels::TriggerOperandType& nOperandType)
{
    switch (nOperandType)
    {
        case ra::ui::viewmodels::TriggerOperandType::Address:
            return L"Address";
        case ra::ui::viewmodels::TriggerOperandType::Delta:
            return L"Delta";
        case ra::ui::viewmodels::TriggerOperandType::Value:
            return L"Value";
        case ra::ui::viewmodels::TriggerOperandType::Prior:
            return L"Prior";
        case ra::ui::viewmodels::TriggerOperandType::BCD:
            return L"BCD";
        case ra::ui::viewmodels::TriggerOperandType::Float:
            return L"Float";
        default:
            return std::to_wstring(static_cast<int>(nOperandType));
    }
}

template<>
std::wstring ToString<ra::ui::viewmodels::TriggerOperatorType>(
    const ra::ui::viewmodels::TriggerOperatorType& nConditionType)
{
    switch (nConditionType)
    {
        case ra::ui::viewmodels::TriggerOperatorType::Equals:
            return L"Equals";
        case ra::ui::viewmodels::TriggerOperatorType::LessThan:
            return L"LessThan";
        case ra::ui::viewmodels::TriggerOperatorType::LessThanOrEqual:
            return L"LessThanOrEqual";
        case ra::ui::viewmodels::TriggerOperatorType::GreaterThan:
            return L"GreaterThan";
        case ra::ui::viewmodels::TriggerOperatorType::GreaterThanOrEqual:
            return L"GreaterThanOrEqual";
        case ra::ui::viewmodels::TriggerOperatorType::NotEquals:
            return L"NotEquals";
        case ra::ui::viewmodels::TriggerOperatorType::None:
            return L"None";
        case ra::ui::viewmodels::TriggerOperatorType::Multiply:
            return L"Multiply";
        case ra::ui::viewmodels::TriggerOperatorType::Divide:
            return L"Divide";
        case ra::ui::viewmodels::TriggerOperatorType::BitwiseAnd:
            return L"BitwiseAnd";
        default:
            return std::to_wstring(static_cast<int>(nConditionType));
    }
}

#pragma warning(pop)

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

#endif /* !RA_UNITTEST_TRIGGERCONDITIONASSERTS_H */
