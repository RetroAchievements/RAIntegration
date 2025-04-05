#include "AchievementModel.hh"

#include "RA_Log.h"

#include "data\context\GameContext.hh"

#include "data\models\LocalBadgesModel.hh"
#include "data\models\TriggerValidation.hh"

#include "services\AchievementRuntime.hh"
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
            if (m_pAchievement)
                SyncTrigger();
        }
        else if (args.Property == PointsProperty)
        {
            if (m_pAchievement)
                SyncPoints();
        }
        else if (args.Property == AchievementTypeProperty)
        {
            if (m_pAchievement)
                SyncAchievementType();
        }
        else if (args.Property == IDProperty)
        {
            if (m_pAchievement)
                SyncID();
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
        if (m_pAchievement)
            SyncDescription();
    }
    else if (args.Property == NameProperty)
    {
        if (m_pAchievement)
            SyncTitle();
    }
    else if (args.Property == BadgeProperty)
    {
        if (ra::StringStartsWith(args.tOldValue, L"local\\"))
        {
            auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
            auto* pLocalBadges = dynamic_cast<LocalBadgesModel*>(pGameContext.Assets().FindAsset(ra::data::models::AssetType::LocalBadges, 0));
            if (pLocalBadges)
            {
                const bool bCommitted = (GetLocalValue(BadgeProperty) == args.tOldValue);
                pLocalBadges->RemoveReference(args.tOldValue, bCommitted);
            }
        }

        if (ra::StringStartsWith(args.tNewValue, L"local\\"))
        {
            auto& pGameContext = ra::services::ServiceLocator::GetMutable<ra::data::context::GameContext>();
            auto* pLocalBadges = dynamic_cast<LocalBadgesModel*>(pGameContext.Assets().FindAsset(ra::data::models::AssetType::LocalBadges, 0));
            if (pLocalBadges)
            {
                const bool bCommitted = (GetLocalValue(BadgeProperty) == args.tNewValue);
                pLocalBadges->AddReference(args.tNewValue, bCommitted);
            }
        }

        if (m_pAchievement)
            SyncBadge();
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
        sError = ra::StringPrintf(L"Serialized length exceeds limit: %d/%d", sTrigger.length(), MaxSerializedLength);
        return false;
    }

    return TriggerValidation::Validate(sTrigger, sError, AssetType::Achievement);
}

