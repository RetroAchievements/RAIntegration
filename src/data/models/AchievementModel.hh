#ifndef RA_DATA_ACHIEVEMENT_MODEL_H
#define RA_DATA_ACHIEVEMENT_MODEL_H
#pragma once

#include "data/models/AssetModelBase.hh"

struct rc_client_achievement_info_t;
struct rc_trigger_t;

namespace ra {
namespace data {
namespace models {

enum AchievementType
{
    None = 0,
    Missable,
    Progression,
    Win,
};

class AchievementModel : public AssetModelBase
{
public:
    AchievementModel() noexcept;

    /// <summary>
    /// The <see cref="ModelProperty" /> for the achievement points.
    /// </summary>
    static const IntModelProperty PointsProperty;

    /// <summary>
    /// Gets the points for the achievement.
    /// </summary>
    int GetPoints() const { return GetValue(PointsProperty); }

    /// <summary>
    /// Sets the points for the achievement.
    /// </summary>
    void SetPoints(int nValue) { SetValue(PointsProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the achievement type.
    /// </summary>
    static const IntModelProperty AchievementTypeProperty;

    /// <summary>
    /// Gets the type of the achievement.
    /// </summary>
    AchievementType GetAchievementType() const { return ra::itoe<AchievementType>(GetValue(AchievementTypeProperty)); }

    /// <summary>
    /// Sets the type for the achievement.
    /// </summary>
    void SetAchievementType(AchievementType nValue) { SetValue(AchievementTypeProperty, ra::etoi(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the badge.
    /// </summary>
    static const StringModelProperty BadgeProperty;

    /// <summary>
    /// Gets the name of the badge associated to the achievement.
    /// </summary>
    const std::wstring& GetBadge() const { return GetValue(BadgeProperty); }

    /// <summary>
    /// Sets the name of the badge associated to the achievement.
    /// </summary>
    void SetBadge(const std::wstring& sValue) { SetValue(BadgeProperty, sValue); }

    /// <summary>
    /// Determines if the badge has been committed.
    /// </summary>
    bool IsBadgeCommitted() const
    {
        return (GetLocalValue(BadgeProperty) == GetValue(BadgeProperty));
    }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the emulator should be paused when the trigger is reset.
    /// </summary>
    static const BoolModelProperty PauseOnResetProperty;

    /// <summary>
    /// Gets whether or not the emulator should be paused when the trigger is reset.
    /// </summary>
    bool IsPauseOnReset() const { return GetValue(PauseOnResetProperty); }

    /// <summary>
    /// Sets whether or not the emulator should be paused when the trigger is reset.
    /// </summary>
    void SetPauseOnReset(bool nValue) { SetValue(PauseOnResetProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the emulator should be paused when the achievement triggers.
    /// </summary>
    static const BoolModelProperty PauseOnTriggerProperty;

    /// <summary>
    /// Gets whether or not the emulator should be paused when the achievement triggers.
    /// </summary>
    bool IsPauseOnTrigger() const { return GetValue(PauseOnTriggerProperty); }

    /// <summary>
    /// Sets whether or not the emulator should be paused when the achievement triggers.
    /// </summary>
    void SetPauseOnTrigger(bool nValue) { SetValue(PauseOnTriggerProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the trigger version.
    /// </summary>
    static const IntModelProperty TriggerProperty;

    /// <summary>
    /// Gets the trigger definition.
    /// </summary>
    const std::string& GetTrigger() const { return GetAssetDefinition(m_pTrigger); }
    
    /// <summary>
    /// Sets the trigger definition.
    /// </summary>
    void SetTrigger(const std::string& sTrigger) { SetAssetDefinition(m_pTrigger, sTrigger); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the rich presence at the time of the unlock.
    /// </summary>
    static const StringModelProperty UnlockRichPresenceProperty;

    /// <summary>
    /// Gets the trigger definition.
    /// </summary>
    const std::wstring& GetUnlockRichPresence() const { return GetValue(UnlockRichPresenceProperty); }

    /// <summary>
    /// Sets the trigger definition.
    /// </summary>
    /// <remarks>For unit testing - should not be called in the code</remarks>
    void SetUnlockRichPresence(const std::wstring& sValue) { SetValue(UnlockRichPresenceProperty, sValue); }

    /// <summary>
    /// Gets whether the asset should not be displayed in the asset list.
    /// </summary>
    /// <returns><c>true</c> to display the asset in the asset list, <c>false</c> otherwise.</returns>
    bool IsShownInList() const override;

    void Activate() override;
    void Deactivate() override;

    void DoFrame() override;

    void Serialize(ra::services::TextWriter& pWriter) const override;
    bool Deserialize(ra::util::Tokenizer& pTokenizer) override;

    /// <summary>
    /// Associates a published achievement to the model.
    /// </summary>
    /// <param name="pAchievement">Reference to the published achievement</param>
    /// <param name="sTrigger">The published achievement definition</param>
    void InitializeFromPublishedAchievement(const struct rc_client_achievement_info_t& pAchievement, const std::string& sTrigger);

    /// <summary>
    /// Associates a local achievement info to the model. Any changes made to the model will be updated in this runtime model
    /// </summary>
    void SetLocalAchievementInfo(struct rc_client_achievement_info_t& pAchievement);

    /// <summary>
    /// Updates all properties on the local achievement info from the current state of the model.
    /// </summary>
    void SyncToLocalAchievementInfo();

    /// <summary>
    /// Gets the attached rc_client_achievement_info_t.
    /// </summary>
    const struct rc_client_achievement_info_t* GetRuntimeAchievementInfo() const noexcept { return m_pAchievementInfo; }

    /// <summary>
    /// Gets the attached rc_trigger_t.
    /// </summary>
    const struct rc_trigger_t* GetRuntimeTrigger() const;
    struct rc_trigger_t* GetMutableRuntimeTrigger();

    /// <summary>
    /// Gets the list of acceptable point values.
    /// </summary>
    static const std::array<int, 10>& ValidPoints() noexcept
    {
        return s_vValidPoints;
    }

    /// <summary>
    /// Gets the maximum size of the serialized trigger definition.
    /// </summary>
    static constexpr size_t MaxSerializedLength = 65535;

protected:
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;

    void CommitTransaction() override;

    bool ValidateAsset(std::wstring& sErrorMessage) override;

private:
    static const std::array<int, 10> s_vValidPoints;

    void HandleStateChanged(AssetState nOldState, AssetState nNewState);

    void SyncIDToRuntime() const;
    void SyncTitleToRuntime();
    void SyncDescriptionToRuntime();
    void SyncBadgeToRuntime() const;
    void SyncPointsToRuntime() const;
    void SyncAchievementTypeToRuntime() const;
    void SyncCategoryToRuntime() const;
    void SyncStateToRuntime() const;
    void SyncTriggerToRuntime();

    void SyncStateFromRuntime(uint8_t nState);

    void ParseTrigger() const;
    AssetDefinition m_pTrigger;

    // the current achievement information
    struct rc_client_achievement_info_t* m_pAchievementInfo = nullptr;

    // the original achievement information received from the server
    const struct rc_client_achievement_info_t* m_pPublishedAchievementInfo = nullptr;

    // temporary buffers for local changes
    std::string m_sTitleBuffer;
    std::string m_sDescriptionBuffer;
    mutable std::unique_ptr<uint8_t[]> m_pTriggerBuffer;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_ACHIEVEMENT_MODEL_H
