#include "devkit/testutil/CppUnitTest.hh"

#include "services/ServiceLocator.hh"

#include "util/Strings.hh"

static void TestNoServiceHandler(const std::string& sMessage)
{
    Assert::Fail(ra::util::String::Widen(sMessage).c_str());
}

#pragma warning(push)
#pragma warning(disable : 26440) // Function can be declared 'noexcept' (f.6)

TEST_MODULE_INITIALIZE(GlobalSetup)
{
    ra::services::ServiceLocator::SetNoServiceHandler(TestNoServiceHandler);
}

#pragma warning(pop)