void AchievementModel::DoFrame()
{
    if (!m_pAchievement)
        return;

    const auto* pTrigger = m_pAchievement->trigger;
    if (pTrigger == nullptr)
    {
        if (IsActive())
            SetState(AssetState::Inactive);
    }
    else
    {
        switch (pTrigger->state)
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
}

void AchievementModel::HandleStateChanged(AssetState nOldState, AssetState nNewState)
{
    if (!m_pAchievement)
        return;

    const bool bWasActive = IsActive(nOldState);
    const bool bIsActive = IsActive(nNewState);

    if (!bIsActive && bWasActive)
    {
        Expects(m_pAchievement->trigger != nullptr);
        m_pCapturedTriggerHits.Capture(m_pAchievement->trigger, GetTrigger());

        if (m_pAchievement->trigger->state == RC_TRIGGER_STATE_PRIMED)
        {
            auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
            pRuntime.RaiseClientEvent(*m_pAchievement, RC_CLIENT_EVENT_ACHIEVEMENT_CHALLENGE_INDICATOR_HIDE);
        }
    }
    else if (bIsActive && !bWasActive)
    {
        if (m_pAchievement->trigger)
            rc_reset_trigger(m_pAchievement->trigger);
        else
            SyncTrigger();
    }

    SyncState();
}

void AchievementModel::SyncState()
{
    switch (GetState())
    {
        case ra::data::models::AssetState::Triggered: {
            m_pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_UNLOCKED;
            if (m_pAchievement->trigger)
                m_pAchievement->trigger->state = RC_TRIGGER_STATE_TRIGGERED;

            if (m_bCaptureTrigger)
            {
                const auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
                m_tUnlock = pClock.Now();

                auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
                if (pRuntime.HasRichPresence())
                    SetUnlockRichPresence(pRuntime.GetRichPresenceDisplayString());
            }
            break;
        }

        case ra::data::models::AssetState::Disabled:
            m_pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_DISABLED;
            m_pAchievement->public_.bucket = RC_CLIENT_ACHIEVEMENT_BUCKET_UNSUPPORTED;
            if (m_pAchievement->trigger)
                m_pAchievement->trigger->state = RC_TRIGGER_STATE_DISABLED;
            break;

        case ra::data::models::AssetState::Inactive:
            m_pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_INACTIVE;
            if (m_pAchievement->trigger)
                m_pAchievement->trigger->state = RC_TRIGGER_STATE_INACTIVE;
            break;

        case ra::data::models::AssetState::Waiting:
            m_pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
            if (m_pAchievement->trigger)
                m_pAchievement->trigger->state = RC_TRIGGER_STATE_WAITING;
            break;

        case ra::data::models::AssetState::Active:
            m_pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
            if (m_pAchievement->trigger)
                m_pAchievement->trigger->state = RC_TRIGGER_STATE_ACTIVE;
            break;

        case ra::data::models::AssetState::Primed:
            m_pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
            if (m_pAchievement->trigger)
                m_pAchievement->trigger->state = RC_TRIGGER_STATE_PRIMED;
            break;

        default:
            m_pAchievement->public_.state = RC_CLIENT_ACHIEVEMENT_STATE_ACTIVE;
            break;
    }
}

void AchievementModel::SyncID()
{
    m_pAchievement->public_.id = GetID();
}

void AchievementModel::SyncTitle()
{
    m_sTitleBuffer = ra::Narrow(GetName());
    m_pAchievement->public_.title = m_sTitleBuffer.c_str();
}

void AchievementModel::SyncDescription()
{
    m_sDescriptionBuffer = ra::Narrow(GetDescription());
    m_pAchievement->public_.description = m_sDescriptionBuffer.c_str();
}

void AchievementModel::SyncBadge()
{
    const auto& sBadge = GetBadge();
    if (ra::StringStartsWith(sBadge, L"local\\"))
    {
        // cannot fit "local/md5.png" into 8 byte buffer. also, client may not understand.
        // encode a value that we can intercept in rc_client_achievement_get_image_url.
        snprintf(m_pAchievement->public_.badge_name, sizeof(m_pAchievement->public_.badge_name), "L%06x",
                 m_pAchievement->public_.id);
    }
    else
    {
        snprintf(m_pAchievement->public_.badge_name, sizeof(m_pAchievement->public_.badge_name), "%s",
                 ra::Narrow(sBadge).c_str());
    }
}

void AchievementModel::SyncPoints()
{
    m_pAchievement->public_.points = GetPoints();
}

void AchievementModel::SyncAchievementType()
{
    switch (GetAchievementType())
    {
        case ra::data::models::AchievementType::None:
            m_pAchievement->public_.type = RC_CLIENT_ACHIEVEMENT_TYPE_STANDARD;
            break;
        case ra::data::models::AchievementType::Missable:
            m_pAchievement->public_.type = RC_CLIENT_ACHIEVEMENT_TYPE_MISSABLE;
            break;
        case ra::data::models::AchievementType::Progression:
            m_pAchievement->public_.type = RC_CLIENT_ACHIEVEMENT_TYPE_PROGRESSION;
            break;
        case ra::data::models::AchievementType::Win:
            m_pAchievement->public_.type = RC_CLIENT_ACHIEVEMENT_TYPE_WIN;
            break;
        default:
            m_pAchievement->public_.type = gsl::narrow_cast<uint8_t>(GetValue(AchievementTypeProperty));
            break;
    }
}

void AchievementModel::SyncCategory()
{
    switch (GetCategory())
    {
        case ra::data::models::AssetCategory::Core:
        case ra::data::models::AssetCategory::Local:
            m_pAchievement->public_.category = RC_CLIENT_ACHIEVEMENT_CATEGORY_CORE;
            break;

        default:
            m_pAchievement->public_.category = RC_CLIENT_ACHIEVEMENT_CATEGORY_UNOFFICIAL;
            break;
    }
}

void AchievementModel::SyncTrigger()
{
    if (!IsActive())
    {
        if (m_pAchievement->trigger)
        {
            auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
            if (pRuntime.DetachMemory(m_pAchievement->trigger))
                free(m_pAchievement->trigger);

            m_pAchievement->trigger = nullptr;
        }

        memset(m_pAchievement->md5, 0, sizeof(m_pAchievement->md5));
        return;
    }

    const auto& sTrigger = GetTrigger();
    uint8_t md5[16];
    rc_runtime_checksum(sTrigger.c_str(), md5);
    if (memcmp(m_pAchievement->md5, md5, sizeof(md5)) != 0)
    {
        memcpy(m_pAchievement->md5, md5, sizeof(md5));

        const auto* pOldTrigger = m_pAchievement->trigger;
        auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
        if (m_pAchievement->trigger && pRuntime.DetachMemory(m_pAchievement->trigger))
        {
            free(m_pAchievement->trigger);
            m_pAchievement->trigger = nullptr;
        }

        const auto* pPublishedAchievementInfo = pRuntime.GetPublishedAchievementInfo(m_pAchievement->public_.id);
        if (pPublishedAchievementInfo && memcmp(pPublishedAchievementInfo->md5, md5, sizeof(md5)) == 0)
        {
            Expects(pPublishedAchievementInfo->trigger != nullptr);
            m_pAchievement->trigger = pPublishedAchievementInfo->trigger;
            rc_reset_trigger(m_pAchievement->trigger);
            return;
        }

        auto* pGame = pRuntime.GetClient()->game;
        Expects(pGame != nullptr);

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
            void* trigger_buffer = malloc(nSize);
            if (trigger_buffer)
            {
                // populate the item, using the communal memrefs pool
                rc_reset_parse_state(&preparse.parse, trigger_buffer);
                trigger = RC_ALLOC(rc_trigger_with_memrefs_t, &preparse.parse);
                rc_preparse_alloc_memrefs(&trigger->memrefs, &preparse);
                Expects(preparse.parse.memrefs == &trigger->memrefs);

                preparse.parse.existing_memrefs = pGame->runtime.memrefs;

                sMemaddr = sTrigger.c_str();
                m_pAchievement->trigger = &trigger->trigger;
                rc_parse_trigger_internal(m_pAchievement->trigger, &sMemaddr, &preparse.parse);
                trigger->trigger.has_memrefs = 1;

                pRuntime.AttachMemory(m_pAchievement->trigger);

                // update the runtime memory reference too
                auto* pRuntimeTrigger = pGame->runtime.triggers;
                const auto* pRuntimeTriggerStop = pRuntimeTrigger + pGame->runtime.trigger_count;
                for (; pRuntimeTrigger < pRuntimeTriggerStop; ++pRuntimeTrigger)
                {
                    if (pRuntimeTrigger->trigger == pOldTrigger &&
                        pRuntimeTrigger->id == m_pAchievement->public_.id)
                    {
                        pRuntimeTrigger->trigger = m_pAchievement->trigger;
                        pRuntimeTrigger->serialized_size = 0;
                        memcpy(pRuntimeTrigger->md5, md5, sizeof(md5));
                        break;
                    }
                }
            }
        }

        rc_destroy_preparse_state(&preparse);
    }
}

