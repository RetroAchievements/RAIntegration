#include "LocalBadgesModel.hh"

#include "data\context\GameContext.hh"

#include "services\IFileSystem.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace data {
namespace models {

LocalBadgesModel::LocalBadgesModel() noexcept
{
    GSL_SUPPRESS_F6 SetValue(TypeProperty, ra::etoi(AssetType::LocalBadges));
}

LocalBadgesModel::~LocalBadgesModel()
{
    if (ra::services::ServiceLocator::Exists<ra::services::IFileSystem>())
        DeleteUncommittedBadges();
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
    auto pIter = m_mReferences.find(sBadgeName);
    if (pIter != m_mReferences.end())
    {
        if (bCommitted)
            ++pIter->second.nCommittedCount;
        else
            ++pIter->second.nUncommittedCount;
    }
    else
    {
        // a committed entry should exist in m_mReferences from the Deserialize call
        // or from a previously uncommitted entry
        Expects(!bCommitted); 

        BadgeReferenceCount pCount;
        pCount.nCommittedCount = 0;
        pCount.nUncommittedCount = 1;

        m_mReferences.insert_or_assign(sBadgeName, pCount);
    }
}

void LocalBadgesModel::RemoveReference(const std::wstring& sBadgeName, bool bCommitted)
{
    auto pIter = m_mReferences.find(sBadgeName);
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

int LocalBadgesModel::GetReferenceCount(const std::wstring& sBadgeName) const
{
    auto pIter = m_mReferences.find(sBadgeName);
    if (pIter != m_mReferences.end())
        return pIter->second.nCommittedCount + pIter->second.nUncommittedCount;

    return 0;
}

void LocalBadgesModel::Commit(const std::wstring& sPreviousBadgeName, const std::wstring& sNewBadgeName)
{
    auto pIter = m_mReferences.find(sPreviousBadgeName);
    if (pIter != m_mReferences.end())
    {
        Expects(pIter->second.nCommittedCount > 0);
        if (--pIter->second.nCommittedCount == 0 && pIter->second.nUncommittedCount == 0)
        {
            const auto& pFileSystem = ra::services::ServiceLocator::Get<ra::services::IFileSystem>();
            pFileSystem.DeleteFile(pIter->first);
            m_mReferences.erase(pIter);
        }
    }

    pIter = m_mReferences.find(sNewBadgeName);
    if (pIter != m_mReferences.end())
    {
        Expects(pIter->second.nUncommittedCount > 0);
        --pIter->second.nUncommittedCount;
        ++pIter->second.nCommittedCount;
    }
}

bool LocalBadgesModel::NeedsSerialized() const
{
    // ASSERT: NeedsSerialized and Serialize are called after each asset has had a chance to call Commit
    for (const auto& pIter : m_mReferences)
    {
        if (pIter.second.nCommittedCount != 0)
            return true;
    }

    return false;
}

void LocalBadgesModel::Serialize(ra::services::TextWriter& pWriter) const
{
    pWriter.Write("b");
    for (const auto& pIter : m_mReferences)
    {
        if (pIter.second.nCommittedCount > 0)
        {
            const auto nIndex = pIter.first.find('-');
            pWriter.Write(":");
            pWriter.Write(pIter.first.substr(nIndex + 1));
            pWriter.Write("=");
            pWriter.Write(std::to_string(pIter.second.nCommittedCount));
        }
    }
}

bool LocalBadgesModel::Deserialize(ra::Tokenizer& pTokenizer)
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    const auto sPrefix = ra::StringPrintf(L"local\\%u-", pGameContext.GameId());

    m_mReferences.clear();

    do
    {
        const auto sFilename = pTokenizer.ReadTo('=');
        pTokenizer.Advance();

        const auto nCount = pTokenizer.ReadNumber();
        if (nCount > 0)
        {
            BadgeReferenceCount pCount;
            pCount.nCommittedCount = nCount;
            pCount.nUncommittedCount = 0;

            m_mReferences.insert_or_assign(sPrefix + ra::Widen(sFilename), pCount);
        }
    } while (pTokenizer.Consume(':'));

    return true;
}

} // namespace models
} // namespace data
} // namespace ra
