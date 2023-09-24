#ifndef RA_SERVICES_RCHEEVOS_CLIENT_HH
#define RA_SERVICES_RCHEEVOS_CLIENT_HH
#pragma once

#include "ra_fwd.h"

#include "data\Types.hh"
#include "data\context\EmulatorContext.hh"

#include "services\TextReader.hh"

#include <string>

#include <rcheevos\include\rc_client.h>

struct rc_api_fetch_game_data_response_t;


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

    void Shutdown() noexcept;

    rc_client_t* GetClient() const noexcept { return m_pClient.get(); }

    void SyncAssets();

    void BeginLoginWithToken(const std::string& sUsername, const std::string& sApiToken,
                             rc_client_callback_t fCallback, void* pCallbackData);
    void BeginLoginWithPassword(const std::string& sUsername, const std::string& sPassword,
                                rc_client_callback_t fCallback, void* pCallbackData);

    void BeginLoadGame(const std::string& sHash, unsigned id,
                       rc_client_callback_t fCallback, void* pCallbackData);

    class Synchronizer
    {
    public:
        void Wait()
        {
#ifdef RA_UTEST
            // unit tests are single-threaded. if m_bWaiting is true, it would wait indefinitely.
            if (m_bWaiting)
                Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(L"Sycnhronous request was not handled.");
#else
            std::unique_lock<std::mutex> lock(m_pMutex);
            if (m_bWaiting)
                m_pCondVar.wait(lock);
#endif
        }

        void Notify() noexcept
        {
            m_bWaiting = false;
            m_pCondVar.notify_all();
        }

        void CaptureResult(int nResult, const char* sErrorMessage)
        {
            m_nResult = nResult;

            if (sErrorMessage)
                m_sErrorMessage = sErrorMessage;
            else
                m_sErrorMessage.clear();
        }

        int GetResult() const noexcept { return m_nResult; }

        const std::string& GetErrorMessage() const noexcept { return m_sErrorMessage; }

    private:
        std::mutex m_pMutex;
        std::condition_variable m_pCondVar;
        std::string m_sErrorMessage;
        int m_nResult = 0;
        bool m_bWaiting = true;
    };

private:
    std::unique_ptr<rc_client_t> m_pClient;

    class ClientSynchronizer;
    std::unique_ptr<ClientSynchronizer> m_pClientSynchronizer;

    static void LogMessage(const char* sMessage, const rc_client_t* pClient);
    static uint32_t ReadMemory(uint32_t nAddress, uint8_t* pBuffer, uint32_t nBytes, rc_client_t* pClient);
    static void ServerCallAsync(const rc_api_request_t* pRequest, rc_client_server_callback_t fCallback,
                           void* pCallbackData, rc_client_t* pClient);

    class CallbackWrapper
    {
    public:
        CallbackWrapper(rc_client_t* client, rc_client_callback_t callback, void* callback_userdata) noexcept :
            m_pClient(client), m_fCallback(callback), m_pCallbackUserdata(callback_userdata)
        {}

        void DoCallback(int nResult, const char* sErrorMessage) noexcept
        {
            m_fCallback(nResult, sErrorMessage, m_pClient, m_pCallbackUserdata);
        }

        static void Dispatch(int nResult, const char* sErrorMessage, rc_client_t*, void* pUserdata)
        {
            auto* pWrapper = static_cast<CallbackWrapper*>(pUserdata);
            Expects(pWrapper != nullptr);

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
                                                     CallbackWrapper* pCallbackWrapper) noexcept;
    rc_client_async_handle_t* BeginLoginWithToken(const char* sUsername, const char* sApiToken,
                                                  CallbackWrapper* pCallbackWrapper) noexcept;
    static void LoginCallback(int nResult, const char* sErrorMessage, rc_client_t* pClient, void* pUserdata);

    class LoadGameCallbackWrapper : public CallbackWrapper
    {
    public:
        LoadGameCallbackWrapper(rc_client_t* client, rc_client_callback_t callback, void* callback_userdata) noexcept :
            CallbackWrapper(client, callback, callback_userdata)
        {}

        std::map<uint32_t, std::string> m_mAchievementDefinitions;
        std::map<uint32_t, std::string> m_mLeaderboardDefinitions;
    };

    rc_client_async_handle_t* BeginLoadGame(const char* sHash, unsigned id, CallbackWrapper* pCallbackWrapper) noexcept;
    static void LoadGameCallback(int nResult, const char* sErrorMessage, rc_client_t* pClient, void* pUserdata);

    static void PostProcessGameDataResponse(const rc_api_server_response_t* server_response,
                                            struct rc_api_fetch_game_data_response_t* game_data_response,
                                            rc_client_t* client, void* pUserdata);
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_RCHEEVOS_CLIENT_HH
