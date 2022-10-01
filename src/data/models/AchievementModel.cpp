#include "AchievementModel.hh"

#include "data\context\GameContext.hh"

#include "data\models\LocalBadgesModel.hh"
#include "data\models\TriggerValidation.hh"

#include "services\AchievementRuntime.hh"
#include "services\IClock.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace data {
namespace models {

const IntModelProperty AchievementModel::PointsProperty("AchievementModel", "Points", 5);
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
            if (IsActive())
            {
                auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
                pRuntime.ActivateAchievement(GetID(), GetTrigger());
            }
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

    if (args.Property == BadgeProperty)
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
        sError = ra::StringPrintf(L"Serialized length exceeds database limit: %d/%d", sTrigger.length(), MaxSerializedLength);
        return false;
    }

    return TriggerValidation::Validate(sTrigger, sError, AssetType::Achievement);
}

void AchievementModel::DoFrame()
{
    const auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
    const auto* pTrigger = pRuntime.GetAchievementTrigger(GetID());
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
    if (!ra::services::ServiceLocator::Exists<ra::services::AchievementRuntime>())
        return;

    const bool bWasActive = IsActive(nOldState);
    const bool bIsActive = IsActive(nNewState);
    if (bWasActive == bIsActive)
        return;

    auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
    auto* pTrigger = pRuntime.GetAchievementTrigger(GetID());

    if (bIsActive)
    {
        const bool bRuntimeActive = (pTrigger != nullptr);
        if (!bRuntimeActive)
        {
            pRuntime.ActivateAchievement(GetID(), GetTrigger());
            pTrigger = pRuntime.GetAchievementTrigger(GetID());
        }
        else
        {
            rc_reset_trigger(pTrigger);
        }
    }
    else if (pTrigger)
    {
        m_pCapturedTriggerHits.Capture(pTrigger, GetTrigger());
        pRuntime.DeactivateAchievement(GetID());
    }

    switch (nNewState)
    {
        case AssetState::Waiting:
            if (pTrigger)
                pTrigger->state = RC_TRIGGER_STATE_WAITING;
            break;

        case AssetState::Active:
            if (pTrigger)
                pTrigger->state = RC_TRIGGER_STATE_ACTIVE;
            break;

        case AssetState::Inactive:
            if (pTrigger)
                pTrigger->state = RC_TRIGGER_STATE_INACTIVE;
            break;

        case AssetState::Primed:
            if (pTrigger)
                pTrigger->state = RC_TRIGGER_STATE_PRIMED;
            break;

        case AssetState::Triggered:
            const auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
            m_tUnlock = pClock.Now();

            if (pRuntime.HasRichPresence())
                SetUnlockRichPresence(pRuntime.GetRichPresenceDisplayString());
            break;
    }
}

void AchievementModel::Serialize(ra::services::TextWriter& pWriter) const
{
    WriteQuoted(pWriter, GetLocalAssetDefinition(m_pTrigger));
    WritePossiblyQuoted(pWriter, GetLocalValue(NameProperty));
    WritePossiblyQuoted(pWriter, GetLocalValue(DescriptionProperty));
    pWriter.Write(":::"); // progress/max/format/author
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

    // field 7: progress format (unused)
    pTokenizer.AdvanceTo(':');
    if (!pTokenizer.Consume(':'))
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

    return true;
}

} // namespace models
} // namespace data
} // namespace ra
