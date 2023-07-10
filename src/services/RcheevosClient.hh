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

    void BeginLoginWithToken(const std::string& sUsername, const std::string& sApiToken,
                             rc_client_callback_t fCallback, void* pCallbackData);
    void BeginLoginWithPassword(const std::string& sUsername, const std::string& sPassword,
                                rc_client_callback_t fCallback, void* pCallbackData);

    class Synchronizer
    {
    public:
        void Wait()
        {
#ifdef RA_UTEST
            // unit tests are single-threaded. if m_bWaiting is true, it would wait indefinitely.
            if (m_bWaiting)
                throw std::runtime_error("Sycnhronous request was not handled");
#else
            std::unique_lock<std::mutex> lock(m_pMutex);
            if (m_bWaiting)
                m_pCondVar.wait(lock);
#endif
        }

        void Notify()
        {
            m_bWaiting = false;
            m_pCondVar.notify_all();
        }

    private:
        std::mutex m_pMutex;
        std::condition_variable m_pCondVar;
        bool m_bWaiting = true;
    };

private:
    std::unique_ptr<rc_client_t> m_pClient;

    static void LogMessage(const char* sMessage, const rc_client_t* pClient);
    static uint32_t ReadMemory(uint32_t nAddress, uint8_t* pBuffer, uint32_t nBytes, rc_client_t* pClient);
    static void ServerCallAsync(const rc_api_request_t* pRequest, rc_client_server_callback_t fCallback,
                           void* pCallbackData, rc_client_t* pClient);

    class CallbackWrapper
    {
    public:
        CallbackWrapper(rc_client_t* client, rc_client_callback_t callback, void* callback_userdata) :
            m_pClient(client), m_fCallback(callback), m_pCallbackUserdata(callback_userdata)
        {}

        void DoCallback(int nResult, const char* sErrorMessage)
        {
            m_fCallback(nResult, sErrorMessage, m_pClient, m_pCallbackUserdata);
        }

        static void Dispatch(int nResult, const char* sErrorMessage, rc_client_t*, void* pUserdata)
        {
            auto* pWrapper = (CallbackWrapper*)pUserdata;

            pWrapper->DoCallback(nResult, sErrorMessage);

            delete pWrapper;
        }

    private:
        rc_client_t* m_pClient;
        rc_client_callback_t m_fCallback;
        void* m_pCallbackUserdata;
    };

    friend class RcheevosClientExports;

    rc_client_async_handle_t* BeginLoginWithPassword(const char* sUsername, const char* sPassword,
                                                     CallbackWrapper* pCallbackWrapper);
    rc_client_async_handle_t* BeginLoginWithToken(const char* sUsername, const char* sApiToken,
                                                  CallbackWrapper* pCallbackWrapper);
    static void LoginCallback(int nResult, const char* sErrorMessage, rc_client_t* pClient, void* pUserdata);
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_RCHEEVOS_CLIENT_HH
