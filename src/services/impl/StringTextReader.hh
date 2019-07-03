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
    explicit StringTextReader(const std::string& sInput)
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

    size_t GetBytes(_Inout_ char pBuffer[], _In_ size_t nBytes) override
    {
        const auto nPos = GetPosition();
        m_iStream.read(pBuffer, nBytes);
        return gsl::narrow_cast<size_t>(GetPosition() - nPos);
    }

    std::streampos GetPosition() const override
    {
        if (!m_iStream.good())
        {
            // if we've set the eof flag, tellg() will return -1 unless we reset it
            if (m_iStream.eof())
            {
                m_iStream.clear();
                m_iStream.seekg(0, m_iStream.end);
            }
        }

        return m_iStream.tellg();
    }

    std::string GetString() { return m_iStream.str(); }

private:
    mutable std::istringstream m_iStream;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_STRINGTEXTREADER
