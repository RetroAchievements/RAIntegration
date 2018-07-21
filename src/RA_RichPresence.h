#ifndef RA_RICHPRESENCE_H
#define RA_RICHPRESENCE_H
#pragma once

#include "RA_Leaderboard.h"
#include "RA_Defs.h"

class RA_Lookup
{
public:
    RA_Lookup(const std::string& sDesc);

public:
    void AddLookupData(ra::DataPos nValue, const std::string& sLookupData) { m_lookupData[nValue] = sLookupData; }
    const std::string& Lookup(ra::DataPos nValue) const;

    const std::string& Description() const { return m_sLookupDescription; }

private:
    std::string m_sLookupDescription;
    std::map<ra::DataPos, std::string> m_lookupData;
};

class RA_ConditionalDisplayString
{
public:
    RA_ConditionalDisplayString(const char* pLine);

    bool Test();
    const std::string& GetDisplayString() const { return m_sDisplayString; }

private:
    std::string                 m_sDisplayString;     //  display string

    void*                       m_pTrigger = nullptr; //  rc_trigger_t
    std::vector<unsigned char>  m_pTriggerBuffer;     //  buffer for rc_trigger_t
};

class RA_RichPresenceInterpretter
{
public:
    static void PersistAndParseScript(ra::GameID nGameID, const std::string& sScript);

public:
    RA_RichPresenceInterpretter() {}

public:
    void ParseRichPresenceFile(const std::string& sFilename);

    const std::string& GetRichPresenceString();
    const std::string Lookup(const std::string& sLookupName, const std::string& sMemString) const;

    bool Enabled() const;

private:
    std::vector<RA_Lookup> m_lookups;
    std::map<std::string, int> m_formats;

    std::vector<RA_ConditionalDisplayString> m_conditionalDisplayStrings;
    std::string m_sDisplay;
};

extern RA_RichPresenceInterpretter g_RichPresenceInterpretter;

#endif // !RA_RICHPRESENCE_H
