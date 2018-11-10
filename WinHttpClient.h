#pragma once

static const unsigned int INT_RETRYTIMES = 3;
static const int INT_BUFFERSIZE = (10 << 10) * 5; // 50 KB temporary buffer, double if it is not enough.

class WinHttpClient
{
public:
    inline WinHttpClient(const std::wstring &url);
    inline ~WinHttpClient(void);

    inline bool SendPost(bool disableAutoRedirect = false);
    inline std::wstring GetResponseHeader(void);
    inline std::wstring GetResponseContent(void);
    inline std::wstring GetResponseStatusCode(void);
    inline const BYTE*  GetRawResponseContent(void);
    inline unsigned int GetRawResponseReceivedContentLength(void);

    inline bool SetDataToSend(BYTE* data, unsigned int dataSize);
    inline bool SetRequestHeaders(const std::wstring &additionalRequestHeaders);

private:
    WinHttpClient(const WinHttpClient &other);
    WinHttpClient &operator =(const WinHttpClient &other) = delete;

    HINTERNET m_sessionHandle;
	std::wstring m_requestURL;
	std::wstring m_responseHeader;
	std::wstring m_responseContent;
    BYTE* m_pResponse;
    unsigned int m_responseByteCountReceived; // Up to 4GB.
    BYTE *m_pDataToSend;
    unsigned int m_dataToSendSize;
	std::wstring m_additionalRequestHeaders;
    DWORD m_dwLastError;
	std::wstring m_statusCode;
	std::wstring m_userAgent;
    unsigned int m_resolveTimeout;
    unsigned int m_connectTimeout;
    unsigned int m_sendTimeout;
    unsigned int m_receiveTimeout;
};

WinHttpClient::WinHttpClient(const std::wstring &url)
    : m_requestURL(url),
      m_sessionHandle(NULL),
      m_responseHeader(L""),
      m_responseContent(L""),
      m_pResponse(NULL),
      m_responseByteCountReceived(0),
      m_pDataToSend(NULL),
      m_dataToSendSize(0),
      m_dwLastError(ERROR_SUCCESS),
      m_statusCode(L""),
      m_userAgent(L""),
      m_resolveTimeout(0),
      m_connectTimeout(60000),
      m_sendTimeout(30000),
      m_receiveTimeout(30000)
{
}

WinHttpClient::~WinHttpClient(void)
{
    if (m_pResponse != NULL)
    {
        delete[] m_pResponse;
    }
    if (m_pDataToSend != NULL)
    {
        delete[] m_pDataToSend;
    }

    if (m_sessionHandle != NULL)
    {
        WinHttpCloseHandle(m_sessionHandle);
    }
}

