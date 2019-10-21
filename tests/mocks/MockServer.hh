#ifndef RA_SERVICES_MOCK_SERVER_HH
#define RA_SERVICES_MOCK_SERVER_HH
#pragma once

#include "RA_StringUtils.h"

#include "api\IServer.hh"
#include "services\ServiceLocator.hh"

#include "CppUnitTest.h"

namespace ra {
namespace api {
namespace mocks {

class MockServer : public IServer
{
public:
    MockServer() noexcept : m_Override(this) {}

    const char* Name() const noexcept override { return "MockServer"; }

    /// <summary>
    /// Registers a callback method to handle the associated request type.
    /// </summary>
    /// <remarks>
    /// Callback should return <c>true</c> if it populated the response, <c>false</c> to return the default response
    /// (unsupported)
    /// </remarks>
    template<typename TApi>
    void HandleRequest(std::function<bool(const typename TApi::Request&, typename TApi::Response&)>&& fHandler)
    {
        m_mHandlers.insert_or_assign(
            std::string(TApi::Name()),
            [fHandler = std::move(fHandler)](const void* restrict pRequest, void* restrict pResponse) 
        {
                const gsl::not_null<const typename TApi::Request* const> pTRequest{
                    gsl::make_not_null(static_cast<const typename TApi::Request*>(pRequest))};
                const gsl::not_null<typename TApi::Response* const> pTResponse{
                    gsl::make_not_null(static_cast<typename TApi::Response*>(pResponse))};
                return fHandler(*pTRequest, *pTResponse);
        });
    }

    /// <summary>
    /// Throws an exception if the associated request type is invoked.
    /// </summary>
    template<typename TApi>
    void ExpectUncalled()
    {
        HandleRequest<TApi>([](const typename TApi::Request&, typename TApi::Response&) -> bool
        {
            Assert::Fail(ra::StringPrintf(L"%s should not have been invoked", TApi::Name()).c_str());
        });
    }

    // === user functions ===

    Login::Response Login(const Login::Request& request) override
    {
        return HandleRequest<ra::api::Login>(request);
    }

    Logout::Response Logout(const Logout::Request& request) override
    {
        return HandleRequest<ra::api::Logout>(request);
    }

    StartSession::Response StartSession(const StartSession::Request& request) override
    {
        return HandleRequest<ra::api::StartSession>(request);
    }

    Ping::Response Ping(const Ping::Request& request) override
    {
        return HandleRequest<ra::api::Ping>(request);
    }

    FetchUserUnlocks::Response FetchUserUnlocks(const FetchUserUnlocks::Request& request) override
    {
        return HandleRequest<ra::api::FetchUserUnlocks>(request);
    }

    AwardAchievement::Response AwardAchievement(const AwardAchievement::Request& request) override
    {
        return HandleRequest<ra::api::AwardAchievement>(request);
    }

    SubmitLeaderboardEntry::Response SubmitLeaderboardEntry(const SubmitLeaderboardEntry::Request& request) override
    {
        return HandleRequest<ra::api::SubmitLeaderboardEntry>(request);
    }

    FetchUserFriends::Response FetchUserFriends(const FetchUserFriends::Request& request) override
    {
        return HandleRequest<ra::api::FetchUserFriends>(request);
    }

    // === game functions ===

    ResolveHash::Response ResolveHash(const ResolveHash::Request& request) override
    {
        return HandleRequest<ra::api::ResolveHash>(request);
    }

    FetchGameData::Response FetchGameData(const FetchGameData::Request& request) override
    {
        return HandleRequest<ra::api::FetchGameData>(request);
    }

    FetchCodeNotes::Response FetchCodeNotes(const FetchCodeNotes::Request& request) override
    {
        return HandleRequest<ra::api::FetchCodeNotes>(request);
    }

    UpdateCodeNote::Response UpdateCodeNote(const UpdateCodeNote::Request& request) override
    {
        return HandleRequest<ra::api::UpdateCodeNote>(request);
    }

    DeleteCodeNote::Response DeleteCodeNote(const DeleteCodeNote::Request& request) override
    {
        return HandleRequest<ra::api::DeleteCodeNote>(request);
    }

    UpdateAchievement::Response UpdateAchievement(const UpdateAchievement::Request& request) override
    {
        return HandleRequest<ra::api::UpdateAchievement>(request);
    }

    FetchAchievementInfo::Response FetchAchievementInfo(const FetchAchievementInfo::Request& request) override
    {
        return HandleRequest<ra::api::FetchAchievementInfo>(request);
    }

    FetchLeaderboardInfo::Response FetchLeaderboardInfo(const FetchLeaderboardInfo::Request& request) override
    {
        return HandleRequest<ra::api::FetchLeaderboardInfo>(request);
    }

    // === other functions ===

    LatestClient::Response LatestClient(const LatestClient::Request& request) override
    {
        return HandleRequest<ra::api::LatestClient>(request);
    }

    FetchGamesList::Response FetchGamesList(const FetchGamesList::Request& request) override
    {
        return HandleRequest<ra::api::FetchGamesList>(request);
    }

    SubmitNewTitle::Response SubmitNewTitle(const SubmitNewTitle::Request& request) override
    {
        return HandleRequest<ra::api::SubmitNewTitle>(request);
    }

    SubmitTicket::Response SubmitTicket(const SubmitTicket::Request& request) override
    {
        return HandleRequest<ra::api::SubmitTicket>(request);
    }

    FetchBadgeIds::Response FetchBadgeIds(const FetchBadgeIds::Request& request) override
    {
        return HandleRequest<ra::api::FetchBadgeIds>(request);
    }

    UploadBadge::Response UploadBadge(const UploadBadge::Request& request) override
    {
        return HandleRequest<ra::api::UploadBadge>(request);
    }

protected:
    template<typename TApi>
    inline auto HandleRequest(const ApiRequestBase& pRequest) const
    {
        typename TApi::Response response;
        std::string sApiName(TApi::Name());

        const auto pIter = m_mHandlers.find(sApiName);
        if (pIter != m_mHandlers.end())
        {
            if (pIter->second(&pRequest, &response))
                return response;
        }

        response.Result = ApiResult::Unsupported;
        response.ErrorMessage = ra::StringPrintf("%s is not supported by %s", sApiName, Name());

        return response;
    }

private:
    std::unordered_map<std::string, std::function<bool(const void* restrict, void* restrict)>> m_mHandlers;

    ra::services::ServiceLocator::ServiceOverride<ra::api::IServer> m_Override;
};

} // namespace mocks
} // namespace api
} // namespace ra

#endif // !RA_SERVICES_MOCK_SERVER_HH
