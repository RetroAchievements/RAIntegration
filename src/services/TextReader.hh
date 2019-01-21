#ifndef RA_SERVICES_TEXTREADER
#define RA_SERVICES_TEXTREADER
#pragma once

namespace ra {
namespace services {

class TextReader
{
public:
    virtual ~TextReader() noexcept = default;
    TextReader(const TextReader&) noexcept = delete;
    TextReader& operator=(const TextReader&) noexcept = delete;
    TextReader(TextReader&&) noexcept = delete;
    TextReader& operator=(TextReader&&) noexcept = delete;

    /// <summary>
    /// Reads the next line from the input.
    /// </summary>
    /// <param name="sLine">The string to read the next line into.</param>
    /// <returns><c>true</c> if a line was read, <c>false</c> if the end of the input was reached.</returns>
    virtual bool GetLine(_Out_ std::string& sLine) = 0;

    /// <summary>
    /// Reads the next line from the input.
    /// </summary>
    /// <param name="sLine">The string to read the next line into.</param>
    /// <returns><c>true</c> if a line was read, <c>false</c> if the end of the input was reached.</returns>
    virtual bool GetLine(_Out_ std::wstring& sLine) = 0;
    
    /// <summary>
    /// Gets the current read offset within the input.
    /// </summary>
    virtual std::streampos GetPosition() const = 0;

protected:
    TextReader() noexcept = default;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_TEXTREADER
