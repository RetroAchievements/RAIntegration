#ifndef RA_SERVICES_MOCK_HTTPREQUESTER_HH
#define RA_SERVICES_MOCK_HTTPREQUESTER_HH
#pragma once

#include "services\IHttpRequester.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace services {
namespace mocks {

class MockHttpRequester : public IHttpRequester
{
public:
    explicit MockHttpRequester(std::function<Http::Response(const Http::Request&)>&& fHandler)
        : m_Override(this), m_fHandler(fHandler)
    {
    }

    void SetUserAgent(const std::string& sUserAgent) override { m_sUserAgent = sUserAgent; }
    const std::string& GetUserAgent() const noexcept { return m_sUserAgent; }

    unsigned int Request(const Http::Request& pRequest, TextWriter& pContentWriter) const override
    {
        auto response = m_fHandler(pRequest);
        pContentWriter.Write(response.Content());
        return ra::etoi(response.StatusCode());
    }

    bool IsRetryable(unsigned int) const noexcept override
    {
        return false;
    }

private:
    ServiceLocator::ServiceOverride<IHttpRequester> m_Override;
    std::string m_sUserAgent;

    std::function<Http::Response(const Http::Request&)> m_fHandler;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_HTTPREQUESTER_HH
