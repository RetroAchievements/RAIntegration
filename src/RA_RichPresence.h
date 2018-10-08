#ifndef RA_RICHPRESENCE_H
#define RA_RICHPRESENCE_H
#pragma once

#include "RA_Condition.h"
#include "RA_MemValue.h"

class RA_RichPresenceInterpreter
{
public:
    RA_RichPresenceInterpreter() {}

    void ParseFromString(const char* sRichPresence);

    std::string GetRichPresenceString();
    
    bool Enabled() const { return !m_vDisplayStrings.empty(); }

protected:
    class Lookup
    {
    public:
        Lookup(const std::string& sDesc);

        const std::string& Description() const { return m_sLookupDescription; }
        const std::string& GetText(unsigned int nValue) const;

        void AddLookupData(unsigned int nValue, const std::string& sLookupData) { m_mLookupData[nValue] = sLookupData; }
        void SetDefault(const std::string& sDefault) { m_sDefault = sDefault; }
        size_t NumItems() const { return m_mLookupData.size(); }

    private:
        std::string m_sDefault;
        std::string m_sLookupDescription;
        std::map<unsigned int, std::string> m_mLookupData;
    };

    class DisplayString
    {
    public:
        DisplayString();
        DisplayString(const std::string& sCondition);
        void InitializeParts(const std::string& sDisplayString,
            std::map<std::string, MemValue::Format>& mFormats, std::vector<Lookup>& vLookups);

        bool Test();
        std::string GetDisplayString() const;

    protected:
        struct Part
        {
            std::string m_sDisplayString;
            MemValue m_memValue;
            const Lookup* m_pLookup{ nullptr };
            MemValue::Format m_nFormat = MemValue::Format::Value;
#pragma warning(push)
#pragma warning(disable : 26495) // "variable" uninitialized
        };
#pragma warning(pop)


    private:

        std::vector<Part> m_vParts;
        ConditionSet m_conditions;
    };

private:
    std::vector<Lookup> m_vLookups;
    std::vector<DisplayString> m_vDisplayStrings;
};

extern RA_RichPresenceInterpreter g_RichPresenceInterpreter;

#endif // !RA_RICHPRESENCE_H
