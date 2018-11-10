#pragma once

#include <comutil.h>

inline std::string WideStringToUTF8(const std::wstring& wstr)
{
	if (wstr.empty()) return std::string();
	const int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	std::string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

inline std::wstring UTF8ToWideString(const std::string& str)
{
	if (str.empty()) return std::wstring();
	const int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	std::wstring strTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &strTo[0], size_needed);
	return strTo;
}

inline std::wstring Trim(const std::wstring &source, const std::wstring &targets)
{
	std::wstring::size_type start = 0;
	std::wstring::size_type end = 0;

    for (start = 0; start < source.size(); start++)
    {
        bool bIsTarget = false;
        for (std::wstring::size_type i = 0; i < targets.size(); i++)
        {
            if (source[start] == targets[i])
            {
                bIsTarget = true;
                break;
            }
        }
        if (!bIsTarget)
        {
            break;
        }
    }
    for (end = source.size() - 1; (int)end >= 0; end--)
    {
        bool bIsTarget = false;
        for (std::wstring::size_type i = 0; i < targets.size(); i++)
        {
            if (source[end] == targets[i])
            {
                bIsTarget = true;
                break;
            }
        }
        if (!bIsTarget)
        {
            break;
        }
    }
	std::wstring result = L"";
    if (end >= start && start < source.size() && end >= 0)
    {
        result = source.substr(start, end-start+1);
    }

    return result;
}

inline bool PrepareString(wchar_t *dest, size_t *size, const std::wstring &src)
{
    if (dest == NULL)
    {
        if (size != NULL)
        {
            *size = src.size();
        }
        return false;
    }
    else
    {
        if (size != NULL)
        {
            wcsncpy_s(dest, *size, src.c_str(), _TRUNCATE);
            if (*size <= src.size())
            {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                return false;
            }
        }
    }
    return true;
}

inline std::wstring ReplaceString(const std::wstring &srcStr, const std::wstring &oldStr, const std::wstring &newStr)
{
    if (srcStr.size() <= 0 || oldStr.size() <= 0)
    {
        return srcStr;
    }
	std::wstring strReturn = srcStr;
	std::wstring::size_type offset = 0;
	std::wstring::size_type start = strReturn.find(oldStr);
    while (start != std::wstring::npos)
    {
        offset = start + newStr.size();
        strReturn.replace(start, oldStr.size(), newStr);
        start = strReturn.find(oldStr, offset);
    }

    return strReturn;
}

inline int StringToInteger(const std::wstring &number)
{
    if (number.size() <= 0)
    {
        return 0;
    }
	std::wstring num = ReplaceString(number, L",", L"");
    num = ReplaceString(num, L" ", L"");

    return _wtoi(num.c_str());
}

inline std::wstring LowerString(const std::wstring &text)
{
    if (text.size() <= 0)
    {
        return L"";
    }
    unsigned int iLength = text.size() + 1;
    wchar_t *pTemp = new wchar_t[iLength];
    if (pTemp == NULL)
    {
        return L"";
    }
    wcscpy_s(pTemp, iLength, text.c_str());
    _wcslwr_s(pTemp, iLength);
	std::wstring retStr = pTemp;
    delete[] pTemp;

    return retStr;
}

inline std::wstring UpperString(const std::wstring &text)
{
    if (text.size() <= 0)
    {
        return L"";
    }
    unsigned int iLength = text.size() + 1;
    wchar_t *pTemp = new wchar_t[iLength];
    if (pTemp == NULL)
    {
        return L"";
    }
    wcscpy_s(pTemp, iLength, text.c_str());
    _wcsupr_s(pTemp, iLength);
	std::wstring retStr = pTemp;
    delete[] pTemp;

    return retStr;
}

inline bool SeparateString(const std::wstring &content, const std::wstring &delimiter, std::vector<std::wstring> &result)
{
    if (content.size() <= 0 || delimiter.size() <= 0)
    {
        return false;
    }

    result.clear();
	std::wstring::size_type start = 0;
	std::wstring::size_type index = 0;
    index = content.find(delimiter, start);
    while (index != std::wstring::npos)
    {
		std::wstring::size_type size = index - start;
        if (size > 0)
        {
			std::wstring temp = content.substr(start, size);
            if (temp.size() > 0)
            {
                result.push_back(temp);
            }
        }
        start  += size + delimiter.size();
        index = content.find(delimiter, start);
    }
    if (content.find(delimiter) != std::wstring::npos)
    {
		std::wstring last = content.substr(start);
        if (last.size() > 0)
        {
            result.push_back(last);
        }
    }
    else
    {
        false;
    }

    return true;
}

