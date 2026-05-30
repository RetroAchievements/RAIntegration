#include "RichPresenceModel.hh"

#include "context/IGameContext.hh"
#include "context/IRcClient.hh"

#include "services/ILocalStorage.hh"
#include "services/ServiceLocator.hh"

#include "util/Strings.hh"

#include <rcheevos/src/rcheevos/rc_internal.h>
#include <rcheevos/src/rc_client_internal.h>

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
        if (args.Property == ScriptProperty)
        {
            if (GetState() == AssetState::Disabled)
            {
                // State is Disabled, Script changed. Try to reactivate
                SetState(AssetState::Active);
            }
            else
            {
                SyncScriptToRuntime();

                if (!m_sParseError.empty())
                    SetState(AssetState::Disabled);
            }
        }
        else if (args.Property == StateProperty)
        {
            SyncScriptToRuntime();

            if (!m_sParseError.empty() && ra::itoe<AssetState>(args.tNewValue) == AssetState::Active)
            {
                SetState(AssetState::Disabled);

                // if this was triggered as a result of the State changing to Active, and we changed the State
                // to Disabled, swallow the event for the State changing to Active as it's no longer accurate.
                return;
            }
        }
        else if (args.Property == ChangesProperty)
        {
            const auto nNewValue = ra::itoe<AssetChanges>(args.tNewValue);
            if (nNewValue == AssetChanges::None)
            {
                // asset was reverted - flush the original value back to disk so the external
                // editor sees the reversion.
                WriteRichPresenceScript();
            }
        }
    }

    AssetModelBase::OnValueChanged(args);
}

bool RichPresenceModel::ValidateAsset(std::wstring& sError)
{
    if (!m_sParseError.empty())
    {
        sError = m_sParseError;
        return false;
    }

    const auto nScriptLength = gsl::narrow_cast<uint32_t>(GetScript().length());
    if (nScriptLength > MaxScriptLength)
    {
        sError = ra::util::String::Printf(L"Script exceeds maximum length (%u/%u)", nScriptLength, MaxScriptLength);
        return false;
    }

    // TODO: validate logic

    return true;
}

void RichPresenceModel::InitializeFromPublishedScript(const rc_runtime_richpresence_t* pDefinition, const std::string& sScript)
{
    if (pDefinition)
    {
        // attaching runtime
        m_pPublishedRichPresenceInfo = pDefinition;
    }
    else
    {
        // preloading server script definition
        BeginUpdate(); // prevents call to SyncScriptToRuntime, which may re-parse the script if it gets normalized
        SetScript(sScript);
        EndUpdate();
    }
}

