#ifndef RA_RICHPRESENCE_H
#define RA_RICHPRESENCE_H
#pragma once

#include "RA_Condition.h"

#include <memory>
#include <map>

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
            std::map<std::string, int>& mFormats, std::vector<Lookup>& vLookups);

        bool Test();
        std::string GetDisplayString() const;

    protected:
        struct Part
        {
            std::string                      m_sDisplayString;

            void*                            m_pValue;        //  rc_value_t
            std::shared_ptr<unsigned char[]> m_pValueBuffer;  //  buffer for rc_value_t

            const Lookup*                    m_pLookup = nullptr;
            int                              m_nFormat = 0;
        };

    private:
        void InitializeValue(Part& part, const char* sValue);

        std::vector<Part>                m_vParts;

        void*                            m_pTrigger;        //  rc_trigger_t
        std::shared_ptr<unsigned char[]> m_pTriggerBuffer;  //  buffer for rc_trigger_t
    };

    std::vector<Lookup> m_vLookups;
    std::vector<DisplayString> m_vDisplayStrings;
};

extern RA_RichPresenceInterpreter g_RichPresenceInterpreter;

#endif // !RA_RICHPRESENCE_H
