#ifndef RA_DATA_ACHIEVEMENT_MODEL_H
#define RA_DATA_ACHIEVEMENT_MODEL_H
#pragma once

#include "data/models/AssetModelBase.hh"

#include "CapturedTriggerHits.hh"

struct rc_client_achievement_info_t;

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
    /// Gets when the achievement was unlocked.
    /// </summary>
    std::chrono::system_clock::time_point GetUnlockTime() const noexcept { return m_tUnlock; }

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
    bool Deserialize(ra::Tokenizer& pTokenizer) override;

    // Attaches an rc_client_achievement_info_t and populates the model from it.
    void Attach(struct rc_client_achievement_info_t& pAchievement,
        AssetCategory nCategory, const std::string& sTrigger);
    // Replaces the attached rc_client_achievement_info_t. Does not update model or source.
    void ReplaceAttached(struct rc_client_achievement_info_t& pAchievement);
    // Attaches an rc_client_achievement_info_t and populates it from the model.
    void AttachAndInitialize(struct rc_client_achievement_info_t& pAchievement);
    // Gets the attached rc_client_achievement_info_t.
    const struct rc_client_achievement_info_t* GetAttached() const noexcept { return m_pAchievement; }

    const CapturedTriggerHits& GetCapturedHits() const noexcept { return m_pCapturedTriggerHits; }
    void ResetCapturedHits() noexcept { m_pCapturedTriggerHits.Reset(); }

    static const std::array<int, 10>& ValidPoints() noexcept
    {
        return s_vValidPoints;
    }

    static constexpr size_t MaxSerializedLength = 65535;

protected:
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;

    void CommitTransaction() override;

    bool ValidateAsset(std::wstring& sErrorMessage) override;

private:
    static const std::array<int, 10> s_vValidPoints;

    void HandleStateChanged(AssetState nOldState, AssetState nNewState);
    void SyncID();
    void SyncTitle();
    void SyncDescription();
    void SyncBadge();
    void SyncPoints();
    void SyncAchievementType();
    void SyncCategory();
    void SyncState();
    void SyncTrigger();

    AssetDefinition m_pTrigger;
    std::chrono::system_clock::time_point m_tUnlock;

    struct rc_client_achievement_info_t* m_pAchievement = nullptr;
    std::string m_sTitleBuffer;
    std::string m_sDescriptionBuffer;

    CapturedTriggerHits m_pCapturedTriggerHits;
    bool m_bCaptureTrigger = false;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_ACHIEVEMENT_MODEL_H
