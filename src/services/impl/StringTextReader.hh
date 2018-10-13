#ifndef RA_SERVICES_STRINGTEXTREADER
#define RA_SERVICES_STRINGTEXTREADER
#pragma once

#include "services\TextReader.hh"

#include "RA_StringUtils.h"

namespace ra {
namespace services {
namespace impl {

class StringTextReader : public ra::services::TextReader
{
public:
    explicit StringTextReader(const std::string& sInput) noexcept
        : m_iStream(sInput)
    {
    }

    bool GetLine(_Out_ std::string& sLine) override
    {
        if (!std::getline(m_iStream, sLine))
            return false;

        ra::TrimLineEnding(sLine);
        return true;
    }

	_Success_(return)
    _NODISCARD bool GetLine(_Out_ std::wstring& sLine) override
    {
        std::string sNarrowLine;
        if (!std::getline(m_iStream, sNarrowLine))
            return false;

        sLine = ra::Widen(ra::TrimLineEnding(sNarrowLine));
        return true;
    }

    std::string GetString() { return m_iStream.str(); }

private:
    std::istringstream m_iStream;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_STRINGTEXTREADER
