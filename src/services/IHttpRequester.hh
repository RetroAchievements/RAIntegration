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
};

} // namespace services
} // namespace ra

#endif !RA_SERVICES_IHTTP_REQUESTER_H
