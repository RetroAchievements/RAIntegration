#ifndef RA_API_UPDATE_CODE_NOTE_HH
#define RA_API_UPDATE_CODE_NOTE_HH
#pragma once

#include "ApiCall.hh"

#include "data\Types.hh"

namespace ra {
namespace api {

class UpdateCodeNote
{
public:
    static constexpr const char* const Name() noexcept { return "UpdateCodeNote"; }

    struct Response : ApiResponseBase
    {

    };

    struct Request : ApiRequestBase
    {
        unsigned int GameId{ 0U };
        ra::data::ByteAddress Address{ 0U };
        std::wstring Note;

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

#endif // !RA_API_UPDATE_CODE_NOTE_HH
