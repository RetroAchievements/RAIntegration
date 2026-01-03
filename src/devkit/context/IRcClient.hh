#ifndef RA_CONTEXT_IRCCLIENT_H
#define RA_CONTEXT_IRCCLIENT_H
#pragma once

#include <rcheevos/include/rc_client.h>

#include <functional>
#include <memory>

namespace ra {
namespace context {

class IRcClient
{
public:
    IRcClient() noexcept = default;
    virtual ~IRcClient() noexcept;
    IRcClient(const IRcClient&) noexcept = delete;
    IRcClient& operator=(const IRcClient&) noexcept = delete;
    IRcClient(IRcClient&&) noexcept = delete;
    IRcClient& operator=(IRcClient&&) noexcept = delete;

    /// <summary>
    /// Shuts down the runtime.
    /// </summary>
    virtual void Shutdown() noexcept {};

    /// <summary>
    /// Gets the underlying rc_client object for directly calling into rcheevos.
    /// </summary>
    rc_client_t* GetClient() const noexcept { return m_pClient.get(); }

    /// <summary>
    /// Makes an API call to the server asynchronously.
    /// </summary>
    /// <param name="pRequest">Information about the API call to make.</param>
    /// <param name="fCallback">Function to call with the API response.</param>
    /// <param name="pCallbackData">Additional data to pass to the <paramref name="fCallback" /> function.</param>
    virtual void DispatchRequest(const rc_api_request_t& pRequest, std::function<void(const rc_api_server_response_t&, void*)> fCallback, void* fCallbackData) const = 0;

protected:
    std::unique_ptr<rc_client_t> m_pClient;
};

} // namespace context
} // namespace ra

#endif RA_CONTEXT_IRCCLIENT_H
