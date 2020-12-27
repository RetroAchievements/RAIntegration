#include "CppUnitTest.h"

#include "data\models\LocalBadgesModel.hh"

#include "services\impl\StringTextWriter.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\data\DataAsserts.hh"

#include "tests\mocks\MockFileSystem.hh"
#include "tests\mocks\MockGameContext.hh"
#include "tests\mocks\MockImageRepository.hh"

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
        ra::ui::mocks::MockImageRepository mockImageRepository;
    };

public:
    TEST_METHOD(TestInitialValues)
    {
        LocalBadgesModelHarness badges;

        Assert::AreEqual(AssetType::LocalBadges, badges.GetType());
        Assert::AreEqual(0U, badges.GetID());
        Assert::AreEqual(std::wstring(L"Local Badges"), badges.GetName());
        Assert::AreEqual(std::wstring(L""), badges.GetDescription());
        Assert::AreEqual(AssetCategory::Core, badges.GetCategory());
        Assert::AreEqual(AssetState::Inactive, badges.GetState());
        Assert::AreEqual(AssetChanges::None, badges.GetChanges());
        Assert::IsFalse(badges.NeedsSerialized());
    }

    TEST_METHOD(TestDeleteUncommittedBadges)
    {
        LocalBadgesModelHarness badges;
        badges.mockGameContext.SetGameId(123U);
        badges.mockFileSystem.MockFileSize(L"local\\123-A.png", 1234);
        badges.AddReference(L"local\\123-A.png", true);
        badges.mockFileSystem.MockFileSize(L"local\\123-B.png", 2345);
        badges.AddReference(L"local\\123-B.png", true);
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
