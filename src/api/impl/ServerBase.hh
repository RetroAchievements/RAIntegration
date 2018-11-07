#pragma once

#include "api/IServer.hh"

#include "ra_fwd.h"

#include <string>

namespace ra {
namespace api {
namespace impl {

class ServerBase : public IServer
{
public:
    // === user functions ===

    Login::Response Login(_UNUSED const Login::Request& request) noexcept override
    {
        return UnsupportedApi<Login::Response>(Login::Name());
    }

    Logout::Response Logout(_UNUSED const Logout::Request& request) noexcept override
    {
        return UnsupportedApi<Logout::Response>(Logout::Name());
    }

protected:
    template<typename TResponse>
    inline typename TResponse UnsupportedApi(const char* const apiName) const noexcept
    {
        static_assert(std::is_base_of<ApiResponseBase, TResponse>::value, "TResponse must derive from ApiResponseBase");

        TResponse response;
        response.Result = ApiResult::Unsupported;
        response.ErrorMessage = std::string(apiName) + " is not supported by " + Name();
        return std::move(response);
    }
};

} // namespace impl
} // namespace api
} // namespace ra
