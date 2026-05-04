#ifndef RA_DATA_MODELS_LOCALBADGESMODEL_H
#define RA_DATA_MODELS_LOCALBADGESMODEL_H
#pragma once

#include "data/models/AssetModelBase.hh"

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

	bool IsShownInList() const noexcept override { return false; }
	void Serialize(ra::services::TextWriter&) const noexcept override {}
	bool Deserialize(ra::util::Tokenizer&) noexcept override { return true; }

	/// <summary>
	/// Adds a reference to a local badge.
	/// </summary>
	/// <param name="sBadgeName">The name of the local badge.</param>
	/// <param name="bCommitted"><c>true</c> if the reference has been committed.</param>
	/// <remarks>An uncommitted reference may be deleted at exit.</remarks>
	void AddReference(const std::wstring& sBadgeName, bool bCommitted);

	/// <summary>
	/// Adds a reference to a local badge.
	/// </summary>
	/// <param name="sBadgeName">The name of the local badge.</param>
	/// <param name="bCommitted"><c>true</c> if the reference has been committed.</param>
	void RemoveReference(const std::wstring& sBadgeName, bool bCommitted);

	/// <summary>
	/// Gets the number of references to a local badge.
	/// </summary>
	/// <param name="sBadgeName">The name of the local badge.</param>
	/// <param name="bCommitted"><c>true</c> to look for committed references.</param>
	int GetReferenceCount(const std::wstring& sBadgeName, bool bCommitted) const;

	/// <summary>
	/// Deletes any badges not associated to committed assets.
	/// </summary>
	void DeleteUncommittedBadges();

	/// <summary>
	/// Converts an uncommitted badge name to a committed badge name.
	/// </summary>
	void Commit(const std::wstring& sPreviousBadgeName, const std::wstring& sNewBadgeName);

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

#endif RA_DATA_MODELS_LOCALBADGESMODEL_H
