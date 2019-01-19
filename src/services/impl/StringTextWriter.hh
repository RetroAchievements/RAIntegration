#ifndef RA_SERVICES_STRINGTEXTWRITER
#define RA_SERVICES_STRINGTEXTWRITER
#pragma once

#include "services\TextWriter.hh"

#include "RA_StringUtils.h"

namespace ra {
namespace services {
namespace impl {

class StringTextWriter : public ra::services::TextWriter
{
public:
    StringTextWriter() noexcept
        : StringTextWriter(m_sBuffer)
    {
    }

    explicit StringTextWriter(std::string& sOutput) noexcept
        : m_sOutput(sOutput)
    {
        m_nWritePosition = sOutput.length();
    }

    void Write(_In_ const std::string& sText) override
    {
        if (m_nWritePosition < m_sOutput.length())
        {
            m_sOutput.replace(gsl::narrow_cast<std::size_t>(m_nWritePosition), sText.length(), sText.c_str());
            m_nWritePosition += sText.length();
        }
        else
        {
            m_sOutput.append(sText);
            m_nWritePosition = m_sOutput.length();
        }
    }

    void Write(_In_ const std::wstring& sText) override { Write(ra::Narrow(sText)); }
    void WriteLine() override { Write(std::string("\n")); }
    std::streampos GetPosition() const noexcept override { return m_nWritePosition; }

    void SetPosition(std::streampos nNewPosition) noexcept override
    {
        GSL_SUPPRESS_F6 Expects(nNewPosition >= 0 && nNewPosition <= ra::to_signed(m_sOutput.length()));
        GSL_SUPPRESS_F6 m_nWritePosition = nNewPosition;
    }

    std::string& GetString() noexcept { return m_sOutput; }

private:
    std::string& m_sOutput;
    std::string m_sBuffer;
    std::streamoff m_nWritePosition;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_STRINGTEXTWRITER
