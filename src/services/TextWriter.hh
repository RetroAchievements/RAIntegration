#ifndef RA_SERVICES_TEXTWRITER
#define RA_SERVICES_TEXTWRITER
#pragma once

namespace ra {
namespace services {

class TextWriter
{
public:
    virtual ~TextWriter() noexcept = default;
	TextWriter(const TextWriter&) noexcept = delete;
	TextWriter& operator=(const TextWriter&) noexcept = delete;
	TextWriter(TextWriter&&) noexcept = delete;
	TextWriter& operator=(TextWriter&&) noexcept = delete;

    /// <summary>Writes text to the output.</summary>
    /// <param name="sText">The string to write.</param>
    virtual void Write(_In_ const std::string& sText) = 0;

    /// <summary>Writes text to the output.</summary>
    /// <param name="sText">The string to write.</param>
    virtual void Write(_In_ const std::wstring& sText) = 0;

    /// <summary>Writes a newline to the output.</summary>
    /// <param name="sText">The string to write.</param>
    virtual void WriteLine() = 0;

    /// <summary>Writes text and a newline to the output.</summary>
    /// <param name="sText">The string to write.</param>
    void WriteLine(_In_ const std::string& sText)
    {
        Write(sText);
        WriteLine();
    }

    /// <summary>Writes text and a newline to the output.</summary>
    /// <param name="sText">The string to write.</param>
    void WriteLine(_In_ const std::wstring& sText)
    {
        Write(sText);
        WriteLine();
    }
protected:
	TextWriter() noexcept = default;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_TEXTWRITER
