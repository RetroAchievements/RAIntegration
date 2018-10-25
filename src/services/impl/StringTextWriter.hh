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
    }

    void Write(_In_ const std::string& sText) override
    {
        m_sOutput.append(sText);
    }

    void Write(_In_ const std::wstring& sText) override
    {
        m_sOutput.append(ra::Narrow(sText));
    }

    void WriteLine() override
    {
        m_sOutput.append("\n");
    }

    std::string& GetString() { return m_sOutput; }

private:
    std::string& m_sOutput;
    std::string m_sBuffer;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_STRINGTEXTWRITER
