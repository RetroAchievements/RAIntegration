#ifndef RA_SERVICES_FILETEXTREADER
#define RA_SERVICES_FILETEXTREADER
#pragma once

#include "services\TextReader.hh"

#include "RA_StringUtils.h"

namespace ra {
namespace services {
namespace impl {

class FileTextReader : public ra::services::TextReader
{
public:
    explicit FileTextReader(const std::wstring& sFilename)
        : m_iStream(sFilename)
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

    std::ifstream& GetFStream() noexcept { return m_iStream; }

private:
    mutable std::ifstream m_iStream;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_FILETEXTREADER
