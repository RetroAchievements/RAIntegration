#include "RA_RichPresence.h"

#include "RA_Core.h"
#include "RA_GameData.h"
#include "RA_MemValue.h"

RA_RichPresenceInterpretter g_RichPresenceInterpretter;

RA_Lookup::RA_Lookup(const std::string& sDesc)
    : m_sLookupDescription(sDesc)
{
}

const std::string& RA_Lookup::Lookup(DataPos nValue) const
{
    if (m_lookupData.find(nValue) != m_lookupData.end())
        return m_lookupData.find(nValue)->second;

    static const std::string sUnknown = "";
    return sUnknown;
}

RA_ConditionalDisplayString::RA_ConditionalDisplayString(const char* pBuffer)
{
    ++pBuffer; // skip first question mark
    m_conditions.ParseFromString(pBuffer);

    if (*pBuffer == '?')
    {
        // valid condition
        *pBuffer++;
        m_sDisplayString = pBuffer;
    }
    else
    {
        // not a valid condition, ensure Test() never returns true
        m_conditions.Clear();
    }
}

bool RA_ConditionalDisplayString::Test()
{
    bool bDirtyConditions, bResetRead; // for HitCounts - not supported in RichPresence
    return m_conditions.Test(bDirtyConditions, bResetRead);
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

                        DataPos nVal = static_cast<DataPos>(strtol(pValue, nullptr, nBase));

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

                    MemValue::Format nType = MemValue::ParseFormat(pType);

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
            MemValue nValue;
            nValue.ParseFromString(sMemString.c_str());
            return m_lookups.at(i).Lookup(static_cast<DataPos>(nValue.GetValue()));
        }
    }

    //	check formatters
    auto iter = m_formats.find(sName);
    if (iter != m_formats.end())
    {
        MemValue nValue;
        nValue.ParseFromString(sMemString.c_str());
        return nValue.GetFormattedValue(iter->second);
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
void RA_RichPresenceInterpretter::PersistAndParseScript(GameID nGameID, const std::string& str)
{
    //	Read to file:
    ChangeToHomeDirectory();
    _WriteBufferToFile(RA_DIR_DATA + std::to_string(nGameID) + "-Rich.txt", str);

    //	Then install it
    g_RichPresenceInterpretter.ParseRichPresenceFile(RA_DIR_DATA + std::to_string(nGameID) + "-Rich.txt");
}

