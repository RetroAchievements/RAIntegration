#include "AchievementModel.hh"

#include "context\IRcClient.hh"

#include "util\Log.hh"

#include "data\context\GameContext.hh"

#include "data\models\LocalBadgesModel.hh"
#include "data\models\TriggerValidation.hh"

#include "services\IClock.hh"
#include "services\ServiceLocator.hh"

#include <rcheevos\src\rcheevos\rc_internal.h>
#include <rcheevos\src\rc_client_internal.h>

namespace ra {
namespace data {
namespace models {

const IntModelProperty AchievementModel::PointsProperty("AchievementModel", "Points", 5);
const IntModelProperty AchievementModel::AchievementTypeProperty("AchievementModel", "Type", ra::etoi(ra::data::models::AchievementType::None));
const StringModelProperty AchievementModel::BadgeProperty("AchievementModel", "Badge", L"00000");
const BoolModelProperty AchievementModel::PauseOnResetProperty("AchievementModel", "PauseOnReset", false);
const BoolModelProperty AchievementModel::PauseOnTriggerProperty("AchievementModel", "PauseOnTrigger", false);
const IntModelProperty AchievementModel::TriggerProperty("AchievementModel", "Trigger", 0);
const StringModelProperty AchievementModel::UnlockRichPresenceProperty("AchievementModel", "UnlockRichPresence", L"");

const std::array<int, 10> AchievementModel::s_vValidPoints = {0, 1, 2, 3, 4, 5, 10, 25, 50, 100};

AchievementModel::AchievementModel() noexcept
{
    GSL_SUPPRESS_F6 SetValue(TypeProperty, ra::etoi(AssetType::Achievement));

    SetTransactional(PointsProperty);
    SetTransactional(AchievementTypeProperty);
    SetTransactional(BadgeProperty);

    GSL_SUPPRESS_F6 AddAssetDefinition(m_pTrigger, TriggerProperty);
}

void AchievementModel::Activate()
{
    if (!IsActive())
        SetState(AssetState::Waiting);
}

void AchievementModel::Deactivate()
{
    SetState(AssetState::Inactive);
}

void AchievementModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    // if we're still loading the asset, ignore the event. we'll get recalled once loading finishes
    if (!IsUpdating())
    {
        if (args.Property == StateProperty)
        {
            HandleStateChanged(ra::itoe<AssetState>(args.tOldValue), ra::itoe<AssetState>(args.tNewValue));
        }
        else if (args.Property == TriggerProperty)
        {
            if (m_pAchievementInfo)
                SyncTriggerToRuntime();
        }
        else if (args.Property == PointsProperty)
        {
            if (m_pAchievementInfo)
                SyncPointsToRuntime();
        }
        else if (args.Property == AchievementTypeProperty)
        {
            if (m_pAchievementInfo)
                SyncAchievementTypeToRuntime();
        }
        else if (args.Property == IDProperty)
        {
            if (m_pAchievementInfo)
                SyncIDToRuntime();
        }
    }

