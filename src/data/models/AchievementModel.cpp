#include "AchievementModel.hh"

#include "data\context\GameContext.hh"

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
const IntModelProperty AchievementModel::TriggerProperty("AchievementModel", "Trigger", ra::etoi(AssetChanges::None));

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
    if (args.Property == StateProperty)
    {
        const bool bWasActive = IsActive(ra::itoe<AssetState>(args.tOldValue));
        const bool bIsActive = IsActive(ra::itoe<AssetState>(args.tNewValue));
        if (bWasActive != bIsActive)
        {
            auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();
            const bool bRuntimeActive = (pRuntime.GetAchievementTrigger(GetID()) != nullptr);
            if (bRuntimeActive != bIsActive)
            {
                if (bIsActive)
                    pRuntime.ActivateAchievement(GetID(), GetTrigger());
                else
                    pRuntime.DeactivateAchievement(GetID());
            }
        }

        if (ra::itoe<AssetState>(args.tNewValue) == AssetState::Triggered)
        {
            const auto& pClock = ra::services::ServiceLocator::Get<ra::services::IClock>();
            m_tUnlock = pClock.Now();
        }
    }

    AssetModelBase::OnValueChanged(args);
}

void AchievementModel::DoFrame()
{
    const auto& pRuntime = ra::services::ServiceLocator::Get<ra::services::AchievementRuntime>();
    const auto* pTrigger = pRuntime.GetAchievementTrigger(GetID());
    if (pTrigger == nullptr)
    {
        if (GetState() != AssetState::Triggered)
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
        }
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
