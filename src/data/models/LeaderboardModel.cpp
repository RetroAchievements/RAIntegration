#include "LeaderboardModel.hh"

#include "context\IRcClient.hh"

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
            if (m_pLeaderboardInfo)
                SyncDefinitionToRuntime();
        }
        else if (args.Property == ValueFormatProperty)
        {
            if (m_pLeaderboardInfo)
                SyncValueFormatToRuntime();
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
        if (m_pLeaderboardInfo)
            SyncDescriptionToRuntime();
    }
    else if (args.Property == NameProperty)
    {
        if (m_pLeaderboardInfo)
            SyncTitleToRuntime();
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
        sError = ra::util::String::Printf(L"Serialized length exceeds limit: %d/%d", nSerializedSize, MaxSerializedLength);
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
    if (m_pLeaderboardInfo && m_pLeaderboardInfo->lboard)
        SyncStateFromRuntime(m_pLeaderboardInfo->lboard->state);
}

void LeaderboardModel::SyncStateFromRuntime(uint8_t nState)
{
    switch (nState)
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

static void ReleaseLeaderboardTracker(struct rc_client_leaderboard_info_t* pLeaderboardInfo)
{
    rc_client_leaderboard_tracker_info_t* tracker = pLeaderboardInfo->tracker;
    if (tracker)
    {
        auto* pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>().GetClient();
        rc_client_release_leaderboard_tracker(pClient->game, pLeaderboardInfo);

        if (tracker->pending_events & RC_CLIENT_LEADERBOARD_TRACKER_PENDING_EVENT_HIDE)
        {
            // no other references. hide it immediately.
            rc_client_event_t pEvent;
            memset(&pEvent, 0, sizeof(pEvent));
            pEvent.leaderboard_tracker = &tracker->public_;
            pEvent.type = RC_CLIENT_EVENT_LEADERBOARD_TRACKER_HIDE;
            pClient->callbacks.event_handler(&pEvent, pClient);

            tracker->pending_events = RC_CLIENT_LEADERBOARD_TRACKER_PENDING_EVENT_NONE;
        }
    }
}

void LeaderboardModel::HandleStateChanged(AssetState nOldState, AssetState nNewState)
{
    if (!m_pLeaderboardInfo)
        return;

    const bool bWasActive = IsActive(nOldState);
    const bool bIsActive = IsActive(nNewState);

    if (!bIsActive && bWasActive)
    {
        const auto* pLeaderboard = m_pLeaderboardInfo->lboard;
        if (pLeaderboard != nullptr)
        {
            if (pLeaderboard->state == RC_LBOARD_STATE_STARTED)
            {
                // if the runtime thinks the leaderboard has started, there's most likely a tracker
                // visible. release the reference, which will hide it if no other references exist.
                ReleaseLeaderboardTracker(m_pLeaderboardInfo);
            }
        }
    }
    else if (bIsActive && !bWasActive)
    {
        if (m_pLeaderboardInfo->lboard)
            rc_reset_lboard(m_pLeaderboardInfo->lboard);
        else
            SyncDefinitionToRuntime();
    }

    SyncStateToRuntime(nNewState);
}

void LeaderboardModel::SyncStateToRuntime(AssetState nNewState) const
{
    auto* pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>().GetClient();

    rc_mutex_lock(&pClient->state.mutex);

    switch (nNewState)
    {
        case ra::data::models::AssetState::Disabled:
            m_pLeaderboardInfo->public_.state = RC_CLIENT_LEADERBOARD_STATE_DISABLED;
            if (m_pLeaderboardInfo->lboard)
                m_pLeaderboardInfo->lboard->state = RC_LBOARD_STATE_DISABLED;
            break;

        case ra::data::models::AssetState::Inactive:
            m_pLeaderboardInfo->public_.state = RC_CLIENT_LEADERBOARD_STATE_INACTIVE;
            if (m_pLeaderboardInfo->lboard)
                m_pLeaderboardInfo->lboard->state = RC_LBOARD_STATE_INACTIVE;
            break;

        case ra::data::models::AssetState::Primed:
            m_pLeaderboardInfo->public_.state = RC_CLIENT_LEADERBOARD_STATE_TRACKING;
            if (m_pLeaderboardInfo->lboard)
                m_pLeaderboardInfo->lboard->state = RC_LBOARD_STATE_STARTED;
            break;

        case ra::data::models::AssetState::Waiting:
            m_pLeaderboardInfo->public_.state = RC_CLIENT_LEADERBOARD_STATE_ACTIVE;
            if (m_pLeaderboardInfo->lboard)
                m_pLeaderboardInfo->lboard->state = RC_LBOARD_STATE_WAITING;
            break;

        default:
            m_pLeaderboardInfo->public_.state = RC_CLIENT_LEADERBOARD_STATE_ACTIVE;
            if (m_pLeaderboardInfo->lboard)
                m_pLeaderboardInfo->lboard->state = RC_LBOARD_STATE_ACTIVE;
            break;
    }

    rc_mutex_unlock(&pClient->state.mutex);
}

void LeaderboardModel::SyncTitleToRuntime()
{
    m_sTitleBuffer = ra::util::String::Narrow(GetName());
    m_pLeaderboardInfo->public_.title = m_sTitleBuffer.c_str();
}

void LeaderboardModel::SyncDescriptionToRuntime()
{
    m_sDescriptionBuffer = ra::util::String::Narrow(GetDescription());
    m_pLeaderboardInfo->public_.description = m_sDescriptionBuffer.c_str();
}

void LeaderboardModel::SyncValueFormatToRuntime() const
{
    m_pLeaderboardInfo->format = ra::etoi(GetValueFormat());
    m_pLeaderboardInfo->public_.format = rc_client_map_leaderboard_format(m_pLeaderboardInfo->format);

    SyncTrackerToRuntime();
}

void LeaderboardModel::SyncTrackerToRuntime() const
{
    if (m_pLeaderboardInfo->tracker)
    {
        auto* pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>().GetClient();
        auto* pGame = pClient->game;

        rc_client_release_leaderboard_tracker(pGame, m_pLeaderboardInfo);
        rc_client_allocate_leaderboard_tracker(pGame, m_pLeaderboardInfo);
    }
}

void LeaderboardModel::SyncDefinitionToRuntime()
{
    Expects(m_pLeaderboardInfo != nullptr);

    if (!IsActive())
    {
        // if the leaderboard isn't active, we don't have to parse it or load it into the runtime.
        m_pLeaderboardInfo->lboard = nullptr;
        m_pLeaderboardBuffer.reset();

        memset(m_pLeaderboardInfo->md5, 0, sizeof(m_pLeaderboardInfo->md5));
        return;
    }

    ParseDefinition();
}

void LeaderboardModel::ParseDefinition() const
{
    const auto& sMemAddr = GetDefinition();
    uint8_t md5[16];
    rc_runtime_checksum(sMemAddr.c_str(), md5);

    if (memcmp(m_pLeaderboardInfo->md5, md5, sizeof(md5)) == 0)
    {
        // leaderboard is already up-to-date. do nothing.
        return;
    }

    memcpy(m_pLeaderboardInfo->md5, md5, sizeof(md5));

    if (m_pPublishedLeaderboardInfo && memcmp(m_pPublishedLeaderboardInfo->md5, md5, sizeof(md5)) == 0)
    {
        // leaderboard changed back to published state. just reference that.
        if (m_pPublishedLeaderboardInfo->lboard != nullptr)
            rc_reset_lboard(m_pLeaderboardInfo->lboard);

        m_pLeaderboardInfo->lboard = m_pPublishedLeaderboardInfo->lboard;
        m_pLeaderboardBuffer.reset();
        return;
    }

    // attempt to parse the leaderboard
    auto* pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>().GetClient();
    auto* pGame = pClient->game;
    Expects(pGame != nullptr);

    rc_mutex_lock(&pClient->state.mutex);

    rc_preparse_state_t preparse;
    rc_init_preparse_state(&preparse);
    preparse.parse.existing_memrefs = pGame->runtime.memrefs;

    rc_lboard_with_memrefs_t* lboard = RC_ALLOC(rc_lboard_with_memrefs_t, &preparse.parse);
    rc_parse_lboard_internal(&lboard->lboard, sMemAddr.c_str(), &preparse.parse);
    rc_preparse_alloc_memrefs(nullptr, &preparse);

    const auto nSize = preparse.parse.offset;
    if (nSize > 0)
    {
        auto lboard_buffer = std::make_unique<uint8_t[]>(nSize);
        if (lboard_buffer)
        {
            // populate the item, using the communal memrefs pool
            rc_reset_parse_state(&preparse.parse, lboard_buffer.get());
            lboard = RC_ALLOC(rc_lboard_with_memrefs_t, &preparse.parse);
            rc_preparse_alloc_memrefs(&lboard->memrefs, &preparse);

            preparse.parse.existing_memrefs = pGame->runtime.memrefs;
            preparse.parse.memrefs = &lboard->memrefs;

            rc_parse_lboard_internal(&lboard->lboard, sMemAddr.c_str(), &preparse.parse);
            lboard->lboard.has_memrefs = 1;

            const auto* pOldLboard = m_pLeaderboardInfo->lboard;
            m_pLeaderboardInfo->lboard = &lboard->lboard;

            // update the runtime memory reference too
            auto* pRuntimeLboard = pGame->runtime.lboards;
            const auto* pRuntimeLboardStop = pRuntimeLboard + pGame->runtime.lboard_count;
            for (; pRuntimeLboard < pRuntimeLboardStop; ++pRuntimeLboard)
            {
                if (pRuntimeLboard->lboard == pOldLboard &&
                    pRuntimeLboard->id == m_pLeaderboardInfo->public_.id)
                {
                    pRuntimeLboard->lboard = m_pLeaderboardInfo->lboard;
                    pRuntimeLboard->serialized_size = 0;
                    memcpy(pRuntimeLboard->md5, md5, sizeof(md5));
                    break;
                }
            }

            m_pLeaderboardBuffer = std::move(lboard_buffer);

            // sync state to new leaderboard
            SyncStateToRuntime(GetState());

            // sync tracker to new leaderboard in case value changed
            SyncTrackerToRuntime();
        }
    }
    else
    {
        // parse error - discard old tracker
        ReleaseLeaderboardTracker(m_pLeaderboardInfo);
    }

    rc_mutex_unlock(&pClient->state.mutex);

    rc_destroy_preparse_state(&preparse);
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

void LeaderboardModel::InitializeFromPublishedLeaderboard(
    const struct rc_client_leaderboard_info_t& pLeaderboard, const std::string& sDefinition)
{
    SetID(pLeaderboard.public_.id);
    SetName(ra::util::String::Widen(pLeaderboard.public_.title));
    SetDescription(ra::util::String::Widen(pLeaderboard.public_.description));
    SetCategory(AssetCategory::Core);
    SetValueFormat(ra::itoe<ValueFormat>(pLeaderboard.format));
    SetLowerIsBetter(pLeaderboard.public_.lower_is_better);
    SetHidden(pLeaderboard.hidden);
    SetDefinition(sDefinition);

    CreateServerCheckpoint();
    CreateLocalCheckpoint();

    m_pPublishedLeaderboardInfo = &pLeaderboard;

    if (pLeaderboard.lboard)
        SyncStateFromRuntime(pLeaderboard.public_.state);
    else
        SetState(AssetState::Inactive);
}

const struct rc_lboard_t* LeaderboardModel::GetRuntimeLeaderboard() const
{
    if (m_pLeaderboardInfo != nullptr)
    {
        if (!m_pLeaderboardInfo->lboard)
            ParseDefinition();

        return m_pLeaderboardInfo->lboard;
    }

    return nullptr;
}

struct rc_lboard_t* LeaderboardModel::GetMutableRuntimeLeaderboard()
{
    if (m_pLeaderboardInfo != nullptr)
    {
        if (!m_pLeaderboardInfo->lboard)
            ParseDefinition();

        return m_pLeaderboardInfo->lboard;
    }

    return nullptr;
}

void LeaderboardModel::SetLocalLeaderboardInfo(struct rc_client_leaderboard_info_t& pLeaderboard)
{
    m_pLeaderboardInfo = &pLeaderboard;

    SyncStateToRuntime(GetState());
}

void LeaderboardModel::SyncToLocalLeaderboardInfo()
{
    Expects(m_pLeaderboardInfo != nullptr);

    SyncTitleToRuntime();
    SyncDescriptionToRuntime();
    SyncDefinitionToRuntime();
    SyncValueFormatToRuntime();
}

void LeaderboardModel::Serialize(ra::services::TextWriter& pWriter) const
{
    WriteQuoted(pWriter, GetLocalAssetDefinition(m_pStartTrigger));
    WriteQuoted(pWriter, GetLocalAssetDefinition(m_pCancelTrigger));
    WriteQuoted(pWriter, GetLocalAssetDefinition(m_pSubmitTrigger));
    WriteQuoted(pWriter, GetLocalAssetDefinition(m_pValueDefinition));

    WritePossiblyQuoted(pWriter, ValueFormatToString(GetValueFormat()));

    WritePossiblyQuoted(pWriter, GetLocalValue(NameProperty));
    WritePossiblyQuoted(pWriter, GetLocalValue(DescriptionProperty));
    WriteNumber(pWriter, IsLowerBetter() ? 1 : 0);
}

bool LeaderboardModel::Deserialize(ra::util::Tokenizer& pTokenizer)
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
    SetName(ra::util::String::Widen(sTitle));
    SetDescription(ra::util::String::Widen(sDescription));
    SetStartTrigger(sStartTrigger);
    SetSubmitTrigger(sSubmitTrigger);
    SetCancelTrigger(sCancelTrigger);
    SetValueDefinition(sValueDefinition);
    SetLowerIsBetter(nLowerIsBetter != 0);

    SetValueFormat(ValueFormatFromString(sFormat));

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
