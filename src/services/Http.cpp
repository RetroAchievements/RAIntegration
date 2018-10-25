#include "Http.hh"

#include "services\IFileSystem.hh"
#include "services\IHttpRequester.hh"
#include "services\IThreadPool.hh"
#include "services\ServiceLocator.hh"

#include "services\impl\StringTextWriter.hh"

namespace ra {
namespace services {

Http::Response Http::Request::Call() const
{
    std::string sResponse;
    ra::services::impl::StringTextWriter pWriter(sResponse);

    auto& pHttpRequester = ra::services::ServiceLocator::Get<ra::services::IHttpRequester>();
    const auto nStatusCode = pHttpRequester.Request(*this, pWriter);

    return Response(nStatusCode, std::move(sResponse));
}

void Http::Request::CallAsync(Callback&& fCallback) const
{
    auto& pThreadPool = ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>();
    pThreadPool.RunAsync([request=*this, f = std::move(fCallback)]() {
        auto response = request.Call();
        f(response);
    });
}

Http::Response Http::Request::Download(const std::wstring& sFilename) const
{
    auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
    auto pFile = pFileSystem.CreateTextFile(sFilename);

    auto& pHttpRequester = ra::services::ServiceLocator::Get<ra::services::IHttpRequester>();
    const auto nStatusCode = pHttpRequester.Request(*this, *pFile);

    return Response(nStatusCode, "");
}

void Http::Request::DownloadAsync(const std::wstring& sFilename, Callback&& fCallback) const
{
    auto& pThreadPool = ra::services::ServiceLocator::GetMutable<ra::services::IThreadPool>();
    pThreadPool.RunAsync([request=*this, sFilename, f = std::move(fCallback)]() {
        auto response = request.Download(sFilename);
        f(response);
    });
}

std::string Http::UrlEncode(const std::string& sInput)
{
    std::string sEncoded;
    UrlEncodeAppend(sEncoded, sInput);
    return sEncoded;
}

void Http::UrlEncodeAppend(std::string& sOutput, const std::string& sInput)
{
    static const char hex[] = "0123456789ABCDEF";

    const auto nNeeded = sOutput.length() + sInput.length();
    if (sOutput.capacity() < nNeeded)
        sOutput.reserve(nNeeded);

    for (auto c : sInput)
    {
        // unreserved characters per RFC3986 (section 2.3)
        if (isalnum(c) || c == '-' || c == '.' || c == '_' || c == '~')
        {
            sOutput.push_back(c);
        }
        else if (c == ' ')
        {
            sOutput.push_back('+');
        }
        else
        {
            sOutput.push_back('%');
            sOutput.push_back(hex[c / 16]);
            sOutput.push_back(hex[c % 16]);
        }
    }
}

} // namespace services
} // namespace ra