bool WinHttpClient::SendPost(bool disableAutoRedirect)
{
    if (m_requestURL.size() <= 0)
    {
        m_dwLastError = ERROR_PATH_NOT_FOUND;
        return false;
    }

    bool bRetVal = true;

    if (m_sessionHandle == NULL)
    {
        m_sessionHandle = WinHttpOpen(m_userAgent.c_str(),
                                      WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                      WINHTTP_NO_PROXY_NAME, 
                                      WINHTTP_NO_PROXY_BYPASS,
                                      0);
        if (m_sessionHandle == NULL)
        {
            m_dwLastError = GetLastError();
            return false;
        }
    }

   WinHttpSetTimeouts(m_sessionHandle,
                      m_resolveTimeout,
                      m_connectTimeout,
                      m_sendTimeout,
                      m_receiveTimeout);

    wchar_t szHostName[MAX_PATH] = L"";
    wchar_t szURLPath[MAX_PATH * 5] = L"";
    URL_COMPONENTS urlComp;
    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.lpszHostName = szHostName;
    urlComp.dwHostNameLength = MAX_PATH;
    urlComp.lpszUrlPath = szURLPath;
    urlComp.dwUrlPathLength = MAX_PATH * 5;
    urlComp.dwSchemeLength = 1; // None zero

    if (WinHttpCrackUrl(m_requestURL.c_str(), m_requestURL.size(), 0, &urlComp))
    {
        HINTERNET hConnect = WinHttpConnect(m_sessionHandle, szHostName, urlComp.nPort, 0);

        if (hConnect != NULL)
        {
            DWORD dwOpenRequestFlag = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
            HINTERNET hRequest = NULL;
            hRequest = WinHttpOpenRequest(hConnect,
                                            L"POST",
                                            urlComp.lpszUrlPath,
                                            NULL,
                                            WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            dwOpenRequestFlag);
            if (hRequest != NULL)
            {
                if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) // If HTTPS, then client is very susceptible to invalid certificates. Easiest to accept anything for now.
                {
                    const DWORD options = SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_UNKNOWN_CA;
                    WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, (LPVOID)&options, sizeof(DWORD));
                }

                bool bGetReponseSucceed = false;
                unsigned int iRetryTimes = 0;

                while (!bGetReponseSucceed && iRetryTimes++ < INT_RETRYTIMES) // Retry for several times if fails.
                {
                    if (m_additionalRequestHeaders.size() > 0)
                    {
                        if (!WinHttpAddRequestHeaders(hRequest, m_additionalRequestHeaders.c_str(), m_additionalRequestHeaders.size(), WINHTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON))
                        {
                            m_dwLastError = GetLastError();
                        }
                    }

                    if (disableAutoRedirect)
                    {
                        DWORD dwDisableFeature = WINHTTP_DISABLE_REDIRECTS;
                        if (!::WinHttpSetOption(hRequest, WINHTTP_OPTION_DISABLE_FEATURE, &dwDisableFeature, sizeof(dwDisableFeature)))
                        {
                            m_dwLastError = GetLastError();
                        }
                    }
                    bool bSendRequestSucceed = false;
                    if (WinHttpSendRequest(hRequest,
                                           WINHTTP_NO_ADDITIONAL_HEADERS,
                                           0,
                                           WINHTTP_NO_REQUEST_DATA,
                                           0,
                                           0,
                                           NULL))
                    {
                        bSendRequestSucceed = true;
                    }
                    else
                    {
                        // Query the proxy information from IE setting and set the proxy if any.
                        WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxyConfig;
                        ZeroMemory(&proxyConfig, sizeof(proxyConfig));

                        if (WinHttpGetIEProxyConfigForCurrentUser(&proxyConfig))
                        {
                            if (proxyConfig.lpszAutoConfigUrl != NULL)
                            {
                                WINHTTP_AUTOPROXY_OPTIONS autoProxyOptions;
                                ZeroMemory(&autoProxyOptions, sizeof(autoProxyOptions));
                                autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT | WINHTTP_AUTOPROXY_CONFIG_URL;
                                autoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP;
                                autoProxyOptions.lpszAutoConfigUrl = proxyConfig.lpszAutoConfigUrl;
                                autoProxyOptions.fAutoLogonIfChallenged = TRUE;
                                autoProxyOptions.dwReserved = 0;
                                autoProxyOptions.lpvReserved = NULL;

                                WINHTTP_PROXY_INFO proxyInfo;
                                ZeroMemory(&proxyInfo, sizeof(proxyInfo));

                                if (WinHttpGetProxyForUrl(m_sessionHandle, m_requestURL.c_str(), &autoProxyOptions, &proxyInfo))
                                {
                                    if (WinHttpSetOption(hRequest, WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo)))
                                    {
                                        if (WinHttpSendRequest(hRequest,
                                                               WINHTTP_NO_ADDITIONAL_HEADERS,
                                                               0,
                                                               WINHTTP_NO_REQUEST_DATA,
                                                               0,
                                                               0,
                                                               NULL))
                                        {
                                            bSendRequestSucceed = true;
                                        }
                                    }
                                    if (proxyInfo.lpszProxy != NULL)
                                    {
                                        GlobalFree(proxyInfo.lpszProxy);
                                    }
                                    if (proxyInfo.lpszProxyBypass != NULL)
                                    {
                                        GlobalFree(proxyInfo.lpszProxyBypass);
                                    }
                                }
                                else
                                {
                                    m_dwLastError = GetLastError();
                                }
                            }
                            else if (proxyConfig.lpszProxy != NULL)
                            {
                                WINHTTP_PROXY_INFO proxyInfo;
                                ZeroMemory(&proxyInfo, sizeof(proxyInfo));
                                proxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
                                wchar_t szProxy[MAX_PATH] = L"";
                                wcscpy_s(szProxy, MAX_PATH, proxyConfig.lpszProxy);
                                proxyInfo.lpszProxy = szProxy;

                                if (proxyConfig.lpszProxyBypass != NULL)
                                {
                                    wchar_t szProxyBypass[MAX_PATH] = L"";
                                    wcscpy_s(szProxyBypass, MAX_PATH, proxyConfig.lpszProxyBypass);
                                    proxyInfo.lpszProxyBypass = szProxyBypass;
                                }

                                if (!WinHttpSetOption(hRequest, WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo)))
                                {
                                    m_dwLastError = GetLastError();
                                }
                            }

                            if (proxyConfig.lpszAutoConfigUrl != NULL)
                                GlobalFree(proxyConfig.lpszAutoConfigUrl);
                            if (proxyConfig.lpszProxy != NULL)
                                GlobalFree(proxyConfig.lpszProxy);
                            if (proxyConfig.lpszProxyBypass != NULL)
                                GlobalFree(proxyConfig.lpszProxyBypass);
                        }
                        else
                        {
                            m_dwLastError = GetLastError();
                        }
                    }
                    if (bSendRequestSucceed)
                    {
                        if (m_pDataToSend != NULL)
                        {
                            DWORD dwWritten = 0;
                            if (!WinHttpWriteData(hRequest,
                                                  m_pDataToSend,
                                                  m_dataToSendSize,
                                                  &dwWritten))
                            {
                                m_dwLastError = GetLastError();
                            }
                        }
                        if (WinHttpReceiveResponse(hRequest, NULL))
                        {
                            DWORD dwSize = 0;
                            BOOL bResult = FALSE;
                            bResult = WinHttpQueryHeaders(hRequest,
                                                          WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                                          WINHTTP_HEADER_NAME_BY_INDEX,
                                                          NULL,
                                                          &dwSize,
                                                          WINHTTP_NO_HEADER_INDEX);

                            if (bResult || (!bResult && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)))
                            {
                                wchar_t* szHeader = new wchar_t[dwSize];

                                if (szHeader != NULL)
                                {
                                    ZeroMemory(szHeader, dwSize * sizeof(wchar_t));

                                    if (WinHttpQueryHeaders(hRequest,
                                                            WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                                            WINHTTP_HEADER_NAME_BY_INDEX,
                                                            szHeader,
                                                            &dwSize,
                                                            WINHTTP_NO_HEADER_INDEX))
                                        m_responseHeader.assign(szHeader);

                                    delete[] szHeader;
                                }
                            }

                            dwSize = 0;
                            bResult = WinHttpQueryHeaders(hRequest,
                                                          WINHTTP_QUERY_STATUS_CODE,
                                                          WINHTTP_HEADER_NAME_BY_INDEX,
                                                          NULL,
                                                          &dwSize,
                                                          WINHTTP_NO_HEADER_INDEX);

                            if (bResult || (!bResult && (GetLastError() == ERROR_INSUFFICIENT_BUFFER)))
                            {
                                wchar_t* szStatusCode = new wchar_t[dwSize];

                                if (szStatusCode != NULL)
                                {
                                    ZeroMemory(szStatusCode, dwSize * sizeof(wchar_t));

                                    if (WinHttpQueryHeaders(hRequest,
                                                            WINHTTP_QUERY_STATUS_CODE,
                                                            WINHTTP_HEADER_NAME_BY_INDEX,
                                                            szStatusCode,
                                                            &dwSize,
                                                            WINHTTP_NO_HEADER_INDEX))
                                        m_statusCode = szStatusCode;

                                    delete[] szStatusCode;
                                }
                            }

                            unsigned int iMaxBufferSize = INT_BUFFERSIZE;
                            unsigned int iCurrentBufferSize = 0;

                            if (m_pResponse != NULL)
                            {
                                delete[] m_pResponse;
                                m_pResponse = NULL;
                            }

                            m_pResponse = new BYTE[iMaxBufferSize];

                            if (m_pResponse == NULL)
                            {
                                bRetVal = false;
                                break;
                            }

                            ZeroMemory(m_pResponse, iMaxBufferSize);

                            do
                            {
                                dwSize = 0;
                                if (WinHttpQueryDataAvailable(hRequest, &dwSize))
                                {
                                    BYTE *pResponse = new BYTE[dwSize + 1];
                                    if (pResponse != NULL)
                                    {
                                        ZeroMemory(pResponse, (dwSize + 1)*sizeof(BYTE));
                                        DWORD dwRead = 0;
                                        if (WinHttpReadData(hRequest,
                                                            pResponse,
                                                            dwSize,
                                                            &dwRead))
                                        {
                                            if (dwRead + iCurrentBufferSize > iMaxBufferSize)
                                            {
                                                BYTE *pOldBuffer = m_pResponse;
                                                m_pResponse = new BYTE[iMaxBufferSize * 2];
                                                if (m_pResponse == NULL)
                                                {
                                                    m_pResponse = pOldBuffer;
                                                    bRetVal = false;
                                                    break;
                                                }
                                                iMaxBufferSize *= 2;
                                                ZeroMemory(m_pResponse, iMaxBufferSize);
                                                CopyMemory(m_pResponse, pOldBuffer, iCurrentBufferSize);
                                                delete[] pOldBuffer;
                                            }
                                            CopyMemory(m_pResponse + iCurrentBufferSize, pResponse, dwRead);
                                            iCurrentBufferSize += dwRead;
                                        }
                                        delete[] pResponse;
                                    }
                                }
                                else
                                {
                                    m_dwLastError = GetLastError();
                                }
                            } while (dwSize > 0);

                            m_responseByteCountReceived = iCurrentBufferSize;
                            UINT codePage = CP_UTF8;
                            DWORD dwFlag = 0;

                            int iLength = MultiByteToWideChar(codePage,
                                                              dwFlag,
                                                              (LPCSTR)m_pResponse,
                                                              m_responseByteCountReceived + 1,
                                                              NULL, 
                                                              0);
                            if (iLength <= 0)
                            {
                                codePage = CP_ACP;
                                dwFlag  = MB_PRECOMPOSED;
                                iLength = MultiByteToWideChar(codePage,
                                                              dwFlag,
                                                              (LPCSTR)m_pResponse,
                                                              m_responseByteCountReceived + 1,
                                                              NULL,
                                                              0);
                            }
                            if (iLength > 0)
                            {
                                wchar_t* wideChar = new wchar_t[iLength];

                                if (wideChar != NULL)
                                {
                                    ZeroMemory(wideChar, iLength * sizeof(wchar_t));
                                    iLength = MultiByteToWideChar(codePage,
                                                                  dwFlag,
                                                                  (LPCSTR)m_pResponse,
                                                                  m_responseByteCountReceived + 1,
                                                                  wideChar, 
                                                                  iLength);
                                    if (iLength > 0)
                                       m_responseContent = wideChar;

                                    delete[] wideChar;
                                }
                            }

                            bGetReponseSucceed = true;
                        }
                        else
                        {
                            m_dwLastError = GetLastError();
                        }
                    }
                } // while

                if (!bGetReponseSucceed)
                {
                    bRetVal = false;
                }

                WinHttpCloseHandle(hRequest);
            }

            WinHttpCloseHandle(hConnect);
        }
    }

    return bRetVal;
}

std::wstring WinHttpClient::GetResponseHeader(void)
{
    return m_responseHeader;
}

std::wstring WinHttpClient::GetResponseContent(void)
{
    return m_responseContent;
}

bool WinHttpClient::SetDataToSend(BYTE* data, unsigned int dataSize)
{
    if (data == NULL || dataSize < 0)
        return false;

    if (m_pDataToSend != NULL)
        delete[] m_pDataToSend;

    m_pDataToSend = NULL;
    m_pDataToSend = new BYTE[dataSize];

    if (m_pDataToSend != NULL)
    {
        CopyMemory(m_pDataToSend, data, dataSize);
        m_dataToSendSize = dataSize;
        return true;
    }

    return false;
}

bool WinHttpClient::SetRequestHeaders(const std::wstring &additionalRequestHeaders)
{
    m_additionalRequestHeaders = additionalRequestHeaders;

    return true;
}

const BYTE *WinHttpClient::GetRawResponseContent(void)
{
    return m_pResponse;
}

unsigned int WinHttpClient::GetRawResponseReceivedContentLength(void)
{
    return m_responseByteCountReceived;
}

std::wstring WinHttpClient::GetResponseStatusCode(void)
{
    return m_statusCode;
}