void RichPresenceModel::SetScript(const std::string& sScript)
{
    bool bIsOnlyWhitespace = true;
    for (const char c : sScript)
    {
        if (!ra::util::String::IsSpace(c))
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
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::context::IGameContext>();
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();
    auto pRich = pLocalStorage.ReadText(ra::services::StorageItemType::RichPresence, std::to_wstring(pGameContext.ActiveGameId()));

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
    if (ra::util::String::StartsWith(sRichPresence, "\xef\xbb\xbf"))
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

    if (GetChanges() != ra::data::models::AssetChanges::None && IsActive() &&
        ra::services::ServiceLocator::Get<ra::context::IRcClient>().IsHardcodeEnabled())
    {
        // ignore modified rich presence in hardcore
        Deactivate();
    }

    EndUpdate();
}

void RichPresenceModel::WriteRichPresenceScript()
{
    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::context::IGameContext>();
    auto& pLocalStorage = ra::services::ServiceLocator::GetMutable<ra::services::ILocalStorage>();

    auto pWriter = pLocalStorage.WriteText(ra::services::StorageItemType::RichPresence,
                                           std::to_wstring(pGameContext.ActiveGameId()));
    const auto& pScript = GetScript();
    pWriter->Write(pScript);
}

void RichPresenceModel::Serialize(ra::services::TextWriter&) const noexcept
{
}

bool RichPresenceModel::Deserialize(ra::util::Tokenizer&) noexcept
{
    return false;
}

void RichPresenceModel::SyncScriptToRuntime()
{
    ParseScript();
}

void RichPresenceModel::DetachRuntimeLeaderboard(struct rc_client_game_info_t* pGame) noexcept
{
    if (pGame)
        pGame->runtime.richpresence = nullptr;

    if (m_pRichPresenceInfo)
    {
        free(m_pRichPresenceInfo);
        m_pRichPresenceInfo = nullptr;
    }

    m_pDefinitionBuffer.reset();
}

void RichPresenceModel::ParseScript()
{
    auto* pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>().GetClient();
    auto* pGame = pClient->game;

    const auto& sScript = GetScript();
    uint8_t md5[16];
    rc_runtime_checksum(sScript.c_str(), md5);

    rc_mutex_lock(&pClient->state.mutex);

    m_sParseError.clear();

    if (!pGame || sScript.empty())
    {
        // empty script or no game loaded - clear out previous one
        DetachRuntimeLeaderboard(pGame);

        rc_mutex_unlock(&pClient->state.mutex);
        return;
    }

    if (m_pRichPresenceInfo && memcmp(m_pRichPresenceInfo->md5, md5, sizeof(md5)) == 0)
    {
        // trigger is already up-to-date. attach if active. otherwise, do nothing.
        pGame->runtime.richpresence = IsActive() ? m_pRichPresenceInfo : nullptr;

        rc_mutex_unlock(&pClient->state.mutex);
        return;
    }

    if (m_pPublishedRichPresenceInfo && memcmp(m_pPublishedRichPresenceInfo->md5, md5, sizeof(md5)) == 0)
    {
        // switched back to published rich presence
        DetachRuntimeLeaderboard(pGame);

        if (IsActive()) // attach if active
        {
            GSL_SUPPRESS_TYPE3 pGame->runtime.richpresence = const_cast<rc_runtime_richpresence_t*>(m_pPublishedRichPresenceInfo);
            rc_reset_richpresence(pGame->runtime.richpresence->richpresence);
        }

        rc_mutex_unlock(&pClient->state.mutex);
        return;
    }

    // attempt to parse the script
    rc_preparse_state_t preparse;
    rc_init_preparse_state(&preparse);
    preparse.parse.existing_memrefs = pGame->runtime.memrefs;

    rc_richpresence_with_memrefs_t* richpresence = RC_ALLOC(rc_richpresence_with_memrefs_t, &preparse.parse);
    preparse.parse.variables = &richpresence->richpresence.values;
    rc_parse_richpresence_internal(&richpresence->richpresence, sScript.c_str(), &preparse.parse);
    rc_preparse_alloc_memrefs(nullptr, &preparse);

    const auto nSize = preparse.parse.offset;
    if (nSize < 0)
    {
        // parse error - disable rich presence
        DetachRuntimeLeaderboard(pGame);

        // set error
        m_sParseError = ra::util::String::Printf(L"Parse error %d (line %u): %s",
            nSize, preparse.parse.lines_read, rc_error_str(nSize));
        SetValue(ValidationErrorProperty, m_sParseError);
    }
    else if (!IsActive())
    {
        // not active. don't actually load into runtime
        DetachRuntimeLeaderboard(pGame);
    }
    else
    {
        auto definition_buffer = std::make_unique<uint8_t[]>(nSize);
        if (definition_buffer)
        {
            // populate the item, using the communal memrefs pool
            rc_reset_parse_state(&preparse.parse, definition_buffer.get());
            preparse.parse.existing_memrefs = pGame->runtime.memrefs;

            richpresence = RC_ALLOC(rc_richpresence_with_memrefs_t, &preparse.parse);
            rc_preparse_alloc_memrefs(&richpresence->memrefs, &preparse);
            Expects(preparse.parse.memrefs == &richpresence->memrefs);

            preparse.parse.existing_memrefs = pGame->runtime.memrefs;

            preparse.parse.variables = &richpresence->richpresence.values;
            rc_parse_richpresence_internal(&richpresence->richpresence, sScript.c_str(), &preparse.parse);
            richpresence->richpresence.has_memrefs = 1;

            // alloc here so runtime can free
            if (!m_pRichPresenceInfo)
                m_pRichPresenceInfo = static_cast<rc_runtime_richpresence_t*>(calloc(1, sizeof(rc_runtime_richpresence_t)));

            // switch to the new rich presence
            if (m_pRichPresenceInfo != nullptr)
            {
                m_pRichPresenceInfo->richpresence = &richpresence->richpresence;
                memcpy(m_pRichPresenceInfo->md5, md5, sizeof(md5));
            }

            pGame->runtime.richpresence = m_pRichPresenceInfo;

            m_pDefinitionBuffer = std::move(definition_buffer);
        }
    }

    rc_mutex_unlock(&pClient->state.mutex);

    rc_destroy_preparse_state(&preparse);
}

const struct rc_richpresence_t* RichPresenceModel::GetRuntimeDefinition() const
{
    if (m_pRichPresenceInfo != nullptr)
        return m_pRichPresenceInfo->richpresence;

    if (m_pPublishedRichPresenceInfo != nullptr && GetChanges() == AssetChanges::None)
        return m_pPublishedRichPresenceInfo->richpresence;

    return nullptr;
}

struct rc_richpresence_t* RichPresenceModel::GetMutableRuntimeDefinition()
{
    if (m_pRichPresenceInfo != nullptr)
        return m_pRichPresenceInfo->richpresence;

    if (m_pPublishedRichPresenceInfo != nullptr && GetChanges() == AssetChanges::None)
        return m_pPublishedRichPresenceInfo->richpresence;

    return nullptr;
}

std::wstring RichPresenceModel::GetDisplayMessage() const
{
    if (!m_sParseError.empty())
        return m_sParseError;

    auto* pClient = ra::services::ServiceLocator::Get<ra::context::IRcClient>().GetClient();
    if (!rc_client_has_rich_presence(pClient))
        return L"No Rich Presence defined.";

    char sRichPresence[256];
    const auto nResult = gsl::narrow_cast<int>(rc_client_get_rich_presence_message(pClient, sRichPresence, sizeof(sRichPresence)));
    if (nResult > 0)
        return ra::util::String::Widen(sRichPresence);

    return ra::util::String::Widen(rc_error_str(nResult));
}

} // namespace models
} // namespace data
} // namespace ra
