#ifndef RA_SERVICES_FILETEXTREADER
#define RA_SERVICES_FILETEXTREADER
#pragma once

#include "services\TextReader.hh"

#include "RA_StringUtils.h"

#include <fstream>

namespace ra {
namespace services {
namespace impl {

class FileTextReader : public ra::services::TextReader
{
public:
    explicit FileTextReader(const std::wstring& sFilename) noexcept
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

    bool GetLine(_Out_ std::wstring& sLine) override
    {
        std::string sNarrowLine;
        if (!std::getline(m_iStream, sNarrowLine))
            return false;

        sLine = ra::Widen(ra::TrimLineEnding(sNarrowLine));
        return true;
    }

    std::ifstream& GetFStream() { return m_iStream; }

private:
    std::ifstream m_iStream;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_FILETEXTREADER
