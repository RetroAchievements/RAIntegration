#include "testutil/CppUnitTest.hh"

#include "services/ServiceLocator.hh"

#include "util/Strings.hh"

static void TestNoServiceHandler(const std::string& sMessage)
{
    Assert::Fail(ra::util::String::Widen(sMessage).c_str());
}

TEST_MODULE_INITIALIZE(GlobalSetup)
{
    ra::services::ServiceLocator::SetNoServiceHandler(TestNoServiceHandler);
}
