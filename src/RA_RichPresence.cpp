#include "RA_RichPresence.h"

#include "RA_Defs.h"
#include "RA_MemValue.h"

RA_RichPresenceInterpreter g_RichPresenceInterpreter;

RA_RichPresenceInterpreter::Lookup::Lookup(const std::string& sDesc)
    : m_sLookupDescription(sDesc)
{
}

const std::string& RA_RichPresenceInterpreter::Lookup::GetText(unsigned int nValue) const
{
    auto iter = m_mLookupData.find(nValue);
    if (iter != m_mLookupData.end())
        return iter->second;

    return m_sDefault;
}

RA_RichPresenceInterpreter::DisplayString::DisplayString()
{
    m_conditions.SetAlwaysTrue();
}

RA_RichPresenceInterpreter::DisplayString::DisplayString(const std::string& sCondition)
{
    const char* pBuffer = sCondition.data();
    m_conditions.ParseFromString(pBuffer);

    if (*pBuffer != '\0')
        m_conditions.SetAlwaysFalse();
}

void RA_RichPresenceInterpreter::DisplayString::InitializeParts(const std::string& sDisplayString,
    std::map<std::string, MemValue::Format>& mFormats, std::vector<Lookup>& vLookups)
{
    bool bHasEscapes = false;
    size_t nIndex = 0;
    size_t nStart = 0;
    do
    {
        if (nIndex < sDisplayString.length() && sDisplayString[nIndex] != '@')
        {
            if (sDisplayString[nIndex] == '\\')
            {
                bHasEscapes = true;
                if (nIndex + 1 < sDisplayString.length())
                    ++nIndex;
            }

            ++nIndex;
            continue;
        }

        Part& part = m_vParts.emplace_back();
        if (nIndex > nStart)
        {
            if (bHasEscapes)
            {
                do
                {
                    if (sDisplayString[nStart] == '\\')
                        ++nStart;

                    part.m_sDisplayString.push_back(sDisplayString[nStart]);
                    ++nStart;
                } while (nStart < nIndex);

                bHasEscapes = false;
            }
            else
            {
                part.m_sDisplayString.assign(sDisplayString, nStart, nIndex - nStart);
            }
        }

        if (nIndex == sDisplayString.length())
            break;

        nStart = nIndex + 1;
        nIndex = nStart;
        while (nIndex < sDisplayString.length() && sDisplayString[nIndex] != '(')
            nIndex++;
        if (nIndex == sDisplayString.length())
            break;

        std::string sLookup(sDisplayString, nStart, nIndex - nStart);

        nStart = nIndex + 1;
        nIndex = nStart;
        while (nIndex < sDisplayString.length() && sDisplayString[nIndex] != ')')
            nIndex++;
        if (nIndex == sDisplayString.length())
            break;

        std::string sMemValue(sDisplayString, nStart, nIndex - nStart);
        nStart = nIndex + 1;

        auto iter = mFormats.find(sLookup);
        if (iter != mFormats.end())
        {
            part.m_nFormat = iter->second;
            part.m_memValue.ParseFromString(sMemValue.c_str());
        }
        else
        {
            for (std::vector<Lookup>::const_iterator lookup = vLookups.begin(); lookup != vLookups.end(); ++lookup)
            {
                if (lookup->Description() == sLookup)
                {
                    part.m_pLookup = &(*lookup);
                    part.m_memValue.ParseFromString(sMemValue.c_str());
                    break;
                }
            }
        }
    } while (true);
}

bool RA_RichPresenceInterpreter::DisplayString::Test()
{
    bool bDirtyConditions, bResetRead; // for HitCounts - not supported in RichPresence
    return m_conditions.Test(bDirtyConditions, bResetRead);
}

std::string RA_RichPresenceInterpreter::DisplayString::GetDisplayString() const
{
    std::string sResult;
    for (const auto& part : m_vParts)
    {
        if (!part.m_sDisplayString.empty())
            sResult.append(part.m_sDisplayString);

        if (!part.m_memValue.IsEmpty())
        {
            unsigned int nValue = part.m_memValue.GetValue();

            if (part.m_pLookup != nullptr)
                sResult.append(part.m_pLookup->GetText(nValue));
            else
                sResult.append(MemValue::FormatValue(nValue, part.m_nFormat));
        }
    }

    return sResult;
}

