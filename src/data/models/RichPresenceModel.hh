#ifndef RA_DATA_RICHPRESENCE_MODEL_H
#define RA_DATA_RICHPRESENCE_MODEL_H
#pragma once

#include "AssetModelBase.hh"

#include "CapturedTriggerHits.hh"

#include "data\Types.hh"

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

    void ReloadRichPresenceScript();

    void Activate() override;
    void Deactivate() override;

    void Serialize(ra::services::TextWriter& pWriter) const noexcept override;
    bool Deserialize(ra::Tokenizer& pTokenizer) noexcept override;

protected:
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

private:
    void WriteRichPresenceScript();

    AssetDefinition m_pScript;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_RICHPRESENCE_MODEL_H
