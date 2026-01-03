#ifndef RA_CONTEXT_IMPL_RCCLIENT_H
#define RA_CONTEXT_IMPL_RCCLIENT_H
#pragma once

#include "context/IRcClient.hh"

#include "services/Http.hh"

namespace ra {
namespace context {
namespace impl {

class RcClient : public IRcClient
{
public:
    RcClient() noexcept;
    ~RcClient() { Shutdown(); }
    RcClient(const RcClient&) noexcept = delete;
    RcClient& operator=(const RcClient&) noexcept = delete;
    RcClient(RcClient&&) noexcept = delete;
    RcClient& operator=(RcClient&&) noexcept = delete;

    void Shutdown() noexcept override;

    void DispatchRequest(const rc_api_request_t& pRequest, std::function<void(const rc_api_server_response_t&, void*)> fCallback, void* fCallbackData) const override;

protected:
    virtual void CallApi(const std::string& sApi, const ra::services::Http::Request& pRequest,
        std::function<void(const rc_api_server_response_t&, void*)> fCallback,
        void* pCallbackData) const;

private:
    static void LogMessage(const char* sMessage, const rc_client_t* pClient);
    static void ServerCallAsync(const rc_api_request_t* pRequest, rc_client_server_callback_t fCallback,
        void* pCallbackData, rc_client_t* pClient);
};

} // namespace impl
} // namespace context
} // namespace ra

#endif RA_CONTEXT_IMPL_RCCLIENT_H
