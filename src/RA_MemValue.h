#ifndef RA_MEMVALUE_H
#define RA_MEMVALUE_H
#pragma once

#include "RA_Condition.h"

#include <vector>

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
        Other,          //	Point-based value (i.e. 1942 ships killed). %06d
    };

    _NODISCARD std::string GetFormattedValue(Format nFormat) const { return FormatValue(GetValue(), nFormat); }

    _NODISCARD static std::string FormatValue(unsigned int nValue, Format nFormat);
    _NODISCARD static Format ParseFormat(const std::string& sFormat);
    _NODISCARD static const char* GetFormatString(Format format);

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
        // This class is having similar problems as the leaderboard
        // FYI, the default constructors are constexpr we just have put it back since they aren't defaulted (0 constructors)
        inline constexpr Clause() noexcept = default; // uses values below
        explicit inline constexpr Clause(_In_ ClauseOperation nOperation) noexcept :
            Clause{}
        {
            m_nOperation = nOperation;
        }
        ~Clause() noexcept = default;

        // should be relatively fast to copy a literal type unless you don't want it
        inline constexpr Clause(const Clause&) noexcept = default;
        inline constexpr Clause& operator=(const Clause&) noexcept = default;
        inline constexpr Clause(Clause&&) noexcept = default;
        inline constexpr Clause& operator=(Clause&&) noexcept = default;


        _NODISCARD const char* ParseFromString(const char* pBuffer); // Parse string into values, returns end of string
        _NODISCARD double GetValue() const;                          // Get the value in-memory with modifiers
        _NODISCARD ClauseOperation GetOperation() const noexcept { return m_nOperation; }

    protected:
        unsigned int			   m_nAddress{};               // Raw address of an 8-bit, or value.
        ra::ComparisonVariableSize m_nVarSize       = ra::ComparisonVariableSize::EightBit;
        double					   m_fModifier      = 1.0F; // * 60 etc
        bool					   m_bBCDParse{};           // Parse as a binary coded decimal.
        bool					   m_bParseVal{};           // Parse as a value
        bool					   m_bInvertBit{};
        unsigned int			   m_nSecondAddress{};
        ra::ComparisonVariableSize m_nSecondVarSize = ra::ComparisonVariableSize::EightBit;
        ClauseOperation            m_nOperation{};
    };

    std::vector<Clause> m_vClauses;
};

#endif /* !RA_MEMVALUE_H */
