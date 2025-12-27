#ifndef RA_CONTEXT_IMPL_OFFLINE_RCCLIENT_H
#define RA_CONTEXT_IMPL_OFFLINE_RCCLIENT_H
#pragma once

#include "context\impl\RcClient.hh"

namespace ra {
namespace services {
namespace impl {

class OfflineRcClient : public context::impl::RcClient
{
protected:
    void CallApi(const std::string& sApi, const ra::services::Http::Request& pRequest,
        std::function<void(const rc_api_server_response_t&, void*)> fCallback,
        void* pCallbackData) const override;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif RA_CONTEXT_IMPL_OFFLINE_RCCLIENT_H
