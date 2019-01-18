#ifndef RA_SERVICES_IHTTP_REQUESTER_H
#define RA_SERVICES_IHTTP_REQUESTER_H
#pragma once

#include "services\Http.hh"
#include "services\TextWriter.hh"

namespace ra {
namespace services {

class IHttpRequester
{
public:
    virtual ~IHttpRequester() noexcept = default;
    IHttpRequester(const IHttpRequester&) noexcept = delete;
    IHttpRequester& operator=(const IHttpRequester&) noexcept = delete;
    IHttpRequester(IHttpRequester&&) noexcept = delete;
    IHttpRequester& operator=(IHttpRequester&&) noexcept = delete;

    /// <summary>
    /// Sets the default User-Agent to send for requests made by this service.
    /// </summary>
    virtual void SetUserAgent(const std::string& sUserAgent) = 0;

    /// <summary>
    /// Calls this server and waits for the response.
    /// </summary>
    /// <param name="pRequest">The request to send.</param>
    /// <param name="pContentWriter">A writer to write the content to as its received.</param>
    /// <returns>The status code from the server, or an error code if the request failed before reaching the server.</returns>
    virtual unsigned int Request(const Http::Request& pRequest, TextWriter& pContentWriter) const = 0;

    /// <summary>
    /// Determines whether or not it would be reasonable to retry the request for the provided error code.
    /// </summary>
    _NODISCARD virtual bool IsRetryable(unsigned int nStatusCode) const noexcept = 0;

protected:
    IHttpRequester() noexcept = default;
};

} // namespace services
} // namespace ra

#endif !RA_SERVICES_IHTTP_REQUESTER_H
