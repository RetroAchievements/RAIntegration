#include "RA_RichPresence.h"

#include "RA_Core.h"
#include "RA_GameData.h"
#include "RA_MemManager.h"

#include "rcheevos\include\rcheevos.h"

RA_RichPresenceInterpretter g_RichPresenceInterpretter;

RA_Lookup::RA_Lookup(const std::string& sDesc)
    : m_sLookupDescription(sDesc)
{
}

const std::string& RA_Lookup::Lookup(ra::DataPos nValue) const
{
    if (m_lookupData.find(nValue) != m_lookupData.end())
        return m_lookupData.find(nValue)->second;

    static const std::string sUnknown = "";
    return sUnknown;
}

RA_ConditionalDisplayString::RA_ConditionalDisplayString(const char* pBuffer)
{
    const char* pTrigger = ++pBuffer; // skip first question mark
    while (*pBuffer != '?')
    {
        if (*pBuffer == '\0')
        {
            // not a valid condition, ensure Test() never returns true
            m_pTrigger = nullptr;
            return;
        }

        ++pBuffer;
    }

    // call with nullptr to determine space required
    int nSize = rc_trigger_size(pTrigger);
    if (nSize < 0)
    {
        // parse error occurred
        RA_LOG("rc_parse_trigger returned %d", nSize);
        m_pTrigger = nullptr;
    }
    else
    {
        // allocate space and parse again
        m_pTriggerBuffer.reset(new unsigned char[nSize]);
        m_pTrigger = rc_parse_trigger(static_cast<void*>(m_pTriggerBuffer.get()), pTrigger, nullptr, 0);

        // valid condition
        m_sDisplayString = ++pBuffer; // skip second question mark
    }
}

bool RA_ConditionalDisplayString::Test()
{
    if (m_pTrigger == nullptr)
        return false;

    rc_trigger_t* pTrigger = static_cast<rc_trigger_t*>(m_pTrigger);
    return rc_test_trigger(pTrigger, rc_peek_callback, nullptr, nullptr);
}

void RA_RichPresenceInterpretter::ParseRichPresenceFile(const std::string& sFilename)
{
    m_formats.clear();
    m_lookups.clear();
    m_sDisplay.clear();
    m_conditionalDisplayStrings.clear();

    const char EndLine = '\n';

    const char* LookupStr = "Lookup:";
    const char* FormatStr = "Format:";
    const char* FormatTypeStr = "FormatType=";
    const char* DisplayableStr = "Display:";

    FILE* pFile = nullptr;
    fopen_s(&pFile, sFilename.c_str(), "r");
    if (pFile != nullptr)
    {
        DWORD nCharsRead = 0;
        char buffer[4096];

        _ReadTil(EndLine, buffer, 4096, &nCharsRead, pFile);
        while (nCharsRead != 0)
        {
            if (strncmp(LookupStr, buffer, strlen(LookupStr)) == 0)
            {
                //	Lookup type
                char* pBuf = buffer + (strlen(LookupStr));
                RA_Lookup newLookup(_ReadStringTil(EndLine, pBuf, TRUE));
                while (nCharsRead != 0 && (buffer[0] != EndLine))
                {
                    _ReadTil(EndLine, buffer, 4096, &nCharsRead, pFile);
                    if (nCharsRead > 2)
                    {
                        char* pBuf = &buffer[0];
                        const char* pValue = _ReadStringTil('=', pBuf, TRUE);
                        const char* pName = _ReadStringTil(EndLine, pBuf, TRUE);

                        int nBase = 10;
                        if (pValue[0] == '0' && pValue[1] == 'x')
                            nBase = 16;

                        ra::DataPos nVal = static_cast<ra::DataPos>(strtoul(pValue, nullptr, nBase));

                        newLookup.AddLookupData(nVal, pName);
                    }
                }

                RA_LOG("RP: Adding Lookup %s\n", newLookup.Description().c_str());
                m_lookups.push_back(newLookup);

            }
            else if (strncmp(FormatStr, buffer, strlen(FormatStr)) == 0)
            {
                //	
                char* pBuf = &buffer[0];
                const char* pUnused = _ReadStringTil(':', pBuf, TRUE);
                std::string sDesc = _ReadStringTil(EndLine, pBuf, TRUE);

                _ReadTil(EndLine, buffer, 4096, &nCharsRead, pFile);
                if (nCharsRead > 0 && strncmp(FormatTypeStr, buffer, strlen(FormatTypeStr)) == 0)
                {
                    char* pBuf = &buffer[0];
                    const char* pUnused = _ReadStringTil('=', pBuf, TRUE);
                    const char* pType = _ReadStringTil(EndLine, pBuf, TRUE);

                    int nType = rc_parse_format(pType);

                    RA_LOG("RP: Adding Formatter %s (%s)\n", sDesc.c_str(), pType);
                    m_formats[sDesc] = nType;
                }
            }
            else if (strncmp(DisplayableStr, buffer, strlen(DisplayableStr)) == 0)
            {
                _ReadTil(EndLine, buffer, 4096, &nCharsRead, pFile);

                char* pBuf = &buffer[0];
                m_sDisplay = _ReadStringTil('\n', pBuf, TRUE);	//	Terminates \n instead

                while (buffer[0] == '?')
                {
                    RA_ConditionalDisplayString conditionalString(buffer);
                    m_conditionalDisplayStrings.push_back(conditionalString);

                    _ReadTil(EndLine, buffer, 4096, &nCharsRead, pFile);
                    pBuf = &buffer[0];
                    m_sDisplay = _ReadStringTil('\n', pBuf, TRUE);
                }
            }

            _ReadTil(EndLine, buffer, 4096, &nCharsRead, pFile);
        }

        fclose(pFile);
    }
}

