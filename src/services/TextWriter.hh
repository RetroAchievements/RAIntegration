#ifndef RA_SERVICES_TEXTWRITER
#define RA_SERVICES_TEXTWRITER
#pragma once

#include <string>

namespace ra {
namespace services {

class TextWriter
{
public:
    virtual ~TextWriter() {}

    /// <summary>
    /// Writes text to the output.
    /// </summary>
    /// <param name="sText">The string to write.</param>
    virtual void Write(_In_ std::string& sText) = 0;

    /// <summary>
    /// Writes text to the output.
    /// </summary>
    /// <param name="sText">The string to write.</param>
    virtual void Write(_In_ std::wstring& sText) = 0;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_TEXTWRITER