static bool GetLine(std::stringstream& stream, std::string& sLine)
{
    if (!std::getline(stream, sLine, '\n'))
        return false;

    if (!sLine.empty())
    {
        size_t index = sLine.find("//");
        while (index != std::string::npos && index > 0 && sLine[index - 1] == '\\')
            index = sLine.find("//", index + 1);

        if (index != std::string::npos)
        {
            // if a comment marker was found, remove it and any trailing whitespace
            while (index > 0 && isspace(sLine[index - 1]))
                index--;

            sLine.resize(index);
        }
        else if (sLine[sLine.length() - 1] == '\r')
        {
            // also remove CR, not just LF
            sLine.resize(sLine.length() - 1);
        }
    }

    return true;
}

void RA_RichPresenceInterpreter::ParseFromString(const char* sRichPresence)
{
    m_vLookups.clear();
    m_vDisplayStrings.clear();

    std::map<std::string, std::string> mDisplayStrings;
    std::string sDisplayString;

    std::map<std::string, MemValue::Format> mFormats;

    std::stringstream ssRichPresence(sRichPresence);
    std::string sLine;
    while (GetLine(ssRichPresence, sLine))
    {
        if (strncmp("Lookup:", sLine.c_str(), 7) == 0)
        {
            std::string sLookupName(sLine, 7);
            Lookup& newLookup = m_vLookups.emplace_back(sLookupName);
            do
            {
                if (!GetLine(ssRichPresence, sLine) || sLine.length() < 2)
                    break;

                size_t nIndex = sLine.find('=');
                if (nIndex == std::string::npos)
                    continue;

                std::string sLabel(sLine, nIndex + 1);

                if (sLine[0] == '*')
                {
                    newLookup.SetDefault(sLabel);
                    continue;
                }

                unsigned int nVal;
                if (sLine[0] == '0' && sLine[1] == 'x')
                    nVal = strtoul(&sLine[2], nullptr, 16);
                else
                    nVal = strtoul(&sLine[0], nullptr, 10);

                newLookup.AddLookupData(nVal, sLabel);
            } while (true);

            RA_LOG("RP: Adding Lookup %s (%zu items)\n", sLookupName.c_str(), newLookup.NumItems());
        }
        else if (strncmp("Format:", sLine.c_str(), 7) == 0)
        {
            std::string sFormatName(sLine, 7);
            if (GetLine(ssRichPresence, sLine) && strncmp("FormatType=", sLine.c_str(), 11) == 0)
            {
                std::string sFormatType(sLine, 11);
                MemValue::Format nType = MemValue::ParseFormat(sFormatType);

                RA_LOG("RP: Adding Formatter %s (%s)\n", sFormatName.c_str(), sFormatType.c_str());
                mFormats[sFormatName] = nType;
            }
        }
        else if (strncmp("Display:", sLine.c_str(), 8) == 0)
        {
            do
            {
                if (!GetLine(ssRichPresence, sLine) || sLine.length() < 2)
                    break;

                if (sLine[0] == '?')
                {
                    size_t nIndex = sLine.find('?', 1);
                    if (nIndex != std::string::npos)
                    {
                        std::string sCondition(sLine, 1, nIndex - 1);
                        std::string sDisplay(sLine, nIndex + 1);

                        mDisplayStrings[sCondition] = sDisplay;
                        continue;
                    }
                }

                sDisplayString = sLine;
                break;
            } while (true);
        }
    }

    if (!sDisplayString.empty())
    {
        for (std::map<std::string, std::string>::const_iterator iter = mDisplayStrings.begin(); iter != mDisplayStrings.end(); ++iter)
        {
            auto& displayString = m_vDisplayStrings.emplace_back(iter->first);
            displayString.InitializeParts(iter->second, mFormats, m_vLookups);
        }

        auto& displayString = m_vDisplayStrings.emplace_back();
        displayString.InitializeParts(sDisplayString, mFormats, m_vLookups);
    }
}

std::string RA_RichPresenceInterpreter::GetRichPresenceString()
{
    for (auto& displayString : m_vDisplayStrings)
    {
        if (displayString.Test())
            return displayString.GetDisplayString();
    }

    return std::string();
}
