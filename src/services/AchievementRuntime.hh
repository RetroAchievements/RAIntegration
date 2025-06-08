#ifndef RA_SERVICES_ACHIEVEMENT_RUNTIME_HH
#define RA_SERVICES_ACHIEVEMENT_RUNTIME_HH
#pragma once

#include "ra_fwd.h"

#include "data\Types.hh"
#include "data\context\EmulatorContext.hh"
#include "data\models\AchievementModel.hh"
#include "data\models\LeaderboardModel.hh"

#include "services\TextReader.hh"

#include <string>

#include <rcheevos\include\rc_client.h>

struct rc_api_fetch_game_sets_response_t;

namespace ra {
namespace services {

class AchievementRuntime
{
public:
    GSL_SUPPRESS_F6 AchievementRuntime();
    virtual ~AchievementRuntime();

    AchievementRuntime(const AchievementRuntime&) noexcept = delete;
    AchievementRuntime& operator=(const AchievementRuntime&) noexcept = delete;
    AchievementRuntime(AchievementRuntime&&) noexcept = delete;
    AchievementRuntime& operator=(AchievementRuntime&&) noexcept = delete;

    /// <summary>
    /// Shuts down the runtime.
    /// </summary>
    void Shutdown() noexcept;

    /// <summary>
    /// Gets the underlying rc_client object for directly calling into rcheevos.
    /// </summary>
    rc_client_t* GetClient() const noexcept { return m_pClient.get(); }

    void BeginLoginWithToken(const std::string& sUsername, const std::string& sApiToken,
                             rc_client_callback_t fCallback, void* pCallbackData);
    void BeginLoginWithPassword(const std::string& sUsername, const std::string& sPassword,
                                rc_client_callback_t fCallback, void* pCallbackData);

    void BeginLoadGame(const std::string& sHash, unsigned id,
                       rc_client_callback_t fCallback, void* pCallbackData);
    void UnloadGame() noexcept;

    /// <summary>
    /// Syncs data in Assets to rc_client.
    /// </summary>
    void SyncAssets();

    /// <summary>
    /// Clears all active achievements/leaderboards/rich presence from the runtime.
    /// </summary>
    void ResetRuntime() noexcept;

    /// <summary>
    /// Gets the raw trigger for the achievement.
    /// </summary>
    rc_trigger_t* GetAchievementTrigger(ra::AchievementID nId) const noexcept;
    const rc_client_achievement_info_t* GetPublishedAchievementInfo(ra::AchievementID nId) const;

    static std::string GetAchievementBadge(const rc_client_achievement_t& pAchievement);

    void GetSubsets(std::vector<std::pair<uint32_t, std::wstring>>& vSubsets) const;

    void RaiseClientEvent(rc_client_achievement_info_t& pAchievement, uint32_t nEventType) const noexcept;

    void UpdateActiveAchievements() noexcept;
    void UpdateActiveLeaderboards() noexcept;

    /// <summary>
    /// Gets the raw definition for the leaderboard.
    /// </summary>
    rc_lboard_t* GetLeaderboardDefinition(ra::LeaderboardID nId) const noexcept;
    const rc_client_leaderboard_info_t* GetPublishedLeaderboardInfo(ra::LeaderboardID nId) const;

    void ReleaseLeaderboardTracker(ra::LeaderboardID nId) noexcept;

    /// <summary>
    /// Specifies the rich presence to process each frame.
    /// </summary>
    /// <remarks>Only updates the memrefs each frame (for deltas), the script is not processed here.</remarks>
    /// <returns><c>true</c> if the rich presence was activated, <c>false</c> if there was an error.</returns>
    bool ActivateRichPresence(const std::string& sScript);

    /// <summary>
    /// Gets whether or not the loaded game has a rich presence script.
    /// </summary>
    bool HasRichPresence() const noexcept;

    /// <summary>
    /// Gets the current rich presence display string.
    /// </summary>
    std::wstring GetRichPresenceDisplayString() const;

    void InvalidateAddress(ra::ByteAddress nAddress) noexcept;

    /// <summary>
    /// Processes all active achievements for the current frame.
    /// </summary>
    void DoFrame();

    /// <summary>
    /// Processes stuff not related to a frame.
    /// </summary>
    void Idle() const noexcept;

