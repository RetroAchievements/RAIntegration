#ifndef RA_SERVICES_TEXTWRITER
#define RA_SERVICES_TEXTWRITER
#pragma once

#ifndef PCH_H
#include <xstring>
#endif /* !PCH_H */

namespace ra {
namespace services {

class TextWriter
{
public:
    virtual ~TextWriter() noexcept = default;

    /// <summary>
    /// Writes text to the output.
    /// </summary>
    /// <param name="sText">The string to write.</param>
    virtual void Write(_In_ const std::string& sText) = 0;

    /// <summary>
    /// Writes text to the output.
    /// </summary>
    /// <param name="sText">The string to write.</param>
    virtual void Write(_In_ const std::wstring& sText) = 0;

    /// <summary>
    /// Writes a newline to the output.
    /// </summary>
    /// <param name="sText">The string to write.</param>
    virtual void WriteLine() = 0;

    /// <summary>
    /// Writes text and a newline to the output.
    /// </summary>
    /// <param name="sText">The string to write.</param>
    void WriteLine(_In_ const std::string& sText)
    {
        Write(sText);
        WriteLine();
    }

    /// <summary>
    /// Writes text and a newline to the output.
    /// </summary>
    /// <param name="sText">The string to write.</param>
    void WriteLine(_In_ const std::wstring& sText)
    {
        Write(sText);
        WriteLine();
    }
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_TEXTWRITER