    AssetModelBase::OnValueChanged(args);
}

void AchievementModel::OnValueChanged(const StringModelProperty::ChangeArgs& args)
{
    // call base first so the transaction gets updated for GetLocalValue to work properly
    AssetModelBase::OnValueChanged(args);

    // if we're still loading the asset, ignore the event. we'll get recalled once loading finishes
    if (IsUpdating())
        return;

    if (args.Property == DescriptionProperty)
    {
        if (m_pAchievementInfo)
            SyncDescriptionToRuntime();
    }
    else if (args.Property == NameProperty)
    {
        if (m_pAchievementInfo)
            SyncTitleToRuntime();
    }
    else if (args.Property == BadgeProperty)
    {
        if (ra::util::String::StartsWith(args.tOldValue, L"local\\"))
        {
            auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
            auto* pLocalBadges = dynamic_cast<LocalBadgesModel*>(pGameContext.Assets().FindAsset(ra::data::models::AssetType::LocalBadges, 0));
            if (pLocalBadges)
            {
                const bool bCommitted = (GetLocalValue(BadgeProperty) == args.tOldValue);
                pLocalBadges->RemoveReference(args.tOldValue, bCommitted);
            }
        }

        if (ra::util::String::StartsWith(args.tNewValue, L"local\\"))
        {
            auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
            auto* pLocalBadges = dynamic_cast<LocalBadgesModel*>(pGameContext.Assets().FindAsset(ra::data::models::AssetType::LocalBadges, 0));
            if (pLocalBadges)
            {
                const bool bCommitted = (GetLocalValue(BadgeProperty) == args.tNewValue);
                pLocalBadges->AddReference(args.tNewValue, bCommitted);
            }
        }

        if (m_pAchievementInfo)
            SyncBadgeToRuntime();
    }
}

void AchievementModel::CommitTransaction()
{
    Expects(m_pTransaction != nullptr);
    const auto* pPreviousBadgeName = m_pTransaction->GetPreviousValue(BadgeProperty);
    if (pPreviousBadgeName != nullptr)
    {
        const auto& pBadgeName = GetBadge();
        if (*pPreviousBadgeName != pBadgeName)
        {
            auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
            auto* pLocalBadges = dynamic_cast<LocalBadgesModel*>(pGameContext.Assets().FindAsset(ra::data::models::AssetType::LocalBadges, 0));
            if (pLocalBadges)
                pLocalBadges->Commit(*pPreviousBadgeName, pBadgeName);
        }
    }

    AssetModelBase::CommitTransaction();
}

bool AchievementModel::IsShownInList() const
{
    // don't show warning achievements in asset list
    if (GetID() > 101000000 && GetCategory() == AssetCategory::Core)
        return false;

    return true;
}

bool AchievementModel::ValidateAsset(std::wstring& sError)
{
    const auto pIter = std::find(s_vValidPoints.begin(), s_vValidPoints.end(), GetPoints());
    if (pIter == s_vValidPoints.end())
    {
        sError = L"Invalid point value";
        return false;
    }

    const auto& sTrigger = GetAssetDefinition(m_pTrigger);
    if (sTrigger.length() > MaxSerializedLength)
    {
        sError = ra::util::String::Printf(L"Serialized length exceeds limit: %d/%d", sTrigger.length(), MaxSerializedLength);
        return false;
    }

    return TriggerValidation::Validate(sTrigger, sError, AssetType::Achievement);
}

void AchievementModel::DoFrame()
{
    if (m_pAchievementInfo && m_pAchievementInfo->trigger)
        SyncStateFromRuntime(m_pAchievementInfo->trigger->state);
}

void AchievementModel::SyncStateFromRuntime(uint8_t nState)
{
    switch (nState)
    {
        case RC_TRIGGER_STATE_ACTIVE:
            SetState(AssetState::Active);
            break;
        case RC_TRIGGER_STATE_INACTIVE:
            SetState(AssetState::Inactive);
            break;
        case RC_TRIGGER_STATE_PAUSED:
            SetState(AssetState::Paused);
            break;
        case RC_TRIGGER_STATE_TRIGGERED:
            SetState(AssetState::Triggered);
            break;
        case RC_TRIGGER_STATE_WAITING:
            SetState(AssetState::Waiting);
            break;
        case RC_TRIGGER_STATE_DISABLED:
            SetState(AssetState::Disabled);
            break;
        case RC_TRIGGER_STATE_PRIMED:
            SetState(AssetState::Primed);
            break;
    }
}

static void HideChallengeIndicator(struct rc_client_achievement_info_t* pAchievement)
{
    auto* client = ra::services::ServiceLocator::Get<ra::context::IRcClient>().GetClient();
    if (client->game && client->callbacks.event_handler)
    {
        rc_client_event_t pEvent;
        memset(&pEvent, 0, sizeof(pEvent));
        pEvent.achievement = &pAchievement->public_;
        pEvent.type = RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_HIDE;
        client->callbacks.event_handler(&pEvent, client);
    }
}

void AchievementModel::HandleStateChanged(AssetState nOldState, AssetState nNewState)
{
    if (!m_pAchievementInfo)
        return;

    const bool bWasActive = IsActive(nOldState);
    const bool bIsActive = IsActive(nNewState);

    if (!bIsActive && bWasActive)
    {
        if (m_pAchievementInfo->trigger)
        {
            if (m_pAchievementInfo->trigger->state == RC_TRIGGER_STATE_PRIMED)
            {
                // the runtime thinks the achievement is primed, so there's most likely a challenge
                // indicator visible. raise the event to force it to be hidden
                HideChallengeIndicator(m_pAchievementInfo);
            }
        }
    }
    else if (bIsActive && !bWasActive)
    {
        if (m_pAchievementInfo->trigger)
            rc_reset_trigger(m_pAchievementInfo->trigger);
        else
            SyncTriggerToRuntime();
    }

    SyncStateToRuntime();
}

void AchievementModel::SyncStateToRuntime() const
{
    auto* pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>().GetClient();

    rc_mutex_lock(&pClient->state.mutex);

    switch (GetState())
    {
        case ra::data::models::AssetState::Triggered: {
            m_pAchievementInfo->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
            if (m_pAchievementInfo->trigger)
                m_pAchievementInfo->trigger->state = RC_TRIGGER_STATE_TRIGGERED;
            break;
        }

        case ra::data::models::AssetState::Disabled:
            m_pAchievementInfo->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_DISABLED;
            m_pAchievementInfo->public_.bucket = RC_CLIENT_ACHIEVEMENT_BUCKET_UNSUPPORTED;
            if (m_pAchievementInfo->trigger)
                m_pAchievementInfo->trigger->state = RC_TRIGGER_STATE_DISABLED;
            break;

        case ra::data::models::AssetState::Inactive:
            m_pAchievementInfo->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_INACTIVE;
            if (m_pAchievementInfo->trigger)
                m_pAchievementInfo->trigger->state = RC_TRIGGER_STATE_INACTIVE;
            break;

        case ra::data::models::AssetState::Waiting:
            m_pAchievementInfo->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
            if (m_pAchievementInfo->trigger)
                m_pAchievementInfo->trigger->state = RC_TRIGGER_STATE_WAITING;
            break;

        case ra::data::models::AssetState::Active:
            m_pAchievementInfo->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
            if (m_pAchievementInfo->trigger)
                m_pAchievementInfo->trigger->state = RC_TRIGGER_STATE_ACTIVE;
            break;

        case ra::data::models::AssetState::Primed:
            m_pAchievementInfo->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
            if (m_pAchievementInfo->trigger)
                m_pAchievementInfo->trigger->state = RC_TRIGGER_STATE_PRIMED;
            break;

        default:
            m_pAchievementInfo->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
            break;
    }

    rc_mutex_unlock(&pClient->state.mutex);
}

void AchievementModel::SyncIDToRuntime() const
{
    m_pAchievementInfo->public_.id = GetID();
}

void AchievementModel::SyncTitleToRuntime()
{
    m_sTitleBuffer = ra::util::String::Narrow(GetName());
    m_pAchievementInfo->public_.title = m_sTitleBuffer.c_str();
}

void AchievementModel::SyncDescriptionToRuntime()
{
    m_sDescriptionBuffer = ra::util::String::Narrow(GetDescription());
    m_pAchievementInfo->public_.description = m_sDescriptionBuffer.c_str();
}

void AchievementModel::SyncBadgeToRuntime() const
{
    const auto& sBadge = GetBadge();
    if (ra::util::String::StartsWith(sBadge, L"local\\"))
    {
        // cannot fit "local/md5.png" into 8 byte buffer. also, client may not understand.
        // encode a value that we can intercept in rc_client_achievement_get_image_url.
        snprintf(m_pAchievementInfo->public_.badge_name, sizeof(m_pAchievementInfo->public_.badge_name), "L%06x",
                 m_pAchievementInfo->public_.id);
    }
    else
    {
        snprintf(m_pAchievementInfo->public_.badge_name, sizeof(m_pAchievementInfo->public_.badge_name), "%s",
                 ra::util::String::Narrow(sBadge).c_str());
    }
}

void AchievementModel::SyncPointsToRuntime() const
{
    m_pAchievementInfo->public_.points = GetPoints();
}

void AchievementModel::SyncAchievementTypeToRuntime() const
{
    switch (GetAchievementType())
    {
        case ra::data::models::AchievementType::None:
            m_pAchievementInfo->public_.type = RC_CLIENT_ACHIEVEMENT_TYPE_STANDARD;
            break;
        case ra::data::models::AchievementType::Missable:
            m_pAchievementInfo->public_.type = RC_CLIENT_ACHIEVEMENT_TYPE_MISSABLE;
            break;
        case ra::data::models::AchievementType::Progression:
            m_pAchievementInfo->public_.type = RC_CLIENT_ACHIEVEMENT_TYPE_PROGRESSION;
            break;
        case ra::data::models::AchievementType::Win:
            m_pAchievementInfo->public_.type = RC_CLIENT_ACHIEVEMENT_TYPE_WIN;
            break;
        default:
            m_pAchievementInfo->public_.type = gsl::narrow_cast<uint8_t>(GetValue(AchievementTypeProperty));
            break;
    }
}

void AchievementModel::SyncCategoryToRuntime() const
{
    switch (GetCategory())
    {
        case ra::data::models::AssetCategory::Core:
        case ra::data::models::AssetCategory::Local:
            m_pAchievementInfo->public_.category = RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE;
            break;

        default:
            m_pAchievementInfo->public_.category = RC_CLIENT_ACHIEVEMENT_CATEGORY_UNOFFICIAL;
            break;
    }
}

void AchievementModel::SyncTriggerToRuntime()
{
    Expects(m_pAchievementInfo != nullptr);

    if (!IsActive())
    {
        // if the achievement isn't active, we don't have to parse it or load it into the runtime.
        m_pAchievementInfo->trigger = nullptr;
        m_pTriggerBuffer.reset();

        memset(m_pAchievementInfo->md5, 0, sizeof(m_pAchievementInfo->md5));
        return;
    }

    ParseTrigger();
}

void AchievementModel::ParseTrigger() const
{
    const auto& sTrigger = GetTrigger();
    uint8_t md5[16];
    rc_runtime_checksum(sTrigger.c_str(), md5);

    if (memcmp(m_pAchievementInfo->md5, md5, sizeof(md5)) == 0)
    {
        // trigger is already up-to-date. do nothing.
        return;
    }

    memcpy(m_pAchievementInfo->md5, md5, sizeof(md5));

    if (m_pPublishedAchievementInfo && memcmp(m_pPublishedAchievementInfo->md5, md5, sizeof(md5)) == 0)
    {
        // trigger changed back to published state. just reference that.
        if (m_pPublishedAchievementInfo->trigger != nullptr)
            rc_reset_trigger(m_pPublishedAchievementInfo->trigger);

        m_pAchievementInfo->trigger = m_pPublishedAchievementInfo->trigger;
        m_pTriggerBuffer.reset();
        return;
    }

    // attempt to parse the trigger
    auto* pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>().GetClient();
    auto* pGame = pClient->game;
    Expects(pGame != nullptr);

    rc_mutex_lock(&pClient->state.mutex);

    rc_preparse_state_t preparse;
    rc_init_preparse_state(&preparse);
    preparse.parse.existing_memrefs = pGame->runtime.memrefs;

    rc_trigger_with_memrefs_t* trigger = RC_ALLOC(rc_trigger_with_memrefs_t, &preparse.parse);
    const char* sMemaddr = sTrigger.c_str();
    rc_parse_trigger_internal(&trigger->trigger, &sMemaddr, &preparse.parse);
    rc_preparse_alloc_memrefs(nullptr, &preparse);

    const auto nSize = preparse.parse.offset;
    if (nSize > 0)
    {
        auto trigger_buffer = std::make_unique<uint8_t[]>(nSize);
        if (trigger_buffer)
        {
            // populate the item, using the communal memrefs pool
            rc_reset_parse_state(&preparse.parse, trigger_buffer.get());
            trigger = RC_ALLOC(rc_trigger_with_memrefs_t, &preparse.parse);
            rc_preparse_alloc_memrefs(&trigger->memrefs, &preparse);
            Expects(preparse.parse.memrefs == &trigger->memrefs);

            preparse.parse.existing_memrefs = pGame->runtime.memrefs;

            sMemaddr = sTrigger.c_str();
            rc_parse_trigger_internal(&trigger->trigger, &sMemaddr, &preparse.parse);
            trigger->trigger.has_memrefs = 1;

            const auto* pOldTrigger = m_pAchievementInfo->trigger;
            m_pAchievementInfo->trigger = &trigger->trigger;

            // update the runtime memory reference too
            auto* pRuntimeTrigger = pGame->runtime.triggers;
            const auto* pRuntimeTriggerStop = pRuntimeTrigger + pGame->runtime.trigger_count;
            for (; pRuntimeTrigger < pRuntimeTriggerStop; ++pRuntimeTrigger)
            {
                if (pRuntimeTrigger->trigger == pOldTrigger &&
                    pRuntimeTrigger->id == m_pAchievementInfo->public_.id)
                {
                    pRuntimeTrigger->trigger = m_pAchievementInfo->trigger;
                    pRuntimeTrigger->serialized_size = 0;
                    memcpy(pRuntimeTrigger->md5, md5, sizeof(md5));
                    break;
                }
            }

            m_pTriggerBuffer = std::move(trigger_buffer);

            SyncStateToRuntime();
        }
    }

    rc_mutex_unlock(&pClient->state.mutex);

    rc_destroy_preparse_state(&preparse);
}

void AchievementModel::InitializeFromPublishedAchievement(
    const struct rc_client_achievement_info_t& pAchievement, const std::string& sTrigger)
{
    SetID(pAchievement.public_.id);
    SetName(ra::util::String::Widen(pAchievement.public_.title));
    SetDescription(ra::util::String::Widen(pAchievement.public_.description));
    SetPoints(pAchievement.public_.points);
    if (pAchievement.author)
        SetAuthor(ra::util::String::Widen(pAchievement.author));
    SetBadge(ra::util::String::Widen(pAchievement.public_.badge_name));
    SetCreationTime(pAchievement.created_time);
    SetUpdatedTime(pAchievement.updated_time);
    SetTrigger(sTrigger);

    switch (pAchievement.public_.category)
    {
        case RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE:
            SetCategory(AssetCategory::Core);
            break;

        case RC_CLIENT_ACHIEVEMENT_CATEGORY_UNOFFICIAL:
            SetCategory(AssetCategory::Unofficial);
            break;

        default:
            SetCategory(AssetCategory::None);
            break;
    }

    switch (pAchievement.public_.type)
    {
        case RC_CLIENT_ACHIEVEMENT_TYPE_STANDARD:
            SetAchievementType(ra::data::models::AchievementType::None);
            break;
        case RC_CLIENT_ACHIEVEMENT_TYPE_MISSABLE:
            SetAchievementType(ra::data::models::AchievementType::Missable);
            break;
        case RC_CLIENT_ACHIEVEMENT_TYPE_PROGRESSION:
            SetAchievementType(ra::data::models::AchievementType::Progression);
            break;
        case RC_CLIENT_ACHIEVEMENT_TYPE_WIN:
            SetAchievementType(ra::data::models::AchievementType::Win);
            break;
        default:
            RA_LOG_ERR("Unsupported achievement type: %d", pAchievement.public_.type);
            SetValue(AchievementTypeProperty, pAchievement.public_.type);
            break;
    }

    CreateServerCheckpoint();
    CreateLocalCheckpoint();

    m_pPublishedAchievementInfo = &pAchievement;

    if (pAchievement.trigger)
        SyncStateFromRuntime(pAchievement.trigger->state);
    else
        SetState(AssetState::Inactive);
}

const struct rc_trigger_t* AchievementModel::GetRuntimeTrigger() const
{
    if (m_pAchievementInfo != nullptr)
    {
        if (!m_pAchievementInfo->trigger)
            ParseTrigger();

        return m_pAchievementInfo->trigger;
    }

    return nullptr;
}

void AchievementModel::SetLocalAchievementInfo(struct rc_client_achievement_info_t& pAchievement)
{
    m_pAchievementInfo = &pAchievement;

    // make sure the runtime state matches the model state
    SyncStateToRuntime();
}

void AchievementModel::SyncToLocalAchievementInfo()
{
    Expects(m_pAchievementInfo != nullptr);

    SyncIDToRuntime();
    SyncTitleToRuntime();
    SyncDescriptionToRuntime();
    SyncBadgeToRuntime();
    SyncPointsToRuntime();
    SyncCategoryToRuntime();
    SyncTriggerToRuntime();
    SyncStateToRuntime();
    SyncAchievementTypeToRuntime();
}

void AchievementModel::Serialize(ra::services::TextWriter& pWriter) const
{
    WriteQuoted(pWriter, GetLocalAssetDefinition(m_pTrigger));
    WritePossiblyQuoted(pWriter, GetLocalValue(NameProperty));
    WritePossiblyQuoted(pWriter, GetLocalValue(DescriptionProperty));
    pWriter.Write("::"); // progress/max

    switch (GetAchievementType())
    {
        case ra::data::models::AchievementType::None:
            WritePossiblyQuoted(pWriter, "");
            break;
        case ra::data::models::AchievementType::Missable:
            WritePossiblyQuoted(pWriter, "missable");
            break;
        case ra::data::models::AchievementType::Progression:
            WritePossiblyQuoted(pWriter, "progression");
            break;
        case ra::data::models::AchievementType::Win:
            WritePossiblyQuoted(pWriter, "win_condition");
            break;
        default:
            WriteNumber(pWriter, GetValue(TypeProperty));
            break;
    }

    WritePossiblyQuoted(pWriter, GetLocalValue(AuthorProperty));
    WriteNumber(pWriter, GetLocalValue(PointsProperty));
    pWriter.Write("::::"); // created/modified/upvotes/downvotes
    WritePossiblyQuoted(pWriter, GetLocalValue(BadgeProperty));
}

bool AchievementModel::Deserialize(ra::util::Tokenizer& pTokenizer)
{
    // field 2: trigger
    std::string sTrigger;
    if (pTokenizer.PeekChar() == '"')
    {
        sTrigger = pTokenizer.ReadQuotedString();
    }
    else
    {
        // unquoted trigger requires special parsing because flags also use colons
        const char* pTrigger = pTokenizer.GetPointer(pTokenizer.CurrentPosition());
        const char* pScan = pTrigger;
        Expects(pScan != nullptr);
        while (*pScan && (*pScan != ':' || strchr("ABCMNOPRTabcmnoprt", pScan[-1]) != nullptr))
            pScan++;

        sTrigger.assign(pTrigger, pScan - pTrigger);
        pTokenizer.Advance(sTrigger.length());
    }

    if (!pTokenizer.Consume(':'))
        return false;

    // field 3: title
    std::string sTitle;
    if (!ReadPossiblyQuoted(pTokenizer, sTitle))
        return false;

    // field 4: description
    std::string sDescription;
    if (!ReadPossiblyQuoted(pTokenizer, sDescription))
        return false;

    // field 5: progress (unused)
    pTokenizer.AdvanceTo(':');
    if (!pTokenizer.Consume(':'))
        return false;

    // field 6: progress max (unused)
    pTokenizer.AdvanceTo(':');
    if (!pTokenizer.Consume(':'))
        return false;

    // field 7: achievement type
    std::string sType;
    if (!ReadPossiblyQuoted(pTokenizer, sType))
        return false;

    // field 8: author (unused)
    std::string sAuthor;
    if (!ReadPossiblyQuoted(pTokenizer, sAuthor))
        return false;

    // field 9: points
    uint32_t nPoints;
    if (!ReadNumber(pTokenizer, nPoints))
        return false;

    // field 10: created date (unused)
    pTokenizer.AdvanceTo(':');
    if (!pTokenizer.Consume(':'))
        return false;

    // field 11: modified date (unused)
    pTokenizer.AdvanceTo(':');
    if (!pTokenizer.Consume(':'))
        return false;

    // field 12: up votes (unused)
    pTokenizer.AdvanceTo(':');
    if (!pTokenizer.Consume(':'))
        return false;

    // field 13: down votes (unused)
    pTokenizer.AdvanceTo(':');
    if (!pTokenizer.Consume(':'))
        return false;

    // field 14: badge
    std::string sBadge;
    if (!ReadPossiblyQuoted(pTokenizer, sBadge))
        return false;
    if (sBadge.length() < 5)
        sBadge.insert(0, 5 - sBadge.length(), '0');

    // line is valid
    SetName(ra::util::String::Widen(sTitle));
    SetDescription(ra::util::String::Widen(sDescription));
    if (GetID() >= ra::data::context::GameAssets::FirstLocalId || GetAuthor().empty())
        SetAuthor(ra::util::String::Widen(sAuthor));
    SetPoints(nPoints);
    SetBadge(ra::util::String::Widen(sBadge));
    SetTrigger(sTrigger);

    if (sType.empty())
        SetAchievementType(ra::data::models::AchievementType::None);
    else if (sType == "missable")
        SetAchievementType(ra::data::models::AchievementType::Missable);
    else if (sType == "progression")
        SetAchievementType(ra::data::models::AchievementType::Progression);
    else if (sType == "win_condition")
        SetAchievementType(ra::data::models::AchievementType::Win);
    else
        SetValue(AchievementTypeProperty, atoi(sType.c_str()));

    return true;
}

} // namespace models
} // namespace data
} // namespace ra
