#include "services\impl\FileLocalStorage.hh"

#include "tests\mocks\MockFileSystem.hh"
#include "tests\RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::services::mocks::MockFileSystem;

namespace ra {
namespace services {
namespace impl {
namespace tests {

TEST_CLASS(FileLocalStorage_Tests)
{
public:
    TEST_METHOD(TestDirectories)
    {
        MockFileSystem mockFileSystem;
        FileLocalStorage storage(mockFileSystem);
        Assert::IsTrue(mockFileSystem.DirectoryExists(L".\\RACache\\"));
        Assert::IsTrue(mockFileSystem.DirectoryExists(L".\\RACache\\Data\\"));
        Assert::IsTrue(mockFileSystem.DirectoryExists(L".\\RACache\\Badge\\"));
        Assert::IsTrue(mockFileSystem.DirectoryExists(L".\\RACache\\UserPic\\"));
        Assert::IsTrue(mockFileSystem.DirectoryExists(L".\\RACache\\Bookmarks\\"));
    }

    TEST_METHOD(TestFileNames)
    {
        MockFileSystem mockFileSystem;
        FileLocalStorage storage(mockFileSystem);
        Assert::AreEqual(storage.GetPath(ra::services::StorageItemType::GameData, L"12345"), std::wstring(L".\\RACache\\Data\\12345.json"));
        Assert::AreEqual(storage.GetPath(ra::services::StorageItemType::CodeNotes, L"12345"), std::wstring(L".\\RACache\\Data\\12345-Notes.json"));
        Assert::AreEqual(storage.GetPath(ra::services::StorageItemType::RichPresence, L"12345"), std::wstring(L".\\RACache\\Data\\12345-Rich.txt"));
        Assert::AreEqual(storage.GetPath(ra::services::StorageItemType::UserAchievements, L"12345"), std::wstring(L".\\RACache\\Data\\12345-User.txt"));
        Assert::AreEqual(storage.GetPath(ra::services::StorageItemType::Badge, L"12345"), std::wstring(L".\\RACache\\Badge\\12345.png"));
        Assert::AreEqual(storage.GetPath(ra::services::StorageItemType::UserPic, L"12345"), std::wstring(L".\\RACache\\UserPic\\12345.png"));
    }

    TEST_METHOD(TestReadTextNonExistant)
    {
        MockFileSystem mockFileSystem;
        FileLocalStorage storage(mockFileSystem);

        auto pData = storage.ReadText(ra::services::StorageItemType::GameData, L"12345");
        Assert::IsTrue(pData == nullptr);
    }

    TEST_METHOD(TestReadText)
    {
        MockFileSystem mockFileSystem;
        FileLocalStorage storage(mockFileSystem);

        mockFileSystem.MockFile(L".\\RACache\\Data\\12345.json", "{\"Key\": 0}");

        auto pData = storage.ReadText(ra::services::StorageItemType::GameData, L"12345");
        Assert::IsFalse(pData == nullptr);

        std::string sLine;
        Assert::IsTrue(pData->GetLine(sLine));
        Assert::AreEqual(std::string("{\"Key\": 0}"), sLine);
    }

    TEST_METHOD(TestWriteText)
    {
        MockFileSystem mockFileSystem;
        FileLocalStorage storage(mockFileSystem);

        mockFileSystem.MockFile(L".\\RACache\\Data\\12345.json", "{\"Key\": 0}");

        auto pData = storage.WriteText(ra::services::StorageItemType::GameData, L"12345");
        Assert::IsFalse(pData == nullptr);

        pData->Write("{\"Key\": 1}");
        Assert::AreEqual(std::string("{\"Key\": 1}"), mockFileSystem.GetFileContents(L".\\RACache\\Data\\12345.json"));
    }
};

} // namespace tests
} // namespace impl
} // namespace services
} // namespace ra
