#include "LeaderboardModel.hh"

#include "data\context\GameContext.hh"

#include "data\models\TriggerValidation.hh"

#include "services\AchievementRuntime.hh"
#include "services\ServiceLocator.hh"

#include <rcheevos\src\rc_client_internal.h>
#include <rcheevos\src\rcheevos\rc_internal.h>

namespace ra {
namespace data {
namespace models {

const IntModelProperty LeaderboardModel::StartTriggerProperty("LeaderboardModel", "StartTrigger", 0);
const IntModelProperty LeaderboardModel::SubmitTriggerProperty("LeaderboardModel", "SubmitTrigger", 0);
const IntModelProperty LeaderboardModel::CancelTriggerProperty("LeaderboardModel", "CancelTrigger", 0);
const IntModelProperty LeaderboardModel::ValueDefinitionProperty("LeaderboardModel", "ValueDefinition", 0);
const IntModelProperty LeaderboardModel::ValueFormatProperty("LeaderboardModel", "ValueFormat", ra::etoi(ValueFormat::Value));
const IntModelProperty LeaderboardModel::PauseOnResetProperty("LeaderboardModel", "PauseOnReset", ra::etoi(LeaderboardModel::LeaderboardParts::None));
const IntModelProperty LeaderboardModel::PauseOnTriggerProperty("LeaderboardModel", "PauseOnTrigger", ra::etoi(LeaderboardModel::LeaderboardParts::None));
const BoolModelProperty LeaderboardModel::LowerIsBetterProperty("LeaderboardModel", "LowerIsBetter", false);
const BoolModelProperty LeaderboardModel::IsHiddenProperty("LeaderboardModel", "Hidden", false);

LeaderboardModel::LeaderboardModel() noexcept
{
    GSL_SUPPRESS_F6 SetValue(TypeProperty, ra::etoi(AssetType::Leaderboard));

    GSL_SUPPRESS_F6 AddAssetDefinition(m_pStartTrigger, StartTriggerProperty);
    GSL_SUPPRESS_F6 AddAssetDefinition(m_pSubmitTrigger, SubmitTriggerProperty);
    GSL_SUPPRESS_F6 AddAssetDefinition(m_pCancelTrigger, CancelTriggerProperty);
    GSL_SUPPRESS_F6 AddAssetDefinition(m_pValueDefinition, ValueDefinitionProperty);

    SetTransactional(ValueFormatProperty);
    SetTransactional(LowerIsBetterProperty);
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
        else if (args.Property == StartTriggerProperty || args.Property == SubmitTriggerProperty ||
            args.Property == CancelTriggerProperty || args.Property == ValueDefinitionProperty)
        {
            SyncDefinition();
        }
        else if (args.Property == ValueFormatProperty)
        {
            SyncValueFormat();
        }
    }

    AssetModelBase::OnValueChanged(args);
}

void LeaderboardModel::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    // call base first so the transaction gets updated for GetLocalValue to work properly
    AssetModelBase::OnValueChanged(args);

    // if we're still loading the asset, ignore the event. we'll get recalled once loading finishes
    if (IsUpdating())
        return;

