#ifndef RA_SERVICES_HTTP_H
#define RA_SERVICES_HTTP_H
#pragma once

#include <functional>

namespace ra {
namespace services {

class Http
{
public:
    enum class StatusCode
    {
        OK = 200,
        NotFound = 404,
    };

    class Response
    {
    public:
        explicit Response(Http::StatusCode nStatusCode, const std::string& sResponse)
            : m_nStatusCode(nStatusCode), m_sResponse(sResponse)
        {
        }

        ~Response() noexcept = default;
        Response(const Response&) noexcept = default;
        Response& operator=(const Response&) noexcept = delete;
        Response(Response&&) noexcept = default;
        Response& operator=(Response&&) noexcept = delete;

        /// <summary>
        /// Gets the HTTP status code returned from the server.
        /// </summary>
        Http::StatusCode StatusCode() const noexcept { return m_nStatusCode; }

        /// <summary>
        /// Gets the content returned from the server.
        /// </summary>
        const std::string& Content() const noexcept { return m_sResponse; }

    private:
        Http::StatusCode m_nStatusCode{ 0 };
        std::string m_sResponse;
    };

    class Request
    {
    public:
        explicit Request(const std::string& sUrl)
        {
            const auto nIndex = sUrl.find('?');
            if (nIndex == std::string::npos)
            {
                m_sUrl = sUrl;
            }
            else
            {
                m_sUrl.assign(sUrl, 0, nIndex);
                m_sQueryString.assign(sUrl, nIndex + 1, std::string::npos);
            }
        }

        ~Request() noexcept = default;
        Request(const Request&) = default;
        Request& operator=(const Request&) noexcept = delete;
        Request(Request&&) noexcept = default;
        Request& operator=(Request&&) noexcept = delete;

        /// <summary>
        /// Gets the URL to request data from.
        /// </summary>
        const std::string& GetUrl() const noexcept { return m_sUrl; }

        /// <summary>
        /// Adds a parameter to the query string.
        /// </summary>
        /// <param name="sParam">The parameter to add.</param>
        /// <param name="sValue">The value of the parameter.</param>
        void AddQueryParm(const std::string& sParam, const std::string& sValue)
        {
            if (!m_sQueryString.empty())
                m_sQueryString.push_back('&');

            m_sQueryString += sParam;
            m_sQueryString.push_back('=');
            UrlEncodeAppend(m_sQueryString, sValue);
        }

        /// <summary>
        /// Gets the query string.
        /// </summary>
        const std::string& GetQueryString() const noexcept { return m_sQueryString; }

        /// <summary>
        /// Sets the POST data.
        /// </summary>
        /// <remarks>If not empty, POST request will be made, otherwise GET request wil be made.</remarks>
        void SetPostData(const std::string& sValue) { m_sPostData = sValue; }

        /// <summary>
        /// Gets the POST data.
        /// </summary>
        const std::string& GetPostData() const noexcept { return m_sPostData; }

        /// <summary>
        /// Specifies the Content-Type to send. Default: "application/x-www-form-urlencoded"
        /// </summary>
        void SetContentType(const std::string& sValue) { m_sContentType = sValue; }

        /// <summary>
        /// Gets the Content-Type to send. Default: "application/x-www-form-urlencoded"
        /// </summary>
        const std::string& GetContentType() const noexcept { return m_sContentType; }

        using Callback = std::function<void(const Response& response)>;

        /// <summary>
        /// Calls this server and waits for the response.
        /// </summary>
        Response Call() const;

        /// <summary>
        /// Calls the server asynchronously. The provided callback will be called when the response is received.
        /// </summary>
        void CallAsync(Callback&& fCallback) const;

        /// <summary>
        /// Calls this server and waits for the response.
        /// </summary>
        /// <param name="sFilename">The path to the file where the response should be written.</param>
        /// <remarks>Response.Content() will be empty.</remarks>
        Response Download(const std::wstring& sFilename) const;

        /// <summary>
        /// Calls the server asynchronously. The provided callback will be called when the response is received.
        /// </summary>
        /// <param name="sFilename">The path to the file where the response should be written.</param>
        /// <remarks>Response.Content() will be empty.</remarks>
        void DownloadAsync(const std::wstring& sFilename, Callback&& fCallback) const;

    private:
        std::string m_sUrl;
        std::string m_sQueryString;
        std::string m_sPostData;
        std::string m_sContentType{ "application/x-www-form-urlencoded" };
    };
    
    /// <summary>
    /// URL encodes a string.
    /// </summary>
    /// <param name="sInput">The string to encode.</param>
    /// <returns>The encoded string.</returns>
    static std::string UrlEncode(const std::string& sInput);

    /// <summary>
    /// URL encodes a string and appends it to another.
    /// </summary>
    /// <param name="sOutput">The string to append to.</param>
    /// <param name="sInput">The string to encode.</param>
    static void UrlEncodeAppend(std::string& sOutput, const std::string& sInput);
};

} // namespace services
} // namespace ra

#endif !RA_SERVICES_HTTP_H
