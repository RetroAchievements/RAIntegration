#include "Value.hh"

#include "util/Strings.hh"

#include <rc_runtime_types.h>

namespace ra {
namespace data {

const char* Value::FormatToServerEnum(Format nFormat) noexcept
{
    switch (nFormat)
    {
        case Format::Centiseconds:
            return "MILLISECS";
        case Format::Frames:
            return "TIME";
        case Format::Minutes:
            return "MINUTES";
        case Format::Score:
            return "SCORE";
        case Format::Seconds:
            return "TIMESECS";
        case Format::SecondsAsMinutes:
            return "SECS_AS_MINS";
        case Format::Value:
            return "VALUE";
        case Format::Float1:
            return "FLOAT1";
        case Format::Float2:
            return "FLOAT2";
        case Format::Float3:
            return "FLOAT3";
        case Format::Float4:
            return "FLOAT4";
        case Format::Float5:
            return "FLOAT5";
        case Format::Float6:
            return "FLOAT6";
        case Format::Fixed1:
            return "FIXED1";
        case Format::Fixed2:
            return "FIXED2";
        case Format::Fixed3:
            return "FIXED3";
        case Format::Tens:
            return "TENS";
        case Format::Hundreds:
            return "HUNDREDS";
        case Format::Thousands:
            return "THOUSANDS";
        case Format::UnsignedValue:
            return "UNSIGNED";
        default:
            return "UNKNOWN";
    }
}

Value::Format Value::FormatFromServerEnum(const std::string& sFormat) noexcept
{
    return FormatFromRcheevosFormat(gsl::narrow_cast<uint8_t>(rc_parse_format(sFormat.c_str())));
}

Value::Format Value::FormatFromRcheevosFormat(uint8_t nFormat) noexcept
{
    switch (nFormat)
    {
        case RC_FORMAT_FRAMES: return Format::Frames;
        case RC_FORMAT_SECONDS: return Format::Seconds;
        case RC_FORMAT_CENTISECS: return Format::Centiseconds;
        case RC_FORMAT_SCORE: return Format::Score;
        case RC_FORMAT_VALUE: return Format::Value;
        case RC_FORMAT_MINUTES: return Format::Minutes;
        case RC_FORMAT_SECONDS_AS_MINUTES: return Format::SecondsAsMinutes;
        case RC_FORMAT_FLOAT1: return Format::Float1;
        case RC_FORMAT_FLOAT2: return Format::Float2;
        case RC_FORMAT_FLOAT3: return Format::Float3;
        case RC_FORMAT_FLOAT4: return Format::Float4;
        case RC_FORMAT_FLOAT5: return Format::Float5;
        case RC_FORMAT_FLOAT6: return Format::Float6;
        case RC_FORMAT_FIXED1: return Format::Fixed1;
        case RC_FORMAT_FIXED2: return Format::Fixed2;
        case RC_FORMAT_FIXED3: return Format::Fixed3;
        case RC_FORMAT_TENS: return Format::Tens;
        case RC_FORMAT_HUNDREDS: return Format::Hundreds;
        case RC_FORMAT_THOUSANDS: return Format::Thousands;
        case RC_FORMAT_UNSIGNED_VALUE: return Format::UnsignedValue;
        case RC_FORMAT_UNFORMATTED: return Format::Unformatted;
        default:
            assert(!"Unsupported value format");
            return Format::Value;
    }
}

uint8_t Value::FormatToRcheevosFormat(Format nFormat) noexcept
{
    switch (nFormat)
    {
        case Format::Frames: return RC_FORMAT_FRAMES;
        case Format::Seconds: return RC_FORMAT_SECONDS;
        case Format::Centiseconds: return RC_FORMAT_CENTISECS;
        case Format::Score: return RC_FORMAT_SCORE;
        case Format::Value: return RC_FORMAT_VALUE;
        case Format::Minutes: return RC_FORMAT_MINUTES;
        case Format::SecondsAsMinutes: return RC_FORMAT_SECONDS_AS_MINUTES;
        case Format::Float1: return RC_FORMAT_FLOAT1;
        case Format::Float2: return RC_FORMAT_FLOAT2;
        case Format::Float3: return RC_FORMAT_FLOAT3;
        case Format::Float4: return RC_FORMAT_FLOAT4;
        case Format::Float5: return RC_FORMAT_FLOAT5;
        case Format::Float6: return RC_FORMAT_FLOAT6;
        case Format::Fixed1: return RC_FORMAT_FIXED1;
        case Format::Fixed2: return RC_FORMAT_FIXED2;
        case Format::Fixed3: return RC_FORMAT_FIXED3;
        case Format::Tens: return RC_FORMAT_TENS;
        case Format::Hundreds: return RC_FORMAT_HUNDREDS;
        case Format::Thousands: return RC_FORMAT_THOUSANDS;
        case Format::UnsignedValue: return RC_FORMAT_UNSIGNED_VALUE;
        case Format::Unformatted: return RC_FORMAT_UNFORMATTED;
        default:
            assert(!"Unsupported value format");
            return RC_FORMAT_VALUE;
    }
}

std::wstring Value::FormatValue(int32_t nValue, Format nFormat)
{
    char buffer[32];
    rc_format_value(buffer, sizeof(buffer), nValue, FormatToRcheevosFormat(nFormat));
    return ra::util::String::Widen(buffer);
}

} // namespace data
} // namespace ra
