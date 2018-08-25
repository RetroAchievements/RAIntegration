#ifndef RA_MEMVALUE_H
#define RA_MEMVALUE_H
#pragma once

#include "RA_Condition.h"

#ifndef _VECTOR_
#include <vector>  
#endif /* !_VECTOR_ */


// Represents a value expression (one or more values which are added together to create a single value)
class MemValue
{
public:
    /*_NODISCARD */const char* ParseFromString(const char* sBuffer);
    _NODISCARD unsigned int GetValue() const;

    _NODISCARD bool IsEmpty() const noexcept { return m_vClauses.empty(); }


    enum class Format
    {
        TimeFrames,     //	Value is a number describing the amount of frames.
        TimeSecs,       //	Value is a number in amount of seconds.
        TimeMillisecs,  //	Value is a number accurate to hundredths of a second (not actually milliseconds)
        Score,          //	Value is a nondescript 'score' Points.
        Value,          //	Generic value - %01d
        Other           //	Point-based value (i.e. 1942 ships killed). %06d
    };

    _NODISCARD std::string GetFormattedValue(Format nFormat) const { return FormatValue(GetValue(), nFormat); }

    _NODISCARD std::string FormatValue(unsigned int nValue, Format nFormat) const;
    _NODISCARD _CONSTANT_FN ParseFormat(_In_ const char* sFormat) noexcept
    {
        if (sFormat == "VALUE")
            return Format::Value;

        if (sFormat == "SECS" || sFormat == "TIMESECS")
            return Format::TimeSecs;

        if (sFormat == "FRAMES" || sFormat == "TIME")
            return Format::TimeFrames;

        if (sFormat == "POINTS" || sFormat == "SCORE")
            return Format::Score;

        if (sFormat == "MILLISECS")
            return Format::TimeMillisecs;

        if (sFormat == "OTHER")
            return Format::Other;

        return Format::Value;
    }
    _NODISCARD _CONSTANT_FN GetFormatString(_In_ Format format) noexcept
    {
        switch (format)
        {
            case Format::Other: return "OTHER";
            case Format::Score: return "POINTS";
            case Format::TimeFrames: return "FRAMES";
            case Format::TimeMillisecs: return "MILLISECS";
            case Format::TimeSecs: return "SECS";
            case Format::Value: return "VALUE";
            default: return "UNKNOWN";
        }
    }

protected:
    enum class ClauseOperation
    {
        None,
        Addition,
        Maximum
    };

    // Represents a single value which may be scaled (multiplied by another value)
    class Clause
    {
    public:
        explicit inline constexpr Clause(_In_ ClauseOperation nOperation) noexcept :
            m_nOperation{ nOperation }
        {
        }


        _NODISCARD const char* ParseFromString(const char* pBuffer); // Parse string into values, returns end of string
        _NODISCARD double GetValue() const; // Get the value in-memory with modifiers
        _NODISCARD inline constexpr ClauseOperation GetOperation() const noexcept { return m_nOperation; }

    protected:
        unsigned int               m_nAddress{};  // Raw address of an 8-bit, or value.
        ra::ComparisonVariableSize m_nVarSize{ ra::ComparisonVariableSize::EightBit };
        double                     m_fModifier{ 1.0F }; // * 60 etc
        bool                       m_bBCDParse{}; // Parse as a binary coded decimal.
        bool                       m_bParseVal{}; // Parse as a value
        bool                       m_bInvertBit{};
        unsigned int               m_nSecondAddress{};
        ra::ComparisonVariableSize m_nSecondVarSize{ ra::ComparisonVariableSize::EightBit };
        ClauseOperation            m_nOperation{};
    };

    std::vector<Clause> m_vClauses;
};

#endif /* !RA_MEMVALUE_H */
