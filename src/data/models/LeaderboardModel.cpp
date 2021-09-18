#include "LeaderboardModel.hh"

#include "data\context\GameContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace data {
namespace models {

const IntModelProperty LeaderboardModel::StartTriggerProperty("LeaderboardModel", "StartTrigger", 0);
const IntModelProperty LeaderboardModel::SubmitTriggerProperty("LeaderboardModel", "SubmitTrigger", 0);
const IntModelProperty LeaderboardModel::CancelTriggerProperty("LeaderboardModel", "CancelTrigger", 0);
const IntModelProperty LeaderboardModel::ValueDefinitionProperty("LeaderboardModel", "ValueDefinition", 0);
const IntModelProperty LeaderboardModel::ValueFormatProperty("LeaderboardModel", "ValueFormat", ra::etoi(ValueFormat::Value));

LeaderboardModel::LeaderboardModel() noexcept
{
    GSL_SUPPRESS_F6 SetValue(TypeProperty, ra::etoi(AssetType::Leaderboard));

    GSL_SUPPRESS_F6 AddAssetDefinition(m_pStartTrigger, StartTriggerProperty);
    GSL_SUPPRESS_F6 AddAssetDefinition(m_pSubmitTrigger, SubmitTriggerProperty);
    GSL_SUPPRESS_F6 AddAssetDefinition(m_pCancelTrigger, CancelTriggerProperty);
    GSL_SUPPRESS_F6 AddAssetDefinition(m_pValueDefinition, ValueDefinitionProperty);
}

void LeaderboardModel::Activate()
{
    if (!IsActive())
        SetState(AssetState::Waiting);
}

void LeaderboardModel::Deactivate()
{
    SetState(AssetState::Inactive);
}

void LeaderboardModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    // if we're still loading the asset, ignore the event. we'll get recalled once loading finishes
    if (!IsUpdating())
    {
        if (args.Property == StateProperty)
        {
            HandleStateChanged(ra::itoe<AssetState>(args.tOldValue), ra::itoe<AssetState>(args.tNewValue));
        }
    }

    AssetModelBase::OnValueChanged(args);
}

void LeaderboardModel::DoFrame()
{
    const auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
    const auto* pLeaderboard = pRuntime.GetLeaderboardDefinition(GetID());
    if (pLeaderboard == nullptr)
    {
        if (IsActive())
            SetState(AssetState::Inactive);
    }
    else
    {
        switch (pLeaderboard->state)
        {
            case RC_LBOARD_STATE_WAITING:
            case RC_LBOARD_STATE_CANCELED:
            case RC_LBOARD_STATE_TRIGGERED:
                /* all of these states are waiting for the leaderboard to start */
                SetState(AssetState::Waiting);
                break;
            case RC_LBOARD_STATE_ACTIVE:
                SetState(AssetState::Active);
                break;
            case RC_LBOARD_STATE_STARTED:
                SetState(AssetState::Primed);
                break;
            case RC_LBOARD_STATE_INACTIVE:
                SetState(AssetState::Inactive);
                break;
            case RC_LBOARD_STATE_DISABLED:
                SetState(AssetState::Disabled);
                break;
        }
    }
}

void LeaderboardModel::HandleStateChanged(AssetState nOldState, AssetState nNewState)
{
    if (!ra::services::ServiceLocator::Exists<ra::services::AchievementRuntime>())
        return;

    const bool bWasActive = IsActive(nOldState);
    const bool bIsActive = IsActive(nNewState);
    if (bWasActive == bIsActive)
        return;

    auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
    auto* pLeaderboard = pRuntime.GetLeaderboardDefinition(GetID());

    if (bIsActive)
    {
        const bool bRuntimeActive = (pLeaderboard != nullptr);
        if (!bRuntimeActive)
        {
            std::string sDefinition = "STA:" + GetStartTrigger() + "::SUB:" + GetSubmitTrigger() +
                "::CAN:" + GetCancelTrigger() + "::VAL:" + GetValueDefinition();

            pRuntime.ActivateLeaderboard(GetID(), sDefinition);
            pLeaderboard = pRuntime.GetLeaderboardDefinition(GetID());
        }
        else
        {
            rc_reset_lboard(pLeaderboard);
        }
    }
    else if (pLeaderboard)
    {
        m_pCapturedStartTriggerHits.Capture(&pLeaderboard->start, GetStartTrigger());
        m_pCapturedSubmitTriggerHits.Capture(&pLeaderboard->submit, GetSubmitTrigger());
        m_pCapturedCancelTriggerHits.Capture(&pLeaderboard->cancel, GetCancelTrigger());
        m_pCapturedValueDefinitionHits.Capture(&pLeaderboard->value, GetValueDefinition());
        pRuntime.DeactivateLeaderboard(GetID());
    }

    switch (nNewState)
    {
        case AssetState::Waiting:
            if (pLeaderboard)
                pLeaderboard->state = RC_LBOARD_STATE_WAITING;
            break;

        case AssetState::Active:
            if (pLeaderboard)
                pLeaderboard->state = RC_LBOARD_STATE_ACTIVE;
            break;

        case AssetState::Inactive:
            if (pLeaderboard)
                pLeaderboard->state = RC_LBOARD_STATE_INACTIVE;
            break;
    }
}

