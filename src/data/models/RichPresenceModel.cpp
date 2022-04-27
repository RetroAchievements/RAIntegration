#include "RichPresenceModel.hh"

#include "data\context\GameContext.hh"

#include "services\AchievementRuntime.hh"
#include "services\ILocalStorage.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace data {
namespace models {

const IntModelProperty RichPresenceModel::ScriptProperty("RichPresenceModel", "Script", 0);

RichPresenceModel::RichPresenceModel() noexcept
{
    GSL_SUPPRESS_F6 SetValue(TypeProperty, ra::etoi(AssetType::RichPresence));
    GSL_SUPPRESS_F6 SetValue(NameProperty, L"Rich Presence");

    GSL_SUPPRESS_F6 AddAssetDefinition(m_pScript, ScriptProperty);
}

void RichPresenceModel::Activate()
{
    SetState(AssetState::Active);
}

void RichPresenceModel::Deactivate()
{
    SetState(AssetState::Inactive);
}

void RichPresenceModel::OnValueChanged(const IntModelProperty::ChangeArgs& args)
{
    // if we're still loading the asset, ignore the event. we'll get recalled once loading finishes
    if (!IsUpdating())
    {
        if (args.Property == StateProperty || args.Property == ScriptProperty)
        {
            auto& pRuntime = ra::services::ServiceLocator::GetMutable<ra::services::AchievementRuntime>();

            if (IsActive())
            {
                const std::string sScript = GetScript();
                if (!pRuntime.ActivateRichPresence(sScript))
                {
                    SetState(AssetState::Disabled);

                    // if this was triggered as a result of the State changing to Active, and we changed the State
                    // to Disabled, swallow the event for the State changing to Active as it's no longer accurate.
                    if (args.Property == StateProperty)
                        return;
                }
            }
            else if (GetState() != AssetState::Disabled)
            {
                pRuntime.ActivateRichPresence("");
            }
            else if (args.Property == ScriptProperty)
            {
                // State is Disabled, Script changed. Try to reactivate
                SetState(AssetState::Active);
            }
        }
        else if (args.Property == ChangesProperty && args.tNewValue == 0)
        {
            WriteRichPresenceScript();
        }
    }

    AssetModelBase::OnValueChanged(args);
}

void RichPresenceModel::SetScript(const std::string& sScript)
{
    bool bIsOnlyWhitespace = true;
    for (const char c : sScript)
    {
        if (!isspace(static_cast<int>(c)))
        {
            bIsOnlyWhitespace = false;
            break;
        }
    }

    if (bIsOnlyWhitespace)
    {
        SetAssetDefinition(m_pScript, "");
        return;
    }

    if (sScript.back() == '\n' && sScript.find('\r') == std::string::npos)
    {
        SetAssetDefinition(m_pScript, sScript);
        return;
    }

    // normalize (unix line endings, must end with a line ending)
    std::string sNormalizedScript = sScript;
    sNormalizedScript.erase(std::remove(sNormalizedScript.begin(), sNormalizedScript.end(), '\r'), sNormalizedScript.end());
    if (!sNormalizedScript.empty() && sNormalizedScript.back() != '\n')
        sNormalizedScript.push_back('\n');

    SetAssetDefinition(m_pScript, sNormalizedScript);
}

void RichPresenceModel::ReloadRichPresenceScript()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pRich = pLocalStorage.ReadText(ra::services::StorageItemType::RichPresence, std::to_wstring(pGameContext.GameId()));

    if (pRich == nullptr)
    {
        // file doesn't exist. reset back to the server state
        RestoreServerCheckpoint();

        // if server state is not empty, flush it to disk
        if (!GetScript().empty())
            WriteRichPresenceScript();

        return;
    }

    std::string sRichPresence;
    std::string sLine;
    while (pRich->GetLine(sLine))
    {
        sRichPresence.append(sLine);
        sRichPresence.append("\n");
    }

    // remove UTF-8 BOM if present
    if (ra::StringStartsWith(sRichPresence, "\xef\xbb\xbf"))
        sRichPresence.erase(0, 3);

    BeginUpdate();

    SetScript(sRichPresence);

    UpdateLocalCheckpoint();

    // if there's a local script, but no core script, change the category to Local
    if (GetChanges() == ra::data::models::AssetChanges::Unpublished && m_pScript.m_sCoreDefinition.empty())
    {
        SetCategory(ra::data::models::AssetCategory::Local);
        UpdateLocalCheckpoint();
    }

    EndUpdate();
}

void RichPresenceModel::WriteRichPresenceScript()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::context::GameContext>();
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();

    auto pWriter = pLocalStorage.WriteText(ra::services::StorageItemType::RichPresence,
                                           std::to_wstring(pGameContext.GameId()));
    const auto& pScript = GetScript();
    pWriter->Write(pScript);
}

void RichPresenceModel::Serialize(ra::services::TextWriter&) const noexcept
{
}

bool RichPresenceModel::Deserialize(ra::Tokenizer&) noexcept
{
    return false;
}

} // namespace models
} // namespace data
} // namespace ra