inline std::wstring URLEncoding(const std::wstring &keyword, bool convertToUTF8 = true)
{
    int iLength = 0;
    char *szKeyword = NULL;

    if (convertToUTF8)
    {
        iLength = WideCharToMultiByte(CP_UTF8,
                                      0,
                                      keyword.c_str(),
                                      keyword.length(),
                                      NULL,
                                      0,
                                      NULL,
                                      NULL);
        if (iLength <= 0)
        {
            return L"";
        }

        szKeyword = new char[iLength];
        if (szKeyword == NULL)
        {
            return L"";
        }
        iLength = WideCharToMultiByte(CP_UTF8,
                                      0,
                                      keyword.c_str(),
                                      keyword.length(),
                                      szKeyword,
                                      iLength,
                                      NULL,
                                      NULL);
    }
    else
    {
		std::string strKeyword = (char *)(_bstr_t)keyword.c_str();
        iLength = (int)strKeyword.length();
        szKeyword = new char[strKeyword.length() + 1];
        strcpy_s(szKeyword, strKeyword.length() + 1, strKeyword.c_str());
    }

	std::wstring encodedKeyword = L"";
	std::string strEncodedKeyword = "";
    for (int i = 0; i < iLength; i++)
    {
        unsigned char c = (unsigned char)szKeyword[i];
        char temp[MAX_PATH] = "";
        sprintf_s(temp, MAX_PATH, "%%%2X", c);
        if (temp[1] == ' ')
        {
            temp[1] = '0';
        }
        strEncodedKeyword += temp;
    }
    if (szKeyword != NULL)
    {
        delete[] szKeyword;
    }
    encodedKeyword = (wchar_t *)(_bstr_t)strEncodedKeyword.c_str();
    encodedKeyword = ReplaceString(encodedKeyword, L" ", L"+");

    return encodedKeyword;
}

inline std::wstring FilterFileName(const std::wstring &name)
{
    if (name.size() <= 0)
    {
        return L"";
    }

	std::wstring filteredName = name;
    filteredName = ReplaceString(filteredName, L"/",  L"_");
    filteredName = ReplaceString(filteredName, L"\\", L"_");
    filteredName = ReplaceString(filteredName, L":",  L"_");
    filteredName = ReplaceString(filteredName, L"*",  L"_");
    filteredName = ReplaceString(filteredName, L"?",  L"_");
    filteredName = ReplaceString(filteredName, L"\"", L"_");
    filteredName = ReplaceString(filteredName, L"<",  L"_");
    filteredName = ReplaceString(filteredName, L">",  L"_");
    filteredName = ReplaceString(filteredName, L"|",  L"_");

    return filteredName;
}

inline std::wstring GetMagic(unsigned int length)
{
    srand(GetTickCount());
    if (length <= 0)
    {
        return L"";
    }

	std::wstring margic = L"";
    for (unsigned int i = 0; i < length; i++)
    {
        wchar_t szMargic[50] = L"";
        swprintf_s(szMargic, 50, L"%c", rand() % 26 + L'a');
        margic += szMargic;
    }

    return margic;
}

inline std::wstring GetHost(const std::wstring &url)
{
    if (url.size() <= 0)
    {
        return L"";
    }

	std::wstring urlWidthoutHttp = ReplaceString(LowerString(url), L"http://", L"");

    unsigned int index = urlWidthoutHttp.find(L"/");
    if (index == std::wstring::npos)
    {
        index = urlWidthoutHttp.find(L"\\");
    }
    if (index == std::wstring ::npos)
    {
        return urlWidthoutHttp;
    }

    return urlWidthoutHttp.substr(0, index);
}

inline std::wstring GetValidFileName(const std::wstring &fileName)
{
    if (fileName.size() == 0)
    {
        return L"";
    }
	std::wstring tempFileName = fileName;
    tempFileName = ReplaceString(tempFileName, L"\\", L"_");
    tempFileName = ReplaceString(tempFileName, L"/", L"_");
    tempFileName = ReplaceString(tempFileName, L":", L"_");
    tempFileName = ReplaceString(tempFileName, L"*", L"_");
    tempFileName = ReplaceString(tempFileName, L"?", L"_");
    tempFileName = ReplaceString(tempFileName, L"\"", L"_");
    tempFileName = ReplaceString(tempFileName, L"<", L"_");
    tempFileName = ReplaceString(tempFileName, L">", L"_");
    tempFileName = ReplaceString(tempFileName, L"|", L"_");
    tempFileName = ReplaceString(tempFileName, L"\r", L"_");
    tempFileName = ReplaceString(tempFileName, L"\n", L"_");
    tempFileName = ReplaceString(tempFileName, L"%", L"_");

    return tempFileName;
}
