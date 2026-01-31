#include "RA_Defs.h"

#include "context/IEmulatorMemoryContext.hh"

#include "services/ServiceLocator.hh"

#include "util/Strings.hh"

#include <rcheevos/src/rcheevos/rc_internal.h>

namespace ra {
namespace data {

const char* ValueFormatToString(ValueFormat nFormat) noexcept
{
    switch (nFormat)
    {
        case ValueFormat::Centiseconds: return "MILLISECS";
        case ValueFormat::Frames: return "TIME";
        case ValueFormat::Minutes: return "MINUTES";
        case ValueFormat::Score: return "SCORE";
        case ValueFormat::Seconds: return "TIMESECS";
        case ValueFormat::SecondsAsMinutes: return "SECS_AS_MINS";
        case ValueFormat::Value: return "VALUE";
        case ValueFormat::Float1: return "FLOAT1";
        case ValueFormat::Float2: return "FLOAT2";
        case ValueFormat::Float3: return "FLOAT3";
        case ValueFormat::Float4: return "FLOAT4";
        case ValueFormat::Float5: return "FLOAT5";
        case ValueFormat::Float6: return "FLOAT6";
        case ValueFormat::Fixed1: return "FIXED1";
        case ValueFormat::Fixed2: return "FIXED2";
        case ValueFormat::Fixed3: return "FIXED3";
        case ValueFormat::Tens: return "TENS";
        case ValueFormat::Hundreds: return "HUNDREDS";
        case ValueFormat::Thousands: return "THOUSANDS";
        case ValueFormat::UnsignedValue: return "UNSIGNED";
        default: return "UNKNOWN";
    }
}

ValueFormat ValueFormatFromString(const std::string& sFormat) noexcept
{
    return ra::itoe<ValueFormat>(rc_parse_format(sFormat.c_str()));
}

} /* namespace data */
} /* namespace ra */
