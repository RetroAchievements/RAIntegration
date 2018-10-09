#ifndef RA_SERVICES_MOCK_LOCALSTORAGE_HH
#define RA_SERVICES_MOCK_LOCALSTORAGE_HH
#pragma once

#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"
#include "services\impl\StringTextReader.hh"
#include "services\impl\StringTextWriter.hh"

#include <unordered_map>

namespace ra {
namespace services {
namespace mocks {

class MockLocalStorage : public ILocalStorage
{
public:
    MockLocalStorage() noexcept
        : m_Override(this)
    {
    }

    void MockStoredData(ra::services::StorageItemType nType, const std::wstring& sKey, const std::string& sContents)
    {
        auto pMap = m_mStoredData.insert({ nType, {} });
        pMap.first->second.insert_or_assign(sKey, sContents);
    }

    const std::string& GetStoredData(ra::services::StorageItemType nType, const std::wstring& sKey)
    {
        auto pMap = m_mStoredData.find(nType);
        if (pMap != m_mStoredData.end())
        {
            auto pIter = pMap->second.find(sKey);
            if (pIter != pMap->second.end())
                return pIter->second;
        }

        static const std::string sEmpty;
        return sEmpty;
    }

    std::unique_ptr<TextReader> ReadText(StorageItemType nType, const std::wstring& sKey) override
    {
        auto pMap = m_mStoredData.find(nType);
        if (pMap == m_mStoredData.end())
            return std::unique_ptr<TextReader>();

        auto pIter = pMap->second.find(sKey);
        if (pIter == pMap->second.end())
            return std::unique_ptr<TextReader>();

        auto pReader = std::make_unique<ra::services::impl::StringTextReader>(pIter->second);
        return std::unique_ptr<TextReader>(pReader.release());
    }

    std::unique_ptr<TextWriter> WriteText(StorageItemType nType, const std::wstring& sKey) override
    {
        auto pMap = m_mStoredData.insert({ nType, {} });
        auto pIter = pMap.first->second.insert_or_assign(sKey, "");
        auto pWriter = std::make_unique<ra::services::impl::StringTextWriter>(pIter.first->second);
        return std::unique_ptr<TextWriter>(pWriter.release());
    }

private:
    ra::services::ServiceLocator::ServiceOverride<ra::services::ILocalStorage> m_Override;
    mutable std::unordered_map<StorageItemType, std::unordered_map<std::wstring, std::string>> m_mStoredData;
};

} // namespace mocks
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_MOCK_LOCALSTORAGE_HH
