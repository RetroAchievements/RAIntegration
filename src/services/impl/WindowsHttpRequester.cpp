#include "WindowsHttpRequester.hh"

#include "RA_Defs.h"

#include "RA_Log.h"
#include "RA_StringUtils.h"

#include "services\impl\StringTextWriter.hh"

#include <winhttp.h>

namespace ra {
namespace services {
namespace impl {

static bool ReadIntoString(const HINTERNET hRequest, std::string& sBuffer, DWORD& nStatusCode)
{
    DWORD dwSize = sizeof(DWORD);
    DWORD nContentLength = 0;
    if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &nContentLength, &dwSize, WINHTTP_NO_HEADER_INDEX))
        return false;

    // allocate enough space in the string for the whole content
    sBuffer.resize(nContentLength + 1); // reserve space for null terminator
    sBuffer[nContentLength] = '\0'; // set null terminator, will fill remaining portion of buffer
    sBuffer.resize(nContentLength);

    DWORD nAvailableBytes = 0U;
    WinHttpQueryDataAvailable(hRequest, &nAvailableBytes);

    DWORD nInsertAt = 0U;
    while (nAvailableBytes > 0)
    {
        DWORD nBytesFetched = 0U;
        const DWORD nBytesToRead = sBuffer.capacity() - nInsertAt;
        if (WinHttpReadData(hRequest, &sBuffer[nInsertAt], nBytesToRead, &nBytesFetched))
        {
            nInsertAt += nBytesFetched;
        }
        else
        {
            if (nStatusCode == 200)
                nStatusCode = GetLastError();

            break;
        }

        WinHttpQueryDataAvailable(hRequest, &nAvailableBytes);
    }

    return true;
}

static void ReadIntoWriter(const HINTERNET hRequest, ra::services::TextWriter& pContentWriter, DWORD& nStatusCode)
{
    DWORD nAvailableBytes = 0;
    WinHttpQueryDataAvailable(hRequest, &nAvailableBytes);

    // writing to non string buffer - do up to 4K at a time
    std::string sBuffer;

    while (nAvailableBytes > 0)
    {
        const DWORD nBytesToRead = (nAvailableBytes > 4096) ? 4096 : nAvailableBytes;
        sBuffer.resize(nBytesToRead);

        DWORD nBytesFetched = 0U;
        if (WinHttpReadData(hRequest, sBuffer.data(), nBytesToRead, &nBytesFetched))
        {
            sBuffer.resize(nBytesFetched);
            pContentWriter.Write(sBuffer);
        }
        else
        {
            if (nStatusCode == 200)
                nStatusCode = GetLastError();

            break;
        }

        WinHttpQueryDataAvailable(hRequest, &nAvailableBytes);
    }
}

unsigned int WindowsHttpRequester::Request(const Http::Request& pRequest, TextWriter& pContentWriter) const
{
    DWORD nStatusCode = 0;

    // obtain a session handle.
    HINTERNET hSession = WinHttpOpen(m_sUserAgent.c_str(),
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);

    if (hSession == nullptr)
    {
        nStatusCode = GetLastError();
    }
    else
    {
        INTERNET_PORT nPort = INTERNET_DEFAULT_HTTP_PORT;

        auto sUrl = pRequest.GetUrl();
        if (_strnicmp(sUrl.c_str(), "http://", 7) == 0)
        {
            sUrl.erase(0, 7);
        }
        else if (_strnicmp(sUrl.c_str(), "https://", 8) == 0)
        {
            sUrl.erase(0, 8);
            nPort = INTERNET_DEFAULT_HTTPS_PORT;
        }

        std::string sPath;
        const auto nIndex = sUrl.find('/');
        if (nIndex != std::string::npos)
        {
            sPath.assign(sUrl, nIndex + 1, std::string::npos);
            sUrl.resize(nIndex);
        }

        // specify the server
        auto sHostName = ra::Widen(sUrl);
        HINTERNET hConnect = WinHttpConnect(hSession, sHostName.c_str(), INTERNET_DEFAULT_HTTP_PORT, 0);

        if (hConnect == nullptr)
        {
            nStatusCode = GetLastError();
        }
        else
        {
            // merge query parameters onto sPath.
            std::string sQueryString = pRequest.GetQueryString();
            if (!sQueryString.empty())
            {
                sPath.push_back('?');
                sPath += sQueryString;
            }

            auto sPostData = pRequest.GetPostData();

            // open the connection
            auto sPathWide = ra::Widen(sPath);
            HINTERNET hRequest = WinHttpOpenRequest(hConnect,
                sPostData.empty() ? L"GET" : L"POST",
                sPathWide.c_str(),
                nullptr,
                WINHTTP_NO_REFERER,
                WINHTTP_DEFAULT_ACCEPT_TYPES,
                0);

            if (hRequest == nullptr)
            {
                nStatusCode = GetLastError();
            }
            else
            {
                std::wstring sHeaders;
                sHeaders += L"Content-Type: ";
                sHeaders += ra::Widen(pRequest.GetContentType());

                BOOL bResults;

                // send the request
                if (sPostData.empty())
                {
                    bResults = WinHttpSendRequest(hRequest,
                        sHeaders.c_str(), sHeaders.length(),
                        WINHTTP_NO_REQUEST_DATA,
                        0, 0,
                        0);
                }
                else
                {
                    bResults = WinHttpSendRequest(hRequest,
                        sHeaders.c_str(), sHeaders.length(),
                        static_cast<LPVOID>(sPostData.data()),
                        sPostData.length(), sPostData.length(),
                        0);
                }

                if (!bResults || !WinHttpReceiveResponse(hRequest, nullptr))
                {
                    nStatusCode = GetLastError();
                }
                else
                {
                    // get the http status code
                    DWORD dwSize = sizeof(DWORD);
                    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &nStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);

                    // read the response
                    auto* pStringWriter = dynamic_cast<StringTextWriter*>(&pContentWriter);
                    if (pStringWriter != nullptr)
                    {
                        // optimized path for writing to string buffer
                        if (!ReadIntoString(hRequest, pStringWriter->GetString(), nStatusCode))
                        {
                            // could not use optimization, fall back to buffered reader
                            ReadIntoWriter(hRequest, pContentWriter, nStatusCode);
                        }
                    }
                    else
                    {
                        ReadIntoWriter(hRequest, pContentWriter, nStatusCode);
                    }
                }

                WinHttpCloseHandle(hRequest);
            }

            WinHttpCloseHandle(hConnect);
        }

        WinHttpCloseHandle(hSession);
    }

    return nStatusCode;
}


} // namespace impl
} // namespace services
} // namespace ra
