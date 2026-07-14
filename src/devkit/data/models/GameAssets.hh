#ifndef RA_DATA_MODELS_GAMEASSETS_HH
#define RA_DATA_MODELS_GAMEASSETS_HH
#pragma once

#include "data/DataModelCollection.hh"

#include "data/models/AchievementModel.hh"
#include "data/models/AchievementSetModel.hh"
#include "data/models/MemoryNotesModel.hh"
#include "data/models/LeaderboardModel.hh"
#include "data/models/MemoryRegionsModel.hh"
#include "data/models/RichPresenceModel.hh"

#include <set>

struct rc_api_fetch_game_sets_response_t;

namespace ra {
namespace data {
namespace models {

class GameAssets : public DataModelCollection<AssetModelBase>
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
    ra::data::models::AchievementModel* FindAchievement(uint32_t nId)
    {
        return dynamic_cast<ra::data::models::AchievementModel*>(FindAsset(ra::data::models::AssetType::Achievement, nId));
    }

    /// <summary>
    /// Finds the achievement asset for the specified ID.
    /// </summary>
    const ra::data::models::AchievementModel* FindAchievement(uint32_t nId) const
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
    ra::data::models::LeaderboardModel* FindLeaderboard(uint32_t nId)
    {
        return dynamic_cast<ra::data::models::LeaderboardModel*>(FindAsset(ra::data::models::AssetType::Leaderboard, nId));
    }

    /// <summary>
    /// Finds the leaderboard asset for the specified ID.
    /// </summary>
    const ra::data::models::LeaderboardModel* FindLeaderboard(uint32_t nId) const
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
    /// Finds the memory notes asset.
    /// </summary>
    ra::data::models::MemoryNotesModel* FindMemoryNotes()
    {
        return dynamic_cast<ra::data::models::MemoryNotesModel*>(FindAsset(ra::data::models::AssetType::MemoryNotes, 0));
    }

    /// <summary>
    /// Finds the memory notes asset.
    /// </summary>
    const ra::data::models::MemoryNotesModel* FindMemoryNotes() const
    {
        return dynamic_cast<const ra::data::models::MemoryNotesModel*>(FindAsset(ra::data::models::AssetType::MemoryNotes, 0));
    }

    /// <summary>
    /// Finds the memory regions asset.
    /// </summary>
    ra::data::models::MemoryRegionsModel* FindMemoryRegions()
    {
        return dynamic_cast<ra::data::models::MemoryRegionsModel*>(FindAsset(ra::data::models::AssetType::MemoryRegions, 0));
    }

    /// <summary>
    /// Finds the memory regions asset.
    /// </summary>
    const ra::data::models::MemoryRegionsModel* FindMemoryRegions() const
    {
        return dynamic_cast<const ra::data::models::MemoryRegionsModel*>(FindAsset(ra::data::models::AssetType::MemoryRegions, 0));
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

    /// <summary>
    /// Gets the achievements sets for the loaded game.
    /// </summary>
    const DataModelCollection<AchievementSetModel>& AchievementSets() const noexcept { return m_vAchievementSets; }

    /// <summary>
    /// Initializes the achievement sets collection.
    /// </summary>
    void InitializeSubsets(const rc_api_fetch_game_sets_response_t* game_data_response, bool bSubsetWithoutBase);

    /// <summary>
    /// Resets the achievement sets collection.
    /// </summary>
    void ClearAchievementSets() { m_vAchievementSets.Clear(); }

    /// <summary>
    /// The unique identifier of the first local asset.
    /// Anything of this value or higher has not been committed to the server.
    /// </summary>
    static constexpr uint32_t FirstLocalId = 111000001;

    /// <summary>
    /// Resets the counter for new local assets.
    /// </summary>
    void ResetLocalId() noexcept { m_nNextLocalId = FirstLocalId; }

    /// <summary>
    /// Returns <c>true</c> if any assets have their PauseOnX attributes set.
    /// </summary>
    bool HasPauseOnXAssets() const noexcept;

    /// <summary>
    /// Gets the achievements where PauseOnReset is set.
    /// </summary>
    void GetPauseOnResetAchievements(std::vector<const ra::data::models::AchievementModel*>& vAchievements) const;

    /// <summary>
    /// Gets the leaderboards where PauseOnReset is set.
    /// </summary>
    void GetPauseOnResetLeaderboards(std::vector<const ra::data::models::LeaderboardModel*>& vLeaderboards) const;

    /// <summary>
    /// Gets the leaderboards where PauseOnTrigger is set.
    /// </summary>
    void GetPauseOnTriggerLeaderboards(std::vector<const ra::data::models::LeaderboardModel*>& vLeaderboards) const;

protected:
    bool IsWatching() const noexcept override { return true; }

    void OnModelValueChanged(gsl::index nIndex, const BoolModelProperty::ChangeArgs& args) override;
    void OnModelValueChanged(gsl::index nIndex, const IntModelProperty::ChangeArgs& args) override;

    void OnBeforeItemRemoved(ModelBase& pModel) override;

    uint32_t m_nNextLocalId = FirstLocalId;

    DataModelCollection<AchievementSetModel> m_vAchievementSets;
    std::set<uint32_t> m_vPauseOnResetAchievementIds;
    std::set<uint32_t> m_vPauseOnResetLeaderboardIds;
    std::set<uint32_t> m_vPauseOnTriggerLeaderboardIds;
};

} // namespace models
} // namespace data
} // namespace ra

#endif // !RA_DATA_MODELS_GAMEASSETS_HH
