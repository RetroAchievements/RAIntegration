#ifndef RA_SERVICES_MOCK_SERVER_HH
#define RA_SERVICES_MOCK_SERVER_HH
#pragma once

#include "api\IServer.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace api {
namespace mocks {

class MockServer : public IServer
{
public:
    MockServer() noexcept
        : m_Override(this)
    {
    }

    const char* Name() const noexcept override { return "MockServer"; }

    /// <summary>
    /// Registers a callback method to handle the associated request type.
    /// </summary>
    /// <remarks>
    /// Callback should return <c>true</c> if it populated the response, <c>false</c> to return the default response (unsupported)
    /// </remarks>
    template<typename TApi>
    void HandleRequest(std::function<bool(const typename TApi::Request&, typename TApi::Response&)>&& fHandler)
    {
        m_mHandlers.insert_or_assign(std::string(TApi::Name()), [fHandler=std::move(fHandler)](void* pRequest, void* pResponse)
        {
            const TApi::Request* pTRequest = reinterpret_cast<const TApi::Request*>(pRequest);
            auto pTResponse = reinterpret_cast<typename TApi::Response*>(pResponse);
            return fHandler(*pTRequest, *pTResponse);
        });
    }

    // === user functions ===

    Login::Response Login(const Login::Request& request) noexcept override
    {
        return HandleRequest<ra::api::Login>(request);
    }

    Logout::Response Logout(const Logout::Request& request) noexcept override
    {
        return HandleRequest<ra::api::Logout>(request);
    }

    StartSession::Response StartSession(const StartSession::Request& request) noexcept override
    {
        return HandleRequest<ra::api::StartSession>(request);
    }

    Ping::Response Ping(const Ping::Request& request) noexcept override
    {
        return HandleRequest<ra::api::Ping>(request);
    }

    ResolveHash::Response ResolveHash(const ResolveHash::Request& request) noexcept override
    {
        return HandleRequest<ra::api::ResolveHash>(request);
    }

protected:
    template<typename TApi>
    GSL_SUPPRESS(f.6)
    inline auto HandleRequest(const ApiRequestBase& pRequest) const noexcept
    {
        typename TApi::Response response;
        std::string sApiName(TApi::Name());

        auto pIter = m_mHandlers.find(sApiName);
        if (pIter != m_mHandlers.end())
        {
            if (pIter->second(reinterpret_cast<void*>(const_cast<ApiRequestBase*>(&pRequest)), reinterpret_cast<void*>(&response)))
                return std::move(response);
        }

        response.Result = ApiResult::Unsupported;
        response.ErrorMessage = sApiName + " is not supported by " + Name();

        return std::move(response);
    }

private:
    std::unordered_map<std::string, std::function<bool(void*, void*)>> m_mHandlers;

    ra::services::ServiceLocator::ServiceOverride<ra::api::IServer> m_Override;
};

} // namespace mocks
} // namespace api
} // namespace ra

#endif // !RA_SERVICES_MOCK_SERVER_HH
