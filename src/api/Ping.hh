#ifndef RA_API_PING_HH
#define RA_API_PING_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class Ping
{
public:
    static constexpr const char* const Name() noexcept { return "Ping"; }

    struct Response : ApiResponseBase
    {
    };

    struct Request : ApiRequestBase
    {
        unsigned int GameId;
        std::wstring CurrentActivity;

        using Callback = std::function<void(const Response& response)>;

        Response Call() const;

        void CallAsync(Callback&& callback) const
        {
            ApiRequestBase::CallAsync<Request, Callback>(*this, std::move(callback));
        }
    };
};

} // namespace api
} // namespace ra

#endif // !RA_API_PING_HH
