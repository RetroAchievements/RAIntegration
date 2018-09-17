#ifndef RA_SERVICES_TEXTREADER
#define RA_SERVICES_TEXTREADER
#pragma once

#include <string>

namespace ra {
namespace services {

class TextReader
{
public:    
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
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_TEXTREADER