void AchievementModel::Attach(struct rc_client_achievement_info_t& pAchievement,
    AssetCategory nCategory, const std::string& sTrigger)
{
    SetID(pAchievement.public_.id);
    SetName(ra::Widen(pAchievement.public_.title));
    SetDescription(ra::Widen(pAchievement.public_.description));
    SetCategory(nCategory);
    SetPoints(pAchievement.public_.points);
    if (pAchievement.author)
        SetAuthor(ra::Widen(pAchievement.author));
    SetBadge(ra::Widen(pAchievement.public_.badge_name));
    SetCreationTime(pAchievement.created_time);
    SetUpdatedTime(pAchievement.updated_time);
    SetTrigger(sTrigger);

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

    m_pAchievement = &pAchievement;

    m_bCaptureTrigger = false;
    DoFrame(); // sync state
    m_bCaptureTrigger = true;
}

void AchievementModel::ReplaceAttached(struct rc_client_achievement_info_t& pAchievement) noexcept
{
    m_pAchievement = &pAchievement;
}

void AchievementModel::AttachAndInitialize(struct rc_client_achievement_info_t& pAchievement)
{
    m_pAchievement = &pAchievement;

    SyncID();
    SyncTitle();
    SyncDescription();
    SyncBadge();
    SyncPoints();
    SyncCategory();
    SyncTrigger();

    m_bCaptureTrigger = false;
    SyncState();
    m_bCaptureTrigger = true;

    SyncAchievementType();
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

bool AchievementModel::Deserialize(ra::Tokenizer& pTokenizer)
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
    SetName(ra::Widen(sTitle));
    SetDescription(ra::Widen(sDescription));
    if (GetID() >= ra::data::context::GameAssets::FirstLocalId || GetAuthor().empty())
        SetAuthor(ra::Widen(sAuthor));
    SetPoints(nPoints);
    SetBadge(ra::Widen(sBadge));
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
