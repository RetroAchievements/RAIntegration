#ifndef RA_SERVICES_WIN32_HTTPREQUESTER_HH
#define RA_SERVICES_WIN32_HTTPREQUESTER_HH
#pragma once

#include "services\IHttpRequester.hh"

#include "RA_StringUtils.h"

namespace ra {
namespace services {
namespace impl {

#undef CreateDirectory

class WindowsHttpRequester : public IHttpRequester
{
public:
    void SetUserAgent(const std::string& sUserAgent) override { m_sUserAgent = ra::Widen(sUserAgent); }

    unsigned int Request(const Http::Request& pRequest, TextWriter& pContentWriter) const override;

    bool IsRetryable(unsigned int nStatusCode) const noexcept override;

private:
    std::wstring m_sUserAgent;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_WIN32_HTTPREQUESTER_HH
