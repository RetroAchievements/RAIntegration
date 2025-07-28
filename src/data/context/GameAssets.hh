#ifndef RA_DATA_GAMEASSETS_HH
#define RA_DATA_GAMEASSETS_HH
#pragma once

#include "data\DataModelCollection.hh"
#include "data\Types.hh"

#include "data\models\AchievementModel.hh"
#include "data\models\CodeNotesModel.hh"
#include "data\models\LeaderboardModel.hh"
#include "data\models\RichPresenceModel.hh"

namespace ra {
namespace data {
namespace context {

class GameAssets : public ra::data::DataModelCollection<ra::data::models::AssetModelBase>
{
public:
    GSL_SUPPRESS_F6 GameAssets() = default;
    virtual ~GameAssets() noexcept = default;
    GameAssets(const GameAssets&) noexcept = delete;
    GameAssets& operator=(const GameAssets&) noexcept = delete;
    GameAssets(GameAssets&&) noexcept = delete;
    GameAssets& operator=(GameAssets&&) noexcept = delete;

    /// <summary>
    /// Finds the asset of the specified type for the specified ID.
    /// </summary>
    ra::data::models::AssetModelBase* FindAsset(ra::data::models::AssetType nType, uint32_t nId);

    /// <summary>
    /// Finds the asset of the specified type for the specified ID.
    /// </summary>
    const ra::data::models::AssetModelBase* FindAsset(ra::data::models::AssetType nType, uint32_t nId) const;

    /// <summary>
    /// Finds the achievement asset for the specified ID.
    /// </summary>
    ra::data::models::AchievementModel* FindAchievement(ra::AchievementID nId)
    {
        return dynamic_cast<ra::data::models::AchievementModel*>(FindAsset(ra::data::models::AssetType::Achievement, nId));
    }

    /// <summary>
    /// Finds the achievement asset for the specified ID.
    /// </summary>
    const ra::data::models::AchievementModel* FindAchievement(ra::AchievementID nId) const
    {
        return dynamic_cast<const ra::data::models::AchievementModel*>(FindAsset(ra::data::models::AssetType::Achievement, nId));
    }

    /// <summary>
    /// Creates a new achievement asset.
    /// </summary>
    ra::data::models::AchievementModel& NewAchievement();

    /// <summary>
    /// Finds the leaderboard asset for the specified ID.
    /// </summary>
    ra::data::models::LeaderboardModel* FindLeaderboard(ra::LeaderboardID nId)
    {
        return dynamic_cast<ra::data::models::LeaderboardModel*>(FindAsset(ra::data::models::AssetType::Leaderboard, nId));
    }

    /// <summary>
    /// Finds the leaderboard asset for the specified ID.
    /// </summary>
    const ra::data::models::LeaderboardModel* FindLeaderboard(ra::AchievementID nId) const
    {
        return dynamic_cast<const ra::data::models::LeaderboardModel*>(FindAsset(ra::data::models::AssetType::Leaderboard, nId));
    }

    /// <summary>
    /// Creates a new leaderboard asset.
    /// </summary>
    ra::data::models::LeaderboardModel& NewLeaderboard();

    /// <summary>
    /// Finds the rich presence asset.
    /// </summary>
    ra::data::models::RichPresenceModel* FindRichPresence()
    {
        return dynamic_cast<ra::data::models::RichPresenceModel*>(FindAsset(ra::data::models::AssetType::RichPresence, 0));
    }

    /// <summary>
    /// Finds the rich presence asset.
    /// </summary>
    const ra::data::models::RichPresenceModel* FindRichPresence() const
    {
        return dynamic_cast<const ra::data::models::RichPresenceModel*>(FindAsset(ra::data::models::AssetType::RichPresence, 0));
    }

    /// <summary>
    /// Finds the code notes asset.
    /// </summary>
    ra::data::models::CodeNotesModel* FindCodeNotes()
    {
        return dynamic_cast<ra::data::models::CodeNotesModel*>(FindAsset(ra::data::models::AssetType::CodeNotes, 0));
    }

    /// <summary>
    /// Finds the code notes asset.
    /// </summary>
    const ra::data::models::CodeNotesModel* FindCodeNotes() const
    {
        return dynamic_cast<const ra::data::models::CodeNotesModel*>(FindAsset(ra::data::models::AssetType::CodeNotes, 0));
    }

    /// <summary>
    /// Determines if any Core assets exist.
    /// </summary>
    bool HasCoreAssets() const
    {
        return (MostPublishedAssetCategory() == ra::data::models::AssetCategory::Core);
    }

    /// <summary>
    /// Determines the most published asset category.
    /// </summary>
    /// <remarks>Core > Unpublished > Local > None</remarks>
    ra::data::models::AssetCategory MostPublishedAssetCategory() const;

    /// <summary>
    /// Writes the local assets files.
    /// </summary>
    /// <param name="vAssetsToSave">List of assets to write, empty to write all assets.</param>
    void SaveAssets(const std::vector<ra::data::models::AssetModelBase*>& vAssetsToSave);

    /// <summary>
    /// Reloads assets from the local assets files.
    /// </summary>
    /// <param name="vAssetsToReload">List of assets to reload, empty to reload all assets.</param>
    void ReloadAssets(const std::vector<ra::data::models::AssetModelBase*>& vAssetsToReload);

    static constexpr uint32_t FirstLocalId = 111000001;
    void ResetLocalId() noexcept { m_nNextLocalId = FirstLocalId; }

    static constexpr uint32_t LocalSubsetId = 0xFFFFFFFF;

protected:
    void OnBeforeItemRemoved(ModelBase& pModel) override;

    uint32_t m_nNextLocalId = FirstLocalId;
};

} // namespace context
} // namespace data
} // namespace ra

#endif // !RA_DATA_GAMEASSETS_HH
