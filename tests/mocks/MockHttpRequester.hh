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

    void SetUserAgent([[maybe_unused]] const std::string& /*sUserAgent*/) noexcept override {}

    unsigned int Request(const Http::Request& pRequest, TextWriter& pContentWriter) const override
    {
        auto response = m_fHandler(pRequest);
        pContentWriter.Write(response.Content());
        return ra::etoi(response.StatusCode());
    }

    bool IsRetryable(unsigned int) const noexcept
    {
        return false;
    }

private:
    ServiceLocator::ServiceOverride<IHttpRequester> m_Override;

    std::function<Http::Response(const Http::Request&)> m_fHandler;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_HTTPREQUESTER_HH
