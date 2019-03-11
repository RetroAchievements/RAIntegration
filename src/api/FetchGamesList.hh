#ifndef RA_API_FETCH_GAMES_LIST_HH
#define RA_API_FETCH_GAMES_LIST_HH
#pragma once

#include "ApiCall.hh"

namespace ra {
namespace api {

class FetchGamesList
{
public:
    static constexpr const char* const Name() noexcept { return "FetchGamesList"; }

    struct Response : ApiResponseBase
    {
        struct Game
        {
            Game(unsigned int nId, std::wstring&& sName) : Id(nId), Name(sName) {}

            unsigned int Id;
            std::wstring Name;
        };

        std::vector<Game> Games;
    };

    struct Request : ApiRequestBase
    {
        unsigned int ConsoleId{0U};

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

#endif // !RA_API_FETCH_GAMES_LIST_HH
