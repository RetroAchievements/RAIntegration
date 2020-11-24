#include "CppUnitTest.h"

#include "data\models\LocalBadgesModel.hh"

#include "services\impl\StringTextWriter.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"

#include "tests\mocks\MockFileSystem.hh"
#include "tests\mocks\MockGameContext.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace data {
namespace models {
namespace tests {

TEST_CLASS(LocalBadgesModel_Tests)
{
private:
    class LocalBadgesModelHarness : public LocalBadgesModel
    {
    public:
        ra::services::impl::StringTextWriter textWriter;

        ra::data::context::mocks::MockGameContext mockGameContext;
        ra::services::mocks::MockFileSystem mockFileSystem;
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        LocalBadgesModelHarness badges;

        Assert::AreEqual(AssetType::LocalBadges, badges.GetType());
        Assert::AreEqual(0U, badges.GetID());
        Assert::AreEqual(std::wstring(L""), badges.GetName());
        Assert::AreEqual(std::wstring(L""), badges.GetDescription());
        Assert::AreEqual(AssetCategory::Core, badges.GetCategory());
        Assert::AreEqual(AssetState::Inactive, badges.GetState());
        Assert::AreEqual(AssetChanges::None, badges.GetChanges());
        Assert::IsFalse(badges.NeedsSerialized());
    }

    TEST_METHOD(TestSerializeSingle)
    {
        LocalBadgesModelHarness badges;

        badges.AddReference(L"local\\123-A.png", false);
        Assert::IsFalse(badges.NeedsSerialized());
        badges.Serialize(badges.textWriter);
        Assert::AreEqual(std::string("b"), badges.textWriter.GetString());
        badges.textWriter.GetString().clear();

        badges.Commit(L"", L"local\\123-A.png");
        Assert::IsTrue(badges.NeedsSerialized());
        badges.Serialize(badges.textWriter);
        Assert::AreEqual(std::string("b:A.png=1"), badges.textWriter.GetString());
    }

    TEST_METHOD(TestSerializeSingleMultipleReference)
    {
        LocalBadgesModelHarness badges;

        badges.AddReference(L"local\\123-A.png", false);
        badges.AddReference(L"local\\123-A.png", false);
        badges.AddReference(L"local\\123-A.png", false);
        Assert::IsFalse(badges.NeedsSerialized());
        badges.Serialize(badges.textWriter);
        Assert::AreEqual(std::string("b"), badges.textWriter.GetString());
        badges.textWriter.GetString().clear();

        badges.Commit(L"", L"local\\123-A.png");
        badges.Commit(L"12345", L"local\\123-A.png");
        badges.Commit(L"22222", L"local\\123-A.png");
        Assert::IsTrue(badges.NeedsSerialized());
        badges.Serialize(badges.textWriter);
        Assert::AreEqual(std::string("b:A.png=3"), badges.textWriter.GetString());
    }

    TEST_METHOD(TestSerializeSinglePartialMultipleReference)
    {
        LocalBadgesModelHarness badges;

        badges.AddReference(L"local\\123-A.png", false);
        badges.AddReference(L"local\\123-A.png", false);
        badges.AddReference(L"local\\123-A.png", false);
        Assert::IsFalse(badges.NeedsSerialized());
        badges.Serialize(badges.textWriter);
        Assert::AreEqual(std::string("b"), badges.textWriter.GetString());
        badges.textWriter.GetString().clear();

        badges.Commit(L"", L"local\\123-A.png");
        badges.Commit(L"12345", L"local\\123-A.png");
        Assert::IsTrue(badges.NeedsSerialized());
        badges.Serialize(badges.textWriter);
        Assert::AreEqual(std::string("b:A.png=2"), badges.textWriter.GetString());
    }

    TEST_METHOD(TestSerializeMultiple)
    {
        LocalBadgesModelHarness badges;

        badges.AddReference(L"local\\123-A.png", false);
        badges.AddReference(L"local\\123-B.png", false);
        badges.AddReference(L"local\\123-C.png", false);
        Assert::IsFalse(badges.NeedsSerialized());
        badges.Serialize(badges.textWriter);
        Assert::AreEqual(std::string("b"), badges.textWriter.GetString());
        badges.textWriter.GetString().clear();

        badges.Commit(L"", L"local\\123-A.png");
        badges.Commit(L"12345", L"local\\123-B.png");
        badges.Commit(L"22222", L"local\\123-C.png");
        Assert::IsTrue(badges.NeedsSerialized());
        badges.Serialize(badges.textWriter);
        Assert::AreEqual(std::string("b:A.png=1:B.png=1:C.png=1"), badges.textWriter.GetString());
    }

    TEST_METHOD(TestDeserialize)
    {
        LocalBadgesModelHarness badges;
        std::string sInput = "b:A.png=1:B.png=1";
        ra::Tokenizer pTokenizer(sInput);
        badges.mockGameContext.SetGameId(123U);

        pTokenizer.Advance(2); // GameAssets reads "b:"
        Assert::IsTrue(badges.Deserialize(pTokenizer));
        Assert::AreEqual(1, badges.GetReferenceCount(L"local\\123-A.png"));
        Assert::AreEqual(1, badges.GetReferenceCount(L"local\\123-B.png"));

        Assert::IsTrue(badges.NeedsSerialized());
        badges.Serialize(badges.textWriter);
        Assert::AreEqual(std::string("b:A.png=1:B.png=1"), badges.textWriter.GetString());
    }

    TEST_METHOD(TestDeserializeChanged)
    {
        LocalBadgesModelHarness badges;
        std::string sInput = "b:A.png=1:B.png=1";
        ra::Tokenizer pTokenizer(sInput);
        badges.mockGameContext.SetGameId(123U);
        badges.mockFileSystem.MockFileSize(L"local\\123-A.png", 1234);
        badges.mockFileSystem.MockFileSize(L"local\\123-B.png", 2345);

        pTokenizer.Advance(2); // GameAssets reads "b:"
        Assert::IsTrue(badges.Deserialize(pTokenizer));

        badges.RemoveReference(L"local\\123-A.png", false);
        badges.AddReference(L"local\\123-C.png", false);
        badges.Commit(L"local\\123-A.png", L"local\\123-C.png");

        Assert::IsTrue(badges.NeedsSerialized());
        badges.Serialize(badges.textWriter);
        Assert::AreEqual(std::string("b:B.png=1:C.png=1"), badges.textWriter.GetString());

        Assert::AreEqual({ -1 }, badges.mockFileSystem.GetFileSize(L"local\\123-A.png"));
        Assert::AreEqual({ 2345 }, badges.mockFileSystem.GetFileSize(L"local\\123-B.png"));
    }

    TEST_METHOD(TestDeserializePublished)
    {
        LocalBadgesModelHarness badges;
        std::string sInput = "b:A.png=1:B.png=1";
        ra::Tokenizer pTokenizer(sInput);
        badges.mockGameContext.SetGameId(123U);
        badges.mockFileSystem.MockFileSize(L"local\\123-A.png", 1234);
        badges.mockFileSystem.MockFileSize(L"local\\123-B.png", 2345);

        pTokenizer.Advance(2); // GameAssets reads "b:"
        Assert::IsTrue(badges.Deserialize(pTokenizer));

        badges.RemoveReference(L"local\\123-A.png", false);
        badges.Commit(L"local\\123-A.png", L"22222");

        Assert::IsTrue(badges.NeedsSerialized());
        badges.Serialize(badges.textWriter);
        Assert::AreEqual(std::string("b:B.png=1"), badges.textWriter.GetString());

        // combination of RemoveReference and Commit should have deleted 123-A.png
        Assert::AreEqual({ -1 }, badges.mockFileSystem.GetFileSize(L"local\\123-A.png"));
        Assert::AreEqual({ 2345 }, badges.mockFileSystem.GetFileSize(L"local\\123-B.png"));
    }

    TEST_METHOD(TestDeserializeChangedStillReferenced)
    {
        LocalBadgesModelHarness badges;
        std::string sInput = "b:A.png=2:B.png=1";
        ra::Tokenizer pTokenizer(sInput);
        badges.mockGameContext.SetGameId(123U);
        badges.mockFileSystem.MockFileSize(L"local\\123-A.png", 1234);
        badges.mockFileSystem.MockFileSize(L"local\\123-B.png", 2345);

        pTokenizer.Advance(2); // GameAssets reads "b:"
        Assert::IsTrue(badges.Deserialize(pTokenizer));

        badges.RemoveReference(L"local\\123-A.png", false);
        badges.AddReference(L"local\\123-C.png", false);
        badges.Commit(L"local\\123-A.png", L"local\\123-C.png");

        Assert::IsTrue(badges.NeedsSerialized());
        badges.Serialize(badges.textWriter);
        Assert::AreEqual(std::string("b:A.png=1:B.png=1:C.png=1"), badges.textWriter.GetString());

        Assert::AreEqual({ 1234 }, badges.mockFileSystem.GetFileSize(L"local\\123-A.png"));
        Assert::AreEqual({ 2345 }, badges.mockFileSystem.GetFileSize(L"local\\123-B.png"));
    }

    TEST_METHOD(TestDeserializeChangedNotCommitted)
    {
        LocalBadgesModelHarness badges;
        std::string sInput = "b:A.png=1:B.png=1";
        ra::Tokenizer pTokenizer(sInput);
        badges.mockGameContext.SetGameId(123U);
        badges.mockFileSystem.MockFileSize(L"local\\123-A.png", 1234);
        badges.mockFileSystem.MockFileSize(L"local\\123-B.png", 2345);

        pTokenizer.Advance(2); // GameAssets reads "b:"
        Assert::IsTrue(badges.Deserialize(pTokenizer));

        badges.RemoveReference(L"local\\123-A.png", false);
        badges.AddReference(L"local\\123-C.png", false);

        Assert::IsTrue(badges.NeedsSerialized());
        badges.Serialize(badges.textWriter);
        Assert::AreEqual(std::string("b:A.png=1:B.png=1"), badges.textWriter.GetString());

        Assert::AreEqual({ 1234 }, badges.mockFileSystem.GetFileSize(L"local\\123-A.png"));
        Assert::AreEqual({ 2345 }, badges.mockFileSystem.GetFileSize(L"local\\123-B.png"));
    }

    TEST_METHOD(TestDeleteUncommittedBadges)
    {
        LocalBadgesModelHarness badges;
        std::string sInput = "b:A.png=1:B.png=1";
        ra::Tokenizer pTokenizer(sInput);
        badges.mockGameContext.SetGameId(123U);
        badges.mockFileSystem.MockFileSize(L"local\\123-A.png", 1234);
        badges.mockFileSystem.MockFileSize(L"local\\123-B.png", 2345);

        pTokenizer.Advance(2); // GameAssets reads "b:"
        Assert::IsTrue(badges.Deserialize(pTokenizer));

        badges.mockFileSystem.MockFileSize(L"local\\123-C.png", 3456);
        badges.AddReference(L"local\\123-C.png", false);
        Assert::AreEqual({ 3456 }, badges.mockFileSystem.GetFileSize(L"local\\123-C.png"));

        badges.DeleteUncommittedBadges();

        Assert::AreEqual({ 1234 }, badges.mockFileSystem.GetFileSize(L"local\\123-A.png"));
        Assert::AreEqual({ 2345 }, badges.mockFileSystem.GetFileSize(L"local\\123-B.png"));
        Assert::AreEqual({ -1 }, badges.mockFileSystem.GetFileSize(L"local\\123-C.png"));
    }
};


} // namespace tests
} // namespace models
} // namespace data
} // namespace ra
