#ifndef RA_RICHPRESENCE_H
#define RA_RICHPRESENCE_H
#pragma once

#include "RA_Leaderboard.h"

typedef unsigned int DataPos;


class RA_Lookup
{
public:
    explicit RA_Lookup(_In_ const std::string& sDesc);

public:
    // couldn't we just make a generic get function? - Samer
#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
    // I think you have it backwards boss, the key is unique where the value is not  - Samer

    void AddLookupData(_In_ DataPos nValue, _In_ const std::string& sLookupData)
    { m_lookupData.try_emplace(nValue, sLookupData); }

    const std::string& Description() const { return m_sLookupDescription; }
#pragma warning(pop)

   const std::string& Lookup(DataPos nValue) const;
 
private:
    std::string m_sLookupDescription;
    std::map<DataPos, std::string> m_lookupData;
};

class RA_ConditionalDisplayString
{
public:
    RA_ConditionalDisplayString(const char* pLine);

    bool Test();

    // These getters seem useless to me, the compiler does too - Sam
#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
    const std::string& GetDisplayString() const { return m_sDisplayString; }
#pragma warning(pop)

private:
    std::string m_sDisplayString;
    ConditionSet m_conditions;
};

class RA_Formattable
{
public:
    RA_Formattable(const std::string& sDesc, RA_Leaderboard::FormatType nType);

    std::string Lookup(DataPos nValue) const;

#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
    const std::string& Description() const { return m_sLookupDescription; }
#pragma warning(pop)

private:
    std::string m_sLookupDescription;
    RA_Leaderboard::FormatType m_nFormatType;
};

class RA_RichPresenceInterpretter
{
public:
    static void PersistAndParseScript(GameID nGameID, const std::string& sScript);

public:
    RA_RichPresenceInterpretter() {}

public:
    void ParseRichPresenceFile(const std::string& sFilename);

    const std::string& GetRichPresenceString();
    const std::string& Lookup(const std::string& sLookupName, const std::string& sMemString) const;

    bool Enabled() const;

private:
    std::vector<RA_Lookup> m_lookups;
    std::vector<RA_Formattable> m_formats;

    std::vector<RA_ConditionalDisplayString> m_conditionalDisplayStrings;
    std::string m_sDisplay;
};

// you're not supposed to put extern on an auto unless you want to export it (which seems to be the case)
RA_RichPresenceInterpretter g_RichPresenceInterpretter;

#endif // !RA_RICHPRESENCE_H
