#include "CppUnitTest.h"

#include "data\models\AssetModelBase.hh"

#include "services\impl\StringTextWriter.hh"

#include "tests\data\DataAsserts.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

TEST_CLASS(AssetModelBase_Tests)
{
private:
    class AssetModelHarness : public AssetModelBase
    {
    public:
        AssetModelHarness() noexcept
        {
            SetTransactional(StringProperty);
            SetTransactional(IntProperty);
        }

        ra::services::impl::StringTextWriter textWriter;

        static const StringModelProperty StringProperty;
        const std::wstring& GetString() const { return GetValue(StringProperty); }
        void SetString(const std::wstring& sValue) { SetValue(StringProperty, sValue); }

        static const IntModelProperty IntProperty;
        uint32_t GetInt() const { return ra::to_unsigned(GetValue(IntProperty)); }
        void SetInt(uint32_t nValue) { SetValue(IntProperty, ra::to_signed(nValue)); }

        std::string Utf8String;
        std::string Utf8QuotedString;

        void Serialize(ra::services::TextWriter& pWriter) const override
        {
            WriteQuoted(pWriter, GetName());
            WritePossiblyQuoted(pWriter, GetString());
            WriteNumber(pWriter, GetInt());
            WriteQuoted(pWriter, Utf8QuotedString);
            WritePossiblyQuoted(pWriter, Utf8String);
        }

        bool Deserialize(ra::Tokenizer& pTokenizer) override
        {
            std::wstring sName;
            if (!ReadQuoted(pTokenizer, sName))
                return false;
            SetName(sName);

            std::wstring sString;
            if (!ReadPossiblyQuoted(pTokenizer, sString))
                return false;
            SetString(sString);

            uint32_t nNumber;
            if (!ReadNumber(pTokenizer, nNumber))
                return false;
            SetInt(nNumber);

            if (!ReadQuoted(pTokenizer, Utf8QuotedString))
                return false;

            if (!ReadPossiblyQuoted(pTokenizer, Utf8String))
                return false;

            return true;
        }

        void MockValidationError(const std::wstring& sError)
        {
            m_sValidationError = sError;
        }

    protected:
        bool ValidateAsset(std::wstring& sError) override
        {
            if (m_sValidationError.empty())
                return true;

            sError = m_sValidationError;
            return false;
        }

    private:
        std::wstring m_sValidationError;
    };

    class AssetDefinitionViewModelHarness : public AssetModelBase
    {
    public:
        AssetDefinitionViewModelHarness() noexcept
        {
            GSL_SUPPRESS_F6 AddAssetDefinition(m_pDefinition, DefinitionProperty);
        }

        const std::string& GetDefinition() const { return GetAssetDefinition(m_pDefinition); }
        void SetDefinition(const std::string& sValue) { SetAssetDefinition(m_pDefinition, sValue); }

        void Serialize(ra::services::TextWriter&) const noexcept override {}
        bool Deserialize(ra::Tokenizer&) noexcept override { return false; }

    protected:
        static const IntModelProperty DefinitionProperty;
        AssetDefinition m_pDefinition;
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        AssetModelHarness asset;

        Assert::AreEqual(AssetType::Achievement, asset.GetType());
        Assert::AreEqual(0U, asset.GetID());
        Assert::AreEqual(std::wstring(L""), asset.GetName());
        Assert::AreEqual(std::wstring(L""), asset.GetDescription());
        Assert::AreEqual(AssetCategory::Core, asset.GetCategory());
        Assert::AreEqual(AssetState::Inactive, asset.GetState());
        Assert::AreEqual(AssetChanges::None, asset.GetChanges());
    }

    TEST_METHOD(TestIsActive)
    {
        AssetModelHarness asset;

        asset.SetState(AssetState::Active);
        Assert::IsTrue(asset.IsActive());

        asset.SetState(AssetState::Inactive);
        Assert::IsFalse(asset.IsActive());

        asset.SetState(AssetState::Paused);
        Assert::IsTrue(asset.IsActive());

        asset.SetState(AssetState::Triggered);
        Assert::IsFalse(asset.IsActive());

        asset.SetState(AssetState::Waiting);
        Assert::IsTrue(asset.IsActive());

        asset.SetState(AssetState::Primed);
        Assert::IsTrue(asset.IsActive());
    }

    TEST_METHOD(TestSerializeDefaults)
    {
        AssetModelHarness asset;

        asset.Serialize(asset.textWriter);
        Assert::AreEqual(std::string(":\"\"::0:\"\":"), asset.textWriter.GetString());
    }

    TEST_METHOD(TestSerializeSimpleValues)
    {
        AssetModelHarness asset;

        asset.SetName(L"Name");
        asset.SetString(L"String");
        asset.SetInt(99U);
        asset.Utf8QuotedString = "UTF-8-Quoted";
        asset.Utf8String = "UTF-8";

        asset.Serialize(asset.textWriter);
        Assert::AreEqual(std::string(":\"Name\":String:99:\"UTF-8-Quoted\":UTF-8"), asset.textWriter.GetString());
    }

    TEST_METHOD(TestSerializeColon)
    {
        AssetModelHarness asset;

        asset.SetName(L"Mission: Impossible");
        asset.SetString(L"Mission: Impossible");
        asset.Utf8QuotedString = "Mission: Impossible";
        asset.Utf8String = "Mission: Impossible";

        asset.Serialize(asset.textWriter);
        Assert::AreEqual(std::string(":\"Mission: Impossible\":\"Mission: Impossible\":0:\"Mission: Impossible\":\"Mission: Impossible\""), asset.textWriter.GetString());
    }

    TEST_METHOD(TestSerializeQuotes)
    {
        AssetModelHarness asset;

        asset.SetName(L"A \"Test\" B");
        asset.SetString(L"A \"Test\" B");
        asset.Utf8QuotedString = "A \"Test\" B";
        asset.Utf8String = "A \"Test\" B";

        asset.Serialize(asset.textWriter);
        Assert::AreEqual(std::string(":\"A \\\"Test\\\" B\":\"A \\\"Test\\\" B\":0:\"A \\\"Test\\\" B\":\"A \\\"Test\\\" B\""), asset.textWriter.GetString());
    }

    TEST_METHOD(TestSerializeBackslash)
    {
        AssetModelHarness asset;

        asset.SetName(L"A\\B");
        asset.SetString(L"A\\B");
        asset.Utf8QuotedString = "A\\B";
        asset.Utf8String = "A\\B";

        asset.Serialize(asset.textWriter);
        Assert::AreEqual(std::string(":\"A\\\\B\":\"A\\\\B\":0:\"A\\\\B\":\"A\\\\B\""), asset.textWriter.GetString());
    }

    TEST_METHOD(TestDeserializeSimpleValues)
    {
        AssetModelHarness asset;

        std::string sSerialized = ":\"Name\":String:99:\"UTF-8-Quoted\":UTF-8";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Advance(); // skip leading colon
        Assert::IsTrue(asset.Deserialize(pTokenizer));

        Assert::AreEqual(std::wstring(L"Name"), asset.GetName());
        Assert::AreEqual(std::wstring(L"String"), asset.GetString());
        Assert::AreEqual(99U, asset.GetInt());
        Assert::AreEqual(std::string("UTF-8-Quoted"), asset.Utf8QuotedString);
        Assert::AreEqual(std::string("UTF-8"), asset.Utf8String);
    }

    TEST_METHOD(TestDeserializeColon)
    {
        AssetModelHarness asset;

        std::string sSerialized = ":\"Mission: Impossible\":\"Mission: Impossible\":0:\"Mission: Impossible\":\"Mission: Impossible\"";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Advance(); // skip leading colon
        Assert::IsTrue(asset.Deserialize(pTokenizer));

        Assert::AreEqual(std::wstring(L"Mission: Impossible"), asset.GetName());
        Assert::AreEqual(std::wstring(L"Mission: Impossible"), asset.GetString());
        Assert::AreEqual(0U, asset.GetInt());
        Assert::AreEqual(std::string("Mission: Impossible"), asset.Utf8QuotedString);
        Assert::AreEqual(std::string("Mission: Impossible"), asset.Utf8String);
    }

    TEST_METHOD(TestDeserializeQuotes)
    {
        AssetModelHarness asset;

        std::string sSerialized = ":\"A \\\"Test\\\" B\":\"A \\\"Test\\\" B\":0:\"A \\\"Test\\\" B\":\"A \\\"Test\\\" B\"";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Advance(); // skip leading colon
        Assert::IsTrue(asset.Deserialize(pTokenizer));

        Assert::AreEqual(std::wstring(L"A \"Test\" B"), asset.GetName());
        Assert::AreEqual(std::wstring(L"A \"Test\" B"), asset.GetString());
        Assert::AreEqual(0U, asset.GetInt());
        Assert::AreEqual(std::string("A \"Test\" B"), asset.Utf8QuotedString);
        Assert::AreEqual(std::string("A \"Test\" B"), asset.Utf8String);
    }

    TEST_METHOD(TestDeserializeBackslash)
    {
        AssetModelHarness asset;

        std::string sSerialized = ":\"A\\\\B\":\"A\\\\B\":0:\"A\\\\B\":\"A\\\\B\"";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Advance(); // skip leading colon
        Assert::IsTrue(asset.Deserialize(pTokenizer));

        Assert::AreEqual(std::wstring(L"A\\B"), asset.GetName());
        Assert::AreEqual(std::wstring(L"A\\B"), asset.GetString());
        Assert::AreEqual(0U, asset.GetInt());
        Assert::AreEqual(std::string("A\\B"), asset.Utf8QuotedString);
        Assert::AreEqual(std::string("A\\B"), asset.Utf8String);
    }

    TEST_METHOD(TestDeserializeInvalidNumber)
    {
        AssetModelHarness asset;

        std::string sSerialized = ":\"A\":B:C:\"D\":E";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Advance(); // skip leading colon
        Assert::IsFalse(asset.Deserialize(pTokenizer));

        Assert::AreEqual(std::wstring(L"A"), asset.GetName());
        Assert::AreEqual(std::wstring(L"B"), asset.GetString());
        Assert::AreEqual(0U, asset.GetInt());
        Assert::AreEqual(std::string(), asset.Utf8QuotedString);
        Assert::AreEqual(std::string(), asset.Utf8String);
    }

    TEST_METHOD(TestDeserializeMissingField)
    {
        AssetModelHarness asset;

        std::string sSerialized = ":\"A\":B:5:\"D\"";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Advance(); // skip leading colon
        Assert::IsFalse(asset.Deserialize(pTokenizer));

        Assert::AreEqual(std::wstring(L"A"), asset.GetName());
        Assert::AreEqual(std::wstring(L"B"), asset.GetString());
        Assert::AreEqual(5U, asset.GetInt());
        Assert::AreEqual(std::string("D"), asset.Utf8QuotedString);
        Assert::AreEqual(std::string(), asset.Utf8String);
    }

    TEST_METHOD(TestDeserializeNotQuoted)
    {
        AssetModelHarness asset;

        std::string sSerialized = ":A:B:5:D:E";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Advance(); // skip leading colon
        Assert::IsFalse(asset.Deserialize(pTokenizer));

        Assert::AreEqual(std::wstring(), asset.GetName());
        Assert::AreEqual(std::wstring(), asset.GetString());
        Assert::AreEqual(0U, asset.GetInt());
        Assert::AreEqual(std::string(), asset.Utf8QuotedString);
        Assert::AreEqual(std::string(), asset.Utf8String);
    }

    TEST_METHOD(TestDeserializeBadlyQuoted)
    {
        AssetModelHarness asset;

        std::string sSerialized = ":\"A\":\"B\"C:5:\"D\":E";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Advance(); // skip leading colon
        Assert::IsFalse(asset.Deserialize(pTokenizer));

        Assert::AreEqual(std::wstring(L"A"), asset.GetName());
        Assert::AreEqual(std::wstring(), asset.GetString());
        Assert::AreEqual(0U, asset.GetInt());
        Assert::AreEqual(std::string(), asset.Utf8QuotedString);
        Assert::AreEqual(std::string(), asset.Utf8String);
    }

    TEST_METHOD(TestUpdateCheckpoint)
    {
        AssetModelHarness asset;
        asset.SetName(L"ServerName");
        asset.SetCategory(AssetCategory::Core);
        asset.CreateServerCheckpoint();
        asset.CreateLocalCheckpoint();

        Assert::AreEqual(AssetChanges::None, asset.GetChanges());

        asset.SetName(L"LocalName");
        Assert::AreEqual(AssetChanges::Modified, asset.GetChanges());

        asset.UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, asset.GetChanges());
        Assert::AreEqual(std::wstring(L"LocalName"), asset.GetName());

        asset.UpdateServerCheckpoint();
        Assert::AreEqual(AssetChanges::None, asset.GetChanges());
        Assert::AreEqual(std::wstring(L"LocalName"), asset.GetName());
    }

    TEST_METHOD(TestRestoreCheckpoint)
    {
        AssetModelHarness asset;
        asset.SetName(L"ServerName");
        asset.SetCategory(AssetCategory::Core);
        asset.CreateServerCheckpoint();
        asset.SetName(L"LocalName");
        asset.CreateLocalCheckpoint();

        Assert::AreEqual(AssetChanges::Unpublished, asset.GetChanges());

        asset.SetName(L"ModifiedName");
        Assert::AreEqual(AssetChanges::Modified, asset.GetChanges());

        asset.RestoreLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, asset.GetChanges());
        Assert::AreEqual(std::wstring(L"LocalName"), asset.GetName());

        asset.SetName(L"ModifiedName");
        Assert::AreEqual(AssetChanges::Modified, asset.GetChanges());

        asset.RestoreServerCheckpoint();
        Assert::AreEqual(AssetChanges::None, asset.GetChanges());
        Assert::AreEqual(std::wstring(L"ServerName"), asset.GetName());
    }

    TEST_METHOD(TestUpdateDefinitionCheckpoint)
    {
        AssetDefinitionViewModelHarness asset;
        asset.SetDefinition("ServerDefinition");
        asset.CreateServerCheckpoint();
        asset.CreateLocalCheckpoint();

        Assert::AreEqual(AssetChanges::None, asset.GetChanges());
        Assert::AreEqual(std::string("ServerDefinition"), asset.GetDefinition());

        asset.SetDefinition("LocalDefinition");
        Assert::AreEqual(AssetChanges::Modified, asset.GetChanges());
        Assert::AreEqual(std::string("LocalDefinition"), asset.GetDefinition());

        asset.UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, asset.GetChanges());
        Assert::AreEqual(std::string("LocalDefinition"), asset.GetDefinition());

        asset.UpdateServerCheckpoint();
        Assert::AreEqual(AssetChanges::None, asset.GetChanges());
        Assert::AreEqual(std::string("LocalDefinition"), asset.GetDefinition());
    }

    TEST_METHOD(TestRestoreDefinitionCheckpoint)
    {
        AssetDefinitionViewModelHarness asset;
        asset.SetDefinition("ServerDefinition");
        asset.CreateServerCheckpoint();
        asset.SetDefinition("LocalDefinition");
        asset.CreateLocalCheckpoint();

        Assert::AreEqual(AssetChanges::Unpublished, asset.GetChanges());
        Assert::AreEqual(std::string("LocalDefinition"), asset.GetDefinition());

        asset.SetDefinition("ModifiedDefinition");
        Assert::AreEqual(AssetChanges::Modified, asset.GetChanges());
        Assert::AreEqual(std::string("ModifiedDefinition"), asset.GetDefinition());

        asset.RestoreLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, asset.GetChanges());
        Assert::AreEqual(std::string("LocalDefinition"), asset.GetDefinition());

        asset.SetDefinition("ModifiedDefinition");
        Assert::AreEqual(AssetChanges::Modified, asset.GetChanges());
        Assert::AreEqual(std::string("ModifiedDefinition"), asset.GetDefinition());

        asset.RestoreServerCheckpoint();
        Assert::AreEqual(AssetChanges::None, asset.GetChanges());
        Assert::AreEqual(std::string("ServerDefinition"), asset.GetDefinition());
    }

    TEST_METHOD(TestRestoreDefinitionCheckpointEmpty)
    {
        AssetDefinitionViewModelHarness asset;
        asset.SetDefinition("ServerDefinition");
        asset.CreateServerCheckpoint();
        asset.CreateLocalCheckpoint(); // no local value

        Assert::AreEqual(AssetChanges::None, asset.GetChanges());
        Assert::AreEqual(std::string("ServerDefinition"), asset.GetDefinition());

        asset.SetDefinition("");
        Assert::AreEqual(AssetChanges::Modified, asset.GetChanges());
        Assert::AreEqual(std::string(""), asset.GetDefinition());

        asset.UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, asset.GetChanges());
        Assert::AreEqual(std::string(""), asset.GetDefinition());

        asset.RestoreLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, asset.GetChanges());
        Assert::AreEqual(std::string(""), asset.GetDefinition());
    }

    TEST_METHOD(TestResetLocalCheckpoint)
    {
        AssetModelHarness asset;
        asset.SetName(L"ServerName");
        asset.CreateServerCheckpoint();
        asset.SetName(L"LocalName");
        asset.CreateLocalCheckpoint();
        asset.SetName(L"MemoryName");

        std::string sSerialized = ":\"Name\":String:99:\"UTF-8-Quoted\":UTF-8";
        ra::Tokenizer pTokenizer(sSerialized);
        pTokenizer.Advance(); // skip leading colon
        asset.ResetLocalCheckpoint(pTokenizer);

        Assert::AreEqual(std::wstring(L"Name"), asset.GetName());
        Assert::AreEqual(AssetChanges::Unpublished, asset.GetChanges());
    }

    TEST_METHOD(TestSetDefinitionToBlank)
    {
        AssetDefinitionViewModelHarness asset;
        asset.SetName(L"ServerName");
        asset.SetCategory(AssetCategory::Core);
        asset.SetDefinition("ServerDefinition");
        asset.CreateServerCheckpoint();
        asset.CreateLocalCheckpoint();

        Assert::AreEqual(AssetChanges::None, asset.GetChanges());

        asset.SetDefinition("");
        Assert::AreEqual(AssetChanges::Modified, asset.GetChanges());

        asset.SetDefinition("ServerDefinition");
        Assert::AreEqual(AssetChanges::None, asset.GetChanges());

        asset.SetDefinition("LocalDefinition");
        Assert::AreEqual(AssetChanges::Modified, asset.GetChanges());

        asset.UpdateLocalCheckpoint();
        Assert::AreEqual(AssetChanges::Unpublished, asset.GetChanges());

        asset.SetDefinition("");
        Assert::AreEqual(AssetChanges::Modified, asset.GetChanges());

        asset.SetDefinition("LocalDefinition");
        Assert::AreEqual(AssetChanges::Unpublished, asset.GetChanges());
    }

    TEST_METHOD(TestValidateExplicit)
    {
        AssetModelHarness asset;
        Assert::AreEqual(std::wstring(), asset.GetValidationError());

        Assert::IsTrue(asset.Validate());
        Assert::AreEqual(std::wstring(), asset.GetValidationError());

        asset.MockValidationError(L"It's bad.");
        Assert::IsFalse(asset.Validate());
        Assert::AreEqual(std::wstring(L"It's bad."), asset.GetValidationError());

        asset.MockValidationError(L"");
        Assert::IsTrue(asset.Validate());
        Assert::AreEqual(std::wstring(), asset.GetValidationError());
    }

    TEST_METHOD(TestValidateCoreError)
    {
        AssetModelHarness asset;
        asset.SetName(L"ServerName");
        asset.SetCategory(AssetCategory::Core);
        asset.MockValidationError(L"Core Error");

        // creating server checkpoint does not validate
        asset.CreateServerCheckpoint();
        Assert::AreEqual(std::wstring(), asset.GetValidationError());

        // creating local checkpoint does validate
        asset.CreateLocalCheckpoint();
        Assert::AreEqual(std::wstring(L"Core Error"), asset.GetValidationError());
    }

    TEST_METHOD(TestValidateLocalError)
    {
        AssetModelHarness asset;
        asset.SetName(L"ServerName");
        asset.SetCategory(AssetCategory::Core);
        asset.CreateServerCheckpoint();
        Assert::AreEqual(std::wstring(), asset.GetValidationError());

        // creating local checkpoint does validate
        asset.MockValidationError(L"Local Error");
        asset.CreateLocalCheckpoint();
        Assert::AreEqual(std::wstring(L"Local Error"), asset.GetValidationError());
    }

    TEST_METHOD(TestValidateModifiedError)
    {
        AssetModelHarness asset;
        asset.SetName(L"ServerName");
        asset.SetCategory(AssetCategory::Core);
        asset.CreateServerCheckpoint();
        asset.CreateLocalCheckpoint();
        Assert::AreEqual(std::wstring(), asset.GetValidationError());

        // applying the modification does not validate
        asset.MockValidationError(L"Modified Error");
        Assert::AreEqual(std::wstring(), asset.GetValidationError());

        // committing the change does validate
        asset.UpdateLocalCheckpoint();
        Assert::AreEqual(std::wstring(L"Modified Error"), asset.GetValidationError());

        // reverting changes does validate
        asset.MockValidationError(L"Restored Error");
        asset.RestoreLocalCheckpoint();
        Assert::AreEqual(std::wstring(L"Restored Error"), asset.GetValidationError());
    }

    TEST_METHOD(TestValidateDeletedError)
    {
        AssetModelHarness asset;
        asset.SetName(L"ServerName");
        asset.SetCategory(AssetCategory::Core);
        asset.MockValidationError(L"Local Error");
        asset.CreateServerCheckpoint();
        asset.CreateLocalCheckpoint();
        Assert::AreEqual(std::wstring(L"Local Error"), asset.GetValidationError());

        // deleting clears the validation error
        asset.SetDeleted();
        Assert::AreEqual(std::wstring(L""), asset.GetValidationError());
    }
};

const StringModelProperty AssetModelBase_Tests::AssetModelHarness::StringProperty("AssetModelHarness", "String", L"");
const IntModelProperty AssetModelBase_Tests::AssetModelHarness::IntProperty("AssetModelHarness", "Int", 0);
const IntModelProperty AssetModelBase_Tests::AssetDefinitionViewModelHarness::DefinitionProperty("AssetDefinitionViewModelHarness", "Int", ra::etoi(AssetChanges::None));

} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
