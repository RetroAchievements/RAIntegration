#include "CppUnitTest.h"

#include "ui\viewmodels\AssetViewModelBase.hh"

#include "services\impl\StringTextWriter.hh"

#include "tests\RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft {
namespace VisualStudio {
namespace CppUnitTestFramework {

template<>
std::wstring ToString<ra::ui::viewmodels::AssetType>(
    const ra::ui::viewmodels::AssetType& nAssetType)
{
    switch (nAssetType)
    {
        case ra::ui::viewmodels::AssetType::None:
            return L"None";
        case ra::ui::viewmodels::AssetType::Achievement:
            return L"Achievement";
        case ra::ui::viewmodels::AssetType::Leaderboard:
            return L"Leaderboard";
        default:
            return std::to_wstring(static_cast<int>(nAssetType));
    }
}

template<>
std::wstring ToString<ra::ui::viewmodels::AssetCategory>(
    const ra::ui::viewmodels::AssetCategory& nAssetCategory)
{
    switch (nAssetCategory)
    {
        case ra::ui::viewmodels::AssetCategory::Local:
            return L"Local";
        case ra::ui::viewmodels::AssetCategory::Core:
            return L"Core";
        case ra::ui::viewmodels::AssetCategory::Unofficial:
            return L"Unofficial";
        default:
            return std::to_wstring(static_cast<int>(nAssetCategory));
    }
}

template<>
std::wstring ToString<ra::ui::viewmodels::AssetState>(
    const ra::ui::viewmodels::AssetState& nAssetState)
{
    switch (nAssetState)
    {
        case ra::ui::viewmodels::AssetState::Inactive:
            return L"Inactive";
        case ra::ui::viewmodels::AssetState::Active:
            return L"Active";
        case ra::ui::viewmodels::AssetState::Triggered:
            return L"Triggered";
        case ra::ui::viewmodels::AssetState::Waiting:
            return L"Waiting";
        case ra::ui::viewmodels::AssetState::Paused:
            return L"Paused";
        default:
            return std::to_wstring(static_cast<int>(nAssetState));
    }
}

template<>
std::wstring ToString<ra::ui::viewmodels::AssetChanges>(
    const ra::ui::viewmodels::AssetChanges& nAssetChanges)
{
    switch (nAssetChanges)
    {
        case ra::ui::viewmodels::AssetChanges::None:
            return L"None";
        case ra::ui::viewmodels::AssetChanges::Modified:
            return L"Modified";
        case ra::ui::viewmodels::AssetChanges::Unpublished:
            return L"Unpublished";
        default:
            return std::to_wstring(static_cast<int>(nAssetChanges));
    }
}

} // namespace CppUnitTestFramework
} // namespace VisualStudio
} // namespace Microsoft

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(AssetViewModelBase_Tests)
{
private:
    class AssetViewModelHarness : public AssetViewModelBase
    {
    public:
        AssetViewModelHarness() noexcept
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
    };

    class AssetDefinitionViewModelHarness : public AssetViewModelBase
    {
    public:
        AssetDefinitionViewModelHarness() noexcept
        {
            GSL_SUPPRESS_F6 AddAssetDefinition(m_pDefinition, DefinitionProperty);
        }

        const std::string& GetDefinition() const { return GetAssetDefinition(m_pDefinition); }
        void SetDefinition(const std::string& sValue) { SetAssetDefinition(m_pDefinition, sValue); }

        void Serialize(ra::services::TextWriter&) const noexcept override {}

    protected:
        static const IntModelProperty DefinitionProperty;
        AssetDefinition m_pDefinition;
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        AssetViewModelHarness asset;

        Assert::AreEqual(AssetType::Achievement, asset.GetType());
        Assert::AreEqual(0U, asset.GetID());
        Assert::AreEqual(std::wstring(L""), asset.GetName());
        Assert::AreEqual(std::wstring(L""), asset.GetDescription());
        Assert::AreEqual(AssetCategory::Core, asset.GetCategory());
        Assert::AreEqual(AssetState::Inactive, asset.GetState());
        Assert::AreEqual(AssetChanges::None, asset.GetChanges());
        Assert::IsFalse(asset.IsSelected());
    }

    TEST_METHOD(TestSerializeDefaults)
    {
        AssetViewModelHarness asset;

        asset.Serialize(asset.textWriter);
        Assert::AreEqual(std::string(":\"\"::0:\"\":"), asset.textWriter.GetString());
    }

    TEST_METHOD(TestSerializeSimpleValues)
    {
        AssetViewModelHarness asset;

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
        AssetViewModelHarness asset;

        asset.SetName(L"Mission: Impossible");
        asset.SetString(L"Mission: Impossible");
        asset.Utf8QuotedString = "Mission: Impossible";
        asset.Utf8String = "Mission: Impossible";

        asset.Serialize(asset.textWriter);
        Assert::AreEqual(std::string(":\"Mission: Impossible\":\"Mission: Impossible\":0:\"Mission: Impossible\":\"Mission: Impossible\""), asset.textWriter.GetString());
    }

    TEST_METHOD(TestSerializeQuotes)
    {
        AssetViewModelHarness asset;

        asset.SetName(L"A \"Test\" B");
        asset.SetString(L"A \"Test\" B");
        asset.Utf8QuotedString = "A \"Test\" B";
        asset.Utf8String = "A \"Test\" B";

        asset.Serialize(asset.textWriter);
        Assert::AreEqual(std::string(":\"A \\\"Test\\\" B\":\"A \\\"Test\\\" B\":0:\"A \\\"Test\\\" B\":\"A \\\"Test\\\" B\""), asset.textWriter.GetString());
    }

    TEST_METHOD(TestSerializeBackslash)
    {
        AssetViewModelHarness asset;

        asset.SetName(L"A\\B");
        asset.SetString(L"A\\B");
        asset.Utf8QuotedString = "A\\B";
        asset.Utf8String = "A\\B";

        asset.Serialize(asset.textWriter);
        Assert::AreEqual(std::string(":\"A\\\\B\":\"A\\\\B\":0:\"A\\\\B\":\"A\\\\B\""), asset.textWriter.GetString());
    }

    TEST_METHOD(TestUpdateCheckpoint)
    {
        AssetViewModelHarness asset;
        asset.SetName(L"ServerName");
        asset.SetCategory(ra::ui::viewmodels::AssetCategory::Core);
        asset.CreateServerCheckpoint();
        asset.CreateLocalCheckpoint();

        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::None, asset.GetChanges());

        asset.SetName(L"LocalName");
        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::Modified, asset.GetChanges());

        asset.UpdateLocalCheckpoint();
        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::Unpublished, asset.GetChanges());
        Assert::AreEqual(std::wstring(L"LocalName"), asset.GetName());

        asset.UpdateServerCheckpoint();
        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::None, asset.GetChanges());
        Assert::AreEqual(std::wstring(L"LocalName"), asset.GetName());
    }

    TEST_METHOD(TestRestoreCheckpoint)
    {
        AssetViewModelHarness asset;
        asset.SetName(L"ServerName");
        asset.SetCategory(ra::ui::viewmodels::AssetCategory::Core);
        asset.CreateServerCheckpoint();
        asset.SetName(L"LocalName");
        asset.CreateLocalCheckpoint();

        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::Unpublished, asset.GetChanges());

        asset.SetName(L"ModifiedName");
        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::Modified, asset.GetChanges());

        asset.RestoreLocalCheckpoint();
        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::Unpublished, asset.GetChanges());
        Assert::AreEqual(std::wstring(L"LocalName"), asset.GetName());

        asset.SetName(L"ModifiedName");
        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::Modified, asset.GetChanges());

        asset.RestoreServerCheckpoint();
        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::None, asset.GetChanges());
        Assert::AreEqual(std::wstring(L"ServerName"), asset.GetName());
    }

    TEST_METHOD(TestUpdateDefinitionCheckpoint)
    {
        AssetDefinitionViewModelHarness asset;
        asset.SetDefinition("ServerDefinition");
        asset.CreateServerCheckpoint();
        asset.CreateLocalCheckpoint();

        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::None, asset.GetChanges());
        Assert::AreEqual(std::string("ServerDefinition"), asset.GetDefinition());

        asset.SetDefinition("LocalDefinition");
        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::Modified, asset.GetChanges());
        Assert::AreEqual(std::string("LocalDefinition"), asset.GetDefinition());

        asset.UpdateLocalCheckpoint();
        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::Unpublished, asset.GetChanges());
        Assert::AreEqual(std::string("LocalDefinition"), asset.GetDefinition());

        asset.UpdateServerCheckpoint();
        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::None, asset.GetChanges());
        Assert::AreEqual(std::string("LocalDefinition"), asset.GetDefinition());
    }

    TEST_METHOD(TestRestoreDefinitionCheckpoint)
    {
        AssetDefinitionViewModelHarness asset;
        asset.SetDefinition("ServerDefinition");
        asset.CreateServerCheckpoint();
        asset.SetDefinition("LocalDefinition");
        asset.CreateLocalCheckpoint();

        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::Unpublished, asset.GetChanges());
        Assert::AreEqual(std::string("LocalDefinition"), asset.GetDefinition());

        asset.SetDefinition("ModifiedDefinition");
        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::Modified, asset.GetChanges());
        Assert::AreEqual(std::string("ModifiedDefinition"), asset.GetDefinition());

        asset.RestoreLocalCheckpoint();
        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::Unpublished, asset.GetChanges());
        Assert::AreEqual(std::string("LocalDefinition"), asset.GetDefinition());

        asset.SetDefinition("ModifiedDefinition");
        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::Modified, asset.GetChanges());
        Assert::AreEqual(std::string("ModifiedDefinition"), asset.GetDefinition());

        asset.RestoreServerCheckpoint();
        Assert::AreEqual(ra::ui::viewmodels::AssetChanges::None, asset.GetChanges());
        Assert::AreEqual(std::string("ServerDefinition"), asset.GetDefinition());
    }
};

const StringModelProperty AssetViewModelBase_Tests::AssetViewModelHarness::StringProperty("AssetViewModelHarness", "String", L"");
const IntModelProperty AssetViewModelBase_Tests::AssetViewModelHarness::IntProperty("AssetViewModelHarness", "Int", 0);
const IntModelProperty AssetViewModelBase_Tests::AssetDefinitionViewModelHarness::DefinitionProperty("AssetDefinitionViewModelHarness", "Int", ra::etoi(AssetChanges::None));

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
