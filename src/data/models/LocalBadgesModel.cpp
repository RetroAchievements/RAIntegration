#include "LocalBadgesModel.hh"

#include "data\context\GameContext.hh"

#include "services\IFileSystem.hh"
#include "services\ServiceLocator.hh"

#include "ui\ImageReference.hh"

namespace ra {
namespace data {
namespace models {

LocalBadgesModel::LocalBadgesModel() noexcept
{
    GSL_SUPPRESS_F6 SetValue(TypeProperty, ra::etoi(AssetType::LocalBadges));
    GSL_SUPPRESS_F6 SetValue(CategoryProperty, ra::etoi(AssetCategory::Local));
    GSL_SUPPRESS_F6 SetName(L"Local Badges");
}

LocalBadgesModel::~LocalBadgesModel()
{
    if (ra::services::ServiceLocator::Exists<ra::services::IFileSystem>())
    {
        GSL_SUPPRESS_F6 DeleteUncommittedBadges();
    }
}

void LocalBadgesModel::DeleteUncommittedBadges()
{
    std::vector<std::wstring> vToDelete;

    for (const auto& pIter : m_mReferences)
    {
        if (pIter.second.nCommittedCount == 0)
            vToDelete.push_back(pIter.first);
    }

    if (!vToDelete.empty())
    {
        const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
        for (const auto& pFile : vToDelete)
            pFileSystem.DeleteFile(pFile);
    }
}

void LocalBadgesModel::AddReference(const std::wstring& sBadgeName, bool bCommitted)
{
    const auto pIter = m_mReferences.find(sBadgeName);
    if (pIter != m_mReferences.end())
    {
        if (bCommitted)
            ++pIter->second.nCommittedCount;
        else
            ++pIter->second.nUncommittedCount;
    }
    else
    {
        BadgeReferenceCount pCount{};
        pCount.nCommittedCount = bCommitted ? 1 : 0;
        pCount.nUncommittedCount = bCommitted ? 0 : 1;

        m_mReferences.insert_or_assign(sBadgeName, pCount);
    }
}

void LocalBadgesModel::RemoveReference(const std::wstring& sBadgeName, bool bCommitted)
{
    const auto pIter = m_mReferences.find(sBadgeName);
    if (pIter != m_mReferences.end())
    {
        if (bCommitted)
        {
            if (pIter->second.nCommittedCount > 0)
                --pIter->second.nCommittedCount;
        }
        else
        {
            if (pIter->second.nUncommittedCount > 0)
                --pIter->second.nUncommittedCount;
        }
    }
}

int LocalBadgesModel::GetReferenceCount(const std::wstring& sBadgeName, bool bCommitted) const
{
    const auto pIter = m_mReferences.find(sBadgeName);
    if (pIter != m_mReferences.end())
        return bCommitted ? pIter->second.nCommittedCount : pIter->second.nUncommittedCount;

    return 0;
}

void LocalBadgesModel::Commit(const std::wstring& sPreviousBadgeName, const std::wstring& sNewBadgeName)
{
    // the old badge will have already been dereferenced by RemoveReference. if the reference count is 0
    // go ahead and get rid ofit.
    auto pIter = m_mReferences.find(sPreviousBadgeName);
    if (pIter != m_mReferences.end())
    {
        if (pIter->second.nCommittedCount == 0 && pIter->second.nUncommittedCount == 0)
        {
            const auto& pImageRepository = ra::services::ServiceLocator::Get<ra::ui::IImageRepository>();
            const auto sFilename = pImageRepository.GetFilename(ra::ui::ImageType::Badge, ra::Narrow(pIter->first));
            const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
            pFileSystem.DeleteFile(sFilename);
            m_mReferences.erase(pIter);
        }
    }

    // update the new badge reference from uncommitted to committed
    pIter = m_mReferences.find(sNewBadgeName);
    if (pIter != m_mReferences.end())
    {
        Expects(pIter->second.nUncommittedCount > 0);
        --pIter->second.nUncommittedCount;
        ++pIter->second.nCommittedCount;
    }
}

} // namespace models
} // namespace data
} // namespace ra
