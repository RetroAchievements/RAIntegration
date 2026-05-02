#ifndef RA_UTIL_TOKENIZER_HH
#define RA_UTIL_TOKENIZER_HH
#pragma once

#include "GSL.hh"

#include <string>

namespace ra {
namespace util {

class Tokenizer
{
public:
    Tokenizer(const std::string_view sString) noexcept : m_sString(sString) {}

    /// <summary>
    /// Returns <c>true</c> if the entire string has been processed.
    /// </summary>
    _NODISCARD bool EndOfString() const noexcept { return m_nPosition >= m_sString.length(); }

    /// <summary>
    /// Returns the next character without advancing the position.
    /// </summary>
    GSL_SUPPRESS_F6 _NODISCARD char PeekChar() const noexcept { return m_nPosition < m_sString.length() ? m_sString.at(m_nPosition) : '\0'; }

    /// <summary>
    /// Get the current position of the cursor within the string.
    /// </summary>
    _NODISCARD size_t CurrentPosition() const noexcept { return m_nPosition; }

    /// <summary>
    /// Sets the cursor position.
    /// </summary>
    void Seek(size_t nPosition) noexcept { m_nPosition = std::min(nPosition, m_sString.length()); }

    /// <summary>
    /// Advances the cursor one character.
    /// </summary>
    void Advance() noexcept
    {
        if (m_nPosition < m_sString.length())
            ++m_nPosition;
    }

    /// <summary>
    /// Advances the cursor the specified number of characters.
    /// </summary>
    void Advance(size_t nCount) noexcept
    {
        m_nPosition += nCount;
        if (m_nPosition > m_sString.length())
            m_nPosition = m_sString.length();
    }

    /// <summary>
    /// Advances the cursor to the next occurrence of the specified character, or the end of the string if no
    /// occurrences are found.
    /// </summary>
    void AdvanceTo(char cStop)
    {
        while (m_nPosition < m_sString.length() && m_sString.at(m_nPosition) != cStop)
            ++m_nPosition;
    }

    /// <summary>
    /// Advances the cursor to the next occurrence of the specified character, or the end of the string if no
    /// occurrences are found and returns a string containing all of the characters advanced over.
    /// </summary>
    _NODISCARD std::string_view ReadTo(char cStop)
    {
        const size_t nStart = m_nPosition;
        AdvanceTo(cStop);
        return m_sString.substr(nStart, m_nPosition - nStart);
    }

    /// <summary>
    /// Reads from the current quote to the next unescaped quote, unescaping any other characters along the way.
    /// </summary>
    _NODISCARD std::string ReadQuotedString();

    /// <summary>
    /// Advances the cursor over digits and returns the number they represent.
    /// </summary>
    _NODISCARD unsigned int ReadNumber()
    {
        if (EndOfString())
            return 0;

        char* pEnd;
        const auto nResult = strtoul(&m_sString.at(m_nPosition), &pEnd, 10);
        m_nPosition = pEnd - m_sString.data();
        return nResult;
    }

    /// <summary>
    /// Returns the number represented by the next series of digits.
    /// </summary>
    _NODISCARD unsigned int PeekNumber()
    {
        if (EndOfString())
            return 0;

        char* pEnd;
        return strtoul(&m_sString.at(m_nPosition), &pEnd, 10);
    }

    /// <summary>
    /// If the next character is the specified character, advance the cursor over it.
    /// </summary>
    /// <returns><c>true</c> if the next character matched and was skipped over, <c>false</c> if not.</returns>
    bool Consume(char c)
    {
        if (EndOfString())
            return false;
        if (m_sString.at(m_nPosition) != c)
            return false;
        ++m_nPosition;
        return true;
    }

    /// <summary>
    /// Gets the raw pointer to the specified offset within the string.
    /// </summary>
    GSL_SUPPRESS_F6 const char* GetPointer(size_t nPosition) const noexcept
    {
        if (nPosition >= m_sString.length())
            return &m_sString.back() + 1;

        return &m_sString.at(nPosition);
    }

private:
    const std::string_view m_sString;
    size_t m_nPosition = 0;
};

} // namespace util
} // namespace ra

#endif // !RA_UTIL_TOKENIZER_HH
