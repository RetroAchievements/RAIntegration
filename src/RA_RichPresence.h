#ifndef RA_RICHPRESENCE_H
#define RA_RICHPRESENCE_H
#pragma once

#include "RA_Condition.h"

#include <map>
#include <memory>

#include "services\TextReader.hh"

class RA_RichPresenceInterpreter
{
public:
    GSL_SUPPRESS_F6 bool Load(); /*Test won't throw but Integration might*/
    bool Load(ra::services::TextReader& pReader);

    std::string GetRichPresenceString();

    bool Enabled() const noexcept { return !m_vDisplayStrings.empty(); }

protected:

    class Lookup
    {
    public:
        explicit Lookup(const std::string& sDesc);

        const std::string& Description() const noexcept { return m_sLookupDescription; }
        const std::string& GetText(unsigned int nValue) const;

        void AddLookupData(unsigned int nValue, const std::string& sLookupData) { m_mLookupData[nValue] = sLookupData; }
        void SetDefault(const std::string& sDefault) { m_sDefault = sDefault; }
        size_t NumItems() const noexcept { return m_mLookupData.size(); }

    private:
        std::string m_sDefault;
        std::string m_sLookupDescription;
        std::map<unsigned int, std::string> m_mLookupData;
    };

    class DisplayString
    {
    public:
        DisplayString() noexcept = default;
        explicit DisplayString(const std::string& sCondition);
        void InitializeParts(const std::string& sDisplayString, std::map<std::string, int>& mFormats,
                             std::vector<Lookup>& vLookups);

        bool Test() noexcept;
        std::string GetDisplayString() const;

    protected:
        struct Part
        {
            std::string m_sDisplayString;

            void* m_pValue = nullptr;                                   //  rc_value_t
            std::shared_ptr<std::vector<unsigned char>> m_pValueBuffer; //  buffer for rc_value_t

            const Lookup* m_pLookup = nullptr;
            int m_nFormat = 0;
        };

    private:
        void InitializeValue(Part& part, const char* sValue);

        std::vector<Part> m_vParts;

        void* m_pTrigger{};                                           //  rc_trigger_t
        std::shared_ptr<std::vector<unsigned char>> m_pTriggerBuffer; //  buffer for rc_trigger_t
    };

    std::vector<Lookup> m_vLookups;
    std::vector<DisplayString> m_vDisplayStrings;
};

#endif // !RA_RICHPRESENCE_H
