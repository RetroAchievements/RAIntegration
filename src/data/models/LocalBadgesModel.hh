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
	LocalBadgesModel(const LocalBadgesModel&) noexcept = delete;
	LocalBadgesModel& operator=(const LocalBadgesModel&) noexcept = delete;
	LocalBadgesModel(LocalBadgesModel&&) noexcept = delete;
	LocalBadgesModel& operator=(LocalBadgesModel&&) noexcept = delete;

	void AddReference(const std::wstring& sBadgeName, bool bCommitted);
	void RemoveReference(const std::wstring& sBadgeName, bool bCommitted);

	void Commit(const std::wstring& sPreviousBadgeName, const std::wstring& sNewBadgeName);
	bool NeedsSerialized() const noexcept { return false; }

	void Serialize(ra::services::TextWriter&) const noexcept override {}
	bool Deserialize(ra::Tokenizer&) noexcept override { return true; }

	int GetReferenceCount(const std::wstring& sBadgeName, bool bCommitted) const;
	void DeleteUncommittedBadges();

private:
	struct BadgeReferenceCount
	{
		int nUncommittedCount; // references not written to XXX-user.txt
		int nCommittedCount;   // references written to XXX-user.txt
	};

	mutable std::map<std::wstring, BadgeReferenceCount> m_mReferences;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_LOCAL_BADGES_MODEL_H
