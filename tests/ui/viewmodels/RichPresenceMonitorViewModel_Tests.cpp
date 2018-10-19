#include "CppUnitTest.h"

#include "ui\viewmodels\RichPresenceMonitorViewModel.hh"

#include "services\ServiceLocator.hh"

#include "ui\IDesktop.hh"

#include "tests\RA_UnitTestHelpers.h"
#include "tests\mocks\MockDesktop.hh"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ra {
namespace ui {
namespace viewmodels {
namespace tests {

TEST_CLASS(VM_RichPresenceMonitorViewModel_Tests)
{
    TEST_METHOD(TestInitialization)
    {
        RichPresenceMonitorViewModel vmRichPresence;
        Assert::AreEqual(std::wstring(L"Rich Presence Monitor"), vmRichPresence.GetWindowTitle());
        Assert::AreEqual(std::wstring(L"No Rich Presence defined."), vmRichPresence.GetDisplayString());
    }

    TEST_METHOD(TestUpdateDisplayStringNoGame)
    {
        //TODO: MockGameContext mockGameContext;
        RichPresenceMonitorViewModel vmRichPresence;       
    }

    TEST_METHOD(TestStartMonitoringNoGame)
    {
        //TODO: MockGameContext mockGameContext;
        //TODO: MockThreadPool mockThreadPool;
        RichPresenceMonitorViewModel vmRichPresence;
    }
};

} // namespace tests
} // namespace viewmodels
} // namespace ui
} // namespace ra
