#include "services\impl\FileLocalStorage.hh"

#include "tests\mocks\MockClock.hh"
#include "tests\mocks\MockFileSystem.hh"
#include "tests\RA_UnitTestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using ra::services::mocks::MockClock;
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

private:
    static void SetupExpiration(MockFileSystem& mockFileSystem, const std::wstring& sFile, std::chrono::system_clock::time_point tExpire)
    {
        mockFileSystem.MockFile(sFile, "No data");
        mockFileSystem.MockLastModified(sFile, tExpire);
    }

public:
    TEST_METHOD(TestExpiration)
    {
        MockClock mockClock;
        const auto tNotExpire = mockClock.Now() - std::chrono::hours(24 * 30 - 1);
        const auto tExpire = mockClock.Now() - std::chrono::hours(24 * 30 + 1);

        MockFileSystem mockFileSystem;
        mockFileSystem.CreateDirectory(L".\\RACache\\Badge\\");
        SetupExpiration(mockFileSystem, L".\\RACache\\Badge\\00000.png", tExpire);
        SetupExpiration(mockFileSystem, L".\\RACache\\Badge\\00001.png", tExpire);
        SetupExpiration(mockFileSystem, L".\\RACache\\Badge\\00002.png", tNotExpire);
        SetupExpiration(mockFileSystem, L".\\RACache\\Badge\\00003.png", tNotExpire);

        mockFileSystem.CreateDirectory(L".\\RACache\\UserPic\\");
        SetupExpiration(mockFileSystem, L".\\RACache\\UserPic\\00000.png", tExpire);
        SetupExpiration(mockFileSystem, L".\\RACache\\UserPic\\00001.png", tExpire);
        SetupExpiration(mockFileSystem, L".\\RACache\\UserPic\\00002.png", tNotExpire);
        SetupExpiration(mockFileSystem, L".\\RACache\\UserPic\\00003.png", tNotExpire);

        mockFileSystem.CreateDirectory(L".\\RACache\\Data\\");
        SetupExpiration(mockFileSystem, L".\\RACache\\Data\\123.json", tExpire);
        SetupExpiration(mockFileSystem, L".\\RACache\\Data\\123-Rich.txt", tExpire);
        SetupExpiration(mockFileSystem, L".\\RACache\\Data\\123-Notes.json", tExpire);
        SetupExpiration(mockFileSystem, L".\\RACache\\Data\\123-User.txt", tExpire);
        SetupExpiration(mockFileSystem, L".\\RACache\\Data\\456.json", tNotExpire);
        SetupExpiration(mockFileSystem, L".\\RACache\\Data\\456-Rich.txt", tNotExpire);
        SetupExpiration(mockFileSystem, L".\\RACache\\Data\\456-Notes.json", tNotExpire);
        SetupExpiration(mockFileSystem, L".\\RACache\\Data\\456-User.txt", tNotExpire);

        mockFileSystem.CreateDirectory(L".\\RACache\\Bookmarks\\");
        SetupExpiration(mockFileSystem, L".\\RACache\\Bookmarks\\123-Bookmarks.txt", tExpire);
        SetupExpiration(mockFileSystem, L".\\RACache\\Bookmarks\\456-Bookmarks.txt", tNotExpire);

        mockFileSystem.CreateDirectory(L".\\RACache\\");
        SetupExpiration(mockFileSystem, L".\\RACache\\User-history.txt", tExpire);
        SetupExpiration(mockFileSystem, L".\\RACache\\User2-history.txt", tNotExpire);

        FileLocalStorage storage(mockFileSystem);

        std::vector<std::wstring> vFiles;
        Assert::AreEqual(2U, mockFileSystem.GetFilesInDirectory(L".\\RACache\\Badge\\", vFiles));
        Assert::IsTrue(std::find(vFiles.begin(), vFiles.end(), L"00002.png") != vFiles.end());
        Assert::IsTrue(std::find(vFiles.begin(), vFiles.end(), L"00003.png") != vFiles.end());

        vFiles.clear();
        Assert::AreEqual(2U, mockFileSystem.GetFilesInDirectory(L".\\RACache\\UserPic\\", vFiles));
        Assert::IsTrue(std::find(vFiles.begin(), vFiles.end(), L"00002.png") != vFiles.end());
        Assert::IsTrue(std::find(vFiles.begin(), vFiles.end(), L"00003.png") != vFiles.end());

        vFiles.clear();
        Assert::AreEqual(5U, mockFileSystem.GetFilesInDirectory(L".\\RACache\\Data\\", vFiles));
        Assert::IsTrue(std::find(vFiles.begin(), vFiles.end(), L"123-User.txt") != vFiles.end()); // user file should not be deleted, regardless of age
        Assert::IsTrue(std::find(vFiles.begin(), vFiles.end(), L"456.json") != vFiles.end());
        Assert::IsTrue(std::find(vFiles.begin(), vFiles.end(), L"456-Rich.txt") != vFiles.end());
        Assert::IsTrue(std::find(vFiles.begin(), vFiles.end(), L"456-Notes.json") != vFiles.end());
        Assert::IsTrue(std::find(vFiles.begin(), vFiles.end(), L"456-User.txt") != vFiles.end());

        // bookmarks should never be deleted
        vFiles.clear();
        Assert::AreEqual(2U, mockFileSystem.GetFilesInDirectory(L".\\RACache\\Bookmarks\\", vFiles));
        Assert::IsTrue(std::find(vFiles.begin(), vFiles.end(), L"123-Bookmarks.txt") != vFiles.end());
        Assert::IsTrue(std::find(vFiles.begin(), vFiles.end(), L"456-Bookmarks.txt") != vFiles.end());

        // files in the cache root should never be deleted
        vFiles.clear();
        Assert::AreEqual(2U, mockFileSystem.GetFilesInDirectory(L".\\RACache\\", vFiles));
        Assert::IsTrue(std::find(vFiles.begin(), vFiles.end(), L"User-history.txt") != vFiles.end());
        Assert::IsTrue(std::find(vFiles.begin(), vFiles.end(), L"User2-history.txt") != vFiles.end());
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
