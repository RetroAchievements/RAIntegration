#include "Requirement.hh"

#include "context/IEmulatorMemoryContext.hh"

#include "services/ServiceLocator.hh"
#include "services/IConfiguration.hh"

#include <rcheevos/src/rcheevos/rc_internal.h>

namespace ra {
namespace data {

std::wstring Requirement::FormatOperandValue(unsigned nValue, Requirement::OperandType nType)
{
    rc_typed_value_t pValue{};
    pValue.type = RC_VALUE_TYPE_UNSIGNED;
    pValue.value.u32 = nValue;
    return FormatOperandValue(pValue, nType);
}

std::wstring Requirement::FormatOperandValue(float fValue, Requirement::OperandType nType)
{
    rc_typed_value_t pValue{};
    pValue.type = RC_VALUE_TYPE_FLOAT;
    pValue.value.f32 = fValue;
    return FormatOperandValue(pValue, nType);
}

std::wstring Requirement::FormatOperandValue(rc_typed_value_t& pValue, Requirement::OperandType nType)
{
    switch (nType)
    {
        case ra::data::Requirement::OperandType::Value:
        {
            rc_typed_value_convert(&pValue, RC_VALUE_TYPE_UNSIGNED);
            const auto& pConfiguration = ra::services::ServiceLocator::Get<ra::services::IConfiguration>();
            if (pConfiguration.IsFeatureEnabled(ra::services::Feature::PreferDecimal))
                return std::to_wstring(pValue.value.u32);

            return ra::util::String::Printf(L"0x%02x", pValue.value.u32);
        }

        case ra::data::Requirement::OperandType::Float:
        {
            rc_typed_value_convert(&pValue, RC_VALUE_TYPE_FLOAT);
            auto sFloat = std::to_wstring(pValue.value.f32);
            if (sFloat.find('.') != std::string::npos)
            {
                while (sFloat.back() == '0')
                    sFloat.pop_back();
                if (sFloat.back() == '.')
                    sFloat.push_back('0');
            }
            return sFloat;
        }

        default:
            rc_typed_value_convert(&pValue, RC_VALUE_TYPE_UNSIGNED);
            const auto& pMemoryContext = ra::services::ServiceLocator::Get<ra::context::IEmulatorMemoryContext>();
            return pMemoryContext.FormatAddress(pValue.value.u32);
    }
}

} // namespace data
} // namespace ra
