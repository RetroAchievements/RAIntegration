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
            m_sOutput.replace(m_nWritePosition, sText.length(), sText.c_str());
            m_nWritePosition += sText.length();
        }
        else
        {
            m_sOutput.append(sText);
            m_nWritePosition = m_sOutput.length();
        }
    }

    void Write(_In_ const std::wstring& sText) override
    {
        Write(ra::Narrow(sText));
    }

    void WriteLine() override
    {
        Write(std::string("\n"));
    }

    long GetPosition() const override
    {
        return m_nWritePosition;
    }

    void SetPosition(long nNewPosition) override
    {
        assert(nNewPosition >= 0 && nNewPosition <= ra::to_signed(m_sOutput.length()));
        m_nWritePosition = static_cast<size_t>(nNewPosition);
    }

    std::string& GetString() { return m_sOutput; }

private:
    std::string& m_sOutput;
    std::string m_sBuffer;
    size_t m_nWritePosition;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_STRINGTEXTWRITER