    /// <summary>
    /// Loads HitCount data for active achievements from a save state file.
    /// </summary>
    /// <param name="sLoadStateFilename">The name of the save state file.</param>
    /// <returns><c>true</c> if the achievement HitCounts were modified, <c>false</c> if not.</returns>
    bool LoadProgressFromFile(const char* sLoadStateFilename);

    /// <summary>
    /// Loads HitCount data for active achievements from a buffer.
    /// </summary>
    /// <param name="pBuffer">The buffer to read from.</param>
    /// <returns><c>true</c> if the achievement HitCounts were modified, <c>false</c> if not.</returns>
    bool LoadProgressFromBuffer(const uint8_t* pBuffer);

    /// <summary>
    /// Writes HitCount data for active achievements to a save state file.
    /// </summary>
    /// <param name="sLoadStateFilename">The name of the save state file.</param>
    void SaveProgressToFile(const char* sSaveStateFilename) const;

    /// <summary>
    /// Writes HitCount data for active achievements to a buffer.
    /// </summary>
    /// <param name="pBuffer">The buffer to write to.</param>
    /// <param name="nBufferSize">The size of the buffer to write to.</param>
    /// <returns>
    /// The numberof bytes required to capture the HitCount data (may be larger than 
    /// nBufferSize - in which case the caller should allocate the specified amount
    /// and call again.
    /// </returns>
    int SaveProgressToBuffer(uint8_t* pBuffer, int nBufferSize) const;

    /// <summary>
    /// Gets whether achievement processing is temporarily suspended.
    /// </summary>
    bool IsPaused() const noexcept { return m_bPaused; }

    /// <summary>
    /// Sets whether achievement processing should be temporarily suspended.
    /// </summary>
    void SetPaused(bool bValue) noexcept { m_bPaused = bValue; }

    typedef void (*AsyncServerCallCallback)(const rc_api_server_response_t& pResponse, void* pCallbackData);
    /// <summary>
    /// Makes an asynchronous rc_api server call
    /// </summary>
    void AsyncServerCall(const rc_api_request_t* pRequest, AsyncServerCallCallback fCallback, void* pCallbackData) const;

    void QueueMemoryRead(std::function<void()>&& fCallback) const;
    bool IsOnDoFrameThread() const { return m_hDoFrameThread && GetCurrentThreadId() == m_hDoFrameThread; }

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

    void AttachMemory(void* pMemory);
    bool DetachMemory(void* pMemory) noexcept;

private:
    bool m_bPaused = false;
    DWORD m_hDoFrameThread = 0;
    std::unique_ptr<rc_client_t> m_pClient;

    class ClientSynchronizer;
    std::unique_ptr<ClientSynchronizer> m_pClientSynchronizer;

    int m_nRichPresenceParseResult = RC_OK;
    int m_nRichPresenceErrorLine = 0;

    static void LogMessage(const char* sMessage, const rc_client_t* pClient);
    static uint32_t ReadMemory(uint32_t nAddress, uint8_t* pBuffer, uint32_t nBytes, rc_client_t* pClient);
    static void ServerCallAsync(const rc_api_request_t* pRequest, rc_client_server_callback_t fCallback,
                                void* pCallbackData, rc_client_t* pClient);
    static void EventHandler(const rc_client_event_t* pEvent, rc_client_t* pClient);

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

    friend class AchievementRuntimeExports;

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

    rc_client_async_handle_t* BeginIdentifyAndLoadGame(uint32_t console_id, const char* file_path,
                                                       const uint8_t* data, size_t data_size,
                                                       CallbackWrapper* pCallbackWrapper) noexcept;

    rc_client_async_handle_t* BeginIdentifyAndChangeMedia(const char* file_path, const uint8_t* data, size_t data_size,
                                                          CallbackWrapper* pCallbackWrapper) noexcept;
    rc_client_async_handle_t* BeginChangeMedia(const char* sHash, CallbackWrapper* pCallbackWrapper) noexcept;
    static void ChangeMediaCallback(int nResult, const char* sErrorMessage, rc_client_t*, void* pUserdata);

    static void PostProcessGameDataResponse(const rc_api_server_response_t* server_response,
                                            struct rc_api_fetch_game_sets_response_t* game_data_response,
                                            rc_client_t* client, void* pUserdata);
};

} // namespace services
} // namespace ra

extern "C" unsigned int rc_peek_callback(unsigned int nAddress, unsigned int nBytes, _UNUSED void* pData);

#endif // !RA_SERVICES_ACHIEVEMENT_RUNTIME_HH
