#ifndef RA_DATA_RICHPRESENCE_MODEL_H
#define RA_DATA_RICHPRESENCE_MODEL_H
#pragma once

#include "data/models/AssetModelBase.hh"

struct rc_client_game_info_t;
struct rc_richpresence_t;
struct rc_runtime_richpresence_t;

namespace ra {
namespace data {
namespace models {

class RichPresenceModel : public AssetModelBase
{
public:
    RichPresenceModel() noexcept;

    /// <summary>
    /// The <see cref="ModelProperty" /> for the start trigger version.
    /// </summary>
    static const IntModelProperty ScriptProperty;

    /// <summary>
    /// Gets the start trigger definition.
    /// </summary>
    const std::string& GetScript() const { return GetAssetDefinition(m_pScript); }

    /// <summary>
    /// Sets the start trigger definition.
    /// </summary>
    void SetScript(const std::string& sScript);

    /// <summary>
    /// Gets the current evaluation of the rich presence script.
    /// </summary>
    std::wstring GetMessage() const;

    void InitializeFromPublishedScript(const rc_runtime_richpresence_t* pDefinition, const std::string& sScript);

    void ReloadRichPresenceScript();

    void Activate() override;
    void Deactivate() override;

    void Serialize(ra::services::TextWriter& pWriter) const noexcept override;
    bool Deserialize(ra::util::Tokenizer& pTokenizer) noexcept override;

    const struct rc_richpresence_t* GetRuntimeDefinition() const;
    struct rc_richpresence_t* GetMutableRuntimeDefinition();

    /// <summary>
    /// Gets the maximum size of the script.
    /// </summary>
    static constexpr size_t MaxScriptLength = 65535;

protected:
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

    bool ValidateAsset(std::wstring& sErrorMessage) override;

private:
    void WriteRichPresenceScript();

    void DetachRuntimeLeaderboard(struct rc_client_game_info_t* pGame);
    void SyncScriptToRuntime();
    void ParseScript();
    AssetDefinition m_pScript;

    // the current achievement information
    struct rc_runtime_richpresence_t* m_pRichPresenceInfo = nullptr;

    // the original achievement information received from the server
    const struct rc_runtime_richpresence_t* m_pPublishedRichPresenceInfo = nullptr;

    std::unique_ptr<uint8_t[]> m_pDefinitionBuffer;
    std::wstring m_sParseError;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_RICHPRESENCE_MODEL_H
