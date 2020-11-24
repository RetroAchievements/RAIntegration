#ifndef RA_DATA_LOCAL_BADGES_MODEL_H
#define RA_DATA_LOCAL_BADGES_MODEL_H
#pragma once

#include "AssetModelBase.hh"

namespace ra {
namespace data {
namespace models {

class LocalBadgesModel : public AssetModelBase
{
public:
	LocalBadgesModel() noexcept;
	~LocalBadgesModel();

	void AddReference(const std::wstring& sBadgeName, bool bCommitted);
	void RemoveReference(const std::wstring& sBadgeName, bool bCommitted);

	void Commit(const std::wstring& sPreviousBadgeName, const std::wstring& sNewBadgeName);
	bool NeedsSerialized() const;

	void Serialize(ra::services::TextWriter& pWriter) const override;
	bool Deserialize(ra::Tokenizer& pTokenizer) override;

	int GetReferenceCount(const std::wstring& sBadgeName) const;
	void DeleteUncommittedBadges();

private:
	struct BadgeReferenceCount
	{
		int nUncommittedCount;
		int nCommittedCount;
	};

	mutable std::map<std::wstring, BadgeReferenceCount> m_mReferences;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_LOCAL_BADGES_MODEL_H
