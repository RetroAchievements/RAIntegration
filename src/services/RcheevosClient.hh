#ifndef RA_SERVICES_RCHEEVOS_CLIENT_HH
#define RA_SERVICES_RCHEEVOS_CLIENT_HH
#pragma once

#include "ra_fwd.h"

#include "data\Types.hh"
#include "data\context\EmulatorContext.hh"

#include "services\TextReader.hh"

#include <string>

#include <rcheevos\include\rc_client.h>

namespace ra {
namespace services {

class RcheevosClient
{
public:
    GSL_SUPPRESS_F6 RcheevosClient();
    virtual ~RcheevosClient();

    RcheevosClient(const RcheevosClient&) noexcept = delete;
    RcheevosClient& operator=(const RcheevosClient&) noexcept = delete;
    RcheevosClient(RcheevosClient&&) noexcept = delete;
    RcheevosClient& operator=(RcheevosClient&&) noexcept = delete;

    void Shutdown();

    rc_client_t* GetClient() const { return m_pClient.get(); }

    void SetSynchronous(bool bSynchronous);

    void BeginLoginWithToken(const std::string& sUsername, const std::string& sApiToken,
                             rc_client_callback_t fCallback, void* pCallbackData);
    void BeginLoginWithPassword(const std::string& sUsername, const std::string& sPassword,
                                rc_client_callback_t fCallback, void* pCallbackData);

private:
    std::unique_ptr<rc_client_t> m_pClient;

    static void LogMessage(const char* sMessage, const rc_client_t* pClient);
    static uint32_t ReadMemory(uint32_t nAddress, uint8_t* pBuffer, uint32_t nBytes, rc_client_t* pClient);
    static void ServerCall(const rc_api_request_t* pRequest, rc_client_server_callback_t fCallback,
                           void* pCallbackData, rc_client_t* pClient);
    static void ServerCallAsync(const rc_api_request_t* pRequest, rc_client_server_callback_t fCallback,
                           void* pCallbackData, rc_client_t* pClient);

    static void LoginCallback(int nResult, const char* sErrorMessage, rc_client_t* pClient, void* pUserdata);
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_RCHEEVOS_CLIENT_HH