void LeaderboardModel::SetDefinition(const std::string& sDefinition)
{
    size_t nIndex = 0;
    while (nIndex < sDefinition.length())
    {
        size_t nNext = sDefinition.find("::", nIndex);
        if (nNext == std::string::npos)
            nNext = sDefinition.length();

        std::string sPart = sDefinition.substr(nIndex + 4, nNext - nIndex - 4);
        if (sDefinition.compare(nIndex, 3, "STA", 0, 3) == 0)
            SetStartTrigger(sPart);
        else if (sDefinition.compare(nIndex, 3, "SUB", 0, 3) == 0)
            SetSubmitTrigger(sPart);
        else if (sDefinition.compare(nIndex, 3, "CAN", 0, 3) == 0)
            SetCancelTrigger(sPart);
        else if (sDefinition.compare(nIndex, 3, "VAL", 0, 3) == 0)
            SetValueDefinition(sPart);

        nIndex = nNext + 2;
    }
}

void LeaderboardModel::Serialize(ra::services::TextWriter& pWriter) const
{
    WriteQuoted(pWriter, GetLocalAssetDefinition(m_pStartTrigger));
    WriteQuoted(pWriter, GetLocalAssetDefinition(m_pSubmitTrigger));
    WriteQuoted(pWriter, GetLocalAssetDefinition(m_pCancelTrigger));
    WriteQuoted(pWriter, GetLocalAssetDefinition(m_pValueDefinition));

    switch (GetValueFormat())
    {
        case ValueFormat::Centiseconds: WritePossiblyQuoted(pWriter, "MILLISECS"); break;
        case ValueFormat::Frames: WritePossiblyQuoted(pWriter, "FRAMES"); break;
        case ValueFormat::Minutes: WritePossiblyQuoted(pWriter, "MINUTES"); break;
        case ValueFormat::Score: WritePossiblyQuoted(pWriter, "SCORE"); break;
        case ValueFormat::Seconds: WritePossiblyQuoted(pWriter, "SECS"); break;
        case ValueFormat::SecondsAsMinutes: WritePossiblyQuoted(pWriter, "SECS_AS_MINS"); break;
        case ValueFormat::Value: WritePossiblyQuoted(pWriter, "VALUE"); break;
        default: WritePossiblyQuoted(pWriter, ""); break;
    }

    WritePossiblyQuoted(pWriter, GetLocalValue(NameProperty));
    WritePossiblyQuoted(pWriter, GetLocalValue(DescriptionProperty));
}

bool LeaderboardModel::Deserialize(ra::Tokenizer& pTokenizer)
{
    // field 2: start trigger
    std::string sStartTrigger = pTokenizer.ReadQuotedString();
    if (!pTokenizer.Consume(':'))
        return false;

    // field 3: submit trigger
    std::string sSubmitTrigger = pTokenizer.ReadQuotedString();
    if (!pTokenizer.Consume(':'))
        return false;

    // field 4: cancel trigger
    std::string sCancelTrigger = pTokenizer.ReadQuotedString();
    if (!pTokenizer.Consume(':'))
        return false;

    // field 5: value definition
    std::string sValueDefinition = pTokenizer.ReadQuotedString();
    if (!pTokenizer.Consume(':'))
        return false;

    // field 6: format
    std::string sFormat;
    if (!ReadPossiblyQuoted(pTokenizer, sFormat))
        return false;

    // field 7: title
    std::string sTitle;
    if (!ReadPossiblyQuoted(pTokenizer, sTitle))
        return false;

    // field 8: description
    std::string sDescription;
    if (!ReadPossiblyQuoted(pTokenizer, sDescription))
        return false;

    // line is valid
    SetName(ra::Widen(sTitle));
    SetDescription(ra::Widen(sDescription));
    SetStartTrigger(sStartTrigger);
    SetSubmitTrigger(sSubmitTrigger);
    SetCancelTrigger(sCancelTrigger);
    SetValueDefinition(sValueDefinition);

    if (sFormat == "VALUE")
        SetValueFormat(ValueFormat::Value);
    else if (sFormat == "MILLISECS")
        SetValueFormat(ValueFormat::Centiseconds);
    else if (sFormat == "SCORE")
        SetValueFormat(ValueFormat::Score);
    else if (sFormat == "FRAMES")
        SetValueFormat(ValueFormat::Frames);
    else if (sFormat == "SECS")
        SetValueFormat(ValueFormat::Seconds);
    else if (sFormat == "MINUTES")
        SetValueFormat(ValueFormat::Minutes);
    else if (sFormat == "SECS_AS_MINS")
        SetValueFormat(ValueFormat::SecondsAsMinutes);
    else
        SetValueFormat(ra::itoe<ValueFormat>(ValueFormatProperty.GetDefaultValue()));

    return true;
}

} // namespace models
} // namespace data
} // namespace ra
