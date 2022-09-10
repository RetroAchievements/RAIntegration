#ifndef RA_DATA_ACHIEVEMENT_MODEL_H
#define RA_DATA_ACHIEVEMENT_MODEL_H
#pragma once

#include "AssetModelBase.hh"

#include "CapturedTriggerHits.hh"

namespace ra {
namespace data {
namespace models {

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

    void Activate() override;
    void Deactivate() override;

    void DoFrame() override;

    void Serialize(ra::services::TextWriter& pWriter) const override;
    bool Deserialize(ra::Tokenizer& pTokenizer) override;

    const CapturedTriggerHits& GetCapturedHits() const noexcept { return m_pCapturedTriggerHits; }
    void ResetCapturedHits() noexcept { m_pCapturedTriggerHits.Reset(); }

    static const std::array<int, 10>& ValidPoints() noexcept
    {
        return s_vValidPoints;
    }

protected:
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;

    void CommitTransaction() override;

    bool ValidateAsset(std::wstring& sErrorMessage) override;

private:
    static const std::array<int, 10> s_vValidPoints;

    void HandleStateChanged(AssetState nOldState, AssetState nNewState);

    AssetDefinition m_pTrigger;
    std::chrono::system_clock::time_point m_tUnlock;

    CapturedTriggerHits m_pCapturedTriggerHits;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_ACHIEVEMENT_MODEL_H