const std::string RA_RichPresenceInterpretter::Lookup(const std::string& sName, const std::string& sMemString) const
{
    //	check lookups
    for (size_t i = 0; i < m_lookups.size(); ++i)
    {
        if (m_lookups.at(i).Description().compare(sName) == 0)
        {
            char buffer[2048];
            rc_value_t* pValue = rc_parse_value(buffer, sMemString.c_str(), nullptr, 0);
            unsigned int nValue = pValue ? rc_evaluate_value(pValue, rc_peek_callback, nullptr, nullptr) : 0U;
            return m_lookups.at(i).Lookup(static_cast<ra::DataPos>(nValue));
        }
    }

    //	check formatters
    auto iter = m_formats.find(sName);
    if (iter != m_formats.end())
    {
        char buffer[2048];
        rc_value_t* pValue = rc_parse_value(buffer, sMemString.c_str(), nullptr, 0);
        unsigned int nValue = pValue ? rc_evaluate_value(pValue, rc_peek_callback, nullptr, nullptr) : 0U;
        rc_format_value(buffer, sizeof(buffer), nValue, iter->second);
        return std::string(buffer);
    }

    return "";
}

bool RA_RichPresenceInterpretter::Enabled() const
{
    return !m_sDisplay.empty();
}

const std::string& RA_RichPresenceInterpretter::GetRichPresenceString()
{
    static std::string sReturnVal;
    sReturnVal.clear();

    if (g_pCurrentGameData->GetGameID() == 0)
    {
        sReturnVal = "No game loaded";
        return sReturnVal;
    }

    bool bParsingLookupName = false;
    bool bParsingLookupContent = false;

    std::string sLookupName;
    std::string sLookupValue;

    const std::string* sDisplayString = &m_sDisplay;
    for (std::vector<RA_ConditionalDisplayString>::iterator iter = m_conditionalDisplayStrings.begin(); iter != m_conditionalDisplayStrings.end(); ++iter)
    {
        if (iter->Test())
        {
            sDisplayString = &iter->GetDisplayString();
            break;
        }
    }

    for (size_t i = 0; i < sDisplayString->size(); ++i)
    {
        char c = sDisplayString->at(i);

        if (bParsingLookupContent)
        {
            if (c == ')')
            {
                //	End of content
                bParsingLookupContent = false;

                //	Lookup!

                sReturnVal.append(Lookup(sLookupName, sLookupValue));

                sLookupName.clear();
                sLookupValue.clear();
            }
            else
            {
                sLookupValue.push_back(c);
            }
        }
        else if (bParsingLookupName)
        {
            if (c == '(')
            {
                //	Now parsing lookup content
                bParsingLookupContent = true;
                bParsingLookupName = false;
            }
            else
            {
                //	Continue to parse lookup name
                sLookupName.push_back(c);
            }
        }
        else
        {
            //	Not parsing a lookup at all.
            if (c == '@')
            {
                bParsingLookupName = true;
            }
            else
            {
                //	Standard text.
                sReturnVal.push_back(c);
            }
        }
    }

    return sReturnVal;
}

//	static
void RA_RichPresenceInterpretter::PersistAndParseScript(ra::GameID nGameID, const std::string& str)
{
    //	Read to file:
    SetCurrentDirectory(NativeStr(g_sHomeDir).c_str());
    _WriteBufferToFile(RA_DIR_DATA + std::to_string(nGameID) + "-Rich.txt", str);

    //	Then install it
    g_RichPresenceInterpretter.ParseRichPresenceFile(RA_DIR_DATA + std::to_string(nGameID) + "-Rich.txt");
}

