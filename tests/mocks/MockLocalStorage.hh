#ifndef RA_SERVICES_MOCK_LOCALSTORAGE_HH
#define RA_SERVICES_MOCK_LOCALSTORAGE_HH
#pragma once

#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"
#include "services\impl\StringTextReader.hh"
#include "services\impl\StringTextWriter.hh"

namespace ra {
namespace services {
namespace mocks {

class MockLocalStorage : public ILocalStorage
{
public:
    MockLocalStorage() noexcept : m_Override(this) {}

    void MockStoredData(ra::services::StorageItemType nType, const std::wstring& sKey, const std::string& sContents)
    {
        const gsl::not_null<std::string*> pText{gsl::make_not_null(GetText(nType, sKey, true))};
        *pText = sContents;
    }

    bool HasStoredData(ra::services::StorageItemType nType, const std::wstring& sKey) const
    {
        return (GetText(nType, sKey, false) != nullptr);
    }

    const std::string& GetStoredData(ra::services::StorageItemType nType, const std::wstring& sKey) const
    {
        const std::string* const pText = GetText(nType, sKey, false);
        if (pText != nullptr)
            return *pText;

        static const std::string sEmpty;
        return sEmpty;
    }

    std::chrono::system_clock::time_point GetLastModified(_UNUSED StorageItemType nType,
                                                          _UNUSED const std::wstring& sKey) noexcept override
    {
        return std::chrono::system_clock::time_point();
    }

    std::unique_ptr<TextReader> ReadText(StorageItemType nType, const std::wstring& sKey) override
    {
        const auto pText = GetText(nType, sKey, false);
        if (pText == nullptr)
            return std::unique_ptr<TextReader>();

        auto pReader = std::make_unique<ra::services::impl::StringTextReader>(*pText);
        return std::unique_ptr<TextReader>(pReader.release());
    }

    std::unique_ptr<TextWriter> WriteText(StorageItemType nType, const std::wstring& sKey) override
    {
        const gsl::not_null<std::string*> pText{gsl::make_not_null(GetText(nType, sKey, true))};
        pText->clear();

        auto pWriter = std::make_unique<ra::services::impl::StringTextWriter>(*pText);
        return std::unique_ptr<TextWriter>(pWriter.release());
    }

    std::unique_ptr<TextWriter> AppendText(StorageItemType nType, const std::wstring& sKey) override
    {
        const gsl::not_null<std::string*> pText{gsl::make_not_null(GetText(nType, sKey, true))};
        auto pWriter = std::make_unique<ra::services::impl::StringTextWriter>(*pText);
        return std::unique_ptr<TextWriter>(pWriter.release());
    }

private:
    std::string* GetText(StorageItemType nType, const std::wstring& sKey, bool bCreateIfMissing) const
    {
        auto pMap = m_mStoredData.find(nType);
        if (pMap == m_mStoredData.end())
        {
            if (!bCreateIfMissing)
                return nullptr;

            m_mStoredData.insert({nType, {}});
            pMap = m_mStoredData.find(nType);
        }

        auto pIter = pMap->second.find(sKey);
        if (pIter == pMap->second.end())
        {
            if (!bCreateIfMissing)
                return nullptr;

            pMap->second.insert({sKey, ""});
            pIter = pMap->second.find(sKey);
        }

        return &pIter->second;
    }

    ra::services::ServiceLocator::ServiceOverride<ra::services::ILocalStorage> m_Override;
    mutable std::unordered_map<StorageItemType, std::unordered_map<std::wstring, std::string>> m_mStoredData;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_LOCALSTORAGE_HH