    if (args.Property == DescriptionProperty)
    {
        if (m_pLeaderboard)
            SyncDescription();
    }
    else if (args.Property == NameProperty)
    {
        if (m_pLeaderboard)
            SyncTitle();
    }
}

bool LeaderboardModel::ValidateAsset(std::wstring& sError)
{
    const auto& sStartTrigger = GetAssetDefinition(m_pStartTrigger);
    if (sStartTrigger.empty())
    {
        sError = L"No Start condition";
        return false;
    }

    const auto& sCancelTrigger = GetAssetDefinition(m_pCancelTrigger);
    if (sCancelTrigger.empty())
    {
        sError = L"No Cancel condition";
        return false;
    }

    const auto& sSubmitTrigger = GetAssetDefinition(m_pSubmitTrigger);
    if (sSubmitTrigger.empty())
    {
        sError = L"No Submit condition";
        return false;
    }

    const auto& sValueDefinition = GetAssetDefinition(m_pValueDefinition);
    if (sValueDefinition.empty())
    {
        sError = L"No Value definition";
        return false;
    }

    const size_t nSerializedSize = sStartTrigger.length() + sCancelTrigger.length() +
        sSubmitTrigger.length() + sValueDefinition.length() + 16; // "STA:SUB:CAN:VAL:"
    if (nSerializedSize > MaxSerializedLength)
    {
        sError = ra::StringPrintf(L"Serialized length exceeds limit: %d/%d", nSerializedSize, MaxSerializedLength);
        return false;
    }

    if (!TriggerValidation::Validate(sStartTrigger, sError, AssetType::Leaderboard))
    {
        sError.insert(0, L"Start: ");
        return false;
    }

    if (!TriggerValidation::Validate(sCancelTrigger, sError, AssetType::Leaderboard))
    {
        sError.insert(0, L"Cancel: ");
        return false;
    }

    if (!TriggerValidation::Validate(sSubmitTrigger, sError, AssetType::Leaderboard))
    {
        sError.insert(0, L"Submit: ");
        return false;
    }

    return true;
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
    if (!m_pLeaderboard)
        return;

    const bool bWasActive = IsActive(nOldState);
    const bool bIsActive = IsActive(nNewState);

    if (!bIsActive && bWasActive)
    {
        const auto* pLeaderboard = m_pLeaderboard->lboard;
        Expects(pLeaderboard != nullptr);

        m_pCapturedStartTriggerHits.Capture(&pLeaderboard->start, GetStartTrigger());
        m_pCapturedSubmitTriggerHits.Capture(&pLeaderboard->submit, GetSubmitTrigger());
        m_pCapturedCancelTriggerHits.Capture(&pLeaderboard->cancel, GetCancelTrigger());
        m_pCapturedValueDefinitionHits.Capture(&pLeaderboard->value, GetValueDefinition());

        if (pLeaderboard->state == RC_LBOARD_STATE_STARTED)
        {
            auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
            pRuntime.ReleaseLeaderboardTracker(m_pLeaderboard->public_.id);
        }
    }
    else if (bIsActive && !bWasActive)
    {
        if (m_pLeaderboard->lboard)
            rc_reset_lboard(m_pLeaderboard->lboard);
        else
            SyncDefinition();
    }

    switch (nNewState)
    {
        case ra::data::models::AssetState::Disabled:
            m_pLeaderboard->public_.state = RC_CLIENT_LEADERBOARD_STATE_DISABLED;
            if (m_pLeaderboard->lboard)
                m_pLeaderboard->lboard->state = RC_LBOARD_STATE_DISABLED;
            break;

        case ra::data::models::AssetState::Inactive:
            m_pLeaderboard->public_.state = RC_CLIENT_LEADERBOARD_STATE_INACTIVE;
            if (m_pLeaderboard->lboard)
                m_pLeaderboard->lboard->state = RC_LBOARD_STATE_INACTIVE;
            break;

        case ra::data::models::AssetState::Primed:
            m_pLeaderboard->public_.state = RC_CLIENT_LEADERBOARD_STATE_TRACKING;
            if (m_pLeaderboard->lboard)
                m_pLeaderboard->lboard->state = RC_LBOARD_STATE_STARTED;
            break;

        case ra::data::models::AssetState::Waiting:
            m_pLeaderboard->public_.state = RC_CLIENT_LEADERBOARD_STATE_ACTIVE;
            if (m_pLeaderboard->lboard)
                m_pLeaderboard->lboard->state = RC_LBOARD_STATE_WAITING;
            break;

        default:
            m_pLeaderboard->public_.state = RC_CLIENT_LEADERBOARD_STATE_ACTIVE;
            if (m_pLeaderboard->lboard)
                m_pLeaderboard->lboard->state = RC_LBOARD_STATE_ACTIVE;
            break;
    }
}

void LeaderboardModel::SyncTitle()
{
    m_sTitleBuffer = ra::Narrow(GetName());
    m_pLeaderboard->public_.title = m_sTitleBuffer.c_str();
}

void LeaderboardModel::SyncDescription()
{
    m_sDescriptionBuffer = ra::Narrow(GetDescription());
    m_pLeaderboard->public_.description = m_sDescriptionBuffer.c_str();
}

void LeaderboardModel::SyncValueFormat()
{
    m_pLeaderboard->format = ra::etoi(GetValueFormat());
    m_pLeaderboard->public_.format = rc_client_map_leaderboard_format(m_pLeaderboard->format);
}

void LeaderboardModel::SyncDefinition()
{
    if (!m_pLeaderboard)
        return;

    if (!IsActive())
    {
        if (m_pLeaderboard->lboard)
        {
            auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
            if (pRuntime.DetachMemory(m_pLeaderboard->lboard))
                free(m_pLeaderboard->lboard);

            m_pLeaderboard->lboard = nullptr;
        }

        memset(m_pLeaderboard->md5, 0, sizeof(m_pLeaderboard->md5));
        return;
    }

    const auto& sMemAddr = GetDefinition();
    uint8_t md5[16];
    rc_runtime_checksum(sMemAddr.c_str(), md5);
    if (memcmp(m_pLeaderboard->md5, md5, sizeof(md5)) != 0)
    {
        memcpy(m_pLeaderboard->md5, md5, sizeof(md5));

        auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        if (m_pLeaderboard->lboard && pRuntime.DetachMemory(m_pLeaderboard->lboard))
        {
            free(m_pLeaderboard->lboard);
            m_pLeaderboard->lboard = nullptr;
        }

        const auto nSize = rc_lboard_size(sMemAddr.c_str());
        if (nSize > 0)
        {
            void* lboard_buffer = malloc(nSize);
            if (lboard_buffer)
            {
                /* populate the item, using the communal memrefs pool */
                auto* pGame = pRuntime.GetClient()->game;

                rc_parse_state_t parse;
                rc_init_parse_state(&parse, lboard_buffer, nullptr, 0);
                parse.first_memref = &pGame->runtime.memrefs;
                parse.variables = &pGame->runtime.variables;

                m_pLeaderboard->lboard = RC_ALLOC(rc_lboard_t, &parse);
                rc_parse_lboard_internal(m_pLeaderboard->lboard, sMemAddr.c_str(), &parse);
                rc_destroy_parse_state(&parse);

                m_pLeaderboard->lboard->memrefs = nullptr;

                pRuntime.AttachMemory(m_pLeaderboard->lboard);
            }
        }
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

std::string LeaderboardModel::GetDefinition() const
{
    return "STA:" + GetStartTrigger() + "::SUB:" + GetSubmitTrigger() +
        "::CAN:" + GetCancelTrigger() + "::VAL:" + GetValueDefinition();
}

void LeaderboardModel::Attach(struct rc_client_leaderboard_info_t& pLeaderboard,
                              AssetCategory nCategory, const std::string& sDefinition)
{
    SetID(pLeaderboard.public_.id);
    SetName(ra::Widen(pLeaderboard.public_.title));
    SetDescription(ra::Widen(pLeaderboard.public_.description));
    SetCategory(nCategory);
    SetValueFormat(ra::itoe<ValueFormat>(pLeaderboard.format));
    SetLowerIsBetter(pLeaderboard.public_.lower_is_better);
    SetHidden(pLeaderboard.hidden);
    SetDefinition(sDefinition);

    CreateServerCheckpoint();
    CreateLocalCheckpoint();

    m_pLeaderboard = &pLeaderboard;

    DoFrame(); // sync state
}

void LeaderboardModel::ReplaceAttached(struct rc_client_leaderboard_info_t& pLeaderboard) noexcept
{
    m_pLeaderboard = &pLeaderboard;
}

void LeaderboardModel::AttachAndInitialize(struct rc_client_leaderboard_info_t& pLeaderboard)
{
    pLeaderboard.public_.id = GetID();

    m_pLeaderboard = &pLeaderboard;

    SyncTitle();
    SyncDescription();
    SyncDefinition();
    SyncValueFormat();
}

void LeaderboardModel::Serialize(ra::services::TextWriter& pWriter) const
{
    WriteQuoted(pWriter, GetLocalAssetDefinition(m_pStartTrigger));
    WriteQuoted(pWriter, GetLocalAssetDefinition(m_pCancelTrigger));
    WriteQuoted(pWriter, GetLocalAssetDefinition(m_pSubmitTrigger));
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
    WriteNumber(pWriter, IsLowerBetter() ? 1 : 0);
}

bool LeaderboardModel::Deserialize(ra::Tokenizer& pTokenizer)
{
    // field 2: start trigger
    std::string sStartTrigger = pTokenizer.ReadQuotedString();
    if (!pTokenizer.Consume(':'))
        return false;

    // field 3: cancel trigger
    std::string sCancelTrigger = pTokenizer.ReadQuotedString();
    if (!pTokenizer.Consume(':'))
        return false;

    // field 4: submit trigger
    std::string sSubmitTrigger = pTokenizer.ReadQuotedString();
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
    if (!pTokenizer.EndOfString() && !ReadPossiblyQuoted(pTokenizer, sDescription))
        return false;

    // field 9: lower is better
    uint32_t nLowerIsBetter = 0;
    if (!pTokenizer.EndOfString() && !ReadNumber(pTokenizer, nLowerIsBetter))
        return false;

    // line is valid
    SetName(ra::Widen(sTitle));
    SetDescription(ra::Widen(sDescription));
    SetStartTrigger(sStartTrigger);
    SetSubmitTrigger(sSubmitTrigger);
    SetCancelTrigger(sCancelTrigger);
    SetValueDefinition(sValueDefinition);
    SetLowerIsBetter(nLowerIsBetter != 0);

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

std::string LeaderboardModel::FormatScore(int nValue) const
{
    char buffer[32];
    rc_format_value(buffer, sizeof(buffer), nValue, ra::etoi(GetValueFormat()));
    return std::string(buffer);
}

} // namespace models
} // namespace data
} // namespace ra
