#ifndef RA_DATA_LEADERBOARD_MODEL_H
#define RA_DATA_LEADERBOARD_MODEL_H
#pragma once

#include "AssetModelBase.hh"

#include "CapturedTriggerHits.hh"

#include "data\Types.hh"

struct rc_client_leaderboard_info_t;

namespace ra {
namespace data {
namespace models {

class LeaderboardModel : public AssetModelBase
{
public:
    LeaderboardModel() noexcept;

    enum class LeaderboardParts
    {
        None = 0x00,
        Start = 0x01,
        Submit = 0x02,
        Cancel = 0x04,
        Value = 0x08
    };

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the emulator should be paused when the trigger is reset.
    /// </summary>
    static const IntModelProperty PauseOnResetProperty;

    /// <summary>
    /// Sets whether or not the emulator should be paused when the trigger is reset.
    /// </summary>
    LeaderboardParts GetPauseOnReset() const { return ra::itoe<LeaderboardParts>(GetValue(PauseOnResetProperty)); }

    /// <summary>
    /// Sets whether or not the emulator should be paused when the trigger is reset.
    /// </summary>
    void SetPauseOnReset(LeaderboardParts nValue) { SetValue(PauseOnResetProperty, ra::etoi(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the emulator should be paused when a part triggers.
    /// </summary>
    static const IntModelProperty PauseOnTriggerProperty;

    /// <summary>
    /// Gets whether or not the emulator should be paused when a part triggers.
    /// </summary>
    LeaderboardParts GetPauseOnTrigger() const { return ra::itoe<LeaderboardParts>(GetValue(PauseOnTriggerProperty)); }

    /// <summary>
    /// Sets whether or not the emulator should be paused when a part triggers.
    /// </summary>
    void SetPauseOnTrigger(LeaderboardParts nValue) { SetValue(PauseOnTriggerProperty, ra::etoi(nValue)); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the start trigger version.
    /// </summary>
    static const IntModelProperty StartTriggerProperty;

    /// <summary>
    /// Gets the start trigger definition.
    /// </summary>
    const std::string& GetStartTrigger() const { return GetAssetDefinition(m_pStartTrigger); }

    /// <summary>
    /// Sets the start trigger definition.
    /// </summary>
    void SetStartTrigger(const std::string& sTrigger) { SetAssetDefinition(m_pStartTrigger, sTrigger); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the submit trigger version.
    /// </summary>
    static const IntModelProperty SubmitTriggerProperty;

    /// <summary>
    /// Gets the submit trigger definition.
    /// </summary>
    const std::string& GetSubmitTrigger() const { return GetAssetDefinition(m_pSubmitTrigger); }

    /// <summary>
    /// Sets the submit trigger definition.
    /// </summary>
    void SetSubmitTrigger(const std::string& sTrigger) { SetAssetDefinition(m_pSubmitTrigger, sTrigger); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the cancel trigger version.
    /// </summary>
    static const IntModelProperty CancelTriggerProperty;

    /// <summary>
    /// Gets the cancel trigger definition.
    /// </summary>
    const std::string& GetCancelTrigger() const { return GetAssetDefinition(m_pCancelTrigger); }

    /// <summary>
    /// Sets the cancel trigger definition.
    /// </summary>
    void SetCancelTrigger(const std::string& sTrigger) { SetAssetDefinition(m_pCancelTrigger, sTrigger); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the value definition.
    /// </summary>
    static const IntModelProperty ValueDefinitionProperty;

    /// <summary>
    /// Gets the value definition.
    /// </summary>
    const std::string& GetValueDefinition() const { return GetAssetDefinition(m_pValueDefinition); }

    /// <summary>
    /// Sets the value definition.
    /// </summary>
    void SetValueDefinition(const std::string& sTrigger) { SetAssetDefinition(m_pValueDefinition, sTrigger); }

    /// <summary>
    /// Gets the calculated hash for the value definition.
    /// </summary>
    const int GetValueDefinitionHash() const { return GetValue(ValueDefinitionProperty); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for the value format.
    /// </summary>
    static const IntModelProperty ValueFormatProperty;

    /// <summary>
    /// Gets the value format.
    /// </summary>
    ValueFormat GetValueFormat() const { return ra::itoe<ValueFormat>(GetValue(ValueFormatProperty)); }

    /// <summary>
    /// Sets the value format.
    /// </summary>
    void SetValueFormat(ValueFormat nValue) { SetValue(ValueFormatProperty, ra::etoi(nValue)); }

    /// <summary>
    /// Converts a raw score into a display string.
    /// </summary>
    std::string FormatScore(int nValue) const;

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not lower values are better.
    /// </summary>
    static const BoolModelProperty LowerIsBetterProperty;

    /// <summary>
    /// Gets whether or not lower values are better.
    /// </summary>
    bool IsLowerBetter() const { return GetValue(LowerIsBetterProperty); }

    /// <summary>
    /// Sets whether or not lower values are better.
    /// </summary>
    void SetLowerIsBetter(bool nValue) { SetValue(LowerIsBetterProperty, nValue); }

    /// <summary>
    /// The <see cref="ModelProperty" /> for whether or not the leaderboard should not be displayed in the overlay.
    /// </summary>
    static const BoolModelProperty IsHiddenProperty;

    /// <summary>
    /// Gets whether or not the leaderboard should not be displayed in the overlay.
    /// </summary>
    bool IsHidden() const { return GetValue(IsHiddenProperty); }

    /// <summary>
    /// Sets whether or not the leaderboard should not be displayed in the overlay.
    /// </summary>
    void SetHidden(bool nValue) { SetValue(IsHiddenProperty, nValue); }

    /// <summary>
    /// Populates the start/submit/cancel/value fields from a compound definition.
    /// </summary>
    void SetDefinition(const std::string& sDefinition);

    /// <summary>
    /// Constructs a compound definition from start/submit/cancel/value fields.
    /// </summary>
    std::string GetDefinition() const;

    void Activate() override;
    void Deactivate() override;

    void DoFrame() override;

    void Serialize(ra::services::TextWriter& pWriter) const override;
    bool Deserialize(ra::Tokenizer& pTokenizer) override;

    // Attaches an rc_client_leaderboard_info_t and populates the model from it.
    void Attach(struct rc_client_leaderboard_info_t& pLeaderboard,
        AssetCategory nCategory, const std::string& sDefinition);
    // Replaces the attached rc_client_leaderboard_info_t. Does not update model or source.
    void ReplaceAttached(struct rc_client_leaderboard_info_t& pLeaderboard) noexcept;
    // Attaches an rc_client_leaderboard_info_t and populates it from the model.
    void AttachAndInitialize(struct rc_client_leaderboard_info_t& pLeaderboard);
    // Gets the attached rc_client_achievement_info_t.
    const struct rc_client_leaderboard_info_t* GetAttached() const noexcept { return m_pLeaderboard; }

    const CapturedTriggerHits& GetStartCapturedHits() const noexcept { return m_pCapturedStartTriggerHits; }
    const CapturedTriggerHits& GetSubmitCapturedHits() const noexcept { return m_pCapturedSubmitTriggerHits; }
    const CapturedTriggerHits& GetCancelCapturedHits() const noexcept { return m_pCapturedCancelTriggerHits; }
    const CapturedTriggerHits& GetValueCapturedHits() const noexcept { return m_pCapturedValueDefinitionHits; }
    void ResetCapturedHits() noexcept 
    {
        m_pCapturedStartTriggerHits.Reset();
        m_pCapturedSubmitTriggerHits.Reset();
        m_pCapturedCancelTriggerHits.Reset();
        m_pCapturedValueDefinitionHits.Reset();
    }

    static constexpr size_t MaxSerializedLength = 65535;

protected:
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;
    void OnValueChanged(const StringModelProperty::ChangeArgs& args) override;

    bool ValidateAsset(std::wstring& sErrorMessage) override;

private:
    void HandleStateChanged(AssetState nOldState, AssetState nNewState);
    void SyncTitle();
    void SyncDescription();
    void SyncState(AssetState nNewState) noexcept;
    void SyncValueFormat();
    void SyncTracker();
    void SyncDefinition();

    AssetDefinition m_pStartTrigger;
    AssetDefinition m_pSubmitTrigger;
    AssetDefinition m_pCancelTrigger;
    AssetDefinition m_pValueDefinition;

    struct rc_client_leaderboard_info_t* m_pLeaderboard = nullptr;
    std::string m_sTitleBuffer;
    std::string m_sDescriptionBuffer;

    CapturedTriggerHits m_pCapturedStartTriggerHits;
    CapturedTriggerHits m_pCapturedSubmitTriggerHits;
    CapturedTriggerHits m_pCapturedCancelTriggerHits;
    CapturedTriggerHits m_pCapturedValueDefinitionHits;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_LEADERBOARD_MODEL_H
