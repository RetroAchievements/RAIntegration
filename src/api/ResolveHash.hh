#ifndef RA_API_RESOLVE_HASH_HH
#define RA_API_RESOLVE_HASH_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class ResolveHash
{
public:
    static constexpr const char* const Name() noexcept { return "ResolveHash"; }

    struct Response : ApiResponseBase
    {
        unsigned int GameId{0U};
    };

    struct Request : ApiRequestBase
    {
        std::string Hash;

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

#endif // !RA_API_RESOLVE_HASH_HH
