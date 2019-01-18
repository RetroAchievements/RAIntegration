#ifndef RA_API_LATESTCLIENT_HH
#define RA_API_LATESTCLIENT_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class LatestClient
{
public:
    static constexpr const char* const Name() noexcept { return "LatestClient"; }

    struct Response : ApiResponseBase
    {
        std::string LatestVersion;
    };

    struct Request : ApiRequestBase
    {
        unsigned int EmulatorId;

        using Callback = std::function<void(const Response& response)>;

        Response Call() const noexcept;

        void CallAsync(Callback&& callback) const
        {
            ApiRequestBase::CallAsync<Request, Callback>(*this, std::move(callback));
        }
    };
};

} // namespace api
} // namespace ra

#endif // !RA_API_LATESTCLIENT_HH
