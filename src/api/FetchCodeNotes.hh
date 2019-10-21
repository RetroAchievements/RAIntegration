#ifndef RA_API_FETCH_CODE_NOTES_HH
#define RA_API_FETCH_CODE_NOTES_HH
#pragma once

#include "ApiCall.hh"

#include "data\Types.hh"

namespace ra {
namespace api {

class FetchCodeNotes
{
public:
    static constexpr const char* const Name() noexcept { return "FetchCodeNotes"; }

    struct Response : ApiResponseBase
    {
        struct CodeNote
        {
            ra::ByteAddress Address{ 0U };
            std::wstring Note;
            std::string Author;
        };
        std::vector<CodeNote> Notes;
    };

    struct Request : ApiRequestBase
    {
        unsigned int GameId{ 0U };

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

#endif // !RA_API_FETCH_CODE_NOTES_HH
