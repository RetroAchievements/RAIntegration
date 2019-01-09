#include "RA_RichPresence.h"

#include "RA_Defs.h"
#include "RA_MemManager.h"

#include "data\GameContext.hh"

#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

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

RA_RichPresenceInterpreter::DisplayString::DisplayString(const std::string& sCondition)
{
    const int nSize = rc_trigger_size(sCondition.c_str());
    if (nSize < 0)
    {
        // parse error occurred
        RA_LOG("rc_trigger_size returned %d", nSize);
        m_pTrigger = nullptr;
    }
    else
    {
        // allocate space and parse again
        m_pTriggerBuffer = std::make_shared<std::vector<unsigned char>>(nSize);
        auto* pTrigger = rc_parse_trigger(m_pTriggerBuffer.get()->data(), sCondition.c_str(), nullptr, 0);
        m_pTrigger = pTrigger;
    }
}

void RA_RichPresenceInterpreter::DisplayString::InitializeParts(const std::string& sDisplayString,
    std::map<std::string, int>& mFormats, std::vector<Lookup>& vLookups)
{
    bool bHasEscapes = false;
    size_t nIndex = 0;
    size_t nStart = 0;
    do
    {
        if (nIndex < sDisplayString.length() && sDisplayString.at(nIndex) != '@')
        {
            if (sDisplayString.at(nIndex) == '\\')
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
                    // Most of the tests need this
                    if ((sDisplayString.at(nStart) == '\\') && (sDisplayString.at(nStart) != sDisplayString.back()))
                        ++nStart;
                    part.m_sDisplayString.push_back(sDisplayString.at(nStart));

                    // Only one test needs this
                    if ((sDisplayString.at(nStart) == '\\') && (sDisplayString.at(nStart) == sDisplayString.back()))
                        part.m_sDisplayString.pop_back();
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
        while (nIndex < sDisplayString.length() && sDisplayString.at(nIndex) != '(')
            nIndex++;
        if (nIndex == sDisplayString.length())
            break;

        std::string sLookup(sDisplayString, nStart, nIndex - nStart);

        nStart = nIndex + 1;
        nIndex = nStart;
        while (nIndex < sDisplayString.length() && sDisplayString.at(nIndex) != ')')
            nIndex++;
        if (nIndex == sDisplayString.length())
            break;

        std::string sMemValue(sDisplayString, nStart, nIndex - nStart);
        nStart = nIndex + 1;

        auto iter = mFormats.find(sLookup);
        if (iter != mFormats.end())
        {
            part.m_nFormat = iter->second;
            InitializeValue(part, sMemValue.c_str());
        }
        else
        {
            for (std::vector<Lookup>::const_iterator lookup = vLookups.begin(); lookup != vLookups.end(); ++lookup)
            {
                if (lookup->Description() == sLookup)
                {
                    part.m_pLookup = &(*lookup);
                    InitializeValue(part, sMemValue.c_str());
                    break;
                }
            }
        }
    } while (true);
}

void RA_RichPresenceInterpreter::DisplayString::InitializeValue(RA_RichPresenceInterpreter::DisplayString::Part& part, const char* sValue)
{
    const int nSize = rc_value_size(sValue);
    if (nSize < 0)
    {
        // parse error occurred
        RA_LOG("rc_value_size returned %d", nSize);
        part.m_pValue = nullptr;
    }
    else
    {
        // allocate space and parse again
        part.m_pValueBuffer = std::make_shared<std::vector<unsigned char>>(nSize);
        auto* pValue = rc_parse_value(part.m_pValueBuffer.get()->data(), sValue, nullptr, 0);
        part.m_pValue = pValue;
    }
}

bool RA_RichPresenceInterpreter::DisplayString::Test() noexcept
{
    rc_trigger_t* pTrigger = static_cast<rc_trigger_t*>(m_pTrigger);
    return pTrigger && rc_test_trigger(pTrigger, rc_peek_callback, nullptr, nullptr);
}

std::string RA_RichPresenceInterpreter::DisplayString::GetDisplayString() const
{
    std::string sResult;
    for (const auto& part : m_vParts)
    {
        if (!part.m_sDisplayString.empty())
            sResult.append(part.m_sDisplayString);

        if (part.m_pValue)
        {
            auto* pValue = static_cast<rc_value_t*>(part.m_pValue);
            const unsigned int nValue = rc_evaluate_value(pValue, rc_peek_callback, nullptr, nullptr);

            if (part.m_pLookup != nullptr)
            {
                sResult.append(part.m_pLookup->GetText(nValue));
            }
            else
            {
                char buffer[32];
                rc_format_value(buffer, sizeof(buffer), nValue, part.m_nFormat);
                sResult.append(buffer);
            }
        }
    }

    return sResult;
}

static bool GetLine(ra::services::TextReader& pReader, std::string& sLine)
{
    if (!pReader.GetLine(sLine))
        return false;

    if (!sLine.empty())
    {
        size_t index = sLine.find("//");
        while (index != std::string::npos && index > 0 && sLine.at(index - 1) == '\\')
            index = sLine.find("//", index + 1);

        if (index != std::string::npos)
        {
            // if a comment marker was found, remove it and any trailing whitespace
            while (index > 0 && isspace(ra::to_unsigned(sLine.at(index - 1))))
                index--;

            sLine.resize(index);
        }
        else if (sLine.at(sLine.length() - 1) == '\r')
        {
            // also remove CR, not just LF
            sLine.resize(sLine.length() - 1);
        }
    }

    return true;
}

bool RA_RichPresenceInterpreter::Load()
{
    m_vLookups.clear();
    m_vDisplayStrings.clear();

#ifdef RA_UTEST
    return false;
#else
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pRich = pLocalStorage.ReadText(ra::services::StorageItemType::RichPresence, std::to_wstring(pGameContext.GameId()));
    if (pRich == nullptr)
        return false;

    return Load(*pRich.get());
#endif
}

bool RA_RichPresenceInterpreter::Load(ra::services::TextReader& pReader)
{
    m_vLookups.clear();
    m_vDisplayStrings.clear();

    std::vector<std::pair<std::string, std::string>> mDisplayStrings;
    std::string sDisplayString;

    std::map<std::string, int> mFormats;

    std::string sLine;
    while (GetLine(pReader, sLine))
    {
        if (strncmp("Lookup:", sLine.c_str(), 7) == 0)
        {
            std::string sLookupName(sLine, 7);
            Lookup& newLookup = m_vLookups.emplace_back(sLookupName);
            do
            {
                if (!GetLine(pReader, sLine) || sLine.length() < 2)
                    break;

                const size_t nIndex = sLine.find('=');
                if (nIndex == std::string::npos)
                    continue;

                std::string sLabel(sLine, nIndex + 1);

                if (sLine.front() == '*')
                {
                    newLookup.SetDefault(sLabel);
                    continue;
                }

                unsigned int nVal{};
                if (ra::StringStartsWith(sLine, "0x"))
                    nVal = std::stoul(sLine.substr(2), nullptr, 16);
                else
                    nVal = std::stoul(sLine);

                newLookup.AddLookupData(nVal, sLabel);
            } while (true);

            RA_LOG("RP: Adding Lookup %s (%zu items)\n", sLookupName.c_str(), newLookup.NumItems());
        }
        else if (strncmp("Format:", sLine.c_str(), 7) == 0)
        {
            std::string sFormatName(sLine, 7);
            if (GetLine(pReader, sLine) && strncmp("FormatType=", sLine.c_str(), 11) == 0)
            {
                std::string sFormatType(sLine, 11);
                int nType = rc_parse_format(sFormatType.c_str());

                RA_LOG("RP: Adding Formatter %s (%s)\n", sFormatName.c_str(), sFormatType.c_str());
                mFormats[sFormatName] = nType;
            }
        }
        else if (strncmp("Display:", sLine.c_str(), 8) == 0)
        {
            do
            {
                if (!GetLine(pReader, sLine) || sLine.length() < 2)
                    break;

                if (sLine.front() == '?')
                {
                    const size_t nIndex = sLine.find('?', 1);
                    if (nIndex != std::string::npos)
                    {
                        std::string sCondition(sLine, 1, nIndex - 1);
                        std::string sDisplay(sLine, nIndex + 1);

                        mDisplayStrings.emplace_back(sCondition, sDisplay);
                        continue;
                    }
                }

                sDisplayString = sLine;
                break;
            } while (true);
        }
    }

    if (sDisplayString.empty())
        return false;

    for (auto& pPair : mDisplayStrings)
    {
        auto& displayString = m_vDisplayStrings.emplace_back(pPair.first);
        displayString.InitializeParts(pPair.second, mFormats, m_vLookups);
    }

    auto& displayString = m_vDisplayStrings.emplace_back();
    displayString.InitializeParts(sDisplayString, mFormats, m_vLookups);

    return true;
}

std::string RA_RichPresenceInterpreter::GetRichPresenceString()
{
    if (m_vDisplayStrings.empty())
        return std::string();

    for (auto it = m_vDisplayStrings.begin(); it != std::prev(m_vDisplayStrings.end()); ++it)
    {
        if (it->Test())
            return it->GetDisplayString();
    }

    return m_vDisplayStrings.back().GetDisplayString();
}
