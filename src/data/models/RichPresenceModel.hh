#ifndef RA_DATA_RICHPRESENCE_MODEL_H
#define RA_DATA_RICHPRESENCE_MODEL_H
#pragma once

#include "data/models/AssetModelBase.hh"

#include "data\Types.hh"

namespace ra {
namespace data {
namespace models {

class RichPresenceModel : public AssetModelBase
{
public:
    RichPresenceModel() noexcept;

    /// <summary>
    /// The <see cref="ModelProperty" /> for the rich presence script.
    /// </summary>
    static const IntModelProperty ScriptProperty;

    /// <summary>
    /// Gets the rich presence script.
    /// </summary>
    const std::string& GetScript() const { return GetAssetDefinition(m_pScript); }

    /// <summary>
    /// Sets the rich presence script.
    /// </summary>
    void SetScript(const std::string& sScript);

    void ReloadRichPresenceScript();

    void Activate() override;
    void Deactivate() override;

    void Serialize(ra::services::TextWriter& pWriter) const noexcept override;
    bool Deserialize(ra::util::Tokenizer& pTokenizer) noexcept override;

    /// <summary>
    /// Gets the maximum size of the rich presence script.
    /// </summary>
    /// <remarks>
    /// This is the absolute maximum number of bytes (in UTF-8).
    /// There's a soft limit of 60000 characters due to the way the webpage validates length.
    /// </remarks>
    static constexpr size_t MaxScriptLength = 65535;

protected:
    void OnValueChanged(const IntModelProperty::ChangeArgs& args) override;

    bool ValidateAsset(std::wstring& sError) override;

private:
    void WriteRichPresenceScript();

    AssetDefinition m_pScript;
};

} // namespace models
} // namespace data
} // namespace ra

#endif RA_DATA_RICHPRESENCE_MODEL_H
