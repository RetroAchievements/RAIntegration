#ifndef RA_API_DELETE_CODE_NOTE_HH
#define RA_API_DELETE_CODE_NOTE_HH
#pragma once

#include "ApiCall.hh"

#include "data\Types.hh"

namespace ra {
namespace api {

class DeleteCodeNote
{
public:
    static constexpr const char* const Name() noexcept { return "DeleteCodeNote"; }

    struct Response : ApiResponseBase
    {

    };

    struct Request : ApiRequestBase
    {
        unsigned int GameId{ 0U };
        ra::ByteAddress Address{ 0U };

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

#endif // !RA_API_DELETE_CODE_NOTE_HH
