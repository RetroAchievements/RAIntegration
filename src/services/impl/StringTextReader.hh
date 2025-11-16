#ifndef RA_SERVICES_STRINGTEXTREADER
#define RA_SERVICES_STRINGTEXTREADER
#pragma once

#include "services\TextReader.hh"

#include "util\Strings.hh"

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

    size_t GetBytes(_Inout_ uint8_t pBuffer[], _In_ size_t nBytes) override
    {
        const auto nPos = GetPosition();
        char* pCharBuffer;
        GSL_SUPPRESS_TYPE1 pCharBuffer = reinterpret_cast<char*>(pBuffer);
        m_iStream.read(pCharBuffer, nBytes);
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

    void SetPosition(std::streampos nNewPosition) override
    {
        Expects(nNewPosition >= 0);
        m_iStream.seekg(nNewPosition);
    }

    size_t GetSize() const override
    {
        const auto nPos = GetPosition();
        m_iStream.seekg(0, m_iStream.end);
        const auto nSize = gsl::narrow_cast<size_t>(m_iStream.tellg());
        m_iStream.seekg(nPos, m_iStream.beg);
        return nSize;
    }

    std::string GetString() { return m_iStream.str(); }

private:
    mutable std::istringstream m_iStream;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_STRINGTEXTREADER
